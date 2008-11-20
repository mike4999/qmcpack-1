//////////////////////////////////////////////////////////////////
// (c) Copyright 2003  by Jeongnim Kim
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
#include "Utilities/OhmmsInfo.h"
#include "Particle/MCWalkerConfiguration.h"
#include "Particle/DistanceTableData.h"
#include "Particle/DistanceTable.h"
#include "ParticleBase/RandomSeqGenerator.h"
#include "Message/Communicate.h"
#include "Message/CommOperators.h"
#include <map>

#ifdef QMC_CUDA
  #include "Particle/accept_kernel.h"
#endif

namespace qmcplusplus {

MCWalkerConfiguration::MCWalkerConfiguration(): 
OwnWalkers(true),ReadyForPbyP(false),UpdateMode(Update_Walker),Polymer(0),
  MaxSamples(10),CurSampleCount(0)
  {
  //move to ParticleSet
  //initPropertyList();
}

MCWalkerConfiguration::MCWalkerConfiguration(const MCWalkerConfiguration& mcw)
: ParticleSet(mcw), OwnWalkers(true), GlobalNumWalkers(mcw.GlobalNumWalkers),
  UpdateMode(Update_Walker), ReadyForPbyP(false), Polymer(0), 
  MaxSamples(mcw.MaxSamples), CurSampleCount(0)
{
  GlobalNumWalkers=mcw.GlobalNumWalkers;
  WalkerOffsets=mcw.WalkerOffsets;
  //initPropertyList();
}

///default destructor
MCWalkerConfiguration::~MCWalkerConfiguration(){
  if(OwnWalkers) destroyWalkers(WalkerList.begin(), WalkerList.end());
}


void MCWalkerConfiguration::createWalkers(int n) 
{
  if(WalkerList.empty())
  {
    while(n) {
      Walker_t* awalker=new Walker_t(GlobalNum);
      awalker->R = R;
      awalker->Drift = 0.0;
      WalkerList.push_back(awalker);
      --n;
    }
  }
  else
  {
    if(WalkerList.size()>=n)
    {
      int iw=WalkerList.size();//copy from the back
      for(int i=0; i<n; ++i)
      {
        WalkerList.push_back(new Walker_t(*WalkerList[--iw]));
      }
    }
    else
    {
      int nc=n/WalkerList.size();
      int nw0=WalkerList.size();
      for(int iw=0; iw<nw0; ++iw)
      {
        for(int ic=0; ic<nc; ++ic) WalkerList.push_back(new Walker_t(*WalkerList[iw]));
      }
      n-=nc*nw0;
      while(n>0) 
      {
        WalkerList.push_back(new Walker_t(*WalkerList[--nw0]));
        --n;
      }
    }
  }
}


void MCWalkerConfiguration::resize(int numWalkers, int numPtcls) {

  WARNMSG("MCWalkerConfiguration::resize cleans up the walker list.")

  ParticleSet::resize(unsigned(numPtcls));

  int dn=numWalkers-WalkerList.size();
  if(dn>0) createWalkers(dn);

  if(dn<0) {
    int nw=-dn;
    if(nw<WalkerList.size())  {
      iterator it = WalkerList.begin();
      while(nw) {
        delete *it; ++it; --nw;
      }
      WalkerList.erase(WalkerList.begin(),WalkerList.begin()-dn);
    }
  }
  //iterator it = WalkerList.begin();
  //while(it != WalkerList.end()) {
  //  delete *it++;
  //}
  //WalkerList.erase(WalkerList.begin(),WalkerList.end());
  //R.resize(np);
  //GlobalNum = np;
  //createWalkers(nw);  
}

///returns the next valid iterator
MCWalkerConfiguration::iterator 
MCWalkerConfiguration::destroyWalkers(iterator first, iterator last) {
  if(OwnWalkers) {
    iterator it = first;
    while(it != last) 
    {
      delete *it++;
    }
  }
  return WalkerList.erase(first,last);
}

void MCWalkerConfiguration::createWalkers(iterator first, iterator last)
{
  destroyWalkers(WalkerList.begin(),WalkerList.end());
  OwnWalkers=true;
  while(first != last) {
    WalkerList.push_back(new Walker_t(**first));
    ++first;
  }
}

void
MCWalkerConfiguration::destroyWalkers(int nw) {
  if(WalkerList.size() == 1 || nw >= WalkerList.size()) {
    app_warning() << "  Cannot remove walkers. Current Walkers = " << WalkerList.size() << endl;
    return;
  }
  nw=WalkerList.size()-nw;
  iterator it(WalkerList.begin()+nw),it_end(WalkerList.end());
  while(it != it_end) {
    delete *it++;
  }
  WalkerList.erase(WalkerList.begin()+nw,WalkerList.end());
}

void MCWalkerConfiguration::copyWalkers(iterator first, iterator last, iterator it)
{
  while(first != last) {
    (*it++)->makeCopy(**first++);
  }
}


void 
MCWalkerConfiguration::copyWalkerRefs(Walker_t* head, Walker_t* tail) {

  if(OwnWalkers) { //destroy the current walkers
    destroyWalkers(WalkerList.begin(), WalkerList.end());
    WalkerList.clear();
    OwnWalkers=false;//set to false to prevent deleting the Walkers
  }

  if(WalkerList.size()<2) {
    WalkerList.push_back(0);
    WalkerList.push_back(0);
  }

  WalkerList[0]=head;
  WalkerList[1]=tail;
}

/** Make Metropolis move to the walkers and save in a temporary array.
 * @param it the iterator of the first walker to work on
 * @param tauinv  inverse of the time step
 *
 * R + D + X
 */
void MCWalkerConfiguration::sample(iterator it, RealType tauinv) {
  makeGaussRandom(R);
  R *= tauinv;
  R += (*it)->R + (*it)->Drift;
}

void MCWalkerConfiguration::reset() {
  iterator it(WalkerList.begin()), it_end(WalkerList.end());
  while(it != it_end) {//(*it)->reset();++it;}
    (*it)->Weight=1.0;
    (*it)->Multiplicity=1.0;
    ++it;
  }
}

//void MCWalkerConfiguration::clearAuxDataSet() {
//  UpdateMode=Update_Particle;
//  int nbytes=128*GlobalNum*sizeof(RealType);//could be pagesize
//  if(WalkerList.size())//check if capacity is bigger than the estimated one
//    nbytes = (WalkerList[0]->DataSet.capacity()>nbytes)?WalkerList[0]->DataSet.capacity():nbytes;
//  iterator it(WalkerList.begin());
//  iterator it_end(WalkerList.end());
//  while(it!=it_end) {
//    (*it)->DataSet.clear(); 
//    //CHECK THIS WITH INTEL 10.1
//    //(*it)->DataSet.reserve(nbytes);
//    ++it;
//  }
//  ReadyForPbyP = true;
//}
//
//bool MCWalkerConfiguration::createAuxDataSet(int nfield) {
//
//  if(ReadyForPbyP) return false;
//
//  ReadyForPbyP=true;
//  UpdateMode=Update_Particle;
//  iterator it(WalkerList.begin());
//  iterator it_end(WalkerList.end());
//  while(it!=it_end) {
//    (*it)->DataSet.reserve(nfield); ++it;
//  }
//
//  return true;
//}

void MCWalkerConfiguration::loadWalker(Walker_t& awalker) {
  R = awalker.R;
  for(int i=0; i< DistTables.size(); i++) {
    DistTables[i]->evaluate(*this);
  }
}

/** reset the Property container of all the walkers
 */
void MCWalkerConfiguration::resetWalkerProperty(int ncopy) {
  int m(PropertyList.size());
  app_log() << "  Resetting Properties of the walkers " << ncopy << " x " << m << endl;
  Properties.resize(ncopy,m);
  iterator it(WalkerList.begin()),it_end(WalkerList.end());
  while(it != it_end) {
    (*it)->resizeProperty(ncopy,m); ++it;
  }
}

void MCWalkerConfiguration::saveEnsemble()
{
  iterator it(WalkerList.begin()),it_end(WalkerList.end());
  while(it != it_end) {
    SampleStack.push_back(new ParticlePos_t((*it)->R));
    ++it;
  }
}

void MCWalkerConfiguration::saveEnsemble(iterator first, iterator last)
{
  while(first != last) 
  {
    SampleStack.push_back(new ParticlePos_t((*first)->R));
    ++first;
  }
}
void MCWalkerConfiguration::loadEnsemble()
{
  if(SampleStack.empty()) return;

  Walker_t::PropertyContainer_t prop(1,PropertyList.size());
  int nsamples=SampleStack.size();

  delete_iter(WalkerList.begin(),WalkerList.end());
  WalkerList.resize(nsamples);
  for(int i=0; i<nsamples; ++i)
  {
    Walker_t* awalker=new Walker_t(GlobalNum);
    awalker->R = *(SampleStack[i]);
    awalker->Drift = 0.0;
    awalker->Properties.copy(prop);
    WalkerList[i]=awalker;
    delete SampleStack[i];
  }
  SampleStack.clear();
}

void MCWalkerConfiguration::loadEnsemble(MCWalkerConfiguration& other)
{
  if(SampleStack.empty()) return;
  Walker_t::PropertyContainer_t prop(1,PropertyList.size());
  int nsamples=SampleStack.size();
  for(int i=0; i<nsamples; ++i)
  {
    Walker_t* awalker=new Walker_t(GlobalNum);
    awalker->R = *(SampleStack[i]);
    awalker->Drift = 0.0;
    awalker->Properties.copy(prop);
    other.WalkerList.push_back(awalker);
    delete SampleStack[i];
  }
  SampleStack.clear();
}

#ifdef QMC_CUDA
void MCWalkerConfiguration::updateLists_GPU()
{
  int nw = WalkerList.size();

  if (Rnew_GPU.size() != nw) {
    Rnew_GPU.resize(nw);
    Rnew_host.resize(nw);
    Rnew.resize(nw);
    AcceptList_GPU.resize(nw);
    AcceptList_host.resize(nw);
  }

  host_vector<CUDA_PRECISION*> hostlist(nw);
  for (int iw=0; iw<nw; iw++)
    hostlist[iw] = (CUDA_PRECISION*)WalkerList[iw]->R_GPU.data();
  RList_GPU = hostlist;

  for (int iw=0; iw<nw; iw++)
    hostlist[iw] = (CUDA_PRECISION*)WalkerList[iw]->Grad_GPU.data();
  GradList_GPU = hostlist;

  for (int iw=0; iw<nw; iw++)
    hostlist[iw] = (CUDA_PRECISION*)WalkerList[iw]->Lap_GPU.data();
  LapList_GPU = hostlist;
  
}

void MCWalkerConfiguration::copyWalkersToGPU()
{
  host_vector<TinyVector<CUDA_PRECISION,OHMMS_DIM> > 
    R_host(WalkerList[0]->R.size());

  for (int iw=0; iw<WalkerList.size(); iw++) {
    for (int i=0; i<WalkerList[iw]->size(); i++)
      for (int dim=0; dim<OHMMS_DIM; dim++) 
	R_host[i][dim] = WalkerList[iw]->R[i][dim];
    WalkerList[iw]->R_GPU = R_host;
  }
}


void MCWalkerConfiguration::proposeMove_GPU
(vector<PosType> &newPos, int iat)
{
  for (int i=0; i<newPos.size(); i++)
    for (int dim=0; dim<OHMMS_DIM; dim++)
      Rnew_host[i][dim] = newPos[i][dim];
  Rnew_GPU = Rnew_host;
  Rnew = newPos;
  
  CurrentParticle = iat;
}


void MCWalkerConfiguration::acceptMove_GPU(vector<bool> &toAccept)
{
  for (int i=0; i<toAccept.size(); i++)
    AcceptList_host[i] = (int)toAccept[i];
  AcceptList_GPU = AcceptList_host;
  accept_move_GPU_cuda 
    (RList_GPU.data(), (CUDA_PRECISION*)Rnew_GPU.data(), 
     AcceptList_GPU.data(), CurrentParticle, WalkerList.size());
}





#endif


void MCWalkerConfiguration::clearEnsemble()
{
  delete_iter(SampleStack.begin(),SampleStack.end());
  SampleStack.clear();
}
}

/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
