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
#ifndef OHMMS_QMC_GENERIC_JASTROW_BUILDER_H
#define OHMMS_QMC_GENERIC_JASTROW_BUILDER_H
#include "QMCWaveFunctions/OrbitalBuilderBase.h"

namespace ohmmsqmc {

  //forward declaration
  class ParticleSet;
  
  /**
   *A builder class to add a one- or two-body Jastrow function to
   *a TrialWaveFunction from a xml file.
   *
   *A xml node with OrbtialBuilderBase::jastrow_tag is parsed recursively.
   */
  class JastrowBuilder: public OrbitalBuilderBase {

    /// a vector containing the different particle sets (ions and electrons)
    vector<ParticleSet*>& PtclSets;

    /// the element name for Correlation, "correlation"
    string corr_tag; 

public:

    JastrowBuilder(TrialWaveFunction& a, vector<ParticleSet*>& psets);
    
    /**@param cur the current xmlNodePtr to be processed by JastrowBuilder
     *@return true if succesful
     */
    bool put(xmlNodePtr cur);
    
   
    template<class JeeType>
    bool createTwoBodySpin(xmlNodePtr cur, JeeType* j2);
    

  
    template<class JeeType>
    bool createTwoBodyNoSpin(xmlNodePtr cur, JeeType* j2);
    
  
    template<class JneType>
    bool createOneBody(xmlNodePtr cur, JneType* j1);
    
  };
}
#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
