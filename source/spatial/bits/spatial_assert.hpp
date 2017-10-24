// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   spatial_assert.hpp
 *  Provide a smiliar functionality than \c cassert, except that
 *  SPATIAL_ENABLE_ASSERT must be defined to enable it, by default, no spatial
 *  assertion check is performed.
 *
 *  This feature is built in the library for the sole purpose of the library
 *  developers, therefore it is encouraged that no one else but the library
 *  developers uses it. It is used during unit testing and debugging.
 */

#ifdef SPATIAL_ENABLE_ASSERT
#  ifndef SPATIAL_ASSERT_HPP_ONCE
#    define SPATIAL_ASSERT_HPP_ONCE
#    include <cstdlib>
#    include <iostream>
namespace spatial
{
  /**
   *  This namespace is meant to isolate the assertion function, preventing that
   *  it conflicts with other functions unintended for that usage.
   */
  namespace assert
  {
    /**
     *  This function will call abort() (and therefore cause the program to stop
     *  with abnormal termination) and will print an error giving the cause of
     *  the failure.
     *
     *  This function is not meant to be used directly, rather, the
     *  \ref SPATIAL_ASSERT_CHECK macro is meant to be used instead:
     *  \code
     *  SPATIAL_ASSERT_CHECK(test == true);
     *  \endcode
     *
     *  If \c test in the file \c example.cpp above is found to be \c true, then
     *  the program carries with its execution. If \c is \c false, then the
     *  program will abort with abort with the following messages:
     *  \code
     *  Assertion failed (example.cpp: line 34): 'test == true'
     *  \endcode
     */
    inline void assert_fail
    (const char* msg, const char* filename, unsigned int line) throw()
    {
      try
        {
          std::cerr << std::endl
                    << "Assertion failed (" << filename << ":" << line
                    << "): '" << msg << "'" << std::endl;
        }
      catch (...) { }
      abort();
    }
  }

  namespace details
  {
    // Prototype declaration for the assertions.
    template <typename Key, typename Value> struct Kdtree_link;
    template <typename Key, typename Value> struct Relaxed_kdtree_link;
    template <typename Link> struct Node;
    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Alloc>
    class Kdtree;
    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Balancing, typename Alloc>
    class Relaxed_kdtree;

    template <typename Value>
    inline const typename Kdtree_link<Value, Value>::key_type&
    const_key(const Node<Kdtree_link<Value, Value> >* node);
    template <typename Key, typename Value>
    inline const typename Kdtree_link<Key, Value>::key_type&
    const_key(const Node<Kdtree_link<Key, Value> >* node);
    template <typename Key, typename Value>
    inline const Kdtree_link<Key, Value>*
    const_link(const Node<Kdtree_link<Key, Value> >* node);

    template <typename Value>
    inline const typename Relaxed_kdtree_link<Value, Value>::key_type&
    const_key(const Node<Relaxed_kdtree_link<Value, Value> >* node);
    template <typename Key, typename Value>
    inline const typename Relaxed_kdtree_link<Key, Value>::key_type&
    const_key(const Node<Relaxed_kdtree_link<Key, Value> >* node);
    template <typename Key, typename Value>
    inline const Relaxed_kdtree_link<Key, Value>*
    const_link(const Node<Relaxed_kdtree_link<Key, Value> >* node);
  }

  namespace assert
  {
    /**
     *  Checks that \c node and the children of \c node are all satifying the
     *  tree invariant.
     *
     *  \param rank The rank of the node being inspected.
     *  \param depth The depth of the node being inspected.
     *  \param node The node to check.
     */
    ///@(
    template <typename Compare, typename Key, typename Value>
    inline bool
    assert_invariant_node
    (const Compare& cmp, dimension_type rank, dimension_type depth,
     const details::Node<details::Kdtree_link<Key, Value> >* node)
    {
      dimension_type next_depth = depth + 1;
      const details::Node<details::Kdtree_link<Key, Value> >
        *left = node->left, *right = node->right;
      while (!header(node->parent))
        {
          if (node->parent->left == node)
            {
              if (!cmp((depth - 1) % rank, const_key(node),
                       const_key(node->parent)))
                return false;
            }
          else
            {
              if (cmp((depth - 1) % rank, const_key(node),
                      const_key(node->parent)))
                return false;
            }
          --depth;
          node = node->parent;
        }
      return
        ((!left || assert_invariant_node(cmp, rank, next_depth, left))
         && (!right || assert_invariant_node(cmp, rank, next_depth, right)));
    }

    template <typename Compare, typename Key, typename Value>
    inline bool
    assert_invariant_node
    (const Compare& cmp, dimension_type rank, dimension_type depth,
     const details::Node<details::Relaxed_kdtree_link<Key, Value> >* node)
    {
      dimension_type next_depth = depth + 1;
      const details::Node<details::Relaxed_kdtree_link<Key, Value> >
        *left = node->left, *right = node->right;
      while (!header(node->parent))
        {
          if (node->parent->left == node)
            {
              if (cmp((depth - 1) % rank, const_key(node->parent),
                      const_key(node)))
                return false;
            }
          else
            {
              if (cmp((depth - 1) % rank, const_key(node),
                      const_key(node->parent)))
                return false;
            }
          --depth;
          node = node->parent;
        }
      return
        ((!left || assert_invariant_node(cmp, rank, next_depth, left))
         && (!right || assert_invariant_node(cmp, rank, next_depth, right)));
    }
    ///@}

    /**
     *  Checks that all node satisfy the invarient in \c container.
     *
     *  \param container The container that is being checked.
     */
    template <typename Container>
    inline bool
    assert_invariant(const Container& container)
    {
      typename Container::const_iterator::node_ptr node
        = container.end().node->parent; // get root node
      if (container.size() == 0 && node != node->parent)
        return false;
      else if (!header(node))
        return assert_invariant_node
          (container.key_comp(), container.dimension(), 0, node);
      else return true;
    }

    /**
     *  Prints the contents of node, so long as an overload for Key and
     *  std::ostream is defined.
     *
     *  \param o The output stream to use.
     *  \param node The node to inspect
     *  \param depth The depth of the node.
     */
    ///@{
    template <typename Compare, typename Key, typename Value>
    inline std::ostream&
    assert_inspect_node
    (const Compare& cmp, dimension_type rank, std::ostream& o,
     const details::Node<details::Kdtree_link<Key, Value> >* node,
     dimension_type depth)
    {
      for (std::size_t i = 0; i < depth; ++i) o << ".";
      if (header(node->parent)) o << "T";
      else if (node->parent->left == node) o << "L";
      else if (node->parent->right == node) o << "R";
      else o << "E";
      const details::Node<details::Kdtree_link<Key, Value> >
        *test = node;
      dimension_type test_depth = depth;
      while (!header(test->parent))
        {
          if (test->parent->left == test)
            {
              if (!cmp((test_depth - 1) % rank, const_key(test),
                       const_key(test->parent)))
                { o << "!"; break; }
            }
          else
            {
              if (cmp((test_depth - 1) % rank, const_key(test),
                      const_key(test->parent)))
                { o << "!"; break; }
            }
          --test_depth;
          test = test->parent;
        }
      o << "<node:" << node << ">{parent:" << node->parent
        << " left:" << node->left << " right:" << node->right
        << std::flush
        << " key:" << details::const_key(node) << "}"
        << std::endl;
      if (node->left)
        assert_inspect_node(cmp, rank, o, node->left, depth + 1);
      if (node->right)
        assert_inspect_node(cmp, rank, o, node->right, depth + 1);
      return o;
    }

    template <typename Compare, typename Key, typename Value>
    inline std::ostream&
    assert_inspect_node
    (const Compare& cmp, dimension_type rank, std::ostream& o,
     const details::Node<details::Relaxed_kdtree_link<Key, Value> >* node,
     dimension_type depth)
    {
      if (node->left)
        assert_inspect_node(cmp, rank, o, node->left, depth + 1);
      for (std::size_t i = 0; i < depth; ++i) o << ".";
      if (header(node->parent)) o << "T";
      else if (node->parent->left == node) o << "L";
      else if (node->parent->right == node) o << "R";
      else o << "E";
      const details::Node<details::Relaxed_kdtree_link<Key, Value> >
        *test = node;
      dimension_type test_depth = depth;
      while (!header(test->parent))
        {
          if (test->parent->left == test)
            {
              if (cmp((test_depth - 1) % rank, const_key(test->parent),
                      const_key(test)))
                { o << "!"; break; }
            }
          else
            {
              if (cmp((test_depth - 1) % rank, const_key(test),
                      const_key(test->parent)))
                { o << "!"; break; }
            }
          --test_depth;
          test = test->parent;
        }
      o << "<node:" << node << ">{parent:" << node->parent
        << " left:" << node->left << " right:" << node->right
        << " weight:" << details::const_link(node)->weight
        << std::flush
        << " key:" << details::const_key(node) << "}"
        << std::endl;
      if (node->right)
        assert_inspect_node(cmp, rank, o, node->right, depth + 1);
      return o;
    }
    ///@}

    /**
     *  This function will call abort() (and therefore cause the program to stop
     *  with abnormal termination) and will print the details of the tree in the
     *  container if the invariant of the tree has been broken in any of the
     *  phones.
     *
     *  This function is not meant to be used directly, rather, the
     *  \ref SPATIAL_ASSERT_CHECK macro is meant to be used instead:
     *  \code
     *  SPATIAL_ASSERT_CHECK(test == true);
     *  \endcode
     *
     *  If \c test in the file \c example.cpp above is found to be \c true, then
     *  the program carries with its execution. If \c is \c false, then the
     *  program will abort with abort with the following messages:
     *  \code
     *  Assertion failed (example.cpp: line 34): 'test == true'
     *  H{<node:0x1b11f010> parent:0x1b11f000 left:0x1b11020 right:0x0 key:{1, 1}}
     *  .L{<node:0x1b11f020> parent:0x1b11f010 left:0x0 right:0x0 key:{0, 1}}
     *  \endcode
     */
    ///@{
    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Alloc>
    inline void assert_inspect
    (const char* msg, const char* filename, unsigned int line,
     const details::Kdtree<Rank, Key, Value, Compare, Alloc>& tree) throw()
    {
      try
        {
          std::cerr << std::endl
                    << "Assertion failed (" << filename << ":" << line
                    << "): '" << msg << "' does not satisfy invariant"
                    << std::endl;
          std::cerr << "<Kdtree:" << &tree << ">{" << std::endl;
          std::cerr << "header:<node:" << tree.end().node
                    << ">{parent:" << tree.end().node->parent
                    << " left:" << tree.end().node->left
                    << " right:" << tree.end().node->right
                    << "}" << std::endl;
          std::cerr << "leftmost:" << tree.begin().node
                    << " size:" << tree.size()
                    << " items:[" << std::endl;
          if (tree.end().node->parent != tree.end().node)
            assert_inspect_node(tree.key_comp(), tree.dimension(), std::cerr,
                                tree.end().node->parent, 0);
          std::cerr << "]}" << std::endl;
        }
      catch (...) { }
      abort();
    }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Balancing, typename Alloc>
    inline void assert_inspect
    (const char* msg, const char* filename, unsigned int line,
     const details::Relaxed_kdtree<Rank, Key, Value, Compare, Balancing, Alloc>&
     tree) throw()
    {
      try
        {
          std::cerr << std::endl
                    << "Assertion failed (" << filename << ":" << line
                    << "): '" << msg << "' does not satisfy invariant"
                    << std::endl;
          std::cerr << "<Relaxed_kdtree:" << &tree << ">{" << std::endl;
          std::cerr << "header:<node:" << tree.end().node
                    << ">{parent:" << tree.end().node->parent
                    << " left:" << tree.end().node->left
                    << " right:" << tree.end().node->right
                    << "}" << std::endl;
          std::cerr << "leftmost:" << tree.begin().node
                    << " size:" << tree.size()
                    << " items:[" << std::endl;
          if (tree.end().node->parent != tree.end().node)
            assert_inspect_node(tree.key_comp(), tree.dimension(), std::cerr,
                                tree.end().node->parent, 0);
          std::cerr << "]}" << std::endl;
        }
      catch (...) { }
      abort();
    }
    ///@}
  } // namespace assert
} // namespace spatial
#  endif
#endif

#ifdef SPATIAL_ASSERT_CHECK
#  undef SPATIAL_ASSERT_CHECK
#endif

#ifndef SPATIAL_ENABLE_ASSERT
/**
 *  \def SPATIAL_ASSERT_CHECK(expr)
 *  Check that expression is true. If expression is false, the program will be
 *  aborted and the expression \c expr, along with the file name and the line
 *  where it occurs will be printed on the output of the program.
 *
 *  The macro is meant to be used this way in a program:
 *  \code
 *  SPATIAL_ASSERT_CHECK(test == true);
 *  \endcode
 *
 *  If \c test in the file \c example.cpp above is found to be \c true, then
 *  the program carries with its execution. If \c is \c false, then the
 *  program will abort with abort with the following messages:
 *  \code
 *  Assertion failed (example.cpp: line 34): 'test == true'
 *  \endcode
 *
 *  \param expr A test expression.
 */
#  define SPATIAL_ASSERT_CHECK(expr)

/**
 *  \def SPATIAL_ASSERT_INVARIANT(container)
 *  If the invariant is broken in any of the node of the container, the program
 *  will be aborted and will print the content of the tree in the container
 *  before stopping, along with the file name and the line where it occurs on
 *  the output of the program.
 *
 *  The macro is meant to be used this way in a program:
 *  \code
 *  SPATIAL_ASSERT_INVARIANT(container);
 *  \endcode
 *
 *  If \c container in the exemple above is found to be \c valide, then
 *  the program carries with its execution. If \c is \c false, then the
 *  program will abort with abort with the following messages:
 *  \code
 *  Assertion failed (example.cpp: line 34): 'test == true'
 *  \endcode
 *
 *  \param container A container to test the invariant.
 */
#  define SPATIAL_ASSERT_INVARIANT(container)
#else
#  define SPATIAL_ASSERT_CHECK(expr)                                    \
  ((expr)                                                               \
   ? static_cast<void>(0)                                               \
   : ::spatial::assert::assert_fail(#expr, __FILE__, __LINE__))
#  define SPATIAL_ASSERT_INVARIANT(container)                           \
  ((::spatial::assert::assert_invariant(container))                     \
   ? static_cast<void>(0)                                               \
   : ::spatial::assert::assert_inspect(#container, __FILE__, __LINE__, container))
#endif
