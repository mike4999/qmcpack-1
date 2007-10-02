/////////////////////////////////////////////////////////////////
// (c) Copyright 2006-  by Jeongnim Kim
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
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
/** @file PWParameterSet.cpp
 * @brief Utility class to handle hdf5
 */
#include "QMCWaveFunctions/PlaneWave/PWParameterSet.h"
#include "Utilities/OhmmsInfo.h"
#include "Message/Communicate.h"

namespace qmcplusplus {
  PWParameterSet::PWParameterSet():
    hasSpin(true),
  twistIndex(0),
  numBands(0),
  Ecut(-1),
  Rcut(-1),
  BufferRadius(-1),
  BoxDup(1),
  paramTag("parameters"),
  basisTag("basis"),
  pwTag("planewaves"),
  pwMultTag("multipliers"),
  eigTag("eigenstates"), 
  twistTag("twist"), 
  bandTag("band"), 
  spinTag("spin"),
  eigvecTag("eigenvector")
  { 
    m_param.setName("h5tag");
    m_param.add(twistIndex,"twistIndex","int");
    m_param.add(Rcut,"rcut","double");
    m_param.add(BufferRadius,"bufferLayer","double");
    m_param.add(BoxDup,"expand","int3");
    m_param.add(paramTag,"parameters","string");
    m_param.add(basisTag,"basis","string");
    m_param.add(pwTag,"planewaves","string");
    m_param.add(pwMultTag,"multiplers","string");
    m_param.add(eigTag,"eigenstates","string");
    m_param.add(twistTag,"twist","string");
    m_param.add(bandTag,"band","string");
    m_param.add(spinTag,"spin","string");
    m_param.add(eigvecTag,"eigenvector","string");
  }   

  double PWParameterSet::getEcut(double ecut) 
  {
    if(Ecut<0 || Ecut >= ecut) 
      Ecut=ecut;
    return Ecut;
  }

  bool PWParameterSet::getEigVectorType(hid_t h)
  {
    ostringstream oss;
    oss << "/"<<eigTag << "/"<<twistTag<<twistIndex << "/"<< bandTag << 0;
    //if(version[1]==10)
    if(hasSpin) oss << "/" << spinTag << 0;
    oss << "/eigenvector";

    hsize_t dimTot[4];
    hid_t dataset = H5Dopen(h,oss.str().c_str());
    hid_t dataspace = H5Dget_space(dataset);
    int rank = H5Sget_simple_extent_ndims(dataspace);
    int status_n = H5Sget_simple_extent_dims(dataspace, dimTot, NULL);
    return rank==4;
  }

  bool PWParameterSet::hasComplexData(hid_t h_file) 
  {
    ostringstream oss;
    oss << paramTag << "/complex_coefficients";
    int iscomplex=0;
    HDFAttribIO<int> creader(iscomplex);
    creader.read(h_file,oss.str().c_str());
    return iscomplex;
  }

  string PWParameterSet::getTwistAngleName()
  {
    ostringstream oss;
    oss << eigTag << "/" << twistTag << twistIndex << "/twist_angle";
    return oss.str();
  }

  string PWParameterSet::getTwistName()
  {
    return getTwistName(twistIndex);
  }

  string PWParameterSet::getTwistName(int i)
  {
    ostringstream oss;
    oss << twistTag << i;
    return oss.str();
  }

  string PWParameterSet::getBandName(int ib, int ispin)
  {
    ostringstream oss;
    oss << bandTag << ib;
    if(version[1]==10)
    {
      oss << "/" << spinTag << ispin;
    }
    return oss.str();
  }

  string PWParameterSet::getEigVectorName(const string& hg, int ib, int ispin)
  {
    ostringstream oss;
    oss << hg << "/"<< bandTag << ib;
    //if(version[1]==10)
    if(hasSpin)
    {
      oss << "/" << spinTag << ispin;
    }
    oss << "/eigenvector";
    return oss.str();
  }

  string PWParameterSet::getCenterName(const string& hg,int ib)
  {
    ostringstream oss;
    oss << hg << "/"<< bandTag << ib << "/center";
    return oss.str();
  }

  string PWParameterSet::getOriginName(const string& hg,int ib)
  {
    ostringstream oss;
    oss << hg << "/"<< bandTag << ib << "/origin";
    return oss.str();
  }

  string PWParameterSet::getEigVectorName(int ib, int ispin)
  {
    ostringstream oss;
    oss << "/" << eigTag << "/" << twistTag<<twistIndex << "/"<< bandTag << ib;
    //if(version[1]==10)
    if(hasSpin)
    {
      oss << "/" << spinTag << ispin;
    }
    oss << "/eigenvector";
    return oss.str();
  }

  string PWParameterSet::getBandName(int ib)
  {
    ostringstream oss;
    oss << bandTag << ib;
    return oss.str();
  }

  string PWParameterSet::getSpinName(int ispin)
  {
    ostringstream oss;
    oss << spinTag << ispin;
    return oss.str();
  }

  void PWParameterSet::checkVersion(hid_t h)
  {
    hid_t dataset=H5Dopen(h,"version");
    hid_t datatype=H5Dget_type(dataset);   
    H5T_class_t classtype = H5Tget_class(datatype);
    H5Tclose(datatype);
    H5Dclose(dataset);

    if(classtype == H5T_INTEGER)
    {
      HDFAttribIO<TinyVector<int,2> > hdfver(version);
      hdfver.read(h,"version");
    }
    else if(classtype == H5T_FLOAT)
    {
      TinyVector<double,2> vt;
      HDFAttribIO<TinyVector<double,2> > hdfver(vt);
      hdfver.read(h,"version");
      version[0]=int(vt[0]);
      version[1]=int(vt[1]);
    }
    else
    {
      app_error() << "  The type of version is not integer or double." << classtype << endl;
      OHMMS::Controller->abort();
    }

    app_log() << "\tWavefunction HDF version: " << version[0] << "." << version[1] << endl;
    if(version[0] == 0)
    { 
      if(version[1] == 11)
      {
        hasSpin=false;
        paramTag="parameters_0";
        basisTag="basis_1";
        pwTag="planewaves";
        pwMultTag="multipliers";
        eigTag="eigenstates_3"; 
        twistTag="twist_";
        bandTag="band_";
      }
      else if(version[1] == 10)
      {
        pwMultTag="planewaves";
        pwTag="0";
      }
    }

  }

  void PWParameterSet::writeParameters(hid_t gid)
  {
#if defined(QMC_COMPLEX)
    int iscomplex=1;
#else
    int iscomplex=0;
#endif
    hid_t h1= H5Gcreate(gid,"parameters",0);
    HDFAttribIO<int> i1(iscomplex);
    i1.write(h1,"complex_coefficients");
    TinyVector<int,2> v1(0,10);
    HDFAttribIO<TinyVector<int,2> > i2(v1);
    i2.write(gid,"version");
    H5Gclose(h1);
  }
}
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$
 ***************************************************************************/
