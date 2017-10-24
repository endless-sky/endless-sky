// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file  spatial_mapping.hpp
 *
 *  Contains the definition of \ref spatial::details::minimum_mapping and \ref
 *  spatial::details::maximum_mapping. These definition are separated from the
 *  rest of the mapping_iterator iterface, since they are also used for general
 *  purpose by the trees.
 */

#ifndef SPATIAL_MAPPING_HPP
#define SPATIAL_MAPPING_HPP

#include <utility> // provides ::std::pair<> and ::std::make_pair()

namespace spatial
{
  namespace details
  {
    /**
     *  Move the iterator given in parameter to the minimum value along the
     *  iterator's mapping dimension but only in the sub-tree composed of the
     *  node pointed to by the iterator and its children.
     *
     *  The tree in the container will be iterated in in-order fashion. As soon
     *  as the minimum is found, the first minimum is retained.
     *
     *  \attention This function is meant to be used by other algorithms in the
     *  library, but not by the end users of the library. If you feel that you
     *  must use this function, maybe you were actually looking for \ref
     *  spatial::mapping_begin(). This function does not perform any sanity
     *  checks on the iterator given in parameter.
     *
     *  \param node     The pointer to the starting node for the iteration.
     *  \param dim      The dimension use to compare the key at \c node.
     *  \param rank     The cardinality or number of dimensions of the
     *                  container being iterated.
     *  \param map      The dimension along which all values in the container
     *                  must be sorted.
     *  \param key_comp A functor to compare two keys in the container along a
     *                  particular dimension.
     *  \return The iterator given in parameter is moved to the value with the
     *  smallest coordinate along \c iter's \c mapping_dim, and among the
     *  children of the node pointed to by \c iter.
     *
     *  \fractime
     */
    template <typename NodePtr, typename Rank, typename KeyCompare>
    inline std::pair<NodePtr, dimension_type>
    minimum_mapping(NodePtr node, dimension_type dim, Rank rank,
                    dimension_type map, const KeyCompare& key_comp)
    {
      SPATIAL_ASSERT_CHECK(map < rank());
      SPATIAL_ASSERT_CHECK(dim < rank());
      SPATIAL_ASSERT_CHECK(!header(node));
      NodePtr end = node->parent;
      while (node->left != 0)
        { node = node->left; dim = incr_dim(rank, dim); }
      NodePtr best = node;
      dimension_type best_dim = dim;
      for (;;)
        {
          if (node->right != 0 && dim != map)
            {
              node = node->right; dim = incr_dim(rank, dim);
              while (node->left != 0)
                { node = node->left; dim = incr_dim(rank, dim); }
            }
          else
            {
              NodePtr prev_node = node;
              node = node->parent; dim = decr_dim(rank, dim);
              while (node != end
                     && prev_node == node->right)
                {
                  prev_node = node;
                  node = node->parent; dim = decr_dim(rank, dim);
                }
              if (node == end) break;
            }
          if (key_comp(map, const_key(node), const_key(best)))
            { best = node; best_dim = dim; }
        }
      SPATIAL_ASSERT_CHECK(best_dim < rank());
      SPATIAL_ASSERT_CHECK(best != 0);
      SPATIAL_ASSERT_CHECK(best != end);
      return std::make_pair(best, best_dim);
    }

    /**
     *  Move the iterator given in parameter to the maximum value along the
     *  iterator's mapping dimension but only in the sub-tree composed of the
     *  node pointed to by the iterator and its children.
     *
     *  \attention This function is meant to be used by other algorithms in the
     *  library, but not by the end users of the library. If you feel that you
     *  must use this function, maybe you were actually looking for \ref
     *  spatial::mapping_end().  This function does not perform any sanity
     *  checks on the iterator given in parameter.
     *
     *  The maximum element in the dimenion \c map is found by looking through
     *  the tree in reversed pre-order fashion. That means we start from the
     *  deepest, right-most element in the tree, and iterate all the way to the
     *  root node. We never, however, visit a left sub-tree when the dimension
     *  of the current node is equal to \c map: it's impossible to find a
     *  greater element in the sub-tree in this case.
     *
     *  \param node     The pointer to the starting node for the iteration.
     *  \param dim      The dimension use to compare the key at \c node.
     *  \param rank     The cardinality or number of dimensions of the
     *                  container being iterated.
     *  \param map      The dimension along which all values in the container
     *                  must be sorted.
     *  \param key_comp A functor to compare two keys in the container along a
     *                  particular dimension.
     *  \return The iterator given in parameter is moved to the value with the
     *  largest coordinate along the dimension \c map, among the children of
     *  the node pointed to by \c iter.
     *
     *  \fractime
     */
    template <typename NodePtr, typename Rank, typename KeyCompare>
    inline std::pair<NodePtr, dimension_type>
    maximum_mapping(NodePtr node, dimension_type dim, Rank rank,
                    dimension_type map, const KeyCompare& key_comp)
    {
      SPATIAL_ASSERT_CHECK(map < rank());
      SPATIAL_ASSERT_CHECK(dim < rank());
      SPATIAL_ASSERT_CHECK(!header(node));
      NodePtr end = node->parent;
      while (node->right != 0)
        { node = node->right; dim = incr_dim(rank, dim); }
      NodePtr best = node;
      dimension_type best_dim = dim;
      for (;;)
        {
          if (node->left != 0 && dim != map)
            {
              node = node->left; dim = incr_dim(rank, dim);
              while (node->right != 0)
                { node = node->right; dim = incr_dim(rank, dim); }
            }
          else
            {
              NodePtr prev_node = node;
              node = node->parent; dim = decr_dim(rank, dim);
              while (node != end
                     && prev_node == node->left)
                {
                  prev_node = node;
                  node = node->parent; dim = decr_dim(rank, dim);
                }
              if (node == end) break;
            }
          if (key_comp(map, const_key(best), const_key(node)))
            { best = node; best_dim = dim; }
        }
      SPATIAL_ASSERT_CHECK(best_dim < rank());
      SPATIAL_ASSERT_CHECK(best != 0);
      SPATIAL_ASSERT_CHECK(best != end);
      return std::make_pair(best, best_dim);
    }

  } // namespace details
} // namespace spatial

#endif // SPATIAL_MAPPING_HPP
