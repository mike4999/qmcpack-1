//////////////////////////////////////////////////////////////////
// (c) Copyright 2008-  by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   National Center for Supercomputing Applications &
//   Materials Computation Center
//   University of Illinois, Urbana-Champaign
//   Urbana, IL 61801
//   e-mail: jnkim@ncsa.uiuc.edu
//
// Supported by 
//   National Center for Supercomputing Applications, UIUC
//   Materials Computation Center, UIUC
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
/**@file einspline3d_benchmark.h
 * @brief define einspline3d_benchmark and random_position_generator
 */
#ifndef QMCPLUSPLUS_EINSPLINE3D_BENCHMARK_H
#define QMCPLUSPLUS_EINSPLINE3D_BENCHMARK_H
#include <Configuration.h>
#include <Utilities/RandomGenerator.h>
#include <Utilities/Timer.h>
#include <OhmmsPETE/OhmmsVector.h>
#include <OhmmsPETE/OhmmsMatrix.h>
#include <OhmmsPETE/OhmmsArray.h>
#include <spline/einspline_engine.hpp>
namespace qmcplusplus
{
  template<typename ENGT>
    struct einspline3d_benchmark
    {
      typedef typename einspline_engine<ENGT>::real_type real_type;
      typedef typename einspline_engine<ENGT>::value_type value_type;
      typedef TinyVector<real_type,3> pos_type;
      einspline_engine<ENGT> einspliner;
      pos_type start;
      pos_type end;
      Vector<value_type> psi;
      Vector<value_type> lap;
      Vector<TinyVector<value_type,3> > grad;
      Vector<Tensor<value_type,3> > hess;

      einspline3d_benchmark():start(0.0),end(1.0) { }

      einspline3d_benchmark(int nx, int ny, int nz, int num_splines)
        : start(0.0),end(1.0)
      {
        set(nx,ny,nz);
      }

      void set(int nx, int ny, int nz, int num_splines)
      {
        TinyVector<int,3> ng(nx,ny,nz);
        einspliner.create(start,end,ng,PERIODIC,num_splines);
        Array<value_type,3> data(nx,ny,nz);
        for(int i=0; i<num_splines; ++i)
        {
          for(int j=0; j<data.size();++j) data(j) = Random();
          einspliner.set(i,data);
        }
	psi.resize(num_splines);
        grad.resize(num_splines);
        lap.resize(num_splines);
        hess.resize(num_splines);
      }

      inline TinyVector<double,3> test_all(const vector<pos_type>& Vpos,
          const vector<pos_type>& VGLpos, const vector<pos_type>& VGHpos)
      {
        TinyVector<double,3> timers;
        Timer clock;
	test_v(Vpos);
	timers[0]+=clock.elapsed();
	clock.restart();
	test_vgl(VGLpos);
	timers[1]+=clock.elapsed();
	clock.restart();
	test_vgh(VGHpos);
	timers[2]+=clock.elapsed();
        return timers;
      }

      inline void test_v(const vector<pos_type>& coord) //const
      {
        for(int i=0; i<coord.size(); ++i) einspliner.evaluate(coord[i],psi);
      }

      inline void test_vgl(const vector<pos_type>& coord)// const
      {
        for(int i=0; i<coord.size(); ++i) einspliner.evaluate(coord[i],psi,grad,lap);
      }

      inline void test_vgh(const vector<pos_type>& coord) //const
      {
        for(int i=0; i<coord.size(); ++i) einspliner.evaluate(coord[i],psi,grad,hess);
      }

    };

  template<typename T>
  struct random_position_generator
  {
    typedef TinyVector<T,3> pos_type;
    typedef RandomGenerator_t::uint_type uint_type;
    RandomGenerator_t myrand;
    vector<pos_type> Vpos, VGLpos, VGHpos;
    random_position_generator(int n,  uint_type seed):myrand(seed)
    {
      Vpos.resize(n);
      VGLpos.resize(n);
      VGHpos.resize(n);
    }
    inline void randomize()
    {
      for(int i=0; i<Vpos.size(); ++i) Vpos[i]=pos_type(myrand(),myrand(),myrand());
      for(int i=0; i<VGLpos.size(); ++i) VGLpos[i]=pos_type(myrand(),myrand(),myrand());
      for(int i=0; i<VGHpos.size(); ++i) VGHpos[i]=pos_type(myrand(),myrand(),myrand());
    }
  };
}
#endif
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 1770 $   $Date: 2007-02-17 17:45:38 -0600 (Sat, 17 Feb 2007) $
 * $Id: OrbitalBase.h 1770 2007-02-17 23:45:38Z jnkim $ 
 ***************************************************************************/
