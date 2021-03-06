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
/**@file WaveFunctionPool.h
 * @brief Declaration of WaveFunctionPool
 */
#ifndef OHMMS_QMC_WAVEFUNCTIONPOOL_H
#define OHMMS_QMC_WAVEFUNCTIONPOOL_H

#include "OhmmsData/OhmmsElementBase.h"
#include <map>
#include <string>

namespace ohmmsqmc {

  class TrialWaveFunction;
  class ParticleSetPool;

  /* A collection of TrialWaveFunction objects
   */
  class WaveFunctionPool : public OhmmsElementBase {

  public:

    WaveFunctionPool(const char* aname = "wavefunction");

    bool get(std::ostream& os) const;
    bool put(std::istream& is);
    bool put(xmlNodePtr cur);
    void reset();

    inline bool empty() const { return myPool.empty();}

    TrialWaveFunction* getPrimary() {
      return primaryPsi;
    }

    TrialWaveFunction* getWaveFunction(const std::string& pname) {
      std::map<std::string,TrialWaveFunction*>::iterator pit(myPool.find(pname));
      if(pit == myPool.end()) 
        return 0;
      else 
        return (*pit).second;
    }

    /** assign a pointer of ParticleSetPool
     */
    inline void 
    setParticleSetPool(ParticleSetPool* pset) { ptclPool=pset;}

  private:

    TrialWaveFunction* primaryPsi;
    std::map<std::string,TrialWaveFunction*> myPool;

    /** pointer to ParticleSetPool
     *
     * TrialWaveFunction needs to know which ParticleSet object
     * is used as an input object for the evaluations.
     */
    ParticleSetPool* ptclPool;
  };
}
#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
