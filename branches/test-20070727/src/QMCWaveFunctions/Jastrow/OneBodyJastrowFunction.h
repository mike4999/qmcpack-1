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
#ifndef QMCPLUSPLUS_GENERIC_ONEBODYJASTROW_H
#define QMCPLUSPLUS_GENERIC_ONEBODYJASTROW_H
#include "Configuration.h"
#include "QMCWaveFunctions/OrbitalBase.h"

namespace qmcplusplus {
  
  /** @ingroup OrbitalComponent
   * @brief generic implementation of one-body Jatrow function.
   *
   *The One-Body Jastrow has the form  
   \f$ \psi = \exp{\left(-J_1\right)} \f$
   \f[ J_{1}({\bf R}) = \sum_{I=1}^{N_I} 
   \sum_{i=1}^{N_e} u(r_{iI}) \f]
   where \f[ r_{iI} = |{\bf r}_i - {\bf R}_I| \f]
   and the first summnation \f$ 1..N_I \f$ is over the centers,
   e.g., nulcei for molecular or solid systems,
   while the second summnation \f$ 1..N_e \f$ is over quantum particles.
   *
   *The template parameter FT is a functional \f$ u(r_{iI}), \f$ e.g.
   *class PadeJastrow<T> or NoCuspJastrow<T>
   *Requriement of the template function is 
   *ValueType evaluate(ValueType r, ValueType& dudr, ValueType& d2udr2).
   *
   To calculate the Gradient use the identity
   \f[ {\bf \nabla}_i(r_{iI}) = \frac{{{\bf r_{iI}}}}{r_{iI}}. \f] 
   \f[ {\bf \nabla}_i(J_{e}({\bf R})= 
   \sum_{I=1}^{N_I} \frac{du}{dr_{iI}}{\bf \hat{r_{iI}}}.
   \f]
   To calculate The Laplacian use the identity
   \f[ \nabla^2_i(r_{iI})=\frac{2}{r_{iI}}, \f]
   and the vector product rule
   \f[ 
   \nabla^2 \cdot (f{\bf A}) =
   f({\bf \nabla} \cdot {\bf A})+{\bf A}\cdot({\bf \nabla} f)
   \f]
   \f[
   \nabla^2_i (J_{e}({\bf R})) = \sum_{I=1}^{N_I} 
   \left(\frac{du}{dr_{iI}}\right) {\bf \nabla}_i
   \cdot {\bf \hat{r_{iI}}} - {\bf \hat{r_{iI}}} \cdot 
   \left(\frac{d^2u}{dr_{iI}^2}\right){\bf \hat{r_{iI}}}
   \f]
   which can be simplified to
   \f[
   \nabla^2_i (J_{e}({\bf R})) = \sum_{I=1}^{N_I}
   \left(\frac{2}{r_{iI}}\frac{du}{dr_{iI}}
   + \frac{d^2u}{dr_{iI}^2}\right)
   \f]
   *
   *A generic OneBodyJastrow function uses a distance table.
   *The indices I(sources) and i(targets) are distinct. In general, the source 
   *particles are fixed, e.g., the nuclei, while the target particles are updated
   *by MC methods.
   */
  template<class FT>
  class OneBodyJastrow: public OrbitalBase {

    const ParticleSet& CenterRef;
    const DistanceTableData* d_table;

    RealType curVal, curLap;
    PosType curGrad;
    ParticleAttrib<RealType> U,d2U;
    ParticleAttrib<PosType> dU;
    RealType *FirstAddressOfdU, *LastAddressOfdU;
    vector<FT*> Fs;
    vector<FT*> Funique;

  public:

    typedef FT FuncType;


    ///constructor
    OneBodyJastrow(const ParticleSet& centers, ParticleSet& els)
      : CenterRef(centers), d_table(0), FirstAddressOfdU(0), LastAddressOfdU(0){ 
      U.resize(els.getTotalNum());
      d_table = DistanceTable::add(CenterRef,els);
    }

    ~OneBodyJastrow(){ }

    //evaluate the distance table with P
    void resetTargetParticleSet(ParticleSet& P) {
      d_table = DistanceTable::add(CenterRef,P);
    }

    void addFunc(int source_type, FT* afunc) {
      if(Fs.empty()) {
        Fs.resize(CenterRef.getTotalNum(),0);
      }
      for(int i=0; i<Fs.size(); i++) {
        if(CenterRef.GroupID[i] == source_type) Fs[i]=afunc;
      }
      Funique.push_back(afunc);
    }

    void resetParameters(OptimizableSetType& optVariables) 
    { 
      for(int i=0; i<Funique.size(); i++) 
        if(Funique[i]) Funique[i]->resetParameters(optVariables);
    }

    /** 
     *@param P input configuration containing N particles
     *@param G a vector containing N gradients
     *@param L a vector containing N laplacians
     *@return The wavefunction value  \f$exp(-J({\bf R}))\f$
     *
     *Upon exit, the gradient \f$G[i]={\bf \nabla}_i J({\bf R})\f$
     *and the laplacian \f$L[i]=\nabla^2_i J({\bf R})\f$ are accumulated.
     *While evaluating the value of the Jastrow for a set of
     *particles add the gradient and laplacian contribution of the
     *Jastrow to G(radient) and L(aplacian) for local energy calculations
     *such that \f[ G[i]+={\bf \nabla}_i J({\bf R}) \f] 
     *and \f[ L[i]+=\nabla^2_i J({\bf R}). \f]
     */
    ValueType evaluateLog(ParticleSet& P,
		          ParticleSet::ParticleGradient_t& G, 
		          ParticleSet::ParticleLaplacian_t& L) {
      LogValue=0.0;
      U=0.0;
      RealType dudr, d2udr2;
      for(int i=0; i<d_table->size(SourceIndex); i++) {
        FT* func=Fs[i];
        if(func == 0) continue;
	for(int nn=d_table->M[i]; nn<d_table->M[i+1]; nn++) {
	  int j = d_table->J[nn];
          //ValueType uij= F[d_table->PairID[nn]]->evaluate(d_table->r(nn), dudr, d2udr2);
          RealType uij= func->evaluate(d_table->r(nn), dudr, d2udr2);
          LogValue -= uij; U[j] += uij;
	  dudr *= d_table->rinv(nn);
	  G[j] -= dudr*d_table->dr(nn);
	  L[j] -= d2udr2+2.0*dudr;
	}
      }
      return LogValue;
    }

    ValueType evaluate(ParticleSet& P,
		       ParticleSet::ParticleGradient_t& G, 
		       ParticleSet::ParticleLaplacian_t& L) {
      return std::exp(evaluateLog(P,G,L));
    }

    /** evaluate the ratio \f$exp(U(iat)-U_0(iat))\f$
     * @param P active particle set
     * @param iat particle that has been moved.
     */
    inline ValueType ratio(ParticleSet& P, int iat) {
      int n=d_table->size(VisitorIndex);
      curVal=0.0;
      for(int i=0, nn=iat; i<d_table->size(SourceIndex); i++,nn+= n) {
        if(Fs[i]) curVal += Fs[i]->evaluate(d_table->Temp[i].r1);
      }
      return std::exp(U[iat]-curVal);
    }

    /** evaluate the ratio \f$exp(U(iat)-U_0(iat))\f$ and fill-in the differential gradients/laplacians
     * @param P active particle set
     * @param iat particle that has been moved.
     * @param dG partial gradients
     * @param dL partial laplacians
     */
    inline ValueType ratio(ParticleSet& P, int iat,
		    ParticleSet::ParticleGradient_t& dG,
		    ParticleSet::ParticleLaplacian_t& dL)  {
      //int n=d_table->size(VisitorIndex);
      //curVal=0.0;
      //curLap=0.0;
      //curGrad = 0.0;
      //RealType dudr, d2udr2;
      //for(int i=0, nn=iat; i<d_table->size(SourceIndex); i++,nn+= n) {
      //  if(Fs[i]) {
      //    curVal += Fs[i]->evaluate(d_table->Temp[i].r1,dudr,d2udr2);
      //    dudr *= d_table->Temp[i].rinv1;
      //    curGrad -= dudr*d_table->Temp[i].dr1;
      //    curLap  -= d2udr2+2.0*dudr;
      //  }
      //  //int ij=d_table->PairID[nn];
      //  //curVal += F[ij]->evaluate(d_table->Temp[i].r1,dudr,d2udr2);
      //  //dudr *= d_table->Temp[i].rinv1;
      //  //curGrad -= dudr*d_table->Temp[i].dr1;
      //  //curLap  -= d2udr2+2.0*dudr;
      //}
      //dG[iat] += curGrad-dU[iat];
      //dL[iat] += curLap-d2U[iat]; 
      //return std::exp(U[iat]-curVal);
      return std::exp(logRatio(P,iat,dG,dL));
    }

    inline ValueType logRatio(ParticleSet& P, int iat,
		    ParticleSet::ParticleGradient_t& dG,
		    ParticleSet::ParticleLaplacian_t& dL)  {
      int n=d_table->size(VisitorIndex);
      curVal=0.0;
      curLap=0.0;
      curGrad = 0.0;
      RealType dudr, d2udr2;
      for(int i=0, nn=iat; i<d_table->size(SourceIndex); i++,nn+= n) {
        if(Fs[i]) {
          curVal += Fs[i]->evaluate(d_table->Temp[i].r1,dudr,d2udr2);
          dudr *= d_table->Temp[i].rinv1;
          curGrad -= dudr*d_table->Temp[i].dr1;
          curLap  -= d2udr2+2.0*dudr;
        }
      }
      dG[iat] += curGrad-dU[iat];
      dL[iat] += curLap-d2U[iat]; 
      return U[iat]-curVal;
    }

    inline void restore(int iat) {}

    void acceptMove(ParticleSet& P, int iat) {
      U[iat] = curVal;
      dU[iat]=curGrad;
      d2U[iat]=curLap;
    }
    

    void update(ParticleSet& P, 
		ParticleSet::ParticleGradient_t& dG, 
		ParticleSet::ParticleLaplacian_t& dL,
		int iat) {
      dG[iat] += curGrad-dU[iat]; dU[iat]=curGrad;
      dL[iat] += curLap-d2U[iat]; d2U[iat]=curLap;
      U[iat] = curVal;
    }

    void evaluateLogAndStore(ParticleSet& P, 
		ParticleSet::ParticleGradient_t& dG, 
		ParticleSet::ParticleLaplacian_t& dL) {

      LogValue = 0.0;
      U=0.0; dU=0.0; d2U=0.0;
      RealType uij, dudr, d2udr2;
      for(int i=0; i<d_table->size(SourceIndex); i++) {
        FT* func=Fs[i];
        if(func == 0) continue;
	for(int nn=d_table->M[i]; nn<d_table->M[i+1]; nn++) {
	  int j = d_table->J[nn];
	  //uij = F[d_table->PairID[nn]]->evaluate(d_table->r(nn), dudr, d2udr2);
	  uij = func->evaluate(d_table->r(nn), dudr, d2udr2);
          LogValue-=uij;
          U[j]+=uij; 
	  dudr *= d_table->rinv(nn);
	  dU[j] -= dudr*d_table->dr(nn);
	  d2U[j] -= d2udr2+2.0*dudr;

	  //add gradient and laplacian contribution
	  dG[j] -= dudr*d_table->dr(nn);
	  dL[j] -= d2udr2+2.0*dudr;
	}
      }
    }

    /** equivalent to evalaute with additional data management */
    ValueType registerData(ParticleSet& P, PooledData<RealType>& buf){

      //U.resize(d_table->size(VisitorIndex));
      d2U.resize(d_table->size(VisitorIndex));
      dU.resize(d_table->size(VisitorIndex));
      FirstAddressOfdU = &(dU[0][0]);
      LastAddressOfdU = FirstAddressOfdU + dU.size()*DIM;

      evaluateLogAndStore(P,P.G,P.L);

      //add U, d2U and dU. Keep the order!!!
      buf.add(U.begin(), U.end());
      buf.add(d2U.begin(), d2U.end());
      buf.add(FirstAddressOfdU,LastAddressOfdU);

      return LogValue;
    }

    ValueType updateBuffer(ParticleSet& P, BufferType& buf)  {
      evaluateLogAndStore(P,P.G,P.L);
      //LogValue = 0.0;
      //U=0.0; dU=0.0; d2U=0.0;
      //RealType uij, dudr, d2udr2;
      //for(int i=0; i<d_table->size(SourceIndex); i++) {
      //  FT* func=Fs[i];
      //  if(func == 0) continue;
      //  for(int nn=d_table->M[i]; nn<d_table->M[i+1]; nn++) {
      //    int j = d_table->J[nn];
      //    //uij = F[d_table->PairID[nn]]->evaluate(d_table->r(nn), dudr, d2udr2);
      //    uij = func->evaluate(d_table->r(nn), dudr, d2udr2);
      //    LogValue-=uij;
      //    U[j]+=uij; 
      //    dudr *= d_table->rinv(nn);
      //    dU[j] -= dudr*d_table->dr(nn);
      //    d2U[j] -= d2udr2+2.0*dudr;

      //    //add gradient and laplacian contribution
      //    P.G[j] -= dudr*d_table->dr(nn);
      //    P.L[j] -= d2udr2+2.0*dudr;
      //  }
      //}

      //FirstAddressOfdU = &(dU[0][0]);
      //LastAddressOfdU = FirstAddressOfdU + dU.size()*DIM;

      buf.put(U.first_address(), U.last_address());
      buf.put(d2U.first_address(), d2U.last_address());
      buf.put(FirstAddressOfdU,LastAddressOfdU);

      return LogValue;
    }

    /** copy the current data from a buffer
     *@param P the ParticleSet to operate on
     *@param buf PooledData which stores the data for each walker
     *
     *copyFromBuffer uses the data stored by registerData or evaluate(P,buf)
     */
    void copyFromBuffer(ParticleSet& P, PooledData<RealType>& buf) {
      buf.get(U.first_address(), U.last_address());
      buf.get(d2U.first_address(), d2U.last_address());
      buf.get(FirstAddressOfdU,LastAddressOfdU);
    }

    /** return the current value and copy the current data to a buffer
     *@param P the ParticleSet to operate on
     *@param buf PooledData which stores the data for each walker
     */
    inline ValueType evaluate(ParticleSet& P, PooledData<RealType>& buf) {
      ValueType sumu = 0.0;
      for(int i=0; i<U.size(); i++) sumu+=U[i];

      buf.put(U.first_address(), U.last_address());
      buf.put(d2U.first_address(), d2U.last_address());
      buf.put(FirstAddressOfdU,LastAddressOfdU);
      return std::exp(-sumu);
    }

  };

}
#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/

