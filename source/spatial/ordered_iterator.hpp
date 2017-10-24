// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   ordered_iterator.hpp
 *  Provides \ordered_iterator and all the functions around it.
 */

#ifndef SPATIAL_ORDERED_ITERATOR_HPP
#define SPATIAL_ORDERED_ITERATOR_HPP

#include "spatial.hpp"
#include "bits/spatial_ordered.hpp"

namespace spatial
{
  /**
   *  This structure defines a pair of mutable \ordered_iterator.
   *
   *  \tparam Ct The container to which these iterator relate to.
   *  \see ordered_iterator
   */
  template <typename Container>
  struct ordered_iterator_pair
    : std::pair<ordered_iterator<Container>, ordered_iterator<Container> >
  {
    /**
     *  A pair of iterators that represents a range (that is: a range of
     *  elements to iterate, and not an orthogonal range).
     */
    typedef std::pair<ordered_iterator<Container>,
                      ordered_iterator<Container> > Base;

    //! \empty
    ordered_iterator_pair() { }

    //! Regular constructor that builds a ordered_iterator_pair out of 2
    //! \ordered_iterator.
    ordered_iterator_pair(const ordered_iterator<Container>& a,
                          const ordered_iterator<Container>& b)
      : Base(a, b) { }
  };

  /**
   *  This structure defines a pair of constant ordered iterator.
   *
   *  \tparam Ct The container to which these iterator relate to.
   *  \see ordered_iterator
   */
  template <typename Container>
  struct ordered_iterator_pair<const Container>
    : std::pair<ordered_iterator<const Container>,
                ordered_iterator<const Container> >
  {
    /**
     *  A pair of iterators that represents a range (that is: a range of
     *  elements to iterate, and not an orthogonal range).
     */
    typedef std::pair<ordered_iterator<const Container>,
                      ordered_iterator<const Container> > Base;

    //! \empty
    ordered_iterator_pair() { }

    //! Regular constructor that builds a ordered_iterator_pair out of 2
    //! \ordered_iterator.
    ordered_iterator_pair(const ordered_iterator<const Container>& a,
                          const ordered_iterator<const Container>& b)
      : Base(a, b) { }

    //! Convert a mutable ordered iterator pair into a const ordered iterator
    //!pair.
    ordered_iterator_pair(const ordered_iterator_pair<Container>& p)
      : Base(p.first, p.second) { }
  };

  /**
   *  Returns a pair of iterator on the first and the last value in the range
   *  that can be iterated. This function is convenient to use with
   *  \c std::tie, and is equivalent to calling \ref ordered_begin() and \ref
   *  ordered_end() on both iterators.
   *
   *  \tparam Container The type of container to iterate.
   *  \param container The container to iterate.
   *  \throw invalid_dimension If the dimension specified is larger than the
   *  dimension from the rank of the container.
   *  \return An iterator pointing to the past-the-end position in the
   *  container.
   *
   *  \fractime
   */
  template <typename Container>
  inline ordered_iterator_pair<Container>
  ordered_range(Container& container)
  {
    return ordered_iterator_pair<Container>(ordered_begin(container),
                                            ordered_end(container));
  }

  /**
   *  Returns a pair of constant iterator on the first and the last value in the
   *  range that can be iterated. This function is convenient to use with
   *  \c std::tie, and is equivalent to calling \ref ordered_begin() and \ref
   *  ordered_end() on both iterators.
   *
   *  \tparam Container The type of container to iterate.
   *  \param container The container to iterate.
   *  \throw invalid_dimension If the dimension specified is larger than the
   *  dimension from the rank of the container.
   *  \return An iterator pointing to the past-the-end position in the
   *  container.
   *
   *  \fractime
   */
  template <typename Container>
  inline ordered_iterator_pair<const Container>
  ordered_crange(const Container& container)
  {
    return ordered_iterator_pair<const Container>(ordered_begin(container),
                                                  ordered_end(container));
  }

  /**
   *  Finds the value with the smallest coordinate along the ordered dimension
   *  that is greater or equal to \c bound, and return an iterator pointing to
   *  this value.
   *
   *  \attention This iterator impose constness constraints on its \c value_type
   *  if the container's is a set and not a map. Iterators on sets prevent
   *  modification of the \c value_type because modifying the key may result in
   *  invalidation of the tree. If the container is a map, only the \c
   *  mapped_type can be modified (the \c second element).
   *
   *  \tparam Container The type of container to iterate.
   *  \param bound The lowest bound to the iterator position.
   *  \param container The container to iterate.
   *  \throw invalid_dimension If the dimension specified is larger than the
   *  dimension from the rank of the container.
   *  \return An iterator pointing to the value with the smallest coordinate
   *  greater or equal to \c bound along \c ordered_dim.
   *
   *  \fractime
   */
  template <typename Container>
  inline ordered_iterator<Container>
  ordered_lower_bound(Container& container,
                      const typename Container::key_type&
                      bound)
  {
    if (container.empty()) return ordered_end(container);
    typename ordered_iterator<Container>::node_ptr node
      = container.end().node->parent;
    dimension_type dim;
    spatial::import::tie(node, dim)
      = lower_bound_ordered(node, 0, container.rank(),
                            container.key_comp(), bound);
    return ordered_iterator<Container>(container, dim, node);
  }

  /**
   *  Finds the value with the smallest coordinate along the ordered dimension
   *  that is greater or equal to \c bound, and return a constant iterator to
   *  this value.
   *
   *  \tparam Container The type of container to iterate.
   *  \param bound The lowest bound to the iterator position.
   *  \param container The container to iterate.
   *  \throw invalid_dimension If the dimension specified is larger than the
   *  dimension from the rank of the container.
   *  \return An iterator pointing to the value with the smallest coordinate
   *  greater or equal to \c bound along \c ordered_dim.
   *
   *  \fractime
   */
  template <typename Container>
  inline ordered_iterator<const Container>
  ordered_clower_bound
  (const Container& container,
   const typename Container::key_type& bound)
  { return ordered_lower_bound(container, bound); }

  /**
   *  Finds the value with the largest coordinate along the ordered dimension
   *  that is stricly less than \c bound, and return an iterator pointing to
   *  this value.
   *
   *  \attention This iterator impose constness constraints on its \c value_type
   *  if the container's is a set and not a map. Iterators on sets prevent
   *  modification of the \c value_type because modifying the key may result in
   *  invalidation of the tree. If the container is a map, only the \c
   *  mapped_type can be modified (the \c second element).
   *
   *  \tparam Container The type of container to iterate.
   *  \param bound The lowest bound to the iterator position.
   *  \param container The container to iterate.
   *  \throw invalid_dimension If the dimension specified is larger than the
   *  dimension from the rank of the container.
   *  \return An iterator pointing to the value with the smallest coordinate
   *  greater or equal to \c bound along \c ordered_dim.
   *
   *  \fractime
   *  \see ordered_iterator
   */
  template <typename Container>
  inline ordered_iterator<Container>
  ordered_upper_bound
  (Container& container,
   const typename Container::key_type& bound)
  {
    if (container.empty()) return ordered_end(container);
    typename ordered_iterator<Container>::node_ptr node
      = container.end().node->parent;
    dimension_type dim;
    spatial::import::tie(node, dim)
      = upper_bound_ordered(node, 0, container.rank(),
                            container.key_comp(), bound);
    return ordered_iterator<Container>(container, dim, node);
  }

  /**
   *  Finds the value with the largest coordinate along the ordered dimension
   *  that is stricly less than \c bound, and return an iterator pointing to
   *  this value.
   *
   *  \tparam Container The type of container to iterate.
   *  \param bound The lowest bound to the iterator position.
   *  \param container The container to iterate.
   *  \throw invalid_dimension If the dimension specified is larger than the
   *  dimension from the rank of the container.
   *  \return An iterator pointing to the value with the smallest coordinate
   *  greater or equal to \c bound along \c ordered_dim.
   *
   *  \fractime
   *  \see ordered_iterator
   */
  template <typename Container>
  inline ordered_iterator<const Container>
  ordered_cupper_bound
  (const Container& container,
   const typename Container::key_type& bound)
  { return ordered_upper_bound(container, bound); }

  namespace details
  {
    /**
     *  Within the sub-tree of node, find the node with the smallest value
     *  greater or equal to bound, according to the iterator's ordering rule.
     *
     *  \tparam Container The type of container to iterate.
     *  \param node The node pointed to by the iterator
     *  \param dim  The dimension of the node pointed to by the iterator.
     *  \param rank The rank of the container which node belongs to.
     *  \param cmp  The comparator used by the container which node belongs to.
     *  \param bound The lowest bound to the iterator position.
     *  \return A pair of node, dimension pointing to the element with the
     *  smallest value greater or equal to bound, according to the iterator's
     *  ordering rule.
     *
     *  Since Container is based on a \kdtree and \kdtrees exhibit good locality
     *  of reference (for arranging values in space, not for values location in
     *  memory), the function will run with time complexity close to \Onlognk in
     *  practice.
     *
     *  \fractime
     */
    template <typename NodePtr, typename Rank,
              typename KeyCompare, typename KeyType>
    inline std::pair<NodePtr, dimension_type>
    lower_bound_ordered
    (NodePtr node, dimension_type dim, const Rank rank,
     const KeyCompare& cmp, const KeyType& bound)
    {
      SPATIAL_ASSERT_CHECK(!header(node));
      SPATIAL_ASSERT_CHECK(node != 0);
      NodePtr end = node->parent;
      while (node->left != 0
             && (dim % rank() > 0
                 || !cmp(0, const_key(node), bound)))
        { node = node->left; ++dim; }
      NodePtr best = 0;
      dimension_type best_dim = 0;
      if (!order_less(cmp, rank, const_key(node), bound))
        { best = node; best_dim = dim; }
      for(;;)
        {
          if (node->right != 0
              && (dim % rank() > 0 || best == 0
                  || !cmp(0, const_key(best), const_key(node))))
            {
              node = node->right; ++dim;
              while (node->left != 0
                     && (dim % rank() > 0
                         || !cmp(0, const_key(node), bound)))
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
          if (!order_less(cmp, rank, const_key(node), bound)
              && (best == 0
                  || order_less(cmp, rank, const_key(node), const_key(best))))
            { best = node; best_dim = dim; }
        }
      if (best == 0)
        { best = node; best_dim = dim; }
      return std::make_pair(best, best_dim);
    }

    /**
     *  Within the sub-tree of node, find the node with the smallest value
     *  greater than bound, according to the iterator's ordering rule.
     *
     *  \tparam Container The type of container to iterate.
     *  \param node The node pointed to by the iterator
     *  \param dim  The dimension of the node pointed to by the iterator.
     *  \param rank The rank of the container which node belongs to.
     *  \param cmp  The comparator used by the container which node belongs to.
     *  \param bound The lowest bound to the iterator position.
     *  \return A pair of node, dimension pointing to the element with the
     *  smallest value greater than bound, according to the iterator's
     *  ordering rule.
     *
     *  Since Container is based on a \kdtree and \kdtrees exhibit good locality
     *  of reference (for arranging values in space, not for values location in
     *  memory), the function will run with time complexity close to \Onlognk in
     *  practice.
     *
     *  \fractime
     */
    template <typename NodePtr, typename Rank,
              typename KeyCompare, typename KeyType>
    inline std::pair<NodePtr, dimension_type>
    upper_bound_ordered
    (NodePtr node, dimension_type dim, const Rank rank,
     const KeyCompare& cmp, const KeyType& bound)
    {
      SPATIAL_ASSERT_CHECK(!header(node));
      SPATIAL_ASSERT_CHECK(node != 0);
      NodePtr end = node->parent;
      while (node->left != 0
             && (dim % rank() > 0
                 || !cmp(0, const_key(node), bound)))
        { node = node->left; ++dim; }
      NodePtr best = 0;
      dimension_type best_dim = 0;
      if (order_less(cmp, rank, bound, const_key(node)))
        { best = node; best_dim = dim; }
      for(;;)
        {
          if (node->right != 0
              && (dim % rank() > 0 || best == 0
                  || !cmp(0, const_key(best), const_key(node))))
            {
              node = node->right; ++dim;
              while (node->left != 0
                     && (dim % rank() > 0
                         || !cmp(0, const_key(node), bound)))
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
          if (order_less(cmp, rank, bound, const_key(node))
              && (best == 0
                  || order_less(cmp, rank, const_key(node), const_key(best))))
            { best = node; best_dim = dim; }
        }
      if (best == 0)
        { best = node; best_dim = dim; }
      return std::make_pair(best, best_dim);
    }

  } // namespace details

} // namespace spatial

#endif // SPATIAL_ORDERED_ITERATOR_HPP
