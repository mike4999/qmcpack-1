//////////////////////////////////////////////////////////////////
// (c) Copyright 2003-  by Jeongnim Kim
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
#ifndef QMCPLUSPLUS_BACKFLOW_FUNCTIONBASE_H
#define QMCPLUSPLUS_BACKFLOW_FUNCTIONBASE_H
#include "QMCWaveFunctions/OrbitalSetTraits.h"
#include "Configuration.h"
#include "Particle/DistanceTable.h"
#include "OhmmsPETE/OhmmsArray.h"

namespace qmcplusplus
{

  /**  Base class for backflow transformations. 
   *  FT is an optimizable functor class that implements the radial function
   *  Any class used for Jastrow functions should work
   */
  class BackflowFunctionBase: public OrbitalSetTraits<QMCTraits::ValueType> 
  {

    public:

    typedef Array<HessType,3>       HessArray_t;
    //typedef Array<GradType,3>       GradArray_t;
    //typedef Array<PosType,3>        PosArray_t;
 
      ///recasting enum of DistanceTableData to maintain consistency
      enum {SourceIndex  = DistanceTableData::SourceIndex,
            VisitorIndex = DistanceTableData::VisitorIndex,
            WalkerIndex  = DistanceTableData::WalkerIndex
           };

    DistanceTableData* myTable; 

    /** enum for a update mode */
    enum
    {
      ORB_PBYP_RATIO,   /*!< particle-by-particle ratio only */
      ORB_PBYP_ALL,     /*!< particle-by-particle, update Value-Gradient-Laplacian */
      ORB_PBYP_PARTIAL, /*!< particle-by-particle, update Value and Grdient */
      ORB_WALKER,    /*!< walker update */
      ORB_ALLWALKER  /*!< all walkers update */
    };

    ///Reference to the center
    const ParticleSet& CenterSys;
    ///number of centers, e.g., ions
    int NumCenters;
    ///number of quantum particles
    int NumTargets;
    // number of variational parameters own by the radial function
    int numParams;
    // index of first parameter in derivative array
    int indexOfFirstParam;
    // temporary storage for derivatives
    vector<TinyVector<RealType,3> > derivs;

    GradMatrix_t UIJ;
    GradVector_t UIJ_temp; 

    HessMatrix_t AIJ;
    HessVector_t AIJ_temp;

    GradMatrix_t BIJ;
    GradVector_t BIJ_temp;

    ValueType *FirstOfU, *LastOfU;
    ValueType *FirstOfA, *LastOfA;
    ValueType *FirstOfB, *LastOfB;

    bool uniqueFunctions;

    BackflowFunctionBase(ParticleSet& ions, ParticleSet& els):
     CenterSys(ions), myTable(0),numParams(0),indexOfFirstParam(-1),
     uniqueFunctions(false) {
      NumCenters=CenterSys.getTotalNum(); // in case
      NumTargets=els.getTotalNum();
    }

    BackflowFunctionBase(BackflowFunctionBase &fn):
     CenterSys(fn.CenterSys), myTable(fn.myTable),NumTargets(fn.NumTargets),NumCenters(fn.NumCenters),numParams(fn.numParams),indexOfFirstParam(fn.indexOfFirstParam),uniqueFunctions(fn.uniqueFunctions)
    {
      derivs.resize(fn.derivs.size());
    }

    virtual
    BackflowFunctionBase* makeClone()=0;

    virtual ~BackflowFunctionBase() {}; 
 
    /** reset the distance table with a new target P
     */
    virtual void resetTargetParticleSet(ParticleSet& P)
    {
      myTable = DistanceTable::add(CenterSys,P);
    }

    virtual void
    acceptMove(int iat, int UpdateType)=0;

    virtual void
    restore(int iat, int UpdateType)=0;

    virtual void resetParameters(const opt_variables_type& active)=0;

    virtual void checkInVariables(opt_variables_type& active)=0;

    virtual void checkOutVariables(const opt_variables_type& active)=0;

    // Note: numParams should be set in Builder class, so it is known here
    inline int
    setParamIndex(int n) {
      indexOfFirstParam=n;
      return numParams;
    }

    virtual inline int indexOffset()=0;

    inline void 
    registerData(PooledData<RealType>& buf)
    {
      FirstOfU = &(UIJ(0,0)[0]); 
      LastOfU = FirstOfU + 3*NumTargets*NumTargets; 
      FirstOfA = &(AIJ(0,0)[0]); 
      LastOfA = FirstOfA + 9*NumTargets*NumTargets; 
      FirstOfB = &(BIJ(0,0)[0]); 
      LastOfB = FirstOfB + 3*NumTargets*NumTargets; 
      buf.add(FirstOfU,LastOfU);
      buf.add(FirstOfA,LastOfA);
      buf.add(FirstOfB,LastOfB);
    }
  
    inline void
    updateBuffer(PooledData<RealType>& buf)
    {
      buf.put(FirstOfU,LastOfU);
      buf.put(FirstOfA,LastOfA);
      buf.put(FirstOfB,LastOfB);
    }

    inline void
    copyFromBuffer(PooledData<RealType>& buf)
    {
      buf.get(FirstOfU,LastOfU);
      buf.get(FirstOfA,LastOfA);
      buf.get(FirstOfB,LastOfB);
    }
    
    /** calculate quasi-particle coordinates only
     */
    virtual inline void evaluate(const ParticleSet& P, ParticleSet& QP)=0;

    /** calculate quasi-particle coordinates, Bmat and Amat 
     */
    virtual inline void evaluate(const ParticleSet& P, ParticleSet& QP, GradMatrix_t& Bmat, HessMatrix_t& Amat)=0;

     /** calculate quasi-particle coordinates after pbyp move  
      */
    virtual void
    evaluatePbyP(const ParticleSet& P, ParticleSet::ParticlePos_t& newQP, const vector<int>& index)=0;

     /** calculate quasi-particle coordinates and Amat after pbyp move  
      */
    virtual void
    evaluatePbyP(const ParticleSet& P, ParticleSet::ParticlePos_t& newQP, const vector<int>& index, HessMatrix_t& Amat)=0;

     /** calculate quasi-particle coordinates, Bmat and Amat after pbyp move  
      */
    virtual void
    evaluatePbyP(const ParticleSet& P, ParticleSet::ParticlePos_t& newQP, const vector<int>& index, GradMatrix_t& Bmat, HessMatrix_t& Amat)=0;

    /** calculate only Bmat
     *  This is used in pbyp moves, in updateBuffer()  
     */ 
    virtual void
    evaluateBmatOnly(const ParticleSet& P,GradMatrix_t& Bmat_full)=0;

    /** calculate quasi-particle coordinates, Bmat and Amat 
     *  calculate derivatives wrt to variational parameters
     */
    virtual inline void evaluateWithDerivatives(const ParticleSet& P, ParticleSet& QP, GradMatrix_t& Bmat, HessMatrix_t& Amat, GradMatrix_t& Cmat, GradMatrix_t& Ymat, HessArray_t& Xmat)=0;

  };

}
  
#endif
