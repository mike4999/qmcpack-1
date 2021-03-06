//////////////////////////////////////////////////////////////////
// (c) Copyright 2003- by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   Jeongnim Kim
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
//   Department of Physics, Ohio State University
//   Ohio Supercomputer Center
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
#include "QMCDrivers/DMC/DMCUpdateAll.h"
#include "ParticleBase/ParticleUtility.h"
#include "ParticleBase/RandomSeqGenerator.h"
#include "QMCDrivers/DriftOperators.h"

namespace qmcplusplus { 

  /// Constructor.
  DMCUpdateAllWithRejection::DMCUpdateAllWithRejection(ParticleSet& w, 
      TrialWaveFunction& psi, QMCHamiltonian& h, RandomGenerator_t& rg): 
    QMCUpdateBase(w,psi,h,rg)
    { }
  
  /// destructor
  DMCUpdateAllWithRejection::~DMCUpdateAllWithRejection() { }

  //void DMCUpdateAllWithRejection::initWalkers(WalkerIter_t it, WalkerIter_t it_end){
  //}

  //void DMCUpdateAllWithRejection::updateWalkers(WalkerIter_t it, WalkerIter_t it_end){
  //}

  /** advance all the walkers with killnode==no
   * @param nat number of particles to move
   * 
   * When killnode==no, any move resulting in node-crossing is treated
   * as a normal rejection.
   */
  void DMCUpdateAllWithRejection::advanceWalkers(WalkerIter_t it, WalkerIter_t it_end, 
      bool measure) 
  {
    while(it != it_end) {
      Walker_t& thisWalker(**it);
      
      //save old local energy
      RealType eold    = thisWalker.Properties(LOCALENERGY);
      RealType signold = thisWalker.Properties(SIGN);
      RealType emixed  = eold;

      //create a 3N-Dimensional Gaussian with variance=1
      makeGaussRandomWithEngine(deltaR,RandomGen);

      W.R = m_sqrttau*deltaR + thisWalker.R + thisWalker.Drift;
      
      //update the distance table associated with W
      //DistanceTable::update(W);
      W.update();
      
      //evaluate wave function
      RealType logpsi(Psi.evaluateLog(W));

      bool accepted=false; 
      if(branchEngine->phaseChanged(Psi.getPhase(),thisWalker.Properties(SIGN))) {
        thisWalker.Age++;
      } else {
        RealType enew(H.evaluate(W));
        RealType logGf = -0.5*Dot(deltaR,deltaR);
        //converting gradients to drifts, D = tau*G (reuse G)
        //RealType scale=getDriftScale(Tau,W.G);
        //drift = scale*W.G;
        setScaledDrift(Tau,W.G,drift);

        deltaR = (*it)->R - W.R - drift;
        RealType logGb = -m_oneover2tau*Dot(deltaR,deltaR);

        RealType prob= std::min(std::exp(logGb-logGf +2.0*(logpsi-thisWalker.Properties(LOGPSI))),1.0);
        if(RandomGen() > prob){
          thisWalker.Age++;
        } else {
          accepted=true;  
          thisWalker.R = W.R;
          thisWalker.Drift = drift;
          thisWalker.resetProperty(logpsi,Psi.getPhase(),enew);
          H.saveProperty(thisWalker.getPropertyBase());
          emixed = (emixed+enew)*0.5;
          eold=enew;
        }
      }

      thisWalker.Weight *= branchEngine->branchGF(Tau,emixed,0.0);
      //branchEngine->accumulate(eold,1);

      if(accepted) 
        ++nAccept;
      else 
        ++nReject;
      ++it;
    }
  }

 /*
  * DMCUpdateAllWithKill member functions
  */
  /// Constructor.
  DMCUpdateAllWithKill::DMCUpdateAllWithKill(ParticleSet& w, 
      TrialWaveFunction& psi, QMCHamiltonian& h, RandomGenerator_t& rg): QMCUpdateBase(w,psi,h,rg)
    { }
  
  /// destructor
  DMCUpdateAllWithKill::~DMCUpdateAllWithKill() { }

  //void DMCUpdateAllWithKill::initWalkers(WalkerIter_t it, WalkerIter_t it_end){
  //}

  //void DMCUpdateAllWithKill::updateWalkers(WalkerIter_t it, WalkerIter_t it_end){
  //}
  /** advance all the walkers with killnode==yes
   */
  void DMCUpdateAllWithKill::advanceWalkers(WalkerIter_t it, WalkerIter_t it_end,
      bool measure) 
  {
    while(it != it_end) {
      
      Walker_t& thisWalker(**it);
      
      //save old local energy
      RealType eold = thisWalker.Properties(LOCALENERGY);
      RealType enew = eold;
      RealType signold = thisWalker.Properties(SIGN);
      //RealType emixed  = eold;

      //create a 3N-Dimensional Gaussian with variance=1
      makeGaussRandomWithEngine(deltaR,RandomGen);
      
      W.R = m_sqrttau*deltaR + thisWalker.R + thisWalker.Drift;
      
      //update the distance table associated with W
      //DistanceTable::update(W);
      W.update();
      
      //evaluate wave function
      RealType logpsi(Psi.evaluateLog(W));

      bool accepted=false; 
      if(branchEngine->phaseChanged(Psi.getPhase(),thisWalker.Properties(SIGN))) {
        thisWalker.Age++;
        thisWalker.willDie();
      } else {
        enew=H.evaluate(W);
        RealType logGf = -0.5*Dot(deltaR,deltaR);
        //converting gradients to drifts, D = tau*G (reuse G)
        //RealType scale=getDriftScale(Tau,W.G);
        //drift = scale*W.G;
        setScaledDrift(Tau,W.G,drift);

        deltaR = (*it)->R - W.R - drift;
        RealType logGb = -m_oneover2tau*Dot(deltaR,deltaR);

        RealType prob= std::min(std::exp(logGb-logGf +2.0*(logpsi-thisWalker.Properties(LOGPSI))),1.0);
        if(RandomGen() > prob){
          enew=eold;
          thisWalker.Age++;
        } else {
          accepted=true;  
          thisWalker.R = W.R;
          thisWalker.Drift = drift;
          thisWalker.resetProperty(logpsi,Psi.getPhase(),enew);
          H.saveProperty(thisWalker.getPropertyBase());
          //emixed = (emixed+enew)*0.5;
          //eold=enew;
        }

        //thisWalker.Weight *= branchEngine->branchGF(Tau,emixed,0.0);
        thisWalker.Weight *= branchEngine->branchWeight(Tau,eold,enew);
        //if(MaxAge) {
        //  RealType M=thisWalker.Weight;
        //  if(thisWalker.Age > MaxAge) M = std::min(0.5,M);
        //  else if(thisWalker.Age > 0) M = std::min(1.0,M);
        //  thisWalker.Multiplicity = M + RandomGen();
        //  branchEngine->accumulate(eold,M);
        //} else {
        //  branchEngine->accumulate(eold,1);
        //}
      }

      if(accepted) 
        ++nAccept;
      else 
        ++nReject;
      ++it;
    }
  }
}

/***************************************************************************
 * $RCSfile: DMCUpdateAll.cpp,v $   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
