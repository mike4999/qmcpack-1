//////////////////////////////////////////////////////////////////
// (c) Copyright 2003 by Jeongnim Kim and Jordan Vincent
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
#include "Utilities/OhmmsInfo.h"
#include "Numerics/LibxmlNumericIO.h"
#include "Numerics/HDFNumericAttrib.h"
#include "Numerics/GaussianBasisSet.h"
#include "Numerics/SlaterBasisSet.h"
#include "Numerics/Transform2GridFunctor.h"
#include "Numerics/OneDimCubicSpline.h"
#include "QMCFactory/OneDimGridFactory.h"
#include "QMCWaveFunctions/MolecularOrbitals/NGOBuilder.h"
namespace qmcplusplus {

  NGOBuilder::NGOBuilder(xmlNodePtr cur): 
    Normalized(true),m_rcut(-1.0), m_infunctype("Gaussian"), m_fileid(-1){
      if(cur != NULL) {
        putCommon(cur);
      }
  }
  NGOBuilder::~NGOBuilder()
  {
    if(m_fileid>-1) H5Fclose(m_fileid);
  }

  bool 
    NGOBuilder::putCommon(xmlNodePtr cur) {
      string normin("yes");
      string afilename("0");
      OhmmsAttributeSet aAttrib;
      aAttrib.add(normin,"normalized");
      aAttrib.add(m_infunctype,"type");
      aAttrib.add(afilename,"href"); aAttrib.add(afilename,"src");
      bool success=aAttrib.put(cur);

      //set the noramlization
      Normalized=(normin=="yes");

      if(afilename.find("h5")<afilename.size())
        m_fileid = H5Fopen(afilename.c_str(),H5F_ACC_RDWR,H5P_DEFAULT);
      else
        m_fileid=-1;

      return success;
    }

  void 
    NGOBuilder::setOrbitalSet(CenteredOrbitalType* oset, const std::string& acenter) { 
    m_orbitals = oset;
    m_species = acenter;
  }

  bool 
    NGOBuilder::addGrid(xmlNodePtr cur) {
    if(!m_orbitals) {
      ERRORMSG("m_orbitals, SphericalOrbitals<ROT,GT>*, is not initialized")
      return false;
    }

    GridType *agrid = OneDimGridFactory::createGrid(cur);
    m_orbitals->Grids.push_back(agrid);
    return true;
  }

  /** Add a new Slater Type Orbital with quantum numbers \f$(n,l,m,s)\f$ 
   * \param cur  the current xmlNode to be processed
   * \param nlms a vector containing the quantum numbers \f$(n,l,m,s)\f$
   * \return true is succeeds
   *
   This function puts the STO on a logarithmic grid and calculates the boundary 
   conditions for the 1D Cubic Spline.  The derivates at the endpoint 
   are assumed to be all zero.  Note: for the radial orbital we use
   \f[ f(r) = \frac{R(r)}{r^l}, \f] where \f$ R(r) \f$ is the usual
   radial orbital and \f$ l \f$ is the angular momentum.
  */
  bool
    NGOBuilder::addRadialOrbital(xmlNodePtr cur, const QuantumNumberType& nlms) {

    if(!m_orbitals) {
      ERRORMSG("m_orbitals, SphericalOrbitals<ROT,GT>*, is not initialized")
      return false;
    }

    string radtype(m_infunctype);
    string dsname("0");

    OhmmsAttributeSet aAttrib;
    aAttrib.add(radtype,"type");
    aAttrib.add(m_rcut,"rmax");
    aAttrib.add(dsname,"ds");
    aAttrib.put(cur);

    //const xmlChar *tptr = xmlGetProp(cur,(const xmlChar*)"type");
    //if(tptr) radtype = (const char*)tptr;

    //tptr = xmlGetProp(cur,(const xmlChar*)"rmax");
    //if(tptr) m_rcut = atof((const char*)tptr);

    int lastRnl = m_orbitals->Rnl.size();

    m_nlms = nlms;
    if(radtype == "Gaussian" || radtype == "GTO") {
      addGaussian(cur);
    } else if(radtype == "Slater" || radtype == "STO") {
      addSlater(cur);
    } else if(radtype == "Pade") {
      app_error() << "  Any2GridBuilder::addPade is disabled." << endl;
      APP_ABORT("NGOBuilder::addRadialOrbital");
      //addPade(cur);
    } else 
    {
      addNumerical(cur,dsname);
    }


    if(lastRnl && m_orbitals->Rnl.size()> lastRnl) {
      //LOGMSG("\tSetting GridManager of " << lastRnl << " radial orbital to false")
      m_orbitals->Rnl[lastRnl]->setGridManager(false);
    }

    return true;
  }

  void NGOBuilder::addGaussian(xmlNodePtr cur) {

    int L= m_nlms[1];
    GaussianCombo<RealType> gset(L,Normalized);
    gset.putBasisGroup(cur);

    GridType* agrid = m_orbitals->Grids[0];
    RadialOrbitalType *radorb = new OneDimCubicSpline<RealType>(agrid);

    if(m_rcut<0) m_rcut = agrid->rmax();
    Transform2GridFunctor<GaussianCombo<RealType>,RadialOrbitalType> transform(gset, *radorb);
    transform.generate(agrid->rmin(),m_rcut,agrid->size());

    m_orbitals->Rnl.push_back(radorb);
    m_orbitals->RnlID.push_back(m_nlms);
  }

  void NGOBuilder::addSlater(xmlNodePtr cur) {

    ////pointer to the grid
    GridType* agrid = m_orbitals->Grids[0];
    RadialOrbitalType *radorb = new OneDimCubicSpline<RealType>(agrid);

    SlaterCombo<RealType> sto(m_nlms[1],Normalized);
    sto.putBasisGroup(cur);

    //spline the slater type orbital
    Transform2GridFunctor<SlaterCombo<RealType>,RadialOrbitalType> transform(sto, *radorb);
    transform.generate(agrid->rmin(), agrid->rmax(),agrid->size());
    //transform.generate(agrid->rmax());
    //add the radial orbital to the list
    m_orbitals->Rnl.push_back(radorb);
    m_orbitals->RnlID.push_back(m_nlms);

  }

  void NGOBuilder::addNumerical(xmlNodePtr cur, const string& dsname) 
  {
    int imin = 0;
    OhmmsAttributeSet aAttrib;
    aAttrib.add(imin,"imin");
    aAttrib.put(cur);

    char grpname[128];
    sprintf(grpname,"radial_basis_states/%s",dsname.c_str());
    hid_t group_id_orb=H5Gopen(m_fileid,grpname);
    int rinv_p=0;
    HDFAttribIO<int> ir(rinv_p);
    ir.read(group_id_orb,"power");

    Vector<double> rad_orb;
    HDFAttribIO<Vector<double> > rin(rad_orb);
    rin.read(group_id_orb,"radial_orbital");

    H5Gclose(group_id_orb);

    GridType* agrid = m_orbitals->Grids[0];
    if(rinv_p != 0)
      for(int ig=0; ig<rad_orb.size(); ig++) 
        rad_orb[ig] *= std::pow(agrid->r(ig),-rinv_p);

    //last valid index for radial grid
    int imax = rad_orb.size()-1;
    RadialOrbitalType *radorb = new OneDimCubicSpline<RealType>(agrid,rad_orb);
    //calculate boundary condition, assume derivates at endpoint are 0.0
    RealType yprime_i = rad_orb[imin+1]-rad_orb[imin];
    if(std::abs(yprime_i)<1e-10)  yprime_i = 0.0;
    yprime_i /= (agrid->r(imin+1)-agrid->r(imin)); 
    //set up 1D-Cubic Spline
    radorb->spline(imin,yprime_i,imax,0.0);

    m_orbitals->Rnl.push_back(radorb);
    m_orbitals->RnlID.push_back(m_nlms);

    //ofstream dfile("spline.dat");
    //dfile.setf(std::ios::scientific, std::ios::floatfield);
    //for(int ig=imin; ig<radorb->size(); ig++) {
    //  RealType dr = (radorb->r(ig+1)- radorb->r(ig))/5.0;
    //  RealType _r = radorb->r(ig),y,dy,d2y;
    //  while(_r<radorb->r(ig+1)) {
    //    //Do not need this anymore
    //    //radorb->setgrid(_r);
    //    y = radorb->evaluate(_r,1.0/_r,dy,d2y);
    //    dfile << setw(15) << _r << setw(20) << setprecision(12) << y 
    //          << setw(20) << dy << setw(20) << d2y
    //          << endl;
    //    _r+=dr;
    //  }
    //}

    //cout << " Power " << rinv_p << " size=" << rad_orb.size() << endl;
    //APP_ABORT("NGOBuilder::addNumerical");
  }

  template<class T>
  struct PadeOrbital: public OptimizableFunctorBase<T> {
  
    typedef typename OptimizableFunctorBase<T>::value_type value_type;
    typedef typename OptimizableFunctorBase<T>::real_type real_type;
    real_type a0,a1,a2,a3,rcut;
    std::string  nodeName;
  
    explicit 
      PadeOrbital(const char* node_name="radfunc"):
        a0(1.0),a1(-0.5),a2(0.0),a3(-0.2),rcut(4.0),nodeName(node_name){}
  
    ~PadeOrbital(){ }
  
    void reset() {}
  
    inline real_type f(real_type r) {
      return a0*std::exp((a1+a2*r)*r/(1.0e0+a3*r));
    }
  
    inline real_type df(real_type r) {
      value_type t = 1.0/(1.0e0+a3*r);
      value_type z=(a1+a2*r)*r*t;
      value_type res = a0*std::exp(z);
      return res*(a1+2.0*a2*r-z*a3)*t;
    }
  
    bool putBasisGroup(xmlNodePtr cur) {
      cur = cur->children;
      while(cur != NULL) {
        string cname((const char*)cur->name);
        if(cname == nodeName) {
          OhmmsAttributeSet radAttrib;
          radAttrib.add(a0,"a0"); radAttrib.add(a1,"a1"); 
          radAttrib.add(a2,"a2"); radAttrib.add(a3,"a3");
          radAttrib.put(cur);
          rcut = -1.0/a3-std::numeric_limits<T>::epsilon();
          LOGMSG("Pade compoenent (a0,a1,a2,a3,rcut) = " << a0 << " " << a1 << " " << a2 << " " << a3 << " " << rcut)
        }
        cur=cur->next;
      }
      return true;
    }

    bool put(xmlNodePtr cur) { return true;}

    void addOptimizables( VarRegistry<real_type>& vlist){}
  };

  void NGOBuilder::addPade(xmlNodePtr cur) {

    //GridType* agrid = m_orbitals->Grids[0];
    //RadialOrbitalType *radorb = new OneDimCubicSpline<RealType>(agrid);

    //PadeOrbital<RealType> pade;
    //pade.putBasisGroup(cur);

    ////spline the slater type orbital
    //Transform2GridFunctor<PadeOrbital<RealType>,RadialOrbitalType> transform(pade, *radorb);
    //if(pade.rcut>0) 
    //  transform.generate(pade.rcut);
    //else 
    //  transform.generate(agrid->rmax());
    ////add the radial orbital to the list
    //m_orbitals->Rnl.push_back(radorb);
    //m_orbitals->RnlID.push_back(m_nlms);
  }
}
/***************************************************************************
 * $RCSfile$   $Author$
 * $Revision$   $Date$
 * $Id$ 
 ***************************************************************************/
