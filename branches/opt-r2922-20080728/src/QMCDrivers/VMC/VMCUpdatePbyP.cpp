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
#include "QMCDrivers/VMC/VMCUpdatePbyP.h"
#include "QMCDrivers/DriftOperators.h"
#include "Message/OpenMP.h"

namespace qmcplusplus { 

  VMCUpdatePbyP::VMCUpdatePbyP(MCWalkerConfiguration& w, TrialWaveFunction& psi, 
      QMCHamiltonian& h, RandomGenerator_t& rg):
    QMCUpdateBase(w,psi,h,rg), nSubSteps(1)
    { 
      myParams.add(nSubSteps,"subSteps","int"); 
      myParams.add(nSubSteps,"substeps","int");

      myTimers.push_back(new NewTimer("VMCUpdatePbyP::advance")); //timer for the walker loop
      myTimers.push_back(new NewTimer("VMCUpdatePbyP::movePbyP")); //timer for MC, ratio etc
      myTimers.push_back(new NewTimer("VMCUpdatePbyP::updateMBO")); //timer for measurements
      myTimers.push_back(new NewTimer("VMCUpdatePbyP::energy")); //timer for measurements
      TimerManager.addTimer(myTimers[0]);
      TimerManager.addTimer(myTimers[1]);
      TimerManager.addTimer(myTimers[2]);
      TimerManager.addTimer(myTimers[3]);
    }

  VMCUpdatePbyP::~VMCUpdatePbyP()
  {
  }

  void VMCUpdatePbyP::advanceWalkers(WalkerIter_t it, WalkerIter_t it_end, bool measure) 
  {

    myTimers[0]->start();
    for(; it != it_end;++it) 
    {

      Walker_t& thisWalker(**it);
      Walker_t::Buffer_t& w_buffer(thisWalker.DataSet);

      W.R = thisWalker.R;
      w_buffer.rewind();
      W.copyFromBuffer(w_buffer);
      Psi.copyFromBuffer(W,w_buffer);


      RealType psi_old = thisWalker.Properties(SIGN);
      RealType psi = psi_old;

      myTimers[1]->start();
      for(int iter=0; iter<nSubSteps; ++iter) 
      {
        makeGaussRandomWithEngine(deltaR,RandomGen);
        bool stucked=true;
        for(int iat=0; iat<W.getTotalNum(); iat++) {

          PosType dr = m_sqrttau*deltaR[iat];
          PosType newpos = W.makeMove(iat,dr);

          RealType ratio = Psi.ratio(W,iat);
          RealType prob = ratio*ratio;
          //RealType prob = std::min(1.0e0,ratio*ratio);
          if(RandomGen() < prob) { 
            stucked=false;
            ++nAccept;
            W.acceptMove(iat);
            Psi.acceptMove(W,iat);
          } else {
            ++nReject; 
            W.rejectMove(iat); 
            Psi.rejectMove(iat);
          }
        }
        if(stucked) {
          ++nAllRejected;
        }
      }
      myTimers[1]->stop();

      myTimers[2]->start();
      thisWalker.R = W.R;
      w_buffer.rewind();
      W.updateBuffer(w_buffer);
      RealType logpsi = Psi.updateBuffer(W,w_buffer);
      //W.copyToBuffer(w_buffer);
      //RealType logpsi = Psi.evaluate(W,w_buffer);
      myTimers[2]->stop();

      myTimers[3]->start();
      RealType eloc=H.evaluate(W);
      myTimers[3]->stop();

      thisWalker.resetProperty(logpsi,Psi.getPhase(),eloc);
      H.saveProperty(thisWalker.getPropertyBase());

    }
    myTimers[0]->stop();
  }

  /// Constructor.
  VMCUpdatePbyPWithDrift::VMCUpdatePbyPWithDrift(MCWalkerConfiguration& w, TrialWaveFunction& psi, 
      QMCHamiltonian& h, RandomGenerator_t& rg):
    QMCUpdateBase(w,psi,h,rg) 
    { 
    }

  VMCUpdatePbyPWithDrift::~VMCUpdatePbyPWithDrift()
  {
  }

  void VMCUpdatePbyPWithDrift::advanceWalkers(WalkerIter_t it, WalkerIter_t it_end, bool measure) 
  {
    for( ;it != it_end; ++it) 
    {
      Walker_t& thisWalker(**it);
      Walker_t::Buffer_t& w_buffer(thisWalker.DataSet);

      W.R = thisWalker.R;
      w_buffer.rewind();
      W.copyFromBuffer(w_buffer);
      Psi.copyFromBuffer(W,w_buffer);

      RealType psi_old = thisWalker.Properties(SIGN);
      RealType psi = psi_old;
      //create a 3N-Dimensional Gaussian with variance=1
      makeGaussRandomWithEngine(deltaR,RandomGen);

      bool moved = false;

      for(int iat=0; iat<W.getTotalNum(); ++iat) 
      {

        //PosType dr = m_sqrttau*deltaR[iat]+thisWalker.Drift[iat];
        RealType sc=getDriftScale(Tau,W.G[iat]);
        PosType dr(m_sqrttau*deltaR[iat]+sc*real(W.G[iat]));

        PosType newpos = W.makeMove(iat,dr);
        RealType ratio = Psi.ratio(W,iat,dG,dL);
        RealType prob = ratio*ratio;

        //zero is always rejected
        if(prob<numeric_limits<RealType>::epsilon()) 
        {
          ++nReject; 
          W.rejectMove(iat); Psi.rejectMove(iat);
          continue; 
        }

        G = W.G+dG;

        //RealType forwardGF = exp(-0.5*dot(deltaR[iat],deltaR[iat]));
        //dr = (*it)->R[iat]-newpos-Tau*G[iat]; 
        //RealType backwardGF = exp(-oneover2tau*dot(dr,dr));
        RealType logGf = -0.5e0*dot(deltaR[iat],deltaR[iat]);

        //RealType scale=getDriftScale(Tau,G);
        RealType scale=getDriftScale(Tau,G[iat]);
        dr = thisWalker.R[iat]-newpos-scale*real(G[iat]);

        RealType logGb = -m_oneover2tau*dot(dr,dr);

        //RealType prob = std::min(1.0e0,ratio*ratio*std::exp(logGb-logGf));
        if(RandomGen() < prob*std::exp(logGb-logGf)) 
        { 
          moved = true;
          ++nAccept;
          W.acceptMove(iat);
          Psi.acceptMove(W,iat);
          W.G = G;
          W.L += dL;

          //do not need to update Drift
          //assignDrift(scale,G,thisWalker.Drift);

        } else {
          ++nReject; 
          W.rejectMove(iat); Psi.rejectMove(iat);
        }
      }

      if(moved) 
      {
        thisWalker.R = W.R;
        PAOps<RealType,OHMMS_DIM>::copy(W.G,thisWalker.Drift);

        w_buffer.rewind();
        W.copyToBuffer(w_buffer);
        psi = Psi.evaluate(W,w_buffer);
        RealType eloc=H.evaluate(W);
        thisWalker.resetProperty(std::log(abs(psi)), psi,eloc);
        H.saveProperty(thisWalker.getPropertyBase());
      }
      else 
      { 
        ++nAllRejected;
      }
    }
  }
}

/***************************************************************************
 * $RCSfile: VMCUpdatePbyP.cpp,v $   $Author: jnkim $
 * $Revision: 1.25 $   $Date: 2006/10/18 17:03:05 $
 * $Id: VMCUpdatePbyP.cpp,v 1.25 2006/10/18 17:03:05 jnkim Exp $ 
 ***************************************************************************/
