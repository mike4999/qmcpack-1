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
#ifndef QMCPLUSPLUS_CORRELATEDLOCALENERGYESTIMATOR_H
#define QMCPLUSPLUS_CORRELATEDLOCALENERGYESTIMATOR_H

#include "Estimators/ScalarEstimatorBase.h"
#include "QMCDrivers/SpaceWarp.h"

namespace qmcplusplus {

  class QMCHamiltonian;
  class TrialWaveFunction;

  struct CSEnergyEstimator: public ScalarEstimatorBase {

    //enum {ENERGY_INDEX, ENERGY_SQ_INDEX, WEIGHT_INDEX, PE_INDEX, KE_INDEX, LE_INDEX};
    enum {ENERGY_INDEX, ENERGY_SQ_INDEX, WEIGHT_INDEX, LE_INDEX};

    ///number of correlated systems
    int NumCopies;
    ///number of observables
    int NumObservables;
    /** constructor
     * @param h QMCHamiltonian to define the components
     * @param hcopy number of copies of QMCHamiltonians
     */
    CSEnergyEstimator(QMCHamiltonian& h, int hcopy=1); 
    CSEnergyEstimator(const CSEnergyEstimator& mest);
    ScalarEstimatorBase* clone();

    inline RealType getUmbrellaWeight(int ipsi)
    {
      return scalars_saved[ipsi*LE_INDEX+WEIGHT_INDEX].result();
      //return d_data[ipsi*LE_INDEX+WEIGHT_INDEX];
    }

    /**  add the local energy, variance and all the Hamiltonian components to the scalar record container
     *@param record storage of scalar records (name,value)
     */
    void add2Record(RecordNamedProperty<RealType>& record);

    void accumulate(const Walker_t& awalker, RealType wgt);

    inline void accumulate(WalkerIterator first, WalkerIterator last, RealType wgt) 
    {
      //accumulate  the number of times accumulation has occurred.
      //d_wgt+=last-first;
      while(first != last) {
        accumulate(**first++,wgt);
      }
    }

    void evaluateDiff();
  };

}
#endif
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 1926 $   $Date: 2007-04-20 12:30:26 -0500 (Fri, 20 Apr 2007) $
 * $Id: CSEnergyEstimator.h 1926 2007-04-20 17:30:26Z jnkim $ 
 ***************************************************************************/
