// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   spatial_node.hpp
 *  Defines the basic nodes and associated iterators.
 */

#ifndef SPATIAL_NODE_HPP
#define SPATIAL_NODE_HPP

#include <iterator> // std::bidirectional_iterator_tag and
                    // std::forward_iterator_tag
#include "../spatial.hpp"
#include "spatial_assert.hpp"
#include "spatial_mutate.hpp"

namespace spatial
{
  namespace details
  {
    /**
     *  The basic node for any tree in the library. It contains only the
     *  information necessary to iterate through all nodes and the library, as
     *  well as access the values of a node. However it does not link to the
     *  value by itself.
     *
     *  All nodes in all containers in the library obey the invariant: if at the
     *  head, the \c left points to the head itself, always, by convention. This
     *  way, the header node can be identified readily. It is a very important
     *  feature of the library that only by looking at a node, one can tell
     *  whether the head of the tree has been reached or not.
     *
     *  Once at the head, the parent pointer will point to the root of the tree,
     *  while the right pointer will point to the right most node in the
     *  tree. Therefore, to store the left-most node in the tree, an additional
     *  pointer is required in each container.
     *
     *  This class also holds a model of \linkmode to access the key
     *  and values of the node, without holding these values. It deliberately
     *  does not hold these values for 2 reasons:
     *
     *  - Not all nodes store their keys and values in the same way.
     *  - Some nodes have more information than just key and values.
     *
     *  Additionally, when deferencing a pointer from a node, only the minimum
     *  amount of information is transferred to the variable holding the node,
     *  which happens in several algorithms.
     *
     *  \tparam Mode A model of \linkmode
     */
    template <typename Link>
    struct Node
    {
      //! The link type that indicate how to reach the key and/or the
      //! value from the node.
      typedef Link              link_type;

      /**
       *  A pointer to the parent of the current node. When we are at head, the
       *  parent pointer is equal to the current node. In all other cases it is
       *  different. When this value is null, the node has not been initialized
       *  and it is dangling.
       */
      Node* parent;

      /**
       *  A pointer to the left child node of the current node. If we are at the
       *  head, this pointer will always points to the head: this is an
       *  important property of the nodes that is exploited by the header()
       *  function used in nearly every algorithm to identify the header
       *  node. If there are no node to the left, the pointer is null.
       */
      Node* left;

      /**
       *  A pointer to the right child node of the current node. If we are at the
       *  head, this pointer points to the right most node in the tree. If there are
       *  no node to the right, the pointer is null.
       */
      Node* right;
    };

    /**
     *  Check if node is a header node.
     *
     *  This is done by checking that the left node points to itself. Only at
     *  the head, should this be the case. This important propriety is a
     *  convention of the library to identify the header node.
     */
    template <typename Link>
    inline bool header(const Node<Link>* x)
    { return (x->left == x); }

    /**
     *  Reach the left most node.
     *
     *  Should not be used on header nodes.
     *  \tparam Link a model of \linkmode.
     *  \param x a pointer to a node.
     */
    ///@{
    template <typename Link>
    inline Node<Link>* minimum(Node<Link>* x)
    {
      SPATIAL_ASSERT_CHECK(!header(x));
      while (x->left != 0) x = x->left; return x;
    }

    template <typename Link>
    inline const Node<Link>* minimum(const Node<Link>* x)
    {
      SPATIAL_ASSERT_CHECK(!header(x));
      while (x->left != 0) x = x->left; return x;
    }
    ///@}

    /**
     *  Reach the left most node.
     *
     *  Should not be used on header nodes.
     *  \tparam Link a model of \linkmode.
     *  \param x a pointer to a node.
     */
    ///@{
    template <typename Link>
    inline Node<Link>* maximum(Node<Link>* x)
    {
      SPATIAL_ASSERT_CHECK(!header(x));
      while (x->right != 0) x = x->right; return x;
    }

    template <typename Link>
    inline const Node<Link>* maximum(const Node<Link>* x)
    {
      SPATIAL_ASSERT_CHECK(!header(x));
      while (x->right != 0) x = x->right; return x;
    }
    ///@}

    /**
     *  Reach the next node in symetric transversal order.
     *
     *  Should not be used on header nodes.
     *  \tparam Link a model of \linkmode.
     *  \param x a pointer to a node.
     */
    ///@{
    template <typename Link> Node<Link>*
    increment(Node<Link>* x);

    template <typename Link>
    inline const Node<Link>*
    increment(const Node<Link>* x)
    { return increment(const_cast<Node<Link>*>(x)); }
    ///@}

    /**
     *  Reach the previous node in symetric transversal order.
     *
     *  Should not be used on empty trees, but can be used on header nodes when
     *  the tree is not empty.
     *  \tparam Link a model of \linkmode.
     *  \param x a pointer to a node.
     */
    ///@{
    template <typename Link>
    Node<Link>*
    decrement(Node<Link>* x);

    template <typename Link>
    inline const Node<Link>*
    decrement(const Node<Link>* x)
    { return decrement(const_cast<Node<Link>*>(x)); }
    ///@}

    /**
     *  Reach the next node in preorder transversal.
     *
     *  Should not be used on empty trees or when the pointer to the node is a
     *  the head, which results in undefined behavior.
     *  \tparam Link a model of \linkmode.
     *  \param x a pointer to a node.
     */
    template <typename Link> const Node<Link>*
    preorder_increment(const Node<Link>* x);

    /**
     *  Calculate the depth of a given node. The returned value is undefined if
     *  the node is a header node.
     */
    template <typename Link> inline dimension_type
    depth(const Node<Link>* x)
    {
      dimension_type d = 0;
      while (!header(x)) { x = x->parent; ++d; }
      return d - 1;
    }

    /**
     *  The category of invariants for a \kdtree node: strict or relaxed.
     *
     *  This tag is an indicator for one of the library's most central concepts:
     *  different types of node use different sets of invariant, or "rules" on
     *  their nodes.
     *
     *  With \f$N\f$ the current node, \f$d\f$ the current dimension of
     *  comparison for the node (or: the node's dimension), \f$k(N)\f$ the
     *  function that associate a node to its key, \f$l(N)\f$ and \f$r(N)\f$ the
     *  functions that associate a node to its left node and right node
     *  respectively, then for each tag, the following invarient rules are
     *  respected: \f[ \{ k(N)_d \geq  k(l(N))_d \\ k(N)_d \leq
     *  k(r(N))_d \} \f] for relaxed invarient tags, and: \f[
     *  \{ k(N)_d >  k(l(N))_d \\ k(N)_d \leq k(r(N))_d
     *  \} \f] for strict invarient tags.
     *
     *  In other words, in relaxed invarient \kdtree, values equal to the node's
     *  value can be found both in the left and right sub-trees of the node,
     *  while in strict invarient \kdtree, value equal to the node's value can
     *  only be found on the right side of the node. This allows strict
     *  invarient \kdtree to be slightly faster during search if many similar
     *  values are present in the nodes. But it on the contrary, it makes them
     *  much harder to rebalance.
     *
     *  \see LibraryInternals
     */
    ///@{
    struct relaxed_invariant_tag { };
    struct strict_invariant_tag { };
    ///@}

    /**
     *  For a given node, this function returns the invariant category of the node.
     */
    template <typename Link>
    inline typename Link::invariant_category
    invariant_category(const Node<Link>*)
    { return typename Link::invariant_category(); }

    /**
     *  Define the link type for a Kdtree that contains the value member.
     *
     *  This type also contains the linking information, so it is a model of the
     *  \linkmode concept.
     *
     *  \tparam Key The key type that is held by the Kdtree_link.
     *  \tparam Value The value type that is held by the Kdtree_link.
     */
    template<typename Key, typename Value>
    struct Kdtree_link : Node<Kdtree_link<Key, Value> >
    {
      //! The link to the key type.
      typedef Key                                  key_type;
      //! The link to the value type.
      typedef Value                                value_type;
      //! The link to the node type, which is the node itself, since link
      //! information are also contained in this node.
      typedef Kdtree_link<Key, Value>              link_type;
      //! The link pointer which is often used, has a dedicated type.
      typedef link_type*                           link_ptr;
      //! The constant link pointer which is often used, has a dedicated type.
      typedef const link_type*                     const_link_ptr;
      //! The node pointer type deduced from the mode.
      typedef Node<link_type>*                     node_ptr;
      //! The constant node pointer deduced from the mode.
      typedef const Node<link_type>*               const_node_ptr;
      //! The category of invariant associated with this mode.
      typedef strict_invariant_tag                 invariant_category;

      //! Default constructor
      Kdtree_link() : value() { }

      /**
       *  The value of the node, required by the \linkmode concept.
       *
       *  In *-map containers, the value is necessarily a pair, with the first
       *  member being a key, and the second member being the mapped type. In
       *  *-set containers, the value and the key are one and the same thing.
       *
       *  In any case the value is always constant for keys, and therefore this
       *  type cannot be used in assignment operations, since keys cannot be
       *  changed. Most algorithm only use a pointer to the link_type.
       */
      Value value;

    private:
      /**
       *  The link_type is a non-assignable type, because the key it contains
       *  is a constant type.
       *
       *  Most algorthims manipulate a pointer to this element rather than this
       *  element directly. Note that copy-construction is permitted.
       */
      Kdtree_link<Key, Value>&
      operator= (const Kdtree_link<Key, Value>&);
    };

    /**
     *  Define a weighted link type for the relaxed \kdtree.
     *
     *  This type also contains the linking information, so it is a model of the
     *  \linkmode concept.
     *
     *  \tparam Key The key type that is held by the Kdtree_link.
     *  \tparam Value The value type that is held by the Kdtree_link.
     */
    template<typename Key, typename Value>
    struct Relaxed_kdtree_link : Node<Relaxed_kdtree_link<Key, Value> >
    {
      //! The link to the key type.
      typedef Key                                 key_type;
      //! The link to the value type.
      typedef Value                               value_type;
      //! The link type, which is also itself, since mode are also
      //! contained in this type.
      typedef Relaxed_kdtree_link<Key, Value>     link_type;
      //! The link pointer which is often used, has a dedicated type.
      typedef link_type*                          link_ptr;
      //! The constant link pointer which is often used, has a dedicated type.
      typedef const link_type*                    const_link_ptr;
      //! The node pointer type deduced from the mode.
      typedef Node<link_type>*                    node_ptr;
      //! The constant node pointer deduced from the mode.
      typedef const Node<link_type>*              const_node_ptr;
      //! The category of invariant with associated with this mode.
      typedef relaxed_invariant_tag               invariant_category;

      //! \empty
      Relaxed_kdtree_link() { }

      /**
       *  The value of the node, required by the \linkmode concept.
       *
       *  In *-map containers, the value is necessarily a pair, with the first
       *  member being a key, and the second member being the mapped type. In
       *  *-set containers, the value and the key are one and the same thing.
       *
       *  In any case the value is always constant for keys, and therefore this
       *  type cannot be used in assignment operations, since keys cannot be
       *  changed. Most algorithm only use a pointer to the link_type.
       */
      Value value;

      //! The weight is equal to 1 plus the amount of child nodes below the
      //! current node. It is always equal to 1 at least.
      weight_type weight;

    private:
      /**
       *  The link_type is a non-assignable type, because the key it contains
       *  is a constant type.
       *
       *  Most algorthims manipulate a pointer to this element rather than this
       *  element directly. Note that copy-construction is permitted.
       */
      Relaxed_kdtree_link<Key, Value>&
      operator= (const Relaxed_kdtree_link<Key, Value>&);
    };

    /**
     *  This function converts a pointer on a node into a link for a \ref
     *  Kdtree_link type.
     *  \tparam Key the key type for the \ref Kdtree_link.
     *  \tparam Value the value type for the \ref Kdtree_link.
     *  \param node the node to convert to a link.
     */
    ///@{
    template <typename Key, typename Value>
    inline Kdtree_link<Key, Value>*
    link(Node<Kdtree_link<Key, Value> >* node)
    {
      return static_cast<Kdtree_link<Key, Value>*>(node);
    }

    template <typename Key, typename Value>
    inline const Kdtree_link<Key, Value>*
    const_link(const Node<Kdtree_link<Key, Value> >* node)
    {
      return static_cast<const Kdtree_link<Key, Value>*>(node);
    }
    ///@}

    /**
     *  This function converts a pointer on a node into a key for a \ref
     *  Kdtree_link type. A key is always a constant type, hence only const_key
     *  is declared.
     *
     *  This overload is used when both the key type and the value type of the
     *  \ref Relaxed_kdtree_link are of the same type, e.g. in set containers.
     *  \tparam Value the value type for the \ref Kdtree_link.
     *  \param node the node to convert to a key.
     */
    template <typename Value>
    inline const typename Kdtree_link<Value, Value>::key_type&
    const_key(const Node<Kdtree_link<Value, Value> >* node)
    {
      return static_cast<const Kdtree_link<Value, Value>*>(node)->value;
    }

    /**
     *  This function converts a pointer on a node into a key for a \ref
     *  Kdtree_link type. A key is always a constant type, hence only const_key
     *  is declared.
     *
     *  \tparam Key the key type for the \ref Kdtree_link.
     *  \tparam Value the value type for the \ref Kdtree_link.
     *  \param node the node to convert to a key.
     */
    template <typename Key, typename Value>
    inline const typename Kdtree_link<Key, Value>::key_type&
    const_key(const Node<Kdtree_link<Key, Value> >* node)
    {
      return static_cast<const Kdtree_link<Key, Value>*>(node)->value.first;
    }

    /**
     *  This function converts a pointer on a node into a value for a \ref
     *  Kdtree_link type.
     *  \tparam Key the key type for the \ref Kdtree_link.
     *  \tparam Value the value type for the \ref Kdtree_link.
     *  \param node the node to convert to a key.
     */
    ///@{
    template <typename Key, typename Value>
    inline typename Kdtree_link<Key, Value>::value_type&
    value(Node<Kdtree_link<Key, Value> >* node)
    {
      return static_cast<Kdtree_link<Key, Value>*>(node)->value;
    }

    template <typename Key, typename Value>
    inline const typename Kdtree_link<Key, Value>::value_type&
    const_value(const Node<Kdtree_link<Key, Value> >* node)
    {
      return static_cast<const Kdtree_link<Key, Value>*>(node)->value;
    }
    ///@}

    /**
     *  This function converts a pointer on a node into a link for a \ref
     *  Relaxed_kdtree_link type.
     *  \tparam Key the key type for the \ref Relaxed_kdtree_link.
     *  \tparam Value the value type for the \ref Relaxed_kdtree_link.
     *  \param node the node to convert to a key.
     */
    ///@{
    template <typename Key, typename Value>
    inline Relaxed_kdtree_link<Key, Value>*
    link(Node<Relaxed_kdtree_link<Key, Value> >* node)
    {
      return static_cast<Relaxed_kdtree_link<Key, Value>*>(node);
    }

    template <typename Key, typename Value>
    inline const Relaxed_kdtree_link<Key, Value>*
    const_link(const Node<Relaxed_kdtree_link<Key, Value> >* node)
    {
      return static_cast<const Relaxed_kdtree_link<Key, Value>*>(node);
    }
    ///@}

    /**
     *  This function converts a pointer on a node into a key for a \ref
     *  Relaxed_kdtree_link type.  A key is always a constant type, hence only
     *  const_key exists.
     *
     *  This overload is used when both the key type and the value type of the
     *  \ref Relaxed_kdtree_link are of the same type, e.g. in set containers.
     *  \tparam Value the value type for the \ref Relaxed_kdtree_link.
     *  \param node the node to convert to a key.
     */
    template <typename Value>
    inline const typename Relaxed_kdtree_link<Value, Value>::key_type&
    const_key(const Node<Relaxed_kdtree_link<Value, Value> >* node)
    {
      return static_cast
        <const Relaxed_kdtree_link<Value, Value>*>(node)->value;
    }

    /**
     *  This function converts a pointer on a node into a key for a \ref
     *  Relaxed_kdtree_link type. A key is always a constant type, hence only
     *  const_key exists.
     *
     *  \tparam Key the key type for the \ref Relaxed_kdtree_link
     *  \tparam Value the value type for the \ref Relaxed_kdtree_link.
     *  \param node the node to convert to a key.
     */
    template <typename Key, typename Value>
    inline const typename Relaxed_kdtree_link<Key, Value>::key_type&
    const_key(const Node<Relaxed_kdtree_link<Key, Value> >* node)
    {
      return static_cast<const Relaxed_kdtree_link<Key, Value>*>
        (node)->value.first;
    }

    /**
     *  This function converts a pointer on a node into a value for a \ref
     *  Relaxed_kdtree_link type.
     *  \tparam Key the key type for the \ref Relaxed_kdtree_link.
     *  \tparam Value the value type for the \ref Relaxed_kdtree_link.
     *  \param node the node to convert to a key.
     */
    ///@{
    template <typename Key, typename Value>
    inline typename Relaxed_kdtree_link<Key, Value>::value_type&
    value(Node<Relaxed_kdtree_link<Key, Value> >* node)
    {
      return static_cast<Relaxed_kdtree_link<Key, Value>*>(node)->value;
    }

    template <typename Key, typename Value>
    inline const typename Relaxed_kdtree_link<Key, Value>::value_type&
    const_value(const Node<Relaxed_kdtree_link<Key, Value> >* node)
    {
      return static_cast<const Relaxed_kdtree_link<Key, Value>*>(node)->value;
    }
    ///@}

    /**
     *  Swaps nodes position in the tree.
     *
     *  This function does not updates the left-most and right-most pointers of
     *  the tree where the nodes belongs to. This is left to the responsibility
     *  of the caller.
     *  \see Node
     *  \see Kdtree_link
     *  \see Relaxed_kdtree_link
     */
    ///@{
    template <typename Link>
    void swap_node_aux(Node<Link>* a, Node<Link>* b);

    template<typename Key, typename Value>
    inline void swap_node(Node<Kdtree_link<Key, Value> >*& a,
                          Node<Kdtree_link<Key, Value> >*& b)
    { swap_node_aux(a, b); std::swap(a, b); }

    template<typename Key, typename Value>
    inline void swap_node(Node<Relaxed_kdtree_link<Key, Value> >*& a,
                          Node<Relaxed_kdtree_link<Key, Value> >*& b)
    {
      swap_node_aux(a, b);
      std::swap(link(a)->weight, link(b)->weight);
      std::swap(a, b);
    }
    ///@}

    /**
     *  A bidirectional iterator traversing all node in the tree in
     *  inorder traversal. This iterator provides mutable access to the nodes in
     *  the tree.
     *
     *  \tparam Link The linking mode for the node.
     */
    template<typename Link>
    class Node_iterator
    {
    public:
      typedef typename mutate<typename Link::value_type>::type value_type;
      typedef typename Link::value_type&         reference;
      typedef typename Link::value_type*         pointer;
      typedef std::ptrdiff_t                     difference_type;
      typedef std::bidirectional_iterator_tag    iterator_category;
      typedef typename Link::node_ptr            node_ptr;

    private:
      typedef Node_iterator<Link>                Self;

    public:
      //! Create an uninintialized iterator. This iterator should not be used
      //! until is has been assigned.
      Node_iterator() : node() { }

      //! Build and assign an interator to a link pointer.
      explicit Node_iterator(node_ptr x) : node(x) { }

      //! Dereferance the iterator: return the value of the node.
      reference operator*()
      { return value(node); }

      //! Dereferance the iterator: return the pointer to the value of the node.
      pointer operator->()
      { return &value(node); }

      //! Moves the iterator to the next node in inorder transversal.
      Self& operator++()
      { node = increment(node); return *this; }

      //! Moves the iterator to the next node in inorder transversal and returns
      //! the iterator value before the move.
      Self operator++(int)
      { Self tmp = *this; node = increment(node); return tmp; }

      //! Moves the iterator to the previous node in inorder transversal.
      Self& operator--()
      { node = decrement(node); return *this; }

      //! Moves the iterator to the previous node in inorder transversal and
      //! returns the iterator value before the move.
      Self operator--(int)
      { Self tmp = *this; node = decrement(node); return tmp; }

      //! Check if 2 iterators are equal: pointing at the same node.
      bool operator==(const Self& x) const
      { return node == x.node; }

      //! Check if 2 iterators are different: pointing at different nodes.
      bool operator!=(const Self& x) const
      { return node != x.node; }

      //! The node pointed to by the iterator.
      node_ptr node;
    };

    /**
     *  A bidirectional iterator traversing all node in the tree in
     *  inorder traversal. This iterator provides constant access to the nodes
     *  in the tree.
     *
     *  \tparam Link The linking mode for the node.
     */
    template<typename Link>
    class Const_node_iterator
    {
    public:
      typedef typename mutate<typename Link::value_type>::type value_type;
      typedef const typename Link::value_type&   reference;
      typedef const typename Link::value_type*   pointer;
      typedef std::ptrdiff_t                     difference_type;
      typedef std::bidirectional_iterator_tag    iterator_category;
      typedef typename Link::const_node_ptr      node_ptr;

    private:
      typedef Const_node_iterator<Link>          Self;
      typedef Node_iterator<Link>                iterator;

    public:
      //! Create an uninintialized iterator. This iterator should not be used
      //! until is has been assigned.
      Const_node_iterator() : node() { }

      //! Build and assign an interator to a link pointer.
      explicit
      Const_node_iterator(node_ptr x) : node(x) { }

      //! Convert an iterator into a constant iterator.
      Const_node_iterator(const iterator& it) : node(it.node) { }

      //! Dereferance the iterator: return the value of the node.
      reference operator*()
      { return const_value(node); }

      //! Dereferance the iterator: return the pointer to the value of the node.
      pointer operator->()
      { return &const_value(node); }

      //! Moves the iterator to the next node in inorder transversal.
      Self& operator++()
      { node = increment(node); return *this; }

      //! Moves the iterator to the next node in inorder transversal and returns
      //! the iterator value before the move.
      Self operator++(int)
      { Self tmp = *this; node = increment(node); return tmp; }

      //! Moves the iterator to the previous node in inorder transversal.
      Self& operator--()
      { node = decrement(node); return *this; }

      //! Moves the iterator to the previous node in inorder transversal and
      //! returns the iterator value before the move.
      Self operator--(int)
      { Self tmp = *this; node = decrement(node); return tmp; }

      //! Check if 2 iterators are equal: pointing at the same node.
      bool operator==(const Self& x) const
      { return node == x.node; }

      //! Check if 2 iterators are different: pointing at different nodes.
      bool operator!=(const Self& x) const
      { return node != x.node; }

      //! The node pointed to by the iterator.
      node_ptr node;
    };

    /**
     *  A forward iterator that iterates through the node of the container in
     *  preorder transversal. It provides constant access to the node. It is
     *  used to clone the tree.
     *
     *  \tparam Link The linking mode for the node.
     */
    template<typename Link>
    struct Preorder_node_iterator
    {
      typedef typename mutate<typename Link::value_type>::type value_type;
      typedef const typename Link::value_type&  reference;
      typedef const typename Link::value_type*  pointer;
      typedef std::ptrdiff_t                    difference_type;
      typedef std::forward_iterator_tag         iterator_category;
      typedef typename Link::const_node_ptr     node_ptr;

    private:
      typedef Preorder_node_iterator<Link>      Self;

    public:
      //! Create an uninintialized iterator. This iterator should not be used
      //! until is has been assigned.
      Preorder_node_iterator() : node() { }

      //! Build and assign an interator to a link pointer.
      explicit
      Preorder_node_iterator(node_ptr x) : node(x) { }

      //! Dereferance the iterator: return the value of the node.
      reference operator*()
      { return const_value(node); }

      //! Dereferance the iterator: return the pointer to the value of the node.
      pointer operator->()
      { return &const_value(node); }

      //! Moves the iterator to the next node in preorder transversal.
      Self& operator++()
      { node = preorder_increment(node); return *this; }

      //! Moves the iterator to the next node in preorder transversal and returns
      //! the iterator value before the move.
      Self operator++(int)
      { Self tmp = *this; node = preorder_increment(node); return tmp; }

      //! Check if 2 iterators are equal: pointing at the same node.
      bool operator==(const Self& x) const
      { return node == x.node; }

      //! Check if 2 iterators are different: pointing at different nodes.
      bool operator!=(const Self& x) const
      { return node != x.node; }

      //! The node pointed to by the iterator.
      node_ptr node;
    };

    template <typename Link> inline Node<Link>* increment(Node<Link>* x)
    {
      SPATIAL_ASSERT_CHECK(!header(x));
      if (x->right != 0)
        {
          x = x->right;
          while (x->left != 0) { x = x->left; }
        }
      else
        {
          Node<Link>* p = x->parent;
          while (!header(p) && x == p->right)
            { x = p; p = x->parent; }
          x = p;
        }
      return x;
    }

    template <typename Link> inline Node<Link>* decrement(Node<Link>* x)
    {
      SPATIAL_ASSERT_CHECK((!header(x) || x->parent != 0));
      if (header(x))
        { x = x->right; } // At header, 'right' points to the right-most node
      else if (x->left != 0)
        {
          Node<Link>* y = x->left;
          while (y->right != 0) { y = y->right; }
          x = y;
        }
      else
        {
          Node<Link>* p = x->parent;
          while (!header(p) && x == p->left)
            { x = p; p = x->parent; }
          x = p;
        }
      return x;
    }

    template <typename Link>
    inline void swap_node_aux
    (Node<Link>* a, Node<Link>* b)
    {
      typedef Node<Link>* node_ptr;
      if (a == b) return;
      SPATIAL_ASSERT_CHECK(!header(a));
      SPATIAL_ASSERT_CHECK(!header(b));
      if (a->parent == b)
        {
          if (header(b->parent))
            { b->parent->parent = a; }
          else
            {
              if (b->parent->left == b) { b->parent->left = a; }
              else { b->parent->right = a; }
            }
          if (a->left != 0) { a->left->parent = b; }
          if (a->right != 0) { a->right->parent = b; }
          a->parent = b->parent;
          b->parent = a;
          node_ptr a_left = a->left;
          node_ptr a_right = a->right;
          if (b->left == a)
            {
              if (b->right != 0) { b->right->parent = a; }
              a->left = b;
              a->right = b->right;
            }
          else
            {
              if (b->left != 0) { b->left->parent = a; }
              a->left = b->left;
              a->right = b;
            }
          b->left = a_left;
          b->right = a_right;
        }
      else if (b->parent == a)
        {
          if (header(a->parent))
            { a->parent->parent = b; }
          else
            {
              if (a->parent->left == a) { a->parent->left = b; }
              else { a->parent->right = b; }
            }
          if (b->left != 0) { b->left->parent = a; }
          if (b->right != 0) { b->right->parent = a; }
          b->parent = a->parent;
          a->parent = b;
          node_ptr b_left = b->left;
          node_ptr b_right = b->right;
          if (a->left == b)
            {
              if (a->right != 0) { a->right->parent = b; }
              b->left = a;
              b->right = a->right;
            }
          else
            {
              if (a->left != 0) { a->left->parent = b; }
              b->left = a->left;
              b->right = a;
            }
          a->left = b_left;
          a->right = b_right;
        }
      else
        {
          if (header(a->parent))
            { a->parent->parent = b; }
          else
            {
              if (a->parent->left == a) { a->parent->left = b; }
              else { a->parent->right = b; }
            }
          if (header(b->parent))
            { b->parent->parent = a; }
          else
            {
              if (b->parent->left == b) { b->parent->left = a; }
              else { b->parent->right = a; }
            }
          if (a->left != 0) { a->left->parent = b; }
          if (b->left != 0) { b->left->parent = a; }
          if (a->right != 0) { a->right->parent = b; }
          if (b->right != 0) { b->right->parent = a; }
          std::swap(a->parent, b->parent);
          std::swap(a->left, b->left);
          std::swap(a->right, b->right);
        }
    }

    template <typename Link> inline const Node<Link>*
    preorder_increment(const Node<Link>* x)
    {
      if (x->left != 0) { x = x->left; }
      else if (x->right != 0) { x = x->right; }
      else
        {
          const Node<Link>* p = x->parent;
          while (!header(p) && (x == p->right || p->right == 0))
            { x = p; p = x->parent; }
          x = p;
          if (!header(p)) { x = x->right; }
        }
      return x;
    }

  } // namespace details
} // namespace spatial

#endif // SPATIAL_NODE_HPP
