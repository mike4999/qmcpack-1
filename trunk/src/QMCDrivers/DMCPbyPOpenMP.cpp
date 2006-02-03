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
#include "QMCDrivers/DMCPbyPOpenMP.h"
#include "Utilities/OhmmsInfo.h"
#include "Particle/MCWalkerConfiguration.h"
#include "Particle/HDFWalkerIO.h"
#include "ParticleBase/ParticleUtility.h"
#include "ParticleBase/RandomSeqGenerator.h"
#include "QMCApp/HamiltonianPool.h"
#include "Message/Communicate.h"
#include "Message/OpenMP.h"

namespace qmcplusplus { 

  /// Constructor.
  DMCPbyPOpenMP::DMCPbyPOpenMP(MCWalkerConfiguration& w, TrialWaveFunction& psi, QMCHamiltonian& h):
    QMCDriver(w,psi,h),
    KillNodeCrossing(0),
    PopIndex(-1), EtrialIndex(-1),
    BenchMarkRun("no"){
    RootName = "dummy";
    QMCType ="dummy";
    NumThreads=omp_get_max_threads();
    wPerNode.resize(NumThreads+1,0);

    QMCDriverMode.set(QMC_UPDATE_MODE,1);
    QMCDriverMode.set(QMC_MULTIPLE,1);

    m_param.add(KillWalker,"killnode","string");
    m_param.add(BenchMarkRun,"benchmark","string");
  }

  void DMCPbyPOpenMP::makeClones(HamiltonianPool& hpool, int np) {

    //np is ignored but will see if ever needed
    wPerNode.resize(NumThreads+1,0);

    if(wClones.size()) {
      ERRORMSG("Cannot make clones again. Use clones")
      return;
    }

    wClones.resize(NumThreads,0);
    psiClones.resize(NumThreads,0);
    hClones.resize(NumThreads,0);
    Rng.resize(NumThreads,0);

    wClones[0]=&W;
    psiClones[0]=&Psi;
    hClones[0]=&H;

    if(NumThreads == 1) {
      WARNMSG("Using a single thread with DMCPbyPOpenMP.")
      return;
    }

    hpool.clone(W,Psi,H,wClones,psiClones,hClones);

  }

  void DMCPbyPOpenMP::resetRun() {

    if(Movers.empty()) {
      Movers.resize(NumThreads,0);
      branchClones.resize(NumThreads,0);
      FairDivideLow(W.getActiveWalkers(),NumThreads,wPerNode);
#pragma omp parallel  
      {
        int ip = omp_get_thread_num();
        if(ip) {
          hClones[ip]->add2WalkerProperty(*wClones[ip]);
        }

        Rng[ip]=new RandomGenerator_t();
        Rng[ip]->init(ip,NumThreads,-1);

        Movers[ip]= new DMCPbyPUpdate(*wClones[ip],*psiClones[ip],*hClones[ip],*Rng[ip]); 
        branchClones[ip] = new BranchEngineType(*branchEngine);
        Movers[ip]->resetRun(branchClones[ip]);

        Movers[ip]->initialize(W.begin()+wPerNode[ip],W.begin()+wPerNode[ip+1]);
      }
    }
  }

  bool DMCPbyPOpenMP::run() {
    if(BenchMarkRun == "yes")  {
      app_log() << "  Running DMCPbyPOpenMP::benchMark " << endl;
      return benchMark();
    } else 
      return runDMC();
  }
  
  bool DMCPbyPOpenMP::runDMC() {

    KillNodeCrossing = (KillWalker == "yes");
    if(KillNodeCrossing) {
      app_log() << "Walkers will be killed if a node crossing is detected." << endl;
    } else {
      app_log() << "Walkers will be kept even if a node crossing is detected." << endl;
    }

    //set the collection mode for the estimator
    Estimators->setCollectionMode(branchEngine->SwapMode);

    IndexType PopIndex = Estimators->addColumn("Population");
    IndexType EtrialIndex = Estimators->addColumn("Etrial");
    Estimators->reportHeader(AppendRun);
    Estimators->reset();

    IndexType block = 0;
    RealType Eest = branchEngine->E_T;

    resetRun();

    nAcceptTot = 0;
    nRejectTot = 0;

    app_log() << "Current step " << CurrentStep << endl;

    do {

      for(int ip=0; ip<NumThreads; ip++) {
        Movers[ip]->resetBlock();
      } 

      FairDivideLow(W.getActiveWalkers(),NumThreads,wPerNode);

      Estimators->startBlock();
      IndexType step = 0;
      IndexType pop_acc=0; 
      do {
       
#pragma omp parallel  
        {
          int ip = omp_get_thread_num();
          Movers[ip]->resetEtrial(Eest);
        ////default is killing
        //if(KillNodeCrossing) 
        //  Mover->advanceKillNodeCrossing(W.begin(), W.end());
        //else
          Movers[ip]->advanceRejectNodeCrossing(W.begin()+wPerNode[ip], W.begin()+wPerNode[ip+1]);
        }

        ++step; ++CurrentStep;
        Estimators->accumulate(W);

        int cur_pop = branchEngine->branch(CurrentStep,W, branchClones);

        pop_acc += cur_pop;
        Eest = branchEngine->CollectAndUpdate(cur_pop, Eest); 

        FairDivideLow(W.getActiveWalkers(),NumThreads,wPerNode);

        for(int ip=0; ip<NumThreads; ip++) Movers[ip]->resetEtrial(Eest); 

        if(CurrentStep%100 == 0) {
#pragma omp parallel  
          {
            int ip = omp_get_thread_num();
            Movers[ip]->updateWalkers(W.begin()+wPerNode[ip], W.begin()+wPerNode[ip+1]);
          }
        }
      } while(step<nSteps);
      
      nAccept=0;
      nReject=0;
      for(int ip=0; ip<NumThreads; ip++) {
        nAccept+=Movers[ip]->nAccept;
        nReject+=Movers[ip]->nReject;
      }
      Estimators->stopBlock(static_cast<RealType>(nAccept)/static_cast<RealType>(nAccept+nReject));

      nAcceptTot += nAccept;
      nRejectTot += nReject;
      
      //update estimator
      Estimators->setColumn(PopIndex,static_cast<RealType>(pop_acc)/static_cast<RealType>(nSteps));
      Estimators->setColumn(EtrialIndex,Eest); 
      Eest = Estimators->average(0);
      RealType totmoves=1.0/static_cast<RealType>(step*W.getActiveWalkers());

      //Need MPI-IO
      //app_log() 
      //  << setw(4) << block 
      //  << setw(20) << static_cast<RealType>(nAllRejected)*totmoves
      //  << setw(20) << static_cast<RealType>(nNodeCrossing)*totmoves << endl;
      block++;

      recordBlock(block);

    } while(block<nBlocks);
    
    Estimators->finalize();
    return true;
  }


  bool DMCPbyPOpenMP::benchMark() { 
    
    //set the collection mode for the estimator
    Estimators->setCollectionMode(branchEngine->SwapMode);

    IndexType PopIndex = Estimators->addColumn("Population");
    IndexType EtrialIndex = Estimators->addColumn("Etrial");
    Estimators->reportHeader(AppendRun);
    Estimators->reset();

    IndexType block = 0;
    RealType Eest = branchEngine->E_T;

    resetRun();

    for(int ip=0; ip<NumThreads; ip++) {
      char fname[16];
      sprintf(fname,"test.%i",ip);
      ofstream fout(fname);
    }

    for(int istep=0; istep<nSteps; istep++) {

      FairDivideLow(W.getActiveWalkers(),NumThreads,wPerNode);
#pragma omp parallel  
      {
        int ip = omp_get_thread_num();
        Movers[ip]->benchMark(W.begin()+wPerNode[ip],W.begin()+wPerNode[ip+1],ip);
      }
    }
    return true;
  }
  
  bool 
  DMCPbyPOpenMP::put(xmlNodePtr q){ 
    //nothing to do
    return true;
  }

}

/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
