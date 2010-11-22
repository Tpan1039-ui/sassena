/*
 *  This file is part of the software sassena
 *
 *  Authors:
 *  Benjamin Lindner, ben@benlabs.net
 *
 *  Copyright 2008-2010 Benjamin Lindner
 *
 */

// direct header
#include "scatter_devices/abstract_scatter_device.hpp"

// standard header
#include <complex>
#include <fstream>

// special library headers
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/lexical_cast.hpp>

// other headers
#include "math/coor3d.hpp"
#include "decomposition/assignment.hpp"
#include "decomposition/decompose.hpp"
#include <fftw3.h>
#include "control.hpp"
#include "log.hpp"
#include "sample.hpp"
#include "scatter_devices/data_stager.hpp"

using namespace std;

AbstractScatterDevice::AbstractScatterDevice(
    boost::mpi::communicator allcomm,
    boost::mpi::communicator partitioncomm,
    Sample& sample,
    std::vector<CartesianCoor3D> vectors,
    size_t NAF,
    boost::asio::ip::tcp::endpoint fileservice_endpoint,
	boost::asio::ip::tcp::endpoint monitorservice_endpoint
) :
    allcomm_(allcomm),
    partitioncomm_(partitioncomm),
    sample_(sample),
    vectors_(vectors),
    current_vector_(0),
    atfinal_(NULL),
    assignment_(partitioncomm.size(),partitioncomm.rank(),NAF)
{
    
    p_hdf5writer_ = boost::shared_ptr<HDF5WriterClient>(new HDF5WriterClient(fileservice_endpoint));
    p_monitor_ = boost::shared_ptr<MonitorClient>(new MonitorClient(monitorservice_endpoint));
    	
    NN = partitioncomm_.size();
	NF = sample_.coordinate_sets.size();
	std::string target = Params::Inst()->scattering.target;
	NA = sample_.atoms.selections[target]->size(); // Number of Atoms
	
	sample_.coordinate_sets.set_selection(sample.atoms.selections[target]);
	
	scatterfactors.set_sample(sample_);
	scatterfactors.set_selection(sample_.atoms.selections[target]);
	scatterfactors.set_background(true);
	
	// defer memory allocation for scattering data
    atfinal_ = NULL;
    // atfinal_.resize(NF);
    // but check limits nevertheless:
    size_t scattering_databytesize = 3*NF*sizeof(fftw_complex); // peak consumption: 2*NF (padded signal) + 1*NF (integral)
    if (Params::Inst()->limits.computation.memory.signal<scattering_databytesize) {
        if (allcomm_.rank()==0) {
            Err::Inst()->write("Insufficient Buffer size for scattering (limits.computation.memory.signal)");
            Err::Inst()->write(string("Requested (bytes): ")+boost::lexical_cast<string>(scattering_databytesize));
        }
        throw;
    }
}

AbstractScatterDevice::~AbstractScatterDevice() {
    if (atfinal_!=NULL) {
        fftw_free(atfinal_);
        atfinal_=NULL;
    }
}

void AbstractScatterDevice::run() {
        
    print_pre_stage_info();
    timer.start("sd:stage");
    stage_data();
    timer.stop("sd:stage");
    
    print_post_stage_info();

    if (allcomm_.rank()==0) {
        scatterfactors.update(CartesianCoor3D(0,0,0));
        Info::Inst()->write("Target initialized. ");
        Info::Inst()->write(string("Target produces a background scattering length density of ")+boost::lexical_cast<string>(scatterfactors.compute_background(CartesianCoor3D(0,0,0))));
    }

    // allocate memory for computation now.
    atfinal_ = (fftw_complex*) fftw_malloc(NF*sizeof(fftw_complex));
    memset(atfinal_,0,NF*sizeof(fftw_complex));

    print_pre_runner_info();
    timer.start("sd:runner");
    runner();
    timer.stop("sd:runner");
    print_post_runner_info();

    // notify the services to finalize
    p_monitor_->update(allcomm_.rank(),1.0);
    p_hdf5writer_->flush();        
}


void AbstractScatterDevice::runner() {
 
    bool threads_on = Params::Inst()->limits.computation.threads.on;
        
    if (threads_on) {
         start_workers();
         while(status()==0) {
            
            timer.start("sd:compute");
         	compute_threaded();
            timer.stop("sd:compute"); 
            
            timer.start("sd:write");        	
     		write();
     		timer.stop("sd:write");
            		    
            next();
         }             
         stop_workers();
    } else {
        while(status()==0) {
            
            timer.start("sd:compute");        	
            compute_serial();
            timer.stop("sd:compute");        	
            
            
            timer.start("sd:write");        	
     		write();
     		timer.stop("sd:write");
     				    
            next();
        }                     
    }
}

void AbstractScatterDevice::next() {
	if (current_vector_>=vectors_.size()) return;
	current_vector_++;    
}

double AbstractScatterDevice::progress() {
    double scale = 1.0/vectors_.size();
    double base =  current_vector_*scale;
    return base;    
}

size_t AbstractScatterDevice::status() {
    if (current_vector_==vectors_.size()) return 1; else return 0;
}

void AbstractScatterDevice::write() {
    if (partitioncomm_.rank()==0) {
        CartesianCoor3D vector = vectors_[current_vector_]; 
        p_hdf5writer_->write(vector,atfinal_,NF);
    }
}

// end of file