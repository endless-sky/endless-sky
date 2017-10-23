// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file spatial_import_tuple.hpp Contains the macro to pull certain
 *  "tuple" dependancies into the spatial::import namespace.
 *
 *  Depending on the library support and compiler version, the tuple
 *  headers can only be found in TR1 namespace (MSVC 2010 and glibc++ without
 *  c++11 support), only be found in STD name space (libc++), or both (MSVC2012
 *  and glibc++ with C++11 support). This file is written to import these types,
 *  regardless of the library being used, into the spatial::import namespace.
 */

#ifndef SPATIAL_IMPORT_TUPLE
#define SPATIAL_IMPORT_TUPLE

#ifdef SPATIAL_TUPLE_NAMESPACE
#  undef SPATIAL_TUPLE_NAMESPACE
#endif

#if defined(__LIBCPP_VERSION) || __cplusplus >= 201103L
#  include <tuple>
#  define SPATIAL_TUPLE_NAMESPACE std
#elif defined(__GLIBCXX__)
#  include <tr1/tuple>
#  define SPATIAL_TUPLE_NAMESPACE std::tr1
#else
#  ifdef __IBMCPP__
#    define __IBMCPP_TR1__
#  endif
#  include <tuple>
#  define SPATIAL_TUPLE_NAMESPACE std::tr1
#endif

namespace spatial
{
  namespace import
  {
    using SPATIAL_TUPLE_NAMESPACE::tuple;
    using SPATIAL_TUPLE_NAMESPACE::tie;
    using SPATIAL_TUPLE_NAMESPACE::get;
    using SPATIAL_TUPLE_NAMESPACE::make_tuple;
  }
}

#endif // SPATIAL_IMPORT_TUPLE
