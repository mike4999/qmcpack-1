//////////////////////////////////////////////////////////////////
// (c) Copyright 2005- by Jeongnim Kim
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
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
#include "QMCDrivers/QMCLinearOptimize.h"
#include "Particle/HDFWalkerIO.h"
#include "Particle/DistanceTable.h"
#include "OhmmsData/AttributeSet.h"
#include "Message/CommOperators.h"
#if defined(ENABLE_OPENMP)
#include "QMCDrivers/VMC/VMCSingleOMP.h"
#include "QMCDrivers/QMCCostFunctionOMP.h"
#endif
#include "QMCDrivers/VMC/VMCSingle.h"
#include "QMCDrivers/QMCCostFunctionSingle.h"
#include "QMCApp/HamiltonianPool.h"
#include "Numerics/Blasf.h"
#include <cassert>
namespace qmcplusplus {

QMCLinearOptimize::QMCLinearOptimize(MCWalkerConfiguration& w,
                                     TrialWaveFunction& psi, QMCHamiltonian& h, HamiltonianPool& hpool): QMCDriver(w,psi,h),
        PartID(0), NumParts(1), WarmupBlocks(10),
        SkipSampleGeneration("no"), hamPool(hpool),
        optTarget(0), vmcEngine(0),Max_iterations(10),
        wfNode(NULL), optNode(NULL), exp0(-9), tries(6),alpha(1e-4),
        costgradtol(1e-4), xi(0.5), lm("no"), rp("no"), ug("no"), rescaleparams(false),linemin(false),usegrad(false)
{
    //set the optimization flag
    QMCDriverMode.set(QMC_OPTIMIZE,1);
    //read to use vmc output (just in case)
    RootName = "pot";
    QMCType ="QMCLinearOptimize"; 
    optmethod = "Linear";
    m_param.add(WarmupBlocks,"warmupBlocks","int");
    m_param.add(SkipSampleGeneration,"skipVMC","string");
    m_param.add(Max_iterations,"max_its","int" );
    m_param.add(tries,"tries","int" );
    m_param.add(exp0,"exp0","int" );
    m_param.add(alpha,"alpha","real" );
    m_param.add(costgradtol,"gradtol","real" );
    m_param.add(xi,"xi","real" ); 
    m_param.add(lm,"linemin","string" ); 
    m_param.add(rp,"rescale","string" ); 
    m_param.add(ug,"usegrad","string" ); 
    
}

/** Clean up the vector */
QMCLinearOptimize::~QMCLinearOptimize()
{
    delete vmcEngine;
    delete optTarget;
}


QMCLinearOptimize::RealType QMCLinearOptimize::Func(RealType dl)
{ 
   for (int i=0;i<optparm.size();i++) optTarget->Params(i) = optparm[i] + dl*optdir[i];
   return optTarget->Cost();
// return 0;
}

/** Add configuration files for the optimization
 * @param a root of a hdf5 configuration file
 */
void QMCLinearOptimize::addConfiguration(const string& a) {
    if (a.size()) ConfigFile.push_back(a);
}

bool QMCLinearOptimize::run()
{
    optTarget->initCommunicator(myComm);
    if ((rp=="yes") || (rp=="true")) rescaleparams=true;
    if ((lm=="yes") || (lm=="true")) linemin=true;
    if ((ug=="yes") || (ug=="true")) usegrad=true;

    //close files automatically generated by QMCDriver
    //branchEngine->finalize();


        //generate samples
        generateSamples();

        app_log() << "<opt stage=\"setup\">" << endl;
        app_log() << "  <log>"<<endl;

        //reset the rootname
        optTarget->setRootName(RootName);
        optTarget->setWaveFunctionNode(wfNode);

        app_log() << "   Reading configurations from h5FileRoot " << endl;
        //get configuration from the previous run
        Timer t1;

        optTarget->getConfigurations(h5FileRoot);
        optTarget->checkConfigurations();

        app_log() << "  Execution time = " << t1.elapsed() << endl;
        app_log() << "  </log>"<<endl;
        app_log() << "</opt>" << endl;

        app_log() << "<opt stage=\"main\" walkers=\""<< optTarget->getNumSamples() << "\">" << endl;
        app_log() << "  <log>" << endl;

        optTarget->setTargetEnergy(branchEngine->getEref());

        t1.restart();

        ///Here is our optimization routine
    bool Valid(true);
    int Total_iterations(0);
    RealType LastCost(optTarget->Cost());
    RealType deltaCost(1e6);
    while ( (deltaCost>costgradtol)&&(Max_iterations>Total_iterations) )
    {
        Total_iterations+=1;
        app_log()<<"Iteration: "<<Total_iterations<<"/"<<Max_iterations<<endl;
        int N=optTarget->NumParams() + 1;
        Matrix<RealType> Ham(N,N);
        Matrix<RealType> S(N,N);

        optTarget->fillOverlapHamiltonianMatrix(Ham, S);
//         app_log()<<"Hamiltonian"<<endl;
//         for (int i=0;i<N;i++)
//         {
//             for (int j=0;j<N;j++)
//             {
//                 app_log()<<Ham(i,j)<<" ";
//             }
//             app_log()<<endl;
//         }
//         app_log()<<endl;
//         app_log()<<"Overlap"<<endl;
//         for (int i=0;i<N;i++)
//         {
//             for (int j=0;j<N;j++)
//             {
//                 app_log()<<S(i,j)<<" ";
//             }
//             app_log()<<endl;
//         }
//         app_log()<<endl;

//int tries(6);
        vector<RealType> dP(N,0.0);
        vector<vector<RealType> > keepdP;
        vector<RealType> keepP(N-1,0);
        for (int i=0;i<(N-1); i++) keepP[i] = optTarget->Params(i);
        vector<RealType> Costs(tries); 
        vector<RealType> Xs(tries);
        for (int it=0;it<tries;it++)
        {
            Matrix<RealType> HamT(N,N), ST(N,N);
            for (int i=0;i<N;i++)
                for (int j=0;j<N;j++)
                {
                    HamT(i,j)= (Ham)(j,i);
                    ST(i,j)= (S)(j,i);
                }
            Xs[it]= std::pow(10.0,exp0+it);
            if ( !rescaleparams && linemin && !usegrad)
            {
              RealType smlst(0.0);
              for (int i=1;i<N;i++) smlst = std::min(HamT(i,i),smlst);
              Xs[it] -= smlst;
              app_log()<<" Negative diagonal :"<<smlst<<endl;
            }
            for (int i=1;i<N;i++) HamT(i,i) += Xs[it];

            char jl('N');
            char jr('V');
            vector<RealType> alphar(N),alphai(N),beta(N);
            Matrix<RealType> eigenT(N,N);
            int info;
            int lwork(-1);
            vector<RealType> work(1);

            RealType tt(0);
            int t(1); 
            dggev(&jl, &jr, &N, HamT.data(), &N, ST.data(), &N, &alphar[0], &alphai[0], &beta[0],&tt,&t, eigenT.data(), &N, &work[0], &lwork, &info);
            //       app_log()<<" Optimal size of work"<<work[0]<<endl;
            lwork=work[0];
            work.resize(lwork);



            ///RealType==double to use this one, ned to write case where RealType==float
            dggev(&jl, &jr, &N, HamT.data(), &N, ST.data(), &N, &alphar[0], &alphai[0], &beta[0],&tt,&t, eigenT.data(), &N, &work[0], &lwork, &info);
            assert(info==0);
//             Matrix<double> eigen(N,N);
//             for (int i=0;i<N;i++)
//                 for (int j=0;j<N;j++)
//                     eigen(i,j)=eigenT(j,i);
            int MinE(0);
            for (int i=1;i<N;i++) if (alphar[i]/beta[i] < alphar[MinE]/beta[MinE]) MinE=i;
            RealType E_lin  = alphar[MinE]/beta[MinE];
            vector<RealType> dP(N,0);

            for (int i=0;i<N;i++) dP[i] = eigenT(MinE,i)/eigenT(MinE,0);

            if (rescaleparams)
            {
              RealType D(1.0);
              for (int i=0;i<N-1;i++)
              {
                  D += 2.0*S(0,i+1)*dP[i+1];
                  for (int j=0;j<N-1;j++) D += S(j+1,i+1)*dP[i+1]*dP[j+1];
              }
              D = std::sqrt(std::abs(D));

              vector<RealType> N_i(N-1,0);
              vector<RealType> M_i(N-1,0);
              for (int i=0;i<N-1;i++)
              {
                  N_i[i] = xi*D*S(0,i+1);
                  M_i[i] = xi*D;
                  RealType tsumN(S(0,i+1));
                  RealType tsumM(1);
                  for (int j=0;j<N-1;j++)
                  {
                      tsumN += S(i+1,j+1)*dP[j+1];
                      tsumM += S(0,j+1)*dP[j+1];
                  }
                  N_i[i] += (1-xi)*tsumN;
                  M_i[i] += (1-xi)*tsumM;
                  N_i[i] *= -1.0/M_i[i];
              }

              RealType rescale(1);
              for (int j=0;j<N-1;j++) rescale -= N_i[j]*dP[j+1];
              rescale = 1.0/rescale; 
              if ((rescale==rescale)&&(rescale!=0)) for (int i=0;i<(N-1); i++) dP[i+1]*=rescale;
            }
            
            for (int i=0;i<(N-1); i++) optTarget->Params(i) = keepP[i] + dP[i+1];
//         optTarget->GradCost(PGradient,parms);
            keepdP.push_back(dP);
//             app_log()<<"EigenValues"<<endl;
//             for (int i=0;i<N;i++) app_log()<<alphar[i]/beta[i]<<"  ";
//             app_log()<<endl;
//             app_log()<<"EigenVectors"<<endl;
//             for (int i=0;i<N;i++)
//             {
//                 for (int j=0;j<N;j++)
//                 {
//                     app_log()<<eigen(i,j)<<" ";
//                 }
//                 app_log()<<endl;
//             }
//             app_log()<<endl;
            ///Umrigar does a correlated Sampling run. Here I will just evaluate the cost function in these new places.
            ///We can fit this to a function and minimize it or just use the best value we find.
            Costs[it] = optTarget->Cost();
        }
        app_log()<<"Costs: ";
        for (int t=0;t<(tries);t++) app_log()<<Costs[t]-LastCost<<" ";
        app_log()<<endl;
        int minCostindex(0);
        //make sure the cost function is a number here.
        for (int t=0;t<tries;t++) if (Costs[t]==Costs[t]) minCostindex=t;
        for (int t=0;t<tries;t++) if (Costs[t]<Costs[minCostindex]) minCostindex=t;
        if ((LastCost-Costs[minCostindex]>costgradtol) && (rescaleparams))
        {
          for (int i=0;i<(N-1); i++) optTarget->Params(i) = keepP[i] + keepdP[minCostindex][i+1];
        }
        else if (linemin)
        {
          app_log()<<" Taking steepest descent step"<<endl;
          optdir.resize(N-1,0);
          optparm=keepP;
          if (usegrad) 
          {
            //steepest descent direction
            optTarget->GradCost(optdir, optparm,0);
//             for (int i=0;i<(N-1); i++) optdir[i] *= -1;
          }
          else for (int i=0;i<(N-1); i++) optdir[i] = keepdP[minCostindex][i+1];

          lineoptimization2();
          RealType newCost(LastCost);
          if (Lambda==Lambda)
          {
            for (int i=0;i<(N-1); i++) optTarget->Params(i) = optparm[i] + Lambda * optdir[i];
            newCost = optTarget->Cost();
            if ((LastCost-newCost<costgradtol) || (!optTarget->IsValid))
            {
              for (int i=0;i<(N-1); i++) optTarget->Params(i) = optparm[i]; 
              optTarget->Cost();
            }
            else  Costs[minCostindex]=newCost; 
          }
          else
          {
            for (int i=0;i<(N-1); i++) optTarget->Params(i) = keepP[i]; 
            optTarget->Cost();
          }
        }
        else for (int i=0;i<(N-1); i++) optTarget->Params(i) = keepP[i];
        MyCounter++;
        deltaCost = LastCost - Costs[minCostindex];
        app_log() << "  deltaCost = " <<deltaCost<<endl;
        LastCost= Costs[minCostindex];
        optTarget->Report();
    }
    app_log() << "  Execution time = " << t1.elapsed() << endl;
    app_log() << "  </log>" << endl;
    optTarget->reportParameters();
    app_log() << "</opt>" << endl;
    app_log() << "</optimization-report>" << endl;
    return (optTarget->getReportCounter() > 0);
}

void QMCLinearOptimize::generateSamples()
{
    Timer t1;
    app_log() << "<optimization-report>" << endl;
    //if(WarmupBlocks)
    //{
    //  app_log() << "<vmc stage=\"warm-up\" blocks=\"" << WarmupBlocks << "\">" << endl;
    //  //turn off QMC_OPTIMIZE
    //  vmcEngine->setValue("blocks",WarmupBlocks);
    //  vmcEngine->QMCDriverMode.set(QMC_WARMUP,1);
    //  vmcEngine->run();
    //  vmcEngine->setValue("blocks",nBlocks);
    //  app_log() << "  Execution time = " << t1.elapsed() << endl;
    //  app_log() << "</vmc>" << endl;
    //}

    if (W.getActiveWalkers()>NumOfVMCWalkers)
    {
        W.destroyWalkers(W.getActiveWalkers()-NumOfVMCWalkers);
        app_log() << "  QMCLinearOptimize::generateSamples removed walkers." << endl;
        app_log() << "  Number of Walkers per node " << W.getActiveWalkers() << endl;
    }

    vmcEngine->QMCDriverMode.set(QMC_OPTIMIZE,1);
    vmcEngine->QMCDriverMode.set(QMC_WARMUP,0);

    //vmcEngine->setValue("recordWalkers",1);//set record
    vmcEngine->setValue("current",0);//reset CurrentStep
    app_log() << "<vmc stage=\"main\" blocks=\"" << nBlocks << "\">" << endl;
    t1.restart();
//     W.reset();
//     branchEngine->flush(0);
//     branchEngine->reset();
    vmcEngine->run();
    app_log() << "  Execution time = " << t1.elapsed() << endl;
    app_log() << "</vmc>" << endl;

    //branchEngine->Eref=vmcEngine->getBranchEngine()->Eref;
    branchEngine->setTrialEnergy(vmcEngine->getBranchEngine()->getEref());
    //set the h5File to the current RootName
    h5FileRoot=RootName;
}

/** Parses the xml input file for parameter definitions for the wavefunction optimization.
 * @param q current xmlNode
 * @return true if successful
 */
bool
QMCLinearOptimize::put(xmlNodePtr q) {

    string vmcMove("pbyp");
    OhmmsAttributeSet oAttrib;
    oAttrib.add(vmcMove,"move");
    oAttrib.put(q);

    xmlNodePtr qsave=q;
    xmlNodePtr cur=qsave->children;

    
    int pid=OHMMS::Controller->rank();
    while (cur != NULL) {
        string cname((const char*)(cur->name));
        if (cname == "mcwalkerset") {
            mcwalkerNodePtr.push_back(cur);
        } else if (cname == "optimizer") {
            xmlChar* att= xmlGetProp(cur,(const xmlChar*)"method");
            if (att) {
                optmethod = (const char*)att;
            }
            optNode=cur;
        } else if (cname == "optimize") {
            xmlChar* att= xmlGetProp(cur,(const xmlChar*)"method");
            if (att) {
                optmethod = (const char*)att;
            } 
        }
        cur=cur->next;
    }
    //no walkers exist, add 10
    if (W.getActiveWalkers() == 0) addWalkers(omp_get_max_threads());

    NumOfVMCWalkers=W.getActiveWalkers();

    //create VMC engine
    if (vmcEngine ==0)
    {
#if defined(ENABLE_OPENMP)
        if (omp_get_max_threads()>1)
//             vmcEngine = new DMCOMP(W,Psi,H,hamPool);
            vmcEngine = new VMCSingleOMP(W,Psi,H,hamPool);
        else   
#endif
        vmcEngine = new VMCSingle(W,Psi,H); 
        vmcEngine->setUpdateMode(vmcMove[0] == 'p');
        vmcEngine->initCommunicator(myComm);
    }

    vmcEngine->setStatus(RootName,h5FileRoot,AppendRun);
    vmcEngine->process(qsave);

    bool success=true;
    if (optTarget == 0)
    {
#if defined(ENABLE_OPENMP)
        if (omp_get_max_threads()>1)
        {
            optTarget = new QMCCostFunctionOMP(W,Psi,H,hamPool);
        }
        else
#endif
        optTarget = new QMCCostFunctionSingle(W,Psi,H);
        optTarget->setStream(&app_log());
        success=optTarget->put(q);
    }
    return success;
}
}
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 1286 $   $Date: 2006-08-17 12:33:18 -0500 (Thu, 17 Aug 2006) $
 * $Id: QMCLinearOptimize.cpp 1286 2006-08-17 17:33:18Z jnkim $
 ***************************************************************************/
