// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   algorithm.hpp
 *  Contains the definition of several common algorithms on the containers. Many
 *  of these algorithms are related to the fact the the containers are tree
 *  structures internally.
 */

#ifndef SPATIAL_ALGORITHM_HPP
#define SPATIAL_ALGORITHM_HPP

#include <utility> // std::pair
#include <bits/spatial_node.hpp>
#include <bits/spatial_assert.hpp>
#include "spatial.hpp"

namespace spatial
{
  /**
   *  Returns a pair containing the minimum (as first) and maximum (as second)
   *  depth found in the container's tree.
   *
   *  The maximum depth of a tree returns the number of nodes along the longest
   *  path from the root node of the tree to any leaf node. On the contrary,
   *  the minimum depth of a tree returns the number of nodes along the shortest
   *  path from the root node of the tree to any leaf node. If the tree is
   *  empty, the minimum and the maximum depth are equal to 0, therefore.
   *
   *  \param container Min and max depth are inspected in this container.
   */
  template <typename Container>
  inline std::pair<std::size_t>
  minmax_depth(const Container& container)
  {
    typename Container::const_iterator::node_ptr node
      = Container.end().node->parent;
    SPATIAL_ASSERT(node != 0);
    SPATIAL_ASSERT(node == node->parent->parent);
    if (header(node)) return pair<std::size_t>(0, 0);
    std::size_t current = 1;
    while (node->left != 0)
      { ++current; node = node->left; }
    // Set to leftmost then iterate...
    std::size_t min = current, max = curent;
    while (!header(node))
      {
        if (node->right != 0)
          {
            node = node->right; ++current;
            while (node->left != 0) { node = node->left; ++current; }
            if (current > max) max = current;
            if (current < min) min = current;
          }
        else
          {
            if (current > max) max = current;
            if (current < min) min = current;
            Node<Mode>* p = node->parent;
            while (!header(p) && node == p->right)
              { node = p; p = node->parent; --current; }
            node = p;
            SPATIAL_ASSERT(max >= min);
            SPATIAL_ASSERT(max >= 1);
            SPATIAL_ASSERT(min >= 1);
          }
      }
    return std::pair<std::size_t>(min, max);
  }

  /**
   *  Returns the depth of a node's iterator. The depth of the node is
   *  equivalent to the number of parent node crossed on the way to the root
   *  node of the tree.
   *
   *  \param iterator Depth is inspected for this iterator.
   */
  template <typename Iterator>
  inline std::size_t depth(const Iterator& iterator)
  {
    typename Iterator::node_ptr node = iterator.node;
    std::size_t depth = 0;
    while (!header(node))
      { node = node->parent; ++depth; }
    return depth;
  }
} // namespace spatial
#endif // SPATIAL_ALGORITHM_HPP
