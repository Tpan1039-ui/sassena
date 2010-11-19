/*
 *  This file is part of the software sassena
 *
 *  Authors:
 *  Benjamin Lindner, ben@benlabs.net
 *
 *  Copyright 2008-2010 Benjamin Lindner
 *
 */

#ifndef SCATTER_DEVICES__ALL_VECTORS_SCATTER_DEVICE_HPP_
#define SCATTER_DEVICES__ALL_VECTORS_SCATTER_DEVICE_HPP_

// common header
#include "common.hpp"

// standard header
#include <complex>
#include <map>
#include <string>
#include <sys/time.h>
#include <vector>
#include <queue>

// special library headers
#include <boost/asio.hpp>
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/mpi.hpp>
#include <boost/thread.hpp>
#include <fftw3.h>

// other headers
#include "sample.hpp"
#include "math/coor3d.hpp"
#include "report/timer.hpp"

#include "scatter_devices/abstract_vectors_scatter_device.hpp"

class AllVectorsScatterDevice : public AbstractVectorsScatterDevice {
protected:

    // first = q, second = frames
    concurrent_queue< size_t > at0_;    
    concurrent_queue< std::pair<size_t,fftw_complex*> > at1_;
    concurrent_queue< fftw_complex* > at2_;
    mutable fftw_complex* at3_;
    mutable boost::mutex at3_mutex;
	std::queue<boost::thread*> worker_threads;
    
	// data, outer loop by frame, inner by atoms, XYZ entries
    coor_t* p_coordinates;
    
	fftw_complex* scatter(size_t this_subvector);
    
    void stage_data();
        
    void worker1();
    void worker2();
    void worker3();

    void worker1_task(size_t this_subvector);
    void worker2_task(size_t this_subvector,fftw_complex* p_a);
    void worker3_task(fftw_complex* p_a);
    
    volatile size_t worker2_counter;
    mutable boost::mutex worker3_mutex;
    mutable boost::mutex worker2_mutex;    
    volatile bool worker2_done;
    volatile bool worker3_done;
    boost::condition_variable worker3_notifier;
    boost::condition_variable worker2_notifier;
    
	void compute_serial();
	void compute_threaded();	
	void start_workers();
    void stop_workers();


    ~AllVectorsScatterDevice();
    fftw_plan fftw_planF_;
    fftw_plan fftw_planB_;
public: 
    AllVectorsScatterDevice(
			boost::mpi::communicator allcomm,
			boost::mpi::communicator partitioncomm,
			Sample& sample,
			std::vector<CartesianCoor3D> vectors,
			size_t NAF,
			boost::asio::ip::tcp::endpoint fileservice_endpoint,
			boost::asio::ip::tcp::endpoint monitorservice_endpoint			

	);
};


#endif

//end of file
