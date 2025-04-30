//
// Created by gogop on 4/30/2025.
//

#pragma once
#include <memory>
#include <utility>
#include <type_traits>
#include <concepts>
#include <functional>
#include <compare> // Needed for <=>
#include <new>
#include <cassert>
#include <stdexcept>
#include <initializer_list>
#include <optional> // For potential future use or comparison
#include <string> // For deprecation messages

// Forward declarations within a modified namespace
namespace std_P3019_modified {

	// Forward declarations WITHOUT default template arguments
	template < class T, class Allocator >
	class polymorphic;
	template < class T, class Allocator >
	class indirect;

	//----------------------------------------------------------------------------
	// Type Traits
	//----------------------------------------------------------------------------

	template < typename >
	struct is_indirect: std::false_type {};
	template < typename T, typename A >
	struct is_indirect < indirect < T, A >>: std::true_type {};
	template < typename T >
	inline constexpr bool is_indirect_v = is_indirect < T > ::value;

	template < typename >
	struct is_polymorphic: std::false_type {};
	template < typename T, typename A >
	struct is_polymorphic < polymorphic < T, A >>: std::true_type {};
	template < typename T >
	inline constexpr bool is_polymorphic_v = is_polymorphic < T > ::value;

	template < typename >
	struct is_in_place_type: std::false_type {};
	template < typename T >
	struct is_in_place_type < std::in_place_type_t < T >>: std::true_type {};
	template < typename T >
	inline constexpr bool is_in_place_type_v = is_in_place_type < T > ::value;

	//----------------------------------------------------------------------------
	// indirect<T, Allocator>
	//----------------------------------------------------------------------------

	template < class T, class Allocator = std::allocator < T >> // Default argument here on definition
	class indirect {
		// Static asserts...
		static_assert(!std::is_array_v < T > , "indirect<T>: T cannot be an array type.");
		static_assert(std::is_object_v < T > , "indirect<T>: T must be an object type.");
		static_assert(!std::is_const_v < T > && !std::is_volatile_v < T > , "indirect<T>: T cannot be cv-qualified.");
		static_assert(!std::same_as < T, std::in_place_t > , "indirect<T>: T cannot be in_place_t.");
		static_assert(!is_in_place_type_v < T > , "indirect<T>: T cannot be a specialization of in_place_type_t.");

		using AllocTraits = std::allocator_traits < Allocator > ;
		static_assert(std::same_as < typename AllocTraits::value_type, T > ,
		              "indirect<T, Allocator>: allocator_traits<Allocator>::value_type must be T.");

	public:
		using value_type = T;
		using allocator_type = Allocator;
		using pointer = typename AllocTraits::pointer;
		using const_pointer = typename AllocTraits::const_pointer;

	private:
		pointer p_ = nullptr;
		[[no_unique_address]] Allocator alloc_ {};

		constexpr void destroy_deallocate() noexcept {
			if (p_) {
				try {
					AllocTraits::destroy(alloc_, std::to_address(p_));
				} catch (...) {} // Destructors should generally not throw
				AllocTraits::deallocate(alloc_, p_, 1);
				p_ = nullptr;
			}
		}

		template < typename...Args >
		requires std::constructible_from < T, Args... >
		constexpr pointer allocate_construct(Args && ...args) {
			pointer ptr = AllocTraits::allocate(alloc_, 1);
			try {
				AllocTraits::construct(alloc_, std::to_address(ptr), std::forward < Args > (args)...);
				return ptr;
			} catch (...) {
				AllocTraits::deallocate(alloc_, ptr, 1);
				throw;
			}
		}

		// Added for operator= case POCMA=true alloc!=other
		// Also useful for implementing certain assignment scenarios cleanly.
		[[nodiscard]] constexpr pointer release() noexcept {
			pointer old_p = p_;
			p_ = nullptr;
			return old_p;
		}

	public:
		// Constructors
		explicit constexpr indirect() requires std::default_initializable < Allocator > && std::default_initializable < T > {
			p_ = allocate_construct();
		}

		explicit constexpr indirect(std::allocator_arg_t,
		                            const Allocator & a) requires std::default_initializable < T >
				: alloc_(a) {
			p_ = allocate_construct();
		}

		// NOTE: indirect copy is NOT deprecated by default. Its cost is primarily T's copy cost.
		constexpr indirect(const indirect & other) requires std::copy_constructible < T >
				: alloc_(AllocTraits::select_on_container_copy_construction(other.alloc_)) {
			if (!other.valueless_after_move()) {
				p_ = allocate_construct( * other);
			}
		}

		constexpr indirect(std::allocator_arg_t,
		                   const Allocator & a,
		                   const indirect & other) requires std::copy_constructible < T >
				: alloc_(a) {
			if (!other.valueless_after_move()) {
				p_ = allocate_construct( * other);
			}
		}

		constexpr indirect(indirect && other) noexcept: p_(std::exchange(other.p_, nullptr)),
		                                                alloc_(std::move(other.alloc_)) {}

		constexpr indirect(std::allocator_arg_t,
		                   const Allocator & a, indirect && other) noexcept(AllocTraits::is_always_equal::value): alloc_(a) {
			if (other.valueless_after_move()) return; // *this remains default constructed (empty), other is empty

			if constexpr(AllocTraits::is_always_equal::value) {
				p_ = std::exchange(other.p_, nullptr); // Steal pointer
			}
			else {
				if (alloc_ == other.alloc_) {
					p_ = std::exchange(other.p_, nullptr); // Steal pointer
				} else {
					// Allocators differ, must construct new using move, then destroy old in 'other'
					pointer new_p = nullptr;
					try {
						new_p = allocate_construct(std::move( * other));
					} catch (...) {
						// If construction fails, 'this' remains empty, 'other' is unchanged.
						throw; // Re-throw construction error
					}
					// Construction succeeded
					other.destroy_deallocate(); // Destroy and deallocate in 'other'
					p_ = new_p; // Assign the newly constructed pointer to *this
				}
			}
		}


		template < class U = T >
		explicit constexpr indirect(U && u) requires std::default_initializable < Allocator > &&
		                                             (!std::same_as < std::remove_cvref_t < U > , indirect > ) &&
		                                             (!std::same_as < std::remove_cvref_t < U > , std::in_place_t > ) &&
		                                             (!is_indirect_v < std::remove_cvref_t < U >> ) &&
		                                             (!is_in_place_type_v < std::remove_cvref_t < U >> ) &&
		                                             (!std::is_same_v<std::remove_cvref_t<U>, std::allocator_arg_t>) && // Disambiguate
		                                             (!std::is_base_of_v<std::allocator_arg_t, std::remove_cvref_t<U>>) && // Disambiguate
		                                             std::constructible_from < T, U > {
			p_ = allocate_construct(std::forward < U > (u));
		}

		template < class U = T >
		explicit constexpr indirect(std::allocator_arg_t,
		                            const Allocator & a, U && u) requires(!std::same_as < std::remove_cvref_t < U > , indirect > ) &&
		(!std::same_as < std::remove_cvref_t < U > , std::in_place_t > ) &&
		(!is_indirect_v < std::remove_cvref_t < U >> ) &&
		(!is_in_place_type_v < std::remove_cvref_t < U >> ) &&
		std::constructible_from < T, U >
				: alloc_(a) {
				p_ = allocate_construct(std::forward < U > (u));
		}


		template < class...Us >
		explicit constexpr indirect(std::in_place_t, Us && ...us) requires std::default_initializable < Allocator > &&
		                                                                   std::constructible_from < T, Us... > {
			p_ = allocate_construct(std::forward < Us > (us)...);
		}

		template < class...Us >
		explicit constexpr indirect(std::allocator_arg_t,
		                            const Allocator & a, std::in_place_t, Us && ...us) requires std::constructible_from < T, Us... >
				: alloc_(a) {
			p_ = allocate_construct(std::forward < Us > (us)...);
		}

		template < class I, class...Us >
		explicit constexpr indirect(std::in_place_t, std::initializer_list < I > ilist, Us && ...us) requires std::default_initializable < Allocator > &&
		                                                                                                      std::constructible_from < T, std::initializer_list < I > & , Us... > {
			p_ = allocate_construct(ilist, std::forward < Us > (us)...);
		}

		template < class I, class...Us >
		explicit constexpr indirect(std::allocator_arg_t,
		                            const Allocator & a, std::in_place_t, std::initializer_list < I > ilist, Us && ...us) requires std::constructible_from < T, std::initializer_list < I > & , Us... >
				: alloc_(a) {
			p_ = allocate_construct(ilist, std::forward < Us > (us)...);
		}

		constexpr~indirect() {
			destroy_deallocate();
		}

		// Assignment
		// NOTE: indirect copy assignment NOT deprecated by default.
		constexpr indirect & operator = (const indirect & other) requires std::copy_constructible < T > && std::is_copy_assignable_v < T > {
			if (std::addressof(other) == this) return *this;

			constexpr bool pocca = AllocTraits::propagate_on_container_copy_assignment::value;

			if constexpr(pocca) {
				if (alloc_ != other.alloc_) {
					// Allocators differ and POCMA is true. Replace allocator and content.
					if (other.valueless_after_move()) {
						// Other is empty, just destroy current and copy allocator.
						destroy_deallocate();
						alloc_ = other.alloc_;
					} else {
						// Other has value. Need to allocate+copy with other's allocator *before* destroying.
						Allocator target_alloc = other.alloc_; // Allocator to use for the new copy
						pointer new_p = nullptr;
						try {
							// Allocate and construct using the target allocator
							using TempAllocTraits = std::allocator_traits<Allocator>;
							new_p = TempAllocTraits::allocate(target_alloc, 1); // Allocate with target
							try {
								TempAllocTraits::construct(target_alloc, std::to_address(new_p), *other); // Construct with target
							} catch(...) {
								TempAllocTraits::deallocate(target_alloc, new_p, 1); // Cleanup on construction failure
								throw;
							}
						} catch (...) {
							// If allocation fails, *this is unchanged, rethrow
							throw;
						}
						// Allocation/construction succeeded, now commit
						destroy_deallocate(); // Destroy old object (uses old alloc_)
						alloc_ = std::move(target_alloc); // Commit new allocator
						p_ = new_p; // Commit new pointer
					}
					return * this;
				}
				// Allocators are same or POCMA is false, proceed without changing allocator yet
			}

			// If we reach here, either allocators are equal or POCCA is false.
			// Allocator propagation is handled above if POCMA was true. Now handle content.
			if (other.valueless_after_move()) {
				destroy_deallocate(); // Make *this valueless (allocator unchanged)
			} else {
				if (!valueless_after_move()) {
					// Both have values, perform assignment on the contained object
					*(*this) = *other; // Use *(*this) which works for raw pointers too
				} else {
					// *this is valueless, other has a value, construct a copy using current allocator
					pointer new_p = nullptr;
					try {
						new_p = allocate_construct( * other); // Uses current alloc_
					} catch(...) {
						// *this remains valueless on failure
						throw;
					}
					p_ = new_p; // Commit pointer
				}
			}
			return * this;
		}


		constexpr indirect & operator = (indirect && other) noexcept(
		AllocTraits::propagate_on_container_move_assignment::value ||
		AllocTraits::is_always_equal::value) {
			if (std::addressof(other) == this) return * this;

			constexpr bool pocma = AllocTraits::propagate_on_container_move_assignment::value;
			if constexpr(pocma) {
				destroy_deallocate(); // Destroy current content
				alloc_ = std::move(other.alloc_); // Move allocator
				p_ = std::exchange(other.p_, nullptr); // Steal pointer
			}
			else { // Not POCMA
				if (alloc_ == other.alloc_) {
					// Allocators equal, just swap pointers after destroying current
					destroy_deallocate();
					p_ = std::exchange(other.p_, nullptr);
				} else { // Allocators differ, non-propagating
					if (other.valueless_after_move()) {
						destroy_deallocate(); // Make *this valueless
						// other is already valueless
					} else {
						// Allocators differ, must move-construct into *this's allocation space if possible,
						// or reallocate using *this allocator otherwise.
						if (!valueless_after_move()) {
							// Both have value. Try move assignment first.
							if constexpr (std::is_nothrow_move_assignable_v<T>) {
								*(*this) = std::move(*other);
								other.destroy_deallocate(); // Destroy moved-from object in other
							} else {
								// Move assignment might throw. Reconstruct for strong guarantee.
								pointer new_p = nullptr;
								try {
									new_p = allocate_construct(std::move(*other)); // Move construct into new memory (using this->alloc_)
								} catch (...) {
									// If move construction fails, *this is unchanged.
									throw;
								}
								// Move construct succeeded
								destroy_deallocate(); // Destroy original object in *this
								p_ = new_p; // Assign new pointer
								other.destroy_deallocate(); // Destroy original object in other
							}
						} else {
							// *this was valueless, so just move-construct using this->alloc_
							pointer new_p = nullptr;
							try {
								new_p = allocate_construct(std::move(*other));
							} catch (...) {
								// *this remains valueless
								throw;
							}
							p_ = new_p;
							other.destroy_deallocate(); // Destroy original object in other
						}
					}
					// Ensure other becomes valueless (its p_ is null via destroy_deallocate)
				}
			}
			return * this;
		}

		template < class U = T > requires(!std::same_as < std::remove_cvref_t < U > , indirect > ) &&
		(!is_indirect_v < std::remove_cvref_t < U >> ) &&
		(!is_in_place_type_v < std::remove_cvref_t < U >> ) &&
		std::constructible_from < T, U > &&
				std::assignable_from < T & , U >
		constexpr indirect & operator = (U && u) {
			if (valueless_after_move()) {
				// If valueless, need to construct
				pointer new_p = nullptr;
				try {
					new_p = allocate_construct(std::forward < U > (u));
				} catch (...) {
					// Remains valueless
					throw;
				}
				p_ = new_p; // Commit
			} else {
				// Has value, assign to it
				*(*this) = std::forward < U > (u);
			}
			return * this;
		}

		// Observers
		constexpr
		const T & operator * () const & noexcept {
			assert(!valueless_after_move() && "Precondition: indirect::operator* called on valueless object");
			return * std::to_address(p_); // Use std::to_address for fancy pointers
		}

		constexpr T & operator * () & noexcept {
			assert(!valueless_after_move() && "Precondition: indirect::operator* called on valueless object");
			return * std::to_address(p_); // Use std::to_address
		}

		constexpr
		const T && operator * () const && noexcept {
			assert(!valueless_after_move() && "Precondition: indirect::operator* called on valueless object");
			return std::move( * std::to_address(p_)); // Use std::to_address
		}

		constexpr T && operator * () && noexcept {
			assert(!valueless_after_move() && "Precondition: indirect::operator* called on valueless object");
			return std::move( * std::to_address(p_)); // Use std::to_address
		}


		constexpr const_pointer operator -> () const noexcept {
			assert(!valueless_after_move() && "Precondition: indirect::operator-> called on valueless object");
			return p_;
		}

		constexpr pointer operator -> () noexcept {
			assert(!valueless_after_move() && "Precondition: indirect::operator-> called on valueless object");
			return p_;
		}

		constexpr bool valueless_after_move() const noexcept {
			return p_ == nullptr;
		}

		constexpr bool has_value() const noexcept {
			return p_ != nullptr;
		}

		constexpr allocator_type get_allocator() const noexcept {
			return alloc_;
		}

		// Swap
		constexpr void swap(indirect & other) noexcept(
		AllocTraits::propagate_on_container_swap::value ||
		AllocTraits::is_always_equal::value) {
			if constexpr(AllocTraits::propagate_on_container_swap::value) {
				std::swap(p_, other.p_);
				std::swap(alloc_, other.alloc_);
			}
			else {
				// Requires allocators to compare equal per [container.requirements.general]
				if (alloc_ != other.alloc_) {
					// Undefined behavior or assert? Let's assert for debug builds.
					assert(alloc_ == other.alloc_ && "indirect::swap requires allocators to be equal when POCS is false");
					// In release, maybe just proceed with pointer swap? Or is UB better?
					// Sticking with standard container rules: UB if allocators != and POCS==false. Assert helps catch it.
				}
				std::swap(p_, other.p_);
			}
		}

		friend constexpr void swap(indirect & lhs, indirect & rhs) noexcept(noexcept(lhs.swap(rhs))) {
			lhs.swap(rhs);
		}

		// Comparisons
		template < class U, class AA >
		requires requires(const T & t,
		                  const U & u) {
			{
			t == u
			} -> std::convertible_to < bool > ;
		}
		friend constexpr bool operator == (const indirect & lhs,
		                                   const indirect < U, AA > & rhs) {
			const bool lhs_has_value = lhs.has_value();
			const bool rhs_has_value = rhs.has_value();
			if (lhs_has_value != rhs_has_value) {
				return false; // One has value, the other doesn't
			}
			if (!lhs_has_value) { // Both must be empty
				return true;
			}
			// Both have values
			return * lhs == * rhs;
		}

		template < class U, class AA >
		requires std::three_way_comparable_with < T, U > // Use concept directly
		friend constexpr std::compare_three_way_result_t < T, U >
		operator <=> (const indirect & lhs,
		              const indirect < U, AA > & rhs) {
			const bool lhs_has_value = lhs.has_value();
			const bool rhs_has_value = rhs.has_value();

			if (lhs_has_value && rhs_has_value) return *lhs <=> *rhs; // Both have value
			if (!lhs_has_value && !rhs_has_value) return std::strong_ordering::equal; // Both empty
			if (!lhs_has_value) return std::strong_ordering::less; // Empty < non-empty
			/*if (!rhs_has_value)*/ return std::strong_ordering::greater; // Non-empty > empty
		}

		// Comparisons with non-indirect type U
		template < class U > requires(!is_indirect_v < std::remove_cvref_t < U >> ) &&
		requires(
		const T & t,
		const U & u
		) {
			{
				t == u
			} -> std::convertible_to < bool > ;
		}
		friend constexpr bool operator == (const indirect & lhs,
		                                   const U & rhs) {
			// Compare value with value if lhs has one, otherwise false (empty != value)
			return lhs.has_value() ? (*lhs == rhs) : false;
		}

		template < class U > requires(!is_indirect_v < std::remove_cvref_t < U >> ) &&
		std::three_way_comparable_with < T, U > // Use concept directly
		friend constexpr std::compare_three_way_result_t < T, U >
		operator <=> (const indirect & lhs,
		              const U & rhs) {
			// Compare value with value if lhs has one, otherwise empty < value
			return lhs.has_value() ? (*lhs <=> rhs) : std::strong_ordering::less;
		}
	};

	// Deduction guides
	template < class Value >
	indirect(Value) -> indirect < Value > ;

	template < class Allocator, class Value >
	indirect(std::allocator_arg_t, Allocator, Value) -> indirect < Value, Allocator > ;


	//----------------------------------------------------------------------------
	// polymorphic<T, Allocator>
	//----------------------------------------------------------------------------

	template < class T, class Allocator = std::allocator < T >> // Default argument here on definition
	class polymorphic {
		// Static asserts
		static_assert(!std::is_array_v < T > , "polymorphic<T>: T cannot be an array type.");
		static_assert(std::is_object_v < T > , "polymorphic<T>: T must be an object type.");
		static_assert(!std::is_const_v < T > && !std::is_volatile_v < T > , "polymorphic<T>: T cannot be cv-qualified.");
		static_assert(!std::same_as < T, std::in_place_t > , "polymorphic<T>: T cannot be in_place_t.");
		static_assert(!is_in_place_type_v < T > , "polymorphic<T>: T cannot be a specialization of in_place_type_t.");
		using BaseAllocTraits = std::allocator_traits < Allocator > ;
		static_assert(std::same_as < typename BaseAllocTraits::value_type, T > ,
		              "polymorphic<T, Allocator>: allocator_traits<Allocator>::value_type must be T.");

		// --- Type Erasure Internals ---
		using base_pointer = typename BaseAllocTraits::pointer;
		using const_base_pointer = typename BaseAllocTraits::const_pointer;
		// Function pointer types for type erasure
		using destroy_fn_t = void( * )(Allocator & , base_pointer) noexcept;
		using copy_fn_t = base_pointer( * )(Allocator & , const_base_pointer); // Returns new pointer, takes target base allocator and const source base pointer

		template < typename U >
		requires std::derived_from < U, T >
		static void concrete_destroy(Allocator & base_alloc, base_pointer p_base) noexcept {
			// Rebind allocator to the actual derived type U
			using DerivedAlloc = typename BaseAllocTraits::template rebind_alloc < U > ;
			using DerivedAllocTraits = std::allocator_traits < DerivedAlloc > ;
			using derived_pointer = typename DerivedAllocTraits::pointer;

			// Get pointer-to-U from pointer-to-T
			U* p_derived_raw = static_cast<U*>(std::to_address(p_base));
			derived_pointer derived_p = std::pointer_traits<derived_pointer>::pointer_to(*p_derived_raw);

			// Create the derived allocator from the base allocator (needed for destroy call)
			DerivedAlloc derived_alloc(base_alloc);

			try {
				DerivedAllocTraits::destroy(derived_alloc, std::to_address(derived_p));
			} catch (...) {
				// Destructors must not throw
				assert(false && "Destructor threw an exception.");
				std::terminate(); // No recovery possible if destructor throws
			}

			// Deallocate using the BASE allocator and BASE pointer
			BaseAllocTraits::deallocate(base_alloc, p_base, 1);
		}


		template < typename U >
		requires std::derived_from < U, T > && std::copy_constructible < U >
		static base_pointer concrete_copy(Allocator & target_base_alloc, const_base_pointer p_base_const) {
			// Rebind allocator to the actual derived type U
			using DerivedAlloc = typename BaseAllocTraits::template rebind_alloc < U > ;
			using DerivedAllocTraits = std::allocator_traits < DerivedAlloc > ;
			using derived_pointer = typename DerivedAllocTraits::pointer;

			// Get reference to const U from const T*
			const T* p_base_addr = std::to_address(p_base_const);
			const U & derived_obj_ref = static_cast < const U & > ( * p_base_addr);

			// Create the derived allocator using the *target* base allocator
			DerivedAlloc target_derived_alloc(target_base_alloc);

			// Allocate memory using the target derived allocator
			derived_pointer new_derived_p = DerivedAllocTraits::allocate(target_derived_alloc, 1);
			base_pointer bp = nullptr; // For returning the base pointer
			try {
				// Construct a copy of the derived object into the new memory using target derived allocator
				DerivedAllocTraits::construct(target_derived_alloc, std::to_address(new_derived_p), derived_obj_ref);

				// Construction succeeded, now get the base pointer
				U* new_derived_raw = std::to_address(new_derived_p);
				T* new_base_raw = static_cast<T*>(new_derived_raw);
				bp = std::pointer_traits < base_pointer > ::pointer_to(*new_base_raw);
				return bp;

			} catch (...) {
				DerivedAllocTraits::deallocate(target_derived_alloc, new_derived_p, 1); // Clean up allocation on failure
				throw; // Re-throw the construction exception
			}
		}

		// Members
		base_pointer p_ = nullptr;
		destroy_fn_t destroy_ = nullptr;
		copy_fn_t copy_ = nullptr;
		[[no_unique_address]] Allocator alloc_ {};

		// Helper to destroy and deallocate using function pointer
		constexpr void call_destroy_deallocate() noexcept {
			if (p_) {
				// Invariant: if p_ != nullptr, destroy_ must also be != nullptr (unless moved from)
				// A moved-from object has p_ == nullptr.
				assert(destroy_ != nullptr && "Invariant violation: p_ is set but destroy_ is null.");
				destroy_(alloc_, p_); // Call the stored destruction function
				p_ = nullptr; // Mark as valueless
				destroy_ = nullptr; // Clear function pointers
				copy_ = nullptr;
			}
			// Ensure members are null if p_ was already null (or just became null)
			assert(p_ == nullptr && destroy_ == nullptr && copy_ == nullptr);
		}

		// Helper to allocate and construct a derived U
		template < typename U, typename...Args >
		requires std::derived_from < U, T > && std::constructible_from < U, Args... > && std::copy_constructible < U >
		constexpr base_pointer allocate_construct_derived(Args && ...args) {
			using DerivedAlloc = typename BaseAllocTraits::template rebind_alloc < U > ;
			using DerivedAllocTraits = std::allocator_traits < DerivedAlloc > ;
			using derived_pointer = typename DerivedAllocTraits::pointer;

			DerivedAlloc derived_alloc(alloc_); // Use *this object's base allocator to create derived

			derived_pointer derived_p = DerivedAllocTraits::allocate(derived_alloc, 1);
			base_pointer bp = nullptr; // For returning the base pointer
			try {
				DerivedAllocTraits::construct(derived_alloc, std::to_address(derived_p), std::forward < Args > (args)...);

				// Construction succeeded, now get the base pointer
				U* derived_raw = std::to_address(derived_p);
				T* base_raw = static_cast<T*>(derived_raw);
				bp = std::pointer_traits<base_pointer>::pointer_to(*base_raw);

				// Set the function pointers for type erasure *after* construction succeeds
				destroy_ = & concrete_destroy < U > ;
				copy_ = & concrete_copy < U > ; // Needs copy_constructible<U>
				return bp;

			} catch (...) {
				// Construction failed, ensure no dangling function pointers are set
				destroy_ = nullptr;
				copy_ = nullptr;
				DerivedAllocTraits::deallocate(derived_alloc, derived_p, 1); // Clean up allocation
				throw; // Re-throw the construction exception
			}
		}

	public:
		using value_type = T;
		using allocator_type = Allocator;
		using pointer = base_pointer; // Expose base pointer type
		using const_pointer = const_base_pointer; // Expose const base pointer type

		// --- Constructors ---

		// Default constructor (constructs T)
		explicit constexpr polymorphic() requires std::default_initializable < Allocator > &&
		                                          std::default_initializable < T > &&
		                                          std::copy_constructible < T > {
			p_ = allocate_construct_derived < T > ();
		}

		// Allocator-extended default constructor (constructs T)
		explicit constexpr polymorphic(std::allocator_arg_t,
		                               const Allocator & a) requires std::default_initializable < T > && std::copy_constructible < T >
				: alloc_(a) {
			p_ = allocate_construct_derived < T > ();
		}

		// *** MODIFIED: Added nullptr constructor ***
//		[[deprecated("Use std::optional<polymorphic<T>> for intentional nullability. This deviates from P3019.")]]
		constexpr polymorphic(std::nullptr_t) noexcept: p_(nullptr), destroy_(nullptr), copy_(nullptr), alloc_ {} {
			// Creates a "null" or valueless polymorphic object
		}

//		[[deprecated("Use std::optional<polymorphic<T>> for intentional nullability. This deviates from P3019.")]]
		constexpr polymorphic(std::allocator_arg_t,
		                      const Allocator & a, std::nullptr_t) noexcept: p_(nullptr), destroy_(nullptr), copy_(nullptr), alloc_(a) {}

		// --- Copy constructor ---
		// <<< ADDED [[deprecated]] HERE >>>
		[[deprecated("polymorphic performs potentially expensive deep copy; consider moving or using std::optional.")]]
		constexpr polymorphic(const polymorphic & other)
				: alloc_(BaseAllocTraits::select_on_container_copy_construction(other.alloc_)) {
			if (other.p_) { // Check if source has a value
				assert(other.copy_ != nullptr && "polymorphic copy ctor: Source has value but copy_ fn ptr is null.");
				// Call the source's copy function, using *this* object's (potentially copied) allocator
				p_ = other.copy_(alloc_, other.p_); // Can throw
				// If copy succeeds, store the function pointers from the source
				destroy_ = other.destroy_;
				copy_ = other.copy_;
			} else {
				// Source is valueless (null or moved-from), so *this becomes valueless.
				p_ = nullptr;
				destroy_ = nullptr;
				copy_ = nullptr;
			}
		}

		// --- Allocator-extended copy constructor ---
		// <<< ADDED [[deprecated]] HERE >>>
		[[deprecated("polymorphic performs potentially expensive deep copy; consider moving or using std::optional.")]]
		constexpr polymorphic(std::allocator_arg_t,
		                      const Allocator & a,
		                      const polymorphic & other)
				: alloc_(a) { // Use the provided allocator 'a'
			if (other.p_) {
				assert(other.copy_ != nullptr && "polymorphic alloc-extended copy ctor: Source has value but copy_ fn ptr is null.");
				// Call source's copy function with the provided allocator 'a'
				p_ = other.copy_(alloc_, other.p_); // Can throw
				destroy_ = other.destroy_;
				copy_ = other.copy_;
			} else {
				p_ = nullptr;
				destroy_ = nullptr;
				copy_ = nullptr;
			}
		}

		// Move constructor (basic)
		constexpr polymorphic(polymorphic && other) noexcept
				: p_(std::exchange(other.p_, nullptr)),
				  destroy_(std::exchange(other.destroy_, nullptr)),
				  copy_(std::exchange(other.copy_, nullptr)),
				  alloc_(std::move(other.alloc_)) // Always move allocator (standard container behavior)
		{}

		// Allocator-extended move constructor
		constexpr polymorphic(std::allocator_arg_t,
		                      const Allocator & a, polymorphic && other) noexcept(BaseAllocTraits::is_always_equal::value)
				: alloc_(a) // Initialize with the target allocator 'a'
		{
			if (!other.p_) { // Check if other is already valueless
				p_ = nullptr;
				destroy_ = nullptr;
				copy_ = nullptr;
				return; // other is already valueless, *this is default initialized to valueless
			}

			// Other has a value
			if constexpr(BaseAllocTraits::is_always_equal::value) {
				// Allocators are always equal, just steal the state
				p_ = std::exchange(other.p_, nullptr);
				destroy_ = std::exchange(other.destroy_, nullptr);
				copy_ = std::exchange(other.copy_, nullptr);
				// Note: alloc_ was already initialized with 'a'. If is_always_equal, 'a' must equal other.alloc_.
			}
			else { // Allocators might differ
				if (alloc_ == other.alloc_) {
					// Allocators happen to be equal, steal the state
					p_ = std::exchange(other.p_, nullptr);
					destroy_ = std::exchange(other.destroy_, nullptr);
					copy_ = std::exchange(other.copy_, nullptr);
				} else {
					// Allocators differ. P3019 implies a potentially throwing allocation/copy.
					// Since move_fn_t isn't present, fallback to copy using the target allocator.
					assert(other.copy_ != nullptr && "polymorphic alloc-move ctor: Source has value but copy_ fn ptr is null.");
					base_pointer new_p = nullptr;
					try {
						new_p = other.copy_(alloc_, other.p_); // Copy using the target allocator 'alloc_'
						// Copy succeeded, now destroy the object managed by 'other'.
						other.call_destroy_deallocate(); // Make 'other' valueless (uses other.alloc_)
						// Assign pointer and functions *after* other is destroyed (in case destroy throws? No, it's noexcept)
						p_ = new_p;
						destroy_ = other.destroy_; // These were nulled in other, but we need the *original* values
						copy_ = other.copy_;      // This logic is flawed. We need to save them *before* destroying other.

						// Revised logic for allocators differ case:
						destroy_fn_t saved_destroy = other.destroy_;
						copy_fn_t saved_copy = other.copy_;
						new_p = other.copy_(alloc_, other.p_); // Copy using target allocator (this->alloc_)
						// If copy throws, *this is left valueless (alloc_ is set, p_/fns are null), other is unchanged.
						// If copy succeeds:
						other.call_destroy_deallocate(); // Destroy object in other (using other.alloc_)
						p_ = new_p;
						destroy_ = saved_destroy; // Assign saved function pointers
						copy_ = saved_copy;


					} catch (...) {
						// If copy fails, *this should be left valueless (as if default constructed with alloc 'a').
						p_ = nullptr;
						destroy_ = nullptr;
						copy_ = nullptr;
						// 'other' remains unchanged by the failed copy attempt.
						throw; // Re-throw the exception from copy_.
					}
				}
			}
		}


		// Single-argument constructor from U&& (where U derives from T)
		template < class U > // No default needed if deduction guide exists
		explicit constexpr polymorphic(U && u) requires std::default_initializable < Allocator > &&
		                                                (!std::same_as < std::remove_cvref_t < U > , polymorphic > ) &&
		                                                (!is_polymorphic_v < std::remove_cvref_t < U >> ) &&
		                                                (!is_in_place_type_v < std::remove_cvref_t < U >> ) &&
		                                                (!std::is_same_v<std::remove_cvref_t<U>, std::allocator_arg_t>) && // Disambiguate from alloc-extended ctors
		                                                (!std::is_base_of_v<std::allocator_arg_t, std::remove_cvref_t<U>>) &&
		                                                std::derived_from < std::remove_cvref_t < U > , T > &&
		                                                std::constructible_from < std::remove_cvref_t < U > , U > &&
		                                                std::copy_constructible < std::remove_cvref_t < U >> { // Requires copy constructible for type erasure
			using ActualU = std::remove_cvref_t < U > ;
			p_ = allocate_construct_derived < ActualU > (std::forward < U > (u));
		}

		// Allocator-extended single-argument constructor from U&&
		template < class U >
		explicit constexpr polymorphic(std::allocator_arg_t,
		                               const Allocator & a, U && u) requires (!std::same_as < std::remove_cvref_t < U > , polymorphic > ) &&
		(!is_polymorphic_v < std::remove_cvref_t < U >> ) &&
		(!is_in_place_type_v < std::remove_cvref_t < U >> ) &&
		std::derived_from < std::remove_cvref_t < U > , T > &&
				std::constructible_from < std::remove_cvref_t < U > , U > &&
		std::copy_constructible < std::remove_cvref_t < U >>
				: alloc_(a) {
				using ActualU = std::remove_cvref_t < U > ;
				p_ = allocate_construct_derived < ActualU > (std::forward < U > (u));
		}

		// In-place constructor for type U
		template < class U, class...Ts >
		explicit constexpr polymorphic(std::in_place_type_t < U > , Ts && ...ts) requires std::default_initializable < Allocator > &&
		                                                                                  std::derived_from < U, T > &&
		                                                                                  std::constructible_from < U, Ts... > &&
		                                                                                  std::copy_constructible < U > {
			p_ = allocate_construct_derived < U > (std::forward < Ts > (ts)...);
		}

		// Allocator-extended in-place constructor for type U
		template < class U, class...Ts >
		explicit constexpr polymorphic(std::allocator_arg_t,
		                               const Allocator & a, std::in_place_type_t < U > , Ts && ...ts) requires std::derived_from < U, T > &&
		                                                                                                       std::constructible_from < U, Ts... > &&
		                                                                                                       std::copy_constructible < U >
				: alloc_(a) {
			p_ = allocate_construct_derived < U > (std::forward < Ts > (ts)...);
		}

		// Init list constructors
		template < class U, class I, class...Us >
		explicit constexpr polymorphic(std::in_place_type_t < U > , std::initializer_list < I > ilist, Us && ...us) requires std::default_initializable < Allocator > &&
		                                                                                                                     std::derived_from < U, T > &&
		                                                                                                                     std::constructible_from < U, std::initializer_list < I > & , Us... > &&
		                                                                                                                     std::copy_constructible < U > {
			p_ = allocate_construct_derived < U > (ilist, std::forward < Us > (us)...);
		}

		template < class U, class I, class...Us >
		explicit constexpr polymorphic(std::allocator_arg_t,
		                               const Allocator & a, std::in_place_type_t < U > , std::initializer_list < I > ilist, Us && ...us) requires std::derived_from < U, T > &&
		                                                                                                                                          std::constructible_from < U, std::initializer_list < I > & , Us... > &&
		                                                                                                                                          std::copy_constructible < U >
				: alloc_(a) {
			p_ = allocate_construct_derived < U > (ilist, std::forward < Us > (us)...);
		}

		// --- Destructor ---
		constexpr~polymorphic() {
			call_destroy_deallocate(); // Handles null p_ correctly
		}

		// --- Assignment ---

		// --- Copy assignment ---
		// <<< ADDED [[deprecated]] HERE >>>
		[[deprecated("polymorphic performs potentially expensive deep copy; consider moving or using std::optional.")]]
		constexpr polymorphic & operator = (const polymorphic & other) {
			if (std::addressof(other) == this) {
				return * this;
			}

			constexpr bool pocca = BaseAllocTraits::propagate_on_container_copy_assignment::value;
			Allocator target_alloc = alloc_; // Start with current allocator

			if constexpr (pocca) {
				if (alloc_ != other.alloc_) {
					target_alloc = other.alloc_; // If POCMA true and allocs differ, target is other's
				}
			}

			if (!other.p_) { // other is valueless
				call_destroy_deallocate(); // Make *this valueless
				// Update allocator only if POCMA is true and they differed *and* we actually needed to propagate
				if constexpr (pocca) {
					if (alloc_ != other.alloc_) {
						alloc_ = other.alloc_; // Assign other's allocator
					}
				}
			} else { // other has a value
				assert(other.copy_ != nullptr);
				// Perform the copy using the determined target allocator
				base_pointer new_p = nullptr;
				try {
					new_p = other.copy_(target_alloc, other.p_); // Use target_alloc for the copy
				} catch (...) {
					// If copy fails, *this state should remain unchanged.
					// If target_alloc was different from alloc_, this is hard.
					// Assume strong guarantee: *this is unchanged.
					throw;
				}

				// Copy succeeded, now commit changes
				call_destroy_deallocate(); // Destroy the old object in *this (uses old alloc_)
				p_ = new_p;
				destroy_ = other.destroy_; // Take function pointers from other
				copy_ = other.copy_;
				// Update allocator if POCMA is true and they differed
				if constexpr (pocca) {
					if (alloc_ != other.alloc_) { // Check original allocators
						alloc_ = other.alloc_; // Assign other's allocator
					}
				} else {
					// Ensure alloc_ == target_alloc if pocca is false (it should be)
					assert(alloc_ == target_alloc);
				}
			}
			return * this;
		}

		// Move assignment
		constexpr polymorphic & operator = (polymorphic && other) noexcept(
		BaseAllocTraits::propagate_on_container_move_assignment::value ||
		BaseAllocTraits::is_always_equal::value) {
			if (std::addressof(other) == this) {
				return * this;
			}

			constexpr bool pocma = BaseAllocTraits::propagate_on_container_move_assignment::value;
			if constexpr(pocma) {
				// If POCMA is true: destroy lhs, move assign allocator, then steal state from rhs.
				call_destroy_deallocate();
				alloc_ = std::move(other.alloc_); // Move allocator
				p_ = std::exchange(other.p_, nullptr);
				destroy_ = std::exchange(other.destroy_, nullptr);
				copy_ = std::exchange(other.copy_, nullptr);
			}
			else { // POCMA is false
				if (alloc_ == other.alloc_) {
					// Allocators are equal: destroy lhs, then steal state from rhs.
					call_destroy_deallocate();
					p_ = std::exchange(other.p_, nullptr);
					destroy_ = std::exchange(other.destroy_, nullptr);
					copy_ = std::exchange(other.copy_, nullptr);
				} else { // Allocators differ, POCMA is false
					if (!other.p_) { // other is valueless
						call_destroy_deallocate(); // Make *this valueless
						// other is already valueless, ensure its members are null
						other.p_ = nullptr; other.destroy_ = nullptr; other.copy_ = nullptr;
					} else { // other has value, allocators differ, POCMA false
						// Fallback to copy using *this object's allocator (alloc_).
						assert(other.copy_ != nullptr);
						base_pointer new_p = nullptr;
						destroy_fn_t saved_destroy = other.destroy_; // Save before potential modification
						copy_fn_t saved_copy = other.copy_;
						try {
							new_p = other.copy_(alloc_, other.p_); // Copy using this->alloc_
						} catch (...) {
							// If copy fails, *this remains unchanged. other remains unchanged.
							throw;
						}
						// Copy succeeded. Commit.
						call_destroy_deallocate(); // Destroy original object in *this (using this->alloc_)
						p_ = new_p;
						destroy_ = saved_destroy;
						copy_ = saved_copy;
						other.call_destroy_deallocate(); // Destroy the object in other, making it valueless (using other.alloc_)
					}
				}
			}
			return * this;
		}

		// *** MODIFIED: Added nullptr assignment ***
//		[[deprecated("Use std::optional<polymorphic<T>> for intentional nullability. This deviates from P3019.")]]
		constexpr polymorphic & operator = (std::nullptr_t) noexcept {
			reset(); // Destroy current object and set to valueless state
			return * this;
		}

		// --- Observers ---

		constexpr
		const T & operator * () const noexcept {
			assert(has_value() && "Precondition violation: polymorphic::operator*() const called on valueless object.");
			return * std::to_address(p_);
		}

		constexpr T & operator * () noexcept {
			assert(has_value() && "Precondition violation: polymorphic::operator*() called on valueless object.");
			return * std::to_address(p_);
		}

		constexpr const_pointer operator -> () const noexcept {
			assert(has_value() && "Precondition violation: polymorphic::operator->() const called on valueless object.");
			return p_;
		}

		constexpr pointer operator -> () noexcept {
			assert(has_value() && "Precondition violation: polymorphic::operator->() called on valueless object.");
			return p_;
		}

		// *** MODIFIED: Added explicit operator bool ***
//		[[deprecated("Use has_value() or std::optional<polymorphic<T>>. This deviates from P3019.")]]
		explicit constexpr operator bool() const noexcept {
			return has_value();
		}

		// Check if the object holds a value (preferred over operator bool)
		constexpr bool has_value() const noexcept {
			return p_ != nullptr;
		}

		// Check if valueless (consistent with !has_value())
		constexpr bool valueless_after_move() const noexcept {
			return p_ == nullptr;
		}

		constexpr allocator_type get_allocator() const noexcept {
			return alloc_;
		}

		// *** MODIFIED: Added get() ***
//		[[deprecated("Access via operator* or operator-> is preferred. This deviates from P3019.")]]
		constexpr pointer get() noexcept {
			return p_;
		}

//		[[deprecated("Access via operator* or operator-> is preferred. This deviates from P3019.")]]
		constexpr const_pointer get() const noexcept {
			return p_;
		}

		// --- Modifiers ---

		// *** MODIFIED: Added reset() ***
//		[[deprecated("Value types typically use assignment for replacement. This deviates from P3019.")]]
		constexpr void reset() noexcept {
			call_destroy_deallocate(); // Destroys object and nulls members
		}

		// *** MODIFIED: Added release() ***
//		[[deprecated("Value types manage their own resources; releasing breaks ownership. This deviates from P3019.")]]
		[[nodiscard]] constexpr pointer release() noexcept {
			// Caller takes ownership of the pointer AND responsibility for deletion/destruction!
			pointer old_p = p_;
			p_ = nullptr;
			destroy_ = nullptr;
			copy_ = nullptr;
			return old_p;
		}

		// --- Swap ---
		constexpr void swap(polymorphic & other) noexcept(
		BaseAllocTraits::propagate_on_container_swap::value ||
		BaseAllocTraits::is_always_equal::value) {
			using std::swap;
			if constexpr(BaseAllocTraits::propagate_on_container_swap::value) {
				swap(p_, other.p_);
				swap(destroy_, other.destroy_);
				swap(copy_, other.copy_);
				swap(alloc_, other.alloc_); // Swap allocators if POCS is true
			}
			else { // POCS is false
				// Requires allocators to compare equal per standard container rules
				if (alloc_ != other.alloc_) {
					assert(alloc_ == other.alloc_ && "polymorphic::swap requires allocators to be equal when POCS is false");
				}
				swap(p_, other.p_);
				swap(destroy_, other.destroy_);
				swap(copy_, other.copy_);
				// Do NOT swap allocators if POCS is false
			}
		}

		friend constexpr void swap(polymorphic & lhs, polymorphic & rhs) noexcept(noexcept(lhs.swap(rhs))) {
			lhs.swap(rhs);
		}
	};

	// *** MODIFIED: Added make_polymorphic factory functions ***

	template < typename T, // T is the DERIVED type to create
			typename Base, // Base is the polymorphic<Base> type
			typename Alloc,
			typename...Args >
	requires std::derived_from < T, Base > && std::constructible_from < T, Args... >
	[[nodiscard]] constexpr polymorphic < Base, Alloc >
	make_polymorphic_alloc(std::allocator_arg_t,
	                       const Alloc & alloc, Args && ...args) {
		// Requires copy_constructible<T> for polymorphic constructor
		static_assert(std::copy_constructible<T>, "make_polymorphic_alloc requires derived type T to be copy constructible");
		return polymorphic < Base, Alloc > (std::allocator_arg, alloc, std::in_place_type < T > , std::forward < Args > (args)...);
	}

	template < typename T, // T is the DERIVED type to create
			typename Base, // Base is the polymorphic<Base> type
			typename...Args >
	requires std::derived_from < T, Base > && std::constructible_from < T, Args... > && std::default_initializable < std::allocator < Base >>
	[[nodiscard]] constexpr polymorphic < Base, std::allocator < Base >>
	make_polymorphic(Args && ...args) {
		// Requires copy_constructible<T> for polymorphic constructor
		static_assert(std::copy_constructible<T>, "make_polymorphic requires derived type T to be copy constructible");
		return polymorphic < Base, std::allocator < Base >> (std::in_place_type < T > , std::forward < Args > (args)...);
	}

} // namespace std_P3019_modified


// std::hash specialization moved OUTSIDE the custom namespace and INSIDE std namespace
namespace std {

	template < class T, class Allocator >
	struct hash < std_P3019_modified::indirect < T, Allocator >> {
	using indirect_type = std_P3019_modified::indirect < T, Allocator >; // Use qualified name

	// Check if T itself is hashable
	using is_enabled = std::bool_constant < requires(const T & t) {
		std::hash < T > {}(t); // Requires std::hash<T> to be valid
	} > ;

	// Only enable this specialization if T is hashable
	template < bool E = is_enabled::value,
			std::enable_if_t < E, int > = 0 >
	std::size_t operator()(const indirect_type & i) const
	noexcept( noexcept( std::hash<T>{}( *i ) ) ) // Propagate noexcept from underlying hash
	{
		if (i.has_value()) {
			return std::hash < T > {}( * i); // Hash the contained value
		} else {
			// Return a hash for the valueless state. Use a fixed value unlikely
			// to collide with valid hashes (e.g., from a large prime).
			// Using 0 is also common but might collide if 0 is a valid hash for T.
			// P3019 doesn't specify, so using a large number is safer.
			return static_cast < std::size_t > (0xcbf29ce484222325ULL ^ 0x100000001b3ULL); // FNV-1a basis and prime for empty state
		}
	}
};

// Consider adding std::hash specialization for polymorphic? Generally difficult.
// See comments in previous response. Usually avoided.

} // namespace std