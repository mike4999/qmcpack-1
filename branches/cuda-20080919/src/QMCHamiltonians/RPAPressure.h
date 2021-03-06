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
#ifndef QMCPLUSPLUS_RPAPRESSURECORRECTION_H
#define QMCPLUSPLUS_RPAPRESSURECORRECTION_H
#include "Particle/ParticleSet.h"
#include "Particle/WalkerSetRef.h"
#include "QMCHamiltonians/QMCHamiltonianBase.h"
#include "ParticleBase/ParticleAttribOps.h"
#include "QMCWaveFunctions/Jastrow/RPAJastrow.h"
// #include "QMCWaveFunctions/OrbitalBase.h"

#include "Configuration.h"

namespace qmcplusplus {

  /** @ingroup hamiltonian
   @brief Evaluate the RPA ZVZB Pressure
  **/

  struct RPAPressure: public QMCHamiltonianBase {
    typedef RealType Return_t;
    ///Laplacian and Gradient for derivative of wave function with respect to r_s
    PtclOnLatticeTraits::ParticleGradient_t dG;
    PtclOnLatticeTraits::ParticleLaplacian_t dL;
    vector<OrbitalBase*> dPsi;
    Return_t Rs;
    Return_t tValue;
    Return_t drsdV;
    Return_t pNorm;
    Return_t ZVCorrection;
    Return_t Press,Energy,Pot;

    /** constructor
     *
     * Pressure operators need to be re-evaluated during optimization.
     */
    RPAPressure(ParticleSet& P): dG(P.G),dL(P.L) {
      UpdateMode.set(OPTIMIZABLE,1);
      pNorm = 1.0/(P.Lattice.DIM*P.Lattice.Volume);
    };
    
    ///destructor
    ~RPAPressure() {};
    
    
    void addObservables(PropertySetType& plist)
    {
      myIndex=plist.add("Pressure");
      plist.add("ZVterm");
      plist.add("dpsi");
      plist.add("Edpsi");
      plist.add("Vdpsi");

    }

    void setObservables(PropertySetType& plist)
    {
      plist[myIndex]=Press;
      plist[myIndex+1]=Value;
      plist[myIndex+2]=tValue;
      plist[myIndex+3]=Energy*tValue;
      plist[myIndex+4]=Pot*tValue;
      
    }

    void resetTargetParticleSet(ParticleSet& P);

    inline Return_t 
    evaluate(ParticleSet& P);

    inline Return_t 
    evaluate(ParticleSet& P, vector<NonLocalData>& Txy) ;
    
    /** implements the virtual function.
     * 
     * Nothing is done but should check the mass
     */
    bool put(xmlNodePtr cur ) {return true;}
    
    bool put(xmlNodePtr cur, ParticleSet& P);
    bool put(xmlNodePtr cur, ParticleSet& P, ParticleSet& source, TrialWaveFunction& Psi);

    bool get(std::ostream& os) const {
      os << myName;
      return true;
    }

    QMCHamiltonianBase* makeClone(ParticleSet& P, TrialWaveFunction& psi);

    string MyName;
  };
}
#endif

/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 1581 $   $Date: 2007-01-04 10:02:14 -0600 (Thu, 04 Jan 2007) $
 * $Id: RPAPressure.h 1581 2007-01-04 16:02:14Z jnkim $ 
 ***************************************************************************/

