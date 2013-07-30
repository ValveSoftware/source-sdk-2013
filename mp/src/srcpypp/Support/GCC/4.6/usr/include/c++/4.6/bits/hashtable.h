// hashtable.h header -*- C++ -*-

// Copyright (C) 2007, 2008, 2009, 2010, 2011 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// Under Section 7 of GPL version 3, you are granted additional
// permissions described in the GCC Runtime Library Exception, version
// 3.1, as published by the Free Software Foundation.

// You should have received a copy of the GNU General Public License and
// a copy of the GCC Runtime Library Exception along with this program;
// see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see
// <http://www.gnu.org/licenses/>.

/** @file bits/hashtable.h
 *  This is an internal header file, included by other library headers.
 *  Do not attempt to use it directly. @headername{unordered_map, unordered_set}
 */

#ifndef _HASHTABLE_H
#define _HASHTABLE_H 1

#pragma GCC system_header

#include <bits/hashtable_policy.h>

namespace std _GLIBCXX_VISIBILITY(default)
{
_GLIBCXX_BEGIN_NAMESPACE_VERSION

  // Class template _Hashtable, class definition.

  // Meaning of class template _Hashtable's template parameters

  // _Key and _Value: arbitrary CopyConstructible types.

  // _Allocator: an allocator type ([lib.allocator.requirements]) whose
  // value type is Value.  As a conforming extension, we allow for
  // value type != Value.

  // _ExtractKey: function object that takes a object of type Value
  // and returns a value of type _Key.

  // _Equal: function object that takes two objects of type k and returns
  // a bool-like value that is true if the two objects are considered equal.

  // _H1: the hash function.  A unary function object with argument type
  // Key and result type size_t.  Return values should be distributed
  // over the entire range [0, numeric_limits<size_t>:::max()].

  // _H2: the range-hashing function (in the terminology of Tavori and
  // Dreizin).  A binary function object whose argument types and result
  // type are all size_t.  Given arguments r and N, the return value is
  // in the range [0, N).

  // _Hash: the ranged hash function (Tavori and Dreizin). A binary function
  // whose argument types are _Key and size_t and whose result type is
  // size_t.  Given arguments k and N, the return value is in the range
  // [0, N).  Default: hash(k, N) = h2(h1(k), N).  If _Hash is anything other
  // than the default, _H1 and _H2 are ignored.

  // _RehashPolicy: Policy class with three members, all of which govern
  // the bucket count. _M_next_bkt(n) returns a bucket count no smaller
  // than n.  _M_bkt_for_elements(n) returns a bucket count appropriate
  // for an element count of n.  _M_need_rehash(n_bkt, n_elt, n_ins)
  // determines whether, if the current bucket count is n_bkt and the
  // current element count is n_elt, we need to increase the bucket
  // count.  If so, returns make_pair(true, n), where n is the new
  // bucket count.  If not, returns make_pair(false, <anything>).

  // ??? Right now it is hard-wired that the number of buckets never
  // shrinks.  Should we allow _RehashPolicy to change that?

  // __cache_hash_code: bool.  true if we store the value of the hash
  // function along with the value.  This is a time-space tradeoff.
  // Storing it may improve lookup speed by reducing the number of times
  // we need to call the Equal function.

  // __constant_iterators: bool.  true if iterator and const_iterator are
  // both constant iterator types.  This is true for unordered_set and
  // unordered_multiset, false for unordered_map and unordered_multimap.

  // __unique_keys: bool.  true if the return value of _Hashtable::count(k)
  // is always at most one, false if it may be an arbitrary number.  This
  // true for unordered_set and unordered_map, false for unordered_multiset
  // and unordered_multimap.

  template<typename _Key, typename _Value, typename _Allocator,
	   typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash,
	   typename _RehashPolicy,
	   bool __cache_hash_code,
	   bool __constant_iterators,
	   bool __unique_keys>
    class _Hashtable
    : public __detail::_Rehash_base<_RehashPolicy,
				    _Hashtable<_Key, _Value, _Allocator,
					       _ExtractKey,
					       _Equal, _H1, _H2, _Hash,
					       _RehashPolicy,
					       __cache_hash_code,
					       __constant_iterators,
					       __unique_keys> >,
      public __detail::_Hash_code_base<_Key, _Value, _ExtractKey, _Equal,
				       _H1, _H2, _Hash, __cache_hash_code>,
      public __detail::_Map_base<_Key, _Value, _ExtractKey, __unique_keys,
				 _Hashtable<_Key, _Value, _Allocator,
					    _ExtractKey,
					    _Equal, _H1, _H2, _Hash,
					    _RehashPolicy,
					    __cache_hash_code,
					    __constant_iterators,
					    __unique_keys> >,
      public __detail::_Equality_base<_ExtractKey, __unique_keys,
				      _Hashtable<_Key, _Value, _Allocator,
						 _ExtractKey,
						 _Equal, _H1, _H2, _Hash,
						 _RehashPolicy,
						 __cache_hash_code,
						 __constant_iterators,
						 __unique_keys> >
    {
    public:
      typedef _Allocator                                  allocator_type;
      typedef _Value                                      value_type;
      typedef _Key                                        key_type;
      typedef _Equal                                      key_equal;
      // mapped_type, if present, comes from _Map_base.
      // hasher, if present, comes from _Hash_code_base.
      typedef typename _Allocator::pointer                pointer;
      typedef typename _Allocator::const_pointer          const_pointer;
      typedef typename _Allocator::reference              reference;
      typedef typename _Allocator::const_reference        const_reference;

      typedef std::size_t                                 size_type;
      typedef std::ptrdiff_t                              difference_type;
      typedef __detail::_Node_iterator<value_type, __constant_iterators,
				       __cache_hash_code>
							  local_iterator;
      typedef __detail::_Node_const_iterator<value_type,
					     __constant_iterators,
					     __cache_hash_code>
							  const_local_iterator;

      typedef __detail::_Hashtable_iterator<value_type, __constant_iterators,
					    __cache_hash_code>
							  iterator;
      typedef __detail::_Hashtable_const_iterator<value_type,
						  __constant_iterators,
						  __cache_hash_code>
							  const_iterator;

      template<typename _Key2, typename _Value2, typename _Ex2, bool __unique2,
	       typename _Hashtable2>
	friend struct __detail::_Map_base;

    private:
      typedef __detail::_Hash_node<_Value, __cache_hash_code> _Node;
      typedef typename _Allocator::template rebind<_Node>::other
							_Node_allocator_type;
      typedef typename _Allocator::template rebind<_Node*>::other
							_Bucket_allocator_type;

      typedef typename _Allocator::template rebind<_Value>::other
							_Value_allocator_type;

      _Node_allocator_type   _M_node_allocator;
      _Node**                _M_buckets;
      size_type              _M_bucket_count;
      size_type              _M_begin_bucket_index; // First non-empty bucket.
      size_type              _M_element_count;
      _RehashPolicy          _M_rehash_policy;

      template<typename... _Args>
	_Node*
	_M_allocate_node(_Args&&... __args);

      void
      _M_deallocate_node(_Node* __n);

      void
      _M_deallocate_nodes(_Node**, size_type);

      _Node**
      _M_allocate_buckets(size_type __n);

      void
      _M_deallocate_buckets(_Node**, size_type __n);

    public:
      // Constructor, destructor, assignment, swap
      _Hashtable(size_type __bucket_hint,
		 const _H1&, const _H2&, const _Hash&,
		 const _Equal&, const _ExtractKey&,
		 const allocator_type&);

      template<typename _InputIterator>
	_Hashtable(_InputIterator __first, _InputIterator __last,
		   size_type __bucket_hint,
		   const _H1&, const _H2&, const _Hash&,
		   const _Equal&, const _ExtractKey&,
		   const allocator_type&);

      _Hashtable(const _Hashtable&);

      _Hashtable(_Hashtable&&);

      _Hashtable&
      operator=(const _Hashtable& __ht)
      {
	_Hashtable __tmp(__ht);
	this->swap(__tmp);
	return *this;
      }

      _Hashtable&
      operator=(_Hashtable&& __ht)
      {
	// NB: DR 1204.
	// NB: DR 675.
	this->clear();
	this->swap(__ht);
	return *this;
      }

      ~_Hashtable();

      void swap(_Hashtable&);

      // Basic container operations
      iterator
      begin()
      { return iterator(_M_buckets + _M_begin_bucket_index); }

      const_iterator
      begin() const
      { return const_iterator(_M_buckets + _M_begin_bucket_index); }

      iterator
      end()
      { return iterator(_M_buckets + _M_bucket_count); }

      const_iterator
      end() const
      { return const_iterator(_M_buckets + _M_bucket_count); }

      const_iterator
      cbegin() const
      { return const_iterator(_M_buckets + _M_begin_bucket_index); }

      const_iterator
      cend() const
      { return const_iterator(_M_buckets + _M_bucket_count); }

      size_type
      size() const
      { return _M_element_count; }

      bool
      empty() const
      { return size() == 0; }

      allocator_type
      get_allocator() const
      { return allocator_type(_M_node_allocator); }

      size_type
      max_size() const
      { return _M_node_allocator.max_size(); }

      // Observers
      key_equal
      key_eq() const
      { return this->_M_eq; }

      // hash_function, if present, comes from _Hash_code_base.

      // Bucket operations
      size_type
      bucket_count() const
      { return _M_bucket_count; }

      size_type
      max_bucket_count() const
      { return max_size(); }

      size_type
      bucket_size(size_type __n) const
      { return std::distance(begin(__n), end(__n)); }

      size_type
      bucket(const key_type& __k) const
      {
	return this->_M_bucket_index(__k, this->_M_hash_code(__k),
				     bucket_count());
      }

      local_iterator
      begin(size_type __n)
      { return local_iterator(_M_buckets[__n]); }

      local_iterator
      end(size_type)
      { return local_iterator(0); }

      const_local_iterator
      begin(size_type __n) const
      { return const_local_iterator(_M_buckets[__n]); }

      const_local_iterator
      end(size_type) const
      { return const_local_iterator(0); }

      // DR 691.
      const_local_iterator
      cbegin(size_type __n) const
      { return const_local_iterator(_M_buckets[__n]); }

      const_local_iterator
      cend(size_type) const
      { return const_local_iterator(0); }

      float
      load_factor() const
      {
	return static_cast<float>(size()) / static_cast<float>(bucket_count());
      }

      // max_load_factor, if present, comes from _Rehash_base.

      // Generalization of max_load_factor.  Extension, not found in TR1.  Only
      // useful if _RehashPolicy is something other than the default.
      const _RehashPolicy&
      __rehash_policy() const
      { return _M_rehash_policy; }

      void
      __rehash_policy(const _RehashPolicy&);

      // Lookup.
      iterator
      find(const key_type& __k);

      const_iterator
      find(const key_type& __k) const;

      size_type
      count(const key_type& __k) const;

      std::pair<iterator, iterator>
      equal_range(const key_type& __k);

      std::pair<const_iterator, const_iterator>
      equal_range(const key_type& __k) const;

    private:
      // Find and insert helper functions and types
      _Node*
      _M_find_node(_Node*, const key_type&,
		   typename _Hashtable::_Hash_code_type) const;

      template<typename _Arg>
	iterator
	_M_insert_bucket(_Arg&&, size_type,
			 typename _Hashtable::_Hash_code_type);

      template<typename _Arg>
	std::pair<iterator, bool>
	_M_insert(_Arg&&, std::true_type);

      template<typename _Arg>
	iterator
	_M_insert(_Arg&&, std::false_type);

      typedef typename std::conditional<__unique_keys,
					std::pair<iterator, bool>,
					iterator>::type
	_Insert_Return_Type;

      typedef typename std::conditional<__unique_keys,
					std::_Select1st<_Insert_Return_Type>,
					std::_Identity<_Insert_Return_Type>
				   >::type
	_Insert_Conv_Type;

    public:
      // Insert and erase
      _Insert_Return_Type
      insert(const value_type& __v)
      { return _M_insert(__v, std::integral_constant<bool, __unique_keys>()); }

      iterator
      insert(const_iterator, const value_type& __v)
      { return _Insert_Conv_Type()(insert(__v)); }

      _Insert_Return_Type
      insert(value_type&& __v)
      { return _M_insert(std::move(__v),
			 std::integral_constant<bool, __unique_keys>()); }

      iterator
      insert(const_iterator, value_type&& __v)
      { return _Insert_Conv_Type()(insert(std::move(__v))); }

      template<typename _Pair, typename = typename
	       std::enable_if<!__constant_iterators
			      && std::is_convertible<_Pair,
						     value_type>::value>::type>
	_Insert_Return_Type
	insert(_Pair&& __v)
	{ return _M_insert(std::forward<_Pair>(__v),
			   std::integral_constant<bool, __unique_keys>()); }

      template<typename _Pair, typename = typename
	       std::enable_if<!__constant_iterators
			      && std::is_convertible<_Pair,
						     value_type>::value>::type>
	iterator
	insert(const_iterator, _Pair&& __v)
	{ return _Insert_Conv_Type()(insert(std::forward<_Pair>(__v))); }

      template<typename _InputIterator>
	void
	insert(_InputIterator __first, _InputIterator __last);

      void
      insert(initializer_list<value_type> __l)
      { this->insert(__l.begin(), __l.end()); }

      iterator
      erase(const_iterator);

      // LWG 2059.
      iterator
      erase(iterator __it)
      { return erase(const_iterator(__it)); }

      size_type
      erase(const key_type&);

      iterator
      erase(const_iterator, const_iterator);

      void
      clear();

      // Set number of buckets to be appropriate for container of n element.
      void rehash(size_type __n);

      // DR 1189.
      // reserve, if present, comes from _Rehash_base.

    private:
      // Unconditionally change size of bucket array to n.
      void _M_rehash(size_type __n);
    };


  // Definitions of class template _Hashtable's out-of-line member functions.
  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    template<typename... _Args>
      typename _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
			  _H1, _H2, _Hash, _RehashPolicy,
			  __chc, __cit, __uk>::_Node*
      _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
		 _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
      _M_allocate_node(_Args&&... __args)
      {
	_Node* __n = _M_node_allocator.allocate(1);
	__try
	  {
	    _M_node_allocator.construct(__n, std::forward<_Args>(__args)...);
	    __n->_M_next = 0;
	    return __n;
	  }
	__catch(...)
	  {
	    _M_node_allocator.deallocate(__n, 1);
	    __throw_exception_again;
	  }
      }

  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    void
    _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
    _M_deallocate_node(_Node* __n)
    {
      _M_node_allocator.destroy(__n);
      _M_node_allocator.deallocate(__n, 1);
    }

  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    void
    _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
    _M_deallocate_nodes(_Node** __array, size_type __n)
    {
      for (size_type __i = 0; __i < __n; ++__i)
	{
	  _Node* __p = __array[__i];
	  while (__p)
	    {
	      _Node* __tmp = __p;
	      __p = __p->_M_next;
	      _M_deallocate_node(__tmp);
	    }
	  __array[__i] = 0;
	}
    }

  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    typename _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
			_H1, _H2, _Hash, _RehashPolicy,
			__chc, __cit, __uk>::_Node**
    _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
    _M_allocate_buckets(size_type __n)
    {
      _Bucket_allocator_type __alloc(_M_node_allocator);

      // We allocate one extra bucket to hold a sentinel, an arbitrary
      // non-null pointer.  Iterator increment relies on this.
      _Node** __p = __alloc.allocate(__n + 1);
      std::fill(__p, __p + __n, (_Node*) 0);
      __p[__n] = reinterpret_cast<_Node*>(0x1000);
      return __p;
    }

  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    void
    _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
    _M_deallocate_buckets(_Node** __p, size_type __n)
    {
      _Bucket_allocator_type __alloc(_M_node_allocator);
      __alloc.deallocate(__p, __n + 1);
    }

  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
    _Hashtable(size_type __bucket_hint,
	       const _H1& __h1, const _H2& __h2, const _Hash& __h,
	       const _Equal& __eq, const _ExtractKey& __exk,
	       const allocator_type& __a)
    : __detail::_Rehash_base<_RehashPolicy, _Hashtable>(),
      __detail::_Hash_code_base<_Key, _Value, _ExtractKey, _Equal,
				_H1, _H2, _Hash, __chc>(__exk, __eq,
							__h1, __h2, __h),
      __detail::_Map_base<_Key, _Value, _ExtractKey, __uk, _Hashtable>(),
      _M_node_allocator(__a),
      _M_bucket_count(0),
      _M_element_count(0),
      _M_rehash_policy()
    {
      _M_bucket_count = _M_rehash_policy._M_next_bkt(__bucket_hint);
      _M_buckets = _M_allocate_buckets(_M_bucket_count);
      _M_begin_bucket_index = _M_bucket_count;
    }

  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    template<typename _InputIterator>
      _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
		 _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
      _Hashtable(_InputIterator __f, _InputIterator __l,
		 size_type __bucket_hint,
		 const _H1& __h1, const _H2& __h2, const _Hash& __h,
		 const _Equal& __eq, const _ExtractKey& __exk,
		 const allocator_type& __a)
      : __detail::_Rehash_base<_RehashPolicy, _Hashtable>(),
	__detail::_Hash_code_base<_Key, _Value, _ExtractKey, _Equal,
				  _H1, _H2, _Hash, __chc>(__exk, __eq,
							  __h1, __h2, __h),
	__detail::_Map_base<_Key, _Value, _ExtractKey, __uk, _Hashtable>(),
	_M_node_allocator(__a),
	_M_bucket_count(0),
	_M_element_count(0),
	_M_rehash_policy()
      {
	_M_bucket_count = std::max(_M_rehash_policy._M_next_bkt(__bucket_hint),
				   _M_rehash_policy.
				   _M_bkt_for_elements(__detail::
						       __distance_fw(__f,
								     __l)));
	_M_buckets = _M_allocate_buckets(_M_bucket_count);
	_M_begin_bucket_index = _M_bucket_count;
	__try
	  {
	    for (; __f != __l; ++__f)
	      this->insert(*__f);
	  }
	__catch(...)
	  {
	    clear();
	    _M_deallocate_buckets(_M_buckets, _M_bucket_count);
	    __throw_exception_again;
	  }
      }

  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
    _Hashtable(const _Hashtable& __ht)
    : __detail::_Rehash_base<_RehashPolicy, _Hashtable>(__ht),
      __detail::_Hash_code_base<_Key, _Value, _ExtractKey, _Equal,
				_H1, _H2, _Hash, __chc>(__ht),
      __detail::_Map_base<_Key, _Value, _ExtractKey, __uk, _Hashtable>(__ht),
      _M_node_allocator(__ht._M_node_allocator),
      _M_bucket_count(__ht._M_bucket_count),
      _M_begin_bucket_index(__ht._M_begin_bucket_index),
      _M_element_count(__ht._M_element_count),
      _M_rehash_policy(__ht._M_rehash_policy)
    {
      _M_buckets = _M_allocate_buckets(_M_bucket_count);
      __try
	{
	  for (size_type __i = 0; __i < __ht._M_bucket_count; ++__i)
	    {
	      _Node* __n = __ht._M_buckets[__i];
	      _Node** __tail = _M_buckets + __i;
	      while (__n)
		{
		  *__tail = _M_allocate_node(__n->_M_v);
		  this->_M_copy_code(*__tail, __n);
		  __tail = &((*__tail)->_M_next);
		  __n = __n->_M_next;
		}
	    }
	}
      __catch(...)
	{
	  clear();
	  _M_deallocate_buckets(_M_buckets, _M_bucket_count);
	  __throw_exception_again;
	}
    }

  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
    _Hashtable(_Hashtable&& __ht)
    : __detail::_Rehash_base<_RehashPolicy, _Hashtable>(__ht),
      __detail::_Hash_code_base<_Key, _Value, _ExtractKey, _Equal,
				_H1, _H2, _Hash, __chc>(__ht),
      __detail::_Map_base<_Key, _Value, _ExtractKey, __uk, _Hashtable>(__ht),
      _M_node_allocator(__ht._M_node_allocator),
      _M_buckets(__ht._M_buckets),
      _M_bucket_count(__ht._M_bucket_count),
      _M_begin_bucket_index(__ht._M_begin_bucket_index),
      _M_element_count(__ht._M_element_count),
      _M_rehash_policy(__ht._M_rehash_policy)
    {
      __ht._M_rehash_policy = _RehashPolicy();
      __ht._M_bucket_count = __ht._M_rehash_policy._M_next_bkt(0);
      __ht._M_buckets = __ht._M_allocate_buckets(__ht._M_bucket_count);
      __ht._M_begin_bucket_index = __ht._M_bucket_count;
      __ht._M_element_count = 0;
    }

  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
    ~_Hashtable()
    {
      clear();
      _M_deallocate_buckets(_M_buckets, _M_bucket_count);
    }

  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    void
    _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
    swap(_Hashtable& __x)
    {
      // The only base class with member variables is hash_code_base.  We
      // define _Hash_code_base::_M_swap because different specializations
      // have different members.
      __detail::_Hash_code_base<_Key, _Value, _ExtractKey, _Equal,
	_H1, _H2, _Hash, __chc>::_M_swap(__x);

      // _GLIBCXX_RESOLVE_LIB_DEFECTS
      // 431. Swapping containers with unequal allocators.
      std::__alloc_swap<_Node_allocator_type>::_S_do_it(_M_node_allocator,
							__x._M_node_allocator);

      std::swap(_M_rehash_policy, __x._M_rehash_policy);
      std::swap(_M_buckets, __x._M_buckets);
      std::swap(_M_bucket_count, __x._M_bucket_count);
      std::swap(_M_begin_bucket_index, __x._M_begin_bucket_index);
      std::swap(_M_element_count, __x._M_element_count);
    }

  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    void
    _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
    __rehash_policy(const _RehashPolicy& __pol)
    {
      _M_rehash_policy = __pol;
      size_type __n_bkt = __pol._M_bkt_for_elements(_M_element_count);
      if (__n_bkt > _M_bucket_count)
	_M_rehash(__n_bkt);
    }

  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    typename _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
			_H1, _H2, _Hash, _RehashPolicy,
			__chc, __cit, __uk>::iterator
    _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
    find(const key_type& __k)
    {
      typename _Hashtable::_Hash_code_type __code = this->_M_hash_code(__k);
      std::size_t __n = this->_M_bucket_index(__k, __code, _M_bucket_count);
      _Node* __p = _M_find_node(_M_buckets[__n], __k, __code);
      return __p ? iterator(__p, _M_buckets + __n) : this->end();
    }

  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    typename _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
			_H1, _H2, _Hash, _RehashPolicy,
			__chc, __cit, __uk>::const_iterator
    _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
    find(const key_type& __k) const
    {
      typename _Hashtable::_Hash_code_type __code = this->_M_hash_code(__k);
      std::size_t __n = this->_M_bucket_index(__k, __code, _M_bucket_count);
      _Node* __p = _M_find_node(_M_buckets[__n], __k, __code);
      return __p ? const_iterator(__p, _M_buckets + __n) : this->end();
    }

  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    typename _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
			_H1, _H2, _Hash, _RehashPolicy,
			__chc, __cit, __uk>::size_type
    _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
    count(const key_type& __k) const
    {
      typename _Hashtable::_Hash_code_type __code = this->_M_hash_code(__k);
      std::size_t __n = this->_M_bucket_index(__k, __code, _M_bucket_count);
      std::size_t __result = 0;
      for (_Node* __p = _M_buckets[__n]; __p; __p = __p->_M_next)
	if (this->_M_compare(__k, __code, __p))
	  ++__result;
      return __result;
    }

  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    std::pair<typename _Hashtable<_Key, _Value, _Allocator,
				  _ExtractKey, _Equal, _H1,
				  _H2, _Hash, _RehashPolicy,
				  __chc, __cit, __uk>::iterator,
	      typename _Hashtable<_Key, _Value, _Allocator,
				  _ExtractKey, _Equal, _H1,
				  _H2, _Hash, _RehashPolicy,
				  __chc, __cit, __uk>::iterator>
    _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
    equal_range(const key_type& __k)
    {
      typename _Hashtable::_Hash_code_type __code = this->_M_hash_code(__k);
      std::size_t __n = this->_M_bucket_index(__k, __code, _M_bucket_count);
      _Node** __head = _M_buckets + __n;
      _Node* __p = _M_find_node(*__head, __k, __code);

      if (__p)
	{
	  _Node* __p1 = __p->_M_next;
	  for (; __p1; __p1 = __p1->_M_next)
	    if (!this->_M_compare(__k, __code, __p1))
	      break;

	  iterator __first(__p, __head);
	  iterator __last(__p1, __head);
	  if (!__p1)
	    __last._M_incr_bucket();
	  return std::make_pair(__first, __last);
	}
      else
	return std::make_pair(this->end(), this->end());
    }

  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    std::pair<typename _Hashtable<_Key, _Value, _Allocator,
				  _ExtractKey, _Equal, _H1,
				  _H2, _Hash, _RehashPolicy,
				  __chc, __cit, __uk>::const_iterator,
	      typename _Hashtable<_Key, _Value, _Allocator,
				  _ExtractKey, _Equal, _H1,
				  _H2, _Hash, _RehashPolicy,
				  __chc, __cit, __uk>::const_iterator>
    _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
    equal_range(const key_type& __k) const
    {
      typename _Hashtable::_Hash_code_type __code = this->_M_hash_code(__k);
      std::size_t __n = this->_M_bucket_index(__k, __code, _M_bucket_count);
      _Node** __head = _M_buckets + __n;
      _Node* __p = _M_find_node(*__head, __k, __code);

      if (__p)
	{
	  _Node* __p1 = __p->_M_next;
	  for (; __p1; __p1 = __p1->_M_next)
	    if (!this->_M_compare(__k, __code, __p1))
	      break;

	  const_iterator __first(__p, __head);
	  const_iterator __last(__p1, __head);
	  if (!__p1)
	    __last._M_incr_bucket();
	  return std::make_pair(__first, __last);
	}
      else
	return std::make_pair(this->end(), this->end());
    }

  // Find the node whose key compares equal to k, beginning the search
  // at p (usually the head of a bucket).  Return nil if no node is found.
  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    typename _Hashtable<_Key, _Value, _Allocator, _ExtractKey,
			_Equal, _H1, _H2, _Hash, _RehashPolicy,
			__chc, __cit, __uk>::_Node*
    _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
    _M_find_node(_Node* __p, const key_type& __k,
		typename _Hashtable::_Hash_code_type __code) const
    {
      for (; __p; __p = __p->_M_next)
	if (this->_M_compare(__k, __code, __p))
	  return __p;
      return false;
    }

  // Insert v in bucket n (assumes no element with its key already present).
  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    template<typename _Arg>
      typename _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
			  _H1, _H2, _Hash, _RehashPolicy,
			  __chc, __cit, __uk>::iterator
      _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
		 _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
      _M_insert_bucket(_Arg&& __v, size_type __n,
		       typename _Hashtable::_Hash_code_type __code)
      {
	std::pair<bool, std::size_t> __do_rehash
	  = _M_rehash_policy._M_need_rehash(_M_bucket_count,
					    _M_element_count, 1);

	if (__do_rehash.first)
	  {
	    const key_type& __k = this->_M_extract(__v);
	    __n = this->_M_bucket_index(__k, __code, __do_rehash.second);
	  }

	// Allocate the new node before doing the rehash so that we don't
	// do a rehash if the allocation throws.
	_Node* __new_node = _M_allocate_node(std::forward<_Arg>(__v));

	__try
	  {
	    if (__do_rehash.first)
	      _M_rehash(__do_rehash.second);

	    __new_node->_M_next = _M_buckets[__n];
	    this->_M_store_code(__new_node, __code);
	    _M_buckets[__n] = __new_node;
	    ++_M_element_count;
	    if (__n < _M_begin_bucket_index)
	      _M_begin_bucket_index = __n;
	    return iterator(__new_node, _M_buckets + __n);
	  }
	__catch(...)
	  {
	    _M_deallocate_node(__new_node);
	    __throw_exception_again;
	  }
      }

  // Insert v if no element with its key is already present.
  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    template<typename _Arg>
      std::pair<typename _Hashtable<_Key, _Value, _Allocator,
				    _ExtractKey, _Equal, _H1,
				    _H2, _Hash, _RehashPolicy,
				    __chc, __cit, __uk>::iterator, bool>
      _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
		 _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
      _M_insert(_Arg&& __v, std::true_type)
      {
	const key_type& __k = this->_M_extract(__v);
	typename _Hashtable::_Hash_code_type __code = this->_M_hash_code(__k);
	size_type __n = this->_M_bucket_index(__k, __code, _M_bucket_count);

	if (_Node* __p = _M_find_node(_M_buckets[__n], __k, __code))
	  return std::make_pair(iterator(__p, _M_buckets + __n), false);
	return std::make_pair(_M_insert_bucket(std::forward<_Arg>(__v),
			      __n, __code), true);
      }

  // Insert v unconditionally.
  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    template<typename _Arg>
      typename _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
			  _H1, _H2, _Hash, _RehashPolicy,
			  __chc, __cit, __uk>::iterator
      _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
		 _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
      _M_insert(_Arg&& __v, std::false_type)
      {
	std::pair<bool, std::size_t> __do_rehash
	  = _M_rehash_policy._M_need_rehash(_M_bucket_count,
					    _M_element_count, 1);
	if (__do_rehash.first)
	  _M_rehash(__do_rehash.second);

	const key_type& __k = this->_M_extract(__v);
	typename _Hashtable::_Hash_code_type __code = this->_M_hash_code(__k);
	size_type __n = this->_M_bucket_index(__k, __code, _M_bucket_count);

	// First find the node, avoid leaking new_node if compare throws.
	_Node* __prev = _M_find_node(_M_buckets[__n], __k, __code);
	_Node* __new_node = _M_allocate_node(std::forward<_Arg>(__v));

	if (__prev)
	  {
	    __new_node->_M_next = __prev->_M_next;
	    __prev->_M_next = __new_node;
	  }
	else
	  {
	    __new_node->_M_next = _M_buckets[__n];
	    _M_buckets[__n] = __new_node;
	    if (__n < _M_begin_bucket_index)
	      _M_begin_bucket_index = __n;
	  }
	this->_M_store_code(__new_node, __code);

	++_M_element_count;
	return iterator(__new_node, _M_buckets + __n);
      }

  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    template<typename _InputIterator>
      void
      _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
		 _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
      insert(_InputIterator __first, _InputIterator __last)
      {
	size_type __n_elt = __detail::__distance_fw(__first, __last);
	std::pair<bool, std::size_t> __do_rehash
	  = _M_rehash_policy._M_need_rehash(_M_bucket_count,
					    _M_element_count, __n_elt);
	if (__do_rehash.first)
	  _M_rehash(__do_rehash.second);

	for (; __first != __last; ++__first)
	  this->insert(*__first);
      }

  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    typename _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
			_H1, _H2, _Hash, _RehashPolicy,
			__chc, __cit, __uk>::iterator
    _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
    erase(const_iterator __it)
    {
      iterator __result(__it._M_cur_node, __it._M_cur_bucket);
      ++__result;

      _Node* __cur = *__it._M_cur_bucket;
      if (__cur == __it._M_cur_node)
	{
	  *__it._M_cur_bucket = __cur->_M_next;

	  // If _M_begin_bucket_index no longer indexes the first non-empty
	  // bucket - its single node is being erased - update it.
	  if (!_M_buckets[_M_begin_bucket_index])
	    _M_begin_bucket_index = __result._M_cur_bucket - _M_buckets;
	}
      else
	{
	  _Node* __next = __cur->_M_next;
	  while (__next != __it._M_cur_node)
	    {
	      __cur = __next;
	      __next = __cur->_M_next;
	    }
	  __cur->_M_next = __next->_M_next;
	}

      _M_deallocate_node(__it._M_cur_node);
      --_M_element_count;

      return __result;
    }

  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    typename _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
			_H1, _H2, _Hash, _RehashPolicy,
			__chc, __cit, __uk>::size_type
    _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
    erase(const key_type& __k)
    {
      typename _Hashtable::_Hash_code_type __code = this->_M_hash_code(__k);
      std::size_t __n = this->_M_bucket_index(__k, __code, _M_bucket_count);
      size_type __result = 0;

      _Node** __slot = _M_buckets + __n;
      while (*__slot && !this->_M_compare(__k, __code, *__slot))
	__slot = &((*__slot)->_M_next);

      _Node** __saved_slot = 0;
      while (*__slot && this->_M_compare(__k, __code, *__slot))
	{
	  // _GLIBCXX_RESOLVE_LIB_DEFECTS
	  // 526. Is it undefined if a function in the standard changes
	  // in parameters?
	  if (std::__addressof(this->_M_extract((*__slot)->_M_v))
	      != std::__addressof(__k))
	    {
	      _Node* __p = *__slot;
	      *__slot = __p->_M_next;
	      _M_deallocate_node(__p);
	      --_M_element_count;
	      ++__result;
	    }
	  else
	    {
	      __saved_slot = __slot;
	      __slot = &((*__slot)->_M_next);
	    }
	}

      if (__saved_slot)
	{
	  _Node* __p = *__saved_slot;
	  *__saved_slot = __p->_M_next;
	  _M_deallocate_node(__p);
	  --_M_element_count;
	  ++__result;
	}

      // If the entire bucket indexed by _M_begin_bucket_index has been
      // erased look forward for the first non-empty bucket.
      if (!_M_buckets[_M_begin_bucket_index])
	{
	  if (!_M_element_count)
	    _M_begin_bucket_index = _M_bucket_count;
	  else
	    {
	      ++_M_begin_bucket_index;
	      while (!_M_buckets[_M_begin_bucket_index])
		++_M_begin_bucket_index;
	    }
	}

      return __result;
    }

  // ??? This could be optimized by taking advantage of the bucket
  // structure, but it's not clear that it's worth doing.  It probably
  // wouldn't even be an optimization unless the load factor is large.
  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    typename _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
			_H1, _H2, _Hash, _RehashPolicy,
			__chc, __cit, __uk>::iterator
    _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
    erase(const_iterator __first, const_iterator __last)
    {
       while (__first != __last)
	 __first = this->erase(__first);
      return iterator(__last._M_cur_node, __last._M_cur_bucket);
    }

  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    void
    _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
    clear()
    {
      _M_deallocate_nodes(_M_buckets, _M_bucket_count);
      _M_element_count = 0;
      _M_begin_bucket_index = _M_bucket_count;
    }

  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    void
    _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
    rehash(size_type __n)
    {
      _M_rehash(std::max(_M_rehash_policy._M_next_bkt(__n),
			 _M_rehash_policy._M_bkt_for_elements(_M_element_count
							      + 1)));
    }

  template<typename _Key, typename _Value,
	   typename _Allocator, typename _ExtractKey, typename _Equal,
	   typename _H1, typename _H2, typename _Hash, typename _RehashPolicy,
	   bool __chc, bool __cit, bool __uk>
    void
    _Hashtable<_Key, _Value, _Allocator, _ExtractKey, _Equal,
	       _H1, _H2, _Hash, _RehashPolicy, __chc, __cit, __uk>::
    _M_rehash(size_type __n)
    {
      _Node** __new_array = _M_allocate_buckets(__n);
      __try
	{
	  _M_begin_bucket_index = __n;
	  for (size_type __i = 0; __i < _M_bucket_count; ++__i)
	    while (_Node* __p = _M_buckets[__i])
	      {
		std::size_t __new_index = this->_M_bucket_index(__p, __n);
		_M_buckets[__i] = __p->_M_next;
		__p->_M_next = __new_array[__new_index];
		__new_array[__new_index] = __p;
		if (__new_index < _M_begin_bucket_index)
		  _M_begin_bucket_index = __new_index;
	      }
	  _M_deallocate_buckets(_M_buckets, _M_bucket_count);
	  _M_bucket_count = __n;
	  _M_buckets = __new_array;
	}
      __catch(...)
	{
	  // A failure here means that a hash function threw an exception.
	  // We can't restore the previous state without calling the hash
	  // function again, so the only sensible recovery is to delete
	  // everything.
	  _M_deallocate_nodes(__new_array, __n);
	  _M_deallocate_buckets(__new_array, __n);
	  _M_deallocate_nodes(_M_buckets, _M_bucket_count);
	  _M_element_count = 0;
	  _M_begin_bucket_index = _M_bucket_count;
	  __throw_exception_again;
	}
    }

_GLIBCXX_END_NAMESPACE_VERSION
} // namespace std

#endif // _HASHTABLE_H
