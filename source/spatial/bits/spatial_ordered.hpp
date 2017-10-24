// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   spatial_ordered.hpp
 *  Contains the definition of \ordered_iterator.
 */

#ifndef SPATIAL_ORDERED_HPP
#define SPATIAL_ORDERED_HPP

#include <utility> // provides ::std::pair<>
#include "spatial_bidirectional.hpp"
#include "spatial_import_tuple.hpp"

namespace spatial
{
  /**
   *  All elements returned by this iterator are ordered from the smallest to
   *  the largest value of their key's coordinate along a single dimension.
   *
   *  These iterators walk through all items in the container in order from the
   *  lowest to the highest value along a particular dimension. The key
   *  comparator of the container is used for comparision.
   */
  template<typename Container>
  class ordered_iterator
    : public details::Bidirectional_iterator<typename Container::mode_type,
                                             typename Container::rank_type>
  {
  private:
    typedef details::Bidirectional_iterator<typename Container::mode_type,
                                            typename Container::rank_type>
    Base;

  public:
    using Base::node;
    using Base::node_dim;
    using Base::rank;

    //! Alias for the key_compare type used by the iterator.
    typedef typename Container::key_compare key_compare;

    //! Uninitialized iterator.
    ordered_iterator() { }

    /**
     *  This constructor provides a convert a container's iterator into an
     *  ordered iterator pointing at the same node.
     *
     *  \param container   The container to iterate.
     *  \param iter        Use the value of \c iter as the start point for the
     *                     iteration.
     */
    ordered_iterator(Container& container,
                     typename Container::iterator iter)
      : Base(container.rank(), iter.node, depth(iter.node)),
        _cmp(container.key_comp())
    { }

    /**
     *  This constructor builds an ordered iterator from a container's node
     *  and its related depth.
     *
     *  \param container The container to iterate.
     *  \param depth     The depth of the node pointed to by iterator.
     *  \param ptr       Use the value of node as the start point for the
     *                   iteration.
     */
    ordered_iterator(Container& container, dimension_type depth,
                     typename Container::mode_type::node_ptr ptr)
      : Base(container.rank(), ptr, depth), _cmp(container.key_comp())
    { }

    //! Increments the iterator and returns the incremented value. Prefer to
    //! use this form in \c for loops.
    ordered_iterator<Container>& operator++()
    {
      spatial::import::tie(node, node_dim)
        = increment_ordered(node, node_dim, rank(), key_comp());
      return *this;
    }

    //! Increments the iterator but returns the value of the iterator before
    //! the increment. Prefer to use the other form in \c for loops.
    ordered_iterator<Container> operator++(int)
    {
      ordered_iterator<Container> x(*this);
      spatial::import::tie(node, node_dim)
        = increment_ordered(node, node_dim, rank(), key_comp());
      return x;
    }

    //! Decrements the iterator and returns the decremented value. Prefer to
    //! use this form in \c for loops.
    ordered_iterator<Container>& operator--()
    {
      spatial::import::tie(node, node_dim)
        = decrement_ordered(node, node_dim, rank(), key_comp());
      return *this;
    }

    //! Decrements the iterator but returns the value of the iterator before
    //! the decrement. Prefer to use the other form in \c for loops.
    ordered_iterator<Container> operator--(int)
    {
      ordered_iterator<Container> x(*this);
      spatial::import::tie(node, node_dim)
        = decrement_ordered(node, node_dim, rank(), key_comp());
      return x;
    }

    //! Return the key_comparator used by the iterator
    key_compare
    key_comp() const { return _cmp; }

  private:
    //! The related data for the iterator.
    key_compare _cmp;
  };

  /**
   *  All elements returned by this iterator are ordered from the smallest to
   *  the largest value of their key's coordinate along a single dimension,
   *  called the ordered dimension.
   *
   *  Object deferenced by this iterator are always constant.
   */
  template<typename Container>
  class ordered_iterator<const Container>
    : public details::Const_bidirectional_iterator<typename Container::mode_type,
                                                   typename Container::rank_type>
  {
  private:
    typedef details::Const_bidirectional_iterator<typename Container::mode_type,
                                                  typename Container::rank_type>
    Base;

  public:
    using Base::node;
    using Base::node_dim;
    using Base::rank;

    //! Alias for the key_compare type used by the iterator.
    typedef typename Container::key_compare key_compare;

    //! Build an uninitialized iterator.
    ordered_iterator() { }

    /**
     *  This constructor provides a convert a container's iterator into an
     *  ordered iterator pointing at the same node.
     *
     *  \param container   The container to iterate.
     *  \param iter        Use the value of \c iter as the start point for the
     *                     iteration.
     */
    ordered_iterator(const Container& container,
                     typename Container::const_iterator iter)
      : Base(container.rank(), iter.node, depth(iter.node)),
        _cmp(container.key_comp())
    { }

    /**
     *  This constructor builds an ordered iterator from a container's node
     *  and its related depth.
     *
     *  \param container The container to iterate.
     *  \param depth     The depth of the node pointed to by iterator.
     *  \param ptr       Use the value of node as the start point for the
     *                   iteration.
     */
    ordered_iterator
    (const Container& container, dimension_type depth,
     typename Container::mode_type::const_node_ptr ptr)
      : Base(container.rank(), ptr, depth), _cmp(container.key_comp())
    { }

    //! Convertion of mutable iterator into a constant iterator is permitted.
    ordered_iterator(const ordered_iterator<Container>& iter)
      : Base(iter.rank(), iter.node, iter.node_dim), _cmp(iter.key_comp())
    { }

    //! Increments the iterator and returns the incremented value. Prefer to
    //! use this form in \c for loops.
    ordered_iterator<const Container>& operator++()
    {
      spatial::import::tie(node, node_dim)
        = increment_ordered(node, node_dim, rank(), key_comp());
      return *this;
    }

    //! Increments the iterator but returns the value of the iterator before
    //! the increment. Prefer to use the other form in \c for loops.
    ordered_iterator<const Container> operator++(int)
    {
      ordered_iterator<const Container> x(*this);
      spatial::import::tie(node, node_dim)
        = increment_ordered(node, node_dim, rank(), key_comp());
      return x;
    }

    //! Decrements the iterator and returns the decremented value. Prefer to
    //! use this form in \c for loops.
    ordered_iterator<const Container>& operator--()
    {
      spatial::import::tie(node, node_dim)
        = decrement_ordered(node, node_dim, rank(), key_comp());
      return *this;
    }

    //! Decrements the iterator but returns the value of the iterator before
    //! the decrement. Prefer to use the other form in \c for loops.
    ordered_iterator<const Container> operator--(int)
    {
      ordered_iterator<const Container> x(*this);
      spatial::import::tie(node, node_dim)
        = decrement_ordered(node, node_dim, rank(), key_comp());
      return x;
    }

    //! Return the key_comparator used by the iterator
    key_compare
    key_comp() const { return _cmp; }

  private:
    //! The related data for the iterator.
    key_compare _cmp;
  };

  /**
   *  Finds the past-the-end position in \c container for this constant
   *  iterator.
   *
   *  \tparam Container The type of container to iterate.
   *  \param container The container to iterate.
   *  \throw invalid_dimension If the dimension specified is larger than the
   *  dimension from the rank of the container.
   *  \return An iterator pointing to the past-the-end position in the
   *  container.
   *
   *  \consttime
   *
   *  \attention This iterator impose constness constraints on its \c value_type
   *  if the container's is a set and not a map. Iterators on sets prevent
   *  modification of the \c value_type because modifying the key may result in
   *  invalidation of the tree. If the container is a map, only the \c
   *  mapped_type can be modified (the \c second element).
   */
  template <typename Container>
  inline ordered_iterator<Container>
  ordered_end(Container& container)
  {
    return ordered_iterator<Container>
      // At header (dim = rank - 1)
      (container, container.dimension() - 1, container.end().node);
  }

  template <typename Container>
  inline ordered_iterator<const Container>
  ordered_cend(const Container& container)
  { return ordered_end(container); }
  ///@}

  /**
   *  Finds the value in \c container for which its key has the smallest
   *  coordinate over the dimension \c ordered_dim.
   *
   *  \tparam Container The type of container to iterate.
   *  \param container The container to iterate.
   *  \throw invalid_dimension If the dimension specified is larger than the
   *  dimension from the rank of the container.
   *
   *  \fractime
   *
   *  \attention This iterator impose constness constraints on its \c value_type
   *  if the container's is a set and not a map. Iterators on sets prevent
   *  modification of the \c value_type because modifying the key may result in
   *  invalidation of the tree. If the container is a map, only the \c
   *  mapped_type can be modified (the \c second element).
   */
  template <typename Container>
  inline ordered_iterator<Container>
  ordered_begin(Container& container)
  {
    if (container.empty()) return ordered_end(container);
    typename ordered_iterator<Container>::node_ptr node
      = container.end().node->parent;
    dimension_type dim;
    spatial::import::tie(node, dim)
      = first_ordered(node, 0, container.rank(),
                      container.key_comp());
    return ordered_iterator<Container>(container, dim, node);
  }

  template <typename Container>
  inline ordered_iterator<const Container>
  ordered_cbegin(const Container& container)
  { return ordered_begin(container); }
  ///@}

  namespace details
  {
    template <typename Cmp, typename Rank, typename Key>
    inline bool
    order_less(const Cmp& cmp, const Rank rank,
               const Key& a, const Key& b)
    {
      for (dimension_type d = 0; d < rank(); ++d)
        {
          if (cmp(d, a, b)) return true;
          if (cmp(d, b, a)) return false;
        }
      return false;
    }

    /**
     *  Within the sub-tree of node, find the node with the minimum value
     *  according to the iterator's ordering rule.
     *
     *  \tparam Container The type of container to iterate.
     *  \param node The node pointed to by the iterator
     *  \param dim  The dimension of the node pointed to by the iterator.
     *  \param rank The rank of the container which node belongs to.
     *  \param cmp  The comparator used by the container which node belongs to.
     *  \return A pair of node, dimension pointing to the minimum element in
     *  the ordered iteration.
     *
     *  Since Container is based on a \kdtree and \kdtrees exhibit good locality
     *  of reference (for arranging values in space, not for values location in
     *  memory), the function will run with time complexity close to \Onlognk in
     *  practice.
     *
     *  \fractime
     */
    template <typename NodePtr, typename Rank, typename KeyCompare>
    inline std::pair<NodePtr, dimension_type>
    first_ordered(NodePtr node, dimension_type dim, const Rank rank,
                  const KeyCompare& cmp)
    {
      SPATIAL_ASSERT_CHECK(!header(node));
      SPATIAL_ASSERT_CHECK(node != 0);
      NodePtr end = node->parent;
      while (node->left != 0)
        { node = node->left; ++dim; }
      NodePtr best = node;
      dimension_type best_dim = dim;
      // Similarly to mapping iterator, scans all nodes and prune whichever is
      // higher in the dimension 0 only. When the search is over, the lowest
      // in the total order must have been found (since all other dimensions are
      // explored with no pruning).
      for(;;)
        {
          if (node->right != 0
              && ((dim % rank()) > 0
                  || !cmp(0, const_key(best), const_key(node))))
            {
              node = node->right; ++dim;
              while (node->left != 0)
                { node = node->left; ++dim; }
            }
          else
            {
              NodePtr prev_node = node;
              node = node->parent; --dim;
              while (node != end && prev_node == node->right)
                {
                  prev_node = node;
                  node = node->parent; --dim;
                }
              if (node == end) break;
            }
          if (order_less(cmp, rank, const_key(node), const_key(best)))
            { best = node; best_dim = dim; }
        }
      SPATIAL_ASSERT_CHECK(best != 0);
      SPATIAL_ASSERT_CHECK(best != end);
      return std::make_pair(best, best_dim);
    }

    /**
     *  Within the sub-tree of node, find the node with the maximum value
     *  according to the iterator's ordering rule.
     *
     *  \tparam Container The type of container to iterate.
     *  \param node The node pointed to by the iterator
     *  \param dim  The dimension of the node pointed to by the iterator.
     *  \param rank The rank of the container which node belongs to.
     *  \param cmp  The comparator used by the container which node belongs to.
     *  \return A pair of node, dimension pointing to the maximum element in
     *  the ordered iteration.
     *
     *  Since Container is based on a \kdtree and \kdtrees exhibit good locality
     *  of reference (for arranging values in space, not for values location in
     *  memory), the function will run with time complexity close to \Onlognk in
     *  practice.
     *
     *  \fractime
     */
    template <typename NodePtr, typename Rank, typename KeyCompare>
    inline std::pair<NodePtr, dimension_type>
    last_ordered(NodePtr node, dimension_type dim, const Rank rank,
                 const KeyCompare& cmp)
    {
      SPATIAL_ASSERT_CHECK(!header(node));
      SPATIAL_ASSERT_CHECK(node != 0);
      NodePtr end = node->parent;
      while (node->right != 0)
        { node = node->right; ++dim; }
      NodePtr best = node;
      dimension_type best_dim = dim;
      for (;;)
        {
          if (node->left != 0
              && ((dim % rank()) > 0
                  || !cmp(0, const_key(node), const_key(best))))
            {
              node = node->left; ++dim;
              while (node->right != 0)
                { node = node->right; ++dim; }
            }
          else
            {
              NodePtr prev_node = node;
              node = node->parent; --dim;
              while (node != end
                     && prev_node == node->left)
                {
                  prev_node = node;
                  node = node->parent; --dim;
                }
              if (node == end) break;
            }
          if (order_less(cmp, rank, const_key(best), const_key(node)))
            { best = node; best_dim = dim; }
        }
      SPATIAL_ASSERT_CHECK(best != 0);
      SPATIAL_ASSERT_CHECK(best != end);
      return std::make_pair(best, best_dim);
    }

    /**
     *  Move the pointer given in parameter to the next element in the
     *  iteration.
     *
     *  \tparam Container The type of container to iterate.
     *  \param node The node pointed to by the iterator
     *  \param dim  The dimension of the node pointed to by the iterator.
     *  \param rank The rank of the container which node belongs to.
     *  \param cmp  The comparator used by the container which node belongs to.
     *  \return A pair of node, dimension pointing to the next element in the
     *  ordered iteration.
     *
     *  Since Container is based on a \kdtree and \kdtrees exhibit good locality
     *  of reference (for arranging values in space, not for values location in
     *  memory), the function will run with time complexity close to \Onlognk in
     *  practice.
     *
     *  \fractime
     */
    template <typename NodePtr, typename Rank, typename KeyCompare>
    inline std::pair<NodePtr, dimension_type>
    increment_ordered
    (NodePtr node, dimension_type dim, const Rank rank,
     const KeyCompare& cmp)
    {
      SPATIAL_ASSERT_CHECK(!header(node));
      NodePtr orig = node;
      dimension_type orig_dim = dim;
      NodePtr best = 0;
      dimension_type best_dim = 0;
      // Look forward to find an equal or greater next best
      // If an equal next best is found, then no need to look further
      for (;;)
        {
          if (node->right != 0
              && ((dim % rank()) > 0 || best == 0
                  || !cmp(0, const_key(best), const_key(node))))
            {
              node = node->right; ++dim;
              while (node->left != 0
                     && ((dim % rank()) > 0
                         || !cmp(0, const_key(node), const_key(orig))))
                { node = node->left; ++dim; }
            }
          else
            {
              NodePtr prev_node = node;
              node = node->parent; --dim;
              while (!header(node) && prev_node == node->right)
                {
                  prev_node = node;
                  node = node->parent; --dim;
                }
              if (header(node)) break;
            }
          if (order_less(cmp, rank, const_key(orig), const_key(node)))
            {
              if (best == 0
                  || order_less(cmp, rank, const_key(node), const_key(best)))
                { best = node; best_dim = dim; }
            }
          else if (!order_less(cmp, rank, const_key(node), const_key(orig)))
            { return std::make_pair(node, dim); }
        }
      SPATIAL_ASSERT_CHECK(header(node));
      // Maybe there is a better best looking backward...
      node = orig;
      dim = orig_dim;
      for (;;)
        {
          if (node->left != 0
              && (dim % rank() > 0
                  || !cmp(0, const_key(node), const_key(orig))))
            {
              node = node->left; ++dim;
              while (node->right != 0
                     && (dim % rank() > 0 || best == 0
                         || !cmp(0, const_key(best), const_key(node))))
                { node = node->right; ++dim; }
            }
          else
            {
              NodePtr prev_node = node;
              node = node->parent; --dim;
              while (!header(node) && prev_node == node->left)
                {
                  prev_node = node;
                  node = node->parent; --dim;
                }
              if (header(node)) break;
            }
          if (order_less(cmp, rank, const_key(orig), const_key(node))
              && (best == 0
                  || !order_less(cmp, rank, const_key(best), const_key(node))))
            { best = node; best_dim = dim; }
        }
      if (best != 0)
        { node = best; dim = best_dim; }
      SPATIAL_ASSERT_CHECK((best == 0 && header(node))
                           || (best != 0 && !header(node)));
      return std::make_pair(node, dim);
    }

    /**
     *  Move the pointer given in parameter to the previous element in the
     *  iteration.
     *
     *  \tparam Container The type of container to iterate.
     *  \param node The node pointed to by the iterator
     *  \param dim  The dimension of the node pointed to by the iterator.
     *  \param rank The rank of the container which node belongs to.
     *  \param cmp  The comparator used by the container which node belongs to.
     *  \return A pair of node, dimension pointing to the previous element in
     *  the ordered iteration.
     *
     *  Since Container is based on a \kdtree and \kdtrees exhibit good locality
     *  of reference (for arranging values in space, not for values location in
     *  memory), the function will run with time complexity close to \Onlognk in
     *  practice.
     *
     *  \fractime
     */
    template <typename NodePtr, typename Rank, typename KeyCompare>
    inline std::pair<NodePtr, dimension_type>
    decrement_ordered
    (NodePtr node, dimension_type dim, const Rank rank,
     const KeyCompare& cmp)
    {
      if (header(node))
        return last_ordered(node->parent, 0, rank, cmp);
      NodePtr orig = node;
      dimension_type orig_dim = dim;
      NodePtr best = 0;
      dimension_type best_dim = 0;
      // Look forward to find an equal or greater next best
      // If an equal next best is found, then no need to look further
      for (;;)
        {
          if (node->left != 0
              && ((dim % rank()) > 0 || best == 0
                  || !cmp(0, const_key(node), const_key(best))))
            {
              node = node->left; ++dim;
              while (node->right != 0
                     && ((dim % rank()) > 0
                         || !cmp(0, const_key(orig), const_key(node))))
                { node = node->right; ++dim; }
            }
          else
            {
              NodePtr prev_node = node;
              node = node->parent; --dim;
              while (!header(node) && prev_node == node->left)
                {
                  prev_node = node;
                  node = node->parent; --dim;
                }
              if (header(node)) break;
            }
          if (order_less(cmp, rank, const_key(node), const_key(orig)))
            {
              if (best == 0
                  || order_less(cmp, rank, const_key(best), const_key(node)))
                { best = node; best_dim = dim; }
            }
          else if (!order_less(cmp, rank, const_key(orig), const_key(node)))
            { return std::make_pair(node, dim); }
        }
      SPATIAL_ASSERT_CHECK(header(node));
      // Maybe there is a better best looking backward...
      node = orig;
      dim = orig_dim;
      for (;;)
        {
          if (node->right != 0
              && ((dim % rank()) > 0
                  || !cmp(0, const_key(orig), const_key(node))))
            {
              node = node->right; ++dim;
              while (node->left != 0
                     && ((dim % rank()) > 0 || best == 0
                         || !cmp(0, const_key(node), const_key(best))))
                { node = node->left; ++dim; }
            }
          else
            {
              NodePtr prev_node = node;
              node = node->parent; --dim;
              while (!header(node) && prev_node == node->right)
                {
                  prev_node = node;
                  node = node->parent; --dim;
                }
              if (header(node)) break;
            }
          if (order_less(cmp, rank, const_key(node), const_key(orig))
              && (best == 0
                  || !order_less(cmp, rank, const_key(node), const_key(best))))
            { best = node; best_dim = dim; }
        }
      if (best != 0)
        { node = best; dim = best_dim; }
      SPATIAL_ASSERT_CHECK((best == 0 && header(node))
                           || (best != 0 && !header(node)));
      return std::make_pair(node, dim);
    }

  } // namespace details
} // namespace spatial

#endif // SPATIAL_ORDERED_HPP
