//////////////////////////////////////////////////////////////////
// (c) Copyright 1998-2002 by Jeongnim Kim
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
#ifndef OHMMS_NUMERIC_DETERMINANT_H
#define OHMMS_NUMERIC_DETERMINANT_H

#include <algorithm>
#include "OhmmsPETE/OhmmsVector.h"
#include "OhmmsPETE/OhmmsMatrix.h"
#include "Numerics/Blasf.h"

inline void LUFactorization(const int& n, const int& m, double* a, const int& n0, 
		     int* piv) {
  int status;
  dgetrf(n,m,a,n0,piv,status);
}

inline void LUFactorization(const int& n, const int& m, complex<double>* a, const int& n0, 
		     int* piv) {
  int status;
  zgetrf(n,m,a,n0,piv,status);
}

inline void InvertLU(const int& n, double* a, const int& n0, 
	      int* piv, double* work, const int& n1){
  int status;
  dgetri(n,a,n0,piv,work,n1,status);
}

inline double Invert(double* x, int n, int m) {
  double detvalue;
  if(n == 1) {
    detvalue = x[0];
    x[0]=1.0/detvalue;
  } else { 
    vector<double> work(n);
    vector<int> pivot(n);
    LUFactorization(n,m,x,n,&pivot[0]);
    detvalue = x[0];
    for (int i=1; i<m; ++i) detvalue *= x[i*m+i];
    InvertLU(n,x, n, &pivot[0], &work[0], n);
  }
    /*
  switch(n) {
  case(1):
    detvalue = x[0];
    x[0]=1.0/detvalue;break;
  case(2):
    detvalue = x[0] * x[3] - x[1] * x[2];
    double deti = 1.0/detvalue;
    x[0] = deti*x[3]; 
    x[1]*=-deti; 
    x[2]*=-deti;
    x[3] = deti*x[0];
    break;
  defaults:
    cout << "using generic inversion " << endl;
    vector<double> work(n);
    vector<int> pivot(n);
    LUFactorization(n,m,x,n,&pivot[0]);
    for (int i=1; i<m; ++i) detvalue *= x[i*m+i];
    InvertLU(n,x, n, &pivot[0], &work[0], n);
    break;
  }
    */
//   if(n == 1) {
//     x[0]=1.0/detvalue;
//   } else {
//     vector<double> work(n);
//     vector<int> pivot(n);
//     LUFactorization(n,m,x,n,&pivot[0]);
//     for (int i=1; i<m; ++i) detvalue *= x[i*m+i];
//     InvertLU(n,x, n, &pivot[0], &work[0], n);
//   }
  return detvalue;
}
/*!\fn invert_matrix(MatrixA& M, bool getdet)
 * \param MatrixA, a matrix to be inverted
 * \param bool, if true, calculate the determinant
 * \return the determinant
 */
template<class MatrixA>
inline double
invert_matrix(MatrixA& M, bool getdet=true) {

  typedef typename MatrixA::value_type value_type;
  Vector<int> pivot(M.rows());
  Vector<value_type> work(M.rows());
  int status;

  dgetrf(M.rows(), M.cols(), M.data(), M.rows(), pivot.data(), status);

  value_type det0 = 1.0; 

  if(getdet) {// calculate determinant
    int sign = 1;
    for(int i=0; i<M.rows(); ++i){
      if(pivot[i] != i+1) sign *= -1;
      det0 *= M(i,i);
    }
    det0 *= static_cast<value_type>(sign);
  }

  dgetri(M.rows(), M.data(),  M.rows(), pivot.data(), work.data(), 
	 M.rows(), status);
  return det0;
}  

template<class MatA, class Iter>
inline
typename MatA::value_type 
DetRatio(const MatA& Minv, Iter newrow, int rowchanged) {
  typename MatA::value_type res = 0.0;
  for(int j=0; j<Minv.cols(); j++,newrow++)
    res += Minv(rowchanged,j)*(*newrow);
  return res;
}

template<class T, unsigned D>
inline TinyVector<T,D>
dot(const T* a, const TinyVector<T,D>* b, int n) {
  TinyVector<T,D> res;
  for(int i=0; i<n; i++) res += a[i]*b[i];
  return res;
}

template<class T>
inline T dot(const T* restrict a, const T* restrict b, int n) {
  T res = 0.0;
  for(int i=0; i<n; i++) res += a[i]*b[i];
  return res;
}

// template<class T1, class T2>
// inline T2
// dot(const T1* restrict a, const T2* restrict b, int n) {
//   T2 res;
//   for(int i=0; i<n; i++) res += a[i]*b[i];
//   return res;
// }
#endif
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
