//////////////////////////////////////////////////////////////////
// (c) Copyright 2009 - by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   Ken Esler
//   National Center for Supercomputing Applications &
//   Materials Computation Center
//   University of Illinois, Urbana-Champaign
//   Urbana, IL 61801
//   e-mail: kpesler@gmail.com
//
// Supported by 
//   National Center for Supercomputing Applications, UIUC
//   Materials Computation Center, UIUC
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
#include "QMCDrivers/QMCCostFunctionCUDA.h"
#include "Particle/MCWalkerConfiguration.h"
#include "QMCWaveFunctions/TrialWaveFunction.h"
#include "Particle/HDFWalkerInputCollect.h"
#include "Message/CommOperators.h"

namespace qmcplusplus {

  QMCCostFunctionCUDA::QMCCostFunctionCUDA
  ( MCWalkerConfiguration& w, TrialWaveFunction& psi, 
    QMCHamiltonian& h, HamiltonianPool& hpool):
    QMCCostFunctionBase(w,psi,h), CloneManager(hpool)
  { 
  }


  /** Clean up the vector */
  QMCCostFunctionCUDA::~QMCCostFunctionCUDA() {
  }

  /**  Perform the correlated sampling algorthim.
   */
  QMCCostFunctionCUDA::Return_t QMCCostFunctionCUDA::correlatedSampling() 
  {
    Return_t wgt_tot=0.0;
    Return_t wgt_tot2=0.0;
    Return_t NSm1 = 1.0/NumSamples;
    int nw = W.getActiveWalkers();

    //#pragma omp parallel reduction(+:wgt_tot)
    Return_t eloc_new;
    //int totalElements=W.getTotalNum()*OHMMS_DIM;
    MCWalkerConfiguration::iterator it(W.begin());
    MCWalkerConfiguration::iterator it_end(W.end());
    int iw=0;
    int numParams = OptVariablesForPsi.size();
    int numPtcl   = W.getTotalNum();

    vector<RealType> logpsi_new(nw), logpsi_fixed(nw), KE(nw);
    TrialWaveFunction::ValueMatrix_t d_logpsi_dalpha(nw, numParams), d_hpsioverpsi_dalpha(nw, numParams);
    TrialWaveFunction::GradMatrix_t  fixedG(nw, numPtcl);
    TrialWaveFunction::ValueMatrix_t fixedL(nw, numPtcl);
    TrialWaveFunction::GradMatrix_t  newG(nw, numPtcl);
    TrialWaveFunction::ValueMatrix_t newL(nw, numPtcl);
    Psi.evaluateOptimizableLog(W, logpsi_new, newG, newL);
    RealType factor = samplePsi2 ? 2.0 : 1.0;
    // Add optimizable and non-optimizable gradients and Laplacians



    for (int iw=0; iw<nw; iw++) {
      ParticleSet::Walker_t& walker = *(W[iw]);
      for (int iat=0; iat<numPtcl; iat++) {
	walker.Grad[iat] = newG(iw, iat) + dlogPsi_fixed(iw, iat);
	walker.Lap[iat]  = newL(iw, iat) + d2logPsi_fixed(iw, iat);	
      }
      // W.R = walker.R;
      // W.update();
      // Return_t logpsi=Psi.evaluateDeltaLog(W);
      // fprintf (stderr, "iw=%4d deltaLog CPU = %12.6e  deltaLog GPU = %12.6e\n",
      // 	       iw, logpsi, logpsi_new[iw]);
    }
    W.copyWalkersToGPU(true);

    H_KE.evaluate (W, KE);
    Psi.evaluateDerivatives(W, OptVariablesForPsi,
			    d_logpsi_dalpha, d_hpsioverpsi_dalpha);
    
    /*    for (int iw=0; iw<nw; iw++) {
      ParticleSet::Walker_t& walker = *(W[iw]);
      W.R = walker.R;
      W.G = walker.Grad;  
      W.update();
      //Return_t logpsi=Psi.evaluateDeltaLog(W);
      vector<RealType> d_alpha_logpsi_CPU(numParams);
      vector<RealType> d_alpha_hpsioverpsi_CPU(numParams);

      Psi.evaluateDerivatives(W,0.0,OptVariablesForPsi,d_alpha_logpsi_CPU,
			      d_alpha_hpsioverpsi_CPU);
      fprintf (stderr, "iw=%4d GPU:  ", iw);
      for (int ip=0; ip<numParams; ip++)
	fprintf (stderr, "%10.4f ", d_hpsioverpsi_dalpha(iw, ip));
      fprintf (stderr, "\niw=%4d CPU:  ", iw);
      for (int ip=0; ip<numParams; ip++)
	fprintf (stderr, "%10.4f ", d_alpha_hpsioverpsi_CPU[ip]);
      fprintf (stderr, "\n");

      // fprintf (stderr, "iw=%4d GPU:  ", iw);
      // for (int ip=0; ip<numParams; ip++)
      // 	fprintf (stderr, "%10.4f ", d_logpsi_dalpha(iw, ip));
      // fprintf (stderr, "\niw=%4d CPU:  ", iw);
      // for (int ip=0; ip<numParams; ip++)
      // 	fprintf (stderr, "%10.4f ", d_alpha_logpsi_CPU[ip]);
      // fprintf (stderr, "\n");
      }*/

//     FILE *fout = fopen ("VarDerivs.dat", "w");
//     for (int iw=0; iw<nw; iw++) {
//       for (int iv=0; iv<numParams; iv++)
// 	fprintf (fout, "%12.6e %12.6e ", d_logpsi_dalpha(iw,iv), d_hpsioverpsi_dalpha(iw,iv));
//       fprintf (fout, "\n");
//     }
//     fclose(fout);
    for (int iw=0; iw<nw; iw++) {
      ParticleSet::Walker_t& walker = *(W[iw]);
      Return_t* restrict saved= &(Records(iw,0));
      RealType weight = std::exp(factor*(logpsi_new[iw] - saved[LOGPSI_FREE]));
      RealType eloc_new = KE[iw] + saved[ENERGY_FIXED];
      saved[ENERGY_NEW] = eloc_new;
      saved[REWEIGHT]   = weight;
      for (int ip=0; ip<NumOptimizables; ip++) {
	TempDerivRecords[iw][ip]  =      d_logpsi_dalpha(iw,ip);
	TempHDerivRecords[iw][ip] = d_hpsioverpsi_dalpha(iw,ip);
      }
    }

//     for (; it!= it_end;++it,++iw) {
//       ParticleSet::Walker_t& thisWalker(**it);
//       W.R=thisWalker.R;
//       W.update();
//       Return_t logpsi = Psi.evaluateDeltaLog(W);
//       W.G += *dLogPsi[iw];
//       W.L += *d2LogPsi[iw];
      
//       Return_t* restrict saved = &(Records(iw,0));
      
//       RealType KEtemp = H_KE.evaluate(W);
//       eloc_new = KEtemp + saved[ENERGY_FIXED];
//       Return_t weight;
//       if (samplePsi2) weight = std::exp(2.0*(logpsi-saved[LOGPSI_FREE])) ;
//       else weight = std::exp( logpsi-saved[LOGPSI_FREE] ) ;
//       if (KEtemp<MinKE) weight=0.0;
//       saved[ENERGY_NEW]=eloc_new;
//       saved[REWEIGHT]=weight;
      
//       vector<Return_t> &Dsaved=  TempDerivRecords[iw];
//       vector<Return_t> &HDsaved= TempHDerivRecords[iw];
//        Psi.evaluateDerivatives
// 	 (W,KEtemp,OptVariablesForPsi,Dsaved,HDsaved);
      
//       wgt_tot+=weight;
//       wgt_tot2+=weight*weight;
//     }
    
    //this is MPI barrier
    //OHMMS::Controller->barrier();
    //collect the total weight for normalization and apply maximum weight



    myComm->allreduce(wgt_tot);
    myComm->allreduce(wgt_tot2);

    Return_t wgtnorm = (1.0*NumSamples)/wgt_tot;
    wgt_tot=0.0;
    for (int iw=0; iw<nw;iw++) {
      Return_t* restrict saved = Records[iw];
      saved[REWEIGHT] = std::min(saved[REWEIGHT]*wgtnorm,MaxWeight) ;
      wgt_tot+= saved[REWEIGHT];
    }
    myComm->allreduce(wgt_tot);
    
    for (int i=0; i<SumValue.size(); i++) SumValue[i]=0.0;
    CSWeight=wgt_tot=1.0/wgt_tot;
    for (int iw=0; iw<nw;iw++) {
      const Return_t* restrict saved = &(Records(iw,0));
      Return_t weight=saved[REWEIGHT]*wgt_tot;
      Return_t eloc_new=saved[ENERGY_NEW];
      Return_t delE=std::pow(abs(eloc_new-EtargetEff),PowerE);
      SumValue[SUM_E_BARE]    += eloc_new;
      SumValue[SUM_ESQ_BARE]  += eloc_new*eloc_new;
      SumValue[SUM_ABSE_BARE] += delE;
      SumValue[SUM_E_WGT]     += eloc_new*saved[REWEIGHT];
      SumValue[SUM_ESQ_WGT]   += eloc_new*eloc_new*saved[REWEIGHT];
      SumValue[SUM_ABSE_WGT]  += delE*saved[REWEIGHT];
      SumValue[SUM_WGT]       += saved[REWEIGHT];
      SumValue[SUM_WGTSQ]     += saved[REWEIGHT]*saved[REWEIGHT];
    }
    //collect everything
    myComm->allreduce(SumValue);
    
    RealType effective_walkers = SumValue[SUM_WGT]*SumValue[SUM_WGT]/SumValue[SUM_WGTSQ];
    cerr << "EffectiveWalker = " << effective_walkers << endl;
    return SumValue[SUM_WGT]*SumValue[SUM_WGT]/SumValue[SUM_WGTSQ];
  }

  void 
  QMCCostFunctionCUDA::getConfigurations(const string& aroot) {
    app_log() << "   Loading configuration from MCWalkerConfiguration::SampleStack " << endl;
    app_log() << "    number of walkers before load " << W.getActiveWalkers() << endl;
    
    OhmmsInfo::Log->turnoff();
    OhmmsInfo::Warn->turnoff();
    W.loadEnsemble();
    OhmmsInfo::Log->reset();
    OhmmsInfo::Warn->reset();

    app_log() << "    number of walkers after load: "
	      << W.getActiveWalkers() << endl;

    if(dLogPsi.size() != W.getActiveWalkers())
    {
      delete_iter(dLogPsi.begin(),dLogPsi.end());
      delete_iter(d2LogPsi.begin(),d2LogPsi.end());
      int nptcl=W.getTotalNum();
      int nwtot=W.getActiveWalkers();
      dLogPsi.resize(nwtot,0);
      d2LogPsi.resize(nwtot,0);
      for(int i=0; i<nwtot; ++i) dLogPsi[i] =new ParticleGradient_t (nptcl);
      for(int i=0; i<nwtot; ++i) d2LogPsi[i]=new ParticleLaplacian_t(nptcl);
    }
    PointerPool<Walker_t::cuda_Buffer_t > pool;
    // Reserve memory only for optimizable parts of the wavefunction
    Psi.reserve (pool, true);
    app_log() << "Each walker requires " << pool.getTotalSize() * sizeof(CudaRealType)
	      << " bytes in GPU memory.\n";
    for (int iw=0; iw<W.WalkerList.size(); iw++) {
      Walker_t &walker = *(W.WalkerList[iw]);
      pool.allocate(walker.cuda_DataSet);
    }
    W.copyWalkersToGPU();
    W.updateLists_GPU();
    app_log() << "Successfully allocated walkers.\n";
  }



  /** evaluate everything before optimization */
  void 
  QMCCostFunctionCUDA::checkConfigurations() {
    RealType et_tot=0.0;
    RealType e2_tot=0.0;

    int numWalkers=W.getActiveWalkers();
    Records.resize(numWalkers,6);
    int nptcl = W.getTotalNum();
    dlogPsi_opt.resize (numWalkers, nptcl);
    d2logPsi_opt.resize(numWalkers, nptcl);
    dlogPsi_fixed.resize (numWalkers, nptcl);
    d2logPsi_fixed.resize(numWalkers, nptcl);
    
    TempHDerivRecords.resize(numWalkers,vector<Return_t>(NumOptimizables,0));
    TempDerivRecords.resize(numWalkers,vector<Return_t>(NumOptimizables,0));
    LogPsi_Derivs.resize(numWalkers, NumOptimizables);
    LocE_Derivs.resize  (numWalkers, NumOptimizables);

    Return_t e0=0.0;
    Return_t e2=0.0;
    vector<RealType> logPsi_free(numWalkers), d2logPsi_free(numWalkers);
    vector<GradType> dlogPsi_free(numWalkers);
    Psi.evaluateDeltaLog(W, logPsi_free);
    Psi.evaluateOptimizableLog (W, logPsi_free, dlogPsi_opt, d2logPsi_opt);
    int nat = W.getTotalNum();
    MCWalkerConfiguration::iterator it(W.begin()); 
    MCWalkerConfiguration::iterator it_end(W.end()); 
    for(int iw=0; it!=it_end; ++it,++iw)  {
      Walker_t& w(**it);
      RealType *prop = w.getPropertyBase();
      Return_t* restrict saved= &(Records(iw,0));
      saved[ENERGY_TOT]   = prop[LOCALENERGY];
      saved[ENERGY_FIXED] = prop[LOCALPOTENTIAL];
      saved[LOGPSI_FIXED] = prop[LOGPSI] - logPsi_free[iw];
      saved[LOGPSI_FREE]  = logPsi_free[iw];
      e0 += prop[LOCALENERGY];
      e2 += prop[LOCALENERGY] * prop[LOCALENERGY];
      for (int iat=0; iat<nat; iat++) {
	dlogPsi_fixed(iw, iat)  = w.Grad[iat] - dlogPsi_opt(iw,nat);
	d2logPsi_fixed(iw, iat) = w.Lap[iat] - d2logPsi_opt(iw,nat);
      }
    }
//     for(int iw=0; it!=it_end; ++it,++iw)  {
//       Walker_t& thisWalker(**it);
//       W.R=thisWalker.R;
//       W.update();
//       Return_t* restrict saved= &(Records(iw,0));
//       Psi.evaluateDeltaLog(W, saved[LOGPSI_FIXED], saved[LOGPSI_FREE], *dLogPsi[iw],*d2LogPsi[iw]);
//       Return_t x= H.evaluate(W);
//       e0 += saved[ENERGY_TOT] = x;
//       e2 += x*x;
//       saved[ENERGY_FIXED] = H.getLocalPotential();
//     }
    et_tot+=e0;
    e2_tot+=e2;
    
    //Need to sum over the processors
    vector<Return_t> etemp(3);
    etemp[0]=et_tot;
    etemp[1]=static_cast<Return_t>(W.getActiveWalkers());
    etemp[2]=e2_tot;
    
    myComm->allreduce(etemp);
    Etarget = static_cast<Return_t>(etemp[0]/etemp[1]);
    NumSamples = static_cast<int>(etemp[1]);
    
    app_log() << "  VMC Eavg = " << Etarget << endl;
    app_log() << "  VMC Evar = " << etemp[2]/etemp[1]-Etarget*Etarget << endl;
    app_log() << "  Total weights = " << etemp[1] << endl;
    
    setTargetEnergy(Etarget);
    
    ReportCounter=0;
  }

  void QMCCostFunctionCUDA::resetPsi(bool final_reset)
  {
    if(OptVariables.size() < OptVariablesForPsi.size())
    {
      for(int i=0; i<equalVarMap.size(); ++i)
        OptVariablesForPsi[equalVarMap[i][0]]=OptVariables[equalVarMap[i][1]];
    }
    else
      for(int i=0; i<OptVariables.size(); ++i) OptVariablesForPsi[i]=OptVariables[i];

    //cout << "######### QMCCostFunctionCUDA::resetPsi " << endl;
    //OptVariablesForPsi.print(cout);
    //cout << "-------------------------------------- " << endl;
    Psi.resetParameters(OptVariablesForPsi);
  }
  

  void 
  QMCCostFunctionCUDA::GradCost(vector<Return_t>& PGradient, vector<Return_t> PM, Return_t FiniteDiff)
  {
    if (FiniteDiff != 0) {
      QMCTraits::RealType dh=1.0/(2.0*FiniteDiff);
      for (int i=0; i<NumOptimizables ; i++) {
	for (int j=0; j<NumOptimizables; j++) OptVariables[j]=PM[j];
	OptVariables[i] = PM[i]+ FiniteDiff;
	QMCTraits::RealType CostPlus = this->Cost();
	OptVariables[i] = PM[i]- FiniteDiff;
	QMCTraits::RealType CostMinus = this->Cost();
	PGradient[i]= (CostPlus-CostMinus)*dh;
      }
    }
    else {
      for (int j=0; j<NumOptimizables; j++) OptVariables[j]=PM[j];
      resetPsi();

      //evaluate new local energies and derivatives
      NumWalkersEff=correlatedSampling();
      //Estimators::accumulate has been called by correlatedSampling
    
    
      curAvg_w = SumValue[SUM_E_WGT]/SumValue[SUM_WGT];
      Return_t curAvg2_w = curAvg_w*curAvg_w;
      curVar_w = SumValue[SUM_ESQ_WGT]/SumValue[SUM_WGT]-curAvg_w*curAvg_w;
      vector<Return_t> EDtotals(NumOptimizables,0.0);
      vector<Return_t> EDtotals_w(NumOptimizables,0.0);
      vector<Return_t> E2Dtotals_w(NumOptimizables,0.0);
      vector<Return_t> URV(NumOptimizables,0.0);
      vector<Return_t> HD_avg(NumOptimizables,0.0);
    
      Return_t wgtinv = 1.0/SumValue[SUM_WGT];
      Return_t delE_bar;
      int nw=W.getActiveWalkers();
      for (int iw=0; iw<nw;iw++) {
	const Return_t* restrict saved = &(Records(iw,0));
	Return_t weight=saved[REWEIGHT]*wgtinv;
	Return_t eloc_new=saved[ENERGY_NEW]; 
	delE_bar += weight*std::pow(abs(eloc_new-EtargetEff),PowerE);
	vector<Return_t> &Dsaved = TempDerivRecords[iw];
	vector<Return_t> &HDsaved= TempHDerivRecords[iw];
	for (int pm=0; pm<NumOptimizables;pm++) 
	  HD_avg[pm]+= HDsaved[pm];
      }
      myComm->allreduce(HD_avg);
      myComm->allreduce(delE_bar);
      for (int pm=0; pm<NumOptimizables;pm++)  HD_avg[pm] *= 1.0/static_cast<Return_t>(NumSamples);
      
      for (int iw=0; iw<nw;iw++) {
	const Return_t* restrict saved = &(Records(iw,0));
	Return_t weight=saved[REWEIGHT]*wgtinv;
	Return_t eloc_new=saved[ENERGY_NEW]; 
	Return_t delta_l = (eloc_new-curAvg_w);
	bool ltz(true);
	if (eloc_new-EtargetEff<0) ltz=false;
	Return_t delE=std::pow(abs(eloc_new-EtargetEff),PowerE);
	Return_t ddelE = PowerE*std::pow(abs(eloc_new-EtargetEff),PowerE-1);
	vector<Return_t> &Dsaved=  TempDerivRecords[iw];
	vector<Return_t> &HDsaved= TempHDerivRecords[iw];
	for (int pm=0; pm<NumOptimizables;pm++) {
	  EDtotals_w[pm] += weight*(HDsaved[pm] + 2.0*Dsaved[pm]*delta_l ); 
	  URV[pm] += 2.0*(eloc_new*HDsaved[pm] - curAvg*HD_avg[pm]);
	  if (ltz) EDtotals[pm]+= weight*( 2.0*Dsaved[pm]*(delE-delE_bar) + ddelE*HDsaved[pm]);
	  else EDtotals[pm] += weight*( 2.0*Dsaved[pm]*(delE-delE_bar) - ddelE*HDsaved[pm]);
	}
      }
      myComm->allreduce(EDtotals); 
      myComm->allreduce(EDtotals_w); 
      myComm->allreduce(URV);
      Return_t smpinv=1.0/static_cast<Return_t>(NumSamples);
        
      for (int iw=0; iw<nw;iw++) {
	const Return_t* restrict saved = &(Records(iw,0));
	Return_t weight=saved[REWEIGHT]*wgtinv;
	Return_t eloc_new=saved[ENERGY_NEW]; 
	Return_t delta_l = (eloc_new-curAvg_w);
	Return_t sigma_l = delta_l*delta_l;
	vector<Return_t> &Dsaved=  TempDerivRecords[iw];
	vector<Return_t> &HDsaved= TempHDerivRecords[iw];
	for (int pm=0; pm<NumOptimizables;pm++) 
	  E2Dtotals_w[pm] += weight*2.0*( Dsaved[pm]*(sigma_l-curVar_w) + delta_l*(HDsaved[pm]-EDtotals_w[pm]) ); 
      }
      myComm->allreduce(E2Dtotals_w); 
      
      for (int pm=0; pm<NumOptimizables;pm++)  URV[pm] *=smpinv; 
      for (int j=0; j<NumOptimizables; j++) {
	PGradient[j] = 0.0;
	if (std::fabs(w_var) > 1.0e-10)   PGradient[j] += w_var*E2Dtotals_w[j];
	if (std::fabs(w_en)  > 1.0e-10)   PGradient[j] += w_en*EDtotals_w[j];
	if (std::fabs(w_w)   > 1.0e-10)   PGradient[j] += w_w*URV[j];
	if (std::fabs(w_abs) > 1.0e-10)   PGradient[j] += w_abs*EDtotals[j];
      }
      
      IsValid=true;
      
      //         if ((CSWeight/wgtinv) < MinNumWalkers)
      if (NumWalkersEff < MinNumWalkers*NumSamples) {
	ERRORMSG("CostFunction-> Number of Effective Walkers is too small " << NumWalkersEff<< "Minimum required"<<MinNumWalkers*NumSamples)
	  //ERRORMSG("Going to stop now.")
	  IsValid=false;
      }
    }
  }

  QMCCostFunctionCUDA::Return_t 
  QMCCostFunctionCUDA::fillOverlapHamiltonianMatrix(Matrix<Return_t>& Hamiltonian, Matrix<Return_t>& Overlap )
  {

    resetPsi();
    Return_t NWE = NumWalkersEff=correlatedSampling();
    curAvg_w = SumValue[SUM_E_WGT]/SumValue[SUM_WGT];
    vector<Return_t> D_avg(NumParams(),0);
    Return_t wgtinv = 1.0/SumValue[SUM_WGT];
    int nw = W.getActiveWalkers();
    for (int iw=0; iw<nw;iw++) {
      const Return_t* restrict saved = &Records(iw,0);
      Return_t weight=saved[REWEIGHT]*wgtinv;
      vector<Return_t> &Dsaved= TempDerivRecords[iw];
      for (int pm=0; pm<NumParams();pm++) 
	D_avg[pm]+= Dsaved[pm]*weight;
    }
    myComm->allreduce(D_avg);
    ///zero out matrices before we start
    for (int pm=0; pm<NumParams()+1;pm++) 
      for (int pm2=0; pm2<NumParams()+1;pm2++) {
	Overlap(pm,pm2)=0;
	Hamiltonian(pm,pm2)=0;
      }
    
    for (int iw=0; iw<nw;iw++) {
      const Return_t* restrict saved = &Records(iw,0);
      Return_t weight=saved[REWEIGHT]*wgtinv;
      Return_t eloc_new=saved[ENERGY_NEW];
      //             Return_t ke_new=saved[ENERGY_NEW] - saved[ENERGY_FIXED];
      vector<Return_t> &Dsaved = TempDerivRecords[iw];
      vector<Return_t> &HDsaved= TempHDerivRecords[iw];
      
      for (int pm=0; pm<NumParams();pm++) {
	Return_t wfd = (Dsaved[pm]-D_avg[pm])*weight;
	Hamiltonian(0,pm+1) += weight*(HDsaved[pm] + Dsaved[pm]*(eloc_new-curAvg_w));
	Hamiltonian(pm+1,0) += wfd*(eloc_new-curAvg_w);
	for (int pm2=0; pm2<NumParams();pm2++) {
	  Hamiltonian(pm+1,pm2+1) += wfd*(HDsaved[pm2]+ Dsaved[pm2]*(eloc_new-curAvg_w) );
	  Overlap(pm+1,pm2+1) += wfd*(Dsaved[pm2]-D_avg[pm2]);
	}
      }
    }
    myComm->allreduce(Hamiltonian);
    myComm->allreduce(Overlap);
  
    
    Overlap(0,0) = 1;
    Hamiltonian(0,0) = curAvg_w ;
    return NWE;
  }
  


}
/***************************************************************************
 * $RCSfile$   $Author: esler $
 * $Revision: 1898 $   $Date: 2007-04-17 10:07:34 -0500 (Tue, 17 Apr 2007) $
 * $Id: QMCCostFunctionCUDA.cpp 1898 2007-04-17 15:07:34Z jnkim $ 
 ***************************************************************************/
