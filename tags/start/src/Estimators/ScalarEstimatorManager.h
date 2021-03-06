//////////////////////////////////////////////////////////////////
// (c) Copyright 2003- by Jeongnim Kim and Jordan Vincent
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   Jeongnim Kim and Jordan Vincent
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
#ifndef OHMMS_QMC_SCALAR_ESTIMATORMANAGER_H
#define OHMMS_QMC_SCALAR_ESTIMATORMANAGER_H

#include "Configuration.h"
#include "Estimators/ScalarEstimatorBase.h"

namespace ohmmsqmc {

  class MCWalkerConifugration;
  class QMCHamiltonian;

  /**Class to manage a set of ScalarEstimators */
  class ScalarEstimatorManager: public QMCTraits {

  public:

    typedef ScalarEstimatorBase<RealType> EstimatorType;

    ScalarEstimatorManager(QMCHamiltonian& h);
    ~ScalarEstimatorManager();

    ///return the number of ScalarEstimators
    inline int size() const { return Estimators.size();}

    ///process xml tag associated with estimators
    bool put(xmlNodePtr cur);

    void accumulate(const MCWalkerConfiguration& W);
    void resetReportSettings(const string& aname);
    void reportHeader();
    void flushreport(int iter);
    void report(int iter);
    void flush();
    void finalize();
    void reset();
  
    /*!\return the index of the named column
     *\brief add a column with the name
     */
    inline int addColumn(const char* aname) {
      return BlockAverages.add(aname);
    }

    ///add the value of the i-th column with a value v
    inline void setColumn(int i, RealType v) {
      BlockAverages[i] = v;
    }

    /*!\return the index of the newestimator
     *\brief add a new estimator with name aname
     */
    int add(EstimatorType* newestimator, const string& aname);

    ///set the stride for all the estimators
    void setStride(int istride);

    ///return a pointer to the estimator aname
    EstimatorType* getEstimator(const string& a);
 
    ///return the average for estimator i
    inline RealType average(int i) const { 
      return Estimators[i]->average(); 
    }
    
    ///returns a variance for estimator i
    inline RealType variance(int i) const { 
      return Estimators[i]->variance();
    }
  
  private:

    string RootName;
    int Stride;
    int WeightIndex;
    ostream* OutStream;
    RealType WeightSum;
    QMCHamiltonian& H;
    RecordNamedProperty<RealType> BlockAverages;
    std::map<string,int> EstimatorMap;
    vector<EstimatorType*> Estimators;
  };
}
#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
