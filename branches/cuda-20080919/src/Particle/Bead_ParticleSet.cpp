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
#include "Particle/Bead_ParticleSet.h"
#include "Particle/DistanceTableData.h"
#include "Particle/DistanceTable.h"
#include "ParticleBase/RandomSeqGenerator.h"
#include "QMCDrivers/MultiChain.h"
#include "Message/Communicate.h"
#include "Message/CommOperators.h"
#include "QMCWaveFunctions/TrialWaveFunction.h"
#include <map>
#include "Particle/Walker.h"
#include "ParticleBase/ParticleAttribOps.h"
namespace qmcplusplus {




Bead_ParticleSet::Bead_ParticleSet(): 
ReadyForPbyP(false),UpdateMode(Update_Walker)
  {
    
  }


  //Does not save distance tables!  This is
  //to save lightweight data!!
  ///Does not save R or buffers!
  void Bead_ParticleSet::SaveOldData()
  {
    //    for (int i=0;i<R.size();i++)
    //      R_saved[i]=R[i];
    for(int i=0; i<Gradients_saved.size(); i++) *Gradients_saved[i] = *(Gradients[i]);
    for(int i=0; i<Laplacians_saved.size(); i++) *Laplacians_saved[i] = *(Laplacians[i]);
    properties_saved=Properties;
    //    Action_saved=Action;

    for (int ipsi=0;ipsi<nPsi;ipsi++)
      for (int i=0;i<3;i++)
	Action_saved(ipsi,i)=Action(ipsi,i);


    BeadSignWgt_saved=BeadSignWgt;
    TransProb_saved[0]=TransProb[0];
    TransProb_saved[1]=TransProb[1];
    Drift_saved=Drift;
  }

  //Does not save distance tables!  This is
  //to save lightweight data!!
  ///Does not save R or buffers!
  void Bead_ParticleSet::RestoreOldData()
  {
    //    for (int i=0;i<R.size();i++)
    //      R[i]=R_saved[i];
    for(int i=0; i<Gradients_saved.size(); i++) *Gradients[i] = *(Gradients_saved[i]);
    for(int i=0; i<Laplacians_saved.size(); i++) *Laplacians[i] = *(Laplacians_saved[i]);
    //    Action=Action_saved;
    for (int ipsi=0;ipsi<nPsi;ipsi++)
      for (int i=0;i<3;i++)
	Action(ipsi,i)=Action_saved(ipsi,i);

    BeadSignWgt=BeadSignWgt_saved;
    TransProb[0]=TransProb_saved[0];
    TransProb[1]=TransProb_saved[1];
    Properties=properties_saved;
    Drift=Drift_saved;
  }


  
  //Sets the gradient and laplacian to the specific correlated sample
  void Bead_ParticleSet::SetGradientAndLaplacian(int ipsi)
  {
    //    G=Gradients[ipsi];
    Copy(*(Gradients[ipsi]),G);
    L=*(Laplacians[ipsi]);
    
  }


  void Bead_ParticleSet::getDrift(vector<RealType>& LogNorm) {
    int nPsi(Properties.rows());
      //compute Drift
    RealType denom(0.e0),wgtpsi;
    Drift=0.e0;
    for(int ipsi=0; ipsi<nPsi; ipsi++) {
      wgtpsi=BeadSignWgt[ipsi]*std::exp(2.0*( Properties(ipsi,LOGPSI)- LogNorm[ipsi]
					      -Properties(0,LOGPSI)   + LogNorm[0]));
      denom += wgtpsi;
      Drift += (wgtpsi*(*Gradients[ipsi]));
    }
    denom=1.0/denom;
    Drift *= denom;
  }


  
void
 Bead_ParticleSet::CopyFromBead(Bead& b,vector<TrialWaveFunction*> &Psi)
 {
   
 assert(R.size()==b.R.size());
  assert(BeadSignWgt.size()==b.BeadSignWgt.size());
  assert(Gradients.size()==b.Gradients.size());
  assert(Laplacians.size()==b.Laplacians.size());
  assert(b.Action.size()==Action.size());
  assert(b.Drift.size()==Drift.size());
  R=b.R;
  copy(b.BeadSignWgt.begin(),b.BeadSignWgt.end(),BeadSignWgt.begin());
  for(int i=0; i<b.Gradients.size(); i++) *Gradients[i] = *(b.Gradients[i]);
  for(int i=0; i<b.Laplacians.size(); i++) *Laplacians[i] = *(b.Laplacians[i]);
  TransProb[0]=b.TransProb[0];
  TransProb[1]=b.TransProb[1];
    for (int ipsi=0;ipsi<nPsi;ipsi++)
      for (int i=0;i<3;i++)
	Action(ipsi,i)=b.Action(ipsi,i);

    //  Action=b.Action;
  Drift=b.Drift;

  Properties=b.Properties;

 }

void
Bead_ParticleSet::CopyToBead(Bead& b,vector<TrialWaveFunction*> &Psi)
{
  assert(R.size()==b.R.size());
  assert(b.Drift.size()==Drift.size());
  assert(BeadSignWgt.size()==b.BeadSignWgt.size());
  assert(Gradients.size()==b.Gradients.size());
  assert(Laplacians.size()==b.Laplacians.size());
  assert(b.Action.size()==Action.size());
  
  copy(BeadSignWgt.begin(),BeadSignWgt.end(),b.BeadSignWgt.begin());
  for(int i=0; i<Gradients.size(); i++) *(b.Gradients[i]) = *Gradients[i];
  for(int i=0; i<Laplacians.size(); i++) *(b.Laplacians[i])=*Laplacians[i];
  b.TransProb[0]=TransProb[0];
  b.TransProb[1]=TransProb[1];
  
    for (int ipsi=0;ipsi<nPsi;ipsi++)
      for (int i=0;i<3;i++)
	b.Action(ipsi,i)=Action(ipsi,i);
    //    b.Action=Action;
    b.Drift=Drift;
    b.R=R;
    b.Properties=Properties;

}

  

Bead_ParticleSet::Bead_ParticleSet(const ParticleSet& mcw,int nPsi)
  : ParticleSet(mcw),
  UpdateMode(Update_Walker)
{
  int nptcl=mcw.R.size();
  Bead_ParticleSet::nPsi=nPsi;
  //  R.resize(nptcl);
  //  G.resize(nptcl);
  //  L.resize(nptcl);
  BeadSignWgt.resize(nPsi);
  Drift.resize(nptcl);
  for (int i=0;i<nPsi;i++){
    Gradients.push_back(new ParticlePos_t(nptcl));
    Laplacians.push_back(new ParticleLaplacian_t(nptcl));
  }
  Action.resize(nPsi,3);
  
  R_saved.resize(nptcl);
  BeadSignWgt_saved.resize(nPsi);
  Drift_saved.resize(nptcl);
  for (int i=0;i<nPsi;i++){
    Gradients_saved.push_back(new ParticlePos_t(nptcl));
    Laplacians_saved.push_back(new ParticleLaplacian_t(nptcl));
  }
  Action_saved.resize(nPsi,3);
  



}

///default destructor
Bead_ParticleSet::~Bead_ParticleSet(){
  DEBUGMSG("Bead_ParticleSet::~Bead_ParticleSet");
}




// /** Make Metropolis move to the walkers and save in a temporary array.
//  * @param it the iterator of the first walker to work on
//  * @param tauinv  inverse of the time step
//  *
//  * R + D + X
//  */
// void Bead_ParticleSet::sample(iterator it, RealType tauinv) {
//   makeGaussRandom(R);
//   R *= tauinv;
//   R += (*it)->R + (*it)->Drift;
// }

// void Bead_ParticleSet::loadWalker(Walker_t& awalker) {
//   R = awalker.R;
//   for(int i=0; i< DistTables.size(); i++) {
//     DistTables[i]->evaluate(*this);
//   }
// }

// /** reset the Property container of all the walkers
//  */
//   void Bead_ParticleSet::resetWalkerProperty(int ncopy) {
//     int m(PropertyList.size());
//     app_log() << "  Resetting Properties of the walkers " << ncopy << " x " << m << endl;
//     Properties.resize(ncopy,m);
//     iterator it(WalkerList.begin()),it_end(WalkerList.end());
//     while(it != it_end) {
//       (*it)->resizeProperty(ncopy,m); ++it;
//     }
//   }

// void Bead_ParticleSet::loadEnsemble()
// {
//   if(SampleStack.empty()) return;

//   Walker_t::PropertyContainer_t prop(1,PropertyList.size());
//   int nsamples=SampleStack.size();

//   delete_iter(WalkerList.begin(),WalkerList.end());
//   WalkerList.resize(nsamples);
//   for(int i=0; i<nsamples; ++i)
//   {
//     Walker_t* awalker=new Walker_t(GlobalNum);
//     awalker->R = *(SampleStack[i]);
//     awalker->Drift = 0.0;
//     awalker->Properties.copy(prop);
//     WalkerList[i]=awalker;
//     delete SampleStack[i];
//   }
//   SampleStack.clear();
// }

// void Bead_ParticleSet::loadEnsemble(Bead_ParticleSet& other)
// {
//   if(SampleStack.empty()) return;
//   Walker_t::PropertyContainer_t prop(1,PropertyList.size());
//   int nsamples=SampleStack.size();
//   for(int i=0; i<nsamples; ++i)
//   {
//     Walker_t* awalker=new Walker_t(GlobalNum);
//     awalker->R = *(SampleStack[i]);
//     awalker->Drift = 0.0;
//     awalker->Properties.copy(prop);
//     other.WalkerList.push_back(awalker);
//     delete SampleStack[i];
//   }
//   SampleStack.clear();
// }

}

/***************************************************************************
 * $RCSfile$   $Author: bkclark $
 * $Revision: 3062 $   $Date: 2008-08-28 12:00:40 -0500 (Thu, 28 Aug 2008) $
 * $Id: MCWalkerConfiguration.cpp 3062 2008-08-28 17:00:40Z bkclark $ 
 ***************************************************************************/
