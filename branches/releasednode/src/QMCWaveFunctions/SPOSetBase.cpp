//////////////////////////////////////////////////////////////////
// (c) Copyright 2006-  by Jeongnim Kim
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
#include "QMCWaveFunctions/SPOSetBase.h"
#if defined(HAVE_LIBHDF5)
#include "Numerics/HDFNumericAttrib.h"
#endif
#include "Numerics/MatrixOperators.h"
#include "OhmmsData/AttributeSet.h"
#include "Message/Communicate.h"
#include <limits>

namespace qmcplusplus {

  template<typename T>
  inline void transpose(const T* restrict in, T* restrict out, int m)
   {
     for(int i=0,ii=0;i<m;++i)
       for(int j=0,jj=i;j<m; ++j,jj+=m)
         out[ii++]=in[jj];
   }

   void SPOSetBase::evaluate(const ParticleSet& P, int first, int last,
       ValueMatrix_t& logdet, GradMatrix_t& dlogdet, ValueMatrix_t& d2logdet)
   {
     evaluate_notranspose(P,first,last,t_logpsi,dlogdet,d2logdet);
     transpose(t_logpsi.data(),logdet.data(),OrbitalSetSize);
     //evaluate_notranspose(P,first,last,logdet,dlogdet,d2logdet);
     //MatrixOperators::transpose(logdet);
   }

   void SPOSetBase::evaluateGradSource (const ParticleSet &P
       , int first, int last, const ParticleSet &source
       , int iat_src, GradMatrix_t &gradphi) 
   {
     APP_ABORT("SPOSetlBase::evalGradSource is not implemented"); 
   }

   void SPOSetBase::evaluateGradSource (const ParticleSet &P, int first, int last, 
       const ParticleSet &source, int iat_src, 
       GradMatrix_t &grad_phi,
       HessMatrix_t &grad_grad_phi,
       GradMatrix_t &grad_lapl_phi)
   { 
     APP_ABORT("SPOSetlBase::evalGradSource is not implemented"); 
   }

   SPOSetBase* SPOSetBase::makeClone() const
   {
     APP_ABORT("Missing  SPOSetBase::makeClone for "+className);
     return 0;
   }


  /** Parse the xml file for information on the Dirac determinants.
   *@param cur the current xmlNode
   */
  bool SPOSetBase::put(xmlNodePtr cur) 
  {
    //initialize the number of orbital by the basis set size
    int norb= BasisSetSize;
    string debugc("no");

    OhmmsAttributeSet aAttrib;
    aAttrib.add(norb,"orbitals"); aAttrib.add(norb,"size");
    aAttrib.add(debugc,"debug");
    aAttrib.put(cur);

    setOrbitalSetSize(norb);
    TotalOrbitalSize=norb;

    //allocate temporary t_logpsi
    t_logpsi.resize(TotalOrbitalSize,OrbitalSetSize);

    const xmlChar* h=xmlGetProp(cur, (const xmlChar*)"href");
    xmlNodePtr occ_ptr=NULL;
    xmlNodePtr coeff_ptr=NULL;
    cur = cur->xmlChildrenNode;
    while(cur != NULL) {
      string cname((const char*)(cur->name));
      if(cname == "occupation") {
        occ_ptr=cur;
      } else if(cname.find("coeff") < cname.size() || cname == "parameter" || cname == "Var") {
        coeff_ptr=cur;
      }
      cur=cur->next;
    }

    if(coeff_ptr == NULL) {
      app_log() << "   Using Identity for the LCOrbitalSet " << endl;
      return setIdentity(true);
    }

    bool success=putOccupation(occ_ptr);

    if(h == NULL) {
      success = putFromXML(coeff_ptr);
    } else {
      success = putFromH5((const char*)h, coeff_ptr);
    }

    if(debugc=="yes")
    {
      app_log() << "   Single-particle orbital coefficients dims=" << C.rows() << " x " << C.cols() << endl;
      app_log() << C << endl;
    }
    return success;
  }

  void SPOSetBase::checkObject() {
    if(!(OrbitalSetSize == C.rows() && BasisSetSize == C.cols()))
    {
      app_error() << "   SPOSetBase::checkObject Linear coeffient for SPOSet is not consistent with the input." << endl; 
      OHMMS::Controller->abort();
    }
  }

  bool SPOSetBase::putOccupation(xmlNodePtr occ_ptr) {

    //die??
    if(BasisSetSize ==0) 
    {
      APP_ABORT("SPOSetBase::putOccupation detected ZERO BasisSetSize");
      return false;
    }

    Occ.resize(BasisSetSize);
    Occ=0.0;
    for(int i=0; i<OrbitalSetSize; i++) Occ[i]=1.0;

    vector<int> occ_in;
    string occ_mode("table");
    if(occ_ptr == NULL) {
      occ_mode="ground";
    } else {
      const xmlChar* o=xmlGetProp(occ_ptr,(const xmlChar*)"mode");  
      if(o) occ_mode = (const char*)o;
    }
    //Do nothing if mode == ground
    if(occ_mode == "excited") {
      putContent(occ_in,occ_ptr);
      for(int k=0; k<occ_in.size(); k++) {
        if(occ_in[k]<0) //remove this, -1 is to adjust the base
          Occ[-occ_in[k]-1]=0.0;
        else
          Occ[occ_in[k]-1]=1.0;
      }
    } else if(occ_mode == "table") {
      putContent(Occ,occ_ptr);
    }

    return true;
  }

  bool SPOSetBase::putFromXML(xmlNodePtr coeff_ptr) {
    vector<ValueType> Ctemp;
    int total(OrbitalSetSize);
    Identity=true;
    if(coeff_ptr != NULL){
      if(xmlHasProp(coeff_ptr, (const xmlChar*)"size")){
        total = atoi((const char*)(xmlGetProp(coeff_ptr, (const xmlChar *)"size")));
      } 
      Ctemp.resize(total*BasisSetSize);
      putContent(Ctemp,coeff_ptr);
      Identity = false;
    }

    setIdentity(Identity);

    if(!Identity) {
      int n=0,i=0;
      vector<ValueType>::iterator cit(Ctemp.begin());
      while(i<OrbitalSetSize){
        if(Occ[n]>numeric_limits<RealType>::epsilon()){
          std::copy(cit,cit+BasisSetSize,C[i]);
          i++; 
        }
        n++;cit+=BasisSetSize;
      }
    } 
    return true;
  }

    /** read data from a hdf5 file 
     * @param norb number of orbitals to be initialized
     * @param fname hdf5 file name
     * @param occ_ptr xmlnode for occupation
     * @param coeff_ptr xmlnode for coefficients
     */
    bool SPOSetBase::putFromH5(const char* fname, xmlNodePtr coeff_ptr) {

#if defined(HAVE_LIBHDF5)
      int norbs=OrbitalSetSize;
      int neigs=BasisSetSize;

      string setname("invalid");
      if(coeff_ptr) {
         const xmlChar* d=xmlGetProp(coeff_ptr,(const xmlChar*)"dataset");  
         if(d) setname = (const char*)d;
         d=xmlGetProp(coeff_ptr,(const xmlChar*)"size");  
         if(d) neigs=atoi((const char*)d);
      }

      setIdentity(false);

      if(setname != "invalid") {
        Matrix<RealType> Ctemp(neigs,BasisSetSize);
        hid_t h_file=  H5Fopen(fname,H5F_ACC_RDWR,H5P_DEFAULT);
        HDFAttribIO<Matrix<RealType> > h(Ctemp);
        h.read(h_file,setname.c_str());
        H5Fclose(h_file);
	int n=0,i=0;
	while(i<norbs){
	  if(Occ[n]>numeric_limits<RealType>::epsilon()){
            std::copy(Ctemp[n],Ctemp[n+1],C[i]);
	    i++; 
	  }
	  n++;
	}
      }

#else
      ERRORMSG("HDF5 is disabled. Using identity")
#endif
      return true;
    }

}

/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/

