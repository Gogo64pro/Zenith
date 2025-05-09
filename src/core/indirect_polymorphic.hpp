//
// Created by gogop on 4/30/2025.
//

#pragma once
#define DEBUG true
#include <memory> // For unique_ptr, allocator_traits, pointer_traits
#include <utility> // For move, exchange, forward, pair
#include <type_traits> // For standard type traits
#include <concepts> // For concepts
#include <functional> // For hash (potentially)
#include <compare> // Needed for <=>
#include <new> // For placement new if needed, allocation functions
#include <cassert> // For assert
#include <stdexcept> // For exceptions
#include <initializer_list> // For initializer_list constructors
#include <optional> // For comparison (not used directly)
#include <string> // For deprecation messages (potentially)
#include <typeinfo> // For typeid used in control block and target()
#include <iostream> // For DEBUG

// Forward declarations within a modified namespace
namespace std_P3019_modified {

	// Forward declarations WITHOUT default template arguments
	template <class T, class Allocator>
	class polymorphic;
	template <class T, class Allocator>
	class indirect;

	//----------------------------------------------------------------------------
	// Type Traits (Unchanged)
	//----------------------------------------------------------------------------

	template <typename>
	struct is_indirect: std::false_type {};
	template <typename T, typename A>
	struct is_indirect<indirect<T, A>>: std::true_type {};
	template <typename T>
	inline constexpr bool is_indirect_v = is_indirect<T>::value;

	template <typename>
	struct is_polymorphic: std::false_type {};
	template <typename T, typename A>
	struct is_polymorphic<polymorphic<T, A>>: std::true_type {};
	template <typename T>
	inline constexpr bool is_polymorphic_v = is_polymorphic<T>::value;

	template <typename>
	struct is_in_place_type: std::false_type {};
	template <typename T>
	struct is_in_place_type<std::in_place_type_t<T>>: std::true_type {};
	template <typename T>
	inline constexpr bool is_in_place_type_v = is_in_place_type<T>::value;

	//----------------------------------------------------------------------------
	// indirect<T, Allocator> (Unchanged from your provided version)
	//----------------------------------------------------------------------------

	template <class T, class Allocator = std::allocator<T>>
	class indirect {
		// Static asserts...
		static_assert(!std::is_array_v<T>, "indirect<T>: T cannot be an array type.");
		static_assert(std::is_object_v<T>, "indirect<T>: T must be an object type.");
		static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>, "indirect<T>: T cannot be cv-qualified.");
		static_assert(!std::same_as<T, std::in_place_t>, "indirect<T>: T cannot be in_place_t.");
		static_assert(!is_in_place_type_v<T>, "indirect<T>: T cannot be a specialization of in_place_type_t.");

		using AllocTraits = std::allocator_traits<Allocator>;
		static_assert(std::same_as<typename AllocTraits::value_type, T>,
		              "indirect<T, Allocator>: allocator_traits<Allocator>::value_type must be T.");

	public:
		using value_type = T;
		using allocator_type = Allocator;
		using pointer = typename AllocTraits::pointer;
		using const_pointer = typename AllocTraits::const_pointer;

	private:
		pointer p_ = nullptr;
		[[no_unique_address]] Allocator alloc_{};

		constexpr void destroy_deallocate() noexcept {
			if (p_) {
				try {
					AllocTraits::destroy(alloc_, std::to_address(p_));
				} catch (...) {} // Destructors should generally not throw
				AllocTraits::deallocate(alloc_, p_, 1);
				p_ = nullptr;
			}
		}

		template <typename...Args>
		requires std::constructible_from<T, Args...>
		constexpr pointer allocate_construct(Args&&...args) {
			pointer ptr = AllocTraits::allocate(alloc_, 1);
			try {
				AllocTraits::construct(alloc_, std::to_address(ptr), std::forward<Args>(args)...);
				return ptr;
			} catch (...) {
				AllocTraits::deallocate(alloc_, ptr, 1);
				throw;
			}
		}

		[[nodiscard]] constexpr pointer release() noexcept {
			pointer old_p = p_;
			p_ = nullptr;
			return old_p;
		}

	public:
		// Constructors (Unchanged)
		explicit constexpr indirect() requires std::default_initializable<Allocator> && std::default_initializable<T> {
			p_ = allocate_construct();
		}

		explicit constexpr indirect(std::allocator_arg_t,
		                            const Allocator& a) requires std::default_initializable<T>
				: alloc_(a) {
			p_ = allocate_construct();
		}

		constexpr indirect(const indirect& other) requires std::copy_constructible<T>
				: alloc_(AllocTraits::select_on_container_copy_construction(other.alloc_)) {
			if (!other.valueless_after_move()) {
				p_ = allocate_construct(*other);
			}
		}

		constexpr indirect(std::allocator_arg_t,
		                   const Allocator& a,
		                   const indirect& other) requires std::copy_constructible<T>
				: alloc_(a) {
			if (!other.valueless_after_move()) {
				p_ = allocate_construct(*other);
			}
		}

		constexpr indirect(indirect&& other) noexcept: p_(std::exchange(other.p_, nullptr)),
		                                               alloc_(std::move(other.alloc_)) {}

		constexpr indirect(std::allocator_arg_t,
		                   const Allocator& a, indirect&& other) noexcept(AllocTraits::is_always_equal::value): alloc_(a) {
			if (other.valueless_after_move()) return;

			if constexpr(AllocTraits::is_always_equal::value) {
				p_ = std::exchange(other.p_, nullptr);
			}
			else {
				if (alloc_ == other.alloc_) {
					p_ = std::exchange(other.p_, nullptr);
				} else {
					pointer new_p = nullptr;
					try {
						new_p = allocate_construct(std::move(*other));
					} catch (...) {
						throw;
					}
					other.destroy_deallocate();
					p_ = new_p;
				}
			}
		}

		template <class U = T>
		explicit constexpr indirect(U&& u) requires std::default_initializable<Allocator> &&
		                                            (!std::same_as<std::remove_cvref_t<U>, indirect>) &&
		                                            (!std::same_as<std::remove_cvref_t<U>, std::in_place_t>) &&
		                                            (!is_indirect_v<std::remove_cvref_t<U>>) &&
		                                            (!is_in_place_type_v<std::remove_cvref_t<U>>) &&
		                                            (!std::is_same_v<std::remove_cvref_t<U>, std::allocator_arg_t>) &&
		                                            (!std::is_base_of_v<std::allocator_arg_t, std::remove_cvref_t<U>>) &&
		                                            std::constructible_from<T, U> {
			p_ = allocate_construct(std::forward<U>(u));
		}

		template <class U = T>
		explicit constexpr indirect(std::allocator_arg_t,
		                            const Allocator& a, U&& u) requires(!std::same_as<std::remove_cvref_t<U>, indirect>) &&
		(!std::same_as<std::remove_cvref_t<U>, std::in_place_t>) &&
		(!is_indirect_v<std::remove_cvref_t<U>>) &&
		(!is_in_place_type_v<std::remove_cvref_t<U>>) &&
		std::constructible_from<T, U>
				: alloc_(a) {
				p_ = allocate_construct(std::forward<U>(u));
		}

		template <class...Us>
		explicit constexpr indirect(std::in_place_t, Us&&...us) requires std::default_initializable<Allocator> &&
		                                                                 std::constructible_from<T, Us...> {
			p_ = allocate_construct(std::forward<Us>(us)...);
		}

		template <class...Us>
		explicit constexpr indirect(std::allocator_arg_t,
		                            const Allocator& a, std::in_place_t, Us&&...us) requires std::constructible_from<T, Us...>
				: alloc_(a) {
			p_ = allocate_construct(std::forward<Us>(us)...);
		}

		template <class I, class...Us>
		explicit constexpr indirect(std::in_place_t, std::initializer_list<I> ilist, Us&&...us) requires std::default_initializable<Allocator> &&
		                                                                                                 std::constructible_from<T, std::initializer_list<I>&, Us...> {
			p_ = allocate_construct(ilist, std::forward<Us>(us)...);
		}

		template <class I, class...Us>
		explicit constexpr indirect(std::allocator_arg_t,
		                            const Allocator& a, std::in_place_t, std::initializer_list<I> ilist, Us&&...us) requires std::constructible_from<T, std::initializer_list<I>&, Us...>
				: alloc_(a) {
			p_ = allocate_construct(ilist, std::forward<Us>(us)...);
		}

		constexpr ~indirect() {
			destroy_deallocate();
		}

		// Assignment (Unchanged)
		constexpr indirect& operator=(const indirect& other) requires std::copy_constructible<T> && std::is_copy_assignable_v<T> {
			if (std::addressof(other) == this) return *this;
			constexpr bool pocca = AllocTraits::propagate_on_container_copy_assignment::value;
			if constexpr(pocca) {
				if (alloc_ != other.alloc_) {
					if (other.valueless_after_move()) {
						destroy_deallocate();
						alloc_ = other.alloc_;
					} else {
						Allocator target_alloc = other.alloc_;
						pointer new_p = nullptr;
						try {
							using TempAllocTraits = std::allocator_traits<Allocator>;
							new_p = TempAllocTraits::allocate(target_alloc, 1);
							try {
								TempAllocTraits::construct(target_alloc, std::to_address(new_p), *other);
							} catch(...) {
								TempAllocTraits::deallocate(target_alloc, new_p, 1);
								throw;
							}
						} catch (...) {
							throw;
						}
						destroy_deallocate();
						alloc_ = std::move(target_alloc);
						p_ = new_p;
					}
					return *this;
				}
			}
			if (other.valueless_after_move()) {
				destroy_deallocate();
			} else {
				if (!valueless_after_move()) {
					*(*this) = *other;
				} else {
					pointer new_p = nullptr;
					try {
						new_p = allocate_construct(*other);
					} catch(...) {
						throw;
					}
					p_ = new_p;
				}
			}
			return *this;
		}

		constexpr indirect& operator=(indirect&& other) noexcept(
		AllocTraits::propagate_on_container_move_assignment::value ||
		AllocTraits::is_always_equal::value) {
			if (std::addressof(other) == this) return *this;
			constexpr bool pocma = AllocTraits::propagate_on_container_move_assignment::value;
			if constexpr(pocma) {
				destroy_deallocate();
				alloc_ = std::move(other.alloc_);
				p_ = std::exchange(other.p_, nullptr);
			}
			else {
				if (alloc_ == other.alloc_) {
					destroy_deallocate();
					p_ = std::exchange(other.p_, nullptr);
				} else {
					if (other.valueless_after_move()) {
						destroy_deallocate();
					} else {
						if (!valueless_after_move()) {
							if constexpr (std::is_nothrow_move_assignable_v<T>) {
								*(*this) = std::move(*other);
								other.destroy_deallocate();
							} else {
								pointer new_p = nullptr;
								try {
									new_p = allocate_construct(std::move(*other));
								} catch (...) {
									throw;
								}
								destroy_deallocate();
								p_ = new_p;
								other.destroy_deallocate();
							}
						} else {
							pointer new_p = nullptr;
							try {
								new_p = allocate_construct(std::move(*other));
							} catch (...) {
								throw;
							}
							p_ = new_p;
							other.destroy_deallocate();
						}
					}
				}
			}
			return *this;
		}

		template <class U = T> requires(!std::same_as<std::remove_cvref_t<U>, indirect>) &&
		(!is_indirect_v<std::remove_cvref_t<U>>) &&
		(!is_in_place_type_v<std::remove_cvref_t<U>>) &&
		std::constructible_from<T, U> &&
				std::assignable_from<T&, U>
		constexpr indirect& operator=(U&& u) {
			if (valueless_after_move()) {
				pointer new_p = nullptr;
				try {
					new_p = allocate_construct(std::forward<U>(u));
				} catch (...) {
					throw;
				}
				p_ = new_p;
			} else {
				*(*this) = std::forward<U>(u);
			}
			return *this;
		}

		// Observers (Unchanged)
		constexpr const T& operator*() const& noexcept { assert(!valueless_after_move()); return *std::to_address(p_); }
		constexpr T& operator*()& noexcept { assert(!valueless_after_move()); return *std::to_address(p_); }
		constexpr const T&& operator*() const&& noexcept { assert(!valueless_after_move()); return std::move(*std::to_address(p_)); }
		constexpr T&& operator*() && noexcept { assert(!valueless_after_move()); return std::move(*std::to_address(p_)); }
		constexpr const_pointer operator->() const noexcept { assert(!valueless_after_move()); return p_; }
		constexpr pointer operator->() noexcept { assert(!valueless_after_move()); return p_; }
		constexpr bool valueless_after_move() const noexcept { return p_ == nullptr; }
		constexpr bool has_value() const noexcept { return p_ != nullptr; }
		constexpr allocator_type get_allocator() const noexcept { return alloc_; }

		// Swap (Unchanged)
		constexpr void swap(indirect& other) noexcept( AllocTraits::propagate_on_container_swap::value || AllocTraits::is_always_equal::value) {
			if constexpr(AllocTraits::propagate_on_container_swap::value) {
				std::swap(p_, other.p_);
				std::swap(alloc_, other.alloc_);
			}
			else {
				assert(alloc_ == other.alloc_ && "indirect::swap requires allocators to be equal when POCS is false");
				std::swap(p_, other.p_);
			}
		}
		friend constexpr void swap(indirect& lhs, indirect& rhs) noexcept(noexcept(lhs.swap(rhs))) { lhs.swap(rhs); }

		// Comparisons (Unchanged)
		template <class U, class AA> requires requires(const T& t, const U& u) { { t == u } -> std::convertible_to<bool>; }
		friend constexpr bool operator==(const indirect& lhs, const indirect<U, AA>& rhs) {
			const bool lhs_has_value = lhs.has_value();
			const bool rhs_has_value = rhs.has_value();
			if (lhs_has_value != rhs_has_value) return false;
			if (!lhs_has_value) return true;
			return *lhs == *rhs;
		}

		template <class U, class AA> requires std::three_way_comparable_with<T, U>
		friend constexpr std::compare_three_way_result_t<T, U> operator<=>(const indirect& lhs, const indirect<U, AA>& rhs) {
			const bool lhs_has_value = lhs.has_value();
			const bool rhs_has_value = rhs.has_value();
			if (lhs_has_value && rhs_has_value) return *lhs <=> *rhs;
			if (!lhs_has_value && !rhs_has_value) return std::strong_ordering::equal;
			if (!lhs_has_value) return std::strong_ordering::less;
			return std::strong_ordering::greater;
		}
		template <class U> requires(!is_indirect_v<std::remove_cvref_t<U>>) && requires( const T& t, const U& u ) { { t == u } -> std::convertible_to<bool>; }
		friend constexpr bool operator==(const indirect& lhs, const U& rhs) { return lhs.has_value() ? (*lhs == rhs) : false; }
		template <class U> requires(!is_indirect_v<std::remove_cvref_t<U>>) && std::three_way_comparable_with<T, U>
		friend constexpr std::compare_three_way_result_t<T, U> operator<=>(const indirect& lhs, const U& rhs) { return lhs.has_value() ? (*lhs <=> rhs) : std::strong_ordering::less; }
	};

	// Deduction guides (Unchanged)
	template <class Value> indirect(Value) -> indirect<Value>;
	template <class Allocator, class Value> indirect(std::allocator_arg_t, Allocator, Value) -> indirect<Value, Allocator>;

	//----------------------------------------------------------------------------
	// polymorphic<T, Allocator> (Control Block Implementation)
	//----------------------------------------------------------------------------

	namespace detail {
		// Simplified ControlBlockBase
		template<typename BaseT, typename BaseAlloc>
		struct ControlBlockBase {
			using BasePointer = typename std::allocator_traits<BaseAlloc>::pointer;
			virtual ~ControlBlockBase() = default;
			virtual BaseT* get_ptr() noexcept = 0;
			virtual ControlBlockBase<BaseT, BaseAlloc>* clone(const BaseAlloc& target_cb_alloc) const = 0;
			virtual BasePointer copy(const BaseAlloc& target_alloc, BasePointer p) const = 0;
			virtual void destroy(BaseAlloc& alloc, BasePointer p) noexcept = 0;
			[[nodiscard]] virtual const std::type_info& target_type_info() const noexcept = 0;
		};

		// Simplified ControlBlockDerived
		template<typename ConcreteDerived, typename BaseT, typename BaseAlloc>
		struct ControlBlockDerived final : ControlBlockBase<BaseT, BaseAlloc> {
			using BasePointer = typename ControlBlockBase<BaseT, BaseAlloc>::BasePointer;
			ConcreteDerived derived;

			template<typename... Args>
			explicit ControlBlockDerived(Args&&... args) : derived(std::forward<Args>(args)...) {}

			BaseT* get_ptr() noexcept override { return &derived; }

			ControlBlockBase<BaseT, BaseAlloc>* clone(const BaseAlloc& target_cb_alloc) const override {
				using CBDerivedAlloc = typename std::allocator_traits<BaseAlloc>::template rebind_alloc<ControlBlockDerived>;
				using CBDerivedAllocTraits = std::allocator_traits<CBDerivedAlloc>;
				CBDerivedAlloc cb_alloc(target_cb_alloc);

				auto* cb_ptr = CBDerivedAllocTraits::allocate(cb_alloc, 1);
				try {
					CBDerivedAllocTraits::construct(cb_alloc, cb_ptr, derived);
					return cb_ptr;
				} catch (...) {
					CBDerivedAllocTraits::deallocate(cb_alloc, cb_ptr, 1);
					throw;
				}
			}
			BasePointer copy(const BaseAlloc& target_alloc, BasePointer p) const override {
				using DerivedAlloc = typename std::allocator_traits<BaseAlloc>::template rebind_alloc<ConcreteDerived>;
				using DerivedAllocTraits = std::allocator_traits<DerivedAlloc>;

				DerivedAlloc derived_alloc(target_alloc);
				auto* new_p = DerivedAllocTraits::allocate(derived_alloc, 1);

				try {
					DerivedAllocTraits::construct(derived_alloc, new_p,
					                              *static_cast<ConcreteDerived*>(std::to_address(p)));
					return std::pointer_traits<BasePointer>::pointer_to(
							*static_cast<BaseT*>(new_p));
				} catch (...) {
					DerivedAllocTraits::deallocate(derived_alloc, new_p, 1);
					throw;
				}
			}
			void destroy(BaseAlloc& alloc, BasePointer p) noexcept override {
				using DerivedAlloc = typename std::allocator_traits<BaseAlloc>::template rebind_alloc<ConcreteDerived>;
				using DerivedAllocTraits = std::allocator_traits<DerivedAlloc>;

				DerivedAlloc derived_alloc(alloc);
				auto* p_derived = static_cast<ConcreteDerived*>(std::to_address(p));

				try {
					DerivedAllocTraits::destroy(derived_alloc, p_derived);
				} catch (...) {
					assert(false && "Destructor threw an exception");
					std::terminate();
				}
				std::allocator_traits<BaseAlloc>::deallocate(alloc, p, 1);
			}

			[[nodiscard]] const std::type_info& target_type_info() const noexcept override {
				return typeid(ConcreteDerived);
			}
		};

		// Simplified DelegatingControlBlock
		template<typename BaseT, typename DerivedT, typename BaseAlloc>
		struct DelegatingControlBlock final : ControlBlockBase<BaseT, BaseAlloc> {
			using SourceControlBlock = ControlBlockBase<DerivedT, BaseAlloc>;
			using BasePointer = typename ControlBlockBase<BaseT, BaseAlloc>::BasePointer;
			std::unique_ptr<SourceControlBlock> source_cb;

			explicit DelegatingControlBlock(std::unique_ptr<SourceControlBlock>&& src_cb)
					: source_cb(std::move(src_cb)) {}

			BaseT* get_ptr() noexcept override {
				return source_cb->get_ptr();
			}

			ControlBlockBase<BaseT, BaseAlloc>* clone(const BaseAlloc& target_cb_alloc) const override {
				auto cloned_source = std::unique_ptr<SourceControlBlock>(source_cb->clone(target_cb_alloc));

				using CBDerivedAlloc = typename std::allocator_traits<BaseAlloc>::template rebind_alloc<DelegatingControlBlock>;
				using CBDerivedAllocTraits = std::allocator_traits<CBDerivedAlloc>;
				CBDerivedAlloc cb_alloc(target_cb_alloc);

				auto* cb_ptr = CBDerivedAllocTraits::allocate(cb_alloc, 1);
				try {
					CBDerivedAllocTraits::construct(cb_alloc, cb_ptr, std::move(cloned_source));
					return cb_ptr;
				} catch (...) {
					CBDerivedAllocTraits::deallocate(cb_alloc, cb_ptr, 1);
					throw;
				}
			}

			BasePointer copy(const BaseAlloc& target_alloc, BasePointer p) const override {
				auto* p_derived = static_cast<DerivedT*>(std::to_address(p));
				using DerivedPointer = typename std::allocator_traits<BaseAlloc>::template rebind_traits<DerivedT>::pointer;
				DerivedPointer p_derived_wrapped = std::pointer_traits<DerivedPointer>::pointer_to(*p_derived);
				auto copied_derived_ptr = source_cb->copy(target_alloc, p_derived_wrapped);

				return std::pointer_traits<BasePointer>::pointer_to(
						*static_cast<BaseT*>(std::to_address(copied_derived_ptr)));
			}

			void destroy(BaseAlloc& alloc, BasePointer p) noexcept override {
				auto* p_derived = static_cast<DerivedT*>(std::to_address(p));
				using DerivedPointer = typename std::allocator_traits<BaseAlloc>::template rebind_traits<DerivedT>::pointer;
				DerivedPointer p_derived_wrapped = std::pointer_traits<DerivedPointer>::pointer_to(*p_derived);

				source_cb->destroy(alloc, p_derived_wrapped);
			}

			[[nodiscard]] const std::type_info& target_type_info() const noexcept override {
				return source_cb->target_type_info();
			}
		};
	} // namespace detail

	template <class T, class Allocator = std::allocator<T>>
	class polymorphic {
		template<typename, typename> friend class polymorphic; // Allow access for conversions

		// Static asserts...
		static_assert(!std::is_array_v<T>, "polymorphic<T>: T cannot be an array type.");
		static_assert(std::is_object_v<T>, "polymorphic<T>: T must be an object type.");
		static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>, "polymorphic<T>: T cannot be cv-qualified.");
		static_assert(!std::same_as<T, std::in_place_t>, "polymorphic<T>: T cannot be in_place_t.");
		static_assert(!is_in_place_type_v<T>, "polymorphic<T>: T cannot be a specialization of in_place_type_t.");
		using BaseAllocTraits = std::allocator_traits<Allocator>;
		static_assert(std::same_as<typename BaseAllocTraits::value_type, T>,
		              "polymorphic<T, Allocator>: allocator_traits<Allocator>::value_type must be T.");

		using pointer = typename BaseAllocTraits::pointer;
		using const_pointer = typename BaseAllocTraits::const_pointer;
		using ControlBlock = detail::ControlBlockBase<T, Allocator>;

		// Custom deleter for the Control Block unique_ptr
		struct ControlBlockDeleter {
			Allocator alloc;

			// Rebind the allocator to the ControlBlock type for deletion
			using CBAlloc = typename BaseAllocTraits::template rebind_alloc<ControlBlock>;
			using CBAllocTraits = std::allocator_traits<CBAlloc>;

			void operator()(ControlBlock* cb) const noexcept {
				if (!cb) return;
				CBAlloc cb_alloc(alloc);
				try {
					CBAllocTraits::destroy(cb_alloc, cb);
				} catch (...) {
					assert(false && "Control block destructor threw"); std::terminate();
				}
				CBAllocTraits::deallocate(cb_alloc, cb, 1);
			}
		};

		using ControlBlockPtr = std::unique_ptr<ControlBlock, ControlBlockDeleter>;

		pointer p_ = nullptr;
		ControlBlockPtr cb_ptr_ = nullptr;

		// --- Private Helpers ---
		constexpr Allocator& get_allocator_ref() noexcept {
			return cb_ptr_.get_deleter().alloc;
		}

		constexpr const Allocator& get_allocator_ref() const noexcept {
			return cb_ptr_.get_deleter().alloc;
		}

		constexpr void reset() noexcept {
			if (p_) {
				assert(cb_ptr_ && "Invariant violation: p_ set but cb_ptr_ is null");
				cb_ptr_->destroy(get_allocator_ref(), p_);
				p_ = nullptr;
			}
			cb_ptr_.reset();
		}

		template<typename ConcreteDerived, typename... Args>
		requires std::derived_from<ConcreteDerived, T> && std::constructible_from<ConcreteDerived, Args...>
		void allocate_construct_and_init(Allocator current_alloc, Args&&... args) {
			using CBDerived = detail::ControlBlockDerived<ConcreteDerived, T, Allocator>;
			using CBAlloc = typename BaseAllocTraits::template rebind_alloc<CBDerived>;
			using CBAllocTraits = std::allocator_traits<CBAlloc>;

			CBAlloc cb_alloc(current_alloc);
			auto* cb_derived = CBAllocTraits::allocate(cb_alloc, 1);

			try {
				// Construct the control block with the derived object
				CBAllocTraits::construct(cb_alloc, cb_derived, std::forward<Args>(args)...);

				// Get the pointer to the derived object
				auto* derived_ptr = cb_derived->get_ptr();
				p_ = std::pointer_traits<pointer>::pointer_to(*derived_ptr);

				// Create the control block pointer with proper deleter
				cb_ptr_ = ControlBlockPtr(cb_derived, ControlBlockDeleter{current_alloc});
			} catch (...) {
				if (cb_derived) {
					CBAllocTraits::deallocate(cb_alloc, cb_derived, 1);
				}
				throw;
			}
		}

		pointer release_p() noexcept { return std::exchange(p_, nullptr); }
		ControlBlockPtr release_cb_ptr() noexcept { return std::move(cb_ptr_); }

	public:
		using value_type = T;
		using allocator_type = Allocator;

		// --- Constructors ---
		explicit constexpr polymorphic() requires std::default_initializable<Allocator>
				: p_(nullptr), cb_ptr_(nullptr, ControlBlockDeleter{Allocator{}}) {}

		explicit constexpr polymorphic(std::allocator_arg_t, const Allocator& a) noexcept
				: p_(nullptr), cb_ptr_(nullptr, ControlBlockDeleter{a}) {}

		constexpr polymorphic(std::nullptr_t) requires std::default_initializable<Allocator> : polymorphic() {}
		constexpr polymorphic(std::allocator_arg_t, const Allocator& a, std::nullptr_t) noexcept : polymorphic(std::allocator_arg, a) {}

		template <class U, class... Ts>
		explicit constexpr polymorphic(std::in_place_type_t<U>, Ts&&... ts) requires std::default_initializable<Allocator> &&
		                                                                             std::derived_from<U, T> && std::constructible_from<U, Ts...>
				: polymorphic()
		{
			allocate_construct_and_init<U>(Allocator{}, std::forward<Ts>(ts)...);
		}

		template <class U, class... Ts>
		explicit constexpr polymorphic(std::allocator_arg_t, const Allocator& a, std::in_place_type_t<U>, Ts&&... ts) requires
		std::derived_from<U, T> && std::constructible_from<U, Ts...>
				: polymorphic(std::allocator_arg, a)
		{
			allocate_construct_and_init<U>(a, std::forward<Ts>(ts)...);
		}

		template <class U>
		explicit constexpr polymorphic(U&& u) requires std::default_initializable<Allocator> &&
		                                               (!std::same_as<std::remove_cvref_t<U>, polymorphic>) &&
		                                               (!is_polymorphic_v<std::remove_cvref_t<U>>) &&
		                                               (!std::same_as<std::remove_cvref_t<U>, std::in_place_t>) &&
		                                               (!is_in_place_type_v<std::remove_cvref_t<U>>) &&
		                                               (!std::is_same_v<std::remove_cvref_t<U>, std::allocator_arg_t>) &&
		                                               (!std::is_base_of_v<std::allocator_arg_t, std::remove_cvref_t<U>>) &&
		                                               std::derived_from<std::remove_cvref_t<U>, T> &&
		                                               std::constructible_from<std::remove_cvref_t<U>, U> &&
		                                               std::copy_constructible<std::remove_cvref_t<U>>
				: polymorphic(std::in_place_type<std::remove_cvref_t<U>>, std::forward<U>(u))
		{}

		template <class U>
		explicit constexpr polymorphic(std::allocator_arg_t, const Allocator& a, U&& u) requires
		(!std::same_as<std::remove_cvref_t<U>, polymorphic>) &&
		(!is_polymorphic_v<std::remove_cvref_t<U>>) &&
		(!std::same_as<std::remove_cvref_t<U>, std::in_place_t>) &&
		(!is_in_place_type_v<std::remove_cvref_t<U>>) &&
		std::derived_from<std::remove_cvref_t<U>, T> &&
				std::constructible_from<std::remove_cvref_t<U>, U> &&
		std::copy_constructible<std::remove_cvref_t<U>>
				: polymorphic(std::allocator_arg, a, std::in_place_type<std::remove_cvref_t<U>>, std::forward<U>(u))
				{}

		// Copy Constructor
		[[deprecated("polymorphic performs potentially expensive deep copy; consider moving or using std::optional.")]]
		polymorphic(const polymorphic& other) {
			if (other) {
				if(DEBUG) std::cout << "COPY CTOR CALLED\n";
				// Use the control block to create a copy
				try {
					p_ = other.cb_ptr_->copy(get_allocator_ref(), other.p_);
					cb_ptr_.reset(other.cb_ptr_->clone(get_allocator_ref()));
				} catch (...) {
					reset();
					throw;
				}
			}
		}

		// Allocator-extended Copy Constructor
		[[deprecated("polymorphic performs potentially expensive deep copy; consider moving or using std::optional.")]]
		constexpr polymorphic(std::allocator_arg_t, const Allocator& a, const polymorphic& other)
				: cb_ptr_(nullptr, ControlBlockDeleter{a})
		{
			if (other.p_) {
				if(DEBUG) std::cout << "COPY CTOR CALLED\n";
				assert(other.cb_ptr_ && "Source has value but no control block");
				ControlBlock* cb_raw = nullptr;
				try {
					p_ = other.cb_ptr_->copy(get_allocator_ref(), other.p_);
					cb_raw = other.cb_ptr_->clone(get_allocator_ref());
					cb_ptr_.reset(cb_raw);
				} catch(...) {
					if (p_ && !cb_raw) { p_ = nullptr; }
					reset();
					throw;
				}
			}
		}

		// Conversion constructor from derived to base
		template<typename Derived, typename DerivedAlloc>
		requires std::derived_from<Derived, T> &&
		         std::is_convertible_v<DerivedAlloc, Allocator>
		polymorphic(polymorphic<Derived, DerivedAlloc>&& other) noexcept
				: cb_ptr_(nullptr, ControlBlockDeleter{Allocator(std::move(other.get_allocator_ref()))})
		{
			if (other.p_) {
				// Create a new delegating control block
				using DelegatingCB = detail::DelegatingControlBlock<T, Derived, Allocator>;
				using CBAlloc = typename BaseAllocTraits::template rebind_alloc<DelegatingCB>;
				using CBAllocTraits = std::allocator_traits<CBAlloc>;
				using SourceCB = detail::ControlBlockBase<Derived, Allocator>;
				using SourceCBAlloc = typename BaseAllocTraits::template rebind_alloc<detail::ControlBlockDerived<Derived, Derived, Allocator>>;
				using SourceCBAllocTraits = std::allocator_traits<SourceCBAlloc>;

				CBAlloc cb_alloc(get_allocator_ref());
				auto* new_cb = CBAllocTraits::allocate(cb_alloc, 1);

				try {
					// Clone the source control block with the target allocator
					SourceCBAlloc source_cb_alloc(get_allocator_ref());
					auto* source_cb_ptr = other.cb_ptr_->clone(get_allocator_ref());
					auto source_cb = std::unique_ptr<SourceCB>(source_cb_ptr);

					// Construct the delegating control block
					CBAllocTraits::construct(cb_alloc, new_cb, std::move(source_cb));
					cb_ptr_.reset(new_cb);
					p_ = std::exchange(other.p_, nullptr);
				} catch (...) {
					CBAllocTraits::deallocate(cb_alloc, new_cb, 1);
					throw;
				}
			}
		}
		// Copy version of the clone ctor
		template<typename Derived, typename DerivedAlloc>
		requires std::derived_from<Derived, T> &&
		         std::is_constructible_v<Allocator, const DerivedAlloc&> &&
		         std::is_convertible_v<const DerivedAlloc&, Allocator>
		polymorphic(const polymorphic<Derived, DerivedAlloc>& other)
				: cb_ptr_(nullptr, ControlBlockDeleter{Allocator(other.get_allocator_ref())})
		{
			if (other) {
				try {
					// Create a new delegating control block to handle the type conversion
					using SourceCB = detail::ControlBlockBase<Derived, DerivedAlloc>;
					using DelegatingCB = detail::DelegatingControlBlock<T, Derived, Allocator>;
					using CBAlloc = typename BaseAllocTraits::template rebind_alloc<DelegatingCB>;
					using CBAllocTraits = std::allocator_traits<CBAlloc>;

					CBAlloc cb_alloc(get_allocator_ref());
					auto* new_cb = CBAllocTraits::allocate(cb_alloc, 1);

					// Clone the source control block with its own allocator
					std::unique_ptr<SourceCB> cloned_source(other.cb_ptr_->clone(other.get_allocator_ref()));

					try {
						// Construct the delegating control block
						CBAllocTraits::construct(cb_alloc, new_cb, std::move(cloned_source));
						cb_ptr_.reset(new_cb);
						// Copy the pointer, converting it to the base type
						p_ = other.cb_ptr_->copy(get_allocator_ref(), other.p_);
					} catch (...) {
						CBAllocTraits::deallocate(cb_alloc, new_cb, 1);
						throw;
					}
				} catch (...) {
					reset();
					throw;
				}
			}
		}

		//template<typename Derived, typename DerivedAlloc>
		//requires std::derived_from<Derived, T> &&
		//         std::is_convertible_v<DerivedAlloc, Allocator>
		//polymorphic(const polymorphic<Derived, DerivedAlloc>& other)
		//		: cb_ptr_(nullptr, ControlBlockDeleter{Allocator(other.get_allocator_ref())})
		//{
		//	if (other) {
		//		p_ = other.cb_ptr_->copy(get_allocator_ref(), other.p_);
		//		cb_ptr_.reset(other.cb_ptr_->clone(get_allocator_ref()));
		//	}
		//}


		// Move Constructor
		constexpr polymorphic(polymorphic&& other) noexcept
				: p_(other.release_p()),
				  cb_ptr_(other.release_cb_ptr())
		{}

		// Allocator-extended Move Constructor
		constexpr polymorphic(std::allocator_arg_t, const Allocator& a, polymorphic&& other)
		noexcept(BaseAllocTraits::is_always_equal::value)
				: cb_ptr_(nullptr, ControlBlockDeleter{a})
		{
			if (!other.p_) { return; }

			if constexpr (BaseAllocTraits::is_always_equal::value) {
				p_ = other.release_p();
				cb_ptr_ = other.release_cb_ptr();
				cb_ptr_.get_deleter().alloc = a;
			} else {
				if (get_allocator_ref() == other.get_allocator_ref()) {
					p_ = other.release_p();
					cb_ptr_ = other.release_cb_ptr();
				} else {
					assert(other.cb_ptr_ && "Source has value but no control block");
					ControlBlock* cb_raw = nullptr;
					try {
						p_ = other.cb_ptr_->copy(get_allocator_ref(), other.p_);
						cb_raw = other.cb_ptr_->clone(get_allocator_ref());
						cb_ptr_.reset(cb_raw);
						other.reset();
					} catch(...) {
						if (p_ && !cb_raw) { p_ = nullptr; }
						reset();
						throw;
					}
				}
			}
		}


		// Converting copy constructor from derived to base
		//template <class U, class A>
		//requires std::derived_from<U, T>
		//polymorphic(const polymorphic<U, A>& other) {
		//	if (other) {
		//		p_ = other.cb_ptr_->copy(get_allocator_ref(), other.p_);
		//		cb_ptr_.reset(other.cb_ptr_->clone(get_allocator_ref()));
		//	}
		//}

		// Destructor
		constexpr ~polymorphic() {
			reset();
		}

		// Assignment Operators
		[[deprecated("polymorphic performs potentially expensive deep copy; consider moving or using std::optional.")]]
		constexpr polymorphic& operator=(const polymorphic& other) {
			if(DEBUG) std::cout << "COPY CTOR CALLED\n";
			if (this == &other) return *this;
			polymorphic temp(other);
			if constexpr (BaseAllocTraits::propagate_on_container_copy_assignment::value) {
				if (get_allocator_ref() != other.get_allocator_ref()) {
					get_allocator_ref() = other.get_allocator_ref();
					cb_ptr_.get_deleter().alloc = get_allocator_ref();
				}
			}
			swap(temp);
			return *this;
		}

		constexpr polymorphic& operator=(polymorphic&& other) noexcept(
		BaseAllocTraits::propagate_on_container_move_assignment::value ||
		BaseAllocTraits::is_always_equal::value)
		{
			if (this == &other) return *this;
			constexpr bool pocma = BaseAllocTraits::propagate_on_container_move_assignment::value;
			if constexpr (pocma) {
				reset();
				get_allocator_ref() = std::move(other.get_allocator_ref());
				cb_ptr_.get_deleter().alloc = get_allocator_ref();
				p_ = other.release_p();
				cb_ptr_ = other.release_cb_ptr();
			} else {
				if (get_allocator_ref() == other.get_allocator_ref()) {
					reset();
					p_ = other.release_p();
					cb_ptr_ = other.release_cb_ptr();
				} else {
					if (other.p_) {
						polymorphic temp(std::allocator_arg, get_allocator_ref(), other);
						swap(temp);
						other.reset();
					} else {
						reset();
					}
				}
			}
			return *this;
		}

		constexpr polymorphic& operator=(std::nullptr_t) noexcept {
			reset();
			return *this;
		}

		template<typename Derived, typename DerivedAllocator>
		requires std::derived_from<Derived, T> &&
		         std::is_constructible_v<Allocator, DerivedAllocator&&> &&
		         std::is_assignable_v<Allocator&, Allocator&&>
		constexpr polymorphic& operator=(polymorphic<Derived, DerivedAllocator>&& other)
		noexcept(
		std::is_nothrow_constructible_v<Allocator, DerivedAllocator&&> &&  // Converting constructor allocator construction
		std::is_nothrow_move_constructible_v<ControlBlockPtr> &&           // Control block move
		std::is_nothrow_swappable_v<ControlBlockPtr> &&                    // Swap operation
		(BaseAllocTraits::propagate_on_container_move_assignment::value || // Allocator move assignment
		 BaseAllocTraits::is_always_equal::value)                          // or allocators are always equal
		) {
			polymorphic temp(std::move(other));
			swap(temp);
			return *this;
		}

		template<typename Derived, typename DerivedAllocator>
		requires std::derived_from<Derived, T> &&
		         std::is_constructible_v<Allocator, const DerivedAllocator&>
		[[deprecated("polymorphic performs potentially expensive deep copy; consider moving or using std::optional.")]]
		constexpr polymorphic& operator=(const polymorphic<Derived, DerivedAllocator>& other) {
			if(DEBUG) std::cout << "COPY CTOR CALLED\n";
			polymorphic temp(other);
			if constexpr (BaseAllocTraits::propagate_on_container_copy_assignment::value) {
				if (get_allocator_ref() != other.get_allocator_ref()) {
					get_allocator_ref() = other.get_allocator_ref();
					cb_ptr_.get_deleter().alloc = get_allocator_ref();
				}
			}
			swap(temp);
			return *this;
		}

		template <typename U, typename A>
		requires std::derived_from<U, T> &&
		         std::is_convertible_v<A, Allocator>
		operator polymorphic<U, A>() const {
			if (!*this) return nullptr;
			return polymorphic<U, A>(std::in_place_type<U>, **this);
		}

		// Observers
		constexpr const T& operator*() const noexcept { assert(has_value()); return *cb_ptr_->get_ptr(); }
		constexpr T& operator*() noexcept { assert(has_value()); return *cb_ptr_->get_ptr(); }
		constexpr const_pointer operator->() const noexcept { assert(has_value()); return std::pointer_traits<const_pointer>::pointer_to(*cb_ptr_->get_ptr()); }
		constexpr pointer operator->() noexcept { assert(has_value()); return std::pointer_traits<pointer>::pointer_to(*cb_ptr_->get_ptr()); }

		constexpr bool has_value() const noexcept { return p_ != nullptr; }
		constexpr bool valueless_after_move() const noexcept { return p_ == nullptr; }
		constexpr explicit operator bool() const noexcept { return has_value(); }
		constexpr allocator_type get_allocator() const noexcept { return get_allocator_ref(); }

		[[deprecated("Access via operator* or operator-> is preferred.")]]
		constexpr pointer get() noexcept { return p_; }

		[[deprecated("Access via operator* or operator-> is preferred.")]]
		constexpr const_pointer get() const noexcept { return p_; }

		// Modifiers
		[[deprecated("Value types typically use assignment for replacement. This deviates from P3019.")]]
		constexpr void reset_public() noexcept { reset(); }

		[[deprecated("Value types manage their own resources; releasing breaks ownership. This deviates from P3019.")]]
		[[nodiscard]] constexpr pointer release_public() noexcept {
			cb_ptr_.reset();
			return std::exchange(p_, nullptr);
		}

		// Swap
		constexpr void swap(polymorphic& other) noexcept(
		BaseAllocTraits::propagate_on_container_swap::value ||
		BaseAllocTraits::is_always_equal::value)
		{
			using std::swap;
			if constexpr(BaseAllocTraits::propagate_on_container_swap::value) {
				swap(p_, other.p_);
				swap(cb_ptr_, other.cb_ptr_);
			}
			else {
				if (get_allocator_ref() != other.get_allocator_ref()) {
					assert(false && "polymorphic::swap requires allocators to be equal when POCS is false");
					return;
				}
				swap(p_, other.p_);
				swap(cb_ptr_, other.cb_ptr_);
			}
		}

		friend constexpr void swap(polymorphic& lhs, polymorphic& rhs) noexcept(noexcept(lhs.swap(rhs))) {
			lhs.swap(rhs);
		}

		// Type Checking/Access
		template <typename Target>
		[[nodiscard]] bool is_type() const noexcept {
			return has_value() && (cb_ptr_->target_type_info() == typeid(Target));
		}

		template <typename Target>
		[[nodiscard]] polymorphic<Target> dynamic_cast_to() const {
			if (!is_type<Target>()) return nullptr;
			return polymorphic<Target>(*this); // Uses converting constructor
		}

		template <typename Target>
		[[nodiscard]] Target* dynamic_get() noexcept {
			// First try static_cast if we're sure of the hierarchy
			if constexpr (std::is_base_of_v<T, Target>) {
				auto* result = static_cast<Target*>(std::to_address(p_));
				// Optional: verify with typeid if debug mode
				assert(typeid(*result) == typeid(Target) && "Static cast violated");
				return result;
			}
			else {
				return dynamic_cast<Target*>(std::to_address(p_));
			}
		}

		template <typename Interface>
		std::optional<Interface*> try_as_interface() const {
			if (!p_) return std::nullopt;
			return dynamic_cast<Interface*>(std::to_address(p_));
		}

		// Inside the polymorphic class definition
		template <typename To>
		[[nodiscard]] polymorphic<To> cast() && {
			if (!has_value()) return nullptr;
			if (cb_ptr_->target_type_info() != typeid(To)) {
				throw std::bad_cast();
			}
			return polymorphic<To>(std::move(*this)); // Will use the copy constructor
		}

		template <typename To>
		[[nodiscard]] polymorphic<To> cast() const& {
			if (!has_value()) return nullptr;
			if (cb_ptr_->target_type_info() != typeid(To)) {
				throw std::bad_cast();
			}
			return polymorphic<To>(*this); // Will use the copy constructor
		}

	};

	template <typename T>
	concept HasBaseType = requires {
		typename T::base_type;
	};


	// Factory Functions
	template <typename ConcreteDerived, typename Base, typename BaseAlloc, typename... Args>
	requires std::derived_from<ConcreteDerived, Base> &&
	         std::constructible_from<ConcreteDerived, Args...> &&
	         std::copy_constructible<ConcreteDerived> &&
	         std::is_same_v<typename std::allocator_traits<BaseAlloc>::value_type, Base>
	[[nodiscard]] constexpr polymorphic<Base, BaseAlloc>
	make_polymorphic_alloc(std::allocator_arg_t, const BaseAlloc& alloc, Args&&... args) {
		return polymorphic<Base, BaseAlloc>(std::allocator_arg, alloc,
		                                    std::in_place_type<ConcreteDerived>,
		                                    std::forward<Args>(args)...);
	}

	template <typename ConcreteDerived, typename Base, typename... Args>
	requires std::derived_from<ConcreteDerived, Base> &&
	         std::constructible_from<ConcreteDerived, Args...> &&
	         std::copy_constructible<ConcreteDerived> &&
	         std::default_initializable<std::allocator<Base>>
	[[nodiscard]] constexpr polymorphic<Base, std::allocator<Base>>
	make_polymorphic(Args&&... args) {
		return polymorphic<Base, std::allocator<Base>>(std::in_place_type<ConcreteDerived>,
		                                               std::forward<Args>(args)...);
	}

	template <typename T, typename Alloc, typename... Args>
	requires std::constructible_from<T, Args...> &&
	         std::copy_constructible<T> &&
	         std::is_same_v<typename std::allocator_traits<Alloc>::value_type, T>
	[[nodiscard]] constexpr polymorphic<T, Alloc>
	make_polymorphic_alloc(std::allocator_arg_t, const Alloc& alloc, Args&&... args) {
		return polymorphic<T, Alloc>(std::allocator_arg, alloc,
		                             std::in_place_type<T>,
		                             std::forward<Args>(args)...);
	}

	template <typename Concrete, typename... Args>
	auto make_polymorphic(Args&&... args) {
		return std_P3019_modified::polymorphic<std::remove_cvref_t<Concrete>>(
				std::in_place_type<std::remove_cvref_t<Concrete>>,
				std::forward<Args>(args)...
		);
	}
	template <typename To, typename From, typename Alloc>
	[[nodiscard]] polymorphic<To> polymorphic_cast(polymorphic<From, Alloc>&& p) {
		return std::move(p).template cast<To>();
	}

	template <typename To, typename From, typename Alloc>
	[[nodiscard]] polymorphic<To> polymorphic_cast(const polymorphic<From, Alloc>& p) {
		return p.template cast<To>();
	}


} // namespace std_P3019_modified

// std::hash specialization
namespace std {

	template<class T, class Allocator>
	struct hash<std_P3019_modified::indirect < T, Allocator>> {
	using indirect_type = std_P3019_modified::indirect<T, Allocator>;
	using is_enabled = std::bool_constant<requires(const T &t) { std::hash<T>{}(t); }>;

	template<bool E = is_enabled::value, std::enable_if_t<E, int> = 0>
	std::size_t operator()(const indirect_type &i) const noexcept(noexcept(std::hash<T>{}(*i))) {
		if (i.has_value()) {
			return std::hash<T>{}(*i);
		}
		else {
			return static_cast<std::size_t>(0xcbf29ce484222325ULL ^ 0x100000001b3ULL);
		}
	}
};
}