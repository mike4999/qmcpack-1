//////////////////////////////////////////////////////////////////
// (c) Copyright 1998-2002,2003- by Jeongnim Kim
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
//   Department of Physics, Ohio State University
//   Ohio Supercomputer Center
//////////////////////////////////////////////////////////////////
// -*- C++ -*-

#ifndef OHMMS_COMMUNICATE_H
#define OHMMS_COMMUNICATE_H
#ifdef HAVE_CONFIG_H
#include "ohmms-config.h"
#endif

#ifdef HAVE_OOMPI
#include "oompi.h"
struct CommunicatorTraits {
  typedef MPI_Comm         mpi_comm_type;
  typedef MPI_Status       mpi_status_type;
  typedef MPI_Request      mpi_request_type;
  typedef OOMPI_Intra_comm intra_comm_type;
};
#else
struct CommunicatorTraits {
  typedef int  mpi_comm_type;
  typedef int  mpi_status_type;
  typedef int  mpi_request_type;
  typedef int  intra_comm_type;
};
#endif


/**@class Communicate
 * @ingroup Message
 * @brief 
 *  Wrapping information on parallelism.
 *  Very limited in functions. Currently, only single-mode or mpi-mode 
 *  is available (mutually exclusive).
 * @todo Possibly, make it a general manager class for mpi+openmp, mpi+mpi
 */
class Communicate: public CommunicatorTraits {

public:

  ///constructor
  Communicate();

  ///constructor with arguments
  Communicate(int argc, char **argv);

  ///constructor with communicator
  //Communicate(const intra_comm_type& c);
  Communicate(const Communicate& comm, int nparts);

  /**destructor
   * Call proper finalization of Communication library
   */
  virtual ~Communicate();

  void initialize(int argc, char **argv);
  void finalize();
  void abort();
  void abort(const char* msg);

  template<class T> void allreduce(T&);
  template<class T> void reduce(T* restrict, T* restrict, int n);
  template<class T> void bcast(T* restrict, int n);

  ///return the Communicator ID (typically MPI_WORLD_COMM)
  inline mpi_comm_type getMPI() const { return myMPI;}

  inline intra_comm_type& getComm() { return myComm;}
  inline const intra_comm_type& getComm() const { return myComm;}

  ///return the rank of this node
  inline int getNodeID() const { return d_mycontext;}
  inline int mycontext() const { return d_mycontext;}

  ///return the number of nodes
  inline int getNumNodes() const { return d_ncontexts;}
  inline int ncontexts() const { return d_ncontexts;}

  ///return the group id
  inline int getGroupID() const {return d_groupid;}

  inline bool master() const { return (d_mycontext == 0);}

  //intra_comm_type split(int n);
  void cleanupMessage(void*);
  inline void setNodeID(int i) { d_mycontext = i;}
  inline void setNumNodes(int n) { d_ncontexts = n;}
  void barrier();

protected:

  mpi_comm_type myMPI;
  intra_comm_type myComm;
  int d_mycontext; 
  int d_ncontexts;
  int d_groupid;

};


namespace OHMMS {
  /** Global Communicator for a process 
   */
  extern Communicate* Controller;
}
#endif // OHMMS_COMMUNICATE_H
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
