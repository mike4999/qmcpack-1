//////////////////////////////////////////////////////////////////
// (c) Copyright 2006-  by Jeongnim Kim and Ken Esler           //
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   National Center for Supercomputing Applications &          //
//   Materials Computation Center                               //
//   University of Illinois, Urbana-Champaign                   //
//   Urbana, IL 61801                                           //
//   e-mail: jnkim@ncsa.uiuc.edu                                //
//                                                              //
// Supported by                                                 //
//   National Center for Supercomputing Applications, UIUC      //
//   Materials Computation Center, UIUC                         //
//////////////////////////////////////////////////////////////////

#include "QMCWaveFunctions/EinsplineSet.h"
#include <einspline/multi_bspline.h>
#include <einspline/multi_bspline_eval_cuda.h>
#include "Configuration.h"
#include "AtomicOrbitalCuda.h"
#ifdef HAVE_MKL
  #include <mkl_vml.h>
#endif

void apply_phase_factors(float kPoints[], int makeTwoCopies[], 
			 float pos[], float *phi_in[], float *phi_out[], 
			 int num_splines, int num_walkers);

void apply_phase_factors(float kPoints[], int makeTwoCopies[], 
			 float pos[], float *phi_in[], float *phi_out[], 
			 float *GL_in[], float *GL_out[],
			 int num_splines, int num_walkers, int row_stride);

namespace qmcplusplus {
 
  // Real CUDA routines
//   inline void
//   eval_multi_multi_UBspline_3d_cuda (multi_UBspline_3d_s_cuda *spline, 
// 				     float *pos, float *phi[], int N)
//   { eval_multi_multi_UBspline_3d_s_cuda  (spline, pos, phi, N); }

  inline void
  eval_multi_multi_UBspline_3d_cuda (multi_UBspline_3d_s_cuda *spline, 
				     float *pos, float *sign, float *phi[], int N)
  { eval_multi_multi_UBspline_3d_s_sign_cuda  (spline, pos, sign, phi, N); }


  inline void
  eval_multi_multi_UBspline_3d_cuda (multi_UBspline_3d_d_cuda *spline, 
				     double *pos, double *phi[], int N)
  { eval_multi_multi_UBspline_3d_d_cuda  (spline, pos, phi, N); }


//   inline void eval_multi_multi_UBspline_3d_vgl_cuda 
//   (multi_UBspline_3d_s_cuda *spline, float *pos, float Linv[],
//    float *phi[], float *grad_lapl[], int N, int row_stride)
//   {
//     eval_multi_multi_UBspline_3d_s_vgl_cuda
//       (spline, pos, Linv, phi, grad_lapl, N, row_stride);
//   }

  inline void eval_multi_multi_UBspline_3d_vgl_cuda 
  (multi_UBspline_3d_s_cuda *spline, float *pos, float *sign, float Linv[],
   float *phi[], float *grad_lapl[], int N, int row_stride)
  {
    eval_multi_multi_UBspline_3d_s_vgl_sign_cuda
      (spline, pos, sign, Linv, phi, grad_lapl, N, row_stride);
  }


  inline void eval_multi_multi_UBspline_3d_vgl_cuda 
  (multi_UBspline_3d_d_cuda *spline, double *pos, double Linv[],
   double *phi[], double *grad_lapl[], int N, int row_stride)
  {
    eval_multi_multi_UBspline_3d_d_vgl_cuda
      (spline, pos, Linv, phi, grad_lapl, N, row_stride);
  }

  // Complex CUDA routines
  inline void
  eval_multi_multi_UBspline_3d_cuda (multi_UBspline_3d_c_cuda *spline, 
				     float *pos, complex<float> *phi[], int N)
  { eval_multi_multi_UBspline_3d_c_cuda  (spline, pos, phi, N); }

  inline void
  eval_multi_multi_UBspline_3d_cuda (multi_UBspline_3d_z_cuda *spline, 
				     double *pos, complex<double> *phi[], 
				     int N)
  { eval_multi_multi_UBspline_3d_z_cuda  (spline, pos, phi, N); }


  inline void eval_multi_multi_UBspline_3d_vgl_cuda 
  (multi_UBspline_3d_c_cuda *spline, float *pos, float Linv[],
   complex<float> *phi[], complex<float> *grad_lapl[], int N, int row_stride)
  {
    eval_multi_multi_UBspline_3d_c_vgl_cuda
      (spline, pos, Linv, phi, grad_lapl, N, row_stride);
  }

  inline void eval_multi_multi_UBspline_3d_vgl_cuda 
  (multi_UBspline_3d_z_cuda *spline, double *pos, double Linv[],
   complex<double> *phi[], complex<double> *grad_lapl[], int N, int row_stride)
  {
    eval_multi_multi_UBspline_3d_z_vgl_cuda
      (spline, pos, Linv, phi, grad_lapl, N, row_stride);
  }




  EinsplineSet::UnitCellType
  EinsplineSet::GetLattice()
  {
    return SuperLattice;
  }
  
  void
  EinsplineSet::resetTargetParticleSet(ParticleSet& e)
  {
  }

  void
  EinsplineSet::resetSourceParticleSet(ParticleSet& ions)
  {
  }
  
  void
  EinsplineSet::setOrbitalSetSize(int norbs)
  {
    OrbitalSetSize = norbs;
  }
  
  void 
  EinsplineSet::evaluate (const ParticleSet& P, int iat, ValueVector_t& psi)
  {
    app_error() << "Should never instantiate EinsplineSet.\n";
    abort();
  }

  void 
  EinsplineSet::evaluate (const ParticleSet& P, int iat, 
			  ValueVector_t& psi, GradVector_t& dpsi, 
			  ValueVector_t& d2psi)
  {
    app_error() << "Should never instantiate EinsplineSet.\n";
    abort();
  }

  
  void 
  EinsplineSet::evaluate (const ParticleSet& P, int first, int last,
			  ValueMatrix_t& vals, GradMatrix_t& grads, 
			  ValueMatrix_t& lapls)
  {
    app_error() << "Should never instantiate EinsplineSet.\n";
    abort();
  }


  void 
  EinsplineSetLocal::evaluate (const ParticleSet& P, int iat, 
			       ValueVector_t& psi)
  {
    PosType r (P.R[iat]);
    PosType ru(PrimLattice.toUnit(P.R[iat]));
    ru[0] -= std::floor (ru[0]);
    ru[1] -= std::floor (ru[1]);
    ru[2] -= std::floor (ru[2]);
    for(int j=0; j<OrbitalSetSize; j++) {
      complex<double> val;
      Orbitals[j]->evaluate(ru, val);

      double phase = -dot(r, Orbitals[j]->kVec);
      double s,c;
      sincos (phase, &s, &c);
      complex<double> e_mikr (c,s);
      val *= e_mikr;
#ifdef QMC_COMPLEX
      psi[j] = val;
#else
      psi[j] = real(val);
#endif
    }
  }
  
  void 
  EinsplineSetLocal::evaluate (const ParticleSet& P, int iat, 
			       ValueVector_t& psi, GradVector_t& dpsi, 
			       ValueVector_t& d2psi)
  {
    PosType r (P.R[iat]);
    PosType ru(PrimLattice.toUnit(P.R[iat]));
    ru[0] -= std::floor (ru[0]);
    ru[1] -= std::floor (ru[1]);
    ru[2] -= std::floor (ru[2]);
    complex<double> val;
    TinyVector<complex<double>,3> gu;
    Tensor<complex<double>,3> hess;
    complex<double> eye (0.0, 1.0);
    for(int j=0; j<OrbitalSetSize; j++) {
      complex<double> u;
      TinyVector<complex<double>,3> gradu;
      complex<double> laplu;

      Orbitals[j]->evaluate(ru, val, gu, hess);
      u  = val;
      // Compute gradient in cartesian coordinates
      gradu = dot(PrimLattice.G, gu);
      laplu = trace(hess, GGt);      
      
      PosType k = Orbitals[j]->kVec;
      TinyVector<complex<double>,3> ck;
      ck[0]=k[0];  ck[1]=k[1];  ck[2]=k[2];
      double s,c;
      double phase = -dot(P.R[iat], k);
      sincos (phase, &s, &c);
      complex<double> e_mikr (c,s);
#ifdef QMC_COMPLEX
      psi[j]   = e_mikr * u;
      dpsi[j]  = e_mikr*(-eye * ck * u + gradu);
      d2psi[j] = e_mikr*(-dot(k,k)*u - 2.0*eye*dot(ck,gradu) + laplu);
#else
      psi[j]   = real(e_mikr * u);
      dpsi[j]  = real(e_mikr*(-eye * ck * u + gradu));
      d2psi[j] = real(e_mikr*(-dot(k,k)*u - 2.0*eye*dot(ck,gradu) + laplu));
#endif

    }
  }
  
  void 
  EinsplineSetLocal::evaluate (const ParticleSet& P, int first, int last,
			       ValueMatrix_t& vals, GradMatrix_t& grads, 
			       ValueMatrix_t& lapls)
  {
    for(int iat=first,i=0; iat<last; iat++,i++) {
      PosType r (P.R[iat]);
      PosType ru(PrimLattice.toUnit(r));
      ru[0] -= std::floor (ru[0]);
      ru[1] -= std::floor (ru[1]);
      ru[2] -= std::floor (ru[2]);
      complex<double> val;
      TinyVector<complex<double>,3> gu;
      Tensor<complex<double>,3> hess;
      complex<double> eye (0.0, 1.0);
      for(int j=0; j<OrbitalSetSize; j++) {
	complex<double> u;
	TinyVector<complex<double>,3> gradu;
	complex<double> laplu;

	Orbitals[j]->evaluate(ru, val, gu, hess);
	u  = val;
	gradu = dot(PrimLattice.G, gu);
	laplu = trace(hess, GGt);
	
	PosType k = Orbitals[j]->kVec;
	TinyVector<complex<double>,3> ck;
	ck[0]=k[0];  ck[1]=k[1];  ck[2]=k[2];
	double s,c;
	double phase = -dot(r, k);
	sincos (phase, &s, &c);
	complex<double> e_mikr (c,s);
#ifdef QMC_COMPLEX
	vals(j,i)  = e_mikr * u;
	grads(i,j) = e_mikr*(-eye*u*ck + gradu);
	lapls(i,j) = e_mikr*(-dot(k,k)*u - 2.0*eye*dot(ck,gradu) + laplu);
#else
	vals(j,i)  = real(e_mikr * u);
	grads(i,j) = real(e_mikr*(-eye*u*ck + gradu));
	lapls(i,j) = real(e_mikr*(-dot(k,k)*u - 2.0*eye*dot(ck,gradu) + laplu));
#endif

      }
    }
  }

  string 
  EinsplineSet::Type()
  {
    return "EinsplineSet";
  }




  ///////////////////////////////////////////
  // EinsplineSetExtended Member functions //
  ///////////////////////////////////////////

  inline void
  convert (complex<double> a, complex<double> &b)
  { b = a;  }

  inline void
  convert (complex<double> a, double &b)
  { b = a.real();  }

  inline void
  convert (double a, complex<double>&b)
  { b = complex<double>(a,0.0); }

  inline void
  convert (double a, double &b)
  { b = a; }

  template<typename T1, typename T2> void
  convertVec (TinyVector<T1,3> a, TinyVector<T2,3> &b)
  {
    for (int i=0; i<3; i++)
      convert (a[i], b[i]);
  }

  //////////////////////
  // Double precision //
  //////////////////////

  // Real evaluation functions
  inline void 
  EinsplineMultiEval (multi_UBspline_3d_d *restrict spline,
		      TinyVector<double,3> r, 
		      Vector<double> &psi)
  {
    eval_multi_UBspline_3d_d (spline, r[0], r[1], r[2], psi.data());
  }

  inline void
  EinsplineMultiEval (multi_UBspline_3d_d *restrict spline,
		      TinyVector<double,3> r, 
		      vector<double> &psi)
  {
    eval_multi_UBspline_3d_d (spline, r[0], r[1], r[2], &(psi[0]));
  }


  inline void
  EinsplineMultiEval (multi_UBspline_3d_d *restrict spline,
		      TinyVector<double,3> r,
		      Vector<double> &psi,
		      Vector<TinyVector<double,3> > &grad,
		      Vector<Tensor<double,3> > &hess)
  {
    eval_multi_UBspline_3d_d_vgh (spline, r[0], r[1], r[2],
				  psi.data(), 
				  (double*)grad.data(), 
				  (double*)hess.data());
  }

  //////////////////////////////////
  // Complex evaluation functions //
  //////////////////////////////////
  inline void 
  EinsplineMultiEval (multi_UBspline_3d_z *restrict spline,
		      TinyVector<double,3> r, 
		      Vector<complex<double> > &psi)
  {
    eval_multi_UBspline_3d_z (spline, r[0], r[1], r[2], psi.data());
  }


  inline void
  EinsplineMultiEval (multi_UBspline_3d_z *restrict spline,
		      TinyVector<double,3> r,
		      Vector<complex<double> > &psi,
		      Vector<TinyVector<complex<double>,3> > &grad,
		      Vector<Tensor<complex<double>,3> > &hess)
  {
    eval_multi_UBspline_3d_z_vgh (spline, r[0], r[1], r[2],
				  psi.data(), 
				  (complex<double>*)grad.data(), 
				  (complex<double>*)hess.data());
  }

  //////////////////////
  // Single precision //
  //////////////////////

  // Real evaluation functions
  inline void 
  EinsplineMultiEval (multi_UBspline_3d_s *restrict spline,
		      TinyVector<float,3> r, 
		      Vector<float> &psi)
  {
    eval_multi_UBspline_3d_s (spline, r[0], r[1], r[2], psi.data());
  }

  inline void
  EinsplineMultiEval (multi_UBspline_3d_s *restrict spline,
		      TinyVector<float,3> r,
		      Vector<float> &psi,
		      Vector<TinyVector<float,3> > &grad,
		      Vector<Tensor<float,3> > &hess)
  {
    eval_multi_UBspline_3d_s_vgh (spline, r[0], r[1], r[2],
				  psi.data(), 
				  (float*)grad.data(), 
				  (float*)hess.data());
  }

  // Complex evaluation functions 

  inline void 
  EinsplineMultiEval (multi_UBspline_3d_c *restrict spline,
		      TinyVector<float,3> r, 
		      Vector<complex<float> > &psi)
  {
    eval_multi_UBspline_3d_c (spline, r[0], r[1], r[2], psi.data());
  }


  inline void
  EinsplineMultiEval (multi_UBspline_3d_c *restrict spline,
		      TinyVector<float,3> r,
		      Vector<complex<float> > &psi,
		      Vector<TinyVector<complex<float>,3> > &grad,
		      Vector<Tensor<complex<float>,3> > &hess)
  {
    eval_multi_UBspline_3d_c_vgh (spline, r[0], r[1], r[2],
				  psi.data(), 
				  (complex<float>*)grad.data(), 
				  (complex<float>*)hess.data());
  }



			   

  template<typename StorageType> void
  EinsplineSetExtended<StorageType>::resetParameters(const opt_variables_type& active)
  {

  }

  template<typename StorageType> void
  EinsplineSetExtended<StorageType>::resetTargetParticleSet(ParticleSet& e)
  {
  }

  template<typename StorageType> void
  EinsplineSetExtended<StorageType>::setOrbitalSetSize(int norbs)
  {
    OrbitalSetSize = norbs;
  }
  
  template<typename StorageType> void
  EinsplineSetExtended<StorageType>::evaluate
  (const ParticleSet& P, int iat, RealValueVector_t& psi)
  {
    ValueTimer.start();
    PosType r (P.R[iat]);

    // Do core states first
    int icore = NumValenceOrbs;
    for (int tin=0; tin<MuffinTins.size(); tin++) {
      MuffinTins[tin].evaluateCore(r, StorageValueVector, icore);
      icore += MuffinTins[tin].get_num_core();
    }
    // Add phase to core orbitals
    for (int j=NumValenceOrbs; j<StorageValueVector.size(); j++) {
      PosType k = kPoints[j];
      double s,c;
      double phase = -dot(r, k);
      sincos (phase, &s, &c);
      complex<double> e_mikr (c,s);
      StorageValueVector[j] *= e_mikr;
    }

    // Check if we are inside a muffin tin.  If so, compute valence
    // states in the muffin tin.
    bool inTin = false;
    bool need2blend = false;
    double b(0.0);
    for (int tin=0; tin<MuffinTins.size() && !inTin; tin++) {
      MuffinTins[tin].inside(r, inTin, need2blend);
      if (inTin) {
	MuffinTins[tin].evaluate (r, StorageValueVector);
	if (need2blend) {
	  PosType disp = MuffinTins[tin].disp(r);
	  double dr = std::sqrt(dot(disp, disp));
	  MuffinTins[tin].blend_func(dr, b);
	}
	break;
      }
    }

    // Check atomic orbitals
    bool inAtom = false;
    for (int jat=0; jat<AtomicOrbitals.size(); jat++) {
      inAtom = AtomicOrbitals[jat].evaluate (r, StorageValueVector);
      if (inAtom) break;
    }
      
    StorageValueVector_t &valVec = 
      need2blend ? BlendValueVector : StorageValueVector;

    if (!inTin || need2blend) {
      if (!inAtom) {
	PosType ru(PrimLattice.toUnit(P.R[iat]));
	for (int i=0; i<OHMMS_DIM; i++)
	  ru[i] -= std::floor (ru[i]);
	EinsplineTimer.start();
	EinsplineMultiEval (MultiSpline, ru, valVec);
	EinsplineTimer.stop();
      
	// Add e^ikr phase to B-spline orbitals
	for (int j=0; j<NumValenceOrbs; j++) {
	  PosType k = kPoints[j];
	  double s,c;
	  double phase = -dot(r, k);
	  sincos (phase, &s, &c);
	  complex<double> e_mikr (c,s);
	  valVec[j] *= e_mikr;
	}
      }
    }
    int N = StorageValueVector.size();

    // If we are in a muffin tin, don't add the e^ikr term
    // We should add it to the core states, however

    if (need2blend) {
      int psiIndex = 0;
      for (int j=0; j<N; j++) {
	complex<double> psi1 = StorageValueVector[j];
	complex<double> psi2 =   BlendValueVector[j];
	
	complex<double> psi_val = b * psi1 + (1.0-b) * psi2;

	psi[psiIndex] = real(psi_val);
	psiIndex++;
	if (MakeTwoCopies[j]) {
	  psi[psiIndex] = imag(psi_val);
	  psiIndex++;
	}
      }

    }
    else {
      int psiIndex = 0;
      for (int j=0; j<N; j++) {
	complex<double> psi_val = StorageValueVector[j];
	psi[psiIndex] = real(psi_val);
	psiIndex++;
	if (MakeTwoCopies[j]) {
	  psi[psiIndex] = imag(psi_val);
	  psiIndex++;
	}
      }
    }
    ValueTimer.stop();
  }


  template<typename StorageType> void
  EinsplineSetExtended<StorageType>::evaluate
  (const ParticleSet& P, int iat, ComplexValueVector_t& psi)
  {
    ValueTimer.start();
    PosType r (P.R[iat]);
    PosType ru(PrimLattice.toUnit(P.R[iat]));
    for (int i=0; i<OHMMS_DIM; i++)
      ru[i] -= std::floor (ru[i]);
    EinsplineTimer.start();
    EinsplineMultiEval (MultiSpline, ru, StorageValueVector);
    EinsplineTimer.stop();
    //computePhaseFactors(r);
    for (int i=0; i<psi.size(); i++) {
      PosType k = kPoints[i];
      double s,c;
      double phase = -dot(r, k);
      sincos (phase, &s, &c);
      complex<double> e_mikr (c,s);
      convert (e_mikr*StorageValueVector[i], psi[i]);
    }
    ValueTimer.stop();
  }

  // This is an explicit specialization of the above for real orbitals
  // with a real return value, i.e. simulations at the gamma or L 
  // point.
  template<> void
  EinsplineSetExtended<double>::evaluate
  (const ParticleSet &P, int iat, RealValueVector_t& psi)
  {
    ValueTimer.start();
    PosType r (P.R[iat]);
    bool inAtom = false;
    for (int jat=0; jat<AtomicOrbitals.size(); jat++) {
      inAtom = AtomicOrbitals[jat].evaluate (r, psi);
      if (inAtom) 
	break;
    }
    if (!inAtom) {
      PosType ru(PrimLattice.toUnit(P.R[iat]));
      int sign=0;
      for (int i=0; i<OHMMS_DIM; i++) {
	RealType img = std::floor(ru[i]);
	ru[i] -= img;
	sign += HalfG[i] * (int)img;
      }
      // Check atomic orbitals
      EinsplineTimer.start();
      EinsplineMultiEval (MultiSpline, ru, psi);
      EinsplineTimer.stop();
    
      if (sign & 1) 
	for (int j=0; j<psi.size(); j++)
	  psi[j] *= -1.0;
    }
    ValueTimer.stop();
  }

  template<> void
  EinsplineSetExtended<double>::evaluate
  (const ParticleSet &P, PosType r, vector<RealType> &psi)
  {
    ValueTimer.start();
    PosType ru(PrimLattice.toUnit(r));
    int image[OHMMS_DIM];
    for (int i=0; i<OHMMS_DIM; i++) {
      RealType img = std::floor(ru[i]);
      ru[i] -= img;
      image[i] = (int) img;
    }
    EinsplineTimer.start();
    EinsplineMultiEval (MultiSpline, ru, psi);
    EinsplineTimer.stop();
    int sign=0;
    for (int i=0; i<OHMMS_DIM; i++) 
      sign += HalfG[i]*image[i];
    if (sign & 1) 
      for (int j=0; j<psi.size(); j++)
	psi[j] *= -1.0;
    ValueTimer.stop();
  }

  template<> void
  EinsplineSetExtended<complex<double> >::evaluate
  (const ParticleSet &P, PosType r, vector<RealType> &psi)
  {
    cerr << "Not Implemented.\n";
  }


  // Value, gradient, and laplacian
  template<typename StorageType> void
  EinsplineSetExtended<StorageType>::evaluate
  (const ParticleSet& P, int iat, RealValueVector_t& psi, 
   RealGradVector_t& dpsi, RealValueVector_t& d2psi)
  {
    VGLTimer.start();
    PosType r (P.R[iat]);
    complex<double> eye (0.0, 1.0);
    
    // Do core states first
    int icore = NumValenceOrbs;
    for (int tin=0; tin<MuffinTins.size(); tin++) {
      MuffinTins[tin].evaluateCore(r, StorageValueVector, StorageGradVector, 
				   StorageLaplVector, icore);
      icore += MuffinTins[tin].get_num_core();
    }
    
    // Add phase to core orbitals
    for (int j=NumValenceOrbs; j<StorageValueVector.size(); j++) {
      complex<double> u = StorageValueVector[j];
      TinyVector<complex<double>,OHMMS_DIM> gradu = StorageGradVector[j];
      complex<double> laplu = StorageLaplVector[j];
      PosType k = kPoints[j];
      TinyVector<complex<double>,OHMMS_DIM> ck;
      for (int n=0; n<OHMMS_DIM; n++)	  ck[n] = k[n];
      double s,c;
      double phase = -dot(r, k);
      sincos (phase, &s, &c);
      complex<double> e_mikr (c,s);
      StorageValueVector[j] = e_mikr*u;
      StorageGradVector[j]  = e_mikr*(-eye*u*ck + gradu);
      StorageLaplVector[j]  = e_mikr*(-dot(k,k)*u - 2.0*eye*dot(ck,gradu) + laplu);
    }

    // Check muffin tins;  if inside evaluate the orbitals
    bool inTin = false;
    bool need2blend = false;
    PosType disp;
    double b, db, d2b;
    for (int tin=0; tin<MuffinTins.size(); tin++) {
      MuffinTins[tin].inside(r, inTin, need2blend);
      if (inTin) {
	MuffinTins[tin].evaluate (r, StorageValueVector, StorageGradVector, StorageLaplVector);
	if (need2blend) {
	  disp = MuffinTins[tin].disp(r);
	  double dr = std::sqrt(dot(disp, disp));
	  MuffinTins[tin].blend_func(dr, b, db, d2b);
	}
	break;
      }
    }

    bool inAtom = false;
    for (int jat=0; jat<AtomicOrbitals.size(); jat++) {
      inAtom = AtomicOrbitals[jat].evaluate 
	(r, StorageValueVector, StorageGradVector, StorageLaplVector);
      if (inAtom) break;
    }

    StorageValueVector_t &valVec =  
      need2blend ? BlendValueVector : StorageValueVector;
    StorageGradVector_t &gradVec =  
      need2blend ? BlendGradVector : StorageGradVector;
    StorageValueVector_t &laplVec =  
      need2blend ? BlendLaplVector : StorageLaplVector;

    // Otherwise, evaluate the B-splines
    if (!inTin || need2blend) {
      if (!inAtom) {
	PosType ru(PrimLattice.toUnit(P.R[iat]));
	for (int i=0; i<OHMMS_DIM; i++)
	  ru[i] -= std::floor (ru[i]);
	EinsplineTimer.start();
	EinsplineMultiEval (MultiSpline, ru, valVec, gradVec, StorageHessVector);
	EinsplineTimer.stop();
	for (int j=0; j<NumValenceOrbs; j++) {
	  gradVec[j] = dot (PrimLattice.G, gradVec[j]);
	  laplVec[j] = trace (StorageHessVector[j], GGt);
	}
      
	// Add e^-ikr phase to B-spline orbitals
	for (int j=0; j<NumValenceOrbs; j++) {
	  complex<double> u = valVec[j];
	  TinyVector<complex<double>,OHMMS_DIM> gradu = gradVec[j];
	  complex<double> laplu = laplVec[j];
	  PosType k = kPoints[j];
	  TinyVector<complex<double>,OHMMS_DIM> ck;
	  for (int n=0; n<OHMMS_DIM; n++)	  ck[n] = k[n];
	  double s,c;
	  double phase = -dot(r, k);
	  sincos (phase, &s, &c);
	  complex<double> e_mikr (c,s);
	  valVec[j]   = e_mikr*u;
	  gradVec[j]  = e_mikr*(-eye*u*ck + gradu);
	  laplVec[j]  = e_mikr*(-dot(k,k)*u - 2.0*eye*dot(ck,gradu) + laplu);
	}
      }
    }

    // Finally, copy into output vectors
    int psiIndex = 0;
    int N = StorageValueVector.size();
    if (need2blend) {
      for (int j=0; j<NumValenceOrbs; j++) {
	complex<double> psi_val, psi_lapl;
	TinyVector<complex<double>, OHMMS_DIM> psi_grad;
	PosType rhat = 1.0/std::sqrt(dot(disp,disp)) * disp;
	complex<double> psi1 = StorageValueVector[j];
	complex<double> psi2 =   BlendValueVector[j];
	TinyVector<complex<double>,OHMMS_DIM> dpsi1 = StorageGradVector[j];
	TinyVector<complex<double>,OHMMS_DIM> dpsi2 = BlendGradVector[j];
	complex<double> d2psi1 = StorageLaplVector[j];
	complex<double> d2psi2 =   BlendLaplVector[j];
	
	TinyVector<complex<double>,OHMMS_DIM> zrhat;
	for (int i=0; i<OHMMS_DIM; i++)
	  zrhat[i] = rhat[i];

	psi_val  = b * psi1 + (1.0-b)*psi2;
	psi_grad = b * dpsi1 + (1.0-b)*dpsi2 + db * (psi1 - psi2)* zrhat;
	psi_lapl = b * d2psi1 + (1.0-b)*d2psi2 +
	  2.0*db * (dot(zrhat,dpsi1) - dot(zrhat, dpsi2)) +
	  d2b * (psi1 - psi2);
	
	psi[psiIndex] = real(psi_val);
	for (int n=0; n<OHMMS_DIM; n++)
	  dpsi[psiIndex][n] = real(psi_grad[n]);
	d2psi[psiIndex] = real(psi_lapl);
	psiIndex++;
	if (MakeTwoCopies[j]) {
	  psi[psiIndex] = imag(psi_val);
	  for (int n=0; n<OHMMS_DIM; n++)
	    dpsi[psiIndex][n] = imag(psi_grad[n]);
	  d2psi[psiIndex] = imag(psi_lapl);
	  psiIndex++;
	}
      } 
      for (int j=NumValenceOrbs; j<N; j++) {
	complex<double> psi_val, psi_lapl;
	TinyVector<complex<double>, OHMMS_DIM> psi_grad;
	psi_val  = StorageValueVector[j];
	psi_grad = StorageGradVector[j];
	psi_lapl = StorageLaplVector[j];
	
	psi[psiIndex] = real(psi_val);
	for (int n=0; n<OHMMS_DIM; n++)
	  dpsi[psiIndex][n] = real(psi_grad[n]);
	d2psi[psiIndex] = real(psi_lapl);
	psiIndex++;
	if (MakeTwoCopies[j]) {
	  psi[psiIndex] = imag(psi_val);
	  for (int n=0; n<OHMMS_DIM; n++)
	    dpsi[psiIndex][n] = imag(psi_grad[n]);
	  d2psi[psiIndex] = imag(psi_lapl);
	  psiIndex++;
	}
      }
    }
    else {
      for (int j=0; j<N; j++) {
	complex<double> psi_val, psi_lapl;
	TinyVector<complex<double>, OHMMS_DIM> psi_grad;
	psi_val  = StorageValueVector[j];
	psi_grad = StorageGradVector[j];
	psi_lapl = StorageLaplVector[j];
	
	psi[psiIndex] = real(psi_val);
	for (int n=0; n<OHMMS_DIM; n++)
	  dpsi[psiIndex][n] = real(psi_grad[n]);
	d2psi[psiIndex] = real(psi_lapl);
	psiIndex++;
	if (MakeTwoCopies[j]) {
	  psi[psiIndex] = imag(psi_val);
	  for (int n=0; n<OHMMS_DIM; n++)
	    dpsi[psiIndex][n] = imag(psi_grad[n]);
	  d2psi[psiIndex] = imag(psi_lapl);
	  psiIndex++;
	}
      }
    }
    VGLTimer.stop();
  }
  
  // Value, gradient, and laplacian
  template<typename StorageType> void
  EinsplineSetExtended<StorageType>::evaluate
  (const ParticleSet& P, int iat, ComplexValueVector_t& psi, 
   ComplexGradVector_t& dpsi, ComplexValueVector_t& d2psi)
  {
    VGLTimer.start();
    PosType r (P.R[iat]);
    PosType ru(PrimLattice.toUnit(P.R[iat]));
    for (int i=0; i<OHMMS_DIM; i++)
      ru[i] -= std::floor (ru[i]);
    EinsplineTimer.start();
    EinsplineMultiEval (MultiSpline, ru, StorageValueVector,
			StorageGradVector, StorageHessVector);
    EinsplineTimer.stop();
    //computePhaseFactors(r);
    complex<double> eye (0.0, 1.0);
    for (int j=0; j<psi.size(); j++) {
      complex<double> u, laplu;
      TinyVector<complex<double>, OHMMS_DIM> gradu;
      u = StorageValueVector[j];
      gradu = dot(PrimLattice.G, StorageGradVector[j]);
      laplu = trace(StorageHessVector[j], GGt);
      
      PosType k = kPoints[j];
      TinyVector<complex<double>,OHMMS_DIM> ck;
      for (int n=0; n<OHMMS_DIM; n++)	
	ck[n] = k[n];
      double s,c;
      double phase = -dot(r, k);
      sincos (phase, &s, &c);
      complex<double> e_mikr (c,s);
      convert(e_mikr * u, psi[j]);
      convertVec(e_mikr*(-eye*u*ck + gradu), dpsi[j]);
      convert(e_mikr*(-dot(k,k)*u - 2.0*eye*dot(ck,gradu) + laplu), d2psi[j]);
    }
    VGLTimer.stop();
  }
  
  
  template<> void
  EinsplineSetExtended<double>::evaluate
  (const ParticleSet& P, int iat, RealValueVector_t& psi, 
   RealGradVector_t& dpsi, RealValueVector_t& d2psi)
  {
    VGLTimer.start();
    PosType r (P.R[iat]);
    bool inAtom = false;
    for (int jat=0; jat<AtomicOrbitals.size(); jat++) {
      inAtom = AtomicOrbitals[jat].evaluate (r, psi, dpsi, d2psi);
      if (inAtom) 
	break;
    }
    if (!inAtom) {
      PosType ru(PrimLattice.toUnit(P.R[iat]));
      
      int sign=0;
      for (int i=0; i<OHMMS_DIM; i++) {
	RealType img = std::floor(ru[i]);
	ru[i] -= img;
	sign += HalfG[i] * (int)img;
      }
      
      EinsplineTimer.start();
      EinsplineMultiEval (MultiSpline, ru, psi, StorageGradVector, 
			  StorageHessVector);
      EinsplineTimer.stop();
    
      if (sign & 1) 
	for (int j=0; j<psi.size(); j++) {
	  psi[j] *= -1.0;
	  StorageGradVector[j] *= -1.0;
	  StorageHessVector[j] *= -1.0;
	}
      for (int i=0; i<psi.size(); i++) {
	dpsi[i]  = dot(PrimLattice.G, StorageGradVector[i]);
	d2psi[i] = trace(StorageHessVector[i], GGt);
      }
    }
    VGLTimer.stop();
  }
  
  
  template<typename StorageType> void
  EinsplineSetExtended<StorageType>::evaluate
  (const ParticleSet& P, int first, int last, RealValueMatrix_t& psi, 
   RealGradMatrix_t& dpsi, RealValueMatrix_t& d2psi)
  {
    complex<double> eye(0.0,1.0);
    VGLMatTimer.start();
    for (int iat=first,i=0; iat<last; iat++,i++) {
      PosType r (P.R[iat]);
      
      // Do core states first
      int icore = NumValenceOrbs;
      for (int tin=0; tin<MuffinTins.size(); tin++) {
	MuffinTins[tin].evaluateCore(r, StorageValueVector, StorageGradVector,
				     StorageLaplVector, icore);
	icore += MuffinTins[tin].get_num_core();
      }
      
      // Add phase to core orbitals
      for (int j=NumValenceOrbs; j<StorageValueVector.size(); j++) {
	complex<double> u = StorageValueVector[j];
	TinyVector<complex<double>,OHMMS_DIM> gradu = StorageGradVector[j];
	complex<double> laplu = StorageLaplVector[j];
	PosType k = kPoints[j];
	TinyVector<complex<double>,OHMMS_DIM> ck;
	for (int n=0; n<OHMMS_DIM; n++)	  ck[n] = k[n];
	double s,c;
	double phase = -dot(r, k);
	sincos (phase, &s, &c);
	complex<double> e_mikr (c,s);
	StorageValueVector[j] = e_mikr*u;
	StorageGradVector[j]  = e_mikr*(-eye*u*ck + gradu);
	StorageLaplVector[j]  = e_mikr*(-dot(k,k)*u - 2.0*eye*dot(ck,gradu) + laplu);
      }
      
      // Check if we are in the muffin tin;  if so, evaluate
      bool inTin = false, need2blend = false;
      PosType disp;
      double b, db, d2b;
      for (int tin=0; tin<MuffinTins.size(); tin++) {
	MuffinTins[tin].inside(r, inTin, need2blend);
	if (inTin) {
	  MuffinTins[tin].evaluate (r, StorageValueVector, 
				    StorageGradVector, StorageLaplVector);
	  if (need2blend) {
	    disp = MuffinTins[tin].disp(r);
	    double dr = std::sqrt(dot(disp, disp));
	    MuffinTins[tin].blend_func(dr, b, db, d2b);
	  }
	  break;
	}
      }
      bool inAtom = false;
      for (int jat=0; jat<AtomicOrbitals.size(); jat++) {
	inAtom = AtomicOrbitals[jat].evaluate 
	  (r, StorageValueVector, StorageGradVector, StorageLaplVector);
	if (inAtom) break;
      }
      
      StorageValueVector_t &valVec =  
	need2blend ? BlendValueVector : StorageValueVector;
      StorageGradVector_t &gradVec =  
	need2blend ? BlendGradVector : StorageGradVector;
      StorageValueVector_t &laplVec =  
	need2blend ? BlendLaplVector : StorageLaplVector;
      
      // Otherwise, evaluate the B-splines
      if (!inTin || need2blend) {
	if (!inAtom) {
	  PosType ru(PrimLattice.toUnit(P.R[iat]));
	  for (int i=0; i<OHMMS_DIM; i++)
	    ru[i] -= std::floor (ru[i]);
	  EinsplineTimer.start();
	  EinsplineMultiEval (MultiSpline, ru, valVec, gradVec, StorageHessVector);
	  EinsplineTimer.stop();
	  for (int j=0; j<NumValenceOrbs; j++) {
	    gradVec[j] = dot (PrimLattice.G, gradVec[j]);
	    laplVec[j] = trace (StorageHessVector[j], GGt);
	  }
	  
	  // Add e^-ikr phase to B-spline orbitals
	  for (int j=0; j<NumValenceOrbs; j++) {
	    complex<double> u = valVec[j];
	    TinyVector<complex<double>,OHMMS_DIM> gradu = gradVec[j];
	    complex<double> laplu = laplVec[j];
	    PosType k = kPoints[j];
	    TinyVector<complex<double>,OHMMS_DIM> ck;
	    for (int n=0; n<OHMMS_DIM; n++)	  ck[n] = k[n];
	    double s,c;
	    double phase = -dot(r, k);
	    sincos (phase, &s, &c);
	    complex<double> e_mikr (c,s);
	    valVec[j]   = e_mikr*u;
	    gradVec[j]  = e_mikr*(-eye*u*ck + gradu);
	    laplVec[j]  = e_mikr*(-dot(k,k)*u - 2.0*eye*dot(ck,gradu) + laplu);
	  }
	}
      }
      
      // Finally, copy into output vectors
      int psiIndex = 0;
      int N = StorageValueVector.size();
      if (need2blend) {
	for (int j=0; j<NumValenceOrbs; j++) {
	  complex<double> psi_val, psi_lapl;
	  TinyVector<complex<double>, OHMMS_DIM> psi_grad;
	  PosType rhat = 1.0/std::sqrt(dot(disp,disp)) * disp;
	  complex<double> psi1 = StorageValueVector[j];
	  complex<double> psi2 =   BlendValueVector[j];
	  TinyVector<complex<double>,OHMMS_DIM> dpsi1 = StorageGradVector[j];
	  TinyVector<complex<double>,OHMMS_DIM> dpsi2 = BlendGradVector[j];
	  complex<double> d2psi1 = StorageLaplVector[j];
	  complex<double> d2psi2 =   BlendLaplVector[j];
	  
	  TinyVector<complex<double>,OHMMS_DIM> zrhat;
	  for (int n=0; n<OHMMS_DIM; n++)
	    zrhat[n] = rhat[n];
	  
	  psi_val  = b * psi1 + (1.0-b)*psi2;
	  psi_grad = b * dpsi1 + (1.0-b)*dpsi2 + db * (psi1 - psi2)* zrhat;
	  psi_lapl = b * d2psi1 + (1.0-b)*d2psi2 +
	    2.0*db * (dot(zrhat,dpsi1) - dot(zrhat, dpsi2)) +
	    d2b * (psi1 - psi2);
	  
	  psi(psiIndex,i) = real(psi_val);
	  for (int n=0; n<OHMMS_DIM; n++)
	    dpsi(i,psiIndex)[n] = real(psi_grad[n]);
	  d2psi(i,psiIndex) = real(psi_lapl);
	  psiIndex++;
	  if (MakeTwoCopies[j]) {
	    psi(psiIndex,i) = imag(psi_val);
	    for (int n=0; n<OHMMS_DIM; n++)
	      dpsi(i,psiIndex)[n] = imag(psi_grad[n]);
	    d2psi(i,psiIndex) = imag(psi_lapl);
	    psiIndex++;
	  }
	} 
	// Copy core states
	for (int j=NumValenceOrbs; j<N; j++) {
	  complex<double> psi_val, psi_lapl;
	  TinyVector<complex<double>, OHMMS_DIM> psi_grad;
	  psi_val  = StorageValueVector[j];
	  psi_grad = StorageGradVector[j];
	  psi_lapl = StorageLaplVector[j];
	  
	  psi(psiIndex,i) = real(psi_val);
	  for (int n=0; n<OHMMS_DIM; n++)
	    dpsi(i,psiIndex)[n] = real(psi_grad[n]);
	  d2psi(i,psiIndex) = real(psi_lapl);
	  psiIndex++;
	  if (MakeTwoCopies[j]) {
	    psi(psiIndex,i) = imag(psi_val);
	    for (int n=0; n<OHMMS_DIM; n++)
	      dpsi(i,psiIndex)[n] = imag(psi_grad[n]);
	    d2psi(i,psiIndex) = imag(psi_lapl);
	    psiIndex++;
	  }
	}
      }
      else { // No blending needed
	for (int j=0; j<N; j++) {
	  complex<double> psi_val, psi_lapl;
	  TinyVector<complex<double>, OHMMS_DIM> psi_grad;
	  psi_val  = StorageValueVector[j];
	  psi_grad = StorageGradVector[j];
	  psi_lapl = StorageLaplVector[j];
	  
	  psi(psiIndex,i) = real(psi_val);
	  for (int n=0; n<OHMMS_DIM; n++)
	    dpsi(i,psiIndex)[n] = real(psi_grad[n]);
	  d2psi(i,psiIndex) = real(psi_lapl);
	  psiIndex++;
	  // if (psiIndex >= dpsi.cols()) {
	  //   cerr << "Error:  out of bounds writing in EinsplineSet::evalate.\n"
	  // 	 << "psiIndex = " << psiIndex << "  dpsi.cols() = " << dpsi.cols() << endl;
	  // }
	  if (MakeTwoCopies[j]) {
	    psi(psiIndex,i) = imag(psi_val);
	    for (int n=0; n<OHMMS_DIM; n++)
	      dpsi(i,psiIndex)[n] = imag(psi_grad[n]);
	    d2psi(i,psiIndex) = imag(psi_lapl);
	    psiIndex++;
	  }
	}
      }
    }
    VGLMatTimer.stop();
  }
  
  
  
  template<typename StorageType> void
  EinsplineSetExtended<StorageType>::evaluate
  (const ParticleSet& P, int first, int last, ComplexValueMatrix_t& psi, 
   ComplexGradMatrix_t& dpsi, ComplexValueMatrix_t& d2psi)
  {
    VGLMatTimer.start();
    for(int iat=first,i=0; iat<last; iat++,i++) {
      PosType r (P.R[iat]);
      PosType ru(PrimLattice.toUnit(P.R[iat]));
      for (int n=0; n<OHMMS_DIM; n++)
	ru[n] -= std::floor (ru[n]);
      EinsplineTimer.start();
      EinsplineMultiEval (MultiSpline, ru, StorageValueVector,
			  StorageGradVector, StorageHessVector);
      EinsplineTimer.stop();
      //computePhaseFactors(r);
      complex<double> eye (0.0, 1.0);
      for (int j=0; j<OrbitalSetSize; j++) {
	complex<double> u, laplu;
	TinyVector<complex<double>, OHMMS_DIM> gradu;
	u = StorageValueVector[j];
	gradu = dot(PrimLattice.G, StorageGradVector[j]);
	laplu = trace(StorageHessVector[j], GGt);
	
	PosType k = kPoints[j];
	TinyVector<complex<double>,OHMMS_DIM> ck;
	for (int n=0; n<OHMMS_DIM; n++)	
	  ck[n] = k[n];
	double s,c;
	double phase = -dot(r, k);
	sincos (phase, &s, &c);
	complex<double> e_mikr (c,s);
	convert(e_mikr * u, psi(j,i));
	convertVec(e_mikr*(-eye*u*ck + gradu), dpsi(i,j));
	convert(e_mikr*(-dot(k,k)*u - 2.0*eye*dot(ck,gradu) + laplu), d2psi(i,j));
      } 
    }
    VGLMatTimer.stop();
  }
  
  
  
  template<> void
  EinsplineSetExtended<double>::evaluate
  (const ParticleSet& P, int first, int last, RealValueMatrix_t& psi, 
   RealGradMatrix_t& dpsi, RealValueMatrix_t& d2psi)
  {
    VGLMatTimer.start();
    for(int iat=first,i=0; iat<last; iat++,i++) {
      PosType r (P.R[iat]);
      bool inAtom = false;
      for (int jat=0; jat<AtomicOrbitals.size(); jat++) {
	inAtom = AtomicOrbitals[jat].evaluate 
	  (r, StorageValueVector, StorageGradVector, StorageLaplVector);
	if (inAtom) {
	  for (int j=0; j<OrbitalSetSize; j++) {
	    psi(j,i)   = StorageValueVector[j];
	    dpsi(i,j)  = StorageGradVector[j];
	    d2psi(i,j) = StorageLaplVector[j];
	  }  
	  break;
	}
      }
      if (!inAtom) {
	PosType ru(PrimLattice.toUnit(P.R[iat]));
	int sign=0;
	for (int n=0; n<OHMMS_DIM; n++) {
	  RealType img = std::floor(ru[n]);
	ru[n] -= img;
	sign += HalfG[n] * (int)img;
	}
	for (int n=0; n<OHMMS_DIM; n++)
	  ru[n] -= std::floor (ru[n]);
	EinsplineTimer.start();
	EinsplineMultiEval (MultiSpline, ru, StorageValueVector,
			    StorageGradVector, StorageHessVector);
	EinsplineTimer.stop();
	
	if (sign & 1) 
	  for (int j=0; j<OrbitalSetSize; j++) {
	    StorageValueVector[j] *= -1.0;
	    StorageGradVector[j]  *= -1.0;
	    StorageHessVector[j]  *= -1.0;
	  }
	for (int j=0; j<OrbitalSetSize; j++) {
	  psi(j,i)   = StorageValueVector[j];
	  dpsi(i,j)  = dot(PrimLattice.G, StorageGradVector[j]);
	  d2psi(i,j) = trace(StorageHessVector[j], GGt);
	}
      }
    }
    VGLMatTimer.stop();
  }
  

  //////////////////////////////////////////////
  // Vectorized evaluation routines using GPU //
  //////////////////////////////////////////////

  template<> void 
  EinsplineSetExtended<double>::evaluate 
  (vector<Walker_t*> &walkers, int iat,
   gpu::device_vector<CudaRealType*> &phi)
  {
    // app_log() << "Start EinsplineSet CUDA evaluation\n";
    int N = walkers.size();
    CudaRealType plus_minus[2] = {1.0, -1.0};
    if (cudapos.size() < N) {
      hostPos.resize(N);
      cudapos.resize(N);
      hostSign.resize(N);
      cudaSign.resize(N);
    }
    for (int iw=0; iw < N; iw++) {
      PosType r = walkers[iw]->R[iat];
      PosType ru(PrimLattice.toUnit(r));
      int image[OHMMS_DIM];
      for (int i=0; i<OHMMS_DIM; i++) {
	RealType img = std::floor(ru[i]);
	ru[i] -= img;
	image[i] = (int) img;
      }
      int sign = 0;
      for (int i=0; i<OHMMS_DIM; i++) 
	sign += HalfG[i] * image[i];
      
      hostSign[iw] = plus_minus[sign&1];
      hostPos[iw] = ru;
    }

    cudapos = hostPos;
    cudaSign = hostSign;
    eval_multi_multi_UBspline_3d_cuda 
      (CudaMultiSpline, (CudaRealType*)(cudapos.data()), cudaSign.data(), phi.data(), N);
  }

  template<> void 
  EinsplineSetExtended<complex<double> >::evaluate 
  (vector<Walker_t*> &walkers, int iat,
   gpu::device_vector<CudaRealType*> &phi)
  {
    //    app_log() << "Eval 1.\n";
    int N = walkers.size();

    if (CudaValuePointers.size() < N)
      resize_cuda(N);

    if (cudapos.size() < N) {
      hostPos.resize(N);
      cudapos.resize(N);
    }
    for (int iw=0; iw < N; iw++) {
      PosType r = walkers[iw]->R[iat];
      PosType ru(PrimLattice.toUnit(r));
      ru[0] -= std::floor (ru[0]);
      ru[1] -= std::floor (ru[1]);
      ru[2] -= std::floor (ru[2]);
      hostPos[iw] = ru;
    }

    cudapos = hostPos;

    eval_multi_multi_UBspline_3d_cuda 
      (CudaMultiSpline, (float*)cudapos.data(), CudaValuePointers.data(), N);

    // Now, add on phases
    for (int iw=0; iw < N; iw++) 
      hostPos[iw] = walkers[iw]->R[iat];
    cudapos = hostPos;

    apply_phase_factors ((CUDA_PRECISION*) CudakPoints.data(),
			 CudaMakeTwoCopies.data(),
			 (CUDA_PRECISION*)cudapos.data(),
			 (CUDA_PRECISION**)CudaValuePointers.data(),
			 phi.data(), CudaMultiSpline->num_splines, 
			 walkers.size());
  }


  template<> void 
  EinsplineSetExtended<double>::evaluate 
  (vector<Walker_t*> &walkers, vector<PosType> &newpos,
   gpu::device_vector<CudaRealType*> &phi)
  {
    // app_log() << "Start EinsplineSet CUDA evaluation\n";
    int N = newpos.size();
    CudaRealType plus_minus[2] = {1.0, -1.0};
    
    if (cudapos.size() < N) {
      hostPos.resize(N);
      cudapos.resize(N);
      hostSign.resize(N);
      cudaSign.resize(N);
    }

    for (int iw=0; iw < N; iw++) {
      PosType r = newpos[iw];
      PosType ru(PrimLattice.toUnit(r));
      int image[OHMMS_DIM];
      for (int i=0; i<OHMMS_DIM; i++) {
	RealType img = std::floor(ru[i]);
	ru[i] -= img;
	image[i] = (int) img;
      }
      int sign = 0;
      for (int i=0; i<OHMMS_DIM; i++) 
	sign += HalfG[i] * image[i];
      
      hostSign[iw] = plus_minus[sign&1];
      hostPos[iw] = ru;
    }

    cudapos = hostPos;
    cudaSign = hostSign;
    eval_multi_multi_UBspline_3d_cuda 
      (CudaMultiSpline, (CudaRealType*)(cudapos.data()), cudaSign.data(), 
       phi.data(), N);
    //app_log() << "End EinsplineSet CUDA evaluation\n";
  }

  template<typename T> void
  EinsplineSetExtended<T>::resize_cuda(int numWalkers)
  {
    CudaValuePointers.resize(numWalkers);
    CudaGradLaplPointers.resize(numWalkers);
    int N = CudaMultiSpline->num_splines;
    CudaValueVector.resize(N*numWalkers);
    CudaGradLaplVector.resize(4*N*numWalkers);
    gpu::host_vector<CudaStorageType*> hostValuePointers(numWalkers);
    gpu::host_vector<CudaStorageType*> hostGradLaplPointers(numWalkers);
    for (int i=0; i<numWalkers; i++) {
      hostValuePointers[i]    = &(CudaValueVector.data()[i*N]);
      hostGradLaplPointers[i] = &(CudaGradLaplVector.data()[4*i*N]);
    }
    CudaValuePointers    = hostValuePointers;
    CudaGradLaplPointers = hostGradLaplPointers;

    int M = MakeTwoCopies.size();
    CudaMakeTwoCopies.resize(M);
    gpu::host_vector<int> hostMakeTwoCopies(M);
    for (int i=0; i<M; i++)
      hostMakeTwoCopies[i] = MakeTwoCopies[i];
    CudaMakeTwoCopies = hostMakeTwoCopies;

    CudakPoints.resize(M);
    CudakPoints_reduced.resize(M);
    gpu::host_vector<TinyVector<CUDA_PRECISION,OHMMS_DIM> > hostkPoints(M),
      hostkPoints_reduced(M);
    for (int i=0; i<M; i++) {
      PosType k_red = PrimLattice.toCart(kPoints[i]);
      for (int j=0; j<OHMMS_DIM; j++) {
	hostkPoints[i][j]         = kPoints[i][j];
	hostkPoints_reduced[i][j] = k_red[j];
      }
    }
    CudakPoints = hostkPoints;
    CudakPoints_reduced = hostkPoints_reduced;
  }

  template<> void 
  EinsplineSetExtended<complex<double> >::evaluate 
  (vector<Walker_t*> &walkers, vector<PosType> &newpos,
   gpu::device_vector<CudaRealType*> &phi)
  {
    //    app_log() << "Eval 2.\n";
    int N = walkers.size();
    if (CudaValuePointers.size() < N)
      resize_cuda(N);

    if (cudapos.size() < N) {
      hostPos.resize(N);
      cudapos.resize(N);
    }
    for (int iw=0; iw < N; iw++) {
      PosType r = newpos[iw];
      PosType ru(PrimLattice.toUnit(r));
      ru[0] -= std::floor (ru[0]);
      ru[1] -= std::floor (ru[1]);
      ru[2] -= std::floor (ru[2]);
      hostPos[iw] = ru;
    }

    cudapos = hostPos;
    
    eval_multi_multi_UBspline_3d_cuda 
      (CudaMultiSpline, (float*)cudapos.data(), CudaValuePointers.data(), N);
    
    // Now, add on phases
    for (int iw=0; iw < N; iw++) 
      hostPos[iw] = newpos[iw];
    cudapos = hostPos;
    
    apply_phase_factors ((CUDA_PRECISION*) CudakPoints.data(),
			 CudaMakeTwoCopies.data(),
			 (CUDA_PRECISION*)cudapos.data(),
			 (CUDA_PRECISION**)CudaValuePointers.data(),
			 phi.data(), CudaMultiSpline->num_splines, 
			 walkers.size());

  }

  template<> void
  EinsplineSetExtended<double>::evaluate
  (vector<Walker_t*> &walkers, vector<PosType> &newpos, 
   gpu::device_vector<CudaRealType*> &phi, gpu::device_vector<CudaRealType*> &grad_lapl,
   int row_stride)
  {
    int N = walkers.size();
    CudaRealType plus_minus[2] = {1.0, -1.0};
    if (cudapos.size() < N) {
      hostPos.resize(N);
      cudapos.resize(N);
      hostSign.resize(N);
      cudaSign.resize(N);
    }
    for (int iw=0; iw < N; iw++) {
      PosType r = newpos[iw];
      PosType ru(PrimLattice.toUnit(r));
      int image[OHMMS_DIM];
      for (int i=0; i<OHMMS_DIM; i++) {
	RealType img = std::floor(ru[i]);
	ru[i] -= img;
	image[i] = (int) img;
      }
      int sign = 0;
      for (int i=0; i<OHMMS_DIM; i++) 
	sign += HalfG[i] * image[i];
      
      hostSign[iw] = plus_minus[sign&1];
      hostPos[iw] = ru;
    }
    
    cudapos = hostPos;
    cudaSign = hostSign;

    eval_multi_multi_UBspline_3d_vgl_cuda
      (CudaMultiSpline, (CudaRealType*)cudapos.data(), cudaSign.data(), 
       Linv_cuda.data(), phi.data(), grad_lapl.data(), N, row_stride);

    // gpu::host_vector<CudaRealType*> pointers;
    // pointers = phi;
    // CudaRealType data[N];
    // cudaMemcpy (data, pointers[0], N*sizeof(CudaRealType), cudaMemcpyDeviceToHost);
    // for (int i=0; i<N; i++)
    //   fprintf (stderr, "%1.12e\n", data[i]);
  }


  template<> void
  EinsplineSetExtended<complex<double> >::evaluate
  (vector<Walker_t*> &walkers, vector<PosType> &newpos, 
   gpu::device_vector<CudaRealType*> &phi, gpu::device_vector<CudaRealType*> &grad_lapl,
   int row_stride)
  {
    //    app_log() << "Eval 3.\n";
    int N = walkers.size();
    int M = CudaMultiSpline->num_splines;

    if (CudaValuePointers.size() < N)
      resize_cuda(N);

    if (cudapos.size() < N) {
      hostPos.resize(N);
      cudapos.resize(N);
    }
    for (int iw=0; iw < N; iw++) {
      PosType r = newpos[iw];
      PosType ru(PrimLattice.toUnit(r));
      ru[0] -= std::floor (ru[0]);
      ru[1] -= std::floor (ru[1]);
      ru[2] -= std::floor (ru[2]);
      hostPos[iw] = ru;
    }

    cudapos = hostPos;

    eval_multi_multi_UBspline_3d_c_vgl_cuda
      (CudaMultiSpline, (float*)cudapos.data(),  Linv_cuda.data(), CudaValuePointers.data(), 
       CudaGradLaplPointers.data(), N, CudaMultiSpline->num_splines);


    // DEBUG
  //   TinyVector<double,OHMMS_DIM> r(hostPos[0][0], hostPos[0][1], hostPos[0][2]);
  //   Vector<complex<double > > psi(M);
  //   Vector<TinyVector<complex<double>,3> > grad(M);
  //   Vector<Tensor<complex<double>,3> > hess(M);
  //   EinsplineMultiEval (MultiSpline, r, psi, grad, hess);
      
  //   // complex<double> cpuSpline[M];
  //   // TinyVector<complex<double>,OHMMS_DIM> complex<double> cpuGrad[M];
  //   // Tensor cpuHess[M];
  //   // eval_multi_UBspline_3d_z_vgh (MultiSpline, hostPos[0][0], hostPos[0][1], hostPos[0][2],
  //   // 				  cpuSpline);

  //   gpu::host_vector<CudaStorageType*> pointers;
  //   pointers = CudaGradLaplPointers;
  //   complex<float> gpuSpline[4*M];
  //   cudaMemcpy(gpuSpline, pointers[0], 
  // 	       4*M * sizeof(complex<float>), cudaMemcpyDeviceToHost);

  // for (int i=0; i<M; i++)
  //   fprintf (stderr, "%10.6f %10.6f   %10.6f %10.6f\n",
  // 	     trace(hess[i],GGt).real(), gpuSpline[3*M+i].real(),
  // 	     trace(hess[i], GGt).imag(), gpuSpline[3*M+i].imag());

    // Now, add on phases
    for (int iw=0; iw < N; iw++) 
      hostPos[iw] = newpos[iw];
    cudapos = hostPos;
    
    apply_phase_factors ((CUDA_PRECISION*) CudakPoints.data(),
			 CudaMakeTwoCopies.data(),
			 (CUDA_PRECISION*)cudapos.data(),
			 (CUDA_PRECISION**)CudaValuePointers.data(), phi.data(), 
			 (CUDA_PRECISION**)CudaGradLaplPointers.data(), grad_lapl.data(),
			 CudaMultiSpline->num_splines,  walkers.size(), row_stride);
  }


  template<> void 
  EinsplineSetExtended<double>::evaluate 
  (vector<PosType> &pos, gpu::device_vector<CudaRealType*> &phi)
  { 
    int N = pos.size();
    CudaRealType plus_minus[2] = {1.0, -1.0};

    if (cudapos.size() < N) {
      NLhostPos.resize(N);
      NLcudapos.resize(N);
      NLhostSign.resize(N);
      NLcudaSign.resize(N);
    }
    for (int iw=0; iw < N; iw++) {
      PosType r = pos[iw];
      PosType ru(PrimLattice.toUnit(r));
      int image[OHMMS_DIM];
      for (int i=0; i<OHMMS_DIM; i++) {
	RealType img = std::floor(ru[i]);
	ru[i] -= img;
	image[i] = (int) img;
      }
      int sign = 0;
      for (int i=0; i<OHMMS_DIM; i++) 
	sign += HalfG[i] * image[i];
      
      NLhostSign[iw] = plus_minus[sign&1];

      NLhostPos[iw] = ru;
    }

    NLcudapos  = NLhostPos;
    NLcudaSign = NLhostSign;
    eval_multi_multi_UBspline_3d_cuda 
      (CudaMultiSpline, (CudaRealType*)(NLcudapos.data()), 
       NLcudaSign.data(), phi.data(), N);    
  }

  template<> void 
  EinsplineSetExtended<double>::evaluate 
  (vector<PosType> &pos, gpu::device_vector<CudaComplexType*> &phi)
  { 
    app_error() << "EinsplineSetExtended<complex<double> >::evaluate "
		<< "not yet implemented.\n";
    abort();
  }



  template<> void 
  EinsplineSetExtended<complex<double> >::evaluate 
  (vector<PosType> &pos, gpu::device_vector<CudaRealType*> &phi)
  { 
    //    app_log() << "Eval 4.\n";
    int N = pos.size();

    if (CudaValuePointers.size() < N)
      resize_cuda(N);

    if (cudapos.size() < N) {
      hostPos.resize(N);
      cudapos.resize(N);
    }
    for (int iw=0; iw < N; iw++) {
      PosType r = pos[iw];
      PosType ru(PrimLattice.toUnit(r));
      ru[0] -= std::floor (ru[0]);
      ru[1] -= std::floor (ru[1]);
      ru[2] -= std::floor (ru[2]);
      hostPos[iw] = ru;
    }

    cudapos = hostPos;
    eval_multi_multi_UBspline_3d_cuda 
      (CudaMultiSpline, (CUDA_PRECISION*) cudapos.data(), 
       CudaValuePointers.data(), N);
    
    // Now, add on phases
    for (int iw=0; iw < N; iw++) 
      hostPos[iw] = pos[iw];
    cudapos = hostPos;
    
    apply_phase_factors ((CUDA_PRECISION*) CudakPoints.data(),
			 CudaMakeTwoCopies.data(),
			 (CUDA_PRECISION*)cudapos.data(),
			 (CUDA_PRECISION**)CudaValuePointers.data(),
			 phi.data(), CudaMultiSpline->num_splines, N);
  }

  template<> void 
  EinsplineSetExtended<complex<double> >::evaluate 
  (vector<PosType> &pos, gpu::device_vector<CudaComplexType*> &phi)
  { 
    app_error() << "EinsplineSetExtended<complex<double> >::evaluate "
		<< "not yet implemented.\n";
    abort();
  }




  template<typename StorageType> string
  EinsplineSetExtended<StorageType>::Type()
  {
    return "EinsplineSetExtended";
  }


  template<typename StorageType> void
  EinsplineSetExtended<StorageType>::registerTimers()
  {
    ValueTimer.reset();
    VGLTimer.reset();
    VGLMatTimer.reset();
    EinsplineTimer.reset();
    TimerManager.addTimer (&ValueTimer);
    TimerManager.addTimer (&VGLTimer);
    TimerManager.addTimer (&VGLMatTimer);
    TimerManager.addTimer (&EinsplineTimer);
  }


  



  SPOSetBase*
  EinsplineSetLocal::makeClone() const 
  {
    return new EinsplineSetLocal(*this);
  }

  void
  EinsplineSetLocal::resetParameters(const opt_variables_type& active)
  {
  }

  template<typename StorageType> SPOSetBase*
  EinsplineSetExtended<StorageType>::makeClone() const
  {
    EinsplineSetExtended<StorageType> *clone = 
      new EinsplineSetExtended<StorageType> (*this);
    clone->registerTimers();
    for (int iat=0; iat<clone->AtomicOrbitals.size(); iat++)
      clone->AtomicOrbitals[iat].registerTimers();
    return clone;
  }

  template class EinsplineSetExtended<complex<double> >;
  template class EinsplineSetExtended<        double  >;


  
  ////////////////////////////////
  // Hybrid evaluation routines //
  ////////////////////////////////
  // template<typename StorageType> void
  // EinsplineSetHybrid<StorageType>::sort_electrons(vector<PosType> &pos)
  // {
  //   int nw = pos.size();
  //   if (nw > CurrentWalkers)
  //     resize_cuda(nw);

  //   AtomicPolyJobs_CPU.clear();
  //   AtomicSplineJobs_CPU.clear();
  //   rhats_CPU.clear();
  //   int numAtomic = 0;

  //   // First, sort electrons into three categories:
  //   // 1) Interstitial region with 3D-Bsplines
  //   // 2) Atomic region near origin:      polynomial  radial functions
  //   // 3) Atomic region not near origin:  1D B-spline radial functions

  //   for (int i=0; i<newpos.size(); i++) {
  //     PosType r = newpos[i];
  //     // Note: this assumes that the atomic radii are smaller than the simulation cell radius.
  //     for (int j=0; j<AtomicOrbitals.size(); j++) {
  // 	AtomicOrbital<complex<double> > &orb = AtomicOrbitals[j];
  // 	PosType dr = r - orb.Pos;
  // 	PosType u = PrimLattice.toUnit(dr);
  // 	for (int k=0; k<OHMMS_DIM; k++) 
  // 	  u[k] -= round(u[k]);
  // 	dr = PrimLattice.toCart(u);
  // 	RealType dist2 = dot (dr,dr);
  // 	if (dist2 < orb.PolyRadius * orb.PolyRadius) {
  // 	  AtomicPolyJob<CudaRealType> job;
  // 	  RealType dist = std::sqrt(dist2);
  // 	  job.dist = dist;
  // 	  RealType distInv = 1.0/dist;
  // 	  for (int k=0; k<OHMMS_DIM; k++) {
  // 	    CudaRealType x = distInv*dr[k];
  // 	    job.rhat[k] = distInv * dr[k];
  // 	    rhats_CPU.push_back(x);
  // 	  }
  // 	  job.lMax = orb.lMax;
  // 	  job.YlmIndex  = i;
  // 	  job.PolyOrder = orb.PolyOrder;
  // 	  //job.PolyCoefs = orb.PolyCoefs;
  // 	  AtomicPolyJobs_CPU.push_back(job);
  // 	  numAtomic++;
  // 	}
  // 	else if (dist2 < orb.CutoffRadius*orb.CutoffRadius) {
  // 	  AtomicSplineJob<CudaRealType> job;
  // 	   RealType dist = std::sqrt(dist2);
  // 	  job.dist = dist;
  // 	  RealType distInv = 1.0/dist;
  // 	  for (int k=0; k<OHMMS_DIM; k++) {
  // 	    CudaRealType x = distInv*dr[k];
  // 	    job.rhat[k] = distInv * dr[k];
  // 	    rhats_CPU.push_back(x);
  // 	  }
  // 	  job.lMax      = orb.lMax;
  // 	  job.YlmIndex  = i;
  // 	  job.phi       = phi[i];
  // 	  job.grad_lapl = grad_lapl[i];
  // 	  //job.PolyCoefs = orb.PolyCoefs;
  // 	  AtomicSplineJobs_CPU.push_back(job);
  // 	  numAtomic++;
  // 	}
  // 	else { // Regular 3D B-spline job
  // 	  BsplinePos_CPU.push_back (r);
	  
  // 	}
  //     }
  //   }
    

  // }


  ///////////////////////////////
  // Real StorageType versions //
  ///////////////////////////////

  template<> void
  EinsplineSetHybrid<double>::resize_cuda(int numwalkers)
  {
    EinsplineSetExtended<double>::resize_cuda(numwalkers);
    CurrentWalkers = numwalkers;

    // Resize Ylm temporaries
    // Find lMax;
    lMax=-1;
    for (int i=0; i<AtomicOrbitals.size(); i++) 
      lMax = max(AtomicOrbitals[i].lMax, lMax);
    
    numlm = (lMax+1)*(lMax+1);
    Ylm_BS = ((numlm+15)/16) * 16;
    
    Ylm_GPU.resize(numwalkers*Ylm_BS*3);
    Ylm_ptr_GPU.resize        (numwalkers);   Ylm_ptr_CPU.resize       (numwalkers);
    dYlm_dtheta_ptr_GPU.resize(numwalkers);  dYlm_dtheta_ptr_CPU.resize(numwalkers);
    dYlm_dphi_ptr_GPU.resize  (numwalkers);  dYlm_dphi_ptr_CPU.resize  (numwalkers);

    rhats_CPU.resize(OHMMS_DIM*numwalkers);
    rhats_GPU.resize(OHMMS_DIM*numwalkers);
    HybridJobs_GPU.resize(numwalkers);
    HybridData_GPU.resize(numwalkers);

    for (int iw=0; iw<numwalkers; iw++) {
      Ylm_ptr_CPU[iw]         = Ylm_GPU.data() + (3*iw+0)*Ylm_BS;
      dYlm_dtheta_ptr_CPU[iw] = Ylm_GPU.data() + (3*iw+1)*Ylm_BS;
      dYlm_dphi_ptr_CPU[iw]   = Ylm_GPU.data() + (3*iw+2)*Ylm_BS;
    }
    
    Ylm_ptr_GPU         = Ylm_ptr_CPU;
    dYlm_dtheta_ptr_GPU = dYlm_dtheta_ptr_CPU;
    dYlm_dphi_ptr_GPU   = dYlm_dphi_ptr_CPU;
    
    // Resize AtomicJob temporaries
    
    // AtomicPolyJobs_GPU.resize(numwalkers);
    // AtomicSplineJobs_GPU.resize(numwalkers);
  }


  template<> void
  EinsplineSetHybrid<complex<double> >::resize_cuda(int numwalkers)
  {
    EinsplineSetExtended<complex<double> >::resize_cuda(numwalkers);
    CurrentWalkers = numwalkers;

    // Resize Ylm temporaries
    // Find lMax;
    lMax=-1;
    for (int i=0; i<AtomicOrbitals.size(); i++) 
      lMax = max(AtomicOrbitals[i].lMax, lMax);
    
    numlm = (lMax+1)*(lMax+1);
    Ylm_BS = ((2*numlm+15)/16) * 16;
    
    Ylm_GPU.resize(numwalkers*Ylm_BS*3);
    Ylm_ptr_GPU.resize        (numwalkers);   Ylm_ptr_CPU.resize       (numwalkers);
    dYlm_dtheta_ptr_GPU.resize(numwalkers);  dYlm_dtheta_ptr_CPU.resize(numwalkers);
    dYlm_dphi_ptr_GPU.resize  (numwalkers);  dYlm_dphi_ptr_CPU.resize  (numwalkers);
    rhats_CPU.resize(OHMMS_DIM*numwalkers);
    rhats_GPU.resize(OHMMS_DIM*numwalkers);
    HybridJobs_GPU.resize(numwalkers);
    HybridData_GPU.resize(numwalkers);

    for (int iw=0; iw<numwalkers; iw++) {
      Ylm_ptr_CPU[iw]         = Ylm_GPU.data() + (3*iw+0)*Ylm_BS;
      dYlm_dtheta_ptr_CPU[iw] = Ylm_GPU.data() + (3*iw+1)*Ylm_BS;
      dYlm_dphi_ptr_CPU[iw]   = Ylm_GPU.data() + (3*iw+2)*Ylm_BS;
    }
    
    Ylm_ptr_GPU         = Ylm_ptr_CPU;
    dYlm_dtheta_ptr_GPU = dYlm_dtheta_ptr_CPU;
    dYlm_dphi_ptr_GPU   = dYlm_dphi_ptr_CPU;

    // Resize AtomicJob temporaries
    // AtomicPolyJobs_GPU.resize(numwalkers);
    // AtomicSplineJobs_GPU.resize(numwalkers);
  }


  // Vectorized evaluation functions
  template<> void
  EinsplineSetHybrid<double>::evaluate (vector<Walker_t*> &walkers, int iat,
					gpu::device_vector<CudaRealType*> &phi)
  {
    app_error() << "EinsplineSetHybrid<double>::evaluate (vector<Walker_t*> &walkers, int iat,\n"
		<< " gpu::device_vector<CudaRealType*> &phi) not implemented.\n";
    abort();
  }

  
  template<> void
  EinsplineSetHybrid<double>::evaluate (vector<Walker_t*> &walkers, int iat,
				gpu::device_vector<CudaComplexType*> &phi)
  {
    app_error() << "EinsplineSetHybrid<double>::evaluate (vector<Walker_t*> &walkers, int iat,\n"
		<< " gpu::device_vector<CudaComplexType*> &phi) not implemented.\n";
    abort();
  }

  
  template<> void
  EinsplineSetHybrid<double>::evaluate (vector<Walker_t*> &walkers, vector<PosType> &newpos, 
					gpu::device_vector<CudaRealType*> &phi)
  { 
    app_error() << "EinsplineSetHybrid<double>::evaluate \n"
		<< " (vector<Walker_t*> &walkers, vector<PosType> &newpos, \n"
		<< " gpu::device_vector<CudaRealType*> &phi) not implemented.\n";
    abort();
  }

  template<> void
  EinsplineSetHybrid<double>::evaluate (vector<Walker_t*> &walkers, vector<PosType> &newpos,
  		 gpu::device_vector<CudaComplexType*> &phi)
  {
    app_error() << "EinsplineSetHybrid<double>::evaluate \n"
		<< "  (vector<Walker_t*> &walkers, vector<PosType> &newpos,\n"
		<< "   gpu::device_vector<CudaComplexType*> &phi) not implemented.\n";
    abort();
  }

  

  template<> void
  EinsplineSetHybrid<double>::evaluate (vector<Walker_t*> &walkers, vector<PosType> &newpos, 
					gpu::device_vector<CudaRealType*> &phi,
					gpu::device_vector<CudaRealType*> &grad_lapl,
					int row_stride)
  { 
    int N = newpos.size();
    if (cudapos.size() < N) {
      resize_cuda(N);
      hostPos.resize(N);
      cudapos.resize(N);
    }
    for (int iw=0; iw < N; iw++) 
      hostPos[iw] = newpos[iw];
    cudapos = hostPos;
    
    // hostPos = cudapos;
    // for (int i=0; i<newpos.size(); i++)
    //   cerr << "newPos[" << i << "] = " << newpos[i] << endl;

    // gpu::host_vector<CudaRealType> IonPos_CPU(IonPos_GPU.size());
    // IonPos_CPU = IonPos_GPU;
    // for (int i=0; i<IonPos_CPU.size()/3; i++)
    //   fprintf (stderr, "ion[%d] = [%10.6f %10.6f %10.6f]\n",
    // 	       i, IonPos_CPU[3*i+0], IonPos_CPU[3*i+2], IonPos_CPU[3*i+2]);
    // cerr << "cudapos.size()        = " << cudapos.size() << endl;
    // cerr << "IonPos.size()         = " << IonPos_GPU.size() << endl;
    // cerr << "PolyRadii.size()      = " << PolyRadii_GPU.size() << endl;
    // cerr << "CutoffRadii.size()    = " << CutoffRadii_GPU.size() << endl;
    // cerr << "AtomicOrbitals.size() = " << AtomicOrbitals.size() << endl;
    // cerr << "L_cuda.size()         = " << L_cuda.size() << endl;
    // cerr << "Linv_cuda.size()      = " << Linv_cuda.size() << endl;
    // cerr << "HybridJobs_GPU.size() = " << HybridJobs_GPU.size() << endl;
    // cerr << "rhats_GPU.size()      = " << rhats_GPU.size() << endl;

    MakeHybridJobList ((float*) cudapos.data(), N, IonPos_GPU.data(), 
		       PolyRadii_GPU.data(), CutoffRadii_GPU.data(),
		       AtomicOrbitals.size(), L_cuda.data(), Linv_cuda.data(),
		       HybridJobs_GPU.data(), rhats_GPU.data(),
		       HybridData_GPU.data());

    CalcYlmRealCuda (rhats_GPU.data(), HybridJobs_GPU.data(),
		     Ylm_ptr_GPU.data(), dYlm_dtheta_ptr_GPU.data(), 
		     dYlm_dphi_ptr_GPU.data(), lMax, newpos.size());

    evaluate3DSplineReal (HybridJobs_GPU.data(), (float*)cudapos.data(), 
			  (CudaRealType*)CudakPoints_reduced.data(),CudaMultiSpline,
			  Linv_cuda.data(), phi.data(), grad_lapl.data(), 
			  row_stride, NumOrbitals, newpos.size());
    evaluateHybridSplineReal (HybridJobs_GPU.data(), rhats_GPU.data(), Ylm_ptr_GPU.data(),
			      dYlm_dtheta_ptr_GPU.data(), dYlm_dphi_ptr_GPU.data(),
			      AtomicOrbitals_GPU.data(), HybridData_GPU.data(),
			      (CudaRealType*)CudakPoints_reduced.data(), 
			      phi.data(), grad_lapl.data(), 
			      row_stride, NumOrbitals, newpos.size(), lMax);	

#ifdef HYBRID_DEBUG


    gpu::host_vector<CudaRealType*> phi_CPU (phi.size()), grad_lapl_CPU(phi.size());
    phi_CPU = phi;
    grad_lapl_CPU = grad_lapl;
    gpu::host_vector<CudaRealType> vals_CPU(NumOrbitals), GL_CPU(4*row_stride);
    gpu::host_vector<HybridJobType> HybridJobs_CPU(HybridJobs_GPU.size());
    HybridJobs_CPU = HybridJobs_GPU;
    gpu::host_vector<HybridDataFloat> HybridData_CPU(HybridData_GPU.size());
    HybridData_CPU = HybridData_GPU;
    
    rhats_CPU = rhats_GPU;
    
    for (int iw=0; iw<newpos.size(); iw++) 
      if (false && HybridJobs_CPU[iw] == ATOMIC_POLY_JOB) {
	ValueVector_t CPUvals(NumOrbitals), CPUlapl(NumOrbitals);
	GradVector_t CPUgrad(NumOrbitals);
	HybridDataFloat &d = HybridData_CPU[iw];
	AtomicOrbital<double> &atom = AtomicOrbitals[d.ion];
	atom.evaluate (newpos[iw], CPUvals, CPUgrad, CPUlapl);

	cudaMemcpy (&vals_CPU[0], phi_CPU[iw], NumOrbitals*sizeof(float),
		    cudaMemcpyDeviceToHost);
	cudaMemcpy (&GL_CPU[0], grad_lapl_CPU[iw], 4*row_stride*sizeof(float),
		    cudaMemcpyDeviceToHost);
	// fprintf (stderr, " %d %2.0f %2.0f %2.0f  %8.5f  %d %d\n",
	// 	 iw, d.img[0], d.img[1], d.img[2], d.dist, d.ion, d.lMax);
	double mindist = 1.0e5;
	for (int ion=0; ion<AtomicOrbitals.size(); ion++) {
	  PosType disp = newpos[iw] - AtomicOrbitals[ion].Pos;
	  PosType u = PrimLattice.toUnit(disp);
	  PosType img;
	  for (int i=0; i<OHMMS_DIM; i++) 
	    u[i] -= round(u[i]);
	  disp = PrimLattice.toCart(u);
	  double dist = std::sqrt(dot(disp,disp));
	  if (dist < AtomicOrbitals[ion].CutoffRadius)
	    mindist = dist;
	}
	if (std::fabs (mindist - d.dist) > 1.0e-3)
	  fprintf (stderr, "CPU dist = %1.8f  GPU dist = %1.8f\n",
		   mindist, d.dist);

	for (int j=0; j<NumOrbitals; j++) {
	  //	  if (isnan(vals_CPU[j])) {
	  if (true || isnan(GL_CPU[0*row_stride+j])) {
	    cerr << "iw = " << iw << endl;
	    fprintf (stderr, "rhat[%d] = [%10.6f %10.6f %10.6f] dist = %10.6e\n",
		     iw, rhats_CPU[3*iw+0], rhats_CPU[3*iw+1], rhats_CPU[3*iw+2], 
		     d.dist);
	    fprintf (stderr, "val[%2d]  = %10.5e %10.5e\n", 
		     j, vals_CPU[j], CPUvals[j]);
	    fprintf (stderr, "grad[%2d] = %10.5e %10.5e  %10.5e %10.5e  %10.5e %10.5e\n", j,
		     GL_CPU[0*row_stride+j], CPUgrad[j][0],
		     GL_CPU[1*row_stride+j], CPUgrad[j][1],
		     GL_CPU[2*row_stride+j], CPUgrad[j][2]);
	    
	    fprintf (stderr, "lapl[%2d] = %10.5e %10.5e\n", 
		     j, GL_CPU[3*row_stride+j], CPUlapl[j]);
	  }
	}
      }
      else if (HybridJobs_CPU[iw] == BSPLINE_3D_JOB) {
	cerr << "HalfG = " << HalfG << endl;
	ValueVector_t CPUvals(NumOrbitals), CPUlapl(NumOrbitals);
	GradVector_t CPUgrad(NumOrbitals);
	PosType ru(PrimLattice.toUnit(newpos[iw]));
	PosType img;
	int sign = 0;
	for (int i=0; i<3; i++) {
	  img[i] = std::floor(ru[i]);
	  ru[i] -= img[i];
	  sign += HalfG[i]*(int)img[i];
	}
	EinsplineMultiEval (MultiSpline, ru, CPUvals, CPUgrad,
	 		    StorageHessVector);

	cudaMemcpy (&vals_CPU[0], phi_CPU[iw], NumOrbitals*sizeof(float),
		    cudaMemcpyDeviceToHost);
	cudaMemcpy (&GL_CPU[0], grad_lapl_CPU[iw], 4*row_stride*sizeof(float),
		    cudaMemcpyDeviceToHost);

	for (int j=0; j<NumOrbitals; j++) {
	  CPUgrad[j] = dot (PrimLattice.G, CPUgrad[j]);
	  CPUlapl[j] = trace (StorageHessVector[j], GGt);

	  if (sign & 1) {
	    CPUvals[j] *= -1.0;
	    CPUgrad[j] *= -1.0;
	    CPUlapl[j] *= -1.0;
	  }

	  fprintf (stderr, "\nGPU=%10.6f  %10.6f %10.6f %10.6f  %10.6f\n",
		   vals_CPU[j], GL_CPU[0*row_stride+j], GL_CPU[1*row_stride+j], 
		   GL_CPU[2*row_stride+j], GL_CPU[3*row_stride+j]);
	  fprintf (stderr, "CPU=%10.6f  %10.6f %10.6f %10.6f  %10.6f sign = %d\n",
		   CPUvals[j], CPUgrad[j][0], CPUgrad[j][1], CPUgrad[j][2],
		   CPUlapl[j], sign);

	  if (isnan(GL_CPU[0*row_stride+j])) {
	    cerr << "r[" << iw << "] = " << newpos[iw] << endl;
	    cerr << "iw = " << iw << endl;

	    fprintf (stderr, "3D val[%2d]  = %10.5e %10.5e\n", 
		     j, vals_CPU[j], CPUvals[j]);
	    fprintf (stderr, "3D grad[%2d] = %10.5e %10.5e  %10.5e %10.5e  %10.5e %10.5e\n", j,
		     GL_CPU[0*row_stride+j], CPUgrad[j][0],
		     GL_CPU[1*row_stride+j], CPUgrad[j][1],
		     GL_CPU[2*row_stride+j], CPUgrad[j][2]);
	    
	    fprintf (stderr, "3D lapl[%2d] = %10.5e %10.5e\n", 
		     j, GL_CPU[3*row_stride+j], CPUlapl[j]);
	  }
	}
      }
  


    gpu::host_vector<float> Ylm_CPU(Ylm_GPU.size());
    Ylm_CPU = Ylm_GPU;
    
    rhats_CPU = rhats_GPU;
    for (int i=0; i<rhats_CPU.size()/3; i++)
      fprintf (stderr, "rhat[%d] = [%10.6f %10.6f %10.6f]\n",
	       i, rhats_CPU[3*i+0], rhats_CPU[3*i+1], rhats_CPU[3*i+2]);

    gpu::host_vector<HybridJobType> HybridJobs_CPU(HybridJobs_GPU.size());
    HybridJobs_CPU = HybridJobs_GPU;
        
    gpu::host_vector<HybridDataFloat> HybridData_CPU(HybridData_GPU.size());
    HybridData_CPU = HybridData_GPU;

    cerr << "Before loop.\n";
    for (int i=0; i<newpos.size(); i++) 
      if (HybridJobs_CPU[i] != BSPLINE_3D_JOB) {
	cerr << "Inside if.\n";
	PosType rhat(rhats_CPU[3*i+0], rhats_CPU[3*i+1], rhats_CPU[3*i+2]);
	AtomicOrbital<double> &atom = AtomicOrbitals[HybridData_CPU[i].ion];
	int numlm = (atom.lMax+1)*(atom.lMax+1);
	vector<double> Ylm(numlm), dYlm_dtheta(numlm), dYlm_dphi(numlm);
	atom.CalcYlm (rhat, Ylm, dYlm_dtheta, dYlm_dphi);
	
	for (int lm=0; lm < numlm; lm++) {
	  fprintf (stderr, "lm=%3d  Ylm_CPU=%8.5f  Ylm_GPU=%8.5f\n",
		   lm, Ylm[lm], Ylm_CPU[3*i*Ylm_BS+lm]);
	}
      }

    fprintf (stderr, " N  img      dist    ion    lMax\n");
    for (int i=0; i<HybridData_CPU.size(); i++) {
      HybridDataFloat &d = HybridData_CPU[i];
      fprintf (stderr, " %d %2.0f %2.0f %2.0f  %8.5f  %d %d\n",
	       i, d.img[0], d.img[1], d.img[2], d.dist, d.ion, d.lMax);
    }
#endif

    // int N = newpos.size();
    // if (N > CurrentWalkers)
    //   resize_cuda(N);

    // AtomicPolyJobs_CPU.clear();
    // AtomicSplineJobs_CPU.clear();
    // rhats_CPU.clear();
    // BsplinePos_CPU.clear();
    // BsplineVals_CPU.clear();
    // BsplineGradLapl_CPU.clear();
    // int numAtomic = 0;

    // // First, sort electrons into three categories:
    // // 1) Interstitial region with 3D B-splines
    // // 2) Atomic region near origin:      polynomial  radial functions
    // // 3) Atomic region not near origin:  1D B-spline radial functions

    // for (int i=0; i<newpos.size(); i++) {
    //   PosType r = newpos[i];
    //   // Note: this assumes that the atomic radii are smaller than the simulation cell radius.
    //   for (int j=0; j<AtomicOrbitals.size(); j++) {
    // 	AtomicOrbital<complex<double> > &orb = AtomicOrbitals[j];
    // 	PosType dr = r - orb.Pos;
    // 	PosType u = PrimLattice.toUnit(dr);
    // 	for (int k=0; k<OHMMS_DIM; k++) 
    // 	  u[k] -= round(u[k]);
    // 	dr = PrimLattice.toCart(u);
    // 	RealType dist2 = dot (dr,dr);
    // 	if (dist2 < orb.PolyRadius * orb.PolyRadius) {
    // 	  AtomicPolyJob<CudaRealType> job;
    // 	  RealType dist = std::sqrt(dist2);
    // 	  job.dist = dist;
    // 	  RealType distInv = 1.0/dist;
    // 	  for (int k=0; k<OHMMS_DIM; k++) {
    // 	    CudaRealType x = distInv*dr[k];
    // 	    job.rhat[k] = distInv * dr[k];
    // 	    rhats_CPU.push_back(x);
    // 	  }
    // 	  job.lMax = orb.lMax;
    // 	  job.YlmIndex  = i;
    // 	  job.PolyOrder = orb.PolyOrder;
    // 	  //job.PolyCoefs = orb.PolyCoefs;
    // 	  AtomicPolyJobs_CPU.push_back(job);
    // 	  numAtomic++;
    // 	}
    // 	else if (dist2 < orb.CutoffRadius*orb.CutoffRadius) {
    // 	  AtomicSplineJob<CudaRealType> job;
    // 	   RealType dist = std::sqrt(dist2);
    // 	  job.dist = dist;
    // 	  RealType distInv = 1.0/dist;
    // 	  for (int k=0; k<OHMMS_DIM; k++) {
    // 	    CudaRealType x = distInv*dr[k];
    // 	    job.rhat[k] = distInv * dr[k];
    // 	    rhats_CPU.push_back(x);
    // 	  }
    // 	  job.lMax      = orb.lMax;
    // 	  job.YlmIndex  = i;
    // 	  job.phi       = phi[i];
    // 	  job.grad_lapl = grad_lapl[i];
    // 	  //job.PolyCoefs = orb.PolyCoefs;
    // 	  AtomicSplineJobs_CPU.push_back(job);
    // 	  numAtomic++;
    // 	}
    // 	else { // Regular 3D B-spline job
    // 	  BsplinePos_CPU
    // 	}
    //   }
    // }

    // //////////////////////////////////
    // // First, evaluate 3D B-splines //
    // //////////////////////////////////
    // int N = newpos.size();
    // CudaRealType plus_minus[2] = {1.0, -1.0};
    
    // if (cudapos.size() < N) {
    //   hostPos.resize(N);
    //   cudapos.resize(N);
    //   hostSign.resize(N);
    //   cudaSign.resize(N);
    // }

    // for (int iw=0; iw < N; iw++) {
    //   PosType r = newpos[iw];
    //   PosType ru(PrimLattice.toUnit(r));
    //   int image[OHMMS_DIM];
    //   for (int i=0; i<OHMMS_DIM; i++) {
    // 	RealType img = std::floor(ru[i]);
    // 	ru[i] -= img;
    // 	image[i] = (int) img;
    //   }
    //   int sign = 0;
    //   for (int i=0; i<OHMMS_DIM; i++) 
    // 	sign += HalfG[i] * image[i];
      
    //   hostSign[iw] = plus_minus[sign&1];
    //   hostPos[iw] = ru;
    // }

    // cudapos = hostPos;
    // cudaSign = hostSign;
    // eval_multi_multi_UBspline_3d_cuda 
    //   (CudaMultiSpline, (CudaRealType*)(cudapos.data()), cudaSign.data(), 
    //    phi.data(), N);

    // ////////////////////////////////////////////////////////////
    // // Next, evaluate spherical harmonics for atomic orbitals //
    // ////////////////////////////////////////////////////////////

    // // Evaluate Ylms
    // if (rhats_CPU.size()) {
    //   rhats_GPU = rhats_CPU;
    //   CalcYlmComplexCuda(rhats_GPU.data(), Ylm_ptr_GPU.data(), dYlm_dtheta_ptr_GPU.data(),
    // 			 dYlm_dphi_ptr_GPU.data(), lMax, numAtomic);
    //   cerr << "Calculated Ylms.\n";
    // }

    // ///////////////////////////////////////
    // // Next, evaluate 1D spline orbitals //
    // ///////////////////////////////////////
    // if (AtomicSplineJobs_CPU.size()) {
    //   AtomicSplineJobs_GPU = AtomicSplineJobs_CPU;

    // }
    // ///////////////////////////////////////////
    // // Next, evaluate 1D polynomial orbitals //
    // ///////////////////////////////////////////
    // if (AtomicSplineJobs_CPU.size()) {
    //   AtomicPolyJobs_GPU = AtomicPolyJobs_CPU;
    // }



  }

  template<> void
  EinsplineSetHybrid<double>::evaluate (vector<Walker_t*> &walkers, vector<PosType> &newpos, 
					gpu::device_vector<CudaComplexType*> &phi,
					gpu::device_vector<CudaComplexType*> &grad_lapl,
					int row_stride)
  {
    app_error() << "EinsplineSetHybrid<double>::evaluate \n"
		<< "(vector<Walker_t*> &walkers, vector<PosType> &newpos, \n"
		<< " gpu::device_vector<CudaComplexType*> &phi,\n"
		<< " gpu::device_vector<CudaComplexType*> &grad_lapl, int row_stride)\n"
		<< "     is not yet implemented.\n";
    abort();
  }

  template<> void
  EinsplineSetHybrid<double>::evaluate 
  (vector<PosType> &pos, gpu::device_vector<CudaRealType*> &phi)
  {
    int N = pos.size();
    if (cudapos.size() < N) {
      resize_cuda(N);
      hostPos.resize(N);
      cudapos.resize(N);
    }
    for (int iw=0; iw < N; iw++) 
      hostPos[iw] = pos[iw];
    cudapos = hostPos;
    

    MakeHybridJobList ((float*) cudapos.data(), N, IonPos_GPU.data(), 
		       PolyRadii_GPU.data(), CutoffRadii_GPU.data(),
		       AtomicOrbitals.size(), L_cuda.data(), Linv_cuda.data(),
		       HybridJobs_GPU.data(), rhats_GPU.data(),
		       HybridData_GPU.data());

    CalcYlmRealCuda (rhats_GPU.data(), HybridJobs_GPU.data(),
		     Ylm_ptr_GPU.data(), dYlm_dtheta_ptr_GPU.data(), 
		     dYlm_dphi_ptr_GPU.data(), lMax, pos.size());

    evaluateHybridSplineReal (HybridJobs_GPU.data(), Ylm_ptr_GPU.data(), 
    			      AtomicOrbitals_GPU.data(), HybridData_GPU.data(),
    			      (CudaRealType*)CudakPoints_reduced.data(),
			      phi.data(), NumOrbitals, pos.size(), lMax);
    evaluate3DSplineReal (HybridJobs_GPU.data(), (float*)cudapos.data(), 
			  (CudaRealType*)CudakPoints_reduced.data(),CudaMultiSpline,
			  Linv_cuda.data(), phi.data(), NumOrbitals, pos.size());
  }

  template<> void
  EinsplineSetHybrid<double>::evaluate (vector<PosType> &pos, gpu::device_vector<CudaComplexType*> &phi)
  {
    app_error() << "EinsplineSetHybrid<double>::evaluate \n"
		 << "(vector<PosType> &pos, gpu::device_vector<CudaComplexType*> &phi)\n"
		 << "     is not yet implemented.\n";
    abort();
  }
  
  template<> string
  EinsplineSetHybrid<double>::Type()
  {
  }
  
  
  template<typename StorageType> SPOSetBase*
  EinsplineSetHybrid<StorageType>::makeClone() const
  {
    EinsplineSetHybrid<StorageType> *clone = 
      new EinsplineSetHybrid<StorageType> (*this);
    clone->registerTimers();
    return clone;
  }
  

  //////////////////////////////////
  // Complex StorageType versions //
  //////////////////////////////////

  template<> void
  EinsplineSetHybrid<complex<double> >::evaluate (vector<Walker_t*> &walkers, int iat,
					gpu::device_vector<CudaRealType*> &phi)
  {
    app_error() << "EinsplineSetHybrid<complex<double> >::evaluate (vector<Walker_t*> &walkers, int iat,\n"
		<< "			                            gpu::device_vector<CudaRealType*> &phi)\n"
		<< "not yet implemented.\n";
  }

  
  template<> void
  EinsplineSetHybrid<complex<double> >::evaluate (vector<Walker_t*> &walkers, int iat,
						  gpu::device_vector<CudaComplexType*> &phi)
  {
   app_error() << "EinsplineSetHybrid<complex<double> >::evaluate (vector<Walker_t*> &walkers, int iat,\n"
		<< "			                            gpu::device_vector<CudaComplexType*> &phi)\n"
	       << "not yet implemented.\n";
  }

  
  template<> void
  EinsplineSetHybrid<complex<double> >::evaluate (vector<Walker_t*> &walkers, vector<PosType> &newpos, 
						  gpu::device_vector<CudaRealType*> &phi)
  { 
   app_error() << "EinsplineSetHybrid<complex<double> >::evaluate (vector<Walker_t*> &walkers, vector<PosType> &newpos,\n"
		<< "			                            gpu::device_vector<CudaRealType*> &phi)\n"
		<< "not yet implemented.\n";
  }

  template<> void
  EinsplineSetHybrid<complex<double> >::evaluate (vector<Walker_t*> &walkers, vector<PosType> &newpos,
						  gpu::device_vector<CudaComplexType*> &phi)
  {
   app_error() << "EinsplineSetHybrid<complex<double> >::evaluate (vector<Walker_t*> &walkers, vector<PosType> ,\n"
		<< "			                            gpu::device_vector<CudaComplexType*> &phi)\n"
		<< "not yet implemented.\n";
  }

  template<> void
  EinsplineSetHybrid<complex<double> >::evaluate 
    (vector<Walker_t*> &walkers,  vector<PosType> &newpos, 
     gpu::device_vector<CudaRealType*> &phi, gpu::device_vector<CudaRealType*> &grad_lapl,
     int row_stride)
  { 
    static int numAtomic=0;
    static int num3D=0;

    int N = newpos.size();
    if (cudapos.size() < N) {
      resize_cuda(N);
      hostPos.resize(N);
      cudapos.resize(N);
    }
    for (int iw=0; iw < N; iw++) 
      hostPos[iw] = newpos[iw];
    cudapos = hostPos;
    
    MakeHybridJobList ((float*) cudapos.data(), N, IonPos_GPU.data(), 
		       PolyRadii_GPU.data(), CutoffRadii_GPU.data(),
		       AtomicOrbitals.size(), L_cuda.data(), Linv_cuda.data(),
		       HybridJobs_GPU.data(), rhats_GPU.data(),
		       HybridData_GPU.data());

    CalcYlmComplexCuda (rhats_GPU.data(), HybridJobs_GPU.data(),
			Ylm_ptr_GPU.data(), dYlm_dtheta_ptr_GPU.data(), 
			dYlm_dphi_ptr_GPU.data(), lMax, newpos.size());

    evaluate3DSplineComplexToReal (HybridJobs_GPU.data(), 
				   (float*)cudapos.data(), 
    				   (CudaRealType*)CudakPoints.data(),
				   CudaMakeTwoCopies.data(), CudaMultiSpline,
    				   Linv_cuda.data(), 
				   phi.data(), grad_lapl.data(), 
    				   row_stride, CudaMakeTwoCopies.size(), 
				   newpos.size());

    evaluateHybridSplineComplexToReal 
      (HybridJobs_GPU.data(), 
       rhats_GPU.data(), 
       Ylm_ptr_GPU.data(),
        dYlm_dtheta_ptr_GPU.data(), dYlm_dphi_ptr_GPU.data(),
       AtomicOrbitals_GPU.data(), HybridData_GPU.data(),
       (CudaRealType*)CudakPoints_reduced.data(), 
       CudaMakeTwoCopies.data(), (CudaRealType**)phi.data(), 
       grad_lapl.data(), row_stride, 
       CudaMakeTwoCopies.size(), newpos.size(), lMax);

#ifdef HYBRID_DEBUG

    // gpu::host_vector<HybridJobType> HybridJobs_CPU(HybridJobs_GPU.size());
    // HybridJobs_CPU = HybridJobs_GPU;

    // int M = MakeTwoCopies.size();
    // // ComplexValueVector_t CPUzvals(M), CPUzlapl(M);
    // // ComplexGradVector_t CPUzgrad(M);
    // for (int iw=0; iw<newpos.size(); iw++) {
    //   if (HybridJobs_CPU[iw] == BSPLINE_3D_JOB)
    // 	num3D++;
    //   else
    // 	numAtomic++;
    //   // bool atomic=false;
    //   // for (int ion=0; ion<AtomicOrbitals.size(); ion++)
    //   // 	if (AtomicOrbitals[ion].evaluate 
    //   // 	    (newpos[iw], CPUzvals, CPUzgrad, CPUzlapl)) {
    //   // 	  atomic = true;
    //   // 	  break;
    //   // 	}
    //   // if (atomic)
    //   // 	numAtomic++;
    //   // else
    //   // 	num3D++;
    //   // if (atomic && HybridJobs_CPU[iw] == BSPLINE_3D_JOB)
    //   // 	cerr << "Error!  Used BSPLINE_3D when I should have used atomic.\n";
    //   // else if (!atomic && HybridJobs_CPU[iw] != BSPLINE_3D_JOB)
    //   // 	cerr << "Error!  Used atomic when I should have used 3D.\n";
    // }
    // // fprintf (stderr, "Num atomic = %d  Num 3D = %d\n",
    // // 	     numAtomic, num3D);
    // if (numAtomic + num3D > 100000) {
    //   fprintf (stderr, "Num atomic = %d  Num 3D = %d\n",
    // 	       numAtomic, num3D);
    //   fprintf (stderr, "Percent atomic = %1.5f\%\n",
    // 	       100.0*(double)numAtomic / (double)(numAtomic+num3D));
    //   numAtomic = num3D = 0;
    // }

    gpu::host_vector<CudaRealType*> phi_CPU (phi.size()), grad_lapl_CPU(phi.size());
    phi_CPU = phi;
    grad_lapl_CPU = grad_lapl;
    gpu::host_vector<CudaRealType> vals_CPU(NumOrbitals), GL_CPU(4*row_stride);
    gpu::host_vector<HybridJobType> HybridJobs_CPU(HybridJobs_GPU.size());
    HybridJobs_CPU = HybridJobs_GPU;
    gpu::host_vector<HybridDataFloat> HybridData_CPU(HybridData_GPU.size());
    HybridData_CPU = HybridData_GPU;
    
    rhats_CPU = rhats_GPU;
    
    // for (int iw=0; iw<newpos.size(); iw++) 
    //   fprintf (stderr, "rhat[%d] = [%10.6f %10.6f %10.6f]\n",
    // 	       iw, rhats_CPU[3*iw+0], rhats_CPU[3*iw+1], rhats_CPU[3*iw+2]);

    for (int iw=0; iw<newpos.size(); iw++) 
      if (false && HybridJobs_CPU[iw] == ATOMIC_POLY_JOB) {
      //if (HybridJobs_CPU[iw] != BSPLINE_3D_JOB && std::fabs(rhats_CPU[3*iw+2]) < 1.0e-6) {
	int M = MakeTwoCopies.size();
	ComplexValueVector_t CPUzvals(M), CPUzlapl(M);
	ComplexGradVector_t CPUzgrad(M);
	ValueVector_t CPUvals(NumOrbitals), CPUlapl(NumOrbitals);
	GradVector_t CPUgrad(NumOrbitals);
	HybridDataFloat &d = HybridData_CPU[iw];
	AtomicOrbital<complex<double> > &atom = AtomicOrbitals[d.ion];
	atom.evaluate (newpos[iw], CPUzvals, CPUzgrad, CPUzlapl);
	int index=0;
	for (int i=0; i<StorageValueVector.size(); i++) {
	  CPUvals[index] = CPUzvals[i].real();
	  CPUlapl[index] = CPUzlapl[i].real();
	  for (int j=0; j<OHMMS_DIM; j++)
	    CPUgrad[index][j] = CPUzgrad[i][j].real();
	  index++;
	  if (MakeTwoCopies[i]) {
	    CPUvals[index] = CPUzvals[i].imag();
	    CPUlapl[index] = CPUzlapl[i].imag();
	    for (int j=0; j<OHMMS_DIM; j++)
	      CPUgrad[index][j] = CPUzgrad[i][j].imag();
	    index++;
	  }
	}

	cudaMemcpy (&vals_CPU[0], phi_CPU[iw], NumOrbitals*sizeof(float),
		    cudaMemcpyDeviceToHost);
	cudaMemcpy (&GL_CPU[0], grad_lapl_CPU[iw], 4*row_stride*sizeof(float),
		    cudaMemcpyDeviceToHost);
	// fprintf (stderr, " %d %2.0f %2.0f %2.0f  %8.5f  %d %d\n",
	// 	 iw, d.img[0], d.img[1], d.img[2], d.dist, d.ion, d.lMax);
	double mindist = 1.0e5;
	for (int ion=0; ion<AtomicOrbitals.size(); ion++) {
	  PosType disp = newpos[iw] - AtomicOrbitals[ion].Pos;
	  PosType u = PrimLattice.toUnit(disp);
	  PosType img;
	  for (int i=0; i<OHMMS_DIM; i++) 
	    u[i] -= round(u[i]);
	  disp = PrimLattice.toCart(u);
	  double dist = std::sqrt(dot(disp,disp));
	  if (dist < AtomicOrbitals[ion].CutoffRadius)
	    mindist = dist;
	}
	if (std::fabs (mindist - d.dist) > 1.0e-3)
	  fprintf (stderr, "CPU dist = %1.8f  GPU dist = %1.8f\n",
		   mindist, d.dist);
	fprintf (stderr, "rhat[%d] = [%10.6f %10.6f %10.6f] dist = %10.6e\n",
		 iw, rhats_CPU[3*iw+0], rhats_CPU[3*iw+1], rhats_CPU[3*iw+2], 
		 d.dist);

	for (int j=0; j<NumOrbitals; j++) {
	  //	  if (isnan(vals_CPU[j])) {
	  if (true || isnan(GL_CPU[0*row_stride+j])) {
	    cerr << "iw = " << iw << endl;
	    fprintf (stderr, "val[%2d]  = %10.5e %10.5e\n", 
		     j, vals_CPU[j], CPUvals[j]);
	    fprintf (stderr, "grad[%2d] = %10.5e %10.5e  %10.5e %10.5e  %10.5e %10.5e\n", j,
		     GL_CPU[0*row_stride+j], CPUgrad[j][0],
		     GL_CPU[1*row_stride+j], CPUgrad[j][1],
		     GL_CPU[2*row_stride+j], CPUgrad[j][2]);
	    
	    fprintf (stderr, "lapl[%2d] = %10.5e %10.5e\n", 
		     j, GL_CPU[3*row_stride+j], CPUlapl[j]);
	  }
	}
      }
      else if (HybridJobs_CPU[iw] == BSPLINE_3D_JOB){
      	ComplexValueVector_t CPUzvals(NumOrbitals), CPUzlapl(NumOrbitals);
      	ComplexGradVector_t CPUzgrad(NumOrbitals);
      	ValueVector_t CPUvals(NumOrbitals), CPUlapl(NumOrbitals);
      	GradVector_t CPUgrad(NumOrbitals);
      	PosType ru(PrimLattice.toUnit(newpos[iw]));
      	for (int i=0; i<3; i++)
      	  ru[i] -= std::floor(ru[i]);
      	EinsplineMultiEval (MultiSpline, ru, CPUzvals, CPUzgrad,
      	 		    StorageHessVector);


      	for (int j=0; j<MakeTwoCopies.size(); j++) {
      	  CPUzgrad[j] = dot (PrimLattice.G, CPUzgrad[j]);
      	  CPUzlapl[j] = trace (StorageHessVector[j], GGt);
      	}
      	// Add e^-ikr phase to B-spline orbitals
      	complex<double> eye(0.0, 1.0);
      	for (int j=0; j<MakeTwoCopies.size(); j++) {
      	  complex<double> u = CPUzvals[j];
      	  TinyVector<complex<double>,OHMMS_DIM> gradu = CPUzgrad[j];
      	  complex<double> laplu = CPUzlapl[j];
      	  PosType k = kPoints[j];
      	  TinyVector<complex<double>,OHMMS_DIM> ck;
      	  for (int n=0; n<OHMMS_DIM; n++)	  ck[n] = k[n];
      	  double s,c;
      	  double phase = -dot(newpos[iw], k);
      	  sincos (phase, &s, &c);
      	  complex<double> e_mikr (c,s);
      	  CPUzvals[j]   = e_mikr*u;
      	  CPUzgrad[j]  = e_mikr*(-eye*u*ck + gradu);
      	  CPUzlapl[j]  = e_mikr*(-dot(k,k)*u - 2.0*eye*dot(ck,gradu) + laplu);
      	}


      	int index=0;
      	for (int i=0; i<MakeTwoCopies.size(); i++) {
      	  CPUvals[index] = CPUzvals[i].real();
      	  CPUlapl[index] = CPUzlapl[i].real();
      	  for (int j=0; j<OHMMS_DIM; j++)
      	    CPUgrad[index][j] = CPUzgrad[i][j].real();
      	  index++;
      	  if (MakeTwoCopies[i]) {
      	    CPUvals[index] = CPUzvals[i].imag();
      	    CPUlapl[index] = CPUzlapl[i].imag();
      	    for (int j=0; j<OHMMS_DIM; j++)
      	      CPUgrad[index][j] = CPUzgrad[i][j].imag();
      	    index++;
      	  }
      	}

      	cudaMemcpy (&vals_CPU[0], phi_CPU[iw], NumOrbitals*sizeof(float),
      		    cudaMemcpyDeviceToHost);
      	cudaMemcpy (&GL_CPU[0], grad_lapl_CPU[iw], 4*row_stride*sizeof(float),
      		    cudaMemcpyDeviceToHost);

      	// for (int i=0; i<4*row_stride; i++)
      	//   fprintf (stderr, "%d %10.5e\n", i, GL_CPU[i]);

	static long int numgood=0, numbad=0;
	

      	for (int j=0; j<NumOrbitals; j++) {
	  double lap_ratio = GL_CPU[3*row_stride+j] /  CPUlapl[j];
	  if (std::fabs(GL_CPU[3*row_stride+j] - CPUlapl[j]) > 1.0e-4) {
	    fprintf (stderr, "Error:  CPU laplacian = %1.8e  GPU = %1.8e\n",
	    	     CPUlapl[j],  GL_CPU[3*row_stride+j]);
	    fprintf (stderr, "        CPU value     = %1.8e  GPU = %1.8e\n",
	    	     CPUvals[j],  vals_CPU[j]);
	    fprintf (stderr, "u = %1.8f %1.8f %1.8f \n", ru[0], ru[1], ru[2]);
	    cerr << "iw = " << iw << endl;
	    numbad++;
	  }
	  else 
	    numgood++;
	  
	  if (numbad + numgood >= 100000) {
	    double percent_bad = 100.0*(double)numbad / (double)(numbad + numgood);
	    fprintf (stderr, "Percent bad = %1.8f\n", percent_bad);
	    numbad = numgood = 0;
	  }
      	  // if (true || isnan(GL_CPU[0*row_stride+j])) {
      	  //   // cerr << "r[" << iw << "] = " << newpos[iw] << endl;
      	  //   // cerr << "iw = " << iw << endl;

      	  //   fprintf (stderr, "\n3D      val[%2d]  = %10.5e %10.5e\n", 
      	  // 	     j, vals_CPU[j], CPUvals[j]);
      	  //   fprintf (stderr, "3D GPU grad[%2d] = %10.5e %10.5e %10.5e\n", j,
      	  // 	     GL_CPU[0*row_stride+j],
      	  // 	     GL_CPU[1*row_stride+j],
      	  // 	     GL_CPU[2*row_stride+j]);
      	  //   fprintf (stderr, "3D CPU grad[%2d] = %10.5e %10.5e %10.5e\n", j,
      	  // 	     CPUgrad[j][0],
      	  // 	     CPUgrad[j][1],
      	  // 	     CPUgrad[j][2]);
	    
      	  //   fprintf (stderr, "3D     lapl[%2d] = %10.5e %10.5e\n", 
      	  // 	     j, GL_CPU[3*row_stride+j], CPUlapl[j]);
      	  // }
      	}
      }



    gpu::host_vector<float> Ylm_CPU(Ylm_GPU.size());
    Ylm_CPU = Ylm_GPU;
    
    rhats_CPU = rhats_GPU;
    for (int i=0; i<rhats_CPU.size()/3; i++)
      fprintf (stderr, "rhat[%d] = [%10.6f %10.6f %10.6f]\n",
	       i, rhats_CPU[3*i+0], rhats_CPU[3*i+1], rhats_CPU[3*i+2]);

    gpu::host_vector<HybridJobType> HybridJobs_CPU(HybridJobs_GPU.size());
    HybridJobs_CPU = HybridJobs_GPU;
        
    gpu::host_vector<HybridDataFloat> HybridData_CPU(HybridData_GPU.size());
    HybridData_CPU = HybridData_GPU;

    cerr << "Before loop.\n";
    for (int i=0; i<newpos.size(); i++) 
      if (HybridJobs_CPU[i] != BSPLINE_3D_JOB) {
	cerr << "Inside if.\n";
	PosType rhat(rhats_CPU[3*i+0], rhats_CPU[3*i+1], rhats_CPU[3*i+2]);
	AtomicOrbital<double> &atom = AtomicOrbitals[HybridData_CPU[i].ion];
	int numlm = (atom.lMax+1)*(atom.lMax+1);
	vector<double> Ylm(numlm), dYlm_dtheta(numlm), dYlm_dphi(numlm);
	atom.CalcYlm (rhat, Ylm, dYlm_dtheta, dYlm_dphi);
	
	for (int lm=0; lm < numlm; lm++) {
	  fprintf (stderr, "lm=%3d  Ylm_CPU=%8.5f  Ylm_GPU=%8.5f\n",
		   lm, Ylm[lm], Ylm_CPU[3*i*Ylm_BS+lm]);
	}
      }

    fprintf (stderr, " N  img      dist    ion    lMax\n");
    for (int i=0; i<HybridData_CPU.size(); i++) {
      HybridDataFloat &d = HybridData_CPU[i];
      fprintf (stderr, " %d %2.0f %2.0f %2.0f  %8.5f  %d %d\n",
	       i, d.img[0], d.img[1], d.img[2], d.dist, d.ion, d.lMax);
    }
#endif
  }

  template<> void
  EinsplineSetHybrid<complex<double> >::evaluate (vector<Walker_t*> &walkers, vector<PosType> &newpos, 
  		 gpu::device_vector<CudaComplexType*> &phi,
  		 gpu::device_vector<CudaComplexType*> &grad_lapl,
  		 int row_stride)
  {
    app_error() << "EinsplineSetHybrid<complex<double> >::evaluate \n"
		<< "(vector<Walker_t*> &walkers, vector<PosType> &newpos, \n"
		<< " gpu::device_vector<CudaComplexType*> &phi,\n"
		<< " gpu::device_vector<CudaComplexType*> &grad_lapl,\n"
		<< " int row_stride)\n"
		<< "not yet implemented.\n";
    abort();
  }

  template<> void
  EinsplineSetHybrid<complex<double> >::evaluate 
  (vector<PosType> &pos, gpu::device_vector<CudaRealType*> &phi)
  {
    int N = pos.size();
    if (cudapos.size() < N) {
      resize_cuda(N);
      hostPos.resize(N);
      cudapos.resize(N);
    }
    for (int iw=0; iw < N; iw++) 
      hostPos[iw] = pos[iw];
    cudapos = hostPos;
    
    MakeHybridJobList ((float*) cudapos.data(), N, IonPos_GPU.data(), 
		       PolyRadii_GPU.data(), CutoffRadii_GPU.data(),
		       AtomicOrbitals.size(), L_cuda.data(), Linv_cuda.data(),
		       HybridJobs_GPU.data(), rhats_GPU.data(),
		       HybridData_GPU.data());

    CalcYlmComplexCuda (rhats_GPU.data(), HybridJobs_GPU.data(),
			Ylm_ptr_GPU.data(), dYlm_dtheta_ptr_GPU.data(), 
			dYlm_dphi_ptr_GPU.data(), lMax, pos.size());

    evaluate3DSplineComplexToReal 
      (HybridJobs_GPU.data(), (float*)cudapos.data(), 
       (CudaRealType*)CudakPoints.data(),CudaMakeTwoCopies.data(),
       CudaMultiSpline, Linv_cuda.data(), 
       phi.data(), CudaMakeTwoCopies.size(), pos.size());
    evaluateHybridSplineComplexToReal//NLPP 
      (HybridJobs_GPU.data(), 
       Ylm_ptr_GPU.data(),
       AtomicOrbitals_GPU.data(), HybridData_GPU.data(),
       (CudaRealType*)CudakPoints_reduced.data(), 
       CudaMakeTwoCopies.data(), (CudaRealType**)phi.data(), 
       CudaMakeTwoCopies.size(), pos.size(), lMax);

    // gpu::host_vector<HybridJobType> HybridJobs_CPU(HybridJobs_GPU.size());
    // HybridJobs_CPU = HybridJobs_GPU;

    // int M = CudaMakeTwoCopies.size();
    // ComplexValueVector_t CPUzvals(M);
    // for (int iw=0; iw<pos.size(); iw++) 
    //   if (HybridJobs_CPU[iw] == BSPLINE_3D_JOB)
    // 	cerr << "Error:  used BSPLINE_3D for PP eval.  Walker = " 
    // 	     << iw << "\n";

    // cerr << "pos.size() = " << pos.size() << endl;
    // for (int iw=0; iw<pos.size(); iw++) {
    //   // if (HybridJobs_CPU[iw] == BSPLINE_3D_JOB)
    //   // 	num3D++;
    //   // else
    //   // 	numAtomic++;
    //   bool atomic=false;
    //   for (int ion=0; ion<AtomicOrbitals.size(); ion++)
    //   	if (AtomicOrbitals[ion].evaluate (pos[iw], CPUzvals)) {
    //   	  atomic = true;
    //   	  break;
    //   	}
    //   // if (atomic)
    //   // 	numAtomic++;
    //   // else
    //   // 	num3D++;
    //   if (atomic && HybridJobs_CPU[iw] == BSPLINE_3D_JOB)
    //   	cerr << "Error!  Used BSPLINE_3D when I should have used atomic.\n";
    //   else if (!atomic && HybridJobs_CPU[iw] != BSPLINE_3D_JOB)
    //   	cerr << "Error!  Used atomic when I should have used 3D.\n";
    // }

    //////////////////////////
    // This must be tested! //
    //////////////////////////
    

  }

  template<> void
  EinsplineSetHybrid<complex<double> >::evaluate 
  (vector<PosType> &pos, gpu::device_vector<CudaComplexType*> &phi)
  {
    app_error() << "EinsplineSetHybrid<complex<double> >::evaluate \n"
		<< "(vector<PosType> &pos, gpu::device_vector<CudaComplexType*> &phi)\n"
		<< "not yet implemented.\n";
  }
  
  template<> string
  EinsplineSetHybrid<complex<double> >::Type()
  {
  }
  






  template<>
  EinsplineSetHybrid<double>::EinsplineSetHybrid() :
    CurrentWalkers(0)
  {
    ValueTimer.set_name ("EinsplineSetHybrid::ValueOnly");
    VGLTimer.set_name ("EinsplineSetHybrid::VGL");
    ValueTimer.set_name ("EinsplineSetHybrid::VGLMat");
    EinsplineTimer.set_name ("EinsplineSetHybrid::Einspline");
    className = "EinsplineSeHybrid";
    TimerManager.addTimer (&ValueTimer);
    TimerManager.addTimer (&VGLTimer);
    TimerManager.addTimer (&VGLMatTimer);
    TimerManager.addTimer (&EinsplineTimer);
    for (int i=0; i<OHMMS_DIM; i++)
      HalfG[i] = 0;
  }

  template<>
  EinsplineSetHybrid<complex<double > >::EinsplineSetHybrid() :
    CurrentWalkers(0)
  {
    ValueTimer.set_name ("EinsplineSetHybrid::ValueOnly");
    VGLTimer.set_name ("EinsplineSetHybrid::VGL");
    ValueTimer.set_name ("EinsplineSetHybrid::VGLMat");
    EinsplineTimer.set_name ("EinsplineSetHybrid::Einspline");
    className = "EinsplineSeHybrid";
    TimerManager.addTimer (&ValueTimer);
    TimerManager.addTimer (&VGLTimer);
    TimerManager.addTimer (&VGLMatTimer);
    TimerManager.addTimer (&EinsplineTimer);
    for (int i=0; i<OHMMS_DIM; i++)
      HalfG[i] = 0;
  }

  template<> void
  EinsplineSetHybrid<double>::init_cuda()
  {
    // Setup B-spline Acuda matrix in constant memory
    init_atomic_cuda();

    gpu::host_vector<AtomicOrbitalCuda<CudaRealType> > AtomicOrbitals_CPU;
    const int BS=16;
    NumOrbitals = getOrbitalSetSize();
    // Bump up the stride to be a multiple of 512-bit bus width
    int lm_stride = ((NumOrbitals+BS-1)/BS)*BS;
    
    AtomicSplineCoefs_GPU.resize(AtomicOrbitals.size());
    AtomicPolyCoefs_GPU.resize(AtomicOrbitals.size());
    vector<CudaRealType> IonPos_CPU, CutoffRadii_CPU, PolyRadii_CPU;

    for (int iat=0; iat<AtomicOrbitals.size(); iat++) {
      app_log() << "Copying real atomic orbitals for ion " << iat << " to GPU memory.\n";
      AtomicOrbital<double> &atom = AtomicOrbitals[iat];
      for (int i=0; i<OHMMS_DIM; i++) 
	IonPos_CPU.push_back(atom.Pos[i]);
      CutoffRadii_CPU.push_back(atom.CutoffRadius);
      PolyRadii_CPU.push_back(atom.PolyRadius);
      AtomicOrbitalCuda<CudaRealType> atom_cuda;
      atom_cuda.lMax = atom.lMax;
      int numlm = (atom.lMax+1)*(atom.lMax+1);
      atom_cuda.lm_stride = lm_stride;
      atom_cuda.spline_stride = numlm * lm_stride;

      AtomicOrbital<double>::SplineType &cpu_spline = 
	*atom.get_radial_spline();
      atom_cuda.spline_dr_inv = cpu_spline.x_grid.delta_inv;
      int Ngrid = cpu_spline.x_grid.num;
      int spline_size = 2*atom_cuda.spline_stride * (Ngrid+2);
      gpu::host_vector<float> spline_coefs(spline_size);
      AtomicSplineCoefs_GPU[iat].resize(spline_size);
      atom_cuda.spline_coefs = AtomicSplineCoefs_GPU[iat].data();
      // Reorder and copy splines to GPU memory
      for (int igrid=0; igrid<Ngrid; igrid++)
	for (int lm=0; lm<numlm; lm++)
	  for (int orb=0; orb<NumOrbitals; orb++) {
	    // Convert splines to Hermite spline form, because
	    // B-spline form in single precision with dense grids
	    // leads to large truncation error
	    double u = 1.0/6.0*
	      (1.0*cpu_spline.coefs[(igrid+0)*cpu_spline.x_stride + orb*numlm +lm] +
	       4.0*cpu_spline.coefs[(igrid+1)*cpu_spline.x_stride + orb*numlm +lm] +
	       1.0*cpu_spline.coefs[(igrid+2)*cpu_spline.x_stride + orb*numlm +lm]);
	    double d2u = cpu_spline.x_grid.delta_inv * cpu_spline.x_grid.delta_inv *
	       (1.0*cpu_spline.coefs[(igrid+0)*cpu_spline.x_stride + orb*numlm +lm] +
	       -2.0*cpu_spline.coefs[(igrid+1)*cpu_spline.x_stride + orb*numlm +lm] +
		1.0*cpu_spline.coefs[(igrid+2)*cpu_spline.x_stride + orb*numlm +lm]);
	      
	    spline_coefs[(2*igrid+0)*atom_cuda.spline_stride +
			 lm   *atom_cuda.lm_stride + orb] = u;
	    spline_coefs[(2*igrid+1)*atom_cuda.spline_stride +
			 lm   *atom_cuda.lm_stride + orb] = d2u;
	  }

      
      AtomicSplineCoefs_GPU[iat] = spline_coefs;      
      
      atom_cuda.poly_stride = numlm*atom_cuda.lm_stride;
      atom_cuda.poly_order = atom.PolyOrder;
      int poly_size = (atom.PolyOrder+1)*atom_cuda.poly_stride;
      gpu::host_vector<float> poly_coefs(poly_size);
      AtomicPolyCoefs_GPU[iat].resize(poly_size);
      atom_cuda.poly_coefs = AtomicPolyCoefs_GPU[iat].data();
      for (int lm=0; lm<numlm; lm++)
	for (int n=0; n<atom.PolyOrder; n++)
	  for (int orb=0; orb<NumOrbitals; orb++)
	    poly_coefs[n*atom_cuda.poly_stride + lm*atom_cuda.lm_stride + orb] 
	      = atom.get_poly_coefs()(n,orb,lm);
      AtomicPolyCoefs_GPU[iat] = poly_coefs;
      AtomicOrbitals_CPU.push_back(atom_cuda);
    }
    AtomicOrbitals_GPU = AtomicOrbitals_CPU;
    IonPos_GPU      = IonPos_CPU;
    CutoffRadii_GPU = CutoffRadii_CPU;
    PolyRadii_GPU   = PolyRadii_CPU;
  }

  template<> void
  EinsplineSetHybrid<complex<double> >::init_cuda()
  {
    // Setup B-spline Acuda matrix in constant memory
    init_atomic_cuda();

    gpu::host_vector<AtomicOrbitalCuda<CudaRealType> > AtomicOrbitals_CPU;
    const int BS=16;
    NumOrbitals = getOrbitalSetSize();
    // Bump up the stride to be a multiple of 512-bit bus width
    int lm_stride = ((2*NumOrbitals+BS-1)/BS)*BS;
    
    AtomicSplineCoefs_GPU.resize(AtomicOrbitals.size());
    AtomicPolyCoefs_GPU.resize(AtomicOrbitals.size());
    vector<CudaRealType> IonPos_CPU, CutoffRadii_CPU, PolyRadii_CPU;

    for (int iat=0; iat<AtomicOrbitals.size(); iat++) {
      app_log() << "Copying atomic orbitals for ion " << iat << " to GPU memory.\n";
      AtomicOrbital<complex<double> > &atom = AtomicOrbitals[iat];
      for (int i=0; i<OHMMS_DIM; i++) 
	IonPos_CPU.push_back(atom.Pos[i]);
      CutoffRadii_CPU.push_back(atom.CutoffRadius);
      PolyRadii_CPU.push_back(atom.PolyRadius);
      AtomicOrbitalCuda<CudaRealType> atom_cuda;
      atom_cuda.lMax = atom.lMax;
      int numlm = (atom.lMax+1)*(atom.lMax+1);
      atom_cuda.lm_stride = lm_stride;
      atom_cuda.spline_stride = numlm * lm_stride;

      AtomicOrbital<complex<double> >::SplineType &cpu_spline = 
	*atom.get_radial_spline();
      atom_cuda.spline_dr_inv = cpu_spline.x_grid.delta_inv;
      int Ngrid = cpu_spline.x_grid.num;
      int spline_size = 2*atom_cuda.spline_stride * (Ngrid+2);
      gpu::host_vector<float> spline_coefs(spline_size);
      AtomicSplineCoefs_GPU[iat].resize(spline_size);
      atom_cuda.spline_coefs = AtomicSplineCoefs_GPU[iat].data();
      // Reorder and copy splines to GPU memory
      for (int igrid=0; igrid<Ngrid; igrid++)
	for (int lm=0; lm<numlm; lm++)
	  for (int orb=0; orb<NumOrbitals; orb++) {
	    // Convert splines to Hermite spline form, because
	    // B-spline form in single precision with dense grids
	    // leads to large truncation error
	    complex<double> u = 1.0/6.0*
	      (1.0*cpu_spline.coefs[(igrid+0)*cpu_spline.x_stride + orb*numlm +lm] +
	       4.0*cpu_spline.coefs[(igrid+1)*cpu_spline.x_stride + orb*numlm +lm] +
	       1.0*cpu_spline.coefs[(igrid+2)*cpu_spline.x_stride + orb*numlm +lm]);
	    complex<double> d2u = cpu_spline.x_grid.delta_inv * cpu_spline.x_grid.delta_inv *
	      (1.0*cpu_spline.coefs[(igrid+0)*cpu_spline.x_stride + orb*numlm +lm] +
	      -2.0*cpu_spline.coefs[(igrid+1)*cpu_spline.x_stride + orb*numlm +lm] +
	       1.0*cpu_spline.coefs[(igrid+2)*cpu_spline.x_stride + orb*numlm +lm]);
	    
	    

	    spline_coefs[((2*igrid+0)*atom_cuda.spline_stride + lm*atom_cuda.lm_stride + 2*orb)+0] =
	      u.real();
	    spline_coefs[((2*igrid+0)*atom_cuda.spline_stride + lm*atom_cuda.lm_stride + 2*orb)+1] =
	      u.imag();
	    spline_coefs[((2*igrid+1)*atom_cuda.spline_stride + lm*atom_cuda.lm_stride + 2*orb)+0] =
	      d2u.real();
	    spline_coefs[((2*igrid+1)*atom_cuda.spline_stride + lm*atom_cuda.lm_stride + 2*orb)+1] =
	      d2u.imag();
	  }
      AtomicSplineCoefs_GPU[iat] = spline_coefs;

      atom_cuda.poly_stride = numlm*atom_cuda.lm_stride;
      atom_cuda.poly_order = atom.PolyOrder;
      int poly_size = (atom.PolyOrder+1)*atom_cuda.poly_stride;
      gpu::host_vector<float> poly_coefs(poly_size);
      AtomicPolyCoefs_GPU[iat].resize(poly_size);
      atom_cuda.poly_coefs = &AtomicPolyCoefs_GPU[iat].data()[0];
      for (int lm=0; lm<numlm; lm++)
	for (int n=0; n<atom.PolyOrder; n++)
	  for (int orb=0; orb<NumOrbitals; orb++){
	    poly_coefs
	      [n*atom_cuda.poly_stride + lm*atom_cuda.lm_stride + 2*orb+0] =
	      atom.get_poly_coefs()(n,orb,lm).real();
	    poly_coefs
	      [n*atom_cuda.poly_stride + lm*atom_cuda.lm_stride + 2*orb+1] =
	      atom.get_poly_coefs()(n,orb,lm).imag();
	  }
      AtomicPolyCoefs_GPU[iat] = poly_coefs;

      AtomicOrbitals_CPU.push_back(atom_cuda);

    }
    AtomicOrbitals_GPU = AtomicOrbitals_CPU;
    IonPos_GPU      = IonPos_CPU;
    CutoffRadii_GPU = CutoffRadii_CPU;
    PolyRadii_GPU   = PolyRadii_CPU;
  }



  template class EinsplineSetHybrid<complex<double> >;
  template class EinsplineSetHybrid<        double  >;
  

}
