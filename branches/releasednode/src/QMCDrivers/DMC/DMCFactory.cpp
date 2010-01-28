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
//
// Supported by 
//   National Center for Supercomputing Applications, UIUC
//   Materials Computation Center, UIUC
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
#include "QMCDrivers/DMC/DMCFactory.h" 
#include "QMCDrivers/DMC/DMCOMP.h" 
#include "Message/OpenMP.h"
//#define PETA_DMC_TEST
namespace qmcplusplus {

  QMCDriver* DMCFactory::create(MCWalkerConfiguration& w, TrialWaveFunction& psi
      , QMCHamiltonian& h, HamiltonianPool& hpool) 
  {
    app_log() << "Creating DMCMP for the qmc driver" << endl;
    QMCDriver*  qmc = new DMCOMP(w,psi,h,hpool);
    qmc->setUpdateMode(PbyPUpdate);
    qmc->put(myNode);
    return qmc;
  }
}
/***************************************************************************
 * $RCSfile: DMCFactory.cpp,v $   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
