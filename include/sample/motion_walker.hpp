/*
 *  motion_walker.hpp
 *
 *  Created on: May 26, 2008
 *  Authors:
 *  Benjamin Lindner, ben@benlabs.net
 *
 *  Copyright 2008,2009 Benjamin Lindner
 *
 */

#ifndef MOTION_WALKER_HPP_
#define MOTION_WALKER_HPP_

// common header
#include "common.hpp"

// standard header
#include <complex>
#include <map>
#include <string>
#include <sys/time.h>
#include <vector>

// special library headers
#include <boost/serialization/access.hpp>
#include <boost/serialization/map.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>
// special library headers
#include <boost/accumulators/accumulators.hpp>
#include <boost/accumulators/statistics.hpp>

#include <boost/random/mersenne_twister.hpp>	
#include <boost/random/normal_distribution.hpp>	
#include <boost/random/uniform_on_sphere.hpp>	
#include <boost/random/variate_generator.hpp>

// other headers
#include "math/coor3d.hpp"

class MotionWalker {
protected:
    friend class boost::serialization::access;	
	template<class Archive> void serialize(Archive & ar, const unsigned int version)
    {
        
    }

public:
	
	virtual CartesianCoor3D translation(size_t timepos) = 0;	
};

//

class LinearMotionWalker : public MotionWalker {
protected:
    friend class boost::serialization::access;	
	template<class Archive> void serialize(Archive & ar, const unsigned int version)
    {
        ar & boost::serialization::base_object<MotionWalker, LinearMotionWalker>(*this);
        ar & m_translate;
    }    
	CartesianCoor3D m_translate;
	
	void generate(size_t timepos);
    LinearMotionWalker() {}

public:
	LinearMotionWalker(double displace,long sampling, CartesianCoor3D direction);
	
	CartesianCoor3D translation(size_t timepos);
};

class FixedMotionWalker : public MotionWalker {
protected:
    friend class boost::serialization::access;	
	template<class Archive> void serialize(Archive & ar, const unsigned int version)
    {
        ar & boost::serialization::base_object<MotionWalker, FixedMotionWalker>(*this);
        ar & m_translate;        
    }    
    
	CartesianCoor3D m_translate;
    FixedMotionWalker() {}	
    
public:
	FixedMotionWalker(double displace,CartesianCoor3D direction);
	
	CartesianCoor3D translation(size_t timepos);
};

class OscillationMotionWalker : public MotionWalker {
protected:
    friend class boost::serialization::access;	
	template<class Archive> void serialize(Archive & ar, const unsigned int version)
    {
        ar & boost::serialization::base_object<MotionWalker, OscillationMotionWalker>(*this);

        ar & m_translate;        
        ar & m_frequency;
        ar & m_sampling;
    }
	CartesianCoor3D m_translate;
	double m_frequency;
    long m_sampling;

	OscillationMotionWalker() {}
public:
	OscillationMotionWalker(double displace,double frequency,long sampling, CartesianCoor3D direction);
	
	CartesianCoor3D translation(size_t timepos);
};


class RandomMotionWalker : public MotionWalker {
protected:
    friend class boost::serialization::access;	
	template<class Archive> void serialize(Archive & ar, const unsigned int version)
    {
        translations.clear(); // don't transmit cache
        ar & boost::serialization::base_object<MotionWalker, RandomMotionWalker>(*this);
        ar & m_displace;
        ar & m_seed;
        ar & m_direction;
        ar & m_sampling;
    }
	std::map<size_t,CartesianCoor3D> translations;
	
    bool m_init; // init flag 
	
	double m_displace;
	long m_seed;
    long m_sampling;
    	
	CartesianCoor3D m_direction;
    boost::variate_generator<boost::mt19937, boost::uniform_on_sphere<double> >* p_myspheredistribution;
    void init();

	void generate(size_t timepos);
	RandomMotionWalker(): m_init(true) {}    

public:
	RandomMotionWalker(double displace,long seed,long sampling,  CartesianCoor3D direction);
	~RandomMotionWalker();
	
	
	CartesianCoor3D translation(size_t timepos);
};


class BrownianMotionWalker : public MotionWalker {
protected:
    friend class boost::serialization::access;	
	template<class Archive> void serialize(Archive & ar, const unsigned int version)
    {
        translations.clear();
        ar & boost::serialization::base_object<MotionWalker, BrownianMotionWalker>(*this);
        ar & m_displace;
        ar & m_seed;
        ar & m_sampling;
        ar & m_direction;        
    }
	std::map<size_t,CartesianCoor3D> translations;

    bool m_init; // init flag 
    
	double m_displace;
	long m_seed;
    long m_sampling;
	
	CartesianCoor3D m_direction;
    boost::variate_generator<boost::mt19937, boost::normal_distribution<double> >* p_mynormaldistribution;
    boost::variate_generator<boost::mt19937, boost::uniform_on_sphere<double> >* p_myspheredistribution;
	
    void init();
    
	void generate(size_t timepos);

    BrownianMotionWalker() : m_init(true) {}	
public:
	
	BrownianMotionWalker(double displace,long seed,long sampling,  CartesianCoor3D direction);
	~BrownianMotionWalker();
	
	
	CartesianCoor3D translation(size_t timepos);
};

#endif

// end of file
