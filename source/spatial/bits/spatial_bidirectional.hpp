// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   spatial_bidirectional.hpp
 *  Defines the base bidirectional iterators.hpp
 */

#ifndef SPATIAL_BIDIRECTIONAL_HPP
#define SPATIAL_BIDIRECTIONAL_HPP

#include "spatial_node.hpp"

namespace spatial
{
  namespace details
  {
    /**
     *  A common template for bidirectional iterators that work on identical
     *  \ref linkmode_concept "modes of linking".
     *
     *  This template defines all the basic features of a bidirectional
     *  iterator for this library.
     *
     *  \tparam Link      A model of \linkmode.
     *  \tparam Rank      The rank of the iterator.
     */
    template <typename Link, typename Rank>
    class Bidirectional_iterator : private Rank
    {
    public:
      //! The \c value_type can receive a copy of the reference pointed to be
      //! the iterator.
      typedef typename mutate<typename Link::value_type>::type value_type;
      //! The reference type of the object pointed to by the iterator.
      typedef typename Link::value_type&           reference;
      //! The pointer type of the object pointed to by the iterator.
      typedef typename Link::value_type*           pointer;
      //! The difference_type returned by the distance between 2 iterators.
      typedef std::ptrdiff_t                       difference_type;
      //! The iterator category that is always \c Bidirectional_iterator_tag.
      typedef std::bidirectional_iterator_tag      iterator_category;
      //! The type for the node pointed to by the iterator.
      typedef typename Link::node_ptr              node_ptr;
      //! The type of rank used by the iterator.
      typedef Rank                                 rank_type;
      //! The invariant category of the the iterator
      typedef typename Link::invariant_category    invariant_category;

      //! Build an uninitialized iterator
      Bidirectional_iterator() { }

      //! Initialize the node at construction time
      Bidirectional_iterator(const Rank& rank_, node_ptr node_,
                             dimension_type node_dim_)
        : Rank(rank_), node(node_), node_dim(node_dim_) { }

      //! Returns the reference to the value pointed to by the iterator.
      reference operator*()
      { return value(node); }

      //! Returns a pointer to the value pointed to by the iterator.
      pointer operator->()
      { return &value(node); }

      /**
       *  A bidirectional iterator can be compared with a node iterator if they
       *  work on identical \ref linkmode_concept "linking modes".
       *
       *  \param x The iterator on the right.
       */
      bool operator==(const Const_node_iterator<Link>& x) const
      { return node == x.node; }

      /**
       *  A bidirectional iterator can be compared for inequality with a node
       *  iterator if they work on identical \ref linkmode_concept "linking modes".
       *
       *  \param x The iterator on the right.
       */
      bool operator!=(const Const_node_iterator<Link>& x) const
      { return node != x.node; }

      ///@{
      /**
       *  This iterator can be casted silently into a container iterator. You can
       *  therefore use this iterator as an argument to the erase function of
       *  the container, for example.
       *
       *  \warning When using this iterator as an argument to the erase function
       *  of the container, this iterator will get invalidated after erase.
       */
      operator Node_iterator<Link>() const
      { return Node_iterator<Link>(node); }

      operator Const_node_iterator<Link>() const
      { return Const_node_iterator<Link>(node); }
      ///@}

      /**
       *  Return the current Rank type used by the iterator.
       */
      const rank_type& rank() const { return static_cast<const Rank&>(*this); }

      /**
       *  Return the number of dimensions stored by the Rank of the iterator.
       */
      dimension_type dimension() const
      { return static_cast<const Rank&>(*this)(); }

      /**
       *  The pointer to the current node.
       *
       *  Modifying this attribute can potentially invalidate the iterator. Do
       *  not modify this attribute unless you know what you're doing. This
       *  iterator must always point to a valid node in the tree or to the end.
       */
      node_ptr node;

      /**
       *  The dimension of the current node.
       *
       *  Modifying this attribute can potentially invalidate the iterator. Do
       *  not modify this attribute unless you know what you're doing. This
       *  iterator must always point to a valid node in the tree or to the end.
       */
      dimension_type node_dim;
    };

    /**
     *  A common template for constant bidirectional iterators that work on
     *  identical \ref linkmode_concept "modes of linking".
     *
     *  This template defines all the basic features of a bidirectional
     *  iterator for this library.
     *
     *  \tparam Link      A type that is a model of \linkmode.
     *  \tparam Rank      The rank of the iterator.
     */
    template <typename Link, typename Rank>
    class Const_bidirectional_iterator : private Rank
    {
    public:
      //! The \c value_type can receive a copy of the reference pointed to be
      //! the iterator.
      typedef typename mutate<typename Link::value_type>::type value_type;
      //! The reference type of the object pointed to by the iterator.
      typedef const typename Link::value_type&     reference;
      //! The pointer type of the object pointed to by the iterator.
      typedef const typename Link::value_type*     pointer;
      //! The difference_type returned by the distance between 2 iterators.
      typedef std::ptrdiff_t                       difference_type;
      //! The iterator category that is always \c Bidirectional_iterator_tag.
      typedef std::bidirectional_iterator_tag      iterator_category;
      //! The type for the node pointed to by the iterator.
      typedef typename Link::const_node_ptr        node_ptr;
      //! The type of rank used by the iterator.
      typedef Rank                                 rank_type;
      //! The invariant category of the the iterator
      typedef typename Link::invariant_category    invariant_category;

      //! Build an uninitialized iterator
      Const_bidirectional_iterator() { }

      //! Initialize the node at construction time
      Const_bidirectional_iterator(const Rank& rank_, node_ptr node_,
                                   dimension_type node_dim_)
        : Rank(rank_), node(node_), node_dim(node_dim_) { }

      //! Returns the reference to the value pointed to by the iterator.
      reference operator*()
      { return const_value(node); }

      //! Returns a pointer to the value pointed to by the iterator.
      pointer operator->()
      { return &const_value(node); }

      /**
       *  A bidirectional iterator can be compared with a node iterator if they
       *  work on identical \ref linkmode_concept "linking modes".
       *
       *  \param x The iterator on the right.
       */
      bool operator==(const Const_node_iterator<Link>& x) const
      { return node == x.node; }

      /**
       *  A bidirectional iterator can be compared for inequality with a node
       *  iterator if they work on identical \ref linkmode_concept "linking
       *  modes".
       *
       *  \param x The iterator on the right.
       */
      bool operator!=(const Const_node_iterator<Link>& x) const
      { return node != x.node; }

      /**
       *  Children of this iterator can be casted silently into a container
       *  iterator. You can therefore use this iterator as an argument to the
       *  other function of the container that are working on iterators.
       */
      operator Const_node_iterator<Link>() const
      { return Const_node_iterator<Link>(node); }

      /**
       *  Return the current Rank type used by the iterator.
       */
      rank_type rank() const { return static_cast<const Rank>(*this); }

      /**
       *  Return the current number of dimensions given by the Rank of the
       *  iterator.
       */
      dimension_type dimension() const
      { return static_cast<const Rank&>(*this)(); }

      /**
       *  The pointer to the current node.
       *
       *  Modifying this attribute can potentially invalidate the iterator. Do
       *  not modify this attribute unless you know what you're doing. This
       *  iterator must always point to a valid node in the tree or to the end.
       */
      node_ptr node;

      /**
       *  The dimension of the current node.
       *
       *  Modifying this attribute can potentially invalidate the iterator. Do
       *  not modify this attribute unless you know what you're doing. This
       *  iterator must always point to a valid node in the tree or to the end.
       */
      dimension_type node_dim;
    };

  } // namespace details
} // namespace spatial

#endif // SPATIAL_BIDIRECTIONAL_HPP
