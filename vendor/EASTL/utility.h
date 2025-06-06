///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_UTILITY_H
#define EASTL_UTILITY_H


#include "vendor/EASTL/internal/config.h"
#include "vendor/EASTL/type_traits.h"
#include "vendor/EASTL/iterator.h"
#include "vendor/EASTL/numeric_limits.h"
#include "vendor/EASTL/compare.h"
#include "vendor/EASTL/internal/functional_base.h"
#include "vendor/EASTL/internal/move_help.h"
#include "vendor/EASTL/meta.h"
#include "vendor/EABase/eahave.h"

#include "vendor/EASTL/internal/integer_sequence.h"
#include "vendor/EASTL/internal/tuple_fwd_decls.h"
#include "vendor/EASTL/internal/in_place_t.h"
#include "vendor/EASTL/internal/piecewise_construct_t.h"


// 4619 - There is no warning number 'number'.
// 4217 - Member template functions cannot be used for copy-assignment or copy-construction.
// 4512/4626 - 'class' : assignment operator could not be generated.  // This disabling would best be put elsewhere.
EA_DISABLE_VC_WARNING(4619 4217 4512 4626);


#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once // Some compilers (e.g. VC++) benefit significantly from using this. We've measured 3-4% build speed improvements in apps as a result.
#endif



namespace eastl
{

	/// swap
	///
	/// Assigns the contents of a to b and the contents of b to a. 
	/// A temporary instance of type T is created and destroyed
	/// in the process.
	///
	/// This function is used by numerous other algorithms, and as 
	/// such it may in some cases be feasible and useful for the user 
	/// to implement an override version of this function which is 
	/// more efficient in some way. 
	///

	template <typename T> 
	inline void swap(T& a, T& b) EA_NOEXCEPT_IF(eastl::is_nothrow_move_constructible<T>::value && eastl::is_nothrow_move_assignable<T>::value)
	{
		T temp(EASTL_MOVE(a));  // EASTL_MOVE uses EASTL::move when available, else is a no-op.
		a = EASTL_MOVE(b);
		b = EASTL_MOVE(temp);
	}


	/// is_swappable
	///
	/// Determines if two types can be swapped via the swap function. This determines
	/// only if there is a swap function that matches the types and not if the assignments
	/// within the swap implementation are valid.
	/// Returns false for pre-C++11 compilers that don't support decltype.
	///
	/// This is a type trait, but it's not currently found within <type_traits.h>,
	/// as it's dependent on the swap algorithm, which is at a higher level than
	/// type traits.
	///
	/// Example usage:
	///     static_assert(is_swappable<int>::value, "int should be swappable");
	///
	#if defined(EA_COMPILER_NO_DECLTYPE)
		#define EASTL_TYPE_TRAIT_is_swappable_CONFORMANCE 0

		template <typename>
		struct is_swappable
			: public eastl::false_type {};
	#else
		#define EASTL_TYPE_TRAIT_is_swappable_CONFORMANCE 1

		// We declare this version of 'eastl::swap' to make compile-time existance checks for swap functions possible.  
		//
		#if EASTL_VARIADIC_TEMPLATES_ENABLED
			eastl::unused swap(eastl::argument_sink, eastl::argument_sink);
		#else
			// Compilers that do not support variadic templates suffer from a bug with variable arguments list that
			// causes the construction of aligned types in unaligned memory. To prevent the aligned type construction we
			// accept the parameters by reference.
			eastl::unused swap(eastl::argument_sink&, eastl::argument_sink&);
		#endif

		template <typename T>
		struct is_swappable
			: public integral_constant<bool, !eastl::is_same<decltype(swap(eastl::declval<T&>(), eastl::declval<T&>())), eastl::unused>::value> {}; // Don't prefix swap with eastl:: as we want to allow user-defined swaps via argument-dependent lookup.
	#endif
	
	#if EASTL_VARIABLE_TEMPLATES_ENABLED
        template <class T>
        EA_CONSTEXPR bool is_swappable_v = is_swappable<T>::value;
    #endif



	/// is_nothrow_swappable
	///
	/// Evaluates to true if is_swappable, and swap is a nothrow function.
	/// returns false for pre-C++11 compilers that don't support nothrow.
	///
	/// This is a type trait, but it's not currently found within <type_traits.h>,
	/// as it's dependent on the swap algorithm, which is at a higher level than
	/// type traits.
	///
	#define EASTL_TYPE_TRAIT_is_nothrow_swappable_CONFORMANCE EASTL_TYPE_TRAIT_is_swappable_CONFORMANCE

	template <typename T>
	struct is_nothrow_swappable_helper_noexcept_wrapper
		{ const static bool value = noexcept(swap(eastl::declval<T&>(), eastl::declval<T&>())); };

	template <typename T, bool>
	struct is_nothrow_swappable_helper
		: public eastl::integral_constant<bool, is_nothrow_swappable_helper_noexcept_wrapper<T>::value> {}; // Don't prefix swap with eastl:: as we want to allow user-defined swaps via argument-dependent lookup.

	template <typename T>
	struct is_nothrow_swappable_helper<T, false>
		: public eastl::false_type {};

	template <typename T>
	struct is_nothrow_swappable
		: public eastl::is_nothrow_swappable_helper<T, eastl::is_swappable<T>::value> {};

	#if EASTL_VARIABLE_TEMPLATES_ENABLED
        template <class T>
        EA_CONSTEXPR bool is_nothrow_swappable_v = is_nothrow_swappable<T>::value;
    #endif


	
	/// is_swappable_with
	///
	///
	template <typename T, typename U, bool OneTypeIsVoid = (eastl::is_void<T>::value || eastl::is_void<U>::value)>
	struct is_swappable_with_helper
	{
		// Don't prefix swap with eastl:: as we want to allow user-defined swaps via argument-dependent lookup.
	    static const bool value =
	        !eastl::is_same<decltype(swap(eastl::declval<T>(), eastl::declval<U>())), eastl::unused>::value &&
	        !eastl::is_same<decltype(swap(eastl::declval<U>(), eastl::declval<T>())), eastl::unused>::value;
    };

	template <typename T, typename U>
	struct is_swappable_with_helper<T,U, true> { static const bool value = false; };

    template<typename T, typename U>
	struct is_swappable_with 
			: public eastl::bool_constant<is_swappable_with_helper<T, U>::value> {}; 

	#if EASTL_VARIABLE_TEMPLATES_ENABLED
        template <class T, class U>
        EA_CONSTEXPR bool is_swappable_with_v = is_swappable_with<T, U>::value;
    #endif


	
	/// is_nothrow_swappable_with
	///
	///
	#if defined(EA_COMPILER_NO_DECLTYPE) || defined(EA_COMPILER_NO_NOEXCEPT) 
		#define EASTL_TYPE_TRAIT_is_nothrow_swappable_with_CONFORMANCE 0
		template <typename T, typename U>
		struct is_nothrow_swappable_with_helper { static const bool value = false; };
	#else
		#define EASTL_TYPE_TRAIT_is_nothrow_swappable_with_CONFORMANCE 1
		template <typename T, typename U, bool OneTypeIsVoid = (eastl::is_void<T>::value || eastl::is_void<U>::value)>
		struct is_nothrow_swappable_with_helper
		{
	        // Don't prefix swap with eastl:: as we want to allow user-defined swaps via argument-dependent lookup.
	        static const bool value = noexcept(swap(eastl::declval<T>(), eastl::declval<U>())) &&
	                                  noexcept(swap(eastl::declval<U>(), eastl::declval<T>()));
        };

		template <typename T, typename U>
		struct is_nothrow_swappable_with_helper<T,U, true> { static const bool value = false; };
	#endif

    template <typename T, typename U>
    struct is_nothrow_swappable_with : public eastl::bool_constant<is_nothrow_swappable_with_helper<T, U>::value> {};

    #if EASTL_VARIABLE_TEMPLATES_ENABLED
        template <class T, class U>
        EA_CONSTEXPR bool is_nothrow_swappable_with_v = is_nothrow_swappable_with<T, U>::value;
    #endif
	


	// iter_swap helper functions
	//
	template <bool bTypesAreEqual>
	struct iter_swap_impl
	{
		// Handles the false case, where *a and *b are different types.
		template <typename ForwardIterator1, typename ForwardIterator2>
		static void iter_swap(ForwardIterator1 a, ForwardIterator2 b)
		{
			typedef typename eastl::iterator_traits<ForwardIterator1>::value_type value_type_a;

			value_type_a temp(EASTL_MOVE(*a)); // EASTL_MOVE uses EASTL::move when available, else is a no-op.
			*a = EASTL_MOVE(*b);
			*b = EASTL_MOVE(temp); 
		}
	};

	template <>
	struct iter_swap_impl<true>
	{
		template <typename ForwardIterator1, typename ForwardIterator2>
		static void iter_swap(ForwardIterator1 a, ForwardIterator2 b)
		{
			swap(*a, *b);  // Don't prefix swap with eastl:: as we want to allow user-defined swaps via argument-dependent lookup.
		}
	};


	/// iter_swap
	///
	/// Swaps the values of the elements the given iterators are pointing to. 
	///
	/// Equivalent to swap(*a, *b), though the user can provide an override to
	/// iter_swap that is independent of an override which may exist for swap.
	///
	/// We provide a version of iter_swap which uses swap when the swapped types 
	/// are equal but a manual implementation otherwise. We do this because the 
	/// C++ standard defect report says that iter_swap(a, b) must be implemented 
	/// as swap(*a, *b) when possible.
	///
	template <typename ForwardIterator1, typename ForwardIterator2>
	inline void iter_swap(ForwardIterator1 a, ForwardIterator2 b)
	{
		typedef typename eastl::iterator_traits<ForwardIterator1>::value_type value_type_a;
		typedef typename eastl::iterator_traits<ForwardIterator2>::value_type value_type_b;
		typedef typename eastl::iterator_traits<ForwardIterator1>::reference  reference_a;
		typedef typename eastl::iterator_traits<ForwardIterator2>::reference  reference_b;

		eastl::iter_swap_impl<eastl::conjunction<eastl::is_same<value_type_a, value_type_b>, eastl::is_same<value_type_a&, reference_a>, eastl::is_same<value_type_b&, reference_b>>::value >::iter_swap(a, b);
	}



	/// swap_ranges
	///
	/// Swaps each of the elements in the range [first1, last1) with the 
	/// corresponding element in the range [first2, first2 + (last1 - first1)). 
	///
	/// Effects: For each nonnegative integer n < (last1 - first1),
	/// performs: swap(*(first1 + n), *(first2 + n)).
	///
	/// Requires: The two ranges [first1, last1) and [first2, first2 + (last1 - first1))
	/// shall not overlap.
	///
	/// Returns: first2 + (last1 - first1). That is, returns the end of the second range.
	///
	/// Complexity: Exactly 'last1 - first1' swaps.
	///
	template <typename ForwardIterator1, typename ForwardIterator2>
	inline ForwardIterator2
	swap_ranges(ForwardIterator1 first1, ForwardIterator1 last1, ForwardIterator2 first2)
	{
		for(; first1 != last1; ++first1, ++first2)
			iter_swap(first1, first2); // Don't prefix swap with eastl:: as we want to allow user-defined swaps via argument-dependent lookup.
		return first2;
	}


	/// swap
	///
	/// C++11 array swap
	/// http://en.cppreference.com/w/cpp/algorithm/swap
	///
	template <typename T, size_t N>
	inline void
	swap(T (&a)[N], T (&b)[N]) EA_NOEXCEPT_IF(eastl::is_nothrow_swappable<T>::value)
	{
		eastl::swap_ranges(a, a + N, b);
	}


	/// exchange
	///
	/// Replaces the value of the first argument with the new value provided.  
	/// The return value is the previous value of first argument.
	///
	/// http://en.cppreference.com/w/cpp/utility/exchange
	///
	template <typename T, typename U = T>
	inline T exchange(T& obj, U&& new_value)
	{
		T old_value = eastl::move(obj);
		obj = eastl::forward<U>(new_value);
		return old_value;
	}


	/// as_const
	///
	/// Converts a 'T&' into a 'const T&' which simplifies calling const functions on non-const objects. 
	///
	/// http://en.cppreference.com/w/cpp/utility/as_const
	///
	/// C++ proposal paper:
	/// http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/n4380.html
	///
	template <class T>
	EA_CONSTEXPR typename eastl::add_const<T>::type& as_const(T& t) EA_NOEXCEPT
		{ return t; }

	// The C++17 forbids 'eastl::as_const' from accepting rvalues.  Passing an rvalue reference to 'eastl::as_const'
	// generates an 'const T&' or const lvalue reference to a temporary object. 
	template <class T>
	void as_const(const T&&) = delete;


    ///////////////////////////////////////////////////////////////////////
	/// rel_ops
	///
	/// rel_ops allow the automatic generation of operators !=, >, <=, >= from
	/// just operators == and <. These are intentionally in the rel_ops namespace
	/// so that they don't conflict with other similar operators. To use these 
	/// operators, add "using namespace std::rel_ops;" to an appropriate place in 
	/// your code, usually right in the function that you need them to work.
	/// In fact, you will very likely have collision problems if you put such
	/// using statements anywhere other than in the .cpp file like so and may 
	/// also have collisions when you do, as the using statement will affect all
	/// code in the module. You need to be careful about use of rel_ops.
	///
	namespace rel_ops
	{
		template <typename T>
		inline bool operator!=(const T& x, const T& y)
			{ return !(x == y); }

		template <typename T>
		inline bool operator>(const T& x, const T& y)
			{ return (y < x); }

		template <typename T>
		inline bool operator<=(const T& x, const T& y)
			{ return !(y < x); }

		template <typename T>
		inline bool operator>=(const T& x, const T& y)
			{ return !(x < y); }
	}


	#if defined(EA_COMPILER_CPP20_ENABLED)
	///////////////////////////////////////////////////////////////////////
	/// Safe Integral Comparisons
	///
	template <typename T, typename U>
	EA_CONSTEXPR bool cmp_equal(const T x, const U y) EA_NOEXCEPT
	{
		// Assert types are not chars, bools, etc.
		static_assert(eastl::is_integral_v<T> && !eastl::is_same_v<eastl::remove_cv_t<T>, bool> && !eastl::is_same_v<eastl::remove_cv_t<T>, char>);
		static_assert(eastl::is_integral_v<U> && !eastl::is_same_v<eastl::remove_cv_t<U>, bool> && !eastl::is_same_v<eastl::remove_cv_t<U>, char>);

		using UT = eastl::make_unsigned_t<T>;
		using UU = eastl::make_unsigned_t<U>;

		if constexpr (eastl::is_signed_v<T> == eastl::is_signed_v<U>)
		{
			return x == y;
		}
		else if (eastl::is_signed_v<T>)
		{
			return (x < 0) ? false : UT(x) == y;
		}
		else
		{
			return (y < 0) ? false : x == UU(y);
		}
	}


	template <typename T, typename U>
	EA_CONSTEXPR bool cmp_not_equal(const T x, const U y) EA_NOEXCEPT
		{ return !eastl::cmp_equal(x, y); }


	template <typename T, typename U>
	EA_CONSTEXPR bool cmp_less(const T x, const U y) EA_NOEXCEPT
	{
		static_assert(eastl::is_integral_v<T> && !eastl::is_same_v<eastl::remove_cv_t<T>, bool> && !eastl::is_same_v<eastl::remove_cv_t<T>, char>);
		static_assert(eastl::is_integral_v<U> && !eastl::is_same_v<eastl::remove_cv_t<U>, bool> && !eastl::is_same_v<eastl::remove_cv_t<U>, char>);

		using UT = eastl::make_unsigned_t<T>;
		using UU = eastl::make_unsigned_t<U>;

		if constexpr (eastl::is_signed_v<T> == eastl::is_signed_v<U>)
		{
			return x < y;
		}
		else if (eastl::is_signed_v<T>)
		{
			return (x < 0) ? true : UT(x) < y;
		}
		else
		{
			return (y < 0) ? false : x < UU(y);
		}
	}


	template <typename T, typename U>
	EA_CONSTEXPR bool cmp_greater(const T x, const U y) EA_NOEXCEPT
		{ return eastl::cmp_less(y, x); }


	template <typename T, typename U>
	EA_CONSTEXPR bool cmp_less_equal(const T x, const U y) EA_NOEXCEPT
		{ return !eastl::cmp_greater(x, y); }


	template <typename T, typename U>
	EA_CONSTEXPR bool cmp_greater_equal(const T x, const U y) EA_NOEXCEPT
		{ return !eastl::cmp_less(x, y); }


	template <typename T, typename U>
	EA_CONSTEXPR bool in_range(const U x) EA_NOEXCEPT
	{
		static_assert(eastl::is_integral_v<T> && !eastl::is_same_v<eastl::remove_cv_t<T>, bool> && !eastl::is_same_v<eastl::remove_cv_t<T>, char>);
		static_assert(eastl::is_integral_v<U> && !eastl::is_same_v<eastl::remove_cv_t<U>, bool> && !eastl::is_same_v<eastl::remove_cv_t<U>, char>);

		return eastl::cmp_greater_equal(x, eastl::numeric_limits<T>::min()) && eastl::cmp_less_equal(x, eastl::numeric_limits<T>::max());
	}
	#endif


	///////////////////////////////////////////////////////////////////////
	/// pair_first_construct
	///
	/// Disambiguates when a user is requesting the 'single first element' pair constructor.
	///
	struct pair_first_construct_t {};
	EA_CONSTEXPR pair_first_construct_t pair_first_construct = pair_first_construct_t();

	
	///////////////////////////////////////////////////////////////////////
	/// pair
	///
	/// Implements a simple pair, just like the C++ std::pair.
	///
	template <typename T1, typename T2>
	struct pair
	{
		typedef T1           first_type;
		typedef T2           second_type;
		typedef pair<T1, T2> this_type;

		T1 first;
		T2 second;

		template <typename TT1 = T1,
		          typename TT2 = T2,
		          class = eastl::enable_if_t<eastl::is_default_constructible_v<TT1> &&
		                                     eastl::is_default_constructible_v<TT2>>>
		EA_CONSTEXPR pair()
		    : first(), second()
		{
		}

	#if EASTL_ENABLE_PAIR_FIRST_ELEMENT_CONSTRUCTOR
		template <typename TT1 = T1, typename TT2 = T2, typename = eastl::enable_if_t<eastl::is_default_constructible_v<TT2>>>
		EA_CPP14_CONSTEXPR pair(const TT1& x)
		    : first(x), second()
		{
		}

		// GCC has a bug with overloading rvalue and lvalue function templates.	
		// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54425	
		//	
		// error: 'eastl::pair<T1, T2>::pair(T1&&) [with T1 = const int&; T2 = const int&]' cannot be overloaded	
		// error: with 'eastl::pair<T1, T2>::pair(const T1&) [with T1 = const int&; T2 = const int&]'
		#if !defined(EA_COMPILER_GNUC)
			template <typename TT2 = T2, typename = eastl::enable_if_t<eastl::is_default_constructible_v<TT2>>>
			EA_CPP14_CONSTEXPR pair(T1&& x)
				: first(eastl::move(x)), second()
			{
			}
		#endif
	#endif

	
	// NOTE(rparolin): 
	// This is a workaround to a compiler intrinic bug which fails to correctly identify a nested class using
	// non-static data member initialization as default constructible.  
	//
	// See bug submitted to LLVM for more details.
	// https://bugs.llvm.org/show_bug.cgi?id=38374
	#if !defined(__clang__)
		template<typename T>
		using single_pair_ctor_sfinae = eastl::enable_if_t<eastl::is_default_constructible_v<T>>;
	#else
		template<typename>
		using single_pair_ctor_sfinae = void;
	#endif

		template <typename TT1 = T1, typename TT2 = T2, typename = single_pair_ctor_sfinae<TT2>>
		EA_CPP14_CONSTEXPR pair(pair_first_construct_t, const TT1& x)
		    : first(x), second()
		{
		}

		// GCC has a bug with overloading rvalue and lvalue function templates.	
		// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=54425	
		//	
		// error: 'eastl::pair<T1, T2>::pair(T1&&) [with T1 = const int&; T2 = const int&]' cannot be overloaded	
		// error: with 'eastl::pair<T1, T2>::pair(const T1&) [with T1 = const int&; T2 = const int&]'
		#if !defined(EA_COMPILER_GNUC)
			template <typename TT2 = T2, typename = single_pair_ctor_sfinae<TT2>>
			EA_CPP14_CONSTEXPR pair(pair_first_construct_t, T1&& x)
				: first(eastl::move(x)), second()
			{
			}
		#endif

		template <
		    typename TT1 = T1,
		    typename TT2 = T2,
		    class = eastl::enable_if_t<eastl::is_copy_constructible_v<TT1> && eastl::is_copy_constructible_v<TT2>>>
		EA_CPP14_CONSTEXPR pair(const T1& x, const T2& y)
		    : first(x), second(y)
		{
		}

		EA_CPP14_CONSTEXPR pair(pair&& p) = default;
		EA_CPP14_CONSTEXPR pair(const pair&) = default;
		EA_CPP14_CONSTEXPR pair& operator=(const pair&) = default;
		EA_CPP14_CONSTEXPR pair& operator=(pair&&) = default;

		template <
		    typename U,
		    typename V,
		    class = eastl::enable_if_t<eastl::is_convertible_v<const U&, T1> && eastl::is_convertible_v<const V&, T2>>>
		EA_CPP14_CONSTEXPR pair(const pair<U, V>& p)
		    : first(p.first), second(p.second)
		{
		}

		template <typename U,
		          typename V,
		          typename = eastl::enable_if_t<eastl::is_convertible_v<U, T1> && eastl::is_convertible_v<V, T2>>>
		EA_CPP14_CONSTEXPR pair(U&& u, V&& v)
		    : first(eastl::forward<U>(u)), second(eastl::forward<V>(v))
		{
		}

		template <typename U, typename = eastl::enable_if_t<eastl::is_convertible_v<U, T1>>>
		EA_CPP14_CONSTEXPR pair(U&& x, const T2& y)
			: first(eastl::forward<U>(x)), second(y)
		{
		}

		template <typename V, typename = eastl::enable_if_t<eastl::is_convertible_v<V, T2>>>
		EA_CPP14_CONSTEXPR pair(const T1& x, V&& y)
			: first(x), second(eastl::forward<V>(y))
		{
		}

		template <typename U,
		          typename V,
		          typename = eastl::enable_if_t<eastl::is_convertible_v<U, T1> && eastl::is_convertible_v<V, T2>>>
		EA_CPP14_CONSTEXPR pair(pair<U, V>&& p)
		    : first(eastl::forward<U>(p.first)), second(eastl::forward<V>(p.second))
		{
		}

		// Initializes first with arguments of types Args1... obtained by forwarding the elements of first_args and
		// initializes second with arguments of types Args2... obtained by forwarding the elements of second_args.
		template <class... Args1,
		          class... Args2,
		          typename = eastl::enable_if_t<eastl::is_constructible_v<first_type, Args1&&...> &&
		                                        eastl::is_constructible_v<second_type, Args2&&...>>>
		pair(eastl::piecewise_construct_t pwc, eastl::tuple<Args1...> first_args, eastl::tuple<Args2...> second_args)
		    : pair(pwc,
		           eastl::move(first_args),
		           eastl::move(second_args),
		           eastl::make_index_sequence<sizeof...(Args1)>(),
		           eastl::make_index_sequence<sizeof...(Args2)>())
		{
		}

	private:
		// NOTE(rparolin): Internal constructor used to expand the index_sequence required to expand the tuple elements.
		template <class... Args1, class... Args2, size_t... I1, size_t... I2>
		pair(eastl::piecewise_construct_t,
			 eastl::tuple<Args1...> first_args,
			 eastl::tuple<Args2...> second_args,
			 eastl::index_sequence<I1...>,
			 eastl::index_sequence<I2...>)
			: first(eastl::forward<Args1>(eastl::get<I1>(first_args))...)
			, second(eastl::forward<Args2>(eastl::get<I2>(second_args))...)
		{
		}

	public:
		template <typename U,
		          typename V,
		          typename = eastl::enable_if_t<eastl::is_convertible_v<U, T1> && eastl::is_convertible_v<V, T2>>>
		pair& operator=(const pair<U, V>& p)
		{
			first = p.first;
			second = p.second;
			return *this;
		}

		template <typename U,
		          typename V,
		          typename = eastl::enable_if_t<eastl::is_convertible_v<U, T1> && eastl::is_convertible_v<V, T2>>>
		pair& operator=(pair<U, V>&& p)
		{
			first = eastl::forward<U>(p.first);
			second = eastl::forward<V>(p.second);
			return *this;
		}

		void swap(pair& p) EA_NOEXCEPT_IF(eastl::is_nothrow_swappable_v<T1>&& eastl::is_nothrow_swappable_v<T2>)
		{
			eastl::iter_swap(&first, &p.first);
			eastl::iter_swap(&second, &p.second);
		}
	};

	#define EASTL_PAIR_CONFORMANCE 1



	/// use_self
	///
	/// operator()(x) simply returns x. Used in sets, as opposed to maps.
	/// This is a template policy implementation; it is an alternative to 
	/// the use_first template implementation.
	///
	/// The existance of use_self may seem odd, given that it does nothing,
	/// but these kinds of things are useful, virtually required, for optimal 
	/// generic programming.
	///
	template <typename T>
	struct use_self
	{
		typedef T result_type;

		const T& operator()(const T& x) const
			{ return x; }
	};

	/// use_first
	///
	/// operator()(x) simply returns x.first. Used in maps, as opposed to sets.
	/// This is a template policy implementation; it is an alternative to 
	/// the use_self template implementation. This is the same thing as the
	/// SGI SGL select1st utility.
	///
	template <typename Pair>
	struct use_first
	{
		typedef Pair argument_type;
		typedef typename Pair::first_type result_type;

		const result_type& operator()(const Pair& x) const
			{ return x.first; }
	};

	/// use_second
	///
	/// operator()(x) simply returns x.second. 
	/// This is the same thing as the SGI SGL select2nd utility
	///
	template <typename Pair>
	struct use_second
	{
		typedef Pair argument_type;
		typedef typename Pair::second_type result_type;

		const result_type& operator()(const Pair& x) const
			{ return x.second; }
	};





	///////////////////////////////////////////////////////////////////////
	// global operators
	///////////////////////////////////////////////////////////////////////

	template <typename T1, typename T2>
	EA_CPP14_CONSTEXPR inline bool operator==(const pair<T1, T2>& a, const pair<T1, T2>& b)
	{
		return ((a.first == b.first) && (a.second == b.second));
	}

	#if defined(EA_COMPILER_HAS_THREE_WAY_COMPARISON)
	template <typename T1, typename T2>
	EA_CONSTEXPR inline std::common_comparison_category_t<synth_three_way_result<T1>, synth_three_way_result<T2>> operator<=>(const pair<T1, T2>& a, const pair<T1, T2>& b)
	{
		if (auto result = synth_three_way{}(a.first, b.first); result != 0)
		{
			return result;
		}
		return synth_three_way{}(a.second, b.second);
	}
	#else
	template <typename T1, typename T2>
	EA_CPP14_CONSTEXPR inline bool operator<(const pair<T1, T2>& a, const pair<T1, T2>& b)
	{
		// Note that we use only operator < in this expression. Otherwise we could
		// use the simpler: return (a.m1 == b.m1) ? (a.m2 < b.m2) : (a.m1 < b.m1);
		// The user can write a specialization for this operator to get around this
		// in cases where the highest performance is required.
		return ((a.first < b.first) || (!(b.first < a.first) && (a.second < b.second)));
	}


	template <typename T1, typename T2>
	EA_CPP14_CONSTEXPR inline bool operator!=(const pair<T1, T2>& a, const pair<T1, T2>& b)
	{
		return !(a == b);
	}


	template <typename T1, typename T2>
	EA_CPP14_CONSTEXPR inline bool operator>(const pair<T1, T2>& a, const pair<T1, T2>& b)
	{
		return b < a;
	}


	template <typename T1, typename T2>
	EA_CPP14_CONSTEXPR inline bool operator>=(const pair<T1, T2>& a, const pair<T1, T2>& b)
	{
		return !(a < b);
	}


	template <typename T1, typename T2>
	EA_CPP14_CONSTEXPR inline bool operator<=(const pair<T1, T2>& a, const pair<T1, T2>& b)
	{
		return !(b < a);
	}
	#endif



	///////////////////////////////////////////////////////////////////////
	/// make_pair
	///
	/// Create a pair, deducing the element types from the types of arguments.
	/// 
	/// Note: You don't need to use make_pair in order to make a pair, it's a convenience for deducing the element types. 
	/// C++17's class template argument deduction (https://en.cppreference.com/w/cpp/language/class_template_argument_deduction)
	/// performs the same purpose as make_unique, make_tuple and similar.
	/// 
	/// The following code all declare pairs of the same type:
	///     pair<char*, char*> p1(1, "explicit");
	///     auto p2 = make_pair(2, "deduce types");
	///     pair p3(3, "language feature"); // requires C++17
	///
	template <typename T1, typename T2>
	EA_CPP14_CONSTEXPR inline pair<typename eastl::remove_reference_wrapper<typename eastl::decay<T1>::type>::type, 
								   typename eastl::remove_reference_wrapper<typename eastl::decay<T2>::type>::type>
	make_pair(T1&& a, T2&& b)
	{
		typedef typename eastl::remove_reference_wrapper<typename eastl::decay<T1>::type>::type T1Type;
		typedef typename eastl::remove_reference_wrapper<typename eastl::decay<T2>::type>::type T2Type;

		return eastl::pair<T1Type, T2Type>(eastl::forward<T1>(a), eastl::forward<T2>(b));
	}


	// Note: DO NOT pass type parameters to make_pair().
	// eg.
	//   const int myInt = 0;
	//   const char* myCStr = "example";
	//   auto pair1 = eastl::make_pair<int, eastl::string>(myInt, myCStr);
	//                                ^^^^^^^^^^^^^^^^^^^^--- wrong.
	// 
	// use one of the following, depending on the intended pair type: 
	//   eastl::pair<int, const char*> pair2 = eastl::make_pair(myInt, myCStr);
	//   eastl::pair<int, eastl::string> pair3(myInt, myCStr);
	//
#if defined(_MSC_VER)
	template <typename T1, typename T2, typename DeduceT1, typename DeduceT2>
	EASTL_REMOVE_AT_2024_SEPT EA_CPP14_CONSTEXPR inline pair<T1, T2> make_pair(
		const DeduceT1& a,
		const DeduceT2& b,
		typename eastl::enable_if<!eastl::is_array<T1>::value && !eastl::is_array<T2>::value>::type* = 0)
	{
		return eastl::pair<T1, T2>(a, b);
	}
#endif

	// use make_pair() instead. they are equivalent.
	template <typename T1, typename T2>
	EASTL_REMOVE_AT_2024_SEPT
	EA_CPP14_CONSTEXPR inline pair<typename eastl::remove_reference_wrapper<typename eastl::decay<T1>::type>::type, 
								   typename eastl::remove_reference_wrapper<typename eastl::decay<T2>::type>::type>
	make_pair_ref(T1&& a, T2&& b)
	{
		typedef typename eastl::remove_reference_wrapper<typename eastl::decay<T1>::type>::type T1Type;
		typedef typename eastl::remove_reference_wrapper<typename eastl::decay<T2>::type>::type T2Type;

		return eastl::pair<T1Type, T2Type>(eastl::forward<T1>(a), eastl::forward<T2>(b));
	}

#if EASTL_TUPLE_ENABLED

		template <typename T1, typename T2>
		struct tuple_size<pair<T1, T2>> : public integral_constant<size_t, 2>
		{
		};

		template <typename T1, typename T2>
		struct tuple_size<const pair<T1, T2>> : public integral_constant<size_t, 2>
		{
		};

		template <typename T1, typename T2>
		struct tuple_element<0, pair<T1, T2>>
		{
		public:
			typedef T1 type;
		};

		template <typename T1, typename T2>
		struct tuple_element<1, pair<T1, T2>>
		{
		public:
			typedef T2 type;
		};

		template <typename T1, typename T2>
		struct tuple_element<0, const pair<T1, T2>>
		{
		public:
			typedef const T1 type;
		};

		template <typename T1, typename T2>
		struct tuple_element<1, const pair<T1, T2>>
		{
		public:
			typedef const T2 type;
		};

		namespace internal
		{
			template <size_t I>
			struct GetPair;

			template <>
			struct GetPair<0>
			{
				template <typename T1, typename T2>
				static EA_CONSTEXPR T1& get(pair<T1, T2>& p)
				{
					return p.first;
				}

				template <typename T1, typename T2>
				static EA_CONSTEXPR const T1& get(const pair<T1, T2>& p)
				{
					return p.first;
				}

				template <typename T1, typename T2>
				static EA_CONSTEXPR T1&& get(pair<T1, T2>&& p)
				{
					return eastl::forward<T1>(p.first);
				}

				template <typename T1, typename T2>
				static EA_CONSTEXPR const T1&& get(const pair<T1, T2>&& p)
				{
					return eastl::forward<const T1>(p.first);
				}
			};

			template <>
			struct GetPair<1>
			{
				template <typename T1, typename T2>
				static EA_CONSTEXPR T2& get(pair<T1, T2>& p)
				{
					return p.second;
				}

				template <typename T1, typename T2>
				static EA_CONSTEXPR const T2& get(const pair<T1, T2>& p)
				{
					return p.second;
				}

				template <typename T1, typename T2>
				static EA_CONSTEXPR T2&& get(pair<T1, T2>&& p)
				{
					return eastl::forward<T2>(p.second);
				}

				template <typename T1, typename T2>
				static EA_CONSTEXPR const T2&& get(const pair<T1, T2>&& p)
				{
					return eastl::forward<const T2>(p.second);
				}
			};
		} // namespace internal

		template <size_t I, typename T1, typename T2>
		EA_CPP14_CONSTEXPR tuple_element_t<I, pair<T1, T2>>& get(pair<T1, T2>& p)
		{
			return internal::GetPair<I>::get(p);
		}

		template <size_t I, typename T1, typename T2>
		EA_CPP14_CONSTEXPR const tuple_element_t<I, pair<T1, T2>>& get(const pair<T1, T2>& p)
		{
			return internal::GetPair<I>::get(p);
		}

		template <size_t I, typename T1, typename T2>
		EA_CPP14_CONSTEXPR tuple_element_t<I, pair<T1, T2>>&& get(pair<T1, T2>&& p)
		{
			return internal::GetPair<I>::get(eastl::move(p));
		}

		template <size_t I, typename T1, typename T2>
		EA_CPP14_CONSTEXPR const tuple_element_t<I, pair<T1, T2>>&& get(const pair<T1, T2>&& p)
		{
			return internal::GetPair<I>::get(eastl::move(p));
		}

		template <typename T1, typename T2>
		EA_CONSTEXPR T1& get(pair<T1, T2>& p) EA_NOEXCEPT
		{
			return internal::GetPair<0>::get(p);
		}

		template <typename T2, typename T1>
		EA_CONSTEXPR T2& get(pair<T1, T2>& p) EA_NOEXCEPT
		{
			return internal::GetPair<1>::get(p);
		}

		template <typename T1, typename T2>
		EA_CONSTEXPR const T1& get(const pair<T1, T2>& p) EA_NOEXCEPT
		{
			return internal::GetPair<0>::get(p);
		}

		template <typename T2, typename T1>
		EA_CONSTEXPR const T2& get(const pair<T1, T2>& p) EA_NOEXCEPT
		{
			return internal::GetPair<1>::get(p);
		}

		template <typename T1, typename T2>
		EA_CONSTEXPR T1&& get(pair<T1, T2>&& p) EA_NOEXCEPT
		{
			return internal::GetPair<0>::get(eastl::move(p));
		}

		template <typename T2, typename T1>
		EA_CONSTEXPR T2&& get(pair<T1, T2>&& p) EA_NOEXCEPT
		{
			return internal::GetPair<1>::get(eastl::move(p));
		}

		template <typename T1, typename T2>
		EA_CONSTEXPR const T1&& get(const pair<T1, T2>&& p) EA_NOEXCEPT
		{
			return internal::GetPair<0>::get(eastl::move(p));
		}

		template <typename T2, typename T1>
		EA_CONSTEXPR const T2&& get(const pair<T1, T2>&& p) EA_NOEXCEPT
		{
			return internal::GetPair<1>::get(eastl::move(p));
		}

#endif  // EASTL_TUPLE_ENABLED


}  // namespace eastl

///////////////////////////////////////////////////////////////
// C++17 structured bindings support for eastl::pair
//
#ifndef EA_COMPILER_NO_STRUCTURED_BINDING
// we can't forward declare tuple_size and tuple_element because some std implementations
// don't declare it in the std namespace, but instead alias it.
#include <utility>

	namespace std
	{
		template <class... Ts>
		struct tuple_size<::eastl::pair<Ts...>> : public ::eastl::integral_constant<size_t, sizeof...(Ts)>
		{
		};

		template <size_t I, class... Ts>
		struct tuple_element<I, ::eastl::pair<Ts...>> : public ::eastl::tuple_element<I, ::eastl::pair<Ts...>>
		{
		};
	}
#endif


EA_RESTORE_VC_WARNING();


#endif // Header include guard
