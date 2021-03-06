//////////////////////////////////////////////////////////////////
// (c) Copyright 2005- by Jeongnim Kim
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
#ifndef QMCPLUSPLUS_COSTFUNCTIONOMP_H
#define QMCPLUSPLUS_COSTFUNCTIONOMP_H

#include "QMCDrivers/QMCCostFunctionBase.h"
#include "QMCDrivers/CloneManager.h"
#include "QMCWaveFunctions/OrbitalSetTraits.h"

namespace qmcplusplus
  {

  /** @ingroup QMCDrivers
   * @brief Implements wave-function optimization
   *
   * Optimization by correlated sampling method with configurations
   * generated from VMC running on a single thread.
   */
  class QMCCostFunctionOMP: public QMCCostFunctionBase, public CloneManager
    {
    public:

      ///Constructor.
      QMCCostFunctionOMP(MCWalkerConfiguration& w, TrialWaveFunction& psi,
                         QMCHamiltonian& h, HamiltonianPool& hpool);

      ///Destructor
      ~QMCCostFunctionOMP();

      void getConfigurations(const string& aroot);
      void checkConfigurations();
      void resetPsi(bool final_reset=false);
      void GradCost(vector<Return_t>& PGradient, vector<Return_t> PM, Return_t FiniteDiff=0);
      Return_t fillOverlapHamiltonianMatrix(Matrix<Return_t>& Hamiltonian, Matrix<Return_t>& Overlap);
    protected:
      vector<QMCHamiltonian*> H_KE_Node;
      vector<Matrix<Return_t>*> RecordsOnNode;

      /** Temp derivative properties and Hderivative properties of all the walkers
      */
      vector<vector<vector<Return_t> >* > TempDerivRecords;
      vector<vector<vector<Return_t> >* > TempHDerivRecords;
      Return_t CSWeight;

      Return_t correlatedSampling(bool needDerivs);
    };
}
#endif
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 1804 $   $Date: 2007-02-24 14:49:09 -0600 (Sat, 24 Feb 2007) $
 * $Id: QMCCostFunctionOMP.h 1804 2007-02-24 20:49:09Z jnkim $
 ***************************************************************************/
