// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  Contains the definition of the Compress utility as well as the description
 *  for the spatial::details namespace.
 *
 *  \file   spatial_compress.hpp
 */

#ifndef SPATIAL_COMPRESS_HPP
#define SPATIAL_COMPRESS_HPP

namespace spatial
{
  /**
   *  Defines the namespace that contains all implementation-level
   *  utilities needed by the component of the library.
   *
   *  The types and functions defined within this namespace should not normally
   *  be needed by end-users of the library. If you are currently using one of
   *  them, please refer to the documentation for these functions or object as
   *  there may be more appropriate functions for you to use.
   *
   *  If you do not find a more appropriate function to use, then you may
   *  suggest a feature improvement to the library author.
   */
  namespace details
  {
    /**
     *  Uses the empty base class optimization in order to compress a
     *  potentially empty base class with a member of a class.
     *
     *  Provide the \c base() function to access the base class. Provide the
     *  \c operator() to quickly access the stored member.
     *  \tparam Base The base class that will be compressed through empty member
     *  optimization by the compiler, hopefully.
     *  \tparam Member The member class that will be used as a value for the
     *  compression, this class should not be an empty member class.
     */
    template<typename Base, typename Member>
    struct Compress
      : private Base // Empty member optimization
    {
      //! Uninitialized compressed base.
      Compress() { }

      //! Compressed member with uninitialized base.
      //! \param compressed_base The value of the \c Base type.
      explicit Compress(const Base& compressed_base)
        : Base(compressed_base), _member() { }

      //! Standard initializer with \c Base and \c Member values
      //! \param compressed_base The value of the \c Base type.
      //! \param member The value of the \c Member type.
      Compress(const Base& compressed_base, const Member& member)
        : Base(compressed_base), _member(member) { }

      /**
       *  Accessor to the base class.
       */
      ///@{
      const Base&
      base() const
      { return *static_cast<const Base*>(this); }

      Base&
      base()
      { return *static_cast<Base*>(this); }
      ///@}

      /**
       *  Quick accessor to the member.
       */
      ///@{
      const Member&
      operator()() const
      { return _member; }

      Member&
      operator()()
      { return _member; }
      ///@}

    private:
      //! Storage for the member value.
      Member _member;
    };
  } // namespace details
} // namespace spatial

#endif // SPATIAL_COMPRESS_HPP
