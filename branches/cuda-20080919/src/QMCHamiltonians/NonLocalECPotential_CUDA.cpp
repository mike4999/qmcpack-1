//////////////////////////////////////////////////////////////////
// (c) Copyright 2010-  by Ken Esler and Jeongnim Kim
//////////////////////////////////////////////////////////////////
//   National Center for Supercomputing Applications &
//   Materials Computation Center
//   University of Illinois, Urbana-Champaign
//   Urbana, IL 61801
//   e-mail: esler@uiuc.edu
//
// Supported by 
//   National Center for Supercomputing Applications, UIUC
//   Materials Computation Center, UIUC
//////////////////////////////////////////////////////////////////

#include "QMCHamiltonians/NonLocalECPotential_CUDA.h"
#include "QMCHamiltonians/NLPP.h"

namespace qmcplusplus {

  NonLocalECPotential_CUDA::NonLocalECPotential_CUDA
  (ParticleSet& ions, ParticleSet& els, TrialWaveFunction& psi) : 
    NonLocalECPotential(ions, els, psi),
    CurrentNumWalkers(0),
    Ions_GPU("NonLocalECPotential_CUDA::Ions_GPU"), 
    L("NonLocalECPotential_CUDA::L"), 
    Linv("NonLocalECPotential_CUDA::Linv"), 
    Elecs_GPU("NonLocalECPotential_CUDA::Elecs_GPU"), 
    Dist_GPU("NonLocalECPotential_CUDA::Dist_GPU"),
    Eleclist_GPU("NonLocalECPotential_CUDA::Eleclist_GPU"), 
    Distlist_GPU("NonLocalECPotential_CUDA::Distlist_GPU"), 
    NumPairs_GPU("NonLocalECPotential_CUDA::NumPairs_GPU"),
    RatioPos_GPU("NonLocalECPotential_CUDA::RatioPos_GPU"), 
    CosTheta_GPU("NonLocalECPotential_CUDA::CosTheta_GPU"),
    RatioPoslist_GPU("NonLocalECPotential_CUDA::RatioPoslist_GPU")
  {
    setupCUDA(els);
  }


  QMCHamiltonianBase* 
  NonLocalECPotential_CUDA::makeClone
  (ParticleSet& qp, TrialWaveFunction& psi)
  {
    NonLocalECPotential_CUDA* myclone = 
      new NonLocalECPotential_CUDA(IonConfig,qp,psi);

    for(int ig=0; ig<PPset.size(); ++ig)
    {
      if(PPset[ig]) myclone->add(ig,PPset[ig]->makeClone());
    }

    //resize sphere
    qp.resizeSphere(IonConfig.getTotalNum());
    for(int ic=0; ic<IonConfig.getTotalNum(); ic++) {
      if(PP[ic] && PP[ic]->nknot) qp.Sphere[ic]->resize(PP[ic]->nknot);
    }
    return myclone;
  }


  void 
  NonLocalECPotential_CUDA::setupCUDA(ParticleSet &elecs)
  {
    SpeciesSet &sSet = IonConfig.getSpeciesSet();
    NumIonGroups = sSet.getTotalNum();
    gpu::host_vector<CUDA_PRECISION> LHost(OHMMS_DIM*OHMMS_DIM), 
      LinvHost(OHMMS_DIM*OHMMS_DIM);
    for (int i=0; i<OHMMS_DIM; i++)
      for (int j=0; j<OHMMS_DIM; j++) {
	LHost[OHMMS_DIM*i+j]    = (CUDA_PRECISION)elecs.Lattice.a(j)[i];
	LinvHost[OHMMS_DIM*i+j] = (CUDA_PRECISION)elecs.Lattice.b(i)[j];
      }
    L = LHost;
    Linv = LinvHost;
    NumElecs = elecs.getTotalNum();
    
    // Copy ion positions to GPU, sorting by GroupID
    gpu::host_vector<CUDA_PRECISION> Ion_host(OHMMS_DIM*IonConfig.getTotalNum());
    int index=0;
    for (int group=0; group<NumIonGroups; group++) {
      IonFirst.push_back(index);
      for (int i=0; i<IonConfig.getTotalNum(); i++) {
	if (IonConfig.GroupID[i] == group) {
	  for (int dim=0; dim<OHMMS_DIM; dim++) 
	    Ion_host[OHMMS_DIM*index+dim] = IonConfig.R[i][dim];
	  SortedIons.push_back(IonConfig.R[i]);
	  index++;
	}
      }
      IonLast.push_back(index-1);
    }
    Ions_GPU = Ion_host;

  }

  void NonLocalECPotential_CUDA::resizeCUDA(int nw)
  {
    MaxPairs = 2 * NumElecs;
        
    // Note: this will not cover pathological systems in which all
    // the cores overlap
    Elecs_GPU.resize(MaxPairs*nw);
    Dist_GPU.resize(MaxPairs*nw);
    gpu::host_vector<int*> Eleclist_host(nw);
    gpu::host_vector<CUDA_PRECISION*> Distlist_host(nw);
    Eleclist_GPU.resize(nw);
    Distlist_GPU.resize(nw);
    NumPairs_GPU.resize(nw);
    for (int iw=0; iw<nw; iw++) {
      Eleclist_host[iw] = &(Elecs_GPU.data()[MaxPairs*iw]);
      Distlist_host[iw] = &(Dist_GPU.data()[MaxPairs*iw]);
    }
    Eleclist_GPU = Eleclist_host;
    Distlist_GPU = Distlist_host;
    
    // Resize ratio positions vector

    // Compute maximum number of knots
    MaxKnots = 0;
    for (int i=0; i<PPset.size(); i++)
      if (PPset[i])
	MaxKnots = max(MaxKnots,PPset[i]->nknot);

    RatiosPerWalker = MaxPairs * MaxKnots;
    RatioPos_GPU.resize(OHMMS_DIM * RatiosPerWalker * nw);
    CosTheta_GPU.resize(RatiosPerWalker * nw);
    gpu::host_vector<CUDA_PRECISION*> RatioPoslist_host(nw);
    gpu::host_vector<CUDA_PRECISION*> Ratiolist_host(nw);
    gpu::host_vector<CUDA_PRECISION*> CosThetalist_host(nw);
    for (int iw=0; iw<nw; iw++) {
      RatioPoslist_host[iw] = 
	RatioPos_GPU.data() + OHMMS_DIM * RatiosPerWalker * iw;
      CosThetalist_host[iw] = CosTheta_GPU.data()+RatiosPerWalker*iw;
    }
    RatioPoslist_GPU = RatioPoslist_host;
    CosThetalist_GPU = CosThetalist_host;

    QuadPoints_GPU.resize(NumIonGroups);
    QuadPoints_host.resize(NumIonGroups);
    for (int i=0; i<NumIonGroups; i++) 
      if (PPset[i]) {
	QuadPoints_GPU[i].set_name("NonLocalECPotential_CUDA::QuadPoints_GPU");
	QuadPoints_GPU[i].resize(OHMMS_DIM*PPset[i]->nknot);
	QuadPoints_host[i].resize(OHMMS_DIM*PPset[i]->nknot);
      }
    CurrentNumWalkers = nw;
  }

  void NonLocalECPotential_CUDA::addEnergy(MCWalkerConfiguration &W, 
					   vector<RealType> &LocalEnergy)
  {
    vector<Walker_t*> &walkers = W.WalkerList;
    int nw = walkers.size();
    if (CurrentNumWalkers < nw)
      resizeCUDA(nw);

    // Loop over the ionic species
    vector<RealType> esum(walkers.size(), 0.0);
    for (int sp=0; sp<NumIonGroups; sp++) 
      if (PPset[sp]) {
	NonLocalECPComponent &pp = *PPset[sp];
	PPset[sp]->randomize_grid(QuadPoints_host[sp]);
	QuadPoints_GPU[sp] = QuadPoints_host[sp];

	// First, we need to determine which ratios need to be updated
	find_core_electrons 
	  (W.RList_GPU.data(), NumElecs, 
	   Ions_GPU.data(), IonFirst[sp], IonLast[sp],
	   (CUDA_PRECISION)PPset[sp]->Rmax, L.data(), Linv.data(), 
	   QuadPoints_GPU[sp].data(), PPset[sp]->nknot,
	   Eleclist_GPU.data(), RatioPoslist_GPU.data(),
	   Distlist_GPU.data(), CosThetalist_GPU.data(),
	   NumPairs_GPU.data(), walkers.size());

	// Concatenate work into job list
	RatioPos_host = RatioPos_GPU;
	NumPairs_host = NumPairs_GPU;
	Elecs_host    = Elecs_GPU;
	CosTheta_host = CosTheta_GPU;
	Dist_host      = Dist_GPU;
	int numQuad = PPset[sp]->nknot;

	// HACK HACK HACK
	// cerr << "Dist_host.size() = " << Dist_host.size() <<endl;
	// for (int iw=0; iw<nw; iw++) {
	//   for (int ie=0; ie<NumPairs_host[iw]; ie++)
	//     if (Dist_host[MaxPairs*iw+ie] > 1.3)
	//       cerr << "Dist too long:  " << Dist_host[MaxPairs*iw+ie]
	// 	   << endl;
	// }

	JobList.clear();
	QuadPosList.clear();
	for (int iw=0; iw<nw; iw++) {
	  CUDA_PRECISION *pos_host = &(RatioPos_host[OHMMS_DIM*RatiosPerWalker*iw]);
	  int *elecs = &(Elecs_host[iw*MaxPairs]);
	  for (int ie=0; ie<NumPairs_host[iw]; ie++) {
	    JobList.push_back(NLjob(iw,*(elecs++),numQuad));
	    for (int iq=0; iq<numQuad; iq++) {
	      PosType r; 
	      for (int dim=0; dim<OHMMS_DIM; dim++)
		r[dim] = *(pos_host++);
	      QuadPosList.push_back(r);
	    }
	  }
	}




#ifdef CUDA_DEBUG
	gpu::host_vector<int> Elecs_host;
	gpu::host_vector<int> NumPairs_host;
	gpu::host_vector<CUDA_PRECISION> RatioPos_host;
	RatioPos_host = RatioPos_GPU;
	NumPairs_host = NumPairs_GPU;
	Elecs_host    = Elecs_GPU;

	DTD_BConds<double,3,SUPERCELL_BULK> bconds;
	for (int iw=0; iw<walkers.size(); iw++) {
	  Walker_t &w = *walkers[iw];
	  int index = 0;
	  int numPairs = 0;
	  for (int i=IonFirst[sp]; i<=IonLast[sp]; i++) {
	    PosType ion = SortedIons[i];
	    for (int e=0; e<NumElecs; e++) {
	      PosType elec = w.R[e];
	      PosType disp = elec-ion;
	      double dist = 
		std::sqrt(bconds.apply(IonConfig.Lattice, disp));
	      
	      if (dist < PPset[sp]->Rmax) {
		numPairs++;
		// fprintf (stderr, "i_CPU=%d  i_GPU=%d  elec_CPU=%d  elec_GPU=%d\n",
		// 	       i, Elecs_host[index],
		// 	       e, Elecs_host[index]);
		
		// int nknot = PPset[sp]->nknot;
		// for (int k=0; k<PPset[sp]->nknot; k++) {
		// 	PosType r = ion + dist * PPset[sp]->rrotsgrid_m[k];
		// 	fprintf (stderr, "CPU %d %12.6f %12.6f %12.6f\n",
		// 		 index, r[0], r[1], r[2]);
		// 	fprintf (stderr, "GPU %d %12.6f %12.6f %12.6f\n", index,
		// 		 RatioPos_host[3*index*nknot+3*k+0], 
		// 		 RatioPos_host[3*index*nknot+3*k+1],
		// 		 RatioPos_host[3*index*nknot+3*k+2]);
	      }
	      index ++;
	    }
	  }
	  if (numPairs != NumPairs_host[iw]) {
	    cerr << "numPairs = " << numPairs << endl;
	    cerr << "NumPairs_host[" << iw << "] =" << NumPairs_host[iw] << endl;
	  }
	}
#endif
  
	RatioList.resize(QuadPosList.size());

	RealType vrad[pp.nchannel];
	RealType lpol[pp.lmax+1];

	Psi.NLratios(W, JobList, QuadPosList, RatioList);
	int ratioIndex=0;
	for (int iw=0; iw<nw; iw++) {
	  CUDA_PRECISION *cos_ptr = &(CosTheta_host[RatiosPerWalker*iw]);
	  CUDA_PRECISION *dist_ptr = &(Dist_host[MaxPairs*iw]);
	  for (int ie=0; ie<NumPairs_host[iw]; ie++) {
	    RealType dist = *(dist_ptr++);

	    for (int ip=0; ip<pp.nchannel; ip++) 
	      vrad[ip] = pp.nlpp_m[ip]->splint(dist) * pp.wgt_angpp_m[ip];
	      
	    for (int iq=0; iq<numQuad; iq++) {
	      RealType costheta = *(cos_ptr++);
	      RealType ratio  = RatioList[ratioIndex++] * pp.sgridweight_m[iq];
	      // if (std::isnan(ratio)) {
	      // 	cerr << "NAN from ratio number " << ratioIndex-1 << "\n";
	      // 	cerr << "RatioList.size() = " << RatioList.size() << endl;
	      // }
		  
	      RealType lpolprev=0.0;
	      lpol[0] = 1.0;

	      for (int l=0 ; l< pp.lmax ; l++){
		//Not a big difference
		lpol[l+1]  = pp.Lfactor1[l]*costheta*lpol[l]-l*lpolprev; 
		lpol[l+1] *= pp.Lfactor2[l]; 
		lpolprev=lpol[l];
	      }

	      for (int ip=0 ; ip<pp.nchannel ; ip++) 
		esum[iw] += vrad[ip] * lpol[pp.angpp_m[ip]] * ratio;
	    }
	  }
	}
      }
    for (int iw=0; iw<walkers.size(); iw++){
      // if (std::isnan(esum[iw]))
      // 	app_log() << "NAN in esum.\n";

      walkers[iw]->getPropertyBase()[NUMPROPERTIES+myIndex] = esum[iw];
      LocalEnergy[iw] += esum[iw];
    }
  
  }




  void NonLocalECPotential_CUDA::addEnergy
  (MCWalkerConfiguration &W, vector<RealType> &LocalEnergy,
   vector<vector<NonLocalData> > &Txy)
  {
    vector<Walker_t*> &walkers = W.WalkerList;
    int nw = walkers.size();
    if (CurrentNumWalkers < nw)
      resizeCUDA(nw);

    // Loop over the ionic species
    vector<RealType> esum(walkers.size(), 0.0);
    for (int sp=0; sp<NumIonGroups; sp++) 
      if (PPset[sp]) {
	NonLocalECPComponent &pp = *PPset[sp];
	PPset[sp]->randomize_grid(QuadPoints_host[sp]);
	QuadPoints_GPU[sp] = QuadPoints_host[sp];

	// First, we need to determine which ratios need to be updated
	find_core_electrons 
	  (W.RList_GPU.data(), NumElecs, 
	   Ions_GPU.data(), IonFirst[sp], IonLast[sp],
	   (CUDA_PRECISION)PPset[sp]->Rmax, L.data(), Linv.data(), 
	   QuadPoints_GPU[sp].data(), PPset[sp]->nknot,
	   Eleclist_GPU.data(), RatioPoslist_GPU.data(),
	   Distlist_GPU.data(), CosThetalist_GPU.data(),
	   NumPairs_GPU.data(), walkers.size());

	// Concatenate work into job list
	RatioPos_host = RatioPos_GPU;
	NumPairs_host = NumPairs_GPU;
	Elecs_host    = Elecs_GPU;
	CosTheta_host = CosTheta_GPU;
	Dist_host      = Dist_GPU;
	int numQuad = PPset[sp]->nknot;

	JobList.clear();
	QuadPosList.clear();
	vector<int> iTxy(nw);
	for (int iw=0; iw<nw; iw++) {
	  iTxy[iw] = Txy[iw].size();
	  CUDA_PRECISION *pos_host = 
	    &(RatioPos_host[OHMMS_DIM*RatiosPerWalker*iw]);
	  int *elecs = &(Elecs_host[iw*MaxPairs]);
	  for (int ie=0; ie<NumPairs_host[iw]; ie++) {
	    int elec = *(elecs++);
	    JobList.push_back(NLjob(iw,elec,numQuad));
	    PosType rOld = walkers[iw]->R[elec];
	    for (int iq=0; iq<numQuad; iq++) {
	      PosType r; 
	      for (int dim=0; dim<OHMMS_DIM; dim++)
		r[dim] = *(pos_host++);
	      QuadPosList.push_back(r);
	      Txy[iw].push_back(NonLocalData(elec, 1.0, r-rOld));
	    }
	  }
	}
  

	RatioList.resize(QuadPosList.size());

	RealType vrad[pp.nchannel];
	RealType lpol[pp.lmax+1];

	Psi.NLratios(W, JobList, QuadPosList, RatioList);
	int ratioIndex=0;
	for (int iw=0; iw<nw; iw++) {
	  CUDA_PRECISION *cos_ptr = &(CosTheta_host[RatiosPerWalker*iw]);
	  CUDA_PRECISION *dist_ptr = &(Dist_host[MaxPairs*iw]);
	  for (int ie=0; ie<NumPairs_host[iw]; ie++) {
	    RealType dist = *(dist_ptr++);

	    for (int ip=0; ip<pp.nchannel; ip++) 
	      vrad[ip] = pp.nlpp_m[ip]->splint(dist) * pp.wgt_angpp_m[ip];
	      
	    for (int iq=0; iq<numQuad; iq++) {
	      RealType costheta = *(cos_ptr++);
	      RealType ratio  = RatioList[ratioIndex++] * pp.sgridweight_m[iq];
	      RealType lpolprev=0.0;
	      lpol[0] = 1.0;

	      for (int l=0 ; l< pp.lmax ; l++){
		//Not a big difference
		lpol[l+1]  = pp.Lfactor1[l]*costheta*lpol[l]-l*lpolprev; 
		lpol[l+1] *= pp.Lfactor2[l]; 
		lpolprev=lpol[l];
	      }
	      
	      RealType lsum = 0.0;
	      for (int ip=0 ; ip<pp.nchannel ; ip++) 
		lsum += vrad[ip] * lpol[pp.angpp_m[ip]];
	      esum[iw] += lsum * ratio;
	      Txy[iw][iTxy[iw]++].Weight = lsum*ratio;
	    }
	  }
	}
      }
    for (int iw=0; iw<walkers.size(); iw++){
      walkers[iw]->getPropertyBase()[NUMPROPERTIES+myIndex] = esum[iw];
      LocalEnergy[iw] += esum[iw];
    }
  }
  

}
