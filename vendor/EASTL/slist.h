///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// An slist is a singly-linked list. The C++ standard library doesn't define
// such a thing as an slist, nor does the C++ TR1. Our implementation of slist
// largely follows the design of the SGI STL slist container, which is also 
// found in STLPort. Singly-linked lists use less memory than doubly-linked 
// lists, but are less flexible. 
//
// In looking at slist, you will notice a lot of references to things like
// 'before first', 'before last', 'insert after', and 'erase after'. This is 
// due to the fact that std::list insert and erase works on the node before
// the referenced node, whereas slist is singly linked and operations are only
// efficient if they work on the node after the referenced node. This is because
// with an slist node you know the node after it but not the node before it.
//
///////////////////////////////////////////////////////////////////////////////



#ifndef EASTL_SLIST_H
#define EASTL_SLIST_H


#include "vendor/EASTL/internal/config.h"
#include "vendor/EASTL/allocator.h"
#include "vendor/EASTL/type_traits.h"
#include "vendor/EASTL/iterator.h"
#include "vendor/EASTL/algorithm.h"
#include "vendor/EASTL/initializer_list.h"
#include "vendor/EASTL/sort.h"
#include "vendor/EASTL/bonus/compressed_pair.h"
#include <stddef.h>

EA_DISABLE_ALL_VC_WARNINGS();

	#include <new>

EA_RESTORE_ALL_VC_WARNINGS();

EA_DISABLE_SN_WARNING(828); // The EDG SN compiler has a bug in its handling of variadic template arguments and mistakenly reports "parameter "args" was never referenced"


// 4530 - C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
// 4345 - Behavior change: an object of POD type constructed with an initializer of the form () will be default-initialized
// 4571 - catch(...) semantics changed since Visual C++ 7.1; structured exceptions (SEH) are no longer caught.
EA_DISABLE_VC_WARNING(4530 4345 4571);


#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



namespace eastl
{

	/// EASTL_SLIST_DEFAULT_NAME
	///
	/// Defines a default container name in the absence of a user-provided name.
	///
	#ifndef EASTL_SLIST_DEFAULT_NAME
		#define EASTL_SLIST_DEFAULT_NAME EASTL_DEFAULT_NAME_PREFIX " slist" // Unless the user overrides something, this is "EASTL slist".
	#endif


	/// EASTL_SLIST_DEFAULT_ALLOCATOR
	///
	#ifndef EASTL_SLIST_DEFAULT_ALLOCATOR
		#define EASTL_SLIST_DEFAULT_ALLOCATOR allocator_type(EASTL_SLIST_DEFAULT_NAME)
	#endif



	/// SListNodeBase
	///
	/// This is a standalone struct so that operations on it can be done without templates
	/// and so that an empty slist can have an SListNodeBase and thus not create any 
	/// instances of T.
	///
	struct SListNodeBase
	{
		SListNodeBase* mpNext;
	};


	template <typename T>
	struct SListNode : public SListNodeBase
	{
		T mValue;
	};

	/// SListIterator
	///
	template <typename T, typename Pointer, typename Reference>
	struct SListIterator
	{
		typedef SListIterator<T, Pointer, Reference>   this_type;
		typedef SListIterator<T, T*, T&>               iterator;
		typedef SListIterator<T, const T*, const T&>   const_iterator;
		typedef eastl_size_t                           size_type;     // See config.h for the definition of eastl_size_t, which defaults to size_t.
		typedef ptrdiff_t                              difference_type;
		typedef T                                      value_type;
		typedef SListNodeBase                          base_node_type;
		typedef SListNode<T>                           node_type;
		typedef Pointer                                pointer;
		typedef Reference                              reference;
		typedef EASTL_ITC_NS::forward_iterator_tag     iterator_category;

	public:
		base_node_type* mpNode;

	public:
		SListIterator();
		SListIterator(const SListNodeBase* pNode);
		
		template <typename This = this_type, enable_if_t<!is_same_v<This, iterator>, bool> = true>
		inline SListIterator(const iterator& x)
			: mpNode(x.mpNode)
		{
			// Empty
		}

		reference operator*() const;
		pointer   operator->() const;

		this_type& operator++();
		this_type  operator++(int);
	};



	/// SListBase
	///
	/// See VectorBase (class vector) for an explanation of why we 
	/// create this separate base class.
	///
	template <typename T, typename Allocator>
	struct SListBase
	{
	public:
		typedef Allocator                              allocator_type;
		typedef SListNode<T>                           node_type;
		typedef eastl_size_t                           size_type;     // See config.h for the definition of eastl_size_t, which defaults to size_t.
		typedef ptrdiff_t                              difference_type;
		typedef SListNodeBase                      base_node_type; // We use SListNodeBase instead of SListNode<T> because we don't want to create a T.

	protected:
		eastl::compressed_pair<base_node_type, allocator_type>  mNodeAllocator;
		#if EASTL_SLIST_SIZE_CACHE
			size_type  mSize;
		#endif

		base_node_type& internalNode() EA_NOEXCEPT { return mNodeAllocator.first(); }
		base_node_type const& internalNode() const EA_NOEXCEPT { return mNodeAllocator.first(); }
		allocator_type& internalAllocator() EA_NOEXCEPT { return mNodeAllocator.second(); }
		const allocator_type& internalAllocator() const EA_NOEXCEPT { return mNodeAllocator.second(); }

	public:
		const allocator_type& get_allocator() const EA_NOEXCEPT;
		allocator_type&       get_allocator() EA_NOEXCEPT;
		void                  set_allocator(const allocator_type& allocator);

	protected:
		SListBase();
		SListBase(const allocator_type& a);
	   ~SListBase();

		node_type* DoAllocateNode();
		void       DoFreeNode(node_type* pNode);

		SListNodeBase* DoEraseAfter(SListNodeBase* pNode);
		SListNodeBase* DoEraseAfter(SListNodeBase* pNode, SListNodeBase* pNodeLast);

	}; // class SListBase



	/// slist
	///
	/// This is the equivalent of C++11's forward_list.
	///
	/// -- size() is O(n) --
	/// Note that as of this writing, list::size() is an O(n) operation when EASTL_SLIST_SIZE_CACHE is disabled. 
	/// That is, getting the size of the list is not a fast operation, as it requires traversing the list and 
	/// counting the nodes. We could make list::size() be fast by having a member mSize variable. There are reasons 
	/// for having such functionality and reasons for not having such functionality. We currently choose
	/// to not have a member mSize variable as it would add four bytes to the class, add a tiny amount
	/// of processing to functions such as insert and erase, and would only serve to improve the size
	/// function, but no others. The alternative argument is that the C++ standard states that std::list
	/// should be an O(1) operation (i.e. have a member size variable), most C++ standard library list
	/// implementations do so, the size is but an integer which is quick to update, and many users 
	/// expect to have a fast size function. The EASTL_SLIST_SIZE_CACHE option changes this.
	/// To consider: Make size caching an optional template parameter.
	///
	/// Pool allocation
	/// If you want to make a custom memory pool for a list container, your pool 
	/// needs to contain items of type slist::node_type. So if you have a memory
	/// pool that has a constructor that takes the size of pool items and the
	/// count of pool items, you would do this (assuming that MemoryPool implements
	/// the Allocator interface):
	///     typedef slist<Widget, MemoryPool> WidgetList;          // Delare your WidgetList type.
	///     MemoryPool myPool(sizeof(WidgetList::node_type), 100); // Make a pool of 100 Widget nodes.
	///     WidgetList myList(&myPool);                            // Create a list that uses the pool.
	///
	template <typename T, typename Allocator = EASTLAllocatorType >
	class slist : public SListBase<T, Allocator>
	{
		typedef SListBase<T, Allocator>              base_type;
		typedef slist<T, Allocator>                  this_type;

	protected:
		using base_type::mNodeAllocator;
		using base_type::DoEraseAfter;
		using base_type::DoAllocateNode;
		using base_type::DoFreeNode;
#if EASTL_SLIST_SIZE_CACHE
		using base_type::mSize;
#endif
		using base_type::internalNode;
		using base_type::internalAllocator;

	public:
		typedef T                                    value_type;
		typedef value_type*                          pointer;
		typedef const value_type*                    const_pointer;
		typedef value_type&                          reference;
		typedef const value_type&                    const_reference;
		typedef SListIterator<T, T*, T&>             iterator;
		typedef SListIterator<T, const T*, const T&> const_iterator;
		typedef typename base_type::size_type        size_type;
		typedef typename base_type::difference_type  difference_type;
		typedef typename base_type::allocator_type   allocator_type;
		typedef typename base_type::node_type        node_type;
		typedef typename base_type::base_node_type   base_node_type;

	public:
		slist();
		slist(const allocator_type& allocator);
		explicit slist(size_type n, const allocator_type& allocator = EASTL_SLIST_DEFAULT_ALLOCATOR);
		slist(size_type n, const value_type& value, const allocator_type& allocator = EASTL_SLIST_DEFAULT_ALLOCATOR);
		slist(const this_type& x);
		slist(std::initializer_list<value_type> ilist, const allocator_type& allocator = EASTL_SLIST_DEFAULT_ALLOCATOR);
		slist(this_type&& x);
		slist(this_type&& x, const allocator_type& allocator);

		template <typename InputIterator>
		slist(InputIterator first, InputIterator last); // allocator arg removed because VC7.1 fails on the default arg. To do: Make a second version of this function without a default arg.

		this_type& operator=(const this_type& x);
		this_type& operator=(std::initializer_list<value_type>);
		this_type& operator=(this_type&& x);

		void swap(this_type& x);

		void assign(size_type n, const value_type& value);
		void assign(std::initializer_list<value_type> ilist);

		template <typename InputIterator>
		void assign(InputIterator first, InputIterator last);

		iterator       begin() EA_NOEXCEPT;
		const_iterator begin() const EA_NOEXCEPT;
		const_iterator cbegin() const EA_NOEXCEPT;

		iterator       end() EA_NOEXCEPT;
		const_iterator end() const EA_NOEXCEPT;
		const_iterator cend() const EA_NOEXCEPT;

		iterator       before_begin() EA_NOEXCEPT;
		const_iterator before_begin() const EA_NOEXCEPT;
		const_iterator cbefore_begin() const EA_NOEXCEPT;

		iterator        previous(const_iterator position);
		const_iterator  previous(const_iterator position) const;

		reference       front();
		const_reference front() const;

		template <class... Args>
		reference emplace_front(Args&&... args);

		void      push_front(const value_type& value);
		reference push_front();
		void      push_front(value_type&& value);

		void      pop_front();

		bool      empty() const EA_NOEXCEPT;
		size_type size() const EA_NOEXCEPT;

		void resize(size_type n, const value_type& value);
		void resize(size_type n);

		iterator insert(const_iterator position);
		iterator insert(const_iterator position, const value_type& value);
		void     insert(const_iterator position, size_type n, const value_type& value);

		template <typename InputIterator>
		void insert(const_iterator position, InputIterator first, InputIterator last);

		// Returns an iterator pointing to the last inserted element, or position if insertion count is zero.
		iterator insert_after(const_iterator position);
		iterator insert_after(const_iterator position, const value_type& value);
		iterator insert_after(const_iterator position, size_type n, const value_type& value);
		iterator insert_after(const_iterator position, std::initializer_list<value_type> ilist);
		iterator insert_after(const_iterator position, value_type&& value);

		template <class... Args>
		iterator emplace_after(const_iterator position, Args&&... args);

		template <typename InputIterator>
		iterator insert_after(const_iterator position, InputIterator first, InputIterator last);

		iterator erase(const_iterator position);
		iterator erase(const_iterator first, const_iterator last);

		iterator erase_after(const_iterator position);
		iterator erase_after(const_iterator before_first, const_iterator last);

		void clear() EA_NOEXCEPT;
		void reset_lose_memory() EA_NOEXCEPT;    // This is a unilateral reset to an initially empty state. No destructors are called, no deallocation occurs.

		size_type remove(const value_type& value);

		template <typename Predicate>
		size_type remove_if(Predicate predicate);

		void reverse() EA_NOEXCEPT;

		// splice splices to before position, like with the list container. However, in order to do so 
		// it must walk the list from beginning to position, which is an O(n) operation that can thus 
		// be slow. It's recommended that the splice_after functions be used whenever possible as they are O(1).
		void splice(const_iterator position, this_type& x);
		void splice(const_iterator position, this_type& x, const_iterator i);
		void splice(const_iterator position, this_type& x, const_iterator first, const_iterator last);
		void splice(const_iterator position, this_type&& x);
		void splice(const_iterator position, this_type&& x, const_iterator i);
		void splice(const_iterator position, this_type&& x, const_iterator first, const_iterator last);

		void splice_after(const_iterator position, this_type& x);
		void splice_after(const_iterator position, this_type& x, const_iterator i);
		void splice_after(const_iterator position, this_type& x, const_iterator first, const_iterator last);
		void splice_after(const_iterator position, this_type&& x);
		void splice_after(const_iterator position, this_type&& x, const_iterator i);
		void splice_after(const_iterator position, this_type&& x, const_iterator first, const_iterator last);

		// The following splice_after funcions are deprecated, as they don't allow for recognizing 
		// the allocator, cannot maintain the source mSize, and are not in the C++11 Standard definition 
		// of std::forward_list (which is the equivalent of this class).
		EASTL_REMOVE_AT_2024_APRIL void splice_after(const_iterator position, const_iterator before_first, const_iterator before_last);  // before_first and before_last come from a source container.
		EASTL_REMOVE_AT_2024_APRIL void splice_after(const_iterator position, const_iterator previous);                                  // previous comes from a source container.

		size_type unique();

		template <typename BinaryPredicate>
		size_type unique(BinaryPredicate);

		// Sorting functionality
		// This is independent of the global sort algorithms, as lists are 
		// linked nodes and can be sorted more efficiently by moving nodes
		// around in ways that global sort algorithms aren't privy to.
		void sort();

		template <class Compare>
		void sort(Compare compare);

		// Not yet implemented:
		// void merge(this_type& x);
		// void merge(this_type&& x);
		// template <class Compare>
		// void merge(this_type& x, Compare compare);
		// template <class Compare>
		// void merge(this_type&& x, Compare compare);
		// If these get implemented then make sure to override them in fixed_slist.

		bool validate() const;
		int  validate_iterator(const_iterator i) const;

	protected:
		node_type* DoCreateNode();

		template<typename... Args>
		node_type* DoCreateNode(Args&&... args);

		template <typename Integer>
		void DoAssign(Integer n, Integer value, true_type);

		template <typename InputIterator>
		void DoAssign(InputIterator first, InputIterator last, false_type);

		void DoAssignValues(size_type n, const value_type& value);

		template <typename InputIterator>
		node_type* DoInsertAfter(SListNodeBase* pNode, InputIterator first, InputIterator last);

		template <typename Integer>
		node_type* DoInsertAfter(SListNodeBase* pNode, Integer n, Integer value, true_type);

		template <typename InputIterator>
		node_type* DoInsertAfter(SListNodeBase* pNode, InputIterator first, InputIterator last, false_type);

		node_type* DoInsertValueAfter(SListNodeBase* pNode);
		node_type* DoInsertValuesAfter(SListNodeBase* pNode, size_type n, const value_type& value);

		template<typename... Args>
		node_type* DoInsertValueAfter(SListNodeBase* pNode, Args&&... args);

		void DoSwap(this_type& x);

	}; // class slist







	///////////////////////////////////////////////////////////////////////
	// SListNodeBase functions
	///////////////////////////////////////////////////////////////////////

	inline SListNodeBase* SListNodeInsertAfter(SListNodeBase* pPrevNode, SListNodeBase* pNode)
	{
		pNode->mpNext = pPrevNode->mpNext;
		pPrevNode->mpNext = pNode;
		return pNode;
	}

	inline SListNodeBase* SListNodeGetPrevious(SListNodeBase* pNodeBase, const SListNodeBase* pNode)
	{
		while(pNodeBase && (pNodeBase->mpNext != pNode))
			pNodeBase = pNodeBase->mpNext;
		return pNodeBase;
	}

	inline const SListNodeBase* SListNodeGetPrevious(const SListNodeBase* pNodeBase, const SListNodeBase* pNode)
	{
		while(pNodeBase && (pNodeBase->mpNext != pNode))
			pNodeBase = pNodeBase->mpNext;
		return pNodeBase;
	}

	inline void SListNodeSpliceAfter(SListNodeBase* pNode, SListNodeBase* pNodeBeforeFirst, SListNodeBase* pNodeBeforeLast)
	{
		if((pNode != pNodeBeforeFirst) && (pNode != pNodeBeforeLast))
		{
			SListNodeBase* const pFirst    = pNodeBeforeFirst->mpNext;
			SListNodeBase* const pPosition = pNode->mpNext;

			pNodeBeforeFirst->mpNext = pNodeBeforeLast->mpNext;
			pNode->mpNext            = pFirst;
			pNodeBeforeLast->mpNext  = pPosition;
		}
	}

	inline void SListNodeSpliceAfter(SListNodeBase* pNode, SListNodeBase* pNodeBase)
	{
		SListNodeBase* const pNodeBeforeLast = SListNodeGetPrevious(pNodeBase, NULL);

		if(pNodeBeforeLast != pNodeBase)
		{
			SListNodeBase* const pPosition = pNode->mpNext;
			pNode->mpNext           = pNodeBase->mpNext;
			pNodeBase->mpNext       = NULL;
			pNodeBeforeLast->mpNext = pPosition;
		}
	}

	inline SListNodeBase* SListNodeReverse(SListNodeBase* pNode)
	{
		SListNodeBase* pNodeFirst = pNode;
		pNode = pNode->mpNext;
		pNodeFirst->mpNext = NULL;

		while(pNode)
		{
			SListNodeBase* const pTemp = pNode->mpNext;
			pNode->mpNext = pNodeFirst;
			pNodeFirst    = pNode;
			pNode         = pTemp;
		}
		return pNodeFirst;
	}

	inline uint32_t SListNodeGetSize(SListNodeBase* pNode)
	{
		uint32_t n = 0;
		while(pNode)
		{
			++n;
			pNode = pNode->mpNext;
		}
		return n;
	}




	///////////////////////////////////////////////////////////////////////
	// SListIterator functions
	///////////////////////////////////////////////////////////////////////

	template <typename T, typename Pointer, typename Reference>
	inline SListIterator<T, Pointer, Reference>::SListIterator()
		: mpNode(NULL)
	{
		// Empty
	}


	template <typename T, typename Pointer, typename Reference>
	inline SListIterator<T, Pointer, Reference>::SListIterator(const SListNodeBase* pNode)
		: mpNode(const_cast<base_node_type*>(pNode))
	{
		// Empty
	}


	template <typename T, typename Pointer, typename Reference>
	inline typename SListIterator<T, Pointer, Reference>::reference
	SListIterator<T, Pointer, Reference>::operator*() const
	{
		return static_cast<node_type*>(mpNode)->mValue;
	}


	template <typename T, typename Pointer, typename Reference>
	inline typename SListIterator<T, Pointer, Reference>::pointer
	SListIterator<T, Pointer, Reference>::operator->() const
	{
		return &static_cast<node_type*>(mpNode)->mValue;
	}


	template <typename T, typename Pointer, typename Reference>
	inline typename SListIterator<T, Pointer, Reference>::this_type&
	SListIterator<T, Pointer, Reference>::operator++()
	{
		mpNode = mpNode->mpNext;
		return *this;
	}


	template <typename T, typename Pointer, typename Reference>
	inline typename SListIterator<T, Pointer, Reference>::this_type
	SListIterator<T, Pointer, Reference>::operator++(int)
	{
		this_type temp(*this);
		mpNode = mpNode->mpNext;
		return temp;
	}

	// The C++ defect report #179 requires that we support comparisons between const and non-const iterators.
	// Thus we provide additional template paremeters here to support this. The defect report does not
	// require us to support comparisons between reverse_iterators and const_reverse_iterators.
	template <typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB>
	inline bool operator==(const SListIterator<T, PointerA, ReferenceA>& a, 
						   const SListIterator<T, PointerB, ReferenceB>& b)
	{
		return a.mpNode == b.mpNode;
	}


	template <typename T, typename PointerA, typename ReferenceA, typename PointerB, typename ReferenceB>
	inline bool operator!=(const SListIterator<T, PointerA, ReferenceA>& a, 
						   const SListIterator<T, PointerB, ReferenceB>& b)
	{
		return a.mpNode != b.mpNode;
	}


	// We provide a version of operator!= for the case where the iterators are of the 
	// same type. This helps prevent ambiguity errors in the presence of rel_ops.
	template <typename T, typename Pointer, typename Reference>
	inline bool operator!=(const SListIterator<T, Pointer, Reference>& a, 
						   const SListIterator<T, Pointer, Reference>& b)
	{
		return a.mpNode != b.mpNode;
	}




	
	///////////////////////////////////////////////////////////////////////
	// SListBase functions
	///////////////////////////////////////////////////////////////////////

	template <typename T, typename Allocator>
	inline SListBase<T, Allocator>::SListBase()
		: mNodeAllocator(base_node_type(), allocator_type(EASTL_SLIST_DEFAULT_NAME))
		  #if EASTL_SLIST_SIZE_CACHE
		  , mSize(0)
		  #endif
	{ 
		internalNode().mpNext = NULL;
	}


	template <typename T, typename Allocator>
	inline SListBase<T, Allocator>::SListBase(const allocator_type& allocator)
		: mNodeAllocator(base_node_type(), allocator)
		  #if EASTL_SLIST_SIZE_CACHE
		  , mSize(0)
		  #endif
	{ 
		internalNode().mpNext = NULL;
	}


	template <typename T, typename Allocator>
	inline SListBase<T, Allocator>::~SListBase()
	{
		DoEraseAfter(&internalNode(), NULL);
	}


	template <typename T, typename Allocator>
	inline const typename SListBase<T, Allocator>::allocator_type&
	SListBase<T, Allocator>::get_allocator() const EA_NOEXCEPT
	{
		return internalAllocator();
	}


	template <typename T, typename Allocator>
	inline typename SListBase<T, Allocator>::allocator_type&
	SListBase<T, Allocator>::get_allocator() EA_NOEXCEPT
	{
		return internalAllocator();
	}


	template <typename T, typename Allocator>
	void
	SListBase<T, Allocator>::set_allocator(const allocator_type& allocator)
	{
		EASTL_ASSERT((internalAllocator() == allocator) || (static_cast<node_type*>(internalNode().mpNext) == NULL)); // We can only assign a different allocator if we are empty of elements.
		internalAllocator() = allocator;
	}


	template <typename T, typename Allocator>
	inline SListNode<T>* SListBase<T, Allocator>::DoAllocateNode()
	{
		return (node_type*)allocate_memory(internalAllocator(), sizeof(node_type), EASTL_ALIGN_OF(node_type), 0);
	}


	template <typename T, typename Allocator>
	inline void SListBase<T, Allocator>::DoFreeNode(node_type* pNode)
	{
		EASTLFree(internalAllocator(), pNode, sizeof(node_type));
	}


	template <typename T, typename Allocator>
	SListNodeBase* SListBase<T, Allocator>::DoEraseAfter(SListNodeBase* pNode)
	{
		node_type*     const pNodeNext     = static_cast<node_type*>((base_node_type*)pNode->mpNext);
		SListNodeBase* const pNodeNextNext = pNodeNext->mpNext;

		pNode->mpNext = pNodeNextNext;
		pNodeNext->~node_type();
		DoFreeNode(pNodeNext);
		#if EASTL_SLIST_SIZE_CACHE
		   --mSize;
		#endif
		return pNodeNextNext;
	}


	template <typename T, typename Allocator>
	SListNodeBase* SListBase<T, Allocator>::DoEraseAfter(SListNodeBase* pNode, SListNodeBase* pNodeLast)
	{
		node_type* pNodeCurrent = static_cast<node_type*>((base_node_type*)pNode->mpNext);

		while(pNodeCurrent != (base_node_type*)pNodeLast)
		{
			node_type* const pNodeTemp = pNodeCurrent;
			pNodeCurrent = static_cast<node_type*>((base_node_type*)pNodeCurrent->mpNext);
			pNodeTemp->~node_type();
			DoFreeNode(pNodeTemp);
			#if EASTL_SLIST_SIZE_CACHE
			--mSize;
			#endif
		}
		pNode->mpNext = pNodeLast;
		return pNodeLast;
	}




	///////////////////////////////////////////////////////////////////////
	// slist functions
	///////////////////////////////////////////////////////////////////////

	template <typename T, typename Allocator>
	inline slist<T, Allocator>::slist()
		: base_type()
	{
		// Empty
	}


	template <typename T, typename Allocator>
	inline slist<T, Allocator>::slist(const allocator_type& allocator)
		: base_type(allocator)
	{
		// Empty
	}


	template <typename T, typename Allocator>
	inline slist<T, Allocator>::slist(size_type n, const allocator_type& allocator)
		: base_type(allocator)
	{
		DoInsertValuesAfter(&internalNode(), n, value_type());
	}


	template <typename T, typename Allocator>
	inline slist<T, Allocator>::slist(size_type n, const value_type& value, const allocator_type& allocator)
		: base_type(allocator)
	{
		DoInsertValuesAfter(&internalNode(), n, value);
	}


	template <typename T, typename Allocator>
	inline slist<T, Allocator>::slist(const slist& x)
		: base_type(x.internalAllocator())
	{
		DoInsertAfter(&internalNode(), const_iterator(x.internalNode().mpNext), const_iterator(NULL), false_type());
	}


	template <typename T, typename Allocator>
	slist<T, Allocator>::slist(this_type&& x)
		: base_type(x.internalAllocator())
	{
		swap(x);
	}

	template <typename T, typename Allocator>
	slist<T, Allocator>::slist(this_type&& x, const allocator_type& allocator)
		: base_type(allocator)
	{
		swap(x); // member swap handles the case that x has a different allocator than our allocator by doing a copy.
	}


	template <typename T, typename Allocator>
	inline slist<T, Allocator>::slist(std::initializer_list<value_type> ilist, const allocator_type& allocator)
		: base_type(allocator)
	{
		DoInsertAfter(&internalNode(), ilist.begin(), ilist.end());
	}


	template <typename T, typename Allocator>
	template <typename InputIterator>
	inline slist<T, Allocator>::slist(InputIterator first, InputIterator last)
		: base_type(EASTL_SLIST_DEFAULT_ALLOCATOR)
	{
		DoInsertAfter(&internalNode(), first, last);
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::iterator
	slist<T, Allocator>::begin() EA_NOEXCEPT
	{
		return iterator(internalNode().mpNext);
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::const_iterator
	slist<T, Allocator>::begin() const EA_NOEXCEPT
	{
		return const_iterator(internalNode().mpNext);
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::const_iterator
	slist<T, Allocator>::cbegin() const EA_NOEXCEPT
	{
		return const_iterator(internalNode().mpNext);
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::iterator
	slist<T, Allocator>::end() EA_NOEXCEPT
	{
		return iterator(NULL);
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::const_iterator
	slist<T, Allocator>::end() const EA_NOEXCEPT
	{
		return const_iterator(NULL);
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::const_iterator
	slist<T, Allocator>::cend() const EA_NOEXCEPT
	{
		return const_iterator(NULL);
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::iterator
	slist<T, Allocator>::before_begin() EA_NOEXCEPT
	{
		return iterator(&internalNode());
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::const_iterator
	slist<T, Allocator>::before_begin() const EA_NOEXCEPT
	{
		return const_iterator(&internalNode());
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::const_iterator
	slist<T, Allocator>::cbefore_begin() const EA_NOEXCEPT
	{
		return const_iterator(&internalNode());
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::iterator
	slist<T, Allocator>::previous(const_iterator position)
	{
		return iterator(SListNodeGetPrevious(&internalNode(), position.mpNode));
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::const_iterator
	slist<T, Allocator>::previous(const_iterator position) const
	{
		return const_iterator(SListNodeGetPrevious(&internalNode(), position.mpNode));
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::reference
	slist<T, Allocator>::front()
	{
		#if EASTL_ASSERT_ENABLED
			if(EASTL_UNLIKELY(internalNode().mpNext == NULL))
				EASTL_FAIL_MSG("slist::front -- empty container");
		#endif

		EA_ANALYSIS_ASSUME(internalNode().mpNext != NULL);

		return ((node_type*)internalNode().mpNext)->mValue;
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::const_reference
	slist<T, Allocator>::front() const
	{
		#if EASTL_ASSERT_ENABLED
			if(EASTL_UNLIKELY(internalNode().mpNext == NULL))
				EASTL_FAIL_MSG("slist::front -- empty container");
		#endif

		EA_ANALYSIS_ASSUME(internalNode().mpNext != NULL);

		return static_cast<node_type*>(internalNode().mpNext)->mValue;
	}


	template <typename T, typename Allocator>
	template <class... Args>
	typename slist<T, Allocator>::reference slist<T, Allocator>::emplace_front(Args&&... args)
	{
		DoInsertValueAfter(&internalNode(), eastl::forward<Args>(args)...);
		return static_cast<node_type*>(internalNode().mpNext)->mValue; // Same as return front();
	}


	template <typename T, typename Allocator>
	inline void slist<T, Allocator>::push_front(const value_type& value)
	{
		SListNodeInsertAfter(&internalNode(), DoCreateNode(value));
		#if EASTL_SLIST_SIZE_CACHE
		   ++mSize;
		#endif
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::reference
	slist<T, Allocator>::push_front()
	{
		SListNodeInsertAfter(&internalNode(), DoCreateNode());
		#if EASTL_SLIST_SIZE_CACHE
		   ++mSize;
		#endif
		return ((node_type*)internalNode().mpNext)->mValue; // Same as return front();
	}

	template <typename T, typename Allocator>
	void slist<T, Allocator>::push_front(value_type&& value)
	{
		emplace_after(before_begin(), eastl::move(value));
	}


	template <typename T, typename Allocator>
	void slist<T, Allocator>::pop_front()
	{
		#if EASTL_ASSERT_ENABLED
			if(EASTL_UNLIKELY(internalNode().mpNext == NULL))
				EASTL_FAIL_MSG("slist::front -- empty container");
		#endif

		EA_ANALYSIS_ASSUME(internalNode().mpNext != NULL);

		node_type* const pNode = static_cast<node_type*>(internalNode().mpNext);
		internalNode().mpNext = pNode->mpNext;
		pNode->~node_type();
		DoFreeNode(pNode);
		#if EASTL_SLIST_SIZE_CACHE
		   --mSize;
		#endif
	}


	template <typename T, typename Allocator>
	typename slist<T, Allocator>::this_type& slist<T, Allocator>::operator=(const this_type& x)
	{
		if(&x != this)
		{
			// If (EASTL_ALLOCATOR_COPY_ENABLED == 1) and the current contents are allocated by an 
			// allocator that's unequal to x's allocator, we need to reallocate our elements with 
			// our current allocator and reallocate it with x's allocator. If the allocators are 
			// equal then we can use a more optimal algorithm that doesn't reallocate our elements
			// but instead can copy them in place.

			#if EASTL_ALLOCATOR_COPY_ENABLED
				bool bSlowerPathwayRequired = (internalAllocator() != x.internalAllocator());
			#else
				bool bSlowerPathwayRequired = false;
			#endif

			if(bSlowerPathwayRequired)
			{
				clear();

				#if EASTL_ALLOCATOR_COPY_ENABLED
					internalAllocator() = x.internalAllocator();
				#endif
			}

			DoAssign(x.begin(), x.end(), eastl::false_type());
		}

		return *this;
	}


	template <typename T, typename Allocator>
	typename slist<T, Allocator>::this_type& slist<T, Allocator>::operator=(this_type&& x)
	{
		if(this != &x)
		{
			clear();        // To consider: Are we really required to clear here? x is going away soon and will clear itself in its dtor.
			swap(x);        // member swap handles the case that x has a different allocator than our allocator by doing a copy.
		}
		return *this;
	}


	template <typename T, typename Allocator>
	typename slist<T, Allocator>::this_type& slist<T, Allocator>::operator=(std::initializer_list<value_type> ilist)
	{
		DoAssign(ilist.begin(), ilist.end(), false_type());
		return *this;
	}


	template <typename T, typename Allocator>
	inline void slist<T, Allocator>::assign(std::initializer_list<value_type> ilist)
	{
		DoAssign(ilist.begin(), ilist.end(), false_type());
	}


	template <typename T, typename Allocator>
	template <typename InputIterator>                                                // It turns out that the C++ std::list specifies a two argument
	inline void slist<T, Allocator>::assign(InputIterator first, InputIterator last) // version of assign that takes (int size, int value). These are not 
	{                                                                                // iterators, so we need to do a template compiler trick to do the right thing.
		DoAssign(first, last, is_integral<InputIterator>());
	}


	template <typename T, typename Allocator>
	inline void slist<T, Allocator>::assign(size_type n, const value_type& value)
	{
		// To do: get rid of DoAssignValues and put its implementation directly here.
		DoAssignValues(n, value);
	}


	template <typename T, typename Allocator>
	inline void slist<T, Allocator>::swap(this_type& x)
	{
		if(internalAllocator() == x.internalAllocator()) // If allocators are equivalent...
			DoSwap(x);
		else // else swap the contents.
		{
			const this_type temp(*this); // Can't call eastl::swap because that would
			*this = x;                   // itself call this member swap function.
			x     = temp;
		}
	}


	template <typename T, typename Allocator>
	inline bool slist<T, Allocator>::empty() const EA_NOEXCEPT
	{
		return internalNode().mpNext == NULL;
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::size_type
	slist<T, Allocator>::size() const EA_NOEXCEPT
	{
		return SListNodeGetSize(internalNode().mpNext);
	}


	template <typename T, typename Allocator>
	inline void slist<T, Allocator>::clear() EA_NOEXCEPT
	{
		DoEraseAfter(&internalNode(), NULL);
	}


	template <typename T, typename Allocator>
	inline void slist<T, Allocator>::reset_lose_memory() EA_NOEXCEPT
	{
		// The reset function is a special extension function which unilaterally 
		// resets the container to an empty state without freeing the memory of 
		// the contained objects. This is useful for very quickly tearing down a 
		// container built into scratch memory.
		internalNode().mpNext = NULL;
		#if EASTL_SLIST_SIZE_CACHE
			mSize = 0;
		#endif
	}


	template <typename T, typename Allocator>
	void slist<T, Allocator>::resize(size_type n, const value_type& value)
	{
		SListNodeBase* pNode = &internalNode();

		for(; pNode->mpNext && (n > 0); --n)
			pNode = pNode->mpNext;

		if(pNode->mpNext)
			DoEraseAfter(pNode, NULL);
		else
			DoInsertValuesAfter(pNode, n, value);
	}


	template <typename T, typename Allocator>
	inline void slist<T, Allocator>::resize(size_type n)
	{
		resize(n, value_type());
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::iterator
	slist<T, Allocator>::insert(const_iterator position)
	{
		return iterator(DoInsertValueAfter(SListNodeGetPrevious(&internalNode(), position.mpNode), value_type()));
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::iterator
	slist<T, Allocator>::insert(const_iterator position, const value_type& value)
	{
		return iterator(DoInsertValueAfter(SListNodeGetPrevious(&internalNode(), position.mpNode), value));
	}


	template <typename T, typename Allocator>
	inline void slist<T, Allocator>::insert(const_iterator position, size_type n, const value_type& value)
	{
		// To do: get rid of DoAssignValues and put its implementation directly here.
		DoInsertValuesAfter(SListNodeGetPrevious(&internalNode(), position.mpNode), n, value);
	}


	template <typename T, typename Allocator>
	template <typename InputIterator>
	inline void slist<T, Allocator>::insert(const_iterator position, InputIterator first, InputIterator last)
	{
		DoInsertAfter(SListNodeGetPrevious(&internalNode(), position.mpNode), first, last);
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::iterator
	slist<T, Allocator>::insert_after(const_iterator position)
	{
		return insert_after(position, value_type());
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::iterator
	slist<T, Allocator>::insert_after(const_iterator position, const value_type& value)
	{
		return iterator(DoInsertValueAfter(position.mpNode, value));
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::iterator
	slist<T, Allocator>::insert_after(const_iterator position, size_type n, const value_type& value)
	{
		return iterator(DoInsertValuesAfter(position.mpNode, n, value));
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::iterator
	slist<T, Allocator>::insert_after(const_iterator position, std::initializer_list<value_type> ilist)
	{
		return iterator(DoInsertAfter(position.mpNode, ilist.begin(), ilist.end(), false_type()));
	}


	template <typename T, typename Allocator>
	template <typename InputIterator>
	inline typename slist<T, Allocator>::iterator
	slist<T, Allocator>::insert_after(const_iterator position, InputIterator first, InputIterator last)
	{
		return iterator(DoInsertAfter(position.mpNode, first, last));
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::iterator
	slist<T, Allocator>::insert_after(const_iterator position, value_type&& value)
	{
		return emplace_after(position, eastl::move(value));
	}


	template <typename T, typename Allocator>
	template <class... Args>
	inline typename slist<T, Allocator>::iterator
	slist<T, Allocator>::emplace_after(const_iterator position, Args&&... args)
	{
		return iterator(DoInsertValueAfter(position.mpNode, eastl::forward<Args>(args)...));
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::iterator
	slist<T, Allocator>::erase(const_iterator position)
	{
		return DoEraseAfter(SListNodeGetPrevious(&internalNode(), position.mpNode));
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::iterator
	slist<T, Allocator>::erase(const_iterator first, const_iterator last)
	{
		return DoEraseAfter(SListNodeGetPrevious(&internalNode(), first.mpNode), last.mpNode);
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::iterator
	slist<T, Allocator>::erase_after(const_iterator position)
	{
		return iterator(DoEraseAfter(position.mpNode));
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::iterator
	slist<T, Allocator>::erase_after(const_iterator before_first, const_iterator last)
	{
		return iterator(DoEraseAfter(before_first.mpNode, last.mpNode));
	}


	template <typename T, typename Allocator>
	typename slist<T, Allocator>::size_type slist<T, Allocator>::remove(const value_type& value)
	{ 
		base_node_type* pNode = &internalNode();
		size_type numErased = 0;

		while(pNode && pNode->mpNext)
		{
			if (static_cast<node_type*>(pNode->mpNext)->mValue == value)
			{
				DoEraseAfter(pNode); // This will take care of modifying pNode->mpNext.
				++numErased;
			}
			else
				pNode = pNode->mpNext;
		}
		return numErased;
	}

	template <typename T, typename Allocator>
	template <typename Predicate>
	inline typename slist<T, Allocator>::size_type slist<T, Allocator>::remove_if(Predicate predicate)
	{
		base_node_type* pNode = &internalNode();
		size_type numErased = 0;

		while(pNode && pNode->mpNext)
		{
			if (predicate(static_cast<node_type*>(pNode->mpNext)->mValue))
			{
				DoEraseAfter(pNode); // This will take care of modifying pNode->mpNext.
				++numErased;
			}
			else
				pNode = pNode->mpNext;
		}
		return numErased;
	}


	template <typename T, typename Allocator>
	inline void slist<T, Allocator>::splice(const_iterator position, this_type& x)
	{
		// Splicing operations cannot succeed if the two containers use unequal allocators.
		// This issue is not addressed in the C++ 1998 standard but is discussed in the 
		// LWG defect reports, such as #431. There is no simple solution to this problem.
		// One option is to throw an exception. Another option which probably captures the
		// user intent most of the time is to copy the range from the source to the dest and 
		// remove it from the source. Until then it's simply disallowed to splice with unequal allocators.
		// EASTL_ASSERT(internalAllocator() == x.internalAllocator()); // Disabled because our member sort function uses splice but with allocators that may be unequal. There isn't a simple workaround aside from disabling this assert.

		if(x.internalNode().mpNext) // If there is anything to splice...
		{
			if(internalAllocator() == x.internalAllocator())
			{
				SListNodeSpliceAfter(SListNodeGetPrevious(&internalNode(), position.mpNode),
									 &x.internalNode(),
									 SListNodeGetPrevious(&x.internalNode(), NULL));

				#if EASTL_SLIST_SIZE_CACHE
					mSize += x.mSize;
					x.mSize = 0;
				#endif
			}
			else
			{
				insert(position, x.begin(), x.end());
				x.clear();
			}
		}
	}


	template <typename T, typename Allocator>
	inline void slist<T, Allocator>::splice(const_iterator position, this_type& x, const_iterator i)
	{
		if(internalAllocator() == x.internalAllocator())
		{
			SListNodeSpliceAfter(SListNodeGetPrevious(&internalNode(), position.mpNode),
								 SListNodeGetPrevious(&x.internalNode(), i.mpNode),
								 i.mpNode);

			#if EASTL_SLIST_SIZE_CACHE
				++mSize;
				--x.mSize;
			#endif
		}
		else
		{
			insert(position, *i);
			x.erase(i);
		}
	}


	template <typename T, typename Allocator>
	inline void slist<T, Allocator>::splice(const_iterator position, this_type& x, const_iterator first, const_iterator last)
	{
		if(first != last) // If there is anything to splice...
		{
			if(internalAllocator() == x.internalAllocator())
			{
				#if EASTL_SLIST_SIZE_CACHE
					const size_type n = (size_type)eastl::distance(first, last);
					mSize += n;
					x.mSize -= n;
				#endif

				SListNodeSpliceAfter(SListNodeGetPrevious(&internalNode(),       position.mpNode),
									 SListNodeGetPrevious(&x.internalNode(),     first.mpNode),
									 SListNodeGetPrevious(first.mpNode, last.mpNode));
			}
			else
			{
				insert(position, first, last);
				x.erase(first, last);
			}
		}
	}


	template <typename T, typename Allocator>
	void slist<T, Allocator>::splice(const_iterator position, this_type&& x)
	{
		return splice(position, x); // This will splice(const_iterator, this_type&)
	}

	template <typename T, typename Allocator>
	void slist<T, Allocator>::splice(const_iterator position, this_type&& x, const_iterator i)
	{
		return splice(position, x, i); // This will splice_after(const_iterator, this_type&, const_iterator)
	}

	template <typename T, typename Allocator>
	void slist<T, Allocator>::splice(const_iterator position, this_type&& x, const_iterator first, const_iterator last)
	{
		return splice(position, x, first, last); // This will splice(const_iterator, this_type&, const_iterator, const_iterator)
	}


	template <typename T, typename Allocator>
	inline void slist<T, Allocator>::splice_after(const_iterator position, this_type& x)
	{
		if(!x.empty()) // If there is anything to splice...
		{
			if(internalAllocator() == x.internalAllocator())
			{
				SListNodeSpliceAfter(position.mpNode, &x.internalNode());

				#if EASTL_SLIST_SIZE_CACHE
					mSize += x.mSize;
					x.mSize = 0;
				#endif
			}
			else
			{
				insert_after(position, x.begin(), x.end());
				x.clear();
			}
		}
	}


	template <typename T, typename Allocator>
	inline void slist<T, Allocator>::splice_after(const_iterator position, this_type& x, const_iterator i)
	{
		if(internalAllocator() == x.internalAllocator())
		{
			SListNodeSpliceAfter(position.mpNode, i.mpNode);

			#if EASTL_SLIST_SIZE_CACHE
				mSize++;
				x.mSize--;
			#endif
		}
		else
		{
			const_iterator iNext(i);
			insert_after(position, i, ++iNext);
			x.erase(i);
		}
	}


	template <typename T, typename Allocator>
	inline void slist<T, Allocator>::splice_after(const_iterator position, this_type& x, const_iterator first, const_iterator last)
	{
		if(first != last) // If there is anything to splice...
		{
			if(internalAllocator() == x.internalAllocator())
			{
				#if EASTL_SLIST_SIZE_CACHE
					const size_type n = (size_type)eastl::distance(first, last);
					mSize += n;
					x.mSize -= n;
				#endif

				SListNodeSpliceAfter(position.mpNode, first.mpNode, last.mpNode);
			}
			else
			{
				insert_after(position, first, last);
				x.erase(first, last);
			}
		}
	}


	template <typename T, typename Allocator>
	inline void slist<T, Allocator>::splice_after(const_iterator position, this_type&& x)
	{
		return splice_after(position, x);  // This will call splice_after(const_iterator, this_type&)
	}

	template <typename T, typename Allocator>
	inline void slist<T, Allocator>::splice_after(const_iterator position, this_type&& x, const_iterator i) 
	{
		return splice_after(position, x, i);  // This will call splice_after(const_iterator, this_type&, const_iterator)
	}

	template <typename T, typename Allocator>
	inline void slist<T, Allocator>::splice_after(const_iterator position, this_type&& x, const_iterator first, const_iterator last)
	{
		return splice_after(position, x, first, last);  // This will call splice_after(const_iterator, this_type&, const_iterator, const_iterator)
	}


	// This function is deprecated.
	// We have no way of knowing what the container or allocator for before_first/before_last is. 
	// Thus this function requires that the iterators come from equivalent allocators.
	template <typename T, typename Allocator>
	inline void slist<T, Allocator>::splice_after(const_iterator position, const_iterator before_first, const_iterator before_last)
	{
		if(before_first != before_last) // If there is anything to splice...
		{
			#if EASTL_SLIST_SIZE_CACHE
				// We have a problem here because the inserted range may come from *this or 
				// it may come from some other list. We have no choice but to implement an O(n)
				// brute-force search in our list for 'previous'.

				iterator i(&internalNode());
				iterator iEnd(NULL);

				for( ; i != iEnd; ++i)
				{
					if(i == before_first)
						break;
				}
	 
				if(i == iEnd) // If the input came from an external range...
					mSize += (size_type)eastl::distance(before_first, before_last); // Note that we have no way of knowing how to decrementing the size from the external container, assuming it came from one.
				else
					{ EASTL_FAIL_MSG("slist::splice_after: Impossible to decrement source mSize. Use the other splice_after function instead."); }
			#endif

			// Insert the range of [before_first + 1, before_last + 1) after position.
			SListNodeSpliceAfter(position.mpNode, before_first.mpNode, before_last.mpNode);
		}
	}


	// This function is deprecated.
	// We have no way of knowing what the container or allocator for previous is. 
	// Thus this function requires that the iterators come from equivalent allocators.
	template <typename T, typename Allocator>
	inline void slist<T, Allocator>::splice_after(const_iterator position, const_iterator previous)
	{
		#if EASTL_SLIST_SIZE_CACHE
			// We have a problem here because the inserted range may come from *this or 
			// it may come from some other list. We have no choice but to implement an O(n)
			// brute-force search in our list for 'previous'.

			iterator i(&internalNode());
			iterator iEnd(NULL);

			for( ; i != iEnd; ++i)
			{
				if(i == previous)
					break;
			}
 
			if(i == iEnd) // If the input came from an external range...
				++mSize;  // Note that we have no way of knowing how to decrementing the size from the external container, assuming it came from one.
			else
				{ EASTL_FAIL_MSG("slist::splice_after: Impossible to decrement source mSize. Use the other splice_after function instead."); }
		#endif

		// Insert the element at previous + 1 after position.
		SListNodeSpliceAfter(position.mpNode, previous.mpNode, previous.mpNode->mpNext);
	}


	template <typename T, typename Allocator>
	typename slist<T, Allocator>::size_type slist<T, Allocator>::unique()
	{
		size_type      numRemoved = 0;
		iterator       first(begin());
		const iterator last(end());

		if (first != last)
		{
			iterator next(first);

			while (++next != last)
			{
				if (*first == *next)
				{
					DoEraseAfter(first.mpNode);
					++numRemoved;
					next = first;
				}
				else
				{
					first = next;
				}
			}
		}

		return numRemoved;
	}


	template <typename T, typename Allocator>
	template <typename BinaryPredicate>
	typename slist<T, Allocator>::size_type slist<T, Allocator>::unique(BinaryPredicate predicate)
	{
		size_type      numRemoved = 0;
		iterator       first(begin());
		const iterator last(end());

		if (first != last)
		{
			iterator next(first);

			while (++next != last)
			{
				if (predicate(*first, *next))
				{
					DoEraseAfter(first.mpNode);
					++numRemoved;
					next = first;
				}
				else
				{
					first = next;
				}
			}
		}

		return numRemoved;
	}


	template <typename T, typename Allocator>
	inline void slist<T, Allocator>::sort()
	{
		// To do: look at using a merge sort, which may well be faster. 
		eastl::comb_sort(begin(), end());
	}


	template <typename T, typename Allocator>
	template <class Compare>
	inline void slist<T, Allocator>::sort(Compare compare)
	{
		// To do: look at using a merge sort, which may well be faster. 
		eastl::comb_sort(begin(), end(), compare);
	}



	template <typename T, typename Allocator>
	inline void slist<T, Allocator>::reverse() EA_NOEXCEPT
	{
		if(internalNode().mpNext)
			internalNode().mpNext = static_cast<node_type*>((base_node_type*)SListNodeReverse(internalNode().mpNext));
	}


	template <typename T, typename Allocator>
	template<typename... Args>
	inline typename slist<T, Allocator>::node_type*
	slist<T, Allocator>::DoCreateNode(Args&&... args)
	{
		node_type* const pNode = DoAllocateNode();  // pNode is of type node_type, but it's uninitialized memory.

		#if EASTL_EXCEPTIONS_ENABLED
			try
			{
				::new((void*)&pNode->mValue) value_type(eastl::forward<Args>(args)...);
			}
			catch(...)
			{
				DoFreeNode(pNode);
				throw;
			}
		#else
			::new((void*)&pNode->mValue) value_type(eastl::forward<Args>(args)...);
		#endif

		return pNode;
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::node_type*
	slist<T, Allocator>::DoCreateNode()
	{
		node_type* const pNode = DoAllocateNode();
		#if EASTL_EXCEPTIONS_ENABLED
			try
			{
				::new((void*)&pNode->mValue) value_type();
			}
			catch(...)
			{
				DoFreeNode(pNode);
				throw;
			}
		#else
			::new((void*)&pNode->mValue) value_type();
		#endif
		return pNode;
	}


	template <typename T, typename Allocator>
	template <typename Integer>
	void slist<T, Allocator>::DoAssign(Integer n, Integer value, true_type)
	{
		DoAssignValues(static_cast<size_type>(n), static_cast<value_type>(value));
	}


	template <typename T, typename Allocator>
	template <typename InputIterator>
	void slist<T, Allocator>::DoAssign(InputIterator first, InputIterator last, false_type)
	{
		base_node_type* pNodePrev = &internalNode();
		base_node_type* pNode     = internalNode().mpNext;

		for(; pNode && (first != last); ++first)
		{
			static_cast<node_type*>(pNode)->mValue = *first;
			pNodePrev     = pNode;
			pNode         = pNode->mpNext;
		}

		if(first == last)
			DoEraseAfter(pNodePrev, NULL);
		else
			DoInsertAfter(pNodePrev, first, last);
	}


	template <typename T, typename Allocator>
	void slist<T, Allocator>::DoAssignValues(size_type n, const value_type& value)
	{
		base_node_type* pNodePrev = &internalNode();
		base_node_type* pNode     = internalNode().mpNext;

		for(; pNode && (n > 0); --n)
		{
			static_cast<node_type*>(pNode)->mValue = value;
			pNodePrev     = pNode;
			pNode         = pNode->mpNext;
		}

		if(n)
			DoInsertValuesAfter(pNodePrev, n, value);
		else
			DoEraseAfter(pNodePrev, NULL);
	}
		

	template <typename T, typename Allocator>
	template <typename InputIterator>
	inline typename slist<T, Allocator>::node_type*
	slist<T, Allocator>::DoInsertAfter(SListNodeBase* pNode, InputIterator first, InputIterator last)
	{
		return DoInsertAfter(pNode, first, last, is_integral<InputIterator>());
	}


	template <typename T, typename Allocator>
	template <typename Integer>
	inline typename slist<T, Allocator>::node_type*
	slist<T, Allocator>::DoInsertAfter(SListNodeBase* pNode, Integer n, Integer value, true_type)
	{
		return DoInsertValuesAfter(pNode, n, value);
	}


	template <typename T, typename Allocator>
	template <typename InputIterator>
	inline typename slist<T, Allocator>::node_type*
	slist<T, Allocator>::DoInsertAfter(SListNodeBase* pNode, InputIterator first, InputIterator last, false_type)
	{
		for(; first != last; ++first)
		{
			pNode = SListNodeInsertAfter(pNode, DoCreateNode(*first));
			#if EASTL_SLIST_SIZE_CACHE
				++mSize;
			#endif
		}

		return static_cast<node_type*>((base_node_type*)pNode);
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::node_type*
	slist<T, Allocator>::DoInsertValueAfter(SListNodeBase* pNode)
	{
		#if EASTL_SLIST_SIZE_CACHE
			pNode = SListNodeInsertAfter(pNode, DoCreateNode());
			++mSize;
			return static_cast<node_type*>((base_node_type*)pNode);
		#else
			return static_cast<node_type*>((base_node_type*)SListNodeInsertAfter(pNode, DoCreateNode()));
		#endif
	}


	template <typename T, typename Allocator>
	template<typename... Args>
	inline typename slist<T, Allocator>::node_type*
	slist<T, Allocator>::DoInsertValueAfter(SListNodeBase* pNode, Args&&... args)
	{
		SListNodeBase* pNodeNew = DoCreateNode(eastl::forward<Args>(args)...);
		pNode = SListNodeInsertAfter(pNode, pNodeNew);
		#if EASTL_LIST_SIZE_CACHE
			++mSize; // Increment the size after the node creation because we need to assume an exception can occur in the creation.
		#endif
		return static_cast<node_type*>((base_node_type*)pNode);
	}


	template <typename T, typename Allocator>
	inline typename slist<T, Allocator>::node_type*
	slist<T, Allocator>::DoInsertValuesAfter(SListNodeBase* pNode, size_type n, const value_type& value)
	{
		for(size_type i = 0; i < n; ++i)
		{
			pNode = SListNodeInsertAfter(pNode, DoCreateNode(value));
			#if EASTL_SLIST_SIZE_CACHE
				++mSize; // We don't do a single mSize += n at the end because an exception may result in only a partial range insertion.
			#endif
		}
		return static_cast<node_type*>((base_node_type*)pNode);
	}


	template <typename T, typename Allocator>
	inline void slist<T, Allocator>::DoSwap(this_type& x)
	{
		eastl::swap(internalNode().mpNext, x.internalNode().mpNext);
		eastl::swap(internalAllocator(), x.internalAllocator()); // We do this even if EASTL_ALLOCATOR_COPY_ENABLED is 0.
		#if EASTL_LIST_SIZE_CACHE
			eastl::swap(mSize, x.mSize);
		#endif
	}


	template <typename T, typename Allocator>
	inline bool slist<T, Allocator>::validate() const
	{
		#if EASTL_SLIST_SIZE_CACHE
			size_type n = 0;
			
			for(const_iterator i(begin()), iEnd(end()); i != iEnd; ++i)
				++n;

			if(n != mSize)
				return false;
		#endif

		// To do: More validation.
		return true;
	}


	template <typename T, typename Allocator>
	inline int slist<T, Allocator>::validate_iterator(const_iterator i) const
	{
		// To do: Come up with a more efficient mechanism of doing this.

		for(const_iterator temp = begin(), tempEnd = end(); temp != tempEnd; ++temp)
		{
			if(temp == i)
				return (isf_valid | isf_current | isf_can_dereference);
		}

		if(i == end())
			return (isf_valid | isf_current); 

		return isf_none;
	}


	///////////////////////////////////////////////////////////////////////
	// global operators
	///////////////////////////////////////////////////////////////////////

	template <typename T, typename Allocator>
	bool operator==(const slist<T, Allocator>& a, const slist<T, Allocator>& b)
	{
		typename slist<T, Allocator>::const_iterator ia   = a.begin();
		typename slist<T, Allocator>::const_iterator ib   = b.begin();
		typename slist<T, Allocator>::const_iterator enda = a.end();

		#if EASTL_SLIST_SIZE_CACHE
			if(a.size() == b.size())
			{
				while((ia != enda) && (*ia == *ib))
				{
					++ia;
					++ib;
				}
				return (ia == enda);
			}
			return false;
		#else
			typename slist<T, Allocator>::const_iterator endb = b.end();

			while((ia != enda) && (ib != endb) && (*ia == *ib))
			{
				++ia;
				++ib;
			}
			return (ia == enda) && (ib == endb);
		#endif
	}

#if defined(EA_COMPILER_HAS_THREE_WAY_COMPARISON)
	template <typename T, typename Allocator>
	inline synth_three_way_result<T> operator<=>(const slist<T, Allocator>& a, const slist<T, Allocator>& b)
	{
		return eastl::lexicographical_compare_three_way(a.begin(), a.end(), b.begin(), b.end(), synth_three_way{});
	}
#else
	template <typename T, typename Allocator>
	inline bool operator<(const slist<T, Allocator>& a, const slist<T, Allocator>& b)
	{
		return eastl::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end());
	}


	template <typename T, typename Allocator>
	inline bool operator!=(const slist<T, Allocator>& a, const slist<T, Allocator>& b)
	{
		return !(a == b);
	}


	template <typename T, typename Allocator>
	inline bool operator>(const slist<T, Allocator>& a, const slist<T, Allocator>& b)
	{
		return b < a;
	}


	template <typename T, typename Allocator>
	inline bool operator<=(const slist<T, Allocator>& a, const slist<T, Allocator>& b)
	{
		return !(b < a);
	}


	template <typename T, typename Allocator>
	inline bool operator>=(const slist<T, Allocator>& a, const slist<T, Allocator>& b)
	{
		return !(a < b);
	}
#endif

	template <typename T, typename Allocator>
	inline void swap(slist<T, Allocator>& a, slist<T, Allocator>& b)
	{
		a.swap(b);
	}


	/// erase / erase_if
	///
	/// https://en.cppreference.com/w/cpp/container/forward_list/erase2
	template <class T, class Allocator, class U>
	typename slist<T, Allocator>::size_type erase(slist<T, Allocator>& c, const U& value)
	{
		// Erases all elements that compare equal to value from the container.
		return c.remove(value);
	}

	template <class T, class Allocator, class Predicate>
	typename slist<T, Allocator>::size_type erase_if(slist<T, Allocator>& c, Predicate predicate)
	{
		// Erases all elements that satisfy the predicate pred from the container.
		return c.remove_if(predicate);
	}


	/// insert_iterator
	///
	/// We borrow a trick from SGI STL here and define an insert_iterator 
	/// specialization for slist. This allows slist insertions to be O(1) 
	/// instead of O(n/2), due to caching of the previous node.
	///
	template <typename T, typename Allocator>
	class insert_iterator< slist<T, Allocator> >
	{
	public:
		typedef slist<T, Allocator>                 Container;
		typedef typename Container::const_reference const_reference;
		typedef typename Container::iterator        iterator_type;
		typedef EASTL_ITC_NS::output_iterator_tag   iterator_category;
		typedef void                                value_type;
		typedef void                                difference_type;
		typedef void                                pointer;
		typedef void                                reference;

	protected:
		Container&    container;
		iterator_type it;

	public:
		insert_iterator(Container& x, iterator_type i)
			: container(x)
		{
			if(i == x.begin())
				it = x.before_begin();
			else
				it = x.previous(i);
		}

		insert_iterator<Container>& operator=(const_reference value)
			{ it = container.insert_after(it, value); return *this; }

		insert_iterator<Container>& operator*()
			{ return *this; }

		insert_iterator<Container>& operator++()
			{ return *this; } // This is by design.

		insert_iterator<Container>& operator++(int)
			{ return *this; } // This is by design.

	}; // insert_iterator<slist>


} // namespace eastl

EA_RESTORE_SN_WARNING()

EA_RESTORE_VC_WARNING();


#endif // Header include guard
