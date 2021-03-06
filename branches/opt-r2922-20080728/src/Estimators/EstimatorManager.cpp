////////////////////////////////////////////////////////////////////
// (c) Copyright 2003- by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   Jeongnim Kim
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
#include "Particle/MCWalkerConfiguration.h"
#include "Estimators/EstimatorManager.h"
#include "QMCHamiltonians/QMCHamiltonian.h"
#include "Message/Communicate.h"
#include "Message/CommOperators.h"
#include "Message/CommUtilities.h"
#include "Estimators/LocalEnergyEstimator.h"
#include "Estimators/LocalEnergyOnlyEstimator.h"
#include "Estimators/CompositeEstimators.h"
#include "Estimators/GofREstimator.h"
#include "Estimators/SkEstimator.h"
#include "QMCDrivers/SimpleFixedNodeBranch.h"
//#include "Estimators/PolarizationEstimator.h"
#include "Utilities/IteratorUtility.h"
#include "Numerics/HDFNumericAttrib.h"
#include "OhmmsData/HDFStringAttrib.h"
#include "HDFVersion.h"
//#define QMC_ASYNC_COLLECT
//leave it for serialization debug
//#define DEBUG_ESTIMATOR_ARCHIVE

namespace qmcplusplus {

  /** enumeration for EstimatorManager.Options
   */
  enum {COLLECT=0, 
  MANAGE,
  RECORD, 
  POSTIRECV, 
  APPEND};

  //initialize the name of the primary estimator
  EstimatorManager::EstimatorManager(Communicate* c): RecordCount(0),h_file(-1),
  MainEstimatorName("elocal"), Archive(0), DebugArchive(0),
  myComm(0), MainEstimator(0), CompEstimators(0), pendingRequests(0)
  { 
    setCommunicator(c);
  }

  EstimatorManager::EstimatorManager(EstimatorManager& em): RecordCount(0),h_file(-1),
  MainEstimatorName("elocal"),
  Options(em.Options), Archive(0), DebugArchive(0),MainEstimator(0), CompEstimators(0), 
  EstimatorMap(em.EstimatorMap), pendingRequests(0)
  {
    //inherit communicator
    setCommunicator(em.myComm);
    for(int i=0; i<em.Estimators.size(); i++) 
      Estimators.push_back(em.Estimators[i]->clone());

    MainEstimator=Estimators[EstimatorMap[MainEstimatorName]];

    if(em.CompEstimators)//copy composite estimator
    {
      CompEstimators = new CompositeEstimatorSet(*(em.CompEstimators));
    }
  }

  EstimatorManager::~EstimatorManager()
  { 
    delete_iter(Estimators.begin(), Estimators.end());
    if(CompEstimators) delete CompEstimators;
    delete_iter(RemoteData.begin(), RemoteData.end());
  }

  void EstimatorManager::setCommunicator(Communicate* c) 
  {
    if(myComm && myComm == c) return;
    myComm = c ? c:OHMMS::Controller;

    //set the default options
    Options.set(COLLECT,myComm->size()>1);
    Options.set(MANAGE,myComm->rank() == 0);

    myRequest.resize(2);
    if(Options[COLLECT] && Options[MANAGE]) myRequest.resize(myComm->size()-1);

    if(RemoteData.empty())
      for(int i=0; i<myComm->size(); i++) RemoteData.push_back(new BufferType);
  }

  /** set CollectSum
   * @param collect if true, global sum is done over the values
   */
  void EstimatorManager::setCollectionMode(bool collect) 
  {
    if(!myComm) setCommunicator(0);
    Options.set(COLLECT,(myComm->size() == 1)? false:collect);
    //force to be false for serial runs
    //CollectSum = (myComm->size() == 1)? false:collect;
  }

  /** reset names of the properties 
   *
   * The number of estimators and their order can vary from the previous state.
   * Clear properties before setting up a new BlockAverage data list.
   */
  void EstimatorManager::reset()
  {

    weightInd = BlockProperties.add("BlockWeight");
    cpuInd = BlockProperties.add("BlockCPU");
    acceptInd = BlockProperties.add("AcceptRatio");

    BlockAverages.clear();//cleaup the records
    for(int i=0; i<Estimators.size(); i++) 
      Estimators[i]->add2Record(BlockAverages);
  }

  void EstimatorManager::resetTargetParticleSet(ParticleSet& p)
  {
    if(CompEstimators) CompEstimators->resetTargetParticleSet(p);
  }

  void EstimatorManager::addHeader(ostream& o)
  {
    o.setf(ios::scientific, ios::floatfield);
    o.setf(ios::left,ios::adjustfield);
    o << "#   index    ";
    for(int i=0; i<BlockAverages.size(); i++) o << setw(16) << BlockAverages.Names[i];
    //(*Archive) << setw(16) << "WeightSum";
    for(int i=0; i<BlockProperties.size(); i++) o << setw(16) << BlockProperties.Names[i];
    o << endl;
    o.setf(ios::right,ios::adjustfield);
  }

  void EstimatorManager::start(int blocks, bool record)
  {

    reset();
    RecordCount=0;
    energyAccumulator.clear();
    varAccumulator.clear();

    BlockAverages.setValues(0.0);
    AverageCache.resize(BlockAverages.size());
    PropertyCache.resize(BlockProperties.size());

    //count the buffer size for message
    BufferSize=AverageCache.size()+PropertyCache.size();

    if(CompEstimators) 
    {
      int comp_size = CompEstimators->size();
      if(comp_size) 
        BufferSize += comp_size;
      else
      {
        delete CompEstimators;
        CompEstimators=0;
      }
    }

    //allocate buffer for data collection
    if(RemoteData.empty())
      for(int i=0; i<myComm->size(); i++) RemoteData.push_back(new BufferType(BufferSize));
    else
      for(int i=0; i<myComm->size(); i++) RemoteData[i]->resize(BufferSize);

#if defined(DEBUG_ESTIMATOR_ARCHIVE)
    if(record && DebugArchive ==0)
    {
      char fname[128];
      sprintf(fname,"%s.p%03d.scalar.dat",myComm->getName().c_str(), myComm->rank());
      DebugArchive = new ofstream(fname);
      addHeader(*DebugArchive);
    }
#endif

    //set Options[RECORD] to enable/disable output 
    Options.set(RECORD,record&&Options[MANAGE]);

    if(Options[RECORD])
    {
      if(Archive) delete Archive;
      string fname(myComm->getName());
      fname.append(".scalar.dat");
      Archive = new ofstream(fname.c_str());
      addHeader(*Archive);

      //open/close hdf5 file
      if(CompEstimators) 
      {
        RootName=myComm->getName()+".stat.h5";
        hid_t fid = H5Fcreate(RootName.c_str(),H5F_ACC_TRUNC,H5P_DEFAULT,H5P_DEFAULT);
        HDFVersion cur_version;
        cur_version.write(fid,hdf::version);
        CompEstimators->open(fid);
        //H5Fclose(fid);
      }

#if defined(QMC_ASYNC_COLLECT)
      if(Options[COLLECT])
      {//issue a irecv
        pendingRequests=0;
        for(int i=1,is=0; i<myComm->size(); i++,is++) 
        {
          myRequest[is]=myComm->irecv(i,i,*RemoteData[i]);//request only has size-1
          pendingRequests++;
        }
      }
#endif
    }
  }

  void EstimatorManager::stop(const vector<EstimatorManager*> est)
  {

    int num_threads=est.size();
    //normalize by the number of threads per node
    RealType tnorm=1.0/static_cast<RealType>(num_threads);

    //add averages and divide them by the number of threads
    AverageCache=est[0]->AverageCache;
    for(int i=1; i<num_threads; i++) AverageCache+=est[i]->AverageCache;
    AverageCache*=tnorm;

    //add properties and divide them by the number of threads except for the weight
    PropertyCache=est[0]->PropertyCache;
    for(int i=1; i<num_threads; i++) PropertyCache+=est[i]->PropertyCache;
    for(int i=1; i<PropertyCache.size(); i++) PropertyCache[i] *= tnorm;

    stop();
  }

  /** Stop a run
   *
   * Collect data in Cache and print out data into hdf5 and ascii file.
   * This should not be called in a OpenMP parallel region or should
   * be guarded by master/single.
   * Keep the ascii output for now
   */
  void EstimatorManager::stop() 
  {
    //clean up pending messages
    if(pendingRequests) 
    {
      cancel(myRequest);
      pendingRequests=0;
    }

    //close any open files
    if(Archive) 
    {
      delete Archive; Archive=0;
    }

  //  //cleaup file
  //  if(h_file>-1)
  //  {
  //    H5Fclose(h_file); h_file=-1;
  //  }
  }

  void EstimatorManager::startBlock(int steps)
  { 
    MyTimer.restart();
    BlockWeight=0.0;
    if(CompEstimators) CompEstimators->startBlock(steps);
  }

  void EstimatorManager::stopBlock(RealType accept)
  {
    //take block averages and update properties per block
    PropertyCache[weightInd]=BlockWeight;
    PropertyCache[cpuInd] = MyTimer.elapsed();
    PropertyCache[acceptInd] = accept;


    for(int i=0; i<Estimators.size(); i++) 
    {
      Estimators[i]->takeBlockAverage(AverageCache.begin());
    }

    if(CompEstimators) CompEstimators->stopBlock();

    collectBlockAverages();
  }

  void EstimatorManager::stopBlock(const vector<EstimatorManager*> est)
  {
    //normalized it by the thread
    int num_threads=est.size();
    RealType tnorm=1.0/num_threads;

    //BlockWeight=est[0]->BlockWeight;
    //for(int i=1; i<num_threads; i++) BlockWeight += est[i]->BlockWeight;

    AverageCache=est[0]->AverageCache;
    for(int i=1; i<num_threads; i++) AverageCache +=est[i]->AverageCache;
    AverageCache *= tnorm;

    PropertyCache=est[0]->PropertyCache;
    for(int i=1; i<num_threads; i++) PropertyCache+=est[i]->PropertyCache;
    for(int i=1; i<PropertyCache.size(); i++) PropertyCache[i] *= tnorm;

    if(CompEstimators) 
    { //simply clean this up
      CompEstimators->startBlock(1);
      for(int i=0; i<num_threads; i++) 
        CompEstimators->collectBlock(est[i]->CompEstimators);
    }

    for(int i=0; i<num_threads; ++i)
      varAccumulator(est[i]->varAccumulator.mean());

    collectBlockAverages(num_threads);
  }

  void EstimatorManager::collectBlockAverages(int num_threads)
  {
#if defined(DEBUG_ESTIMATOR_ARCHIVE)
    if(DebugArchive)
    {
     if(CompEstimators) CompEstimators->print(*DebugArchive);
      *DebugArchive << setw(10) << RecordCount;
      for(int j=0; j<AverageCache.size(); j++) *DebugArchive << setw(16) << AverageCache[j];
      for(int j=0; j<PropertyCache.size(); j++) *DebugArchive << setw(16) << PropertyCache[j];
      *DebugArchive << endl;
    }
#endif

    if(Options[COLLECT])
    { //copy cached data to RemoteData[0]
      BufferType::iterator cur(RemoteData[0]->begin());
      std::copy(AverageCache.begin(),AverageCache.end(),cur);
      cur+=AverageCache.size();
      std::copy(PropertyCache.begin(),PropertyCache.end(),cur);
      cur+=PropertyCache.size();
      if(CompEstimators)
        CompEstimators->putMessage(cur);

#if defined(QMC_ASYNC_COLLECT)
      if(Options[MANAGE]) 
      { //wait all the message but we can choose to wait one-by-one with a timer
        wait_all(myRequest.size(),&myRequest[0]);
        for(int is=1; is<myComm->size(); is++) 
          accumulate_elements(RemoteData[is]->begin(),RemoteData[is]->end(), RemoteData[0]->begin());
      } 
      else //not a master, pack and send the data
        myRequest[0]=myComm->isend(0,myComm->rank(),*RemoteData[0]);
#else
      myComm->reduce(*RemoteData[0]);
#endif

      if(Options[MANAGE])
      {
        int n1=AverageCache.size();
        int n2=AverageCache.size()+PropertyCache.size();
        std::copy(RemoteData[0]->begin(),RemoteData[0]->begin()+n1, AverageCache.begin());
        std::copy(RemoteData[0]->begin()+n1,RemoteData[0]->begin()+n2, PropertyCache.begin());
        if(CompEstimators) CompEstimators->getMessage(RemoteData[0]->begin()+n2);

        RealType nth=1.0/static_cast<RealType>(myComm->size());
        AverageCache *= nth;
        //do not weight weightInd
        for(int i=1; i<PropertyCache.size(); i++) PropertyCache[i] *= nth;
      }
    }

    //add the block average to summarize
    energyAccumulator(AverageCache[0]);
    if(num_threads==1) varAccumulator(MainEstimator->variance());

    if(Archive)
    {
      *Archive << setw(10) << RecordCount;
      for(int j=0; j<AverageCache.size(); j++) *Archive << setw(16) << AverageCache[j];
      for(int j=0; j<PropertyCache.size(); j++) *Archive << setw(16) << PropertyCache[j];
      *Archive << endl;

      if(CompEstimators) CompEstimators->recordBlock();
    }

    RecordCount++;
  }

  //NOTE: weights are not handled nicely.
  //weight should be done carefully, not valid for DMC
  //will add a function to MCWalkerConfiguration to track total weight
  void EstimatorManager::accumulate(MCWalkerConfiguration& W)
  {
    BlockWeight += W.getActiveWalkers();
    RealType norm=1.0/W.getGlobalNumWalkers();
    for(int i=0; i< Estimators.size(); i++) 
      Estimators[i]->accumulate(W.begin(),W.end(),norm);
    if(CompEstimators) CompEstimators->accumulate(W,norm);
  }

  void EstimatorManager::accumulate(MCWalkerConfiguration& W, 
      MCWalkerConfiguration::iterator it,
      MCWalkerConfiguration::iterator it_end)
  {
    BlockWeight += it_end-it;
    RealType norm=1.0/W.getGlobalNumWalkers();
    for(int i=0; i< Estimators.size(); i++) 
      Estimators[i]->accumulate(it,it_end,norm);
    if(CompEstimators) CompEstimators->accumulate(W,it,it_end,norm);
  }

  void 
    EstimatorManager::getEnergyAndWeight(RealType& e, RealType& w, RealType& var) 
  {
    if(Options[COLLECT])//need to broadcast the value
    {
      RealType tmp[3];
      tmp[0]= energyAccumulator.result();
      tmp[1]= energyAccumulator.count();
      tmp[2]= varAccumulator.mean();

      myComm->bcast(tmp,3);
      e=tmp[0];
      w=tmp[1];
      var=tmp[2];
    }
    else
    {
      e= energyAccumulator.result();
      w= energyAccumulator.count();
      var= varAccumulator.mean();
    }
  }

  EstimatorManager::EstimatorType* 
    EstimatorManager::getMainEstimator() 
    {
      if(MainEstimator==0) 
        add(new LocalEnergyOnlyEstimator(),MainEstimatorName);
      return MainEstimator;
    }

  EstimatorManager::EstimatorType* 
    EstimatorManager::getEstimator(const string& a) 
    {
      std::map<string,int>::iterator it = EstimatorMap.find(a);
      if(it == EstimatorMap.end()) 
        return 0;
      else 
        return Estimators[(*it).second];
    }

  /** This should be moved to branch engine */
  bool EstimatorManager::put(MCWalkerConfiguration& W, QMCHamiltonian& H, xmlNodePtr cur) 
  {
    vector<string> extra;
    cur = cur->children;
    while(cur != NULL) 
    {
      string cname((const char*)(cur->name));
      if(cname == "estimator") 
      {
        xmlChar* att=xmlGetProp(cur,(const xmlChar*)"name");
        if(att) 
        {
          string aname((const char*)att);
          if(aname == "LocalEnergy") 
            add(new LocalEnergyEstimator(H),MainEstimatorName);
          else 
            extra.push_back(aname);
        }
      } 
      cur = cur->next;
    }

    if(Estimators.empty()) 
    {
      app_log() << "  Using a default LocalEnergyOnlyEstimator for the MainEstimator " << endl;
      add(new LocalEnergyOnlyEstimator(),MainEstimatorName);
    } 

    string SkName("sk");
    string GofRName("gofr");
    for(int i=0; i< extra.size(); i++)
    {
      if(extra[i] == SkName && W.Lattice.SuperCellEnum)
      {
        if(CompEstimators == 0) CompEstimators = new CompositeEstimatorSet;
        if(CompEstimators->missing(SkName))
        {
          app_log() << "  EstimatorManager::add " << SkName << endl;
          CompEstimators->add(new SkEstimator(W),SkName);
        }
      } else if(extra[i] == GofRName)
      {
        if(CompEstimators == 0) CompEstimators = new CompositeEstimatorSet;
        if(CompEstimators->missing(GofRName))
        {
          app_log() << "  EstimatorManager::add " << GofRName << endl;
          CompEstimators->add(new GofREstimator(W),GofRName);
        }
      }
    }
    //add extra
    return true;
  }

  int EstimatorManager::add(EstimatorType* newestimator, const string& aname) 
  { 

    std::map<string,int>::iterator it = EstimatorMap.find(aname);
    int n =  Estimators.size();
    if(it == EstimatorMap.end()) 
    {
      Estimators.push_back(newestimator);
      EstimatorMap[aname] = n;
    } 
    else 
    {
      n=(*it).second;
      app_log() << "  EstimatorManager::add replace " << aname << " estimator." << endl;
      delete Estimators[n]; 
      Estimators[n]=newestimator;
    }

    //check the name and set the MainEstimator
    if(aname == MainEstimatorName) MainEstimator=newestimator;
    return n;
  }

  int EstimatorManager::addObservable(const char* aname) 
  {
    int mine = BlockAverages.add(aname);
    int add = TotalAverages.add(aname);
    if(mine < Block2Total.size()) 
      Block2Total[mine] = add;
    else 
      Block2Total.push_back(add);
    return mine;
  }

  void EstimatorManager::getData(int i, vector<RealType>& values)
  {
    int entries = TotalAveragesData.rows();
    values.resize(entries);
    for (int a=0; a<entries; a++)
      values[a] = TotalAveragesData(a,Block2Total[i]);
  }

  //void EstimatorManager::updateRefEnergy()
  //{
  //  CumEnergy[0]+=1.0;
  //  RealType et=AverageCache(RecordCount-1,0);
  //  CumEnergy[1]+=et;
  //  CumEnergy[2]+=et*et;
  //  CumEnergy[3]+=std::sqrt(MainEstimator->d_variance);

  //  RealType wgtnorm=1.0/CumEnergy[0];
  //  RefEnergy[0]=CumEnergy[1]*wgtnorm;
  //  RefEnergy[1]=CumEnergy[2]*wgtnorm-RefEnergy[0]*RefEnergy[0];
  //  if(CumEnergy[0]>1) 
  //    RefEnergy[2]=std::sqrt(RefEnergy[1]*wgtnorm/(CumEnergy[0]-1.0));
  //  RefEnergy[3]=CumEnergy[3]*wgtnorm;//average block variance
  //}

}

/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
