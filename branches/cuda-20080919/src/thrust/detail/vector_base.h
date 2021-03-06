/*
 *  Copyright 2008-2009 NVIDIA Corporation
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */


/*! \file vector_base.h
 *  \brief Defines the interface to a base class for
 *         host_vector & device_vector.
 */

#pragma once

#include <thrust/iterator/detail/normal_iterator.h>
#include <thrust/iterator/iterator_traits.h>
#include <thrust/detail/type_traits.h>
#include <thrust/utility.h>
#include <vector>

namespace thrust
{

namespace detail
{

template<typename T, typename Alloc>
  class vector_base
{
  public:
    // typedefs
    typedef T                               value_type;
    typedef typename Alloc::pointer         pointer;
    typedef typename Alloc::const_pointer   const_pointer;
    typedef typename Alloc::reference       reference;
    typedef typename Alloc::const_reference const_reference;
    typedef std::size_t                     size_type;
    typedef typename Alloc::difference_type difference_type;
    typedef Alloc                           allocator_type;

    typedef normal_iterator<pointer>        iterator;
    typedef normal_iterator<const_pointer>  const_iterator;

    /*! This constructor creates an empty vector_base.
     */
    __host__
    vector_base(void);

    /*! This constructor creates a vector_base with copies
     *  of an exemplar element.
     *  \param n The number of elements to initially create.
     *  \param value An element to copy.
     */
    __host__
    explicit vector_base(size_type n, const value_type &value = value_type());

    /*! Copy constructor copies from an exemplar vector_base.
     *  \param v The vector_base to copy.
     */
    __host__
    vector_base(const vector_base &v);

    /*! assign operator makes a copy of an exemplar vector_base.
     *  \param v The vector_base to copy.
     */
    __host__
    vector_base &operator=(const vector_base &v);

    /*! Copy constructor copies from an exemplar vector_base with different
     *  type.
     *  \param v The vector_base to copy.
     */
    template<typename OtherT, typename OtherAlloc>
    __host__
    vector_base(const vector_base<OtherT, OtherAlloc> &v);

    /*! assign operator makes a copy of an exemplar vector_base with different
     *  type.
     *  \param v The vector_base to copy.
     */
    template<typename OtherT, typename OtherAlloc>
    __host__
    vector_base &operator=(const vector_base<OtherT,OtherAlloc> &v);

    /*! Copy constructor copies from an exemplar std::vector.
     *  \param v The std::vector to copy.
     *  XXX TODO: Make this method redundant with a properly templatized constructor.
     *            We would like to copy from a vector whose element type is anything
     *            assignable to value_type.
     */
    template<typename OtherT, typename OtherAlloc>
    __host__
    vector_base(const std::vector<OtherT, OtherAlloc> &v);

    /*! assign operator makes a copy of an exemplar std::vector.
     *  \param v The vector to copy.
     *  XXX TODO: Templatize this assign on the type of the vector to copy from.
     *            We would like to copy from a vector whose element type is anything
     *            assignable to value_type.
     */
    template<typename OtherT, typename OtherAlloc>
    __host__
    vector_base &operator=(const std::vector<OtherT,OtherAlloc> &v);

    /*! This constructor builds a vector_base from a range.
     *  \param first The beginning of the range.
     *  \param last The end of the range.
     */
    template<typename InputIterator>
    __host__
    vector_base(InputIterator first, InputIterator last);

    /*! The destructor erases the elements.
     */
    __host__
    ~vector_base(void);

    /*! \brief Resizes this vector_base to the specified number of elements.
     *  \param new_size Number of elements this vector_base should contain.
     *  \param x Data with which new elements should be populated.
     *  \throw std::length_error If n exceeds max_size().
     *
     *  This method will resize this vector_base to the specified number of
     *  elements.  If the number is smaller than this vector_base's current
     *  size this vector_base is truncated, otherwise this vector_base is
     *  extended and new elements are populated with given data.
     */
    __host__
    void resize(size_type new_size, value_type x = value_type());

    /*! Returns the number of elements in this vector_base.
     */
    __host__ __device__
    size_type size(void) const;

    /*! Returns the size() of the largest possible vector_base.
     *  \return The largest possible return value of size().
     */
    __host__ __device__
    size_type max_size(void) const;

    /*! \brief If n is less than or equal to capacity(), this call has no effect.
     *         Otherwise, this method is a request for allocation of additional memory. If
     *         the request is successful, then capacity() is greater than or equal to
     *         n; otherwise, capacity() is unchanged. In either case, size() is unchanged.
     *  \throw std::length_error If n exceeds max_size().
     */
    __host__
    void reserve(size_type n);

    /*! Returns the number of elements which have been reserved in this
     *  vector_base.
     */
    __host__ __device__
    size_type capacity(void) const;

    /*! \brief Subscript access to the data contained in this vector_dev.
     *  \param n The index of the element for which data should be accessed.
     *  \return Read/write reference to data.
     *
     *  This operator allows for easy, array-style, data access.
     *  Note that data access with this operator is unchecked and
     *  out_of_range lookups are not defined.
     */
    __host__ __device__
    reference operator[](size_type n);

    /*! \brief Subscript read access to the data contained in this vector_dev.
     *  \param n The index of the element for which data should be accessed.
     *  \return Read reference to data.
     *
     *  This operator allows for easy, array-style, data access.
     *  Note that data access with this operator is unchecked and
     *  out_of_range lookups are not defined.
     */
    __host__ __device__
    const_reference operator[](size_type n) const;

    /*! This method returns an iterator pointing to the beginning of
     *  this vector_base.
     *  \return mStart
     */
    __host__ __device__
    iterator begin(void);

    /*! This method returns a const_iterator pointing to the beginning
     *  of this vector_base.
     *  \return mStart
     */
    __host__ __device__
    const_iterator begin(void) const;

    /*! This method returns a const_iterator pointing to the beginning
     *  of this vector_base.
     *  \return mStart
     */
    __host__ __device__
    const_iterator cbegin(void) const;

    /*! This method returns an iterator pointing to one element past the
     *  last of this vector_base.
     *  \return begin() + size().
     */
    __host__ __device__
    iterator end(void);

    /*! This method returns a const_iterator pointing to one element past the
     *  last of this vector_base.
     *  \return begin() + size().
     */
    __host__ __device__
    const_iterator end(void) const;

    /*! This method returns a const_iterator pointing to one element past the
     *  last of this vector_base.
     *  \return begin() + size().
     */
    __host__ __device__
    const_iterator cend(void) const;

    /*! This method returns a const_reference referring to the first element of this
     *  vector_base.
     *  \return The first element of this vector_base.
     */
    __host__ __device__
    const_reference front(void) const;

    /*! This method returns a reference pointing to the first element of this
     *  vector_base.
     *  \return The first element of this vector_base.
     */
    __host__ __device__
    reference front(void);

    /*! This method returns a const reference pointing to the last element of
     *  this vector_base.
     *  \return The last element of this vector_base.
     */
    __host__ __device__
    const_reference back(void) const;

    /*! This method returns a reference referring to the last element of
     *  this vector_dev.
     *  \return The last element of this vector_base.
     */
    __host__ __device__
    reference back(void);

    /*! This method resizes this vector_base to 0.
     */
    __host__
    void clear(void);

    /*! This method returns true iff size() == 0.
     *  \return true if size() == 0; false, otherwise.
     */
    __host__ __device__
    bool empty(void) const;

    /*! This method appends the given element to the end of this vector_base.
     *  \param x The element to append.
     */
    __host__
    void push_back(const value_type &x);

    /*! This method erases the last element of this vector_base, invalidating
     *  all iterators and references to it.
     */
    __host__
    void pop_back(void);

    /*! This method swaps the contents of this vector_base with another vector_base.
     *  \param v The vector_base with which to swap.
     */
    __host__ __device__
    void swap(vector_base &v);

    /*! This method removes the element at position pos.
     *  \param pos The position of the element of interest.
     *  \return An iterator pointing to the new location of the element that followed the element
     *          at position pos.
     */
    __host__
    iterator erase(iterator pos);

    /*! This method removes the range of elements [first,last) from this vector_base.
     *  \param first The beginning of the range of elements to remove.
     *  \param last The end of the range of elements to remove.
     *  \return An iterator pointing to the new location of the element that followed the last
     *          element in the sequence [first,last).
     */
    __host__
    iterator erase(iterator first, iterator last);

    /*! This method inserts a single copy of a given exemplar value at the
     *  specified position in this vector_base.
     *  \param position The insertion position.
     *  \param x The exemplar element to copy & insert.
     *  \return An iterator pointing to the newly inserted element.
     */
    __host__
    iterator insert(iterator position, const T &x); 

    /*! This method inserts a copy of an exemplar value to a range at the
     *  specified position in this vector_base.
     *  \param position The insertion position
     *  \param n The number of insertions to perform.
     *  \param x The value to replicate and insert.
     */
    __host__
    void insert(iterator position, size_type n, const T &x);

    /*! This method inserts a copy of an input range at the specified position
     *  in this vector_base.
     *  \param position The insertion position.
     *  \param first The beginning of the range to copy.
     *  \param last  The end of the range to copy.
     *
     *  \tparam InputIterator is a model of <a href="http://www.sgi.com/tech/stl/InputIterator.html>Input Iterator</a>,
     *                        and \p InputIterator's \c value_type is a model of <a href="http://www.sgi.com/tech/stl/Assignable.html">Assignable</a>.
     */
    template<typename InputIterator>
    __host__
    void insert(iterator position, InputIterator first, InputIterator last);

    /*! This version of \p assign replicates a given exemplar
     *  \p n times into this vector_base.
     *  \param n The number of times to copy \p x.
     *  \param x The exemplar element to replicate.
     */
    __host__
    void assign(size_type n, const T &x);

    /*! This version of \p assign makes this vector_base a copy of a given input range.
     *  \param first The beginning of the range to copy.
     *  \param last  The end of the range to copy.
     *
     *  \tparam InputIterator is a model of <a href="http://www.sgi.com/tech/stl/InputIterator">Input Iterator</a>.
     */
    template<typename InputIterator>
    __host__
    void assign(InputIterator first, InputIterator last);

  protected:
    // An iterator pointing to the first element of this vector_base.
    iterator mBegin;

    // The size of this vector_base, in number of elements.
    size_type mSize;

    // The capacity of this vector_base, in number of elements.
    size_type mCapacity;

    // Our allocator
    allocator_type mAllocator;

  private:
    // these methods resolve the ambiguity of the constructor template of form (Iterator, Iterator)
    template<typename IteratorOrIntegralType>
      void init_dispatch(IteratorOrIntegralType begin, IteratorOrIntegralType end, false_type); 

    template<typename IteratorOrIntegralType>
      void init_dispatch(IteratorOrIntegralType n, IteratorOrIntegralType value, true_type); 

    template<typename InputHostIterator>
      void range_init(InputHostIterator first, InputHostIterator last, true_type);

    template<typename ForwardIterator>
      void range_init(ForwardIterator first, ForwardIterator last, false_type);

    void fill_init(size_type n, const T &x);

    // these methods resolve the ambiguity of the insert() template of form (iterator, InputIterator, InputIterator)
    template<typename InputIteratorOrIntegralType>
      void insert_dispatch(iterator position, InputIteratorOrIntegralType first, InputIteratorOrIntegralType last, false_type);

    // these methods resolve the ambiguity of the insert() template of form (iterator, InputIterator, InputIterator)
    template<typename InputIteratorOrIntegralType>
      void insert_dispatch(iterator position, InputIteratorOrIntegralType n, InputIteratorOrIntegralType x, true_type);

    // this method performs insertion from a range
    template<typename InputIterator>
      void range_insert(iterator position, InputIterator first, InputIterator last);

    // this method performs insertion from a fill value
    void fill_insert(iterator position, size_type n, const T &x);

    // these methods resolve the ambiguity of the assign() template of form (InputIterator, InputIterator)
    template<typename InputIterator>
      void assign_dispatch(InputIterator first, InputIterator last, false_type);

    // these methods resolve the ambiguity of the assign() template of form (InputIterator, InputIterator)
    template<typename Integral>
      void assign_dispatch(Integral n, Integral x, true_type);

    // this method performs assignment from a range
    template<typename InputIterator>
      void range_assign(InputIterator first, InputIterator last);

    // this method performs assignment from a range of ForwardIterators
    template<typename ForwardIterator>
      void range_assign(ForwardIterator first, ForwardIterator last, false_type);

    // this method performs assignment from a range of InputHostIterators
    template<typename InputHostIterator>
      void range_assign(InputHostIterator first, InputHostIterator last, true_type);

    // this method performs assignment from a fill value
    void fill_assign(size_type n, const T &x);

    // this method allocates new storage and construct copies the given range
    template<typename ForwardIterator>
    void allocate_and_copy(size_type requested_size,
                           ForwardIterator first, ForwardIterator last,
                           size_type &allocated_size,
                           iterator &new_storage);
}; // end vector_base

} // end detail

/*! This function assigns the contents of vector a to vector b and the
 *  contents of vector b to vector a.
 *
 *  \param a The first vector of interest. After completion, the contents
 *           of b will be returned here.
 *  \param b The second vector of interest. After completion, the contents
 *           of a will be returned here.
 */
template<typename T, typename Alloc>
  void swap(detail::vector_base<T,Alloc> &a,
            detail::vector_base<T,Alloc> &b);

} // end thrust

#include <thrust/detail/vector_base.inl>

