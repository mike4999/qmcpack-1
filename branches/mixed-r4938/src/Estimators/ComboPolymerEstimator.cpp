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
#include "Estimators/ComboPolymerEstimator.h"
#include "QMCHamiltonians/QMCHamiltonian.h"
#include "QMCWaveFunctions/TrialWaveFunction.h"
#include "ParticleBase/ParticleAttribOps.h"
#include "Message/CommOperators.h"
#include "QMCDrivers/DriftOperators.h"
#include "QMCDrivers/MultiChain.h"
#include "OhmmsData/AttributeSet.h"
#include "ReptationEstimators/ReptileStatistics.h"
#include "ReptationEstimators/ReptileCorrelation.h"
#include "Utilities/SimpleParser.h"

// #include <string>
// #include <algorithm>
// #include <vector>


namespace qmcplusplus {
  
  //duplicated implementation: use parsewords in SimpleParser.h
  void Tokenize(const string& str,
                      vector<string>& tokens,
                      const string& delimiters = " ")
{
//   used from http://oopweb.com/CPP/Documents/CPPHOWTO/Volume/C++Programming-HOWTO-7.html
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos     = str.find_first_of(delimiters, lastPos);

    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        tokens.push_back(str.substr(lastPos, pos - lastPos));
        // Skip delimiters.  Note the "not_of"
        lastPos = str.find_first_not_of(delimiters, pos);
        // Find next "non-delimiter"
        pos = str.find_first_of(delimiters, lastPos);
    }
}


  /** constructor
   * @param h QMCHamiltonian to define the components
   * @param hcopy number of copies of QMCHamiltonians
   */
  ComboPolymerEstimator::ComboPolymerEstimator(QMCHamiltonian& h, int hcopy, MultiChain* polymer)
  {
    Reptile=polymer;
    NumCopies=hcopy;

    SizeOfHamiltonians = h.sizeOfObservables();
    FirstHamiltonian = h.startIndex();

    //Mixed Estimator quantities
    scalars_name.push_back("LocalEnergy");
    for(int i=0; i < SizeOfHamiltonians; i++)
    {
      scalars_name.push_back(h.getObservableName(i));
    };
  };

  ComboPolymerEstimator::ComboPolymerEstimator(const ComboPolymerEstimator& mest): 
       PolymerEstimator(mest)
  {
    Reptile=mest.Reptile;
    NumCopies=mest.NumCopies;
  }

  void ComboPolymerEstimator::put(xmlNodePtr cur,  MCWalkerConfiguration& refWalker, int ReptileLength)
  {
    xmlNodePtr cur2(cur);
    xmlNodePtr cur3(cur);
    cur2 = cur2->children;
    while(cur2 != NULL) {
      string cname((const char*)cur2->name);
//       string potType("Null");
//       OhmmsAttributeSet attrib;
//       attrib.add(potType,"type");
//       attrib.put(cur2);
      if((cname == "center")||(cname == "Center") )
      {
	string tags;
	OhmmsAttributeSet Tattrib;
	Tattrib.add(tags,"names");
	Tattrib.put(cur2);
	string name;
	vector<string> names;
	Tokenize(tags,names,",");
        //parsewords(tags.c_str(),names);
	app_log()<<"  Found center block: "<<tags<<endl;
// 	some special cases
//         app_log()<<names.size()<<names[names.size()-1]<<tags<<endl;
	for(int j=0;j<names.size();j++){
// 	  app_log()<<names[j]<<endl;
	  if(names[j]=="LocalPotential"){
	    scalars_name.push_back("C_LocPot");
	    scalars_index.push_back(LOCALPOTENTIAL);
	  }else if(names[j]=="LogPsi"){
	    scalars_name.push_back("C_LogPsi");
	    scalars_index.push_back(LOGPSI);
	  }
	}
	vector<string> tmp(scalars_name);
	for(int i=0;i<tmp.size();i++){
	  for(int j=0;j<names.size();j++){
	    if (string::npos!=tmp[i].rfind(names[j]) ){
	      tmp[i]="";
	      name ="C_";
	      name +=scalars_name[i];
	      scalars_name.push_back(name);
	      scalars_index.push_back(i-1+FirstHamiltonian);
	    }
	  }   
	}
      } 
      cur2 = cur2->next;
    }
    //Need to add those first for evaluation purposes
    
    cur3 = cur3->children;
    while(cur3 != NULL) {
      string cname((const char*)cur3->name);
      if ((cname == "stats")||(cname == "Stats")) {
        ReptileStatistics* RS = new ReptileStatistics(Reptile);
	RS->put(cur3);
	RS->addNames(scalars_name);
	RepEstimators.push_back(RS);
      } else if ((cname == "correlation")||(cname == "Correlation")) {
	app_log()<<"  found Correlation Block"<<endl;
	ReptileCorrelationEstimator* RC = new ReptileCorrelationEstimator(Reptile);
        RC->put(cur3,ReptileLength,FirstHamiltonian );
        RC->addNames(scalars_name);
        RepEstimators.push_back(RC);
      }
      cur3 = cur3->next;
    }
    
    
    scalars.resize(scalars_name.size());
    scalars_saved=scalars;
  }
  
  ScalarEstimatorBase* ComboPolymerEstimator::clone()
  {
    return new ComboPolymerEstimator(*this);
  }

  /**  add the local energy, variance and all the Hamiltonian components to the scalar record container
   *@param record storage of scalar records (name,value)
   */
  void ComboPolymerEstimator::add2Record(RecordNamedProperty<RealType>& record) {
    FirstIndex = record.add(scalars_name[0].c_str());
    for(int i=1; i<scalars_name.size(); i++) record.add(scalars_name[i].c_str());
    LastIndex=FirstIndex + scalars_name.size();
    clear();
    char aname[32];
    for(int i=0; i<NumCopies-1; i++) {
      for(int j=i+1; j<NumCopies; j++) {
        sprintf(aname,"DiffS%iS%i",i,j); 
        int dummy=record.add(aname);
      }
    }
  };

  void ComboPolymerEstimator::accumulate(const MCWalkerConfiguration& W
      , WalkerIterator first, WalkerIterator last, RealType wgt)
  {
    for(int i=0; i<NumCopies; i++) 
    {
      RealType uw(Reptile->UmbrellaWeight[i]);
      //Adding all parts from the Hamiltonian
      RealType* restrict HeadProp(Reptile->front()->getPropertyBase(i));
      RealType* restrict TailProp(Reptile->back()->getPropertyBase(i));
      RealType eloc = 0.5*( HeadProp[LOCALENERGY] + TailProp[LOCALENERGY]);
      scalars[0](eloc,uw);
      for(int obsi=0;obsi<SizeOfHamiltonians ;obsi++){
        scalars[obsi+1]( 0.5*( HeadProp[obsi+FirstHamiltonian] + TailProp[obsi+FirstHamiltonian]) , uw);
      }
      RealType* restrict CenProp(Reptile->center()->getPropertyBase(i));
      for(int obsi=0;obsi<scalars_index.size() ;obsi++){
        scalars[obsi+1+SizeOfHamiltonians]( CenProp[scalars_index[obsi]] , uw);
      }
      MultiChain::iterator first(Reptile->begin());
      MultiChain::iterator last(Reptile->end());
      for(int j=0;j<RepEstimators.size();j++){
	RepEstimators[j]->evaluate(first,last,i);
	RepEstimators[j]->setValues(scalars,uw);
      }
    }
  }

  void ComboPolymerEstimator::evaluateDiff() 
  {

  }

  void ComboPolymerEstimator::registerObservables(vector<observable_helper*>& h5dec, hid_t gid)
  {
    //IMPLEMENT for hdf5
  }

}
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 1926 $   $Date: 2007-04-20 12:30:26 -0500 (Fri, 20 Apr 2007) $
 * $Id: MJPolymerEstimator.cpp 1926 2007-04-20 17:30:26Z jnkim $
 ***************************************************************************/
