// -*- C++ -*-
//
// Copyright Sylvain Bougerel 2009 - 2013.
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file COPYING or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/**
 *  \file   spatial_region.hpp
 *  Contains the definition of \region_iterator. These iterators walk through
 *  all items in the container that are contained within an orthogonal region
 *  defined by a predicate.
 */

#ifndef SPATIAL_REGION_HPP
#define SPATIAL_REGION_HPP

#include <utility> // std::pair<> and std::make_pair()
#include "spatial_bidirectional.hpp"
#include "spatial_except.hpp"
#include "spatial_import_tuple.hpp"

namespace spatial
{
  /**
   *  A model of \region_predicate that defines an orthogonal region and checks
   *  if a value of type \c Key is contained within the boundaries marked by \c
   *  lower and \c upper.
   *
   *  To be very specific, given a dimension \f$d\f$ we define that \f$x\f$ is
   *  contained in the boundaries \f$(lower, upper)\f$ if:
   *
   *  \f[lower_d <= x_d < upper_d\f]
   *
   *  Simply stated, \ref bounds used in a \region_iterator will match all keys
   *  that are within the region defined by \c lower and \c upper, but not
   *  "touching" the \c upper edge.
   *
   *  \tparam Key The type used during the comparison.
   *  \tparam Compare The comparison functor using to compare 2 objects of type
   *  \c Key along the same dimension.
   *  \concept_region_predicate
   */
  template <typename Key, typename Compare>
  class bounds
    : private Compare // empty member optimization
  {
  public:
    /**
     *  The default constructor leaves everything un-initialized
     */
    bounds()
      : Compare(), _lower(), _upper() { }

    /**
     *  Set the \c lower and \c upper boundary for the orthogonal region
     *  search.
     */
    bounds(const Compare& compare, const Key& lower, const Key& upper)
      : Compare(compare), _lower(lower), _upper(upper)
    { }

    /**
     *  The operator that returns \c true if the \c key is within the
     *  boundaries, \c false if it isn't.
     */
    relative_order
    operator()(dimension_type dim, dimension_type, const Key& key) const
    {
      return (Compare::operator()(dim, key, _lower)
              ? below
              : (Compare::operator()(dim, key, _upper)
                 ? matching
                 : above));
    }

  private:
    /**
     *  The lower bound for the orthogonal region.
     */
    Key _lower;

    /**
     *  The upper bound for the orthogonal region.
     */
    Key _upper;
  };

  /**
   *  A \ref bounds factory that takes in a \c container, 2 arguments
   *  \c lower and \c upper, and returns a fully constructed \ref bounds
   *  object.
   *
   *  This factory also checks that \c lower is always less or equal to \c upper
   *  for every dimension. If it is not, it raises \ref invalid_bounds.
   *
   *  Because of this extra check, it is safer to invoke the factory rather than
   *  the constructor to build this object, especially if you are expecting user
   *  inputs.
   *
   *  \param container A container to iterate.
   *  \param lower A key defining the lower bound of \ref bounds.
   *  \param upper A key defining the upper bound of \ref bounds.
   *  \return a constructed \ref bounds object.
   *  \throws invalid_bounds
   */
  template <typename Tp>
  bounds<typename Tp::key_type,
         typename Tp::key_compare>
  make_bounds
  (const Tp& container,
   const typename Tp::key_type& lower,
   const typename Tp::key_type& upper)
  {
    except::check_bounds(container, lower, upper);
    return bounds
      <typename Tp::key_type,
      typename Tp::key_compare>
      (container.key_comp(), lower, upper);
  }

  /**
   *  This type provides both an iterator and a constant iterator to iterate
   *  through all elements of a tree that match an orthogonal region defined by
   *  a predicate. If no predicate is provided, the orthogonal region search
   *  default to a \ref bounds predicate, which matches all points
   *  contained within an orthogonal region of space formed by 2 points,
   *  inclusive of lower values, but exclusive of upper values.
   *
   *  \tparam Ct The container upon which these iterator relate to.
   *  \tparam Predicate A model of \region_predicate, defaults to \ref
   *  bounds
   *  \see region_query<>::iterator
   *  \see region_query<>::const_iterator
   */
  template <typename Container, typename Predicate
            = bounds<typename Container::key_type,
                     typename Container::key_compare> >
  class region_iterator
    : public details::Bidirectional_iterator
      <typename Container::mode_type,
       typename Container::rank_type>
  {
  private:
    typedef typename details::Bidirectional_iterator
    <typename Container::mode_type,
     typename Container::rank_type> Base;

  public:
    using Base::node;
    using Base::node_dim;
    using Base::rank;

    //! Uninitialized iterator.
    region_iterator() { }

    /**
     *  Build a region iterator from a container's iterator type.
     *
     *  This constructor should be used in the general case where the
     *  dimension for the node pointed to by \c iter is not known. The
     *  dimension of the node will be recomputed from the given iterator by
     *  iterating through all parents until the header node has been
     *  reached. This iteration is bounded by \Olog in case the tree is
     *  perfectly balanced.
     *
     *  \param container The container being iterated.
     *  \param pred A model of the \region_predicate concept.
     *  \param iter An iterator on the type Ct.
     */
    region_iterator(Container& container, const Predicate& pred,
                    typename Container::iterator iter)
      : Base(container.rank(), iter.node, depth(iter.node)),
        _pred(pred) { }

    /**
     *  Build a region iterator from the node and current dimension of a
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
     *  \param pred A model of the \region_predicate concept.
     *  \param ptr An iterator on the type Ct.
     *  \param dim The node's dimension for the node pointed to by node.
     *  \param container The container being iterated.
     */
    region_iterator
    (Container& container, const Predicate& pred, dimension_type dim,
     typename Container::mode_type::node_ptr ptr)
      : Base(container.rank(), ptr, dim), _pred(pred) { }

    //! Increments the iterator and returns the incremented value. Prefer to
    //! use this form in \c for loops.
    region_iterator<Container, Predicate>& operator++()
    {
      import::tie(node, node_dim)
        = increment_region(node, node_dim, rank(), _pred);
      return *this;
    }

    //! Increments the iterator but returns the value of the iterator before
    //! the increment. Prefer to use the other form in \c for loops.
    region_iterator<Container, Predicate> operator++(int)
    {
      region_iterator<Container, Predicate> x(*this);
      import::tie(node, node_dim)
        = increment_region(node, node_dim, rank(), _pred);
      return x;
    }

    //! Decrements the iterator and returns the decremented value. Prefer to
    //! use this form in \c for loops.
    region_iterator<Container, Predicate>& operator--()
    {
      import::tie(node, node_dim)
        = decrement_region(node, node_dim, rank(), _pred);
      return *this;
    }

    //! Decrements the iterator but returns the value of the iterator before
    //! the decrement. Prefer to use the other form in \c for loops.
    region_iterator<Container, Predicate> operator--(int)
    {
      region_iterator<Container, Predicate> x(*this);
      import::tie(node, node_dim)
        = decrement_region(node, node_dim, rank(), _pred);
      return x;
    }

    //! Return the key_comparator used by the iterator
    Predicate predicate() const { return _pred; }

  private:
    //! The related data for the iterator.
    Predicate _pred;
  };

  /**
   *  This type provides both an iterator and a constant iterator to iterate
   *  through all elements of a tree that match an orthogonal region defined by
   *  a predicate. If no predicate is provided, the orthogonal region search
   *  default to a \ref bounds predicate, which matches all points
   *  contained within an orthogonal region of space formed by 2 points,
   *  inclusive of lower values, but exclusive of upper values.
   *
   *  \tparam Ct The container upon which these iterator relate to.
   *  \tparam Predicate A model of \region_predicate, defaults to \ref
   *  bounds
   *  \see region_query<>::iterator
   *  \see region_query<>::const_iterator
   */
  template <typename Container, typename Predicate>
  class region_iterator<const Container, Predicate>
    : public details::Const_bidirectional_iterator
      <typename Container::mode_type,
       typename Container::rank_type>
  {
  private:
    typedef details::Const_bidirectional_iterator
    <typename Container::mode_type,
     typename Container::rank_type> Base;

  public:
    using Base::node;
    using Base::node_dim;
    using Base::rank;

    //! \empty
    region_iterator() { }

    /**
     *  Build a region iterator from a container's iterator type.
     *
     *  \param container The container being iterated.
     *  \param pred A model of the \region_predicate concept.
     *  \param iter An iterator on the type Ct.
     *
     *  This constructor should be used in the general case where the
     *  dimension for the node pointed to by \c iter is not known. The
     *  dimension of the node will be recomputed from the given iterator by
     *  iterating through all parents until the header node has been
     *  reached. This iteration is bounded by \Olog in case the tree is
     *  perfectly balanced.
     */
    region_iterator(const Container& container, const Predicate& pred,
                    typename Container::const_iterator iter)
      : Base(container.rank(), iter.node, depth(iter.node)),
        _pred(pred) { }

    /**
     *  Build a region iterator from the node and current dimension of a
     *  container's element.
     *
     *  \param container The container being iterated.
     *  \param pred A model of the \region_predicate concept.
     *  \param dim The dimension associated with \c ptr when checking the
     *  invariant in \c container.
     *  \param ptr A pointer to a node belonging to \c container.
     *
     *  This constructor should be used only when the dimension of the node
     *  pointed to by iter is known. If in doubt, use the other
     *  constructor. This constructor perform slightly faster than the other,
     *  since the dimension does not have to be calculated. Note however that
     *  the calculation of the dimension in the other iterator takes slightly
     *  longer than \Olog in general, and so it is not likely to affect the
     *  performance of your application in any major way.
     */
    region_iterator
    (const Container& container, const Predicate& pred, dimension_type dim,
     typename Container::mode_type::const_node_ptr ptr)
      : Base(container.rank(), ptr, dim), _pred(pred) { }

    //! Convertion of an iterator into a const_iterator is permitted.
    region_iterator(const region_iterator<Container, Predicate>& iter)
      : Base(iter.rank(), iter.node, iter.node_dim), _pred(iter.predicate()) { }

    //! Increments the iterator and returns the incremented value. Prefer to
    //! use this form in \c for loops.
    region_iterator<const Container, Predicate>& operator++()
    {
      import::tie(node, node_dim)
        = increment_region(node, node_dim, rank(), _pred);
      return *this;
    }

    //! Increments the iterator but returns the value of the iterator before
    //! the increment. Prefer to use the other form in \c for loops.
    region_iterator<const Container, Predicate> operator++(int)
    {
      region_iterator<const Container, Predicate> x(*this);
      import::tie(node, node_dim)
        = increment_region(node, node_dim, rank(), _pred);
      return x;
    }

    //! Decrements the iterator and returns the decremented value. Prefer to
    //! use this form in \c for loops.
    region_iterator<const Container, Predicate>& operator--()
    {
      import::tie(node, node_dim)
        = decrement_region(node, node_dim, rank(), _pred);
      return *this;
    }

    //! Decrements the iterator but returns the value of the iterator before
    //! the decrement. Prefer to use the other form in \c for loops.
    region_iterator<const Container, Predicate> operator--(int)
    {
      region_iterator<const Container, Predicate> x(*this);
      import::tie(node, node_dim)
        = decrement_region(node, node_dim, rank(), _pred);
      return x;
    }

    //! Return the key_comparator used by the iterator
    Predicate predicate() const { return _pred; }

  private:
    //! The related data for the iterator.
    Predicate _pred;
  };

  namespace details
  {
    /**
     *  In the children of the node pointed to by \c iter, find the first
     *  matching iterator in the region delimited by \c Predicate, using
     *  in-order transversal.  If no match can be found, returns past-the-end.
     *
     *  \param iter A valid region iterator.
     *  \tparam Predicate  The type of predicate for the orthogonal query.
     *  \tparam Ct The type of container to look up.
     *  \return  An iterator pointing the minimum, or past-the-end.
     */
    template <typename Container, typename Predicate>
    region_iterator<Container, Predicate>&
    minimum_region(region_iterator<Container, Predicate>& iter);

    /**
     *  In the children of the node pointed to by \c iter, find the last
     *  matching iterator in the region delimited by \c Predicate, using
     *  in-order transversal. If no match can be found, returns past-the-end.
     *
     *  \param iter A valid region iterator.
     *  \tparam Predicate  The type of predicate for the orthogonal query.
     *  \tparam Ct The type of container to look up.
     *  \return  An iterator pointing the maximum, or past-the-end.
     */
    template <typename Container, typename Predicate>
    region_iterator<Container, Predicate>&
    maximum_region(region_iterator<Container, Predicate>& iter);

  } // namespace details

  template <typename Container, typename Predicate>
  inline region_iterator<Container, Predicate>
  region_end(Container& container, const Predicate& pred)
  {
    return region_iterator<Container, Predicate>
      (container, pred, container.dimension() - 1,
       container.end().node); // At header, dim = rank - 1
  }

  template <typename Container, typename Predicate>
  inline region_iterator<const Container, Predicate>
  region_cend(const Container& container, const Predicate& pred)
  { return region_end(container, pred); }

  template <typename Container>
  inline region_iterator<Container>
  region_end(Container& container,
             const typename Container::key_type& lower,
             const typename Container::key_type& upper)
  { return region_end(container, make_bounds(container, lower, upper)); }

  template <typename Container>
  inline region_iterator<const Container>
  region_cend(const Container& container,
              const typename Container::key_type& lower,
              const typename Container::key_type& upper)
  { return region_cend(container, make_bounds(container, lower, upper)); }

  template <typename Container, typename Predicate>
  inline region_iterator<Container, Predicate>
  region_begin(Container& container, const Predicate& pred)
  {
    if (container.empty()) return region_end(container, pred);
    typename region_iterator<Container>::node_ptr node
      = container.end().node->parent;
    dimension_type depth;
    import::tie(node, depth)
      = first_region(node, 0, container.rank(), pred);
    return region_iterator<Container, Predicate>(container, pred, depth, node);
  }

  template <typename Container, typename Predicate>
  inline region_iterator<const Container, Predicate>
  region_cbegin(const Container& container, const Predicate& pred)
  { return region_begin(container, pred); }

  template <typename Container>
  inline region_iterator<Container>
  region_begin(Container& container,
               const typename Container::key_type& lower,
               const typename Container::key_type& upper)
  { return region_begin(container, make_bounds(container, lower, upper)); }

  template <typename Container>
  inline region_iterator<const Container>
  region_cbegin(const Container& container,
                const typename Container::key_type& lower,
                const typename Container::key_type& upper)
  { return region_begin(container, make_bounds(container, lower, upper)); }

  /**
   *  This structure defines a pair of mutable region iterator.
   *
   *  \tparam Ct The container to which these iterator relate to.
   *  \see region_iterator
   */
  template <typename Container, typename Predicate
            = bounds<typename Container::key_type,
                     typename Container::key_compare> >
  struct region_iterator_pair
    : std::pair<region_iterator<Container, Predicate>,
                region_iterator<Container, Predicate> >
  {
    /**
     *  A pair of iterators that represents a range (that is: a range of
     *  elements to iterate, and not an orthogonal range).
     */
    typedef std::pair<region_iterator<Container, Predicate>,
                      region_iterator<Container, Predicate> > Base;

    //! Empty constructor.
    region_iterator_pair() { }

    //! Regular constructor that builds a region_iterator_pair out of 2
    //! region_iterators.
    region_iterator_pair(const region_iterator<Container, Predicate>& a,
                         const region_iterator<Container, Predicate>& b)
          : Base(a, b) { }
  };

  /**
   *  This structure defines a pair of constant region iterator.
   *
   *  \tparam Ct The container to which these iterator relate to.
   *  \see region_iterator
   */
  template <typename Container, typename Predicate>
  struct region_iterator_pair<const Container, Predicate>
    : std::pair<region_iterator<const Container, Predicate>,
                region_iterator<const Container, Predicate> >
  {
    /**
     *  A pair of iterators that represents a range (that is: a range of
     *  elements to iterate, and not an orthogonal range).
     */
    typedef std::pair<region_iterator<const Container, Predicate>,
                      region_iterator<const Container, Predicate> > Base;

    //! Empty constructor.
    region_iterator_pair() { }

    //! Regular constructor that builds a region_iterator_pair out of 2
    //! region_iterators.
    region_iterator_pair(const region_iterator<const Container, Predicate>& a,
                         const region_iterator<const Container, Predicate>& b)
      : Base(a, b) { }

    //! Convert a mutable region iterator pair into a const region iterator
    //! pair.
    region_iterator_pair(const region_iterator_pair<Container, Predicate>& p)
      : Base(p.first, p.second) { }
  };

  /**
   *  Returns a \region_iterator_pair where \c first points to the first element
   *  matching the predicate \c pred in \c container, and \c second points after
   *  the last element matching \c pred in \c container.
   *
   *  \tparam Ct The type of \c container.
   *  \tparam Predicate The type of \c pred, which must be a model of
   *  \region_predicate.
   *  \param container The container being searched.
   *  \param pred The predicate used for the search.
   */
  ///@{
  template <typename Container, typename Predicate>
  inline region_iterator_pair<Container, Predicate>
  region_range(Container& container, const Predicate& pred)
  {
    return region_iterator_pair<Container, Predicate>
      (region_begin(container, pred), region_end(container, pred));
  }

  //! This overload works only on constant containers and will return a set of
  //! constant iterators, where the value dereferrenced by the iterator is
  //! constant.
  template <typename Container, typename Predicate>
  inline region_iterator_pair<const Container, Predicate>
  region_crange(const Container& container, const Predicate& pred)
  {
    return region_iterator_pair<const Container, Predicate>
      (region_begin(container, pred), region_end(container, pred));
  }
  ///@}

  template <typename Container>
  inline region_iterator_pair<Container>
  region_range(Container& container,
                 const typename Container::key_type& lower,
                 const typename Container::key_type& upper)
  { return region_range(container, make_bounds(container, lower, upper)); }

  template <typename Container>
  inline region_iterator_pair<const Container>
  region_crange(const Container& container,
                  const typename Container::key_type& lower,
                  const typename Container::key_type& upper)
  { return region_crange(container, make_bounds(container, lower, upper)); }

  namespace details
  {
    /**
     *  In the children of the node pointed to by \c node, find the first
     *  matching node in the region delimited by \c Predicate, with pre-order
     *  transversal.  If no match can be found, returns a node pointing to \c
     *  node's parent.
     *
     *  \param node  The node currently pointed to.
     *  \param depth The depth at which the node is located in the tree.
     *  \param rank  The rank for the container.
     *  \param pred  The predicate used to find matching nodes.
     *  \tparam Predicate  The type of predicate for the orthogonal query.
     */
    template <typename NodePtr, typename Rank, typename Predicate>
    inline std::pair<NodePtr, dimension_type>
    first_region(NodePtr node, dimension_type depth, const Rank rank,
                 const Predicate& pred)
    {
      NodePtr end = node->parent;
      dimension_type end_depth = depth - 1;
      SPATIAL_ASSERT_CHECK(!header(node));
      SPATIAL_ASSERT_CHECK(node != 0);
      for (;;)
        {
          dimension_type dim = depth % rank();
          relative_order rel = pred(dim, rank(), const_key(node));
          if (rel == matching)
            {
              dimension_type test = 0;
              for (; test < dim
                     && pred(test, rank(), const_key(node)) == matching;
                   ++test);
              if (test == dim)
                {
                  test = dim + 1;
                  for (; test < rank()
                         && pred(test, rank(), const_key(node)) == matching;
                       ++test);
                  if (test == rank())
                    { return std::make_pair(node, depth); }
                }
            }
          if (rel != above && node->right != 0)
            {
              if (rel != below && node->left != 0)
                {
                  NodePtr other;
                  dimension_type other_depth;
                  import::tie(other, other_depth)
                    = first_region(node->left, depth + 1, rank, pred);
                  if (other != node)
                    { return std::make_pair(other, other_depth); }
                }
              node = node->right; ++depth;
            }
          else if (rel != below && node->left != 0)
            { node = node->left; ++depth; }
          else { return std::make_pair(end, end_depth); }
        }
    }

    /**
     *  In the children of the node pointed to by \c node, find the last
     *  matching node in the region delimited by \c Predicate, with pre-order
     *  transversal.  If no match can be found, returns a node pointing to \c
     *  node's parent
     *
     *  \param node  The node currently pointed to.
     *  \param depth The depth at which the node is located in the tree.
     *  \param rank  The rank for the container.
     *  \param pred  The predicate used to find matching nodes.
     *  \tparam Predicate  The type of predicate for the orthogonal query.
     */
    template <typename NodePtr, typename Rank, typename Predicate>
    inline std::pair<NodePtr, dimension_type>
    last_region(NodePtr node, dimension_type depth, const Rank rank,
                const Predicate& pred)
    {
      SPATIAL_ASSERT_CHECK(!header(node));
      SPATIAL_ASSERT_CHECK(node != 0);
      for (;;)
        {
          relative_order rel = pred(depth % rank(), rank(), const_key(node));
          if (rel != above && node->right != 0)
            { node = node->right; ++depth; }
          else if (rel != below && node->left != 0)
            { node = node->left; ++depth; }
          else break;
        }
      for (;;)
        {
          dimension_type test = 0;
          for(; test < rank()
                && pred(test, rank(), const_key(node)) == matching; ++test);
          if (test == rank())
            { return std::make_pair(node, depth); }
          NodePtr prev_node = node;
          node = node->parent; --depth;
          if (header(node))
            { return std::make_pair(node, depth); }
          if (node->right == prev_node
              && pred(depth % rank(), rank(), const_key(node)) != below
              && node->left != 0)
            {
              node = node->left; ++depth;
              for (;;)
                {
                  relative_order rel = pred(depth % rank(), rank(), const_key(node));
                  if (rel != above && node->right != 0)
                    { node = node->right; ++depth; }
                  else if (rel != below && node->left != 0)
                    { node = node->left; ++depth; }
                  else break;
                }
            }
        }
    }

    /**
     *  Returns the next matching node in the region delimited by \c Predicate,
     *  with pre-order transversal. If node is past-the-end, then the behavior
     *  is unspecified.
     *
     *  \param node  The node currently pointed to.
     *  \param depth The depth at which the node is located in the tree.
     *  \param rank  The rank for the container.
     *  \param pred  The predicate used to find matching nodes.
     *  \tparam Predicate  The type of predicate for the orthogonal query.
     */
    template <typename NodePtr, typename Rank, typename Predicate>
    inline std::pair<NodePtr, dimension_type>
    increment_region(NodePtr node, dimension_type depth, const Rank rank,
                     const Predicate& pred)
    {
      SPATIAL_ASSERT_CHECK(!header(node));
      SPATIAL_ASSERT_CHECK(node != 0);
      for (;;)
        {
          relative_order rel = pred(depth % rank(), rank(), const_key(node));
          if (rel != below && node->left != 0)
            { node = node->left; ++depth; }
          else if (rel != above && node->right != 0)
            { node = node->right; ++depth; }
          else
            {
              NodePtr prev_node = node;
              node = node->parent; --depth;
              while (!header(node)
                     && (prev_node == node->right
                         || pred(depth % rank(), rank(),
                                 const_key(node)) == above
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
          for(; test < rank()
                && pred(test, rank(), const_key(node)) == matching; ++test);
          if (test == rank())
            { return std::make_pair(node, depth); }
        }
    }

    /**
     *  Returns the previous matching node in the region delimited by \c
     *  Predicate, with pre-order transversal. If node is at the start of the
     *  range, then the behavior is unspecified.
     *
     *  \param node  The node currently pointed to.
     *  \param depth The depth at which the node is located in the tree.
     *  \param rank  The rank for the container.
     *  \param pred  The predicate used to find matching nodes.
     *  \tparam Predicate  The type of predicate for the orthogonal query.
     */
    template <typename NodePtr, typename Rank, typename Predicate>
    inline std::pair<NodePtr, dimension_type>
    decrement_region(NodePtr node, dimension_type depth, const Rank rank,
                     const Predicate& pred)
    {
      if (header(node))
        { return last_region(node->parent, 0, rank, pred); }
      SPATIAL_ASSERT_CHECK(node != 0);
      NodePtr prev_node = node;
      node = node->parent; --depth;
      while (!header(node))
        {
          if (node->right == prev_node
              && pred(depth % rank(), rank(), const_key(node)) != below
              && node->left != 0)
            {
              node = node->left; ++depth;
              for (;;)
                {
                  relative_order rel = pred(depth % rank(), rank(), const_key(node));
                  if (rel != above && node->right != 0)
                    { node = node->right; ++depth; }
                  else if (rel != below && node->left != 0)
                    { node = node->left; ++depth; }
                  else break;
                }
            }
          dimension_type test = 0;
          for(; test < rank()
                && pred(test, rank(), const_key(node)) == matching; ++test);
          if (test == rank()) break;
          prev_node = node;
          node = node->parent; --depth;
        }
      return std::make_pair(node, depth);
    }

  } // namespace details
} // namespace spatial

#endif // SPATIAL_REGION_HPP
