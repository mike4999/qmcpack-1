/////////////////////////////////////////////////////////////////
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
/**@file TrialWaveFunction.h
 *@brief Declaration of a TrialWaveFunction 
 */
#ifndef QMCPLUSPLUS_TRIALWAVEFUNCTION_H
#define QMCPLUSPLUS_TRIALWAVEFUNCTION_H

#include "Message/MPIObjectBase.h"
#include "QMCWaveFunctions/OrbitalBase.h"
#include "QMCWaveFunctions/DiffOrbitalBase.h"
#include "Utilities/NewTimer.h"
/**@defgroup MBWfs Many-body wave function group
 * @brief Classes to handle many-body trial wave functions
 */

namespace qmcplusplus {

  /** @ingroup MBWfs
   * @brief Class to represent a many-body trial wave function 
   *
   *A many-body trial wave function is represented by
   *\f[
   *\Psi({\bf R}) = \prod_i \psi_i({\bf R}),
   *\f]
   *where each function \f$\psi_i({\bf R})\f$ is an OrbitalBase
   (see OrbitalComponent).
   *A Composite Pattern is used to handle \f$\prod\f$ operations.
   *Each OrbitalBase should provide proper evaluate functions
   *for the value, gradient and laplacian values.
   */
  class TrialWaveFunction: public MPIObjectBase, public OhmmsElementBase 
  {

  public:
    typedef OrbitalBase::CudaValueType   CudaValueType;
    typedef OrbitalBase::CudaGradType    CudaGradType;
    typedef OrbitalBase::RealType        RealType;
    typedef OrbitalBase::CudaRealType    CudaRealType;
    typedef OrbitalBase::ValueType       ValueType;
    typedef OrbitalBase::PosType         PosType;
    typedef OrbitalBase::GradType        GradType;
    typedef OrbitalBase::BufferType      BufferType;
    typedef OrbitalBase::ValueMatrix_t   ValueMatrix_t;
    typedef OrbitalBase::GradMatrix_t    GradMatrix_t;
    typedef ParticleSet::Walker_t        Walker_t;


    ///differential gradients
    ParticleSet::ParticleGradient_t G;
    ///differential laplacians
    ParticleSet::ParticleLaplacian_t L;

    TrialWaveFunction(Communicate* c);

    ~TrialWaveFunction();

    inline int size() const { return Z.size();}
    inline RealType getPhase() const { return PhaseValue;}
    inline RealType getLogPsi() const { return LogValue;}

    ///Add an OrbitalBase 
    //void addOrbital(OrbitalBase* aterm);
    void addOrbital(OrbitalBase* aterm, const string& aname);

    ///write to ostream
    bool get(std::ostream& ) const;

    ///read from istream
    bool put(std::istream& );

    ///read from xmlNode
    bool put(xmlNodePtr cur);
    ///implement the virtual function
    void reset();
    /** set OrbitalBase::IsOptimizing to true */
    void startOptimization();
    /** set OrbitalBase::IsOptimizing to flase */
    void stopOptimization();
    /** check in an optimizable parameter
     * * @param o a super set of optimizable variables
     *
     * Update myOptIndex if o is found among the "active" paramemters.
     */
    void checkInVariables(opt_variables_type& o);
    /** check out optimizable variables
     */      
    void checkOutVariables(const opt_variables_type& o);
    ///reset member data
    void resetParameters(const opt_variables_type& active);
    /** print out state of the trial wavefunction
     */
    void reportStatus(ostream& os);

    /** recursively change the ParticleSet whose G and L are evaluated */
    void resetTargetParticleSet(ParticleSet& P);

    /////Check if aname-ed Single-Particle-Orbital set exists
    //bool hasSPOSet(const string& aname);
    /////add a Single-Particle-Orbital set
    //void addSPOSet(OhmmsElementBase* spo);
    /////return the aname-ed Single-Particle-Orbital set. 
    //OhmmsElementBase* getSPOSet(const string& aname);

    /** evalaute the values of the wavefunction, gradient and laplacian  for a walkers */
    ValueType evaluate(ParticleSet& P);

    /** evalaute the log of the trial wave function */
    RealType evaluateLog(ParticleSet& P);

    RealType evaluateDeltaLog(ParticleSet& P);

    void evaluateDeltaLog(ParticleSet& P, 
        RealType& logpsi_fixed,
        RealType& logpsi_opt,
        ParticleSet::ParticleGradient_t& fixedG,
        ParticleSet::ParticleLaplacian_t& fixedL);

    /** functions to handle particle-by-particle update */
    RealType ratio(ParticleSet& P, int iat);
    void update(ParticleSet& P, int iat);

    RealType ratio(ParticleSet& P, int iat, 
		    ParticleSet::ParticleGradient_t& dG,
		    ParticleSet::ParticleLaplacian_t& dL);		

    /////////////////////////
    // Vectorized versions //
    /////////////////////////
  private:
    gpu::device_host_vector<CudaValueType>   GPUratios;
    gpu::device_host_vector<CudaGradType>    GPUgrads;
    gpu::device_host_vector<CudaValueType>   GPUlapls;
  public:


    void freeGPUmem();

    void recompute (MCWalkerConfiguration &W, bool firstTime=true);

    void reserve (PointerPool<gpu::device_vector<CudaRealType> > &pool,
		  bool onlyOptimizable=false);

    void getGradient (MCWalkerConfiguration &W, int iat,
		      vector<GradType> &grad);
    void evaluateLog (MCWalkerConfiguration &W,
		      vector<RealType> &logPsi);
    void ratio (MCWalkerConfiguration &W, int iat,
		vector<ValueType> &psi_ratios);
    void ratio (MCWalkerConfiguration &W, int iat,
		vector<ValueType> &psi_ratios,
		vector<GradType> &newG);
    void ratio (MCWalkerConfiguration &W, int iat,
		vector<ValueType> &psi_ratios,
		vector<GradType> &newG,
		vector<ValueType> &newL);
    void ratio (vector<Walker_t*> &walkers, vector<int> &iatList,
		vector<PosType> &rNew,
		vector<ValueType> &psi_ratios,
		vector<GradType> &newG,
		vector<ValueType> &newL);
    void NLratios (MCWalkerConfiguration &W,
		   gpu::device_vector<CUDA_PRECISION*> &Rlist,
		   gpu::device_vector<int*>            &ElecList,
		   gpu::device_vector<int>             &NumCoreElecs,
		   gpu::device_vector<CUDA_PRECISION*> &QuadPosList,
		   gpu::device_vector<CUDA_PRECISION*> &RatioList,
		   int numQuadPoints);
    void NLratios (MCWalkerConfiguration &W,  vector<NLjob> &jobList,
		   vector<PosType> &quadPoints, vector<ValueType> &psi_ratios);

    void update (vector<Walker_t*> &walkers, int iat);
    void update (const vector<Walker_t*> &walkers, 
		 const vector<int> &iatList);

    void gradLapl (MCWalkerConfiguration &W, GradMatrix_t &grads,
	           ValueMatrix_t &lapl);


    void evaluateDeltaLog(MCWalkerConfiguration &W, 
			  vector<RealType>& logpsi_opt);

    void evaluateDeltaLog(MCWalkerConfiguration &W, 
			  vector<RealType>& logpsi_fixed,
			  vector<RealType>& logpsi_opt,
			  GradMatrix_t&  fixedG,
			  ValueMatrix_t& fixedL);

    void evaluateOptimizableLog (MCWalkerConfiguration &W,  
				 vector<RealType>& logpsi_opt,  
				 GradMatrix_t&  optG,
				 ValueMatrix_t& optL);


    void evaluateDerivatives (MCWalkerConfiguration &W, 
			      const opt_variables_type& optvars,
			      ValueMatrix_t &dlogpsi,
			      ValueMatrix_t &dhpsioverpsi);

    

//    RealType logRatio(ParticleSet& P, int iat, 
//		    ParticleSet::ParticleGradient_t& dG,
//		    ParticleSet::ParticleLaplacian_t& dL);


    void rejectMove(int iat);
    void acceptMove(ParticleSet& P, int iat);

    RealType registerData(ParticleSet& P, BufferType& buf);
    RealType updateBuffer(ParticleSet& P, BufferType& buf, bool fromscratch=false);
    void copyFromBuffer(ParticleSet& P, BufferType& buf);
    RealType evaluateLog(ParticleSet& P, BufferType& buf);

    void dumpToBuffer(ParticleSet& P, BufferType& buf);
    void dumpFromBuffer(ParticleSet& P, BufferType& buf);

    RealType KECorrection() const;
    
    void evaluateDerivatives(ParticleSet& P, RealType ke0, 
        const opt_variables_type& optvars,
        vector<RealType>& dlogpsi,
        vector<RealType>& dhpsioverpsi);

    /** evalaute the values of the wavefunction, gradient and laplacian  for all the walkers */
    //void evaluate(WalkerSetRef& W, OrbitalBase::ValueVectorType& psi);

    void reverse();

    TrialWaveFunction* makeClone(ParticleSet& tqp) const;

    void setMassTerm(ParticleSet& P)
    {
        SpeciesSet tspecies(P.getSpeciesSet());
        int massind=tspecies.addAttribute("mass");
        RealType mass = tspecies(massind,0);
        OneOverM = 1.0/mass;
    }

  private:

    ///control how ratio is calculated
    bool Ordered;
    ///the size of ParticleSet
    int NumPtcls;

    ///the size of gradient component (QMCTraits::DIM)*the number of particles 
    int TotalDim;

    ///index of the active particle
    int WorkingPtcl;

    ///sign of the trial wave function
    RealType PhaseValue;

    ///log of the trial wave function
    RealType LogValue;

    ///One over mass of target particleset, needed for Local Energy Derivatives
    RealType OneOverM;

    ///a list of OrbitalBases constituting many-body wave functions
    vector<OrbitalBase*> Z;

    ///a list of single-particle-orbital set
    //vector<OhmmsElementBase*> SPOSet;

    ///differential gradients
    ParticleSet::ParticleGradient_t delta_G;

    ///differential laplacians
    ParticleSet::ParticleLaplacian_t delta_L;

    TrialWaveFunction();

    vector<NewTimer*> myTimers;
    
  };
  /**@}*/
}
#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/

