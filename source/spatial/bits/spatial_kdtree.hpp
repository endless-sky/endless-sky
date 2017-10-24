// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   spatial_kdtree.hpp
 *  Kdtree class is defined in this file and its implementation is in
 *  the corresponding *.tpp file.
 *
 *  The Kdtree class defines all the methods and algorithms to store, delete and
 *  iterate over nodes in a Kdtree. This class is the bare definition of the
 *  kdtree and must be rebalanced by the user after nodes have been inserted.
 *
 *  \see Kdtree
 */

#ifndef SPATIAL_KDTREE_HPP
#define SPATIAL_KDTREE_HPP

#include <algorithm> // for std::equal and std::lexicographical_compare
#include <vector>

#include "spatial_ordered.hpp"
#include "spatial_mapping.hpp"
#include "spatial_equal.hpp"
#include "spatial_rank.hpp"
#include "spatial_compress.hpp"
#include "spatial_value_compare.hpp"
#include "spatial_template_member_swap.hpp"
#include "spatial_assert.hpp"
#include "spatial_except.hpp"

namespace spatial
{
  namespace details
  {
    /**
     *  Detailed implementation of the kd-tree. Used by point_set,
     *  point_multiset, point_map, point_multimap, box_set, box_multiset and
     *  their equivalent in variant orders: variant_pointer_set, as chosen by
     *  the templates. Not used by neighbor_point_set,
     *  neighbor_point_multiset... Compare must provide strict unordered
     *  ordering along each dimension! Each node maintains the count of its
     *  children nodes plus one.
     */
    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Alloc>
    class Kdtree
    {
      typedef Kdtree<Rank, Key, Value, Compare, Alloc>  Self;

    public:
      // Container intrincsic types
      typedef Rank                                    rank_type;
      typedef typename mutate<Key>::type              key_type;
      typedef typename mutate<Value>::type            value_type;
      typedef Compare                                 key_compare;
      typedef ValueCompare<value_type, key_compare>   value_compare;
      typedef Alloc                                   allocator_type;
      typedef Kdtree_link<Key, Value>                 mode_type;

      // Container iterator related types
      typedef Value*                                  pointer;
      typedef const Value*                            const_pointer;
      typedef Value&                                  reference;
      typedef const Value&                            const_reference;
      typedef std::size_t                             size_type;
      typedef std::ptrdiff_t                          difference_type;

      // Container iterators
      // Conformant to C++ ISO standard, if Key and Value are the same type then
      // iterator and const_iterator shall be the same.
      typedef Node_iterator<mode_type>                iterator;
      typedef Const_node_iterator<mode_type>          const_iterator;
      typedef std::reverse_iterator<iterator>         reverse_iterator;
      typedef std::reverse_iterator<const_iterator>   const_reverse_iterator;

    private:
      typedef typename Alloc::template rebind
      <Kdtree_link<Key, Value> >::other               Link_allocator;
      typedef typename Alloc::template rebind
      <value_type>::other                             Value_allocator;

      // The types used to deal with nodes
      typedef typename mode_type::node_ptr            node_ptr;
      typedef typename mode_type::const_node_ptr      const_node_ptr;
      typedef typename mode_type::link_ptr            link_ptr;
      typedef typename mode_type::const_link_ptr      const_link_ptr;

    private:
      /**
       *  \brief The tree header.
       *
       *  The header node contains pointers to the root, the right most node and
       *  the header node marker (which is the left node of the header). The
       *  header class also contains the pointer to the left most node of the
       *  tree, since the place is already used by the header node marker.
       */
      struct Implementation : Rank
      {
        Implementation(const rank_type& rank, const key_compare& compare,
                       const Link_allocator& alloc)
          : Rank(rank), _count(compare, 0), _header(alloc) { initialize(); }

        Implementation(const Implementation& impl)
          : Rank(impl), _count(impl._count.base(), 0),
            _header(impl._header.base()) { initialize(); }

        void initialize()
        {
          _header().parent = &_header();
          _header().left = &_header(); // the end marker, *must* not change!
          _header().right = &_header();
          _leftmost = &_header();      // the substitute left most pointer
        }

        Compress<key_compare, size_type>           _count;
        Compress<Link_allocator, Node<mode_type> > _header;
        Node<mode_type>* _leftmost;
      } _impl;

    private:
      struct Maximum : key_compare
      {
        Maximum(const key_compare& key_comp, node_ptr node_)
          : key_compare(key_comp), node(node_) { }
        key_compare comp() const
        { return static_cast<key_compare>(*this); }
        node_ptr node;
      };

    private:
      // Internal accessors
      node_ptr get_header()
      { return static_cast<node_ptr>(&_impl._header()); }

      const_node_ptr get_header() const
      { return static_cast<const_node_ptr>(&_impl._header()); }

      node_ptr get_leftmost()
      { return _impl._leftmost; }

      const_node_ptr get_leftmost() const
      { return _impl._leftmost; }

      void set_leftmost(node_ptr x)
      { _impl._leftmost = x; }

      node_ptr get_rightmost()
      { return _impl._header().right; }

      const_node_ptr get_rightmost() const
      { return _impl._header().right; }

      void set_rightmost(node_ptr x)
      { _impl._header().right = x; }

      node_ptr get_root()
      { return _impl._header().parent; }

      const_node_ptr get_root() const
      { return _impl._header().parent; }

      void set_root(node_ptr x)
      { _impl._header().parent = x; }

      rank_type& get_rank()
      { return *static_cast<Rank*>(&_impl); }

      key_compare& get_compare()
      { return _impl._count.base(); }

      Link_allocator& get_link_allocator()
      { return _impl._header.base(); }

      Value_allocator get_value_allocator() const
      { return _impl._header.base(); }

    private:
      // Allocation/Deallocation of nodes
      struct safe_allocator // RAII for exception-safe memory management
      {
        Link_allocator* alloc;
        link_ptr link;
        safe_allocator(Link_allocator& a)
          : alloc(&a), link(0)
        { link = alloc->allocate(1); } // may throw
        ~safe_allocator() { if (link) { alloc->deallocate(link, 1); } }
        link_ptr release() { link_ptr p = link; link=0; return p; }
      };

      node_ptr
      create_node(const value_type& value)
      {
        safe_allocator safe(get_link_allocator());
        // the following may throw but we have RAII on safe_allocator
        get_value_allocator().construct(mutate_pointer(&safe.link->value),
                                        value);
        link_ptr node = safe.release();
        // leave parent uninitialized: its value will change during insertion.
        node->left = 0;
        node->right = 0;
        return node; // silently casts into the parent node pointer
      }

      /**
       *  Destroy and deallocate \c node.
       */
      void
      destroy_node(node_ptr node)
      {
        get_value_allocator().destroy(mutate_pointer(&value(node)));
        get_link_allocator().deallocate(link(node), 1);
      }

      /**
       *  Destroy and deallocate all nodes in the container.
       */
      void
      destroy_all_nodes();

    private:
      /**
       *  Insert a node already allocated into the tree.
       */
      iterator insert_node(node_ptr target_node);

      /**
       *  Insert all the nodes in \p [first,last) into the tree, by
       *  first sorting the nodes according to the dimension of interest.
       *
       *  This function is semi-recursive. It iterates when walking down left
       *  nodes and recurse when walking down right nodes.
       */
      node_ptr rebalance_node_insert
      (typename std::vector<node_ptr>::iterator first,
       typename std::vector<node_ptr>::iterator last, dimension_type dim,
       node_ptr header);

      /**
       *  This function finds the median node in a random iterator range. It
       *  respects the invariant of the tree even when equal values are found in
       *  the tree.
       */
      typename std::vector<node_ptr>::iterator
      median
      (typename std::vector<node_ptr>::iterator first,
       typename std::vector<node_ptr>::iterator last, dimension_type dim);

      /**
       *  Copy the exact sturcture of the sub-tree pointed to by \c
       *  other_node into the current empty tree.
       *
       *  The structural copy preserve all characteristics of the sub-tree
       *  pointed to by \c other_node.
       */
      void copy_structure(const Self& other);

      /**
       *  Copy the content of \p other to the tree and rebalances the
       *  values in the tree resulting in most query having an \c O(log(n))
       *  order of complexity.
       */
      void copy_rebalance(const Self& other);

      /**
       *  Erase the node located at \c node with current dimension
       *  \c node_dim. The function returns the node that was used to replace
       *  the previous one, or null if no replacement was needed.
       */
      node_ptr erase_node(dimension_type node_dim, node_ptr node);

    public:
      // Iterators standard interface
      iterator begin()
      { iterator it; it.node = get_leftmost(); return it; }

      const_iterator begin() const
      { const_iterator it; it.node = get_leftmost(); return it; }

      const_iterator cbegin() const { return begin(); }

      iterator end()
      { return iterator(get_header()); }

      const_iterator end() const
      { return const_iterator(get_header()); }

      const_iterator cend() const { return end(); }

      reverse_iterator rbegin()
      { return reverse_iterator(end()); }

      const_reverse_iterator rbegin() const
      { return const_reverse_iterator(end()); }

      const_reverse_iterator crbegin() const
      { return rbegin(); }

      reverse_iterator rend()
      { return reverse_iterator(begin()); }

      const_reverse_iterator rend() const
      { return const_reverse_iterator(begin()); }

      const_reverse_iterator crend() const
      { return rend(); }

    public:
      /**
       *  Returns the rank used to create the tree.
       */
      rank_type rank() const
      { return *static_cast<const Rank*>(&_impl); }

      /**
       *  Returns the dimension of the tree.
       */
      dimension_type dimension() const
      { return rank()(); }

      /**
       *  Returns the compare function used for the key.
       */
      key_compare key_comp() const
      { return _impl._count.base(); }

      /**
       *  Returns the compare function used for the value.
       */
      value_compare value_comp() const
      { return value_compare(_impl._count.base()); }

      /**
       *  Returns the allocator used by the tree.
       */
      allocator_type
      get_allocator() const { return get_value_allocator(); }

      /**
       *  True if the tree is empty.
       */
      bool empty() const { return (get_header() == get_root()); }

      /**
       *  Returns the number of elements in the K-d tree.
       */
      size_type size() const { return _impl._count(); }

      /**
       *  Returns the number of elements in the K-d tree. Same as size().
       *  \see size()
       */
      size_type count() const { return _impl._count(); }

      /**
       *  Erase all elements in the K-d tree.
       */
      void clear()
      { destroy_all_nodes(); _impl.initialize(); _impl._count() = 0; }

      /**
       *  The maximum number of elements that can be allocated.
       */
      size_type max_size() const
      { return _impl._header.base().max_size(); }

    public:
      Kdtree()
        : _impl(rank_type(), key_compare(), allocator_type())
      { }

      explicit Kdtree(const rank_type& rank_)
        : _impl(rank_, key_compare(), allocator_type())
      { }

      explicit Kdtree(const key_compare& compare_)
        : _impl(rank_type(), compare_, allocator_type())
      { }

      Kdtree(const rank_type& rank_, const key_compare& compare_)
        : _impl(rank_, compare_, allocator_type())
      { }

      Kdtree(const rank_type& rank_, const key_compare& compare_,
             const allocator_type& allocator_)
        : _impl(rank_, compare_, allocator_)
      { }

      /**
       *  Deep copy of \c other into the new tree.
       *
       *  If \c balancing is \c false or unspecified, the copy preserve the
       *  structure of \c other tree. Therefore, all operations should behave
       *  similarly to both trees after the copy.
       *
       *  If \c balancing is \c true, the new tree is a balanced copy of a \c
       *  other tree, resulting in \Onfd order of complexity on most search
       *  functions.
       */
      Kdtree(const Self& other, bool balancing = false) : _impl(other._impl)
      {
        if (!other.empty())
          {
            if (balancing) { copy_rebalance(other); }
            else { copy_structure(other); }
          }
      }

      /**
       *  Assignment of \c other into the tree, with deep copy.
       *
       *  The copy preserve the structure of the tree \c other. Therefore, all
       *  operations should behave similarly to both trees after the copy.
       *
       *  \note  The allocator of the tree is not modified by the assignment.
       */
      Self&
      operator=(const Self& other)
      {
        if (&other != this)
          {
            destroy_all_nodes();
            template_member_assign<rank_type>
              ::do_it(get_rank(), other.rank());
            template_member_assign<key_compare>
              ::do_it(get_compare(), other.key_comp());
            _impl.initialize();
            if (!other.empty()) { copy_structure(other); }
            else _impl._count() = 0;
          }
        return *this;
      }

      /**
       *  Deallocate all nodes in the destructor.
       */
      ~Kdtree()
      { destroy_all_nodes(); }

    public:
      /**
       *  Swap the K-d tree content with others
       *
       *  The extra overhead of the test is not required in common cases:
       *  users intentionally swap different objects.
       *  \warning  This function do not test: (this != &other)
       */
      void
      swap(Self& other)
      {
        if (empty() && other.empty()) return;
        template_member_swap<rank_type>::do_it
          (get_rank(), other.get_rank());
        template_member_swap<key_compare>::do_it
          (get_compare(), other.get_compare());
        template_member_swap<Link_allocator>::do_it
          (get_link_allocator(), other.get_link_allocator());
        if (_impl._header().parent == &_impl._header())
          {
            _impl._header().parent = &other._impl._header();
            _impl._header().right = &other._impl._header();
            _impl._leftmost = &other._impl._header();
          }
        else if (other._impl._header().parent == &other._impl._header())
          {
            other._impl._header().parent = &_impl._header();
            other._impl._header().right = &_impl._header();
            other._impl._leftmost = &_impl._header();
          }
        std::swap(_impl._header().parent, other._impl._header().parent);
        std::swap(_impl._header().right, other._impl._header().right);
        std::swap(_impl._leftmost, other._impl._leftmost);
        if (_impl._header().parent != &_impl._header())
          { _impl._header().parent->parent = &_impl._header(); }
        if (other._impl._header().parent != &other._impl._header())
          { other._impl._header().parent->parent = &other._impl._header(); }
        std::swap(_impl._count(), other._impl._count());
      }

      // Mutable functions
      /**
       *  Rebalance the \kdtree near-optimally, resulting in \Ologn order of
       *  complexity on most search functions.
       *
       *  This function is time & memory consuming. Internally, it creates a
       *  vector of pointer to the node, and thus requires a substantial amount
       *  of memory for a large \kdtree. Ideally, this function should be called
       *  only once, when all the elements you will be working on have been
       *  inserted in the tree.
       *
       *  If you need to insert and erase multiple elements continuously, consider
       *  using other containers than the "idle" family of containers.
       */
      void rebalance();

      /**
       *  Insert a single \c value element in the container.
       */
      iterator
      insert(const value_type& value)
      {
        return insert_node(create_node(value));
      }

      /**
       *  Insert a serie of values in the container at once.
       *
       *  The parameter \c first and \c last only need to be a model of \c
       *  InputIterator. Elements are inserted in a single pass.
       */
      template<typename InputIterator>
      void
      insert(InputIterator first, InputIterator last)
      { for (; first != last; ++first) { insert(*first); } }

      /**
       *  Insert a serie of values in the container at once and rebalance the
       *  container after insertion. This method performs generally more
       *  efficiently than calling insert() then reblance() independantly.
       *
       *  The parameter \c first and \c last only need to be a model of \c
       *  InputIterator. Elements are inserted in a single pass.
       */
      template<typename InputIterator>
      void
      insert_rebalance(InputIterator first, InputIterator last);

      ///@{
      /**
       *  Find the first node that matches with \c key and returns an iterator
       *  to it found, otherwise it returns an iterator to the element past the
       *  end of the container.
       *
       *  Notice that this function returns an iterator only to one of the
       *  elements with that key. To obtain the entire range of elements with a
       *  given value, you can use \ref equal_range.
       *
       *  If this function is called on an empty container, returns an iterator
       *  past the end of the container.
       *
       *  \fractime
       *  \param key the value to be searched for.
       *  \return An iterator to that value or an iterator to the element past
       *  the end of the container.
       */
      iterator
      find(const key_type& key)
      {
        if (empty()) return end();
        return iterator(first_equal(get_root(), 0, rank(),
                                    key_comp(), key).first);
      }

      const_iterator
      find(const key_type& key) const
      {
        if (empty()) return end();
        return const_iterator(first_equal(get_root(), 0, rank(),
                                          key_comp(), key).first);
      }
      ///@}

      /**
       *  Deletes the node pointed to by the iterator.
       *
       *  The iterator must be pointing to an existing node belonging to the
       *  related tree, or dire things may happen.
       */
      void
      erase(iterator pointer);

      /**
       *  Deletes all nodes that match key \c value.
       *  \see    find
       *  \param  value that will be compared with the tree nodes.
       *
       *  The type \c key_type must be equally comparable.
       */
      size_type
      erase(const key_type& value);
    };

    /**
     *  Swap the content of the tree \p left and \p right.
     */
    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Alloc>
    inline void swap
    (Kdtree<Rank, Key, Value, Compare, Alloc>& left,
     Kdtree<Rank, Key, Value, Compare, Alloc>& right)
    { left.swap(right); }

    /**
     *  The == and != operations is performed by first comparing sizes, and if
     *  they match, the elements are compared sequentially using algorithm
     *  std::equal, which stops at the first mismatch. The sequence of element
     *  in each container is extracted using \ref ordered_iterator.
     *
     *  The Value type of containers must provide equal comparison operator
     *  in order to use this operation.
     *
     *  \param lhs Left-hand side container.
     *  \param rhs Right-hand side container.
     */
    ///@{
    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Alloc>
    inline bool
    operator==(const Kdtree<Rank, Key, Value, Compare, Alloc>& lhs,
               const Kdtree<Rank, Key, Value, Compare, Alloc>& rhs)
    {
      return lhs.size() == rhs.size()
        && std::equal(ordered_begin(lhs), ordered_end(lhs),
                      ordered_begin(rhs));
    }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Alloc>
    inline bool
    operator!=(const Kdtree<Rank, Key, Value, Compare, Alloc>& lhs,
               const Kdtree<Rank, Key, Value, Compare, Alloc>& rhs)
    { return !(lhs == rhs); }
    ///@}

    /**
     *  Operations <, >, <= and >= behave as if using algorithm
     *  lexicographical_compare, which compares the elements sequentially using
     *  operator< reflexively, stopping at the first mismatch. The sequence of
     *  element in each container is extracted using \ref ordered_iterator.
     *
     *  The Value type of containers must provide less than comparison operator
     *  in order to use these operations.
     *
     *  \param lhs Left-hand side container.
     *  \param rhs Right-hand side container.
     */
    ///@{
    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Alloc>
    inline bool
    operator<(const Kdtree<Rank, Key, Value, Compare, Alloc>& lhs,
              const Kdtree<Rank, Key, Value, Compare, Alloc>& rhs)
    {
      return std::lexicographical_compare
        (ordered_begin(lhs), ordered_end(lhs),
         ordered_begin(rhs), ordered_end(rhs));
    }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Alloc>
    inline bool
    operator>(const Kdtree<Rank, Key, Value, Compare, Alloc>& lhs,
               const Kdtree<Rank, Key, Value, Compare, Alloc>& rhs)
    { return rhs < lhs; }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Alloc>
    inline bool
    operator<=(const Kdtree<Rank, Key, Value, Compare, Alloc>& lhs,
               const Kdtree<Rank, Key, Value, Compare, Alloc>& rhs)
    { return !(rhs < lhs); }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Alloc>
    inline bool
    operator>=(const Kdtree<Rank, Key, Value, Compare, Alloc>& lhs,
               const Kdtree<Rank, Key, Value, Compare, Alloc>& rhs)
    { return !(lhs < rhs); }
    ///@}

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Alloc>
    inline void
    Kdtree<Rank, Key, Value, Compare, Alloc>
    ::destroy_all_nodes()
    {
      node_ptr node = get_root();
      while (!header(node))
        {
          if (node->left != 0) { node = node->left; }
          else if (node->right != 0) { node = node->right; }
          else
            {
              node_ptr p = node->parent;
              if (header(p))
                {
                  set_root(get_header());
                  set_leftmost(get_header());
                  set_rightmost(get_header());
                }
              else
                {
                  if (p->left == node) { p->left = 0; }
                  else { p->right = 0; }
                }
              SPATIAL_ASSERT_CHECK(node != 0);
              SPATIAL_ASSERT_CHECK(p != 0);
              destroy_node(node);
              node = p;
            }
        }
    }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Alloc>
    inline
    typename Kdtree<Rank, Key, Value, Compare, Alloc>::iterator
    Kdtree<Rank, Key, Value, Compare, Alloc>::insert_node
    (typename mode_type::node_ptr target_node)
    {
      SPATIAL_ASSERT_CHECK(target_node != 0);
      SPATIAL_ASSERT_CHECK(target_node->right == 0);
      SPATIAL_ASSERT_CHECK(target_node->left == 0);
      const key_type& target_key = const_key(target_node);
      node_ptr node = get_root();
      dimension_type node_dim = 0;
      if (!header(node))
        {
          while (true)
            {
              if (key_comp()(node_dim % rank()(), target_key, const_key(node)))
                {
                  if (node->left != 0)
                    { node = node->left; ++node_dim; }
                  else
                    {
                      node->left = target_node;
                      target_node->parent = node;
                      if (node == get_leftmost()) { set_leftmost(target_node); }
                      ++_impl._count();
                      break;
                    }
                }
              else
                {
                  if (node->right != 0)
                    { node = node->right; ++node_dim; }
                  else
                    {
                      node->right = target_node;
                      target_node->parent = node;
                      if (node == get_rightmost())
                      { set_rightmost(target_node); }
                      ++_impl._count();
                      break;
                    }
                }
            }
        }
      else
        {
          SPATIAL_ASSERT_CHECK(_impl._count() == 0);
          target_node->parent = get_header();
          set_root(target_node);
          set_leftmost(target_node);
          set_rightmost(target_node);
          ++_impl._count();
        }
      SPATIAL_ASSERT_CHECK(empty() == false);
      SPATIAL_ASSERT_CHECK(_impl._count() != 0);
      SPATIAL_ASSERT_CHECK(target_node->right == 0);
      SPATIAL_ASSERT_CHECK(target_node->left == 0);
      SPATIAL_ASSERT_CHECK(target_node->parent != 0);
      SPATIAL_ASSERT_INVARIANT(*this);
      return iterator(target_node);
    }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Alloc>
    inline void
    Kdtree<Rank, Key, Value, Compare, Alloc>::copy_structure
    (const Self& other)
    {
      SPATIAL_ASSERT_CHECK(!other.empty());
      SPATIAL_ASSERT_CHECK(empty());
      const_node_ptr other_node = other.get_root();
      node_ptr node = create_node(const_value(other_node)); // may throw
      node->parent = get_header();
      set_root(node);
      try
        {
          while(!header(other_node))
            {
              if (other_node->left != 0)
                {
                  other_node = other_node->left;
                  node_ptr target = create_node(const_value(other_node));
                  target->parent = node;
                  target->left = target->right = 0;
                  node->left = target;
                  node = node->left;
                }
              else if (other_node->right != 0)
                {
                  other_node = other_node->right;
                  node_ptr target = create_node(const_value(other_node));
                  target->parent = node;
                  target->right = target->left = 0;
                  node->right = target;
                  node = node->right;
                }
              else
                {
                  const_node_ptr p = other_node->parent;
                  while (!header(p) && (other_node == p->right || p->right == 0))
                    {
                      other_node = p;
                      node = node->parent;
                      p = other_node->parent;
                    }
                  other_node = p;
                  node = node->parent;
                  if (!header(p))
                    {
                      other_node = other_node->right;
                      node_ptr target = create_node(const_value(other_node));
                      target->parent = node;
                      target->right = target->left = 0;
                      node->right = target;
                      node = node->right;
                    }
                }
            }
          SPATIAL_ASSERT_CHECK(!empty());
          SPATIAL_ASSERT_CHECK(header(other_node));
          SPATIAL_ASSERT_CHECK(header(node));
        }
      catch (...)
        { clear(); throw; } // clean-up before re-throw
      set_leftmost(minimum(get_root()));
      set_rightmost(maximum(get_root()));
      _impl._count() = other.size();
      SPATIAL_ASSERT_CHECK(size() != 0);
      SPATIAL_ASSERT_CHECK(size() == other.size());
      SPATIAL_ASSERT_INVARIANT(*this);
    }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Alloc>
    inline void
    Kdtree<Rank, Key, Value, Compare, Alloc>
    ::copy_rebalance(const Self& other)
    {
      SPATIAL_ASSERT_CHECK(empty());
      SPATIAL_ASSERT_CHECK(!other.empty());
      std::vector<node_ptr> ptr_store;
      ptr_store.reserve(other.size()); // may throw
      try
        {
          for(const_iterator i = other.begin(); i != other.end(); ++i)
            { ptr_store.push_back(create_node(*i)); } // may throw
        }
      catch (...)
        {
          for(typename std::vector<node_ptr>::iterator i = ptr_store.begin();
              i != ptr_store.end(); ++i)
            { destroy_node(*i); }
          throw;
        }
      set_root(rebalance_node_insert(ptr_store.begin(), ptr_store.end(), 0,
                                     get_header()));
      node_ptr root = get_root();
      while (root->left != 0) root = root->left;
      set_leftmost(root);
      root = get_root();
      while (root->right != 0) root = root->right;
      set_rightmost(root);
      _impl._count() = other.size();
      SPATIAL_ASSERT_CHECK(!empty());
      SPATIAL_ASSERT_CHECK(size() != 0);
      SPATIAL_ASSERT_INVARIANT(*this);
    }

    template <typename InputIterator>
    inline std::ptrdiff_t
    random_access_iterator_distance
    (InputIterator first, InputIterator last, std::random_access_iterator_tag)
    { return last - first; }

    template <typename InputIterator>
    inline std::ptrdiff_t
    random_access_iterator_distance
    (InputIterator, InputIterator, std::input_iterator_tag)
    { return 0; }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Alloc>
    template <typename InputIterator>
    inline void
    Kdtree<Rank, Key, Value, Compare, Alloc>
    ::insert_rebalance(InputIterator first, InputIterator last)
    {
      if (first == last && empty()) return;
      std::vector<node_ptr> ptr_store;
      ptr_store.reserve // may throw
        (size()
         + random_access_iterator_distance
         (first, last,
          typename std::iterator_traits<InputIterator>::iterator_category()));
      try
        {
          for(InputIterator i = first; i != last; ++i)
            { ptr_store.push_back(create_node(*i)); } // may throw
        }
      catch (...)
        {
          for(typename std::vector<node_ptr>::iterator i = ptr_store.begin();
              i != ptr_store.end(); ++i)
            { destroy_node(*i); }
          throw;
        }
      for(iterator i = begin(); i != end(); ++i)
        { ptr_store.push_back(i.node); }
      set_root(rebalance_node_insert(ptr_store.begin(), ptr_store.end(), 0,
                                     get_header()));
      node_ptr root = get_root();
      while (root->left != 0) root = root->left;
      set_leftmost(root);
      root = get_root();
      while (root->right != 0) root = root->right;
      set_rightmost(root);
      _impl._count() = ptr_store.size();
      SPATIAL_ASSERT_CHECK(!empty());
      SPATIAL_ASSERT_CHECK(size() != 0);
      SPATIAL_ASSERT_INVARIANT(*this);
    }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Alloc>
    inline void
    Kdtree<Rank, Key, Value, Compare, Alloc>::rebalance()
    {
      if (empty()) return;
      std::vector<node_ptr> ptr_store;
      ptr_store.reserve(size()); // may throw
      for(iterator i = begin(); i != end(); ++i)
        { ptr_store.push_back(i.node); }
      set_root(rebalance_node_insert(ptr_store.begin(), ptr_store.end(), 0,
                                     get_header()));
      node_ptr node = get_root();
      while (node->left != 0) node = node->left;
      set_leftmost(node);
      node = get_root();
      while (node->right != 0) node = node->right;
      set_rightmost(node);
      SPATIAL_ASSERT_CHECK(!empty());
      SPATIAL_ASSERT_CHECK(size() != 0);
      SPATIAL_ASSERT_INVARIANT(*this);
    }

    template<typename Compare, typename Node_ptr>
    struct mapping_compare
    {
      Compare compare;
      dimension_type dimension;

      mapping_compare(const Compare& c, dimension_type d)
        : compare(c), dimension(d) { }

      bool
      operator() (const Node_ptr& x, const Node_ptr& y) const
      {
        return compare(dimension, const_key(x), const_key(y));
      }
    };

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Alloc>
    inline typename
    std::vector<typename Kdtree<Rank, Key, Value, Compare, Alloc>::node_ptr>
    ::iterator
    Kdtree<Rank, Key, Value, Compare, Alloc>::median
    (typename std::vector<node_ptr>::iterator first,
     typename std::vector<node_ptr>::iterator last,
     dimension_type dim)
    {
      SPATIAL_ASSERT_CHECK(first != last);
      mapping_compare<Compare, node_ptr> less(key_comp(), dim);
      // Memory ordering varies between machines, so we use '/ 2' and not '>> 1'
      if (first == (last - 1)) return first;
      typename std::vector<node_ptr>::iterator mid = first + (last - first) / 2;
      std::nth_element(first, mid, last, less);
      typename std::vector<node_ptr>::iterator seek = mid;
      typename std::vector<node_ptr>::iterator pivot = mid;
      do
        {
          --seek;
          SPATIAL_ASSERT_CHECK(!less(*mid, *seek));
          if (!less(*seek, *mid))
            {
              --pivot;
              if (seek != pivot) { std::swap(*seek, *pivot); }
              // pivot and mid are equal at this point:
              SPATIAL_ASSERT_CHECK(!less(*pivot, *mid) && !less(*mid, *pivot));
            }
        }
      while (seek != first);
      SPATIAL_ASSERT_CHECK(pivot != last);
      return pivot;
    }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Alloc>
    inline typename Kdtree<Rank, Key, Value, Compare, Alloc>::node_ptr
    Kdtree<Rank, Key, Value, Compare, Alloc>
    ::rebalance_node_insert
    (typename std::vector<node_ptr>::iterator first,
     typename std::vector<node_ptr>::iterator last,
     dimension_type dim, node_ptr parent)
    {
      SPATIAL_ASSERT_CHECK(first != last);
      SPATIAL_ASSERT_CHECK(dim < dimension());
      typename std::vector<node_ptr>::iterator med
        = median(first, last, dim);
      node_ptr root = *med;
      root->parent = parent;
      dim = incr_dim(rank(), dim);
      if (med + 1 != last)
        { root->right = rebalance_node_insert(med + 1, last, dim, root); }
      else root->right = 0;
      last = med;
      parent = root;
      while(first != last)
        {
          med = median(first, last, dim);
          node_ptr node = *med;
          parent->left = node;
          node->parent = parent;
          dim = incr_dim(rank(), dim);
          if (med + 1 != last)
            { node->right = rebalance_node_insert(med + 1, last, dim, node); }
          else node->right = 0;
          last = med;
          parent = node;
        }
      parent->left = 0;
      SPATIAL_ASSERT_CHECK(parent->left == 0);
      SPATIAL_ASSERT_CHECK(root->parent != root);
      return root;
    }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Alloc>
    inline typename Kdtree<Rank, Key, Value, Compare, Alloc>::node_ptr
    Kdtree<Rank, Key, Value, Compare, Alloc>::erase_node
    (dimension_type node_dim, node_ptr node)
    {
      SPATIAL_ASSERT_CHECK(node != 0);
      SPATIAL_ASSERT_CHECK(!header(node));
      node_ptr first_swap = 0;
      while (node->right != 0 || node->left != 0)
        {
          // If there is nothing on the right, to preserve the invariant, we
          // need to shift the whole sub-tree to the right. This K-d tree
          // rotation is not documented anywhere I've searched. The previous
          // known rotation by J. L. Bentely for erasing nodes in the K-d tree
          // is incorrect for strict invariant (left nodes strictly less than
          // root node). This could explain while it is hard to find an
          // implementation of K-d Tree with the O(log(n)) erase function
          // predicted in his paper.
          if (node->right == 0)
            {
              node->right = node->left;
              node->left = 0;
              if (get_rightmost() == node)
                { set_rightmost(maximum(node->right)); }
              const_node_ptr seeker = node->right;
              if (get_leftmost() == seeker) { set_leftmost(node); }
              else
                {
                  while (seeker->left != 0)
                    {
                      seeker = seeker->left;
                      if (get_leftmost() == seeker)
                        { set_leftmost(node); break; }
                    }
                }
            }
          std::pair<node_ptr, dimension_type> candidate
            = minimum_mapping(node->right, incr_dim(rank(), node_dim),
                              rank(), node_dim, key_comp());
          if (get_rightmost() == candidate.first)
            { set_rightmost(node); }
          if (get_leftmost() == node)
            { set_leftmost(candidate.first); }
          if (first_swap == 0)
            { first_swap = candidate.first; }
          swap_node(candidate.first, node);
          node = candidate.first;
          node_dim = candidate.second;
        }
      SPATIAL_ASSERT_CHECK(node != 0);
      SPATIAL_ASSERT_CHECK(node->right == 0);
      SPATIAL_ASSERT_CHECK(node->left == 0);
      SPATIAL_ASSERT_CHECK(node->parent != 0);
      node_ptr p = node->parent;
      if (header(p))
        {
          SPATIAL_ASSERT_CHECK(count() == 1);
          set_root(get_header());
          set_leftmost(get_header());
          set_rightmost(get_header());
        }
      else if (p->left == node)
        {
          p->left = 0;
          if (get_leftmost() == node) { set_leftmost(p); }
        }
      else
        {
          p->right = 0;
          if (get_rightmost() == node) { set_rightmost(p); }
        }
      --_impl._count();
      SPATIAL_ASSERT_CHECK((get_header() == get_root())
                           ? (_impl._count() == 0) : true);
      destroy_node(node);
      SPATIAL_ASSERT_INVARIANT(*this);
      return first_swap;
    }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Alloc>
    inline void
    Kdtree<Rank, Key, Value, Compare, Alloc>
    ::erase(iterator target)
    {
      except::check_node_iterator(target.node);
      node_ptr node = target.node;
      dimension_type node_dim = rank()() - 1;
      while(!header(node))
        {
          node_dim = details::incr_dim(rank(), node_dim);
          node = node->parent;
        }
      except::check_iterator(node, get_header());
      erase_node(node_dim, target.node);
    }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Alloc>
    inline
    typename Kdtree<Rank, Key, Value, Compare, Alloc>::size_type
    Kdtree<Rank, Key, Value, Compare, Alloc>::erase
    (const key_type& key)
    {
      if (empty()) return 0;
      node_ptr node = get_root();
      dimension_type depth;
      import::tie(node, depth)
        = first_equal(node, 0, rank(), key_comp(), key);
      if (header(node)) return 0;
      size_type cnt = 0;
      for (;;)
        {
          node_ptr tmp = erase_node(depth % rank()(), node);
          ++cnt;
          if (tmp == 0) break; // no further node to erase for sure!
          import::tie(node, depth)
            = first_equal(tmp, depth, rank(), key_comp(), key);
          if (tmp->parent == node) break; // no more match
        }
      return cnt;
    }

  } // namespace details
} // namespace spatial

#endif // SPATIAL_KDTREE_HPP
