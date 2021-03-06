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
/**@file QMCMain.cpp
 * @brief Implments QMCMain operators.
 */
#include "QMCApp/QMCMain.h"
#include "QMCApp/ParticleSetPool.h"
#include "QMCApp/WaveFunctionPool.h"
#include "QMCApp/HamiltonianPool.h"
#include "QMCWaveFunctions/TrialWaveFunction.h"
#include "QMCHamiltonians/QMCHamiltonian.h"
#include "QMCHamiltonians/ConservedEnergy.h"
#include "QMCDrivers/VMC.h"
#include "QMCDrivers/VMCMultiple.h"
#include "QMCDrivers/VMCParticleByParticle.h"
#include "QMCDrivers/VMCPbyPMultiple.h"
#include "QMCDrivers/DMCParticleByParticle.h"
#include "QMCDrivers/VMC_OPT.h"
#include "QMCDrivers/MolecuDMC.h"
#include "QMCDrivers/ReptationMC.h"
#include "Utilities/OhmmsInfo.h"
#include "Particle/HDFWalkerIO.h"
#include "QMCApp/InitMolecularSystem.h"
#include "Message/Communicate.h"
#include <queue>
using namespace std;
#include "OhmmsData/AttributeSet.h"

namespace ohmmsqmc {

  QMCMain::QMCMain(int argc, char** argv): QMCAppBase(argc,argv),
                                           qmcDriver(0),qmcSystem(0){ 

    //create ParticleSetPool
    ptclPool = new ParticleSetPool;

    //create WaveFunctionPool
    psiPool = new WaveFunctionPool;
    psiPool->setParticleSetPool(ptclPool);

    //create HamiltonianPool
    hamPool = new HamiltonianPool;
    hamPool->setParticleSetPool(ptclPool);
    hamPool->setWaveFunctionPool(psiPool);
  }

  ///destructor
  QMCMain::~QMCMain() {
    DEBUGMSG("QMCMain::~QMCMain")
    delete hamPool;
    delete psiPool;
    delete ptclPool;
    //Not a good thing.
    //xmlFreeDoc(m_doc);
  }


  bool QMCMain::execute() {

    bool success = validateXML();

    if(!success) {
      ERRORMSG("Input document does not contain valid objects")
      return false;
    }

    curMethod = string("invalid");
    vector<xmlNodePtr> q;

    xmlNodePtr cur=m_root->children;
    while(cur != NULL) {

      string cname((const char*)cur->name);
      if(cname == "qmc") {
        string target("e");
        const xmlChar* t=xmlGetProp(cur,(const xmlChar*)"target");
        if(t) target = (const char*)t;
        qmcSystem = ptclPool->getWalkerSet(target);
        runQMC(cur);
        q.push_back(cur);
        myProject.advance();
      }

      cur=cur->next;
    }

    if(q.size()) {
      xmlNodePtr aqmc = xmlNewNode(NULL,(const xmlChar*)"qmc");
      xmlNewProp(aqmc,(const xmlChar*)"method",(const xmlChar*)"dmc");
      xmlNodePtr aparam = xmlNewNode(NULL,(const xmlChar*)"parameter");
      xmlNewProp(aparam,(const xmlChar*)"name",(const xmlChar*)"en_ref");
      char ref_energy[128];
      sprintf(ref_energy,"%15.5e",qmcSystem->getLocalEnergy());
      xmlNodeSetContent(aparam,(const xmlChar*)ref_energy);
      xmlAddChild(aqmc,aparam);
      xmlAddChild(m_root,aqmc);

      for(int i=0;i<q.size(); i++) {
        xmlUnlinkNode(q[i]);
        xmlFreeNode(q[i]);
      }
    }

    saveXml();
    return true;
  }

  /** validate m_doc
   * @return false, if any of the basic objects is not properly created.
   *
   * Current xml schema is changing. Instead validating the input file,
   * we use xpath to create necessary objects. The order follows
   * - project: determine the simulation title and sequence
   * - random: initialize random number generator
   * - particleset: create all the particleset
   * - wavefunction: create wavefunctions
   * - hamiltonian: create hamiltonians
   * Finally, if /simulation/mcwalkerset exists, read the configurations
   * from the external files.
   */
  bool QMCMain::validateXML() {

    xmlXPathContextPtr m_context = xmlXPathNewContext(m_doc);

    xmlXPathObjectPtr result
      = xmlXPathEvalExpression((const xmlChar*)"//project",m_context);
    if(xmlXPathNodeSetIsEmpty(result->nodesetval)) {
      WARNMSG("Project is not defined")
      myProject.reset();
    } else {
      myProject.put(result->nodesetval->nodeTab[0]);
    }
    xmlXPathFreeObject(result);

    //initialize the random number generator
    xmlNodePtr rptr = myRandomControl.initialize(m_context);
    if(rptr) {
      xmlAddChild(m_root,rptr);
    }

    //check particleset/wavefunction/hamiltonian of the current document
    //processContext(m_context);

    //preserve the input order
    xmlNodePtr cur=m_root->children;
    while(cur != NULL) {
      string cname((const char*)cur->name);
      if(cname == "particleset") {
        ptclPool->put(cur);
      } else if(cname == "wavefunction") {
        psiPool->put(cur);
      } else if(cname == "hamiltonian") {
        hamPool->put(cur);
      } else if(cname == "include") {//file is provided
        const xmlChar* a=xmlGetProp(cur,(const xmlChar*)"href");
        if(a) {
          //root and document are saved
          xmlNodePtr root_save=m_root;
          xmlDocPtr doc_save=m_doc;

          //parse a new file
          m_doc=NULL;
          parse((const char*)a);
          processPWH(xmlDocGetRootElement(m_doc));
          //xmlXPathContextPtr context_=xmlXPathNewContext(m_doc);
          //processContext(context_);
          //xmlXPathFreeContext(context_);
          xmlFreeDoc(m_doc);

          //copy the original root and document
          m_root = root_save;
          m_doc = doc_save;
        }
      } else if(cname == "qmcsystem") {
        processPWH(cur);
      } else if(cname == "init") {
        InitMolecularSystem moinit(ptclPool);
        moinit.put(cur);
      }
      cur=cur->next;
    }

    if(ptclPool->empty()) {
      ERRORMSG("Illegal input. Missing particleset ")
      return false;
    }

    if(psiPool->empty()) {
      ERRORMSG("Illegal input. Missing wavefunction. ")
      return false;
    }

    if(hamPool->empty()) {
      ERRORMSG("Illegal input. Missing hamiltonian. ")
      return false;
    }

    setMCWalkers(m_context);
    xmlXPathFreeContext(m_context);
    return true;
  }   

  /** grep basic objects and add to Pools
   * @param cur current node 
   *
   * Recursive search  all the xml elements with particleset, wavefunction and hamiltonian
   * tags
   */
  void QMCMain::processPWH(xmlNodePtr cur) {

    if(cur == NULL) return;

    cur=cur->children;
    while(cur != NULL) {
      string cname((const char*)cur->name);
      if(cname == "particleset") {
        ptclPool->put(cur);
      } else if(cname == "wavefunction") {
        psiPool->put(cur);
      } else if(cname == "hamiltonian") {
        hamPool->put(cur);
      }
      cur=cur->next;
    }
  }

  /** grep basic objects and add to Pools
   * @param context_ xmlXPathContextPtr 
   *
   * Use xpath to get all the xml elements with particleset, wavefunction and hamiltonian
   * tags. 
   */
  //void QMCMain::processContext(xmlXPathContextPtr context_) {

  //  xmlXPathObjectPtr result 
  //    = xmlXPathEvalExpression((const xmlChar*)"//particleset",context_);
  //  for(int i=0; i<result->nodesetval->nodeNr; i++) {
  //    ptclPool->put(result->nodesetval->nodeTab[i]);
  //  }
  //  xmlXPathFreeObject(result);

  //  result=xmlXPathEvalExpression((const xmlChar*)"//wavefunction",context_);
  //  for(int i=0; i<result->nodesetval->nodeNr; i++) {
  //    psiPool->put(result->nodesetval->nodeTab[i]);
  //  }
  //  xmlXPathFreeObject(result);

  //  result=xmlXPathEvalExpression((const xmlChar*)"//hamiltonian",context_);
  //  for(int i=0; i<result->nodesetval->nodeNr; i++) {
  //    hamPool->put(result->nodesetval->nodeTab[i]);
  //  }
  //  xmlXPathFreeObject(result);
  //}

  /** prepare for a QMC run
   * @param cur qmc element
   * @return true, if a valid QMCDriver is set.
   */
  bool QMCMain::runQMC(xmlNodePtr cur) {

    string what("invalid");
    xmlChar* att=xmlGetProp(cur,(const xmlChar*)"method");
    if(att) what = (const char*)att;
    //if(qmcDriver) {
    //  if(what == curMethod) {
    //    return  true;
    //  } else {
    //    delete qmcDriver;
    //    qmcDriver = 0;
    //  }
    //}
    qmcDriver=0;

    ///////////////////////////////////////////////
    // get primaryPsi and primaryH
    ///////////////////////////////////////////////
    TrialWaveFunction* primaryPsi= 0;
    QMCHamiltonian* primaryH=0;
    queue<TrialWaveFunction*> targetPsi;//FIFO 
    queue<QMCHamiltonian*> targetH;//FIFO

    xmlNodePtr tcur=cur->children;
    while(tcur != NULL) {
       if(xmlStrEqual(tcur->name,(const xmlChar*)"qmcsystem")) {
         const xmlChar* t= xmlGetProp(tcur,(const xmlChar*)"wavefunction");
         targetPsi.push(psiPool->getWaveFunction((const char*)t));
         t= xmlGetProp(tcur,(const xmlChar*)"hamiltonian");
         targetH.push(hamPool->getHamiltonian((const char*)t));
       }
       tcur=tcur->next;
    }

    if(targetH.empty()) {
      primaryPsi=psiPool->getPrimary();
      primaryH=hamPool->getPrimary();
    } else { //mark the first targetPsi and targetH as the primaries
      primaryPsi=targetPsi.front(); targetPsi.pop();
      primaryH=targetH.front();targetH.pop();
    }
    ///////////////////////////////////////////////

    if (what == "vmc"){
      primaryH->add(new ConservedEnergy,"Flux");
      qmcDriver = new VMC(*qmcSystem,*primaryPsi,*primaryH);
    } else if(what == "vmc-ptcl"){
      primaryH->add(new ConservedEnergy,"Flux");
      qmcDriver = new VMCParticleByParticle(*qmcSystem,*primaryPsi,*primaryH);
    } else if(what == "dmc"){
      MolecuDMC *dmc = new MolecuDMC(*qmcSystem,*primaryPsi,*primaryH);
      dmc->setBranchInfo(PrevConfigFile);
      qmcDriver=dmc;
    } else if(what == "dmc-ptcl"){
      DMCParticleByParticle *dmc = new DMCParticleByParticle(*qmcSystem,*primaryPsi,*primaryH);
      dmc->setBranchInfo(PrevConfigFile);
      qmcDriver=dmc;
    } else if(what == "optimize"){
      VMC_OPT *vmc = new VMC_OPT(*qmcSystem,*primaryPsi,*primaryH);
      vmc->addConfiguration(PrevConfigFile);
      qmcDriver=vmc;
    } else if(what == "rmc") {
      qmcDriver = new ReptationMC(*qmcSystem,*primaryPsi,*primaryH);
    } else if(what == "vmc-multi") {
      qmcDriver = new VMCMultiple(*qmcSystem,*primaryPsi,*primaryH);
      while(targetH.size()) {
        qmcDriver->add_H_and_Psi(targetH.front(),targetPsi.front());
        targetH.pop();
        targetPsi.pop(); 
      }
    } else if(what == "vmc-ptcl-multi") {
      qmcDriver = new VMCPbyPMultiple(*qmcSystem,*primaryPsi,*primaryH);
      while(targetH.size()) {
	qmcDriver->add_H_and_Psi(targetH.front(),targetPsi.front());
	targetH.pop();
	targetPsi.pop(); 
      }

    }

    if(qmcDriver) {

      LOGMSG("Starting a QMC simulation " << curMethod)
      qmcDriver->setFileRoot(myProject.CurrentRoot());
      qmcDriver->process(cur);
      qmcDriver->run();

      //keeps track of the configuration file
      PrevConfigFile = myProject.CurrentRoot();

      //change the content of mcwalkerset/@file attribute
      for(int i=0; i<m_walkerset.size(); i++) {
        xmlSetProp(m_walkerset[i],
            (const xmlChar*)"file", (const xmlChar*)myProject.CurrentRoot());
      }

      curMethod = what;
      
      //may want to reuse!
      delete qmcDriver;
      return true;
    } else {
      return false;
    }
  }

  bool QMCMain::setMCWalkers(xmlXPathContextPtr context_) {

    xmlXPathObjectPtr result
      = xmlXPathEvalExpression((const xmlChar*)"/simulation/mcwalkerset",context_);

    if(xmlXPathNodeSetIsEmpty(result->nodesetval)) {
      if(m_walkerset.empty()) {
	xmlNodePtr newnode_ptr = xmlNewNode(NULL,(const xmlChar*)"mcwalkerset");
	xmlNewProp(newnode_ptr,(const xmlChar*)"file", (const xmlChar*)"nothing");
	xmlAddChild(m_root,newnode_ptr);
	m_walkerset.push_back(newnode_ptr);
      }
    } else {
      int pid=OHMMS::Controller->mycontext(); 
      for(int iconf=0; iconf<result->nodesetval->nodeNr; iconf++) {
	xmlNodePtr mc_ptr = result->nodesetval->nodeTab[iconf];
	m_walkerset.push_back(mc_ptr);
        string cfile("invalid"), target("e"), anode("-1");
        OhmmsAttributeSet pAttrib;
        pAttrib.add(cfile,"href"); pAttrib.add(cfile,"file"); 
        pAttrib.add(target,"target"); pAttrib.add(target,"ref"); 
        pAttrib.add(anode,"node");
        pAttrib.put(mc_ptr);

        int pid_target = atoi(anode.c_str());
        if(pid_target<0) pid_target=pid; //node is not given. 

        if(pid_target == pid && cfile != "invalid") {
          MCWalkerConfiguration* el=ptclPool->getWalkerSet(target);
	  XMLReport("Using previous configuration of " << target << " from " << cfile)
          HDFWalkerInput WO(cfile); 
	  //read only the last ensemble of walkers
	  WO.put(*el,-1);
	  PrevConfigFile = cfile;
        }
      }
    }
    xmlXPathFreeObject(result);

    return true;
  }
}
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
