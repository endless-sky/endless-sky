// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   equal_iterator.hpp
 *  Provides equal_iterator and all the functions around it.
 */

#ifndef SPATIAL_EQUAL_ITERATOR_HPP
#define SPATIAL_EQUAL_ITERATOR_HPP

#include "spatial.hpp"
#include "bits/spatial_equal.hpp"
#include "bits/spatial_bidirectional.hpp"
#include "bits/spatial_compress.hpp"

namespace spatial
{
  /**
   *  This type provides an iterator to iterate through all elements of a
   *  container that match a given key, passed as a parameter to the
   *  constructor. The given key is called the model.
   *
   *  \tparam Container The container upon which these iterator relate to.
   *  \headerfile equal_iterator.hpp
   */
  template <typename Container>
  class equal_iterator
    : public details::Bidirectional_iterator
      <typename Container::mode_type,
       typename Container::rank_type>
  {
  private:
    //! The preorder iterator without its criterion
    typedef typename details::Bidirectional_iterator
    <typename Container::mode_type,
     typename Container::rank_type> Base;

  public:
    using Base::node;
    using Base::node_dim;
    using Base::rank;

    //! The type used to store the model key to be looked up in the container.
    typedef typename Container::key_type key_type;

    //! The comparison functor used to compare keys.
    typedef typename Container::key_compare key_compare;

    //! \empty
    equal_iterator() { }

    /**
     *  Build an equal iterator from the node and current dimension of a
     *  container's element.
     *
     *  This constructor should be used only when the dimension of the node
     *  pointed to by iter is known. If in doubt, use the other
     *  constructor. This constructor perform slightly faster than the other,
     *  since the dimension does not have to be calculated. Note however that
     *  the calculation of the dimension in the other iterator takes slightly
     *  longer than \Olog in general, and so it is not likely to affect the
     *  performance of your application in any major way.
     *
     *  \param container The container being iterated.
     *  \param value The key to look for.
     *  \param ptr An iterator on the type Ct.
     *  \param dim The node's dimension for the node pointed to by node.
     *  \param container The container being iterated.
     */
    equal_iterator
    (Container& container, const key_type& value, dimension_type dim,
     typename Container::mode_type::node_ptr ptr)
      : Base(container.rank(), ptr, dim), _data(container.key_comp(), value)
    { }

    //! Increments the iterator and returns the incremented value. Prefer to
    //! use this form in \c for loops.
    equal_iterator<Container>& operator++()
    {
      import::tie(node, node_dim)
        = increment_equal(node, node_dim, rank(), _data.base(), _data());
      return *this;
    }

    //! Increments the iterator but returns the value of the iterator before
    //! the increment. Prefer to use the other form in \c for loops.
    equal_iterator<Container> operator++(int)
    {
      equal_iterator<Container> x(*this);
      import::tie(node, node_dim)
        = increment_equal(node, node_dim, rank(), _data.base(), _data());
      return x;
    }

    //! Decrements the iterator and returns the decremented value. Prefer to
    //! use this form in \c for loops.
    equal_iterator<Container>& operator--()
    {
      import::tie(node, node_dim)
        = decrement_equal(node, node_dim, rank(), _data.base(), _data());
      return *this;
    }

    //! Decrements the iterator but returns the value of the iterator before
    //! the decrement. Prefer to use the other form in \c for loops.
    equal_iterator<Container> operator--(int)
    {
      equal_iterator<Container> x(*this);
      import::tie(node, node_dim)
        = decrement_equal(node, node_dim, rank(), _data.base(), _data());
      return x;
    }

    //! Return the value of key used to find equal keys in the container.
    key_type value() const { return _data(); }

    //! Return the functor used to compare keys in this iterator.
    key_compare key_comp() const { return _data.base(); }

  private:
    //! The model key used to find equal keys in the container.
    details::Compress<key_compare, key_type> _data;
  };

  /**
   *  This type provides an iterator to iterate through all elements of a
   *  container that match a given key, passed as a parameter to the
   *  constructor. The given key is called the model.
   *
   *  The values returned by this iterator will not be mutable.
   *
   *  \tparam Ct The container upon which these iterator relate to.
   */
  template <typename Container>
  class equal_iterator<const Container>
    : public details::Const_bidirectional_iterator
      <typename Container::mode_type,
       typename Container::rank_type>
  {
  private:
    //! The preorder iterator without its criterion.
    typedef details::Const_bidirectional_iterator
    <typename Container::mode_type,
     typename Container::rank_type> Base;

  public:
    using Base::node;
    using Base::node_dim;
    using Base::rank;

    //! The type used to store the model key to be looked up in the container.
    typedef typename Container::key_type key_type;

    //! The comparison functor used to compare keys.
    typedef typename Container::key_compare key_compare;

    //! \empty
    equal_iterator() { }

    /**
     *  Build an equal iterator from the node and current dimension of a
     *  container's element.
     *
     *  This constructor should be used only when the dimension of the node
     *  pointed to by iter is known. If in doubt, use the other
     *  constructor. This constructor perform slightly faster since the
     *  dimension does not have to be calculated.
     *
     *  \param container The container being iterated.
     *  \param value The key to look for.
     *  \param dim The dimension associated with \c ptr when checking the
     *  invariant in \c container.
     *  \param ptr A pointer to a node belonging to \c container.
     */
    equal_iterator
    (const Container& container, const key_type& value, dimension_type dim,
     typename Container::mode_type::const_node_ptr ptr)
      : Base(container.rank(), ptr, dim), _data(container.key_comp(), value)
    { }

    //! Convertion of an iterator into a const_iterator is permitted.
    equal_iterator(const equal_iterator<Container>& iter)
      : Base(iter.rank(), iter.node, iter.node_dim),
        _data(iter.key_comp(), iter.value()) { }

    //! Increments the iterator and returns the incremented value. Prefer to
    //! use this form in \c for loops.
    equal_iterator<const Container>& operator++()
    {
      import::tie(node, node_dim)
        = increment_equal(node, node_dim, rank(), _data.base(), _data());
      return *this;
    }

    //! Increments the iterator but returns the value of the iterator before
    //! the increment. Prefer to use the other form in \c for loops.
    equal_iterator<const Container> operator++(int)
    {
      equal_iterator<const Container> x(*this);
      import::tie(node, node_dim)
        = increment_equal(node, node_dim, rank(), _data.base(), _data());
      return x;
    }

    //! Decrements the iterator and returns the decremented value. Prefer to
    //! use this form in \c for loops.
    equal_iterator<const Container>& operator--()
    {
      import::tie(node, node_dim)
        = decrement_equal(node, node_dim, rank(), _data.base(), _data());
      return *this;
    }

    //! Decrements the iterator but returns the value of the iterator before
    //! the decrement. Prefer to use the other form in \c for loops.
    equal_iterator<const Container> operator--(int)
    {
      equal_iterator<const Container> x(*this);
      import::tie(node, node_dim)
        = preorder_decrement(node, node_dim, rank(), _data.base(), _data());
      return x;
    }

    //! Returns the value used to find equivalent keys in the container.
    key_type value() const { return _data(); }

    //! Returns the functor used to compare keys in this iterator.
    key_compare key_comp() const { return _data.base(); }

  private:
    //! The model key used to find equal keys in the container.
    details::Compress<key_compare, key_type> _data;
  };

  /**
   *  Find the past-the-end element in \c container that compares equally to \c
   *  value, using \c container's \c key_compare comparator.
   *
   *  \tparam Container The container type being iterated.
   *  \param container The container being iterated.
   *  \param value A value to find matches among other keys stored in the
   *  container.
   */
  ///@{
  template <typename Container>
  inline equal_iterator<Container>
  equal_end(Container& container,
            const typename equal_iterator<Container>::key_type& value)
  {
    return equal_iterator<Container>
      (container, value, container.dimension() - 1,
       container.end().node); // At header, dim = rank - 1
  }

  template <typename Container>
  inline equal_iterator<const Container>
  equal_cend(const Container& container,
             const typename equal_iterator<Container>::key_type& value)
  { return equal_end(container, value); }
  ///@}

  /**
   *  Find the first element in \c container that compares equally to \c value,
   *  using \c container's \c key_compare comparator.
   *
   *  \tparam Container The container type being iterated.
   *  \param container The container being iterated.
   *  \param value A value to find matches among other keys stored in the
   *  container.
   */
  ///@{
  template <typename Container>
  inline equal_iterator<Container>
  equal_begin(Container& container,
              const typename equal_iterator<Container>::key_type& value)
  {
    if (container.empty()) return equal_end(container, value);
    typename equal_iterator<Container>::node_ptr node
      = container.end().node->parent;
    dimension_type depth;
    import::tie(node, depth)
      = first_equal(node, 0, container.rank(),
                    container.key_comp(), value);
    return equal_iterator<Container>(container, value, depth, node);
  }

  template <typename Container>
  inline equal_iterator<const Container>
  equal_cbegin(const Container& container,
               const typename equal_iterator<Container>::key_type& value)
  { return equal_begin(container, value); }
  ///@}

  /**
   *  This structure defines a pair of mutable equal iterator.
   *
   *  \tparam Ct The container to which these iterator relate to.
   *  \see equal_iterator
   */
  template <typename Container>
  struct equal_iterator_pair
    : std::pair<equal_iterator<Container>,
                equal_iterator<Container> >
  {
    /**
     *  A pair of iterators that represents a range (that is: a range of
     *  elements to iterate, and not an orthogonal range).
     */
    typedef std::pair<equal_iterator<Container>,
                      equal_iterator<Container> > Base;

    //! Empty constructor.
    equal_iterator_pair() { }

    //! Regular constructor that builds a equal_iterator_pair out of 2
    //! equal_iterators.
    equal_iterator_pair(const equal_iterator<Container>& a,
                        const equal_iterator<Container>& b)
          : Base(a, b) { }
  };

  /**
   *  This structure defines a pair of constant equal iterator.
   *
   *  \tparam Ct The container to which these iterator relate to.
   *  \see equal_iterator
   */
  template <typename Container>
  struct equal_iterator_pair<const Container>
    : std::pair<equal_iterator<const Container>,
                equal_iterator<const Container> >
  {
    /**
     *  A pair of iterators that represents a range (that is: a range of
     *  elements to iterate, and not an orthogonal range).
     */
    typedef std::pair<equal_iterator<const Container>,
                      equal_iterator<const Container> > Base;

    //! Empty constructor.
    equal_iterator_pair() { }

    //! Regular constructor that builds a equal_iterator_pair out of 2
    //! equal_iterators.
    equal_iterator_pair(const equal_iterator<const Container>& a,
                        const equal_iterator<const Container>& b)
      : Base(a, b) { }

    //! Convert a mutable equal iterator pair into a const equal iterator
    //! pair.
    equal_iterator_pair(const equal_iterator_pair<Container>& p)
      : Base(p.first, p.second) { }
  };

  /**
   *  Creates a pair of iterator that represent the range of element in the
   *  container that are equal to the model given.
   *
   *  \tparam Container The type of the container iterated.
   *  \param container The container being iterated.
   *  \param model The key to find in \c container.
   */
  ///@(
  template <typename Container>
  inline equal_iterator_pair<Container>
  equal_range(Container& container,
              const typename equal_iterator<Container>::key_type& model)
  {
    return equal_iterator_pair<Container>
      (equal_begin(container, model), equal_end(container, model));
  }

  template <typename Container>
  inline equal_iterator_pair<const Container>
  equal_crange(const Container& container,
               const typename equal_iterator<Container>::key_type& model)
  {
    return equal_iterator_pair<const Container>
      (equal_begin(container, model), equal_end(container, model));
  }
  ///@)

  namespace details
  {
    template <typename NodePtr, typename Rank, typename KeyCompare,
              typename Key>
    inline std::pair<NodePtr, dimension_type>
    last_equal(NodePtr node, dimension_type depth, const Rank rank,
               const KeyCompare& key_comp, const Key& key)
    {
      SPATIAL_ASSERT_CHECK(!header(node));
      SPATIAL_ASSERT_CHECK(node != 0);
      for (;;)
        {
          if (!key_comp(depth % rank(), key, const_key(node))
              && node->right != 0)
            { node = node->right; ++depth; }
          else if (!key_comp(depth % rank(), const_key(node), key)
                   && node->left != 0)
            { node = node->left; ++depth; }
          else break;
        }
      for (;;)
        {
          dimension_type test = 0;
          for(; test < rank() && !(key_comp(test, key, const_key(node))
                                   || key_comp(test, const_key(node), key));
              ++test);
          if (test == rank())
            { return std::make_pair(node, depth); }
          NodePtr prev_node = node;
          node = node->parent; --depth;
          if (header(node))
            { return std::make_pair(node, depth); }
          if (node->right == prev_node
              && !key_comp(depth % rank(), const_key(node), key)
              && node->left != 0)
            {
              node = node->left; ++depth;
              for (;;)
                {
                  if (!key_comp(depth % rank(), key, const_key(node))
                      && node->right != 0)
                    { node = node->right; ++depth; }
                  else if (!key_comp(depth % rank(), const_key(node), key)
                           && node->left != 0)
                    { node = node->left; ++depth; }
                  else break;
                }
            }
        }
    }

    template <typename NodePtr, typename Rank, typename KeyCompare,
              typename Key>
    inline std::pair<NodePtr, dimension_type>
    increment_equal(NodePtr node, dimension_type depth, const Rank rank,
                    const KeyCompare& key_comp, const Key& key)
    {
      SPATIAL_ASSERT_CHECK(!header(node));
      SPATIAL_ASSERT_CHECK(node != 0);
      for (;;)
        {
          if (!key_comp(depth % rank(), const_key(node), key)
              && node->left != 0)
            { node = node->left; ++depth; }
          else if (!key_comp(depth % rank(), key, const_key(node))
                   && node->right != 0)
            { node = node->right; ++depth; }
          else
            {
              NodePtr prev_node = node;
              node = node->parent; --depth;
              while (!header(node)
                     && (prev_node == node->right
                         || key_comp(depth % rank(), key, const_key(node))
                         || node->right == 0))
                {
                  prev_node = node;
                  node = node->parent; --depth;
                }
              if (!header(node))
                { node = node->right; ++depth; }
              else { return std::make_pair(node, depth); }
            }
          dimension_type test = 0;
          for(; test < rank() && !(key_comp(test, key, const_key(node))
                                   || key_comp(test, const_key(node), key));
              ++test);
          if (test == rank())
            { return std::make_pair(node, depth); }
        }
    }

    template <typename NodePtr, typename Rank, typename KeyCompare,
              typename Key>
    inline std::pair<NodePtr, dimension_type>
    decrement_equal(NodePtr node, dimension_type depth, const Rank rank,
                    const KeyCompare& key_comp, const Key& key)
    {
      if (header(node))
        { return last_equal(node->parent, 0, rank, key_comp, key); }
      SPATIAL_ASSERT_CHECK(node != 0);
      NodePtr prev_node = node;
      node = node->parent; --depth;
      while (!header(node))
        {
          if (node->right == prev_node
              && !key_comp(depth % rank(), const_key(node), key)
              && node->left != 0)
            {
              node = node->left; ++depth;
              for (;;)
                {
                  if (!key_comp(depth % rank(), key, const_key(node))
                      && node->right != 0)
                    { node = node->right; ++depth; }
                  else if (!key_comp(depth % rank(), const_key(node), key)
                           && node->left != 0)
                    { node = node->left; ++depth; }
                  else break;
                }
            }
          dimension_type test = 0;
          for(; test < rank() && !(key_comp(test, key, const_key(node))
                                   || key_comp(test, const_key(node), key));
              ++test);
          if (test == rank()) break;
          prev_node = node;
          node = node->parent; --depth;
        }
      return std::make_pair(node, depth);
    }

  } // namespace details
} // namespace spatial

#endif // SPATIAL_EQUAL_ITERATOR_HPP
