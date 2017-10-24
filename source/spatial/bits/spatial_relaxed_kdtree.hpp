// -*- C++ -*-

/**
 *  \file   spatial_relaxed_kdtree.hpp
 *  Defines a \kdtree with a relaxed invariant. On a given dimension, if
 *  coordinates between a root node and a child node are equal, the child node
 *  may be placed either on the left or the right of the tree. The relaxed
 *  \kdtree is a self-balancing tree.
 *
 *  Balancing the tree occurs when the number of children in the left part of
 *  a node differs from that in the right part, by a significant margin. The
 *  \kdtree is then rebalanced slightly, by shifting one child node from one
 *  side to the other side.
 *
 *  Relaxed \kdtree are implemented as scapegoat tree, and so, for each node
 *  they store an additional size_t count of number of children in the
 *  node. Although they are self-balancing, they are not as memory efficient as
 *  regular \kdtree.
 *
 *  Additionally, scapegoat roatations are not as efficient as the one in the
 *  \rbtree. Sadly, \rbtree rotation cannot be applied for \kdtree, due to the
 *  complexity of in invariant. However scapegoat rotations still provide
 *  amortized insertion and deletion times on the tree, and allow for multiple
 *  balancing policies to be defined.
 *
 *  \see Relaxed_kdtree
 */

#ifndef SPATIAL_RELAXED_KDTREE_HPP
#define SPATIAL_RELAXED_KDTREE_HPP

#include <utility> // for std::pair
#include <algorithm> // for std::min, std::max, std::equal,
                     // std::lexicographical_compare

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
  /**
   *  This policy triggers rebalancing for the node when the
   *  difference in weight between left or right is more than a half. The
   *  default policy for rebalancing.
   *
   *  In effect, this policy leaves the tree roughly balanced: the path from
   *  the root to the furthest leaf is no more than twice as long as the path
   *  from the root to the nearest leaf.
   *
   *  This policy is adequate in many cases cause it prevents worse-case
   *  insertion or deletion time, and worst-case run-time on many search
   *  algorithms, and does not require a large amount of rebalancing.
   */
  struct loose_balancing
  {
    /**
     *  Rebalancing predicate.
     *  \param left  The weight at the left
     *  \param right The weight at the right
     *  \return  true indicate that reblancing must occurs, otherwise false.
     */
    template <typename Rank>
    bool
    operator()(const Rank&, weight_type left, weight_type right) const
    {
      if (left < right)
        { return ((left + 1) < (right >> 1)); }
      else
        { return ((right + 1) < (left >> 1)); }
    }
  };

  /**
   *  A policy that balances a node if the difference in weight
   *  between left and right is higher than the current rank of the tree.
   *
   *  The dimension is choosen as a limiter because balancing the tree even
   *  more strictly will not have an impact on most search algorithm, since
   *  dimensions at each level of the \kdtree are rotated.
   */
  struct tight_balancing
  {
    /**
     *  Rebalancing predicate.
     *  \param rank  The current dimension function to use for examination.
     *  \param left  The weight at the left
     *  \param right The weight at the right
     *  \return true Indicate that reblancing must occurs, otherwise false.
     */
    template <typename Rank>
    bool
    operator()(const Rank& rank, weight_type left, weight_type right) const
    {
      weight_type weight = static_cast<weight_type>(rank());
      if (weight < 2) weight = 2;
      if (left < right)
        { return ((right - left) > weight); }
      else
        { return ((left - right) > weight); }
    }
  };

  /**
   *  A policy that balances a node if the difference in weight
   *  between left and right is higher than 2 (two).
   *
   *  The value of 2 (two) is chosen because it offers optimal balancing in a
   *  \kdtree, this is useful when \point_multiset, \point_multimap,
   *  \box_multiset or \box_multimap is used as a source of a
   *  \idle_point_multiset, \idle_point_multimap \idle_box_multiset or
   *  \idle_box_multimap, respectively.
   */
  struct perfect_balancing
  {
    /**
     *  Rebalancing predicate.
     *  \param left  The weight at the left
     *  \param right The weight at the right
     *  \return true Indicate that reblancing must occurs, otherwise false.
     */
    template <typename Rank>
    bool
    operator()(const Rank&, weight_type left, weight_type right) const
    {
      if (left < right)
        { return ((right - left) > 2); }
      else
        { return ((left - right) > 2); }
    }
  };

  namespace details
  {
    /**
     *  Detailed implementation of the kd-tree. Used by point_set,
     *  point_multiset, point_map, point_multimap, box_set, box_multiset and
     *  their equivalent in variant orders: variant_pointer_set, as chosen by
     *  the templates.
     */
    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Balancing, typename Alloc>
    class Relaxed_kdtree
    {
      typedef Relaxed_kdtree<Rank, Key, Value, Compare, Balancing,
                             Alloc>                   Self;

    public:
      // Container intrincsic types
      typedef Rank                                    rank_type;
      typedef typename mutate<Key>::type              key_type;
      typedef typename mutate<Value>::type            value_type;
      typedef Relaxed_kdtree_link<Key, Value>         mode_type;
      typedef Compare                                 key_compare;
      typedef ValueCompare<value_type, key_compare>   value_compare;
      typedef Alloc                                   allocator_type;
      typedef Balancing                               balancing_policy;

      // Container iterator related types
      typedef Value*                                  pointer;
      typedef const Value*                            const_pointer;
      typedef Value&                                  reference;
      typedef const Value&                            const_reference;
      typedef std::size_t                             size_type;
      typedef std::ptrdiff_t                          difference_type;

      // Container iterators
      // Conformant to C++ standard, if Key and Value are the same type then
      // iterator and const_iterator shall be the same.
      typedef Node_iterator<mode_type>                iterator;
      typedef Const_node_iterator<mode_type>          const_iterator;
      typedef std::reverse_iterator<iterator>         reverse_iterator;
      typedef std::reverse_iterator<const_iterator>   const_reverse_iterator;

    private:
      typedef typename Alloc::template rebind
      <Relaxed_kdtree_link<Key, Value> >::other       Link_allocator;
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
      struct Implementation : rank_type
      {
        Implementation(const rank_type& rank, const key_compare& compare,
                       const Balancing& balance, const Link_allocator& alloc)
          : Rank(rank), _compare(balance, compare),
            _header(alloc, Node<mode_type>()) { initialize(); }

        Implementation(const Implementation& impl)
          : Rank(impl), _compare(impl._compare.base(), impl._compare()),
            _header(impl._header.base()) { initialize(); }

        void initialize()
        {
          _header().parent = &_header();
          _header().left = &_header(); // the end marker, *must* not change!
          _header().right = &_header();
          _leftmost = &_header();      // the substitute left most pointer
        }

        Compress<Balancing, key_compare> _compare;
        Compress<Link_allocator, Node<mode_type> > _header;
        node_ptr _leftmost;
      } _impl;

    private:
      // Internal mutable accessor
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
      { return _impl._compare(); }

      balancing_policy& get_balancing()
      { return _impl._compare.base(); }

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
        // the following may throw. But we have RAII on safe_allocator
        get_value_allocator().construct(mutate_pointer(&safe.link->value),
                                        value);
        link_ptr node = safe.release();
        // leave parent uninitialized: its value will change during insertion.
        node->left = 0;
        node->right = 0;
        node->weight = 1;
        return node; // silently cast into base type node_ptr.
      }

      node_ptr
      clone_node(const_node_ptr node)
      {
        node_ptr new_node = create_node(const_value(node));
        link(new_node)->weight = const_link(node)->weight;
        return new_node; // silently cast into base type node_ptr.
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
       *  Copy the exact sturcture of the sub-tree pointed to by \c
       *  other_node into the current empty tree.
       *
       *  The structural copy preserve all characteristics of the sub-tree
       *  pointed to by \c other_node.
       */
      void
      copy_structure(const Self& other);

      /**
       *  Insert the new node \c new_node into the tree located at node. If the
       *  last parameter is 0, the node will also be created.
       *
       *  \param node_dim  The current dimension for the node.
       *  \param node      The node below which the new key shall be inserted.
       *  \param new_node  The new node to insert
       */
      iterator
      insert_node(dimension_type node_dim, node_ptr node, node_ptr new_node);

      /**
       *  Erase the node pointed by \c node.
       *
       *  \c node cannot be null or a root node, or else dire things may
       *  happen. This function does not destroy the node and it does not
       *  decrement weight to the parents of node.
       *
       *  \param dim      The current dimension for \c node
       *  \param node     The node to erase
       *  \return         The address of the node that has replaced the erased
       *                  one or \c node if it was a leaf.
       */
      node_ptr erase_node(dimension_type dim, node_ptr node);

      /**
       *  Erase the node pointed by \c node and balance tree up to
       *  header. Finish the work started by erase_node by reducing weight in
       *  parent of \c node, up to the header.
       *
       *  \c node cannot be null or a root node, or else dire things may
       *  happen. This function does not destroy the node.
       *
       *  \param dim      The current dimension for \c node
       *  \param node     The node to erase
       */
      void erase_node_balance(dimension_type dim, node_ptr node);

      /**
       *  Attempt to balance the current node.
       *
       *  \c node cannot be null or a root node, or else dire things may
       *  happen. This function does not destroy the node and it does not
       *  modify weight of the parents of node.
       *
       *  \param dim      The current dimension for \c node
       *  \param node     The node to erase
       *  \return         The address of the node that has replaced the current
       *                  node.
       */
      node_ptr balance_node(dimension_type dim, node_ptr node);

    public:
      // Iterators standard interface
      iterator begin()
      { iterator it; it.node = get_leftmost(); return it; }

      const_iterator begin() const
      { const_iterator it; it.node = get_leftmost(); return it; }

      const_iterator cbegin() const
      { return begin(); }

      iterator end()
      { return iterator(get_header()); }

      const_iterator end() const
      { return const_iterator(get_header()); }

      const_iterator cend() const
      { return end(); }

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
      // Functors accessors
      /**
       *  Returns the balancing policy for the container.
       */
      balancing_policy balancing() const
      { return _impl._compare.base(); }

      /**
       *  Returns the rank type used internally to get the number of dimensions
       *  in the container.
       */
      rank_type rank() const
      { return *static_cast<const rank_type*>(&_impl); }

      /**
       *  Returns the dimension of the container.
       */
      dimension_type
      dimension() const
      { return rank()(); }

      /**
       *  Returns the compare function used for the key.
       */
      key_compare key_comp() const
      { return _impl._compare(); }

      /**
       *  Returns the compare function used for the value.
       */
      value_compare value_comp() const
      { return value_compare(_impl._compare()); }

      /**
       *  Returns the allocator used by the tree.
       */
      allocator_type
      get_allocator() const { return get_value_allocator(); }

      /**
       *  True if the tree is empty.
       */
      bool
      empty() const
      { return ( get_root() == get_header() ); }

      /**
       *  Returns the number of elements in the K-d tree.
       */
      size_type
      size() const
      {
        return empty() ? 0
          : static_cast<const_link_ptr>(get_root())->weight;
      }

      /**
       *  Returns the number of elements in the K-d tree. Same as size().
       *  \see size()
       */
      size_type
      count() const
      { return size(); }

      /**
       *  The maximum number of elements that can be allocated.
       */
      size_type
      max_size() const
      { return _impl._header.base().max_size(); }

      ///@{
      /**
       *  Find the first node that matches with \c key and returns an
       *  iterator to it found, otherwise it returns an iterator to the element
       *  past the end of the container.
       *
       *  Notice that this function returns an iterator only to one of the
       *  elements with that key. To obtain the entire range of elements with a
       *  given value, you can use \ref spatial::equal_range().
       *
       *  \param key The value search.
       *  \return An iterator to that value or an iterator to the element past
       *  the end of the container.
       */
      iterator
      find(const key_type& key)
      {
        if (empty()) return end();
        return iterator(first_equal(get_root(), 0, rank(), key_comp(), key)
                        .first);
      }

      const_iterator
      find(const key_type& key) const
      {
        if (empty()) return end();
        return const_iterator(first_equal(get_root(), 0, rank(), key_comp(),
                                          key)
                              .first);
      }
      ///@}

    public:
      Relaxed_kdtree()
        : _impl(rank_type(), key_compare(), balancing_policy(),
                Link_allocator()) { }

      explicit Relaxed_kdtree(const rank_type& rank_)
        : _impl(rank_, Compare(), Balancing(), Link_allocator())
      { }

      Relaxed_kdtree(const rank_type& rank_, const key_compare& compare_)
        : _impl(rank_, compare_, Balancing(), Link_allocator())
      { }

      Relaxed_kdtree(const rank_type& rank_, const key_compare& compare_,
                     const balancing_policy& balancing_)
        : _impl(rank_, compare_, balancing_, Link_allocator())
      { }

      Relaxed_kdtree(const rank_type& rank_, const key_compare& compare_,
                     const balancing_policy& balancing_,
                     const allocator_type& allocator_)
        : _impl(rank_, compare_, balancing_, allocator_)
      { }

      /**
       *  Deep copy of \c other into the new tree.
       *
       *  This operation results in an identical the structure to the \p other
       *  tree. Therefore, all operations should behave similarly to both trees
       *  after the copy.
       */
      Relaxed_kdtree(const Relaxed_kdtree& other) : _impl(other._impl)
      { if (!other.empty()) { copy_structure(other); } }

      /**
       *  Assignment of \c other into the tree, with deep copy.
       *
       *  The copy preserve the structure of the tree \c other. Therefore, all
       *  operations should behave similarly to both trees after the copy.
       *
       *  \note  Allocator is not modified with this assignment and remains the
       *  same.
       */
      Relaxed_kdtree&
      operator=(const Relaxed_kdtree& other)
      {
        if (&other != this)
          {
            destroy_all_nodes();
            template_member_assign<rank_type>
              ::do_it(get_rank(), other.rank());
            template_member_assign<key_compare>
              ::do_it(get_compare(), other.key_comp());
            template_member_assign<balancing_policy>
              ::do_it(get_balancing(), other.balancing());
            _impl.initialize();
            if (!other.empty()) { copy_structure(other); }
          }
        return *this;
      }

      /**
       *  Deallocate all nodes in the destructor.
       */
      ~Relaxed_kdtree()
      { destroy_all_nodes(); }

    public:
      // Mutable functions
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
        template_member_swap<balancing_policy>::do_it
          (get_balancing(), other.get_balancing());
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
      }

      /**
       *  Erase all elements in the K-d tree.
       */
      void
      clear()
      {
        destroy_all_nodes();
        _impl.initialize();
      }

      /**
       *  Insert a single key \c key in the tree.
       */
      iterator
      insert(const value_type& value)
      {
        node_ptr target_node = create_node(value); // may throw
        node_ptr node = get_root();
        if (header(node))
          {
            // insert root node in empty tree
            set_leftmost(target_node);
            set_rightmost(target_node);
            set_root(target_node);
            target_node->parent = node;
            return iterator(target_node);
          }
        else
          {
            iterator i = insert_node(0, node, target_node);
            SPATIAL_ASSERT_INVARIANT(*this);
            return i;
          }
      }

      /**
       *  Insert a serie of values in the tree at once.
       */
      template<typename InputIterator>
      void
      insert(InputIterator first, InputIterator last)
      { for (; first != last; ++first) { insert(*first); } }

      // Deletion
      /**
       *  Deletes the node pointed to by the iterator.
       *
       *  The iterator must be pointing to an existing node belonging to the
       *  related tree, or dire things may happen.
       */
      void
      erase(iterator position);

      /**
       *  Deletes all nodes that match key \c value.
       *  \param  key To be compared with the tree nodes.
       *
       *  The type \c key_type must be equally comparable.
       */
      size_type
      erase(const key_type& key);
    };

    /**
     *  Swap the content of the relaxed \kdtree \p left and \p right.
     */
    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Balancing, typename Alloc>
    inline void swap
    (Relaxed_kdtree<Rank, Key, Value, Compare, Balancing, Alloc>& left,
     Relaxed_kdtree<Rank, Key, Value, Compare, Balancing, Alloc>& right)
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
              typename Balancing, typename Alloc>
    inline bool
    operator==(const Relaxed_kdtree
               <Rank, Key, Value, Compare, Balancing, Alloc>& lhs,
               const Relaxed_kdtree
               <Rank, Key, Value, Compare, Balancing, Alloc>& rhs)
    {
      return lhs.size() == rhs.size()
        && std::equal(ordered_begin(lhs), ordered_end(lhs),
                      ordered_begin(rhs));
    }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Balancing, typename Alloc>
    inline bool
    operator!=(const Relaxed_kdtree
               <Rank, Key, Value, Compare, Balancing, Alloc>& lhs,
               const Relaxed_kdtree
               <Rank, Key, Value, Compare, Balancing, Alloc>& rhs)
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
              typename Balancing, typename Alloc>
    inline bool
    operator<(const Relaxed_kdtree
              <Rank, Key, Value, Compare, Balancing, Alloc>& lhs,
              const Relaxed_kdtree
              <Rank, Key, Value, Compare, Balancing, Alloc>& rhs)
    {
      return std::lexicographical_compare
        (ordered_begin(lhs), ordered_end(lhs),
         ordered_begin(rhs), ordered_end(rhs));
    }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Balancing, typename Alloc>
    inline bool
    operator>(const Relaxed_kdtree
              <Rank, Key, Value, Compare, Balancing, Alloc>& lhs,
              const Relaxed_kdtree
              <Rank, Key, Value, Compare, Balancing, Alloc>& rhs)
    { return rhs < lhs; }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Balancing, typename Alloc>
    inline bool
    operator<=(const Relaxed_kdtree
               <Rank, Key, Value, Compare, Balancing, Alloc>& lhs,
               const Relaxed_kdtree
               <Rank, Key, Value, Compare, Balancing, Alloc>& rhs)
    { return !(rhs < lhs); }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Balancing, typename Alloc>
    inline bool
    operator>=(const Relaxed_kdtree
               <Rank, Key, Value, Compare, Balancing, Alloc>& lhs,
               const Relaxed_kdtree
               <Rank, Key, Value, Compare, Balancing, Alloc>& rhs)
    { return !(lhs < rhs); }
    ///@}

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Balancing, typename Alloc>
    inline void
    Relaxed_kdtree<Rank, Key, Value, Compare, Balancing, Alloc>
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
              typename Balancing, typename Alloc>
    inline void
    Relaxed_kdtree<Rank, Key, Value, Compare, Balancing, Alloc>
    ::copy_structure
    (const Self& other)
    {
      SPATIAL_ASSERT_CHECK(!other.empty());
      SPATIAL_ASSERT_CHECK(empty());
      const_node_ptr other_node = other.get_root();
      node_ptr node = clone_node(other_node); // may throw
      node->parent = get_header();
      set_root(node);
      try
        {
          while(!header(other_node))
            {
              if (other_node->left != 0)
                {
                  other_node = other_node->left;
                  node_ptr target = clone_node(other_node);
                  target->parent = node;
                  node->left = target;
                  node = node->left;
                }
              else if (other_node->right != 0)
                {
                  other_node = other_node->right;
                  node_ptr target = clone_node(other_node);
                  target->parent = node;
                  node->right = target;
                  node = node->right;
                }
              else
                {
                  const_node_ptr p = other_node->parent;
                  while (!header(p)
                         && (other_node == p->right || p->right == 0))
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
                      node_ptr target = clone_node(other_node);
                      target->parent = node;
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
    }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Balancing, typename Alloc>
    inline
    typename Relaxed_kdtree<Rank, Key, Value, Compare, Balancing, Alloc>::node_ptr
    Relaxed_kdtree<Rank, Key, Value, Compare, Balancing, Alloc>
    ::balance_node
    (dimension_type node_dim, node_ptr node)
    {
      const_node_ptr p = node->parent; // Parent is not swapped, node is!
      bool left_node = (p->left == node);
      // erase first...
      erase_node(node_dim, node);
      node_ptr replacing = header(p) ? p->parent
        : (left_node ? p->left : p->right);
      // ...then re-insert.
      insert_node(node_dim, replacing, node);
      return header(p) ? p->parent : (left_node ? p->left : p->right);
    }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Balancing, typename Alloc>
    inline
    typename Relaxed_kdtree<Rank, Key, Value, Compare, Balancing, Alloc>::iterator
    Relaxed_kdtree<Rank, Key, Value, Compare, Balancing, Alloc>
    ::insert_node
    (dimension_type node_dim, node_ptr node, node_ptr target_node)
    {
      SPATIAL_ASSERT_CHECK(node != 0);
      SPATIAL_ASSERT_CHECK(!header(node));
      while (true)
        {
          SPATIAL_ASSERT_CHECK
            ((node->right ? const_link(node->right)->weight : 0)
             + (node->left ? const_link(node->left)->weight: 0)
             + 1 == const_link(node)->weight);
          // Balancing equal values on either side of the tree
          if (key_comp()(node_dim, const_key(target_node), const_key(node))
              || (!key_comp()(node_dim,
                              const_key(node), const_key(target_node))
                  && (node->left == 0
                      || (node->right != 0
                          && const_link(node->left)->weight
                          < const_link(node->right)->weight))))
            {
              if (node->left == 0)
                {
                  node->left = target_node;
                  target_node->parent = node;
                  if (get_leftmost() == node)
                    { set_leftmost(target_node); }
                  ++link(node)->weight;
                  break;
                }
              else
                {
                  if(balancing()
                     (rank(),
                      1 + (node->left ? const_link(node->left)->weight : 0),
                      (node->right ? const_link(node->right)->weight : 0)))
                    {
                      node = balance_node(node_dim, node); // recursive!
                    }
                  else
                    {
                      ++link(node)->weight;
                      node = node->left;
                      node_dim = incr_dim(rank(), node_dim);
                    }
                }
            }
          else
            {
              if (node->right == 0)
                {
                  node->right = target_node;
                  target_node->parent = node;
                  if (get_rightmost() == node)
                    { set_rightmost(target_node); }
                  ++link(node)->weight;
                  break;
                }
              else
                {
                  if(balancing()
                     (rank(),
                      (node->left ? const_link(node->left)->weight : 0),
                      1 + (node->right ? const_link(node->right)->weight : 0)))
                    {
                      node = balance_node(node_dim, node);  // recursive!
                    }
                  else
                    {
                      ++link(node)->weight;
                      node = node->right;
                      node_dim = incr_dim(rank(), node_dim);
                    }
                }
            }
        }
      SPATIAL_ASSERT_CHECK(target_node != 0);
      SPATIAL_ASSERT_CHECK(!header(target_node));
      SPATIAL_ASSERT_CHECK(!header(target_node->parent));
      SPATIAL_ASSERT_CHECK(target_node->right == 0);
      SPATIAL_ASSERT_CHECK(target_node->left == 0);
      SPATIAL_ASSERT_CHECK(target_node->parent != 0);
      return iterator(target_node);
    }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Balancing, typename Alloc>
    inline
    typename Relaxed_kdtree<Rank, Key, Value, Compare, Balancing, Alloc>
    ::node_ptr
    Relaxed_kdtree<Rank, Key, Value, Compare, Balancing, Alloc>
    ::erase_node
    (dimension_type node_dim, node_ptr node)
    {
      SPATIAL_ASSERT_CHECK(node != 0);
      SPATIAL_ASSERT_CHECK(!header(node));
      // never ask to erase a single root node in this function
      SPATIAL_ASSERT_CHECK(get_rightmost() != get_leftmost());
      node_ptr parent = node->parent;
      while (node->right != 0 || node->left != 0)
        {
          std::pair<node_ptr, dimension_type> candidate;
          if (node->left != 0
              && (node->right == 0
                  || const_link(node->right)->weight
                  < const_link(node->left)->weight))
            {
              candidate
                = maximum_mapping(node->left, incr_dim(rank(), node_dim),
                                  rank(), node_dim, key_comp());
              if (get_leftmost() == candidate.first)
                { set_leftmost(node); }
              if (get_rightmost() == node)
                { set_rightmost(candidate.first); }
            }
          else
            {
              candidate
                = minimum_mapping(node->right,
                                  incr_dim(rank(), node_dim),
                                  rank(), node_dim, key_comp());
              if (get_rightmost() == candidate.first)
                { set_rightmost(node); }
              if (get_leftmost() == node)
                { set_leftmost(candidate.first); }
            }
          swap_node(node, candidate.first);
          node = candidate.first;
          node_dim = candidate.second;
        }
      SPATIAL_ASSERT_CHECK(!header(node));
      SPATIAL_ASSERT_CHECK(node != 0);
      SPATIAL_ASSERT_CHECK(node->right == 0);
      SPATIAL_ASSERT_CHECK(node->left == 0);
      SPATIAL_ASSERT_CHECK(node->parent != 0);
      node_ptr p = node->parent;
      if (p->left == node)
        {
          p->left = 0;
          if (get_leftmost() == node) { set_leftmost(p); }
        }
      else
        {
          p->right = 0;
          if (get_rightmost() == node) { set_rightmost(p); }
        }
      // decrease count and rebalance parents up to parent
      while(node->parent != parent)
        {
          node = node->parent;
          node_dim = decr_dim(rank(), node_dim);
          SPATIAL_ASSERT_CHECK(const_link(node)->weight > 1);
          --link(node)->weight;
          if(balancing()
             (rank(),
              (node->left ? const_link(node->left)->weight : 0),
              (node->right ? const_link(node->right)->weight : 0)))
            {
              node = balance_node(node_dim, node);  // recursive!
            }
        }
      SPATIAL_ASSERT_CHECK(!header(node));
      SPATIAL_ASSERT_CHECK(node != 0);
      return node;
    }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Balancing, typename Alloc>
    inline void
    Relaxed_kdtree<Rank, Key, Value, Compare, Balancing, Alloc>
    ::erase_node_balance
    (dimension_type node_dim, node_ptr node)
    {
      SPATIAL_ASSERT_CHECK(!header(node));
      SPATIAL_ASSERT_CHECK(node != 0);
      if (node == get_root()
          && node->left == 0 && node->right == 0)
        {
          // if it's a single root tree, erase root quickly
          set_root(get_header());
          set_leftmost(get_header());
          set_rightmost(get_header());
        }
      else
        {
          node_ptr p = node->parent;
          erase_node(node_dim, node);
          node_dim = decr_dim(rank(), node_dim);
          while(!header(p))
            {
              SPATIAL_ASSERT_CHECK(const_link(p)->weight > 1);
              --link(p)->weight;
              if(balancing()
                 (rank(),
                  (node->left ? const_link(node->left)->weight : 0),
                  (node->right ? const_link(node->right)->weight : 0)))
                { p = balance_node(node_dim, p); } // balance node
              p = p->parent;
              node_dim = decr_dim(rank(), node_dim);
            }
        }
    }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Balancing, typename Alloc>
    inline void
    Relaxed_kdtree<Rank, Key, Value, Compare, Balancing, Alloc>
    ::erase
    (iterator target)
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
      erase_node_balance(node_dim, target.node);
      destroy_node(target.node);
      SPATIAL_ASSERT_INVARIANT(*this);
    }

    template <typename Rank, typename Key, typename Value, typename Compare,
              typename Balancing, typename Alloc>
    inline
    typename Relaxed_kdtree<Rank, Key, Value, Compare, Balancing, Alloc>
    ::size_type
    Relaxed_kdtree<Rank, Key, Value, Compare, Balancing, Alloc>
    ::erase
    (const key_type& key)
    {
      size_type cnt = 0;
      while (!empty())
        {
          node_ptr node;
          dimension_type depth;
          import::tie(node, depth)
            = first_equal(get_root(), 0, rank(), key_comp(), key);
          if (node == get_header()) break;
          erase_node_balance(depth % rank()(), node);
          destroy_node(node);
          ++cnt;
        }
      SPATIAL_ASSERT_INVARIANT(*this);
      return cnt;
    }

  } // namespace details
} // namespace spatial

#endif // SPATIAL_RELAXED_KDTREE_HPP
