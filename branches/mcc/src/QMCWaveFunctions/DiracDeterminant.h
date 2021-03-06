//////////////////////////////////////////////////////////////////
// (c) Copyright 1998-2002 by Jeongnim Kim
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
#ifndef OHMMSQMC_DIRACDETERMINANT_H
#define OHMMSQMC_DIRACDETERMINANT_H
#include "Numerics/DeterminantOperators.h"

namespace ohmmsqmc {

  class ParticleSet;
  class WalkerSetRef;

  /** Generic class to handle a DiracDeterminant.
   *
   *The (S)ingle(P)article(O)rbitalSet template parameter is an 
   *engine which fills in single-particle orbital terms.
   *
   *The DiracDeterminant<SPOSet> class handles the determinant and co-factors:
   *<ul>
   *<li> set up the matrix \f$D\f$ from \f$ \{\psi\} \f$ (the set of 
   *single-particle orbitals) such that \f$D_{ij} =  \psi_j({\bf r}_i).\f$
   *<li> invert the matrix \f$D.\f$
   *<li> evaluate the determinant \f$|D|\f$ and the gradient and 
   *laplacian of the logarithm of the determinant.
   *</ul>
   *The template class SPOSet has to evaluate
   *<ul>
   *<li> psiM(j,i) \f$= \psi_j({\bf r}_i)\f$
   *<li> dpsiM(i,j) \f$= \nabla_i \psi_j({\bf r}_i)\f$
   *<li> d2psiM(i,j) \f$= \nabla_i^2 \psi_j({\bf r}_i)\f$
   *</ul>
   *Very important to note that psiM is a transpose of a matrix defined as
   *\f$D_{ij} =  \psi_j({\bf r}_i)\f$. Upon inversion, other operations
   *to update and evaluate \f$ \nabla_i \ln{\rm D} \f$ and 
   *\f$ \nabla_i^2\ln{\rm D} \f$ can be done efficiently by dot products.
   *
   *Since each determinant operates on a subset of particles (e.g., up
   *or down electrons), auxiliary indices (first, last)
   *are passed to enable operations only on the particles belonging to 
   *this DiracDeterminant term.
   *
   *In evaluating the determinant it is necessary to define the
   *following quatities.  
   *<ul>
   *<li> The minor \f$ M_{ij} \f$ for the element \f$ D_{ij} \f$ 
   *is the determinant of an \f$ (N-1) \times (N-1) \f$ matrix obtained 
   *by removing all the elements of row \f$ i \f$ and column \f$ j \f$.  
   *<li>The cofactor matrix \f$ C \f$ is constructed on an element by 
   *element basis from the minor using the simple relation: 
   *\f[ C_{ij} = (-1)^{i+j}M_{ij} \f]
   *<li>The inverse of a matrix is related to the transpose of the 
   *cofactor matrix by the relation
   \f[ (D^{-1})_{ij} = \frac{(C_{ij})^T}{|D|} = \frac{C_{ji}}{|D|} \f]
   *</ul>
   *Using these definitions the determinant is evaluated as the sum
   *of the products of the elements of any row or
   *column of the matrix and the corresponding cofactor
   \f[
   |D| = \sum_{j=1}^N D_{ij}C_{ij}\;\;\longrightarrow \mbox{along row i}
   \f]
   *or
   \f[
   |D| = \sum_{i=1}^N D_{ij}C_{ij}\;\;\longrightarrow \mbox{along column j}.
   \f]
   *
   *To calculate the local energy \f$E_L\f$ it is necessary to evaluate
   *the gradient and laplacian of the logarithm of the Diracdeterminant
   \f[ 
   \frac{\nabla_i|D({\bf R})|}{|D({\bf R})|} \;\;\;\;\;\;\;\;\;\;\;\;\;
   \mbox{and}\;\;\;\;\;\;\;\;\;\;\;\;\; 
   \frac{\nabla_i^2|D({\bf R})|}{|D({\bf R})|}.
   \f]
   *We have already shown how to evaluate the determinant in terms of its
   *cofactors, taking the gradient follows from this relation  
   \f[
   \nabla_i|D| = |D|\sum_{j=1}^N (\nabla_i D_{ij})(D^{-1})_{ji},
   \f]
   *where \f$ D_{ij} \f$ is a function of \f${\bf r_i}\f$ and 
   *\f$ |D|(D^{-1})_{ji} \f$ is a function of all of the other coordinates 
   *except \f${\bf r_i}\f$.  This leads to the result
   \f[
   \frac{\nabla_i|D|}{|D|} = \sum_{j=1}^N (\nabla_i D_{ij})(D^{-1})_{ji}.
   \f]
   *The laplacian easily follows
   \f[
   \frac{\nabla^2_i|D|}{|D|} = \sum_{j=1}^N (\nabla^2_i D_{ij})(D^{-1})_{ji}.
   \f]
   *
   *@note SlaterDeterminant is a product of DiracDeterminants.

   */
  template<class SPOSet>
  class DiracDeterminant {

  public:

    typedef typename SPOSet::RealType  RealType;
    typedef typename SPOSet::ValueType ValueType;
    typedef typename SPOSet::PosType   PosType;
    typedef typename SPOSet::GradType  GradType;

#if defined(USE_BLITZ)
    typedef blitz::Array<ValueType,2> Determinant_t;
    typedef blitz::Array<GradType,2>  Gradient_t;
    typedef blitz::Array<ValueType,2> Laplacian_t;
#else
    typedef Matrix<ValueType> Determinant_t;
    typedef Matrix<GradType>  Gradient_t;
    typedef Matrix<ValueType> Laplacian_t;
#endif

    /**
     *@param spos the single-particle orbital set
     *@param first index of the first particle
     *@brief constructor
    */
    DiracDeterminant(SPOSet& spos, int first=0): 
      Phi(spos), FirstIndex(first) {}

    ///default destructor
    ~DiracDeterminant() {}
  
    /**copy constructor
     *@brief copy constructor, only resize and assign orbitals
     */
    DiracDeterminant(const DiracDeterminant<SPOSet>& s): Phi(s.Phi){
      resize(s.rows(), s.cols());
    }

    DiracDeterminant<SPOSet>& operator=(const DiracDeterminant<SPOSet>& s)  {
      resize(s.rows(), s.cols());
      return *this;
    }

    /**
     *@param first index of first particle
     *@param nel number of particles in the determinant
     *@brief set the index of the first particle in the
     determinant and reset the size of the determinant
    */
    inline void set(int first, int nel) {
      FirstIndex = first;
      resize(nel,nel);
    }

    ///reset the single-particle orbital set
    inline void reset() { Phi.reset(); }
   
    ///reset the size: with the number of particles and number of orbtials
    inline void resize(int nel, int morb) {
      int norb=morb;
      if(norb <= 0) norb = nel; // for morb == -1 (default)
      psiM.resize(nel,norb);
      dpsiM.resize(nel,norb);
      d2psiM.resize(nel,norb);
      //    work.resize(norb);
      //   pivot.resize(norb);
      LastIndex = FirstIndex + nel;
    }

    void resizeByWalkers(int nw);

    ///return the number of rows (or the number of electrons)
    inline int rows() const { return psiM.rows();}

    ///return the number of coloumns  (or the number of orbitals)
    inline int cols() const { return psiM.cols();}

    ///evaluate for a particle set
    template<class GradVec, class LapVec>
    ValueType evaluate(ParticleSet& P, GradVec& G, LapVec& L);

    ///evaluate for walkers
    template<class WfsVec, class GradMat, class LapMat> 
    void evaluate(WalkerSetRef& W, WfsVec& psi, GradMat& G, LapMat& L);

  private:

    /*a set of single-particle orbitals used to fill in the 
      values of the matrix */
    SPOSet& Phi;

    ///index of the first particle with respect to the particle set
    int FirstIndex;

    ///index of the last particle with respect to the particle set
    int LastIndex;

    ///index of the particle (or row) 
    int WorkingPtcl;      

    ///Current determinant value
    ValueType CurrentDet;

    /// psiM(i,j) \f$= |Det(\{R\})|_{i,j}^{-1}\f$
    Determinant_t psiM;
    Gradient_t    dpsiM;
    Laplacian_t   d2psiM;

    ///storages to process many walkers once
    vector<Determinant_t> psiM_v; 
    vector<Gradient_t>    dpsiM_v; 
    vector<Laplacian_t>   d2psiM_v; 

    //TempMinv(i,j) \f$= |Det(\{R^{'}\})|_{i,j}^{-1}\f$ with before update
    //ValueMatrix_t tDetInv;
  };

  /**
   *@param P input configuration containing N particles
   *@param G a vector containing N gradients
   *@param L a vector containing N laplacians
   *@return the value of the determinant
   *@brief Calculate the value of the Dirac determinant for particles
   *\f$ (first,first+nel). \f$  Add the gradient and laplacian 
   *contribution of the determinant to G(radient) and L(aplacian)
   *for local energy calculations.
   */
  template<class SPOSet>
  template<class GradVec, class LapVec>
  inline 
  typename DiracDeterminant<SPOSet>::ValueType 
  DiracDeterminant<SPOSet>::evaluate(ParticleSet& P, 
				     GradVec& G, 
				     LapVec&  L) {

    int nrows = rows();
    int ncols = cols();

    Phi.evaluate(P, FirstIndex, LastIndex, psiM,dpsiM, d2psiM);
    CurrentDet = Invert(psiM.data(),nrows,ncols);
    //evaluate the values of single-particle orbtials: M and dM
    //     if(nrows == 1) {
    //       CurrentDet = psiM(0,0);
    //       psiM(0,0) = 1.0/psiM(0,0);
    //     } else {
    //     LUFactorization(nrows, ncols, psiM.data(), nrows, &pivot[0]);
    //     CurrentDet = psiM(0,0);
    //     for (int i=1; i<ncols; ++i) CurrentDet *= psiM(i,i);
    //     InvertLU(nrows, psiM.data(), nrows, &pivot[0], &work[0], nrows);
    //     }

    int iat = FirstIndex; //the index of the particle with respect to P
    for(int i=0; i<nrows; i++, iat++) {
      PosType rv = psiM(i,0)*dpsiM(i,0);
      ValueType lap=psiM(i,0)*d2psiM(i,0);
      for(int j=1; j<ncols; j++) {
	rv += psiM(i,j)*dpsiM(i,j);
	lap += psiM(i,j)*d2psiM(i,j);
      }
      G(iat) += rv;
      L(iat) += lap - dot(rv,rv);
    }

    //complete the gradient and laplacian terms and add to the gradient
    //     const ValueType* logdet = psiM.data();
    //     const ValueType* d2logdet = d2psiM.data();
    //     const PosType* dlogdet = dpsiM.data();
    //     int iat = FirstIndex; //the index of the particle with respect to P
    //     for(int i=0; i<nrows; 
    // 	i++, iat++,logdet+=ncols,dlogdet+=ncols,d2logdet+=ncols) {
    //       PosType rv = dot(logdet,dlogdet, ncols);
    //       G(iat) += rv;
    //       L(iat) += dot(logdet,d2logdet,ncols) - dot(rv,rv);
    //     }
    return CurrentDet;
  }


  /**void evaluate(WalkerSetRef& W,  WfsVec& psi, GradMat& G, LapMat& L)
   *@param W Walkers, set of input configurations, Nw is the number of walkers
   *@param psi a vector containing Nw determinants
   *@param G a matrix containing Nw x N gradients
   *@param L a matrix containing Nw x N laplacians
   *@brief N is the number of particles per walker and Nw is the number of walkers.
   *Designed for vectorized move, i.e., all the walkers move simulatenously as
   *in molecu. While calculating the determinant values for a set of walkers,
   *add the gradient and laplacian contribution of the determinant
   *to G and L for local energy calculations.
   */
  template<class SPOSet>
  template<class WfsVec, class GradMat, class LapMat>
  inline void 
  DiracDeterminant<SPOSet>::evaluate(WalkerSetRef& W,  
				     WfsVec& psi, 
				     GradMat& G, 
				     LapMat& L) {

    int nw = W.walkers();

    //evaluate \f$(D_{ij})^t\f$ and other quantities for gradient/laplacians
    Phi.evaluate(W, FirstIndex, LastIndex, psiM_v, dpsiM_v, d2psiM_v);
    int nrows = rows();
    int ncols = cols();
    
    for(int iw=0; iw< nw; iw++) {
      psi[iw] *= Invert(psiM_v[iw].data(),nrows,ncols);
      int iat = FirstIndex; //the index of the particle with respect to P
      const Determinant_t& logdet = psiM_v[iw];
      const Gradient_t& dlogdet = dpsiM_v[iw];
      const Laplacian_t& d2logdet = d2psiM_v[iw];

      for(int i=0; i<nrows; i++, iat++) {
	PosType rv = logdet(i,0)*dlogdet(i,0);
	ValueType lap=logdet(i,0)*d2logdet(i,0);
	for(int j=1; j<ncols; j++) {
	  rv += logdet(i,j)*dlogdet(i,j);
	  lap += logdet(i,j)*d2logdet(i,j);
	}
	G(iw,iat) += rv;
	L(iw,iat) += lap - dot(rv,rv);
      }
    }
  }
  
  template<class SPOSet>
  void DiracDeterminant<SPOSet>::resizeByWalkers(int nw) {
    if(psiM_v.size() < nw) {
      psiM_v.resize(nw);
      dpsiM_v.resize(nw);
      d2psiM_v.resize(nw);
      for(int iw=0; iw<nw; iw++) psiM_v[iw].resize(rows(),cols());
      for(int iw=0; iw<nw; iw++) dpsiM_v[iw].resize(rows(),cols());
      for(int iw=0; iw<nw; iw++) d2psiM_v[iw].resize(rows(),cols());
    }
    Phi.resizeByWalkers(nw);
  }
}
#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
