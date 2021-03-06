#ifndef QMCPLUSPLUS_HDF_HYPERSLAB_IO_H
#define QMCPLUSPLUS_HDF_HYPERSLAB_IO_H
#include <type_traits/container_proxy.h>
#include <io/hdf_datatype.h>
#include <io/hdf_dataspace.h>
#include <io/hdf_dataproxy.h>
namespace qmcplusplus
{
  /** class to use hyperslab with a serialized container
   *
   * container_proxy<CT> handles the size and datatype
   */
  template<typename CT, unsigned MAXDIM>
    struct hyperslab_proxy: public container_proxy<CT>
  {
    ///determine the size of value_type
    enum {element_size=container_proxy<CT>::DIM};
    ///rank of hyperslab
    int slab_rank;
    ///local dimension of the hyperslab
    TinyVector<hsize_t,MAXDIM+1> slab_dims;
    ///offset of the hyperslab
    TinyVector<hsize_t,MAXDIM+1> slab_offset;
    ///1D
    hyperslab_proxy(CT& a): container_proxy<CT>(a)
    {
      slab_dims[0]=this->size();
      if(element_size>1) slab_dims[1]=element_size;
    }

    template<typename IC>
    inline hyperslab_proxy(CT& a, const IC& dims_in):container_proxy<CT>(a)
    {
      slab_rank=dims_in.size();
      for(int i=0; i<dims_in.size(); ++i)
        slab_dims[i]=static_cast<hsize_t>(dims_in[i]);
      if(element_size>1) 
      {
        slab_dims[slab_rank]=element_size;
        slab_rank+=1;
      }
    }

    /** return the size of the i-th dimension
     * @param i dimension
     */
    inline hsize_t size(int i) const { return (i>MAXDIM)?0:slab_dims[i];}

    inline void change_shape() { this->resize(slab_dims.data()); }

  };

  template<typename CT, unsigned MAXDIM>
    struct h5data_proxy<hyperslab_proxy<CT,MAXDIM> >
    {
      hyperslab_proxy<CT,MAXDIM>& ref_;
      h5data_proxy(hyperslab_proxy<CT,MAXDIM>& a): ref_(a){}
      inline bool read(hid_t grp, const std::string& aname, hid_t xfer_plist=H5P_DEFAULT)
      {
        int rank=ref_.slab_rank;
        if(!get_space(grp,aname,rank,ref_.slab_dims.data(),true)) 
        { 
          ref_.change_shape(); 
        }
        return h5d_read(grp,aname,ref_.data(),xfer_plist);
      }
      inline bool write(hid_t grp, const std::string& aname, hid_t xfer_plist=H5P_DEFAULT)
      {
        return h5d_write(grp,aname.c_str(),ref_.slab_rank,ref_.slab_dims.data(),ref_.data(),xfer_plist);
      }
    };
}
#endif
