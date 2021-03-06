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
#ifndef OHMMS_QMC_MULTIPLELOCALENERGYESTIMATOR_H
#define OHMMS_QMC_MULTIPLELOCALENERGYESTIMATOR_H

#include "Estimators/ScalarEstimatorBase.h"

namespace ohmmsqmc {

  class QMCHamiltonian;
  class TrialWaveFunction;

  struct MultipleEnergyEstimator: public ScalarEstimatorBase<ParticleSet::RealType> {

    enum {ENERGY_INDEX, ENERGY_SQ_INDEX, WEIGHT_INDEX, LE_INDEX};

    typedef ParticleSet::RealType RealType;
    typedef ScalarEstimatorBase<RealType>::Walker_t Walker_t;
    typedef Matrix<RealType> EnergyContainer_t;

    using ScalarEstimatorBase<RealType>::b_average;
    using ScalarEstimatorBase<RealType>::b_variance;

    ///index to keep track how many times accumulate is called
    int CurrentWalker;

    int NumWalkers;
    ///starting column index of this estimator 
    int FirstIndex;

    /** the number of duplicated wavefunctions/hamiltonians
     *
     * NumCopies is the row size of EnergyContainer_tUmbrellaSamples
     */
    int NumCopies;

    /** the number of components of a Hamiltonian
     *
     * It also works as the column index for the local energy. For instacne,
     * UmbrellaSamples[walker_index]->operator()(copy_index,NumOperators)
     * is the local energy of the copy_index wavefunction for the walker_index walker
     */ 
    int NumOperators;

    ///the index of weight
    int WeightIndex;

    /** the column size of EnergyContaner_t for UmbrellaSamples
     * 
     * NumCols = NumOperators+2
     * 2 is for the local energy and weight
     */
    int NumCols;

    /** local energy data for each walker. 
     */
    Matrix<RealType> UmbrellaEnergy, UmbrellaWeight, RatioIJ;

    ///accumulated total local energies, weights
    Matrix<RealType> esum;

    /** accumulated local energies
     *
     * The size of eloc is NumCopies by NumOperators.
     */
    Matrix<RealType> elocal;

    ///the names of esum
    Matrix<string> esum_name;
    
    ///the names of elocal
    Matrix<string> elocal_name;

    /** constructor
     * @param h QMCHamiltonian to define the components
     * @param hcopy number of copies of QMCHamiltonians
     */
    MultipleEnergyEstimator(QMCHamiltonian& h, int hcopy=1); 

    /**  add the local energy, variance and all the Hamiltonian components to the scalar record container
     *@param record storage of scalar records (name,value)
     */
    void add2Record(RecordNamedProperty<RealType>& record);

    void accumulate(const Walker_t& awalker, RealType wgt);

    ///reset all the cumulative sums to zero
    void reset();

    /** calculate the averages and reset to zero
     *\param record a container class for storing scalar records (name,value)
     *\param wgtinv the inverse weight
     */
    void report(RecordNamedProperty<RealType>& record, RealType wgtinv);

    /** initialize the multi-configuration data
     *
     * @param W MCWalkerConfiguration
     * @param h Collection of QMCHamiltonians*
     * @param psi Collection of TrialWaveFunctions*
     * @param tau time step for the initial weighted drift
     * @param require_register if true, use buffer for particle-by-particle
     */
    void initialize(MCWalkerConfiguration& W, 
        vector<QMCHamiltonian*>& h, vector<TrialWaveFunction*>& psi,
        RealType tau, bool require_register=false);

    /** update the energy and weight for umbrella sampling
     * @param iw walker index
     * @param ipsi H/Psi index
     * @param e local energy of the iw-th walker for H[ipsi]/Psi[ipsi]
     * @param r umbrella weight
     */
    inline void updateSample(int iw, int ipsi, RealType e, RealType invr) {
      UmbrellaEnergy(iw,ipsi)=e;
      UmbrellaWeight(iw,ipsi)=invr;
    }

  };

}
#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
