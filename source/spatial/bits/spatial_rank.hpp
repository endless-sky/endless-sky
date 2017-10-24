// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   spatial_rank.hpp
 *  Defines \ref spatial::details::Static_rank and
 *  \ref spatial::details::Dynamic_rank as well as associated functions on rank.
 */

#ifndef SPATIAL_RANK_HPP
#define SPATIAL_RANK_HPP

#include "spatial_node.hpp" // for modulo()

namespace spatial
{
  namespace details
  {
    /**
     *  The dimension value is set by a template value, thus consuming
     *  no memory.
     *
     *  \tparam Value The magnitude of the rank.
     */
    template <dimension_type Value>
    struct Static_rank
    {
      //! Returns the dimension for the rank specified in the template parameter
      //! \c Value.
      dimension_type operator()() const
      { return Value; }
    };

    /**
     *  The dimension value is stored by a member of the object, but can be
     *  modified at run time.
     */
    struct Dynamic_rank
    {
      //! Returns the dimension for the rank stored in \c _rank
      dimension_type operator()() const
      { return _rank; }

      //! Build a rank with a default dimension of 1.
      //! \param rank The magnitude of the rank.
      explicit
      Dynamic_rank(dimension_type rank = 1)
        : _rank(rank)
      { }

    private:
      //! The value that stores the rank dimension.
      dimension_type _rank;
    };

    /**
     *  Increment dimension \c node_dim, given \c rank.
     *  \tparam Rank Either \static_rank or \dynamic_rank.
     *  \param rank The magnitude of the rank.
     *  \param node_dim The value of the dimension for the node.
     */
    template<typename Rank>
    inline dimension_type
    incr_dim(Rank rank, dimension_type node_dim)
    { return (node_dim + 1) % rank(); }

    /**
     *  Decrement dimension \c node_dim, given \c rank.
     *  \tparam Rank Either \static_rank or \dynamic_rank.
     *  \param rank The magnitude of the rank.
     *  \param node_dim The value of the dimension for the node.
     */
    template<typename Rank>
    inline dimension_type
    decr_dim(Rank rank, dimension_type node_dim)
    { return (rank() + node_dim - 1) % rank(); }

    /**
     *  Returns the modulo of a node's heigth by a container's rank. This, in
     *  effect, gives the current dimension along which the node's invarient is
     *  evaluated.
     *
     *  If \c x points to the header, by convention the highest dimension for a
     *  node invariant is returned.
     *
     *  \tparam Link A model of \linkmode.
     *  \tparam Rank Either \static_rank or \dynamic_rank.
     *  \param x A constant pointer to a node.
     *  \param r The rank used in the container.
     */
    template <typename Link, typename Rank>
    inline dimension_type
    modulo(const Node<Link>* x, Rank r)
    {
      dimension_type d = r() - 1;
      while(!header(x)) { ++d; x = x->parent; }
      return d % r();
    }
  }
}

#endif // SPATIAL_RANK_HPP
