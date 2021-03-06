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


/*! \file trivial_copy.h
 *  \brief Device implementations for copying memory between host and device.
 */

#pragma once

#include <thrust/iterator/iterator_traits.h>
#include <thrust/detail/type_traits.h>
#include <thrust/detail/device/cuda/trivial_copy.h>

namespace thrust
{

namespace detail
{

namespace device
{


// a trivial copy's iterator's value_types match,
// the iterators themselves are normal_iterators
// and the ToIterator's value_type has_trivial_assign
template<typename FromIterator, typename ToIterator>
  struct is_trivial_copy :
    integral_constant<
      bool,
      is_same<
        typename thrust::iterator_value<FromIterator>::type,
        typename thrust::iterator_value<ToIterator>::type
      >::value
      && is_trivial_iterator<FromIterator>::value
      && is_trivial_iterator<ToIterator>::value
      // XXX we need this for correctness, but let's leave it out for speed since our has_trivial_assign needs work
      // && has_trivial_assign<typename thrust::iterator_value<ToIterator>::type>::value
    > {};


inline void trivial_copy_host_to_device(void *dst, const void *src, size_t count)
{
    thrust::detail::device::cuda::trivial_copy_host_to_device(dst, src, count);
}

inline void trivial_copy_device_to_host(void *dst, const void *src, size_t count)
{
    thrust::detail::device::cuda::trivial_copy_device_to_host(dst, src, count);
}

inline void trivial_copy_device_to_device(void *dst, const void *src, size_t count)
{
    thrust::detail::device::cuda::trivial_copy_device_to_device(dst, src, count);
}

} // end namespace device

} // end namespace detail

} // end namespace thrust

