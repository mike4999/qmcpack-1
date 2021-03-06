//////////////////////////////////////////////////////////////////
// (c) Copyright 2003  by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   National Center for Supercomputing Applications &
//   Materials Computation Center
//   University of Illinois, Urbana-Champaign
//   Urbana, IL 61801
//   e-mail: jnkim@ncsa.uiuc.edu
//   Tel:    217-244-6319 (NCSA) 217-333-3324 (MCC)
//
// Supported by 
//   National Center for Supercomputing Applications, UIUC
//   Materials Computation Center, UIUC
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
#ifndef OHMMS_QMC_COULOMBPOTENTIAL_H
#define OHMMS_QMC_COULOMBPOTENTIAL_H
#include <algo.h>
#include "Particle/ParticleSet.h"
#include "Particle/WalkerSetRef.h"
#include "Particle/DistanceTableData.h"
#include "QMCHamiltonians/QMCHamiltonianBase.h"

namespace ohmmsqmc {

  /**
   *\brief CoulombPotential for the different source and 
   *target particle sets.
   *
   * \f[ H = \sum_i \frac{Z(i)q}{r} \f] 
   * where \f$ Z(i) \f$ is the effective charge of the Ith 
   * ion and \f$ q \f$ is the charge of the set of quantum particles.
   * For instance, \f$ q = -1 \f$ for electrons and 
   * \f$ q = 1 \f$ for positrons.
   */
  struct CoulombPotentialAB: public QMCHamiltonianBase {

    ///number of ions
    int Centers;
    ///container for the ion charges
    vector<RealType> Z;
    DistanceTableData* d_table;

    CoulombPotentialAB(ParticleSet& ions, ParticleSet& els): d_table(NULL) { 

      d_table = DistanceTable::getTable(DistanceTable::add(ions,els));
      //index for attribute charge
      int iz = ions.Species.addAttribute("charge");
      Centers = ions.getTotalNum();
      Z.resize(Centers);
      ///@warning need to be generalized by checking visitor.Species.
      RealType C = -1.0; 
      for(int iat=0; iat<Centers;iat++) {
        Z[iat] = ions.Species(iz,ions.GroupID[iat])*C;
      }
    }
    
    ~CoulombPotentialAB() { }

    inline ValueType evaluate(ParticleSet& P) {
      RealType esum=0.0;
      for(int iat=0; iat<Centers; iat++) {
        for(int nn=d_table->M[iat]; nn<d_table->M[iat+1]; nn++)
	  esum+=Z[iat]*d_table->rinv(nn);
      }
      return esum;
    }

    inline ValueType
    evaluate(ParticleSet& P, RealType& x){
      RealType esum=0.0;
      for(int iat=0; iat<Centers; iat++) {
        for(int nn=d_table->M[iat]; nn<d_table->M[iat+1]; nn++) 
	  esum+=Z[iat]*d_table->rinv(nn);
      }
      x=esum;
      return esum;
    } 

#ifdef USE_FASTWALKER
    inline void 
    evaluate(WalkerSetRef& W, ValueVectorType& LE) {
      register int nw = W.walkers();
      ValueVectorType e(nw);
      for(int iat=0; iat<Centers; iat++) {
        e=0.0;
        for(int nn=d_table->M[iat]; nn<d_table->M[iat+1]; nn++) {
          for(int iw=0; iw<nw; iw++) {
	    e[iw] += d_table->rinv(iw,nn);
	  }
	}
        ValueType z=Z[iat];
        for(int iw=0; iw<W.walkers(); iw++) { LE[iw] += z*e[iw];} 
      }
    }
#else
    inline void 
    evaluate(WalkerSetRef& W, ValueVectorType& LE) {
      for(int iat=0; iat<Centers; iat++) {
        RealType z=Z[iat];
        for(int iw=0; iw<W.walkers(); iw++) {
	  RealType esub = 0.0;
	  for(int nn=d_table->M[iat]; nn<d_table->M[iat+1]; nn++) {
	    esub += d_table->rinv(iw,nn);
	  }
	  LE[iw] += z*esub; ///multiply z
	}
      }
    }
#endif
  };

  /** 
   *\brief CoulombPotential for the indentical source and 
   * target particle sets. 
   *
   * \f[ H = \sum_i \frac{q^2}{r} \f] 
   * where \f$ q \f$ is the charge of the set of quantum 
   * particles.  For instance, \f$ q = -1 \f$ for electrons 
   * and \f$ q = 1 \f$ for positrons.
   */
  struct CoulombPotentialAA: public QMCHamiltonianBase {

    RealType C;
    DistanceTableData* d_table;
    //ElecElecPotential(RealType c=1.0): C(c){}
    CoulombPotentialAA(ParticleSet& P):d_table(NULL) {
      d_table = DistanceTable::getTable(DistanceTable::add(P));
      C = 1.0;
    }

    ~CoulombPotentialAA() { }

    inline ValueType 
    evaluate(ParticleSet& P) {
      RealType sum = 0.0;
      for(int i=0; i<d_table->getTotNadj(); i++) sum += d_table->rinv(i);
      return C*sum;
      //return C*std::accumulate(d_table->rinv.data(), 
      //	  	       d_table->rinv.data()+d_table->getTotNadj(),
      //		       0.0);
    }

    inline ValueType 
    evaluate(ParticleSet& P, RealType& x){
      RealType sum = 0.0;
      for(int i=0; i<d_table->getTotNadj(); i++) sum += d_table->rinv(i);
      return x=sum*C;
    }

#ifdef USE_FASTWALKER
    inline void 
    evaluate(WalkerSetRef& W, ValueVectorType& LE) {
      std::vector<ValueType> e(W.walkers(),0.0);
      for(int nn = 0; nn< d_table->getTotNadj(); nn++) {
	for(int iw=0; iw<W.walkers(); iw++) {
	  e[iw] += d_table->rinv(iw,nn);
	}
      }
      for(int iw=0; iw<W.walkers(); iw++) { LE[iw] += C*e[iw];} 
    }
#else
    inline void 
    evaluate(WalkerSetRef& W, ValueVectorType& LE) {
      for(int iw=0; iw<W.walkers(); iw++) {
	RealType e =0.0;
	for(int nn = 0; nn< d_table->getTotNadj(); nn++)  e += d_table->rinv(iw,nn);
	LE[iw] += C*e; 
      }
    }
#endif
  };
}
#endif

/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/

