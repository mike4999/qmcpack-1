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
#include "QMCWaveFunctions/TrialWaveFunction.h"
#include "OhmmsData/OhmmsElementBase.h"
#include "Particle/Walker.h"
#include "Particle/WalkerSetRef.h"
#include "Utilities/OhmmsInfo.h"

namespace qmcplusplus {

  TrialWaveFunction::TrialWaveFunction():SignValue(1.0),LogValue(0.0){ }

  /** Destructor
   *
   *@warning Have not decided whether Z is cleaned up by TrialWaveFunction 
   *  or not. It will depend on I/O implementation.
   */
  TrialWaveFunction::~TrialWaveFunction(){
    DEBUGMSG("TrialWaveFunction::~TrialWaveFunction")
    for(int i=0; i<Z.size(); i++) delete Z[i];
    for(int i=0; i<SPOSet.size(); i++) delete SPOSet[i];
  }
  
  void 
  TrialWaveFunction::resetTargetParticleSet(ParticleSet& P) {
    for(int i=0; i<Z.size(); i++) Z[i]->resetTargetParticleSet(P);
  }

  /** add an ObritalBase
   *@param aterm a many-body wavefunction 
   */
  void 
  TrialWaveFunction::add(OrbitalBase* aterm) {
    Z.push_back(aterm);
  }

  TrialWaveFunction::ValueType 
  TrialWaveFunction::evaluateLog(ParticleSet& P) {
    P.G = 0.0;
    P.L = 0.0;
    //ValueType logpsi=0.0;
    LogValue=0.0;
    SignValue=1.0;
    vector<OrbitalBase*>::iterator it(Z.begin());
    vector<OrbitalBase*>::iterator it_end(Z.end());
    while(it != it_end) {
      LogValue += (*it)->evaluateLog(P, P.G, P.L); 
      SignValue *= (*it)->SignValue;
      ++it;
    }
    return LogValue;
  }
  
  /** evaluate the log value of a many-body wave function
   * @param P input configuration containing N particles
   * @param needratio users request ratio evaluation
   * @param buf anonymous storage for the reusable data
   * @return the value of \f$ \log( \Pi_i \Psi_i) \f$  many-body wave function
   *
   * @if needratio == true
   *  need to update the data from buf, since external objects need to evaluate ratios, e.g., non-local pseudopotentials
   * @else
   *  evaluate the value only
   *
   * Upon return, the gradient and laplacian operators are added by the components.
   * Each OrbitalBase evaluates SignValue and LogValue = log(abs(psi_i))
   * Jastrow functions always have SignValue=1.
   */
  TrialWaveFunction::ValueType 
  TrialWaveFunction::evaluateDeltaLog(ParticleSet& P) {
    P.G = 0.0;
    P.L = 0.0;
    //ValueType logpsi=0.0;
    LogValue=0.0;
    SignValue=1.0;
    vector<OrbitalBase*>::iterator it(Z.begin());
    vector<OrbitalBase*>::iterator it_end(Z.end());
    while(it != it_end) {
      if((*it)->Optimizable) {
        LogValue += (*it)->evaluateLog(P, P.G, P.L); 
        SignValue *= (*it)->SignValue;
      }
      ++it;
    }
    return LogValue;
  }


  /** evalaute the sum of log value of optimizable many-body wavefunctions
   * @param P  input configuration containing N particles
   * @param logpsi_fixed log(abs(psi)) of the invariant orbitals
   * @param logpsi_opt log(abs(psi)) of the variable orbitals
   * @param fixedG gradients of log(psi) of the fixed wave functions
   * @param fixedL laplacians of log(psi) of the fixed wave functions
   *
   * This function is introduced for optimization only.
   * fixedG and fixedL save the terms coming from the wave functions
   * that are invarient during optimizations.
   * It is expected that evaluateLog(P,false) is called later
   * and the external object adds the varying G and L and the fixed terms.
   * Additionally, dumpToBuffer and dumpFromBuffer is used to manage
   * necessary data for ratio evaluations.
   */
  void
  TrialWaveFunction::evaluateDeltaLog(ParticleSet& P,
      ValueType& logpsi_fixed, 
      ValueType& logpsi_opt,
      ParticleSet::ParticleGradient_t& fixedG,
      ParticleSet::ParticleLaplacian_t& fixedL) {
    P.G = 0.0;
    P.L = 0.0;
    fixedG = 0.0;
    fixedL = 0.0;
    logpsi_fixed=0.0;
    logpsi_opt=0.0;
    vector<OrbitalBase*>::iterator it(Z.begin());
    vector<OrbitalBase*>::iterator it_end(Z.end());
    while(it != it_end) {
      if((*it)->Optimizable) 
        logpsi_opt += (*it)->evaluateLog(P, P.G, P.L); 
      else
        logpsi_fixed += (*it)->evaluateLog(P, fixedG, fixedL); 
      ++it;
    }
    P.G += fixedG; 
    P.L += fixedL;
  }
  
  /** evaluate the value of a many-body wave function
   *@param P input configuration containing N particles
   *@return the value of many-body wave function
   *
   *Upon return, the gradient and laplacian operators are added by the components.
  */
  TrialWaveFunction::ValueType 
  TrialWaveFunction::evaluate(ParticleSet& P) {
    P.G = 0.0;
    P.L = 0.0;
    ValueType psi = 1.0;
    for(int i=0; i<Z.size(); i++) {
      psi *= Z[i]->evaluate(P, P.G, P.L);
    }
    //for(int iat=0; iat<P.getTotalNum(); iat++)
    // cout << P.G[iat] << " " << P.L[iat] << endl;
    return psi;
  }
  
  /** Evaluate local energies, the gradient and laplacian operators
   * @param W the input set of walkers
   * @param psi a array containing Nw wave function values of each walker
  void 
  TrialWaveFunction::evaluate(WalkerSetRef& W, 
			      OrbitalBase::ValueVectorType& psi)
  {
    W.G = 0.0;
    W.L = 0.0;
    psi = 1.0;
    for(int i=0; i<Z.size(); i++) {
      Z[i]->evaluate(W,psi,W.G,W.L);
    }
    //for(int iw=0; iw<psi.size(); iw++) W.Properties(iw,Sign) = psi[iw];
  }
   */
  
  TrialWaveFunction::ValueType
  TrialWaveFunction::ratio(ParticleSet& P,int iat) {
    RealType r=1.0;
    for(int i=0; i<Z.size(); i++) {
      r *= Z[i]->ratio(P,iat);
    }
    return r;
  }
  void   
  TrialWaveFunction::update(ParticleSet& P,int iat) {
    //ready to collect "changes" in the gradients and laplacians by the move
    delta_G=0.0; delta_L=0.0;
    for(int i=0; i<Z.size(); i++) Z[i]->update(P,delta_G,delta_L,iat);
    P.G += delta_G;
    P.L += delta_L;
  }

  
  /** evaluate \f$ frac{\Psi({\bf R}_i^{'})}{\Psi({\bf R}_i)}\f$
   * @param P ParticleSet
   * @param iat index of the particle with a trial move
   * @param dG total differentcal gradients
   * @param dL total differential laplacians
   * @return ratio
   *
   * Each OrbitalBase object adds the differential gradients and lapacians.
   */
  TrialWaveFunction::ValueType 
  TrialWaveFunction::ratio(ParticleSet& P, int iat, 
			   ParticleSet::ParticleGradient_t& dG,
			   ParticleSet::ParticleLaplacian_t& dL) {
    dG = 0.0;
    dL = 0.0;
    RealType r=1.0;
    for(int i=0; i<Z.size(); i++) r *= Z[i]->ratio(P,iat,dG,dL);
    return r;
  }

  TrialWaveFunction::ValueType 
  TrialWaveFunction::logRatio(ParticleSet& P, int iat, 
			   ParticleSet::ParticleGradient_t& dG,
			   ParticleSet::ParticleLaplacian_t& dL) {
    dG = 0.0;
    dL = 0.0;
    RealType logr=0.0;
    SignValue=1.0;
    for(int i=0; i<Z.size(); i++) {
      logr += Z[i]->ratio(P,iat,dG,dL);
      SignValue *= Z[i]->SignValue;
    }
    return logr;
  }

  /** restore to the original state
   * @param iat index of the particle with a trial move
   *
   * The proposed move of the iath particle is rejected.
   * All the temporary data should be restored to the state prior to the move.
   */
  void 
  TrialWaveFunction::restore(int iat) {
    for(int i=0; i<Z.size(); i++) Z[i]->restore(iat);
  }

  /** update the state with the new data
   * @param P ParticleSet
   * @param iat index of the particle with a trial move
   *
   * The proposed move of the iath particle is accepted.
   * All the temporary data should be incorporated so that the next move is valid.
   */
  void   
  TrialWaveFunction::update2(ParticleSet& P,int iat) {
    for(int i=0; i<Z.size(); i++) Z[i]->update(P,iat);
  }

  void TrialWaveFunction::resizeByWalkers(int nwalkers){
    for(int i=0; i<Z.size(); i++) Z[i]->resizeByWalkers(nwalkers);
  }
  
  void TrialWaveFunction::reset(){
    for(int i=0; i<Z.size(); i++) Z[i]->reset();
  }
  
  TrialWaveFunction::ValueType
  TrialWaveFunction::registerData(ParticleSet& P, PooledData<RealType>& buf) {
    delta_G.resize(P.getTotalNum());
    delta_L.resize(P.getTotalNum());
    P.G = 0.0;
    P.L = 0.0;

    LogValue=0.0;
    SignValue=1.0;
    vector<OrbitalBase*>::iterator it(Z.begin());
    vector<OrbitalBase*>::iterator it_end(Z.end());
    while(it != it_end) {
      LogValue += (*it)->registerData(P,buf);
      SignValue *= (*it)->SignValue;
      ++it;
    }

    //append current gradients and laplacians to the buffer
    TotalDim = OHMMS_DIM*P.getTotalNum();
    buf.add(&(P.G[0][0]), &(P.G[0][0])+TotalDim);
    buf.add(P.L.begin(), P.L.end());

    return LogValue;
//     cout << "Registering gradients and laplacians " << endl;
//     for(int i=0; i<P.getLocalNum(); i++) {
//       cout << P.G[i] << " " << P.L[i] << endl;
//     }
  }

  TrialWaveFunction::ValueType
  TrialWaveFunction::updateBuffer(ParticleSet& P, PooledData<RealType>& buf) {
    P.G = 0.0;
    P.L = 0.0;

    LogValue=0.0;
    SignValue=1.0;
    vector<OrbitalBase*>::iterator it(Z.begin());
    vector<OrbitalBase*>::iterator it_end(Z.end());
    while(it != it_end) {
      LogValue += (*it)->updateBuffer(P,buf);
      SignValue *= (*it)->SignValue;
      ++it;
    }

    buf.put(&(P.G[0][0]), &(P.G[0][0])+TotalDim);
    buf.put(P.L.begin(), P.L.end());
    return LogValue;
  }

  void TrialWaveFunction::copyFromBuffer(ParticleSet& P, PooledData<RealType>& buf) {

    for(int i=0; i<Z.size(); i++) Z[i]->copyFromBuffer(P,buf);

    //get the gradients and laplacians from the buffer
    buf.get(&(P.G[0][0]), &(P.G[0][0])+TotalDim);
    buf.get(P.L.begin(), P.L.end());

//     cout << "Checking out gradients and laplacians " << endl;
//     for(int i=0; i<P.getLocalNum(); i++) {
//       cout << P.G[i] << " " << P.L[i] << endl;
//     }
  }

  /** Dump data that are required to evaluate ratios to the buffer
   * @param P active ParticleSet
   * @param buf anonymous buffer to which the data will be dumped.
   * 
   * This function lets the OrbitalBase objects store the minimal data
   * that are required to evaluate the ratio, even though the components
   * are invariant during the optimizations.
   */
  void
  TrialWaveFunction::dumpToBuffer(ParticleSet& P, BufferType& buf) {
    vector<OrbitalBase*>::iterator it(Z.begin());
    vector<OrbitalBase*>::iterator it_end(Z.end());
    while(it != it_end) {
      (*it)->dumpToBuffer(P,buf);
      ++it;
    }
  }

  /** copy data that are required to evaluate ratios from the buffer
   * @param P active ParticleSet
   * @param buf anonymous buffer from which the data will be copied.
   * 
   * This function lets the OrbitalBase objects get the minimal data
   * that are required to evaluate the ratio from the buffer.
   * Only the data registered by dumToBuffer will be available.
   */
  void
  TrialWaveFunction::dumpFromBuffer(ParticleSet& P, BufferType& buf) {
    vector<OrbitalBase*>::iterator it(Z.begin());
    vector<OrbitalBase*>::iterator it_end(Z.end());
    while(it != it_end) {
      (*it)->dumpFromBuffer(P,buf);
      ++it;
    }
  }

  TrialWaveFunction::ValueType
  TrialWaveFunction::evaluate(ParticleSet& P, PooledData<RealType>& buf) {

    ValueType psi = 1.0;
    for(int i=0; i<Z.size(); i++) psi *= Z[i]->evaluate(P,buf);
    buf.put(&(P.G[0][0]), &(P.G[0][0])+TotalDim);
    buf.put(P.L.begin(), P.L.end());

//     cout << "Checking in gradients and laplacians " << endl;
//     for(int i=0; i<P.getLocalNum(); i++) {
//       cout << P.G[i] << " " << P.L[i] << endl;
//     }
    return psi;
  }

  bool TrialWaveFunction::hasSPOSet(const string& aname) {
    bool notfoundit=true;
    vector<OhmmsElementBase*>::iterator it(SPOSet.begin());
    vector<OhmmsElementBase*>::iterator it_end(SPOSet.end());
    while(notfoundit && it != it_end) {
      if((*it)->getName() == aname) notfoundit=false;
      ++it;
    }
    return !notfoundit;
  }

  OhmmsElementBase* 
  TrialWaveFunction::getSPOSet(const string& aname) {
    bool notfoundit=true;
    vector<OhmmsElementBase*>::iterator it(SPOSet.begin());
    vector<OhmmsElementBase*>::iterator it_end(SPOSet.end());
    while(notfoundit && it != it_end) {
      if((*it)->getName() == aname) return *it;
      ++it;
    }
    return 0;
  }

  void TrialWaveFunction::addSPOSet(OhmmsElementBase* spo) {
    SPOSet.push_back(spo);
  }
}
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/

