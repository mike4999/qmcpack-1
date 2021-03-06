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
#include "Utilities/IteratorUtility.h"
#include "QMCWaveFunctions/OrbitalTraits.h"

namespace qmcplusplus {

  typedef enum { V_TIMER, VGL_TIMER, ACCEPT_TIMER, NL_TIMER, 
		 RECOMPUTE_TIMER, DERIVS_TIMER, TIMER_SKIP} TimerEnum;

  TrialWaveFunction::TrialWaveFunction(Communicate* c): MPIObjectBase(c), OhmmsElementBase("psi0"), 
  Ordered(true), NumPtcls(0), PhaseValue(0.0),LogValue(0.0) 
  {
  }

  ///private and cannot be used
  TrialWaveFunction::TrialWaveFunction(): MPIObjectBase(0),OhmmsElementBase("psi0"),
    PhaseValue(0.0),LogValue(0.0) 
  {
  }

  /** Destructor
   *
   *@warning Have not decided whether Z is cleaned up by TrialWaveFunction 
   *  or not. It will depend on I/O implementation.
   */
  TrialWaveFunction::~TrialWaveFunction(){
    delete_iter(Z.begin(),Z.end());
    //delete_iter(SPOSet.begin(),SPOSet.end());
    //delete_iter(myTimers.begin(),myTimers.end());
  }
  
  void 
  TrialWaveFunction::resetTargetParticleSet(ParticleSet& P) {
    for(int i=0; i<Z.size(); i++) Z[i]->resetTargetParticleSet(P);
  }

  void TrialWaveFunction::startOptimization()
  {
    for(int i=0; i<Z.size(); i++) Z[i]->IsOptimizing=true;
  }

  void TrialWaveFunction::stopOptimization()
  {
    for(int i=0; i<Z.size(); i++) {
      Z[i]->IsOptimizing=false;
      Z[i]->finalizeOptimization();
    }
  }

  /** add an ObritalBase
   *@param aterm a many-body wavefunction 
   */
  void 
  //TrialWaveFunction::addOrbital(OrbitalBase* aterm) 
  TrialWaveFunction::addOrbital(OrbitalBase* aterm, const string& aname) 
  {

    Z.push_back(aterm);
    //int n=Z.size();
    char name1[64],name2[64],name3[64],name4[64], name5[64], name6[64];
    sprintf(name1,"WaveFunction::%s_V",aname.c_str());
    sprintf(name2,"WaveFunction::%s_VGL",aname.c_str());
    sprintf(name3,"WaveFunction::%s_accept",aname.c_str());
    sprintf(name4,"WaveFunction::%s_NLratio",aname.c_str());
    sprintf(name5,"WaveFunction::%s_recompute",aname.c_str());
    sprintf(name6,"WaveFunction::%s_derivs",aname.c_str());
    NewTimer *vtimer=new NewTimer(name1);
    NewTimer *vgltimer=new NewTimer(name2);
    NewTimer *accepttimer=new NewTimer(name3);
    NewTimer *NLtimer=new NewTimer(name4);
    NewTimer *recomputetimer=new NewTimer(name5);
    NewTimer *derivstimer=new NewTimer(name6);

    myTimers.push_back(vtimer);
    myTimers.push_back(vgltimer);
    myTimers.push_back(accepttimer);
    myTimers.push_back(NLtimer);
    myTimers.push_back(recomputetimer);
    myTimers.push_back(derivstimer);
    TimerManager.addTimer(vtimer);
    TimerManager.addTimer(vgltimer);
    TimerManager.addTimer(accepttimer);
    TimerManager.addTimer(NLtimer);
    TimerManager.addTimer(recomputetimer);
    TimerManager.addTimer(derivstimer);
  }

  
  /** return log(|psi|)
   *
   * PhaseValue is the phase for the complex wave function
   */
  TrialWaveFunction::RealType
  TrialWaveFunction::evaluateLog(ParticleSet& P) {
    P.G = 0.0;
    P.L = 0.0;

    ValueType logpsi(0.0);
    PhaseValue=0.0;
    vector<OrbitalBase*>::iterator it(Z.begin());
    vector<OrbitalBase*>::iterator it_end(Z.end());

    //WARNING: multiplication for PhaseValue is not correct, fix this!!
    for(; it!=it_end; ++it)
    {
      logpsi += (*it)->evaluateLog(P, P.G, P.L); 
      PhaseValue += (*it)->PhaseValue;
    }
    return LogValue=real(logpsi);
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
   * Each OrbitalBase evaluates PhaseValue and LogValue = log(abs(psi_i))
   * Jastrow functions always have PhaseValue=1.
   */
  TrialWaveFunction::RealType 
  TrialWaveFunction::evaluateDeltaLog(ParticleSet& P) {
    P.G = 0.0;
    P.L = 0.0;
    ValueType logpsi(0.0);
    PhaseValue=0.0;
    vector<OrbitalBase*>::iterator it(Z.begin());
    vector<OrbitalBase*>::iterator it_end(Z.end());
    for(; it!=it_end; ++it)
    {
      if((*it)->Optimizable) {
        logpsi += (*it)->evaluateLog(P, P.G, P.L); 
        PhaseValue += (*it)->PhaseValue;
      }
    }
    return LogValue=real(logpsi);
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
      RealType& logpsi_fixed_r, 
      RealType& logpsi_opt_r,
      ParticleSet::ParticleGradient_t& fixedG,
      ParticleSet::ParticleLaplacian_t& fixedL) {
    P.G = 0.0;
    P.L = 0.0;
    fixedG = 0.0;
    fixedL = 0.0;
    ValueType logpsi_fixed(0.0);
    ValueType logpsi_opt(0.0);
    vector<OrbitalBase*>::iterator it(Z.begin());
    vector<OrbitalBase*>::iterator it_end(Z.end());
    for(; it!=it_end; ++it)
    {
      if((*it)->Optimizable) 
        logpsi_opt += (*it)->evaluateLog(P, P.G, P.L); 
      else
        logpsi_fixed += (*it)->evaluateLog(P, fixedG, fixedL); 
    }
    P.G += fixedG; 
    P.L += fixedL;

    logpsi_fixed_r = real(logpsi_fixed);
    logpsi_opt_r = real(logpsi_opt);
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
    ValueType psi(1.0);
    for(int i=0; i<Z.size(); i++) {
      psi *= Z[i]->evaluate(P, P.G, P.L);
    }
    //for(int iat=0; iat<P.getTotalNum(); iat++)
    // cout << P.G[iat] << " " << P.L[iat] << endl;
    LogValue = evaluateLogAndPhase(psi,PhaseValue);
    return psi;
  }
  
  TrialWaveFunction::RealType
  TrialWaveFunction::ratio(ParticleSet& P,int iat) {
    ValueType r(1.0);
    for(int i=0,ii=V_TIMER; i<Z.size(); ++i,ii+=TIMER_SKIP) 
    {
      myTimers[ii]->start();
      r *= Z[i]->ratio(P,iat);
      myTimers[ii]->stop();
    }
#if defined(QMC_COMPLEX)
    //return std::exp(evaluateLogAndPhase(r,PhaseValue));
    RealType logr=evaluateLogAndPhase(r,PhaseValue);
    return std::exp(logr)*std::cos(PhaseValue);
#else
    PhaseValue=evaluatePhase(r);
    return real(r);
#endif
  }

  void   
  TrialWaveFunction::update(ParticleSet& P,int iat) 
  {
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
  TrialWaveFunction::RealType 
  TrialWaveFunction::ratio(ParticleSet& P, int iat, 
			   ParticleSet::ParticleGradient_t& dG,
			   ParticleSet::ParticleLaplacian_t& dL) 
  {
    dG = 0.0;
    dL = 0.0;
    ValueType r(1.0);
    for(int i=0, ii=VGL_TIMER; i<Z.size(); ++i, ii+=TIMER_SKIP) 
    {
      myTimers[ii]->start();
      r *= Z[i]->ratio(P,iat,dG,dL);
      myTimers[ii]->stop();
    }

#if defined(QMC_COMPLEX)
    return std::exp(evaluateLogAndPhase(r,PhaseValue));
#else
    PhaseValue=evaluatePhase(r);
    return real(r);
#endif
  }

  /** restore to the original state
   * @param iat index of the particle with a trial move
   *
   * The proposed move of the iath particle is rejected.
   * All the temporary data should be restored to the state prior to the move.
   */
  void 
  TrialWaveFunction::rejectMove(int iat) {
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
  TrialWaveFunction::acceptMove(ParticleSet& P,int iat) {
    for(int i=0,ii=ACCEPT_TIMER; i<Z.size(); i++, ii+=TIMER_SKIP) {
      myTimers[ii]->start();
      Z[i]->acceptMove(P,iat);
      myTimers[ii]->stop();
    }
  }

  //void TrialWaveFunction::resizeByWalkers(int nwalkers){
  //  for(int i=0; i<Z.size(); i++) Z[i]->resizeByWalkers(nwalkers);
  //}
  
  void TrialWaveFunction::checkInVariables(opt_variables_type& active) 
  {
    for(int i=0; i<Z.size(); i++) Z[i]->checkInVariables(active);
  }

  void TrialWaveFunction::checkOutVariables(const opt_variables_type& active) 
  {
    for(int i=0; i<Z.size(); i++) Z[i]->checkOutVariables(active);
  }

  void TrialWaveFunction::resetParameters(const opt_variables_type& active)
  {
    for(int i=0; i<Z.size(); i++) Z[i]->resetParameters(active);
  }

  void TrialWaveFunction::reportStatus(ostream& os)
  {
    for(int i=0; i<Z.size(); i++) Z[i]->reportStatus(os);
  }

  TrialWaveFunction::RealType
  TrialWaveFunction::registerData(ParticleSet& P, PooledData<RealType>& buf) {
    delta_G.resize(P.getTotalNum());
    delta_L.resize(P.getTotalNum());

    P.G = 0.0;
    P.L = 0.0;

    ValueType logpsi(0.0);
    PhaseValue=0.0;
    vector<OrbitalBase*>::iterator it(Z.begin());
    vector<OrbitalBase*>::iterator it_end(Z.end());
    for(;it!=it_end; ++it)
    {
      logpsi += (*it)->registerData(P,buf);
      PhaseValue += (*it)->PhaseValue;
    }

    LogValue=real(logpsi);

    //append current gradients and laplacians to the buffer
    NumPtcls = P.getTotalNum();
    TotalDim = PosType::Size*NumPtcls;

    buf.add(&(P.G[0][0]), &(P.G[0][0])+TotalDim);
    buf.add(&(P.L[0]), &(P.L[P.getTotalNum()]));

    return LogValue;
//     cout << "Registering gradients and laplacians " << endl;
//     for(int i=0; i<P.getLocalNum(); i++) {
//       cout << P.G[i] << " " << P.L[i] << endl;
//     }
  }

  TrialWaveFunction::RealType
  TrialWaveFunction::updateBuffer(ParticleSet& P, PooledData<RealType>& buf,
      bool fromscratch) {
    P.G = 0.0;
    P.L = 0.0;

    ValueType logpsi(0.0);
    PhaseValue=0.0;
    vector<OrbitalBase*>::iterator it(Z.begin());
    vector<OrbitalBase*>::iterator it_end(Z.end());
    for(int ii=1; it!=it_end; ++it,ii+=TIMER_SKIP)
    {
      myTimers[ii]->start();
      logpsi += (*it)->updateBuffer(P,buf,fromscratch);
      PhaseValue += (*it)->PhaseValue;
      myTimers[ii]->stop();
    }

    LogValue=real(logpsi);
    buf.put(&(P.G[0][0]), &(P.G[0][0])+TotalDim);
    buf.put(&(P.L[0]), &(P.L[0])+NumPtcls);
    return LogValue;
  }

  void TrialWaveFunction::copyFromBuffer(ParticleSet& P, PooledData<RealType>& buf) {

    for(int i=0; i<Z.size(); i++) Z[i]->copyFromBuffer(P,buf);

    //get the gradients and laplacians from the buffer
    buf.get(&(P.G[0][0]), &(P.G[0][0])+TotalDim);
    buf.get(&(P.L[0]), &(P.L[0])+NumPtcls);

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
    for(; it!=it_end; ++it)
    {
      (*it)->dumpToBuffer(P,buf);
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
    for(; it!=it_end; ++it)
      (*it)->dumpFromBuffer(P,buf);
  }

  TrialWaveFunction::RealType
  TrialWaveFunction::evaluateLog(ParticleSet& P, PooledData<RealType>& buf) 
  {
    LogValue=0.0;
    PhaseValue=0.0;
    for(int i=0; i<Z.size(); i++) 
    {
      LogValue += Z[i]->evaluateLog(P,buf);
      PhaseValue += Z[i]->PhaseValue;
    }
    buf.put(&(P.G[0][0]), &(P.G[0][0])+TotalDim);
    buf.put(&(P.L[0]), &(P.L[0])+NumPtcls);
    return real(LogValue);
  }

  //TrialWaveFunction::RealType
  //TrialWaveFunction::evaluate(ParticleSet& P, PooledData<RealType>& buf) {

  //  ValueType psi(1.0);
  //  for(int i=0; i<Z.size(); i++) psi *= Z[i]->evaluate(P,buf);
  //  buf.put(&(P.G[0][0]), &(P.G[0][0])+TotalDim);
  //  buf.put(&(P.L[0]), &(P.L[0])+NumPtcls);
  //  return real(psi);
  //}
  //bool TrialWaveFunction::hasSPOSet(const string& aname) {
  //  return false;
  //  //bool notfoundit=true;
  //  //vector<OhmmsElementBase*>::iterator it(SPOSet.begin());
  //  //vector<OhmmsElementBase*>::iterator it_end(SPOSet.end());
  //  //while(notfoundit && it != it_end) {
  //  //  if((*it)->getName() == aname) notfoundit=false;
  //  //  ++it;
  //  //}
  //  //return !notfoundit;
  //}

  //OhmmsElementBase* 
  //TrialWaveFunction::getSPOSet(const string& aname) {
  //  //bool notfoundit=true;
  //  //vector<OhmmsElementBase*>::iterator it(SPOSet.begin());
  //  //vector<OhmmsElementBase*>::iterator it_end(SPOSet.end());
  //  //while(notfoundit && it != it_end) {
  //  //  if((*it)->getName() == aname) return *it;
  //  //  ++it;
  //  //}
  //  return 0;
  //}

  //void TrialWaveFunction::addSPOSet(OhmmsElementBase* spo) {
  //  //SPOSet.push_back(spo);
  //}

  bool TrialWaveFunction::get(std::ostream& ) const {
    return true;
  }

  bool TrialWaveFunction::put(std::istream& ) {
    return true;
  }

  bool TrialWaveFunction::put(xmlNodePtr cur) {
    return true;
  }

  void TrialWaveFunction::reset() 
  {
  }

  void TrialWaveFunction::reverse() 
  {
    Ordered=false;
    //vector<OrbitalBase*> zcopy(Z);
    //int n=Z.size()-1;
    //for(int i=0; i<Z.size(); ++i) Z[n-i]=zcopy[i]; 
  }

  TrialWaveFunction* TrialWaveFunction::makeClone(ParticleSet& tqp)  const
  {
    TrialWaveFunction* myclone = new TrialWaveFunction(myComm);
    for(int i=0; i<Z.size(); ++i)
    {
      myclone->addOrbital(Z[i]->makeClone(tqp),"dummy");
    }
    for(int i=0; i<myTimers.size(); i++)
      myclone->myTimers[i]->set_name(myTimers[i]->get_name());
    myclone->OneOverM=OneOverM;
    return myclone;

  }


  void TrialWaveFunction::evaluateDerivatives(ParticleSet& P, RealType ke0, 
					      const opt_variables_type& optvars,
					      vector<RealType>& dlogpsi,
					      vector<RealType>& dhpsioverpsi)
  {
    for(int i=0; i<Z.size(); i++) {
      if (Z[i]->dPsi) 
	(Z[i]->dPsi)->evaluateDerivatives( P, ke0, optvars, dlogpsi, dhpsioverpsi); 
      else 
	Z[i]->evaluateDerivatives( P, ke0, optvars, dlogpsi, dhpsioverpsi);

    }
    //orbitals do not know about mass of particle.
    for(int i=0;i<dhpsioverpsi.size();i++) dhpsioverpsi[i]*=OneOverM;
  }

  TrialWaveFunction::RealType
  TrialWaveFunction::KECorrection() const
  {
    RealType sum = 0.0;
    for (int i=0; i<Z.size(); ++i)
      sum += Z[i]->KECorrection();
    return sum;
  }


  ////////////////////////////////
  // Vectorized member fuctions //
  ///////////////////////////////
  void
  TrialWaveFunction::freeGPUmem()
  {
    for (int i=Z.size()-1; i>=0; i--)
      Z[i]->freeGPUmem();
  }

  void
  TrialWaveFunction::recompute
  (MCWalkerConfiguration &W, bool firstTime)
  {
    for (int i=0,ii=RECOMPUTE_TIMER; i<Z.size(); i++,ii+=TIMER_SKIP) {
      myTimers[ii]->start();
      Z[i]->recompute(W, firstTime);
      myTimers[ii]->stop();
    }
  }


  void
  TrialWaveFunction::reserve
  (PointerPool<gpu::device_vector<CudaRealType> > &pool,
   bool onlyOptimizable)
  {
    for(int i=0; i<Z.size(); i++) 
      if (!onlyOptimizable || Z[i]->Optimizable)
	Z[i]->reserve(pool);
  }

  void 
  TrialWaveFunction::evaluateLog (MCWalkerConfiguration &W,
				  vector<RealType> &logPsi)
  {
    for (int iw=0; iw<logPsi.size(); iw++)
      logPsi[iw] = RealType();
    for (int i=0; i<Z.size(); i++) 
      Z[i]->addLog (W, logPsi);
  }


  void
  TrialWaveFunction::getGradient (MCWalkerConfiguration &W, int iat,
				  vector<GradType> &grad)
  {
    for (int iw=0; iw<grad.size(); iw++)
      grad[iw] = GradType();

    for(int i=0; i<Z.size(); i++) 
      Z[i]->addGradient(W, iat, grad);
  }

  void
  TrialWaveFunction::ratio (MCWalkerConfiguration &W, int iat,
			    vector<ValueType> &psi_ratios,
			    vector<GradType> &newG)
  {

    for (int iw=0; iw<W.WalkerList.size(); iw++) {
      psi_ratios[iw] = 1.0;
      newG[iw] = GradType();
    }
    for (int i=0,ii=0; i<Z.size(); i++,ii+=TIMER_SKIP) {
      myTimers[ii]->start();
      Z[i]->ratio (W, iat, psi_ratios, newG);
      myTimers[ii]->stop();
    }
  }


  void
  TrialWaveFunction::ratio (MCWalkerConfiguration &W, int iat,
			    vector<ValueType> &psi_ratios,
			    vector<GradType> &newG, vector<ValueType> &newL)
  {
    for (int iw=0; iw<W.WalkerList.size(); iw++) {
      psi_ratios[iw] = 1.0;
      newG[iw] = GradType();
      newL[iw] = ValueType();
    }

    for (int i=0,ii=1; i<Z.size(); i++,ii+=TIMER_SKIP) {
      myTimers[ii]->start();
      Z[i]->ratio (W, iat, psi_ratios, newG, newL);
      myTimers[ii]->stop();
    }
  }

  void
  TrialWaveFunction::ratio (vector<Walker_t*> &walkers, vector<int> &iatList,
			    vector<PosType> &rNew, vector<ValueType> &psi_ratios,
			    vector<GradType> &newG, vector<ValueType> &newL)
  {
    for (int iw=0; iw<walkers.size(); iw++) {
      psi_ratios[iw] = 1.0;
      newG[iw] = GradType();
      newL[iw] = ValueType();
    }
    for (int i=0,ii=1; i<Z.size(); i++,ii+=TIMER_SKIP) {
      myTimers[ii]->start();
      Z[i]->ratio (walkers, iatList, rNew, psi_ratios, newG, newL);
      myTimers[ii]->stop();
    } 
  }

  void 
  TrialWaveFunction::ratio (MCWalkerConfiguration &W, int iat,
			    vector<ValueType> &psi_ratios)
  {
    for (int iw=0; iw<W.WalkerList.size(); iw++) 
      psi_ratios[iw] = 1.0;
    for (int i=0,ii=V_TIMER; i<Z.size(); i++,ii+=TIMER_SKIP) {
      myTimers[ii]->start();
      Z[i]->ratio (W, iat, psi_ratios);
      myTimers[ii]->stop();
    } 
  }

  void
  TrialWaveFunction::update (vector<Walker_t*> &walkers, int iat)
  {
    for (int i=0,ii=ACCEPT_TIMER; i<Z.size(); i++,ii+=TIMER_SKIP) {
      myTimers[ii]->start();
      Z[i]->update(walkers, iat);
      myTimers[ii]->stop();
    } 
  }

  void
  TrialWaveFunction::update (const vector<Walker_t*> &walkers, 
			     const vector<int> &iatList)
  {
    for (int i=0,ii=ACCEPT_TIMER; i<Z.size(); i++,ii+=TIMER_SKIP) {
      myTimers[ii]->start();
      Z[i]->update(walkers, iatList);
      myTimers[ii]->stop();
    } 
  }


  void   
  TrialWaveFunction::gradLapl (MCWalkerConfiguration &W, GradMatrix_t &grads,
			       ValueMatrix_t &lapl)
  {
    for (int i=0; i<grads.rows(); i++)
      for (int j=0; j<grads.cols(); j++) {
	grads(i,j) = GradType();
	lapl(i,j)  = ValueType();
      }
    for (int i=0,ii=VGL_TIMER; i<Z.size(); i++,ii+=TIMER_SKIP) {
      myTimers[ii]->start();
      Z[i]->gradLapl (W, grads, lapl);
      myTimers[ii]->stop();
    }     
    for (int iw=0; iw<W.WalkerList.size(); iw++) 
      for (int ptcl=0; ptcl<grads.cols(); ptcl++) {
	W[iw]->Grad[ptcl] = grads(iw, ptcl);
	W[iw]->Lap[ptcl]  = lapl(iw, ptcl);
      }
  }

  void 
  TrialWaveFunction::NLratios (MCWalkerConfiguration &W,  
			       vector<NLjob> &jobList,
			       vector<PosType> &quadPoints, 
			       vector<ValueType> &psi_ratios)
  {
    for (int i=0; i<psi_ratios.size(); i++)
      psi_ratios[i] = 1.0;
    for (int i=0,ii=NL_TIMER; i<Z.size(); i++,ii+=TIMER_SKIP) {
      myTimers[ii]->start();
      Z[i]->NLratios(W, jobList, quadPoints, psi_ratios);
      myTimers[ii]->stop();
    } 
  }

  void 
  TrialWaveFunction::NLratios (MCWalkerConfiguration &W,
			       gpu::device_vector<CUDA_PRECISION*> &Rlist,
			       gpu::device_vector<int*>            &ElecList,
			       gpu::device_vector<int>             &NumCoreElecs,
			       gpu::device_vector<CUDA_PRECISION*> &QuadPosList,
			       gpu::device_vector<CUDA_PRECISION*> &RatioList,
			       int numQuadPoints)
  {
    for (int i=0,ii=NL_TIMER; i<Z.size(); i++,ii+=TIMER_SKIP) {
      myTimers[ii]->start();
      Z[i]->NLratios(W, Rlist, ElecList, NumCoreElecs,
		     QuadPosList, RatioList, numQuadPoints);
      myTimers[ii]->stop();
    } 
  }

  void 
  TrialWaveFunction::evaluateDeltaLog(MCWalkerConfiguration &W, 
				      vector<RealType>& logpsi_opt)
  {
    for (int iw=0; iw<logpsi_opt.size(); iw++)
      logpsi_opt[iw] = RealType();
    for (int i=0,ii=RECOMPUTE_TIMER; i<Z.size(); i++,ii+=TIMER_SKIP) {
      myTimers[ii]->start();
      if (Z[i]->Optimizable)
	Z[i]->addLog(W, logpsi_opt);
      myTimers[ii]->stop();	
    }
  }


 void 
 TrialWaveFunction::evaluateDeltaLog (MCWalkerConfiguration &W,  
				      vector<RealType>& logpsi_fixed,
				      vector<RealType>& logpsi_opt,  
				      GradMatrix_t&  fixedG,
				      ValueMatrix_t& fixedL)
 {
   for (int iw=0; iw<logpsi_fixed.size(); iw++) {
     logpsi_opt[iw] = RealType();
     logpsi_fixed[iw] = RealType();
   }
   fixedG = GradType();
   fixedL = RealType();

   // First, sum optimizable part, using fixedG and fixedL as temporaries
   for (int i=0,ii=RECOMPUTE_TIMER; i<Z.size(); i++,ii+=TIMER_SKIP) {
     myTimers[ii]->start();
     if (Z[i]->Optimizable) {
       Z[i]->addLog(W, logpsi_opt);
       Z[i]->gradLapl(W, fixedG, fixedL);
     }
   }
   for (int iw=0; iw<W.WalkerList.size(); iw++) 
     for (int ptcl=0; ptcl<fixedG.cols(); ptcl++) {
       W[iw]->Grad[ptcl] = fixedG(iw, ptcl);
       W[iw]->Lap[ptcl]  = fixedL(iw, ptcl);
     }
   
   // Reset them, then accumulate the fixe part
   fixedG = GradType();
   fixedL = RealType();
  
   for (int i=0,ii=NL_TIMER; i<Z.size(); i++,ii+=TIMER_SKIP) {
     if (!Z[i]->Optimizable) {
       Z[i]->addLog(W, logpsi_fixed);
       Z[i]->gradLapl(W, fixedG, fixedL);
     }
     myTimers[ii]->stop();	
   }
   
   // Add on the fixed part to the total laplacian and gradient
   for (int iw=0; iw<W.WalkerList.size(); iw++) 
     for (int ptcl=0; ptcl<fixedG.cols(); ptcl++) {
       W[iw]->Grad[ptcl] += fixedG(iw, ptcl);
       W[iw]->Lap[ptcl]  += fixedL(iw, ptcl);
     }
 }

 void 
 TrialWaveFunction::evaluateOptimizableLog (MCWalkerConfiguration &W,  
					    vector<RealType>& logpsi_opt,  
					    GradMatrix_t&  optG,
					    ValueMatrix_t& optL)
 {
   for (int iw=0; iw<W.getActiveWalkers(); iw++) 
     logpsi_opt[iw] = RealType();
   optG = GradType();
   optL = RealType();
   
   // Sum optimizable part of log Psi
   for (int i=0,ii=RECOMPUTE_TIMER; i<Z.size(); i++,ii+=TIMER_SKIP) {
     myTimers[ii]->start();
     if (Z[i]->Optimizable) {
       Z[i]->addLog(W, logpsi_opt);
       Z[i]->gradLapl(W, optG, optL);
     }
     myTimers[ii]->stop();
   }
 }
  


  void 
  TrialWaveFunction::evaluateDerivatives (MCWalkerConfiguration &W, 
					  const opt_variables_type& optvars,
					  ValueMatrix_t &dlogpsi,
					  ValueMatrix_t &dhpsioverpsi)
  {
    for (int i=0,ii=DERIVS_TIMER; i<Z.size(); i++,ii+=TIMER_SKIP) {
      myTimers[ii]->start();
      if (Z[i]->Optimizable)
	Z[i]->evaluateDerivatives(W, optvars, dlogpsi, dhpsioverpsi);
      myTimers[ii]->stop();	
    }
  }
}
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/

