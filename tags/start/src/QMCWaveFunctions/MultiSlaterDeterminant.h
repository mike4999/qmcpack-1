//////////////////////////////////////////////////////////////////
// (c) Copyright 2003-  by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
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
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
#ifndef OHMMS_QMC_MULTISLATERDETERMINANT_ORBITAL_H
#define OHMMS_QMC_MULTISLATERDETERMINANT_ORBITAL_H
#include "Configuration.h"
#include "QMCWaveFunctions/OrbitalBase.h"
#include "QMCWaveFunctions/SlaterDeterminant.h"

namespace ohmmsqmc {

 /**
   *@brief An AntiSymmetric OrbitalBase composed of a linear
   *combination of SlaterDeterminants. 
   *
   *\f[ 
   *MS({\bf R}) = \sum_n c_n S_n({\bf R}) 
   *\f].
   *
   *The (S)ingle(P)article(O)rbitalSet template parameter is an 
   *engine which fills in single-particle orbital terms.  
   * 
   \f[
   \frac{\nabla_i\Psi}{\Psi} = \frac{\sum_{n=1}^M c_n \nabla_i D_n}
   {\sum_{n=1}^M c_n D_n}
   \f]
   \f[ 
   \frac{\sum_{n=1}^M c_n S_n(\sum_{j=1}^N(\nabla_i
   S^{ij}_n({\bf r_i}))(S^{-1})^{ji}_n}{\sum_{n=1}^M c_n S_n}
   \f]
   The Laplacian
   \f[
   \frac{\nabla^2_i\Psi}{\Psi} = \frac{\sum_{n=1}^M c_n S_n(\sum_{j=1}^N
   (\nabla_i^2S^{ij}_n({\bf r_i}))(S^{-1})^{ji}_n}{\sum_{n=1}^M c_n S_n}
   \f]
   */
template<class SPOSet>
class MultiSlaterDeterminant: public OrbitalBase {

public:

  typedef SlaterDeterminant<SPOSet> DeterminantSet_t;

  ///constructor
  MultiSlaterDeterminant() { }
  ///destructor
  ~MultiSlaterDeterminant() { }

  /**
     add a new SlaterDeterminant with coefficient c to the 
     list of determinants
  */
  void add(DeterminantSet_t* sdet, RealType c) {
    SDets.push_back(sdet);
    C.push_back(c);
  }

  void reset() {  }

  void initParameters() { }

  /**
   *@param P input configuration containing N particles
   *@param G a vector containing N gradients
   *@param L a vector containing N laplacians
   *@return SlaterDeterminant value
   *@brief While calculating the Multi-Slater determinant value for 
   *the set of particles, add the gradient and laplacian 
   *contribution of the determinant to G(radient) and L(aplacian)
   *for local energy calculations.
   */
  inline ValueType
  evaluate(ParticleSet& P, //const DistanceTableData* dtable,
	   ParticleSet::ParticleGradient_t& G,
	   ParticleSet::ParticleLaplacian_t& L){

    int n = P.getTotalNum();
    ParticleSet::ParticleGradient_t g(n), gt(n);
    ParticleSet::ParticleLaplacian_t l(n), lt(n);

    ValueType psi = 0.0;
    for(int i=0; i<SDets.size(); i++){
      //ValueType cdet = C[i]*Dets[i]->evaluate(P,dtable,g,l);
      ValueType cdet = C[i]*SDets[i]->evaluate(P,g,l);
      psi += cdet;
      gt += cdet*g;
      lt += cdet*l;
      g=0.0;
      l=0.0;
    }
    ValueType psiinv = 1.0/psi;
    G += gt*psiinv;
    L += lt*psiinv;
    return psi;
  }
  /// returns the dimension of the j-th determinant 
  inline int size(int i, int j) const {return Dets[i]->size(j);}


  inline void evaluate(WalkerSetRef& W, //const DistanceTableData* dtable,
		       ValueVectorType& psi,
		       WalkerSetRef::WalkerGradient_t& G,
		       WalkerSetRef::WalkerLaplacian_t& L) {
    for(int i=0; i<SDets.size(); i++) SDets[i]->evaluate(W,psi,G,L);
      //Dets[i]->evaluate(W,dtable,psi,G,L);
  }


private:
  vector<DeterminantSet_t*> SDets;
  vector<RealType> C;
};

}
#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$
 ***************************************************************************/
