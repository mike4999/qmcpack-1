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
//
// Supported by 
//   National Center for Supercomputing Applications, UIUC
//   Materials Computation Center, UIUC
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
/** @file EstimatorManager.h
 * @brief Manager class of scalar estimators
 * @authors J. Kim and J. Vincent
 */
#ifndef QMCPLUSPLUS_ESTIMATORMANAGER_H
#define QMCPLUSPLUS_ESTIMATORMANAGER_H

#include "Configuration.h"
#include "Utilities/Timer.h"
#include "Utilities/PooledData.h"
#include "Message/Communicate.h"
#include "Estimators/ScalarEstimatorBase.h"
#include "OhmmsPETE/OhmmsVector.h"
#include "OhmmsData/HDFAttribIO.h"
#include <bitset>

namespace qmcplusplus {

  class MCWalkerConifugration;
  class QMCHamiltonian;

  /**Class to manage a set of ScalarEstimators */
  class EstimatorManager: public QMCTraits
  {

  public:

    typedef ScalarEstimatorBase  EstimatorType;
    typedef vector<RealType>     BufferType;
    //enum { WEIGHT_INDEX=0, BLOCK_CPU_INDEX, ACCEPT_RATIO_INDEX, TOTAL_INDEX};

    ///name of the primary estimator name
    std::string MainEstimatorName;
    ///the root file name
    std::string RootName;
    ///energy
    TinyVector<RealType,4> RefEnergy;
   // //Cummulative energy, weight and variance
   // TinyVector<RealType,4>  EPSum;
    ///default constructor
    EstimatorManager(Communicate* c=0);
    ///copy constructor
    EstimatorManager(EstimatorManager& em);
    ///destructor
    virtual ~EstimatorManager();

    /** set the communicator */
    void setCommunicator(Communicate* c);

    /** return the communicator 
     */
    Communicate* getCommunicator()  
    { 
      return myComm;
    }

    /** return true if the rank == 0
     */
    inline bool is_manager() const {
      return !myComm->rank();
    }

    ///return the number of ScalarEstimators
    inline int size() const { return Estimators.size();}

    /** add a property with a name
     * @param aname name of the column
     * @return the property index so that its value can be set by setProperty(i)
     *
     * Append a named column. BlockProperties do not contain any meaning data
     * but manages the name to index map for PropertyCache.
     */
    inline int addProperty(const char* aname) {
      return BlockProperties.add(aname);
    }

    /** set the value of the i-th column with a value v
     * @param i column index
     * @param v value 
     */
    inline void setProperty(int i, RealType v) {
      PropertyCache[i]=v;
    }

    inline RealType getProperty(int i) const {
      return PropertyCache[i];
    }

    int addObservable(const char* aname);

    inline RealType getObservable(int i) const {
      return  TotalAverages[i];
    }

    void getData(int i, vector<RealType>& values);

    /** add an Estimator 
     * @param newestimator New Estimator
     * @param aname name of the estimator
     * @return locator of newestimator
     */
    int add(EstimatorType* newestimator, const string& aname);
    //int add(CompositeEstimatorBase* newestimator, const string& aname);

    /** add a main estimator
     * @param newestimator New Estimator
     * @return locator of newestimator
     */
    int add(EstimatorType* newestimator)
    {
      return add(newestimator,MainEstimatorName);
    }

    ///return a pointer to the estimator aname
    EstimatorType* getEstimator(const string& a);

    ///return a pointer to the estimator 
    EstimatorType* getMainEstimator();
 
    ///return the average for estimator i
    inline RealType average(int i) const { 
      return Estimators[i]->average(); 
    }

    ///returns a variance for estimator i
    inline RealType variance(int i) const { 
      return Estimators[i]->variance();
    }

    void setCollectionMode(bool collect);
    //void setAccumulateMode (bool setAccum) {AccumulateBlocks = setAccum;};

    ///process xml tag associated with estimators
    //bool put(xmlNodePtr cur);
    bool put(MCWalkerConfiguration& W, QMCHamiltonian& H, xmlNodePtr cur);

    void resetTargetParticleSet(ParticleSet& p);

    /** reset the estimator
     */
    void reset();

    /** start a run
     * @param blocks number of blocks
     * @param record if true, will write to a file
     *
     * Replace reportHeader and reset functon.
     */
    void start(int blocks, bool record=true);
    /** stop a qmc run
     *
     * Replace finalize();
     */
    void stop();
    /** stop a qmc run
     */
    void stop(const vector<EstimatorManager*> m);

    /** start  a block
     * @param steps number of steps in a block
     */
    void startBlock(int steps);
    
    void setNumberOfBlocks(int blocks)
     {
     for(int i=0; i<Estimators.size(); i++) 
        Estimators[i]->setNumberOfBlocks(blocks);
     }
     
    /** stop a block
     * @param accept acceptance rate of this block
     */
    void stopBlock(RealType accept, bool collectall=true);
    /** stop a block
     * @param m list of estimator which has been collecting data independently
     */
    void stopBlock(const vector<EstimatorManager*> m);

    void accumulate(MCWalkerConfiguration& W, MCWalkerConfiguration::iterator it,
        MCWalkerConfiguration::iterator it_end);

    /** accumulate the measurements
     */
    void accumulate(MCWalkerConfiguration& W);
    void accumulate(vector<vector<vector<RealType> > > values, vector<vector<int> > weights, int nwalkers);
    ///** set the cummulative energy and weight
    // */
    void getEnergyAndWeight(RealType& e, RealType& w, RealType& var);

    void getCurrentStatistics(MCWalkerConfiguration& W, RealType& eavg, RealType& var);

    template<class CT>
    void write(CT& anything, bool doappend) 
    {
      anything.write(h_file,doappend);
    }

  protected:
    ///use bitset to handle options
    bitset<8> Options;
    ///size of the message buffer
    int BufferSize;
    ///number of records in a block
    int RecordCount;
    ///index for the block weight PropertyCache(weightInd)
    int weightInd;
    ///index for the block cpu PropertyCache(cpuInd) 
    int cpuInd;
    ///index for the acceptance rate PropertyCache(acceptInd) 
    int acceptInd;
    ///hdf5 handler
    hid_t h_file;
    ///total weight accumulated in a block
    RealType BlockWeight;
    ///file handler to write data
    ofstream* Archive;
    ///file handler to write data for debugging
    ofstream* DebugArchive;
    ///communicator to handle communication
    Communicate* myComm;
    /** pointer to the primary ScalarEstimatorBase
     *
     * To be removed 
     */
    ScalarEstimatorBase* MainEstimator;
    /** accumulator for the energy
     *
     * @todo expand it for all the scalar observables to report the final results
     */
    ScalarEstimatorBase::accumulator_type energyAccumulator;
    /** accumulator for the variance **/
    ScalarEstimatorBase::accumulator_type varAccumulator;
    ///cached block averages of the values
    Vector<RealType> AverageCache;
    ///cached block averages of the squared values 
    Vector<RealType> SquaredAverageCache;
    ///cached block averages of properties, e.g. BlockCPU
    Vector<RealType> PropertyCache;
    ///manager of scalar data
    RecordNamedProperty<RealType> BlockAverages;
    ///manager of property data
    RecordNamedProperty<RealType> BlockProperties;
    ///block averages: name to value
    RecordNamedProperty<RealType> TotalAverages;
    ///data accumulated over the blocks
    Matrix<RealType> TotalAveragesData;
    ///index mapping between BlockAverages and TotalAverages
    vector<int> Block2Total;
    ///column map
    std::map<string,int> EstimatorMap;
    ///estimators of simple scalars
    vector<EstimatorType*> Estimators;
    ///convenient descriptors for hdf5
    vector<observable_helper*> h5desc;
    /////estimators of composite data
    //CompositeEstimatorSet* CompEstimators;
    ///Timer
    Timer MyTimer;
private:
    ///number of maximum data for a scalar.dat
    int max4ascii;
    ///number of requests
    int pendingRequests;
    //Data for communication
    vector<BufferType*> RemoteData;
     //storage for MPI_Request
    vector<Communicate::request> myRequest;
    ///collect data and write
    void collectBlockAverages(int num_threads=1);
    ///add header to an ostream
    void addHeader(ostream& o);
    unsigned long FieldWidth;
  };
}
#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
