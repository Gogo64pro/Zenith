module;
#include <memory>
#include <type_traits>
#include <concepts>
#include <functional>
#include <cassert>
#include <optional>
#include <string>
#include <typeinfo>
#include <iostream>
#include <utility>
#include <exception>
export module zenith.core.polymorphic;
export namespace std_P3019_modified {

	template <class T, class Allocator>
	class polymorphic;

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



	namespace detail {
		template<typename BaseT, typename BaseAlloc>
		struct ControlBlockBase {
			using BasePointer = typename std::allocator_traits<BaseAlloc>::pointer;
			virtual ~ControlBlockBase() = default;
			virtual BaseT* get_ptr() noexcept = 0;
			virtual void destroy(const BaseAlloc& alloc, BasePointer p) noexcept = 0;
			[[nodiscard]] virtual const std::type_info& target_type_info() const noexcept = 0;
			virtual void increment_ref_count() = 0;
			virtual bool decrement_ref_count() = 0; // Returns true if this was the last reference
			[[nodiscard]] virtual bool is_convertible_to(const std::type_info& target) const noexcept = 0;

		};

		template<typename ConcreteDerived, typename BaseT, typename BaseAlloc>
		struct ControlBlockDerived final : detail::ControlBlockBase<BaseT, BaseAlloc> {
			using BasePointer = typename detail::ControlBlockBase<BaseT, BaseAlloc>::BasePointer;
			ConcreteDerived derived;
			int ref_count = 1;
			[[no_unique_address]]BaseAlloc alloc;

			template<typename... Args>
			explicit ControlBlockDerived(BaseAlloc alloc, Args&&... args) : derived(std::forward<Args>(args)...), alloc(alloc) {}

			BaseT* get_ptr() noexcept override { return &derived; }

			void destroy(const BaseAlloc& alloc, BasePointer p) noexcept override {
				if (p && decrement_ref_count()) {
					using DerivedAlloc = typename std::allocator_traits<BaseAlloc>::template rebind_alloc<ConcreteDerived>;
					using DerivedAllocTraits = std::allocator_traits<DerivedAlloc>;
					DerivedAlloc derived_alloc(alloc);
					auto* p_derived = static_cast<ConcreteDerived*>(std::to_address(p));
					if (p_derived) {
						DerivedAllocTraits::destroy(derived_alloc, p_derived);
						DerivedAllocTraits::deallocate(derived_alloc, p_derived, 1);
					} else {
					}
				} else {
				}
			}

			[[nodiscard]] const std::type_info& target_type_info() const noexcept override {
				return typeid(ConcreteDerived);
			}

			void increment_ref_count() override {
				++ref_count;
			}

			bool decrement_ref_count() override {
				if (ref_count > 0) {
					return --ref_count == 0;
				}
				return false;
			}

			[[nodiscard]] bool is_convertible_to(const std::type_info& target) const noexcept override {
				return target == typeid(ConcreteDerived) ||
				       std::is_base_of_v<std::remove_pointer_t<decltype(target)>, ConcreteDerived>;
			}
		};

		template<typename BaseT, typename SourceT, typename BaseAlloc, typename SourceAlloc>
		struct DelegatingControlBlock final : detail::ControlBlockBase<BaseT, BaseAlloc> {
			using BasePointer = typename detail::ControlBlockBase<BaseT, BaseAlloc>::BasePointer;
			using SourcePointer = typename detail::ControlBlockBase<SourceT, SourceAlloc>::BasePointer;
			std::unique_ptr<ControlBlockBase<SourceT, SourceAlloc>> source_cb;
			SourcePointer source_ptr;
			int ref_count = 1;
			[[no_unique_address]] SourceAlloc source_alloc;
			[[no_unique_address]] BaseAlloc base_alloc;

			explicit DelegatingControlBlock(std::unique_ptr<ControlBlockBase<SourceT, SourceAlloc>> src_cb, SourcePointer src_ptr, SourceAlloc src_alloc = SourceAlloc(), BaseAlloc b_alloc = BaseAlloc())
					: source_ptr(src_ptr), ref_count(1), source_alloc(src_alloc), base_alloc(b_alloc) {
				if (src_cb) {
					source_cb = std::move(src_cb);
					source_cb->increment_ref_count();
				} else {
				}
				if (!source_ptr) {
				}
			}

			BaseT* get_ptr() noexcept override {
				if (!source_ptr) {
					return nullptr;
				}
				return reinterpret_cast<BaseT*>(source_ptr);
			}

			void destroy(const BaseAlloc& alloc, BasePointer p) noexcept override {
				if (decrement_ref_count()) {
					if (source_cb && source_ptr) {
						const SourceAlloc alloc_copy = source_alloc;
						auto source_p = reinterpret_cast<SourcePointer>(source_ptr);
						source_cb->destroy(alloc_copy, source_p);
						source_cb.reset();
					} else {
					}
				} else {
				}
			}

			[[nodiscard]] const std::type_info& target_type_info() const noexcept override {
				return source_cb ? source_cb->target_type_info() : typeid(void);
			}

			void increment_ref_count() override {
				++ref_count;
				if (source_cb) {
					source_cb->increment_ref_count();
				}
			}

			bool decrement_ref_count() override {
				if (ref_count > 0) {
					bool result = --ref_count == 0;
					if (source_cb && result) {
						source_cb->decrement_ref_count();
					}
					return result;
				}
				return false;
			}
			[[nodiscard]] bool is_convertible_to(const std::type_info& target) const noexcept {
				return source_cb->is_convertible_to(target);
			}
		};
	} // namespace detail

	template <class T, class Allocator = std::allocator<T>>
	class polymorphic {
		template<typename, typename> friend
		class polymorphic;


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

		struct ControlBlockDeleter {
			[[no_unique_address]] Allocator alloc;

			using CBAlloc = typename BaseAllocTraits::template rebind_alloc<ControlBlock>;
			using CBAllocTraits = std::allocator_traits<CBAlloc>;

			void operator()(ControlBlock *cb) const noexcept {
				if (!cb) return;
				CBAlloc cb_alloc(alloc);
				if (!cb->decrement_ref_count()) return;
				try {
					CBAllocTraits::destroy(cb_alloc, cb);
				} catch (...) {
					assert(false && "Control block destructor threw");
					std::terminate();
				}
				CBAllocTraits::deallocate(cb_alloc, cb, 1);
			}
		};

		using ControlBlockPtr = std::unique_ptr<ControlBlock, ControlBlockDeleter>;

		pointer p_ = nullptr;
		ControlBlockPtr cb_ptr_ = nullptr;

		constexpr Allocator &get_allocator_ref() noexcept {
			return cb_ptr_.get_deleter().alloc;
		}

		constexpr const Allocator &get_allocator_ref() const noexcept {
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
		void allocate_construct_and_init(Allocator current_alloc, Args &&... args) {
			using CBDerived = detail::ControlBlockDerived<ConcreteDerived, T, Allocator>;
			using CBAlloc = typename BaseAllocTraits::template rebind_alloc<CBDerived>;
			using CBAllocTraits = std::allocator_traits<CBAlloc>;

			CBAlloc cb_alloc(current_alloc);
			auto *cb_derived = CBAllocTraits::allocate(cb_alloc, 1);

			try {
				new(cb_derived) CBDerived(cb_alloc, std::forward<Args>(args)...);

				auto *derived_ptr = cb_derived->get_ptr();
				p_ = std::pointer_traits<pointer>::pointer_to(*derived_ptr);

				cb_ptr_ = ControlBlockPtr(cb_derived, ControlBlockDeleter{current_alloc});
				cb_ptr_->increment_ref_count();
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


		polymorphic(const polymorphic&) = delete;
		polymorphic& operator=(const polymorphic&) = delete;

		explicit constexpr polymorphic() requires std::default_initializable<Allocator>
				: p_(nullptr), cb_ptr_(nullptr, ControlBlockDeleter{Allocator{}}) {}

		explicit constexpr polymorphic(std::allocator_arg_t, const Allocator &a) noexcept
				: p_(nullptr), cb_ptr_(nullptr, ControlBlockDeleter{a}) {}

		constexpr polymorphic(std::nullptr_t) requires std::default_initializable<Allocator>: polymorphic() {} // NOLINT(google-explicit-constructor)

		constexpr polymorphic(std::allocator_arg_t, const Allocator &a, std::nullptr_t) noexcept: polymorphic(std::allocator_arg, a) {}

		template<class U, class... Ts>
		explicit constexpr polymorphic(std::in_place_type_t<U>, Ts &&... ts) requires std::default_initializable<Allocator> &&
		                                                                              std::derived_from<U, T> && std::constructible_from<U, Ts...>
				: polymorphic() {
			allocate_construct_and_init<U>(Allocator{}, std::forward<Ts>(ts)...);
		}

		template<class U, class... Ts>
		explicit constexpr polymorphic(std::allocator_arg_t, const Allocator &a, std::in_place_type_t<U>, Ts &&... ts) requires
		std::derived_from<U, T> && std::constructible_from<U, Ts...>
				: polymorphic(std::allocator_arg, a) {
			allocate_construct_and_init<U>(a, std::forward<Ts>(ts)...);
		}

		template<class U>
		explicit constexpr polymorphic(U &&u) requires std::default_initializable<Allocator> &&
		                                               (!std::same_as<std::remove_cvref_t<U>, polymorphic>) &&
		                                               (!is_polymorphic_v<std::remove_cvref_t<U>>) &&
		                                               (!std::same_as<std::remove_cvref_t<U>, std::in_place_t>) &&
		                                               (!is_in_place_type_v<std::remove_cvref_t<U>>) &&
		                                               (!std::is_same_v<std::remove_cvref_t<U>, std::allocator_arg_t>) &&
		                                               (!std::is_base_of_v<std::allocator_arg_t, std::remove_cvref_t<U>>) &&
		                                               std::derived_from<std::remove_cvref_t<U>, T> &&
		                                               std::constructible_from<std::remove_cvref_t<U>, U> &&
		                                               std::copy_constructible<std::remove_cvref_t<U>>
				: polymorphic(std::in_place_type<std::remove_cvref_t<U>>, std::forward<U>(u)) {}

		template<class U>
		explicit constexpr polymorphic(std::allocator_arg_t, const Allocator &a, U &&u) requires
		(!std::same_as<std::remove_cvref_t<U>, polymorphic>)

		&&
		(!is_polymorphic_v<std::remove_cvref_t<U>>) &&
		(!std::same_as<std::remove_cvref_t<U>, std::in_place_t>) &&
		(!is_in_place_type_v<std::remove_cvref_t<U>>) &&
		std::derived_from<std::remove_cvref_t<U>, T> &&
				std::constructible_from<std::remove_cvref_t<U>, U>
		&&
		std::copy_constructible<std::remove_cvref_t<U>>
				: polymorphic(std::allocator_arg, a, std::in_place_type<std::remove_cvref_t<U>>, std::forward<U>(u))
				{}

		constexpr polymorphic(polymorphic &&other) noexcept
				: p_(other.release_p()),
				  cb_ptr_(other.release_cb_ptr()) {}

		constexpr polymorphic(std::allocator_arg_t, const Allocator &a, polymorphic &&other)
		noexcept(BaseAllocTraits::is_always_equal::value)
				: cb_ptr_(nullptr, ControlBlockDeleter{a}) {
			if (!other.p_) { return; }

			if constexpr (BaseAllocTraits::is_always_equal::value) {
				p_ = other.release_p();
				cb_ptr_ = other.release_cb_ptr();
				cb_ptr_.get_deleter().alloc = a;
			}
			else {
				if (get_allocator_ref() == other.get_allocator_ref()) {
					p_ = other.release_p();
					cb_ptr_ = other.release_cb_ptr();
				}
				else {
					if (other.p_) {
						polymorphic temp(std::allocator_arg, get_allocator_ref(), other);
						swap(temp);
						other.reset();
					}
					else {
						reset();
					}
				}
			}
		}

		template<typename Derived, typename DerivedAlloc>
		requires std::derived_from<Derived, T> &&
		         std::is_constructible_v<Allocator, const DerivedAlloc &>
		polymorphic(polymorphic<Derived, DerivedAlloc> &&other) // NOLINT(google-explicit-constructor)
				: cb_ptr_(nullptr, ControlBlockDeleter{Allocator(other.get_allocator_ref())}) {
			if (other.p_) {
				try {
					using DelegatingCB = detail::DelegatingControlBlock<T, Derived, Allocator, DerivedAlloc>;
					using CBAlloc = typename BaseAllocTraits::template rebind_alloc<DelegatingCB>;
					using CBAllocTraits = std::allocator_traits<CBAlloc>;

					CBAlloc cb_alloc(get_allocator_ref());
					auto *new_cb = CBAllocTraits::allocate(cb_alloc, 1);

					auto source_cb_ptr = other.cb_ptr_.release();
					auto source_ptr = other.p_;
					other.p_ = nullptr;

					try {
						new(new_cb) detail::DelegatingControlBlock<T, Derived, Allocator, DerivedAlloc>(
								std::unique_ptr<detail::ControlBlockBase<Derived, DerivedAlloc>>(source_cb_ptr),
								source_ptr,
								other.get_allocator_ref(),
								get_allocator_ref()
						);
						cb_ptr_.reset(static_cast<detail::ControlBlockBase<T, Allocator> *>(new_cb));
						p_ = std::pointer_traits<pointer>::pointer_to(*source_cb_ptr->get_ptr());
					} catch (...) {
						if (source_cb_ptr) {
							source_cb_ptr->destroy(other.get_allocator_ref(), source_ptr);
							CBAllocTraits::deallocate(cb_alloc, reinterpret_cast<DelegatingCB *>(source_cb_ptr), 1);
						}
						CBAllocTraits::deallocate(cb_alloc, new_cb, 1);
						throw;
					}
				} catch (...) {
					throw;
				}
			}
		}

		~polymorphic() noexcept {
			if (p_ && cb_ptr_) {
				try {
					if (cb_ptr_->decrement_ref_count()) {
						cb_ptr_->destroy(get_allocator_ref(), p_);
					}
				} catch (...) {
					assert(false && "Control block destruction threw");
					std::terminate();
				}
			}
			p_ = nullptr;
		}

		constexpr polymorphic &operator=(polymorphic &&other) noexcept(
		BaseAllocTraits::propagate_on_container_move_assignment::value ||
		BaseAllocTraits::is_always_equal::value) {
			if (this == &other) return *this;
			constexpr bool pocma = BaseAllocTraits::propagate_on_container_move_assignment::value;
			if constexpr (pocma) {
				reset();
				get_allocator_ref() = std::move(other.get_allocator_ref());
				cb_ptr_.get_deleter().alloc = get_allocator_ref();
				p_ = other.release_p();
				cb_ptr_ = other.release_cb_ptr();
			}
			else {
				if (get_allocator_ref() == other.get_allocator_ref()) {
					reset();
					p_ = other.release_p();
					cb_ptr_ = other.release_cb_ptr();
				}
				else {
					if (other.p_) {
						polymorphic temp(std::allocator_arg, get_allocator_ref(), other);
						swap(temp);
						other.reset();
					}
					else {
						reset();
					}
				}
			}
			return *this;
		}

		constexpr polymorphic &operator=(std::nullptr_t) noexcept {
			reset();
			return *this;
		}

		template<typename Derived, typename DerivedAllocator>
		requires std::derived_from<Derived, T> &&
		         std::is_constructible_v<Allocator, DerivedAllocator &&> &&
		         std::is_assignable_v<Allocator &, Allocator &&>
		constexpr polymorphic &operator=(polymorphic<Derived, DerivedAllocator> &&other)
		noexcept(
		std::is_nothrow_constructible_v<Allocator, DerivedAllocator &&> &&
		std::is_nothrow_move_constructible_v<ControlBlockPtr> &&
		std::is_nothrow_swappable_v<ControlBlockPtr> &&
		(BaseAllocTraits::propagate_on_container_move_assignment::value ||
		 BaseAllocTraits::is_always_equal::value)
		) {
			polymorphic temp(std::move(other));
			swap(temp);
			return *this;
		}

		template<typename U, typename A>
		requires std::derived_from<U, T> &&
		         std::is_convertible_v<A, Allocator>
		operator polymorphic<U, A>() const { //NOLINT
			if (!*this) return nullptr;
			return polymorphic<U, A>(std::in_place_type<U>, **this);
		}

		constexpr const T &operator*() const noexcept {
			assert(has_value());
			return *cb_ptr_->get_ptr();
		}

		constexpr T &operator*() noexcept {
			assert(has_value());
			return *cb_ptr_->get_ptr();
		}

		constexpr const_pointer operator->() const noexcept {
			assert(has_value());
			return std::pointer_traits<const_pointer>::pointer_to(*cb_ptr_->get_ptr());
		}

		constexpr pointer operator->() noexcept {
			assert(has_value());
			return std::pointer_traits<pointer>::pointer_to(*cb_ptr_->get_ptr());
		}

		[[nodiscard]] constexpr bool has_value() const noexcept { return p_ != nullptr; }

		[[nodiscard]] constexpr bool valueless_after_move() const noexcept { return p_ == nullptr; }

		constexpr explicit operator bool() const noexcept { return has_value(); }

		constexpr allocator_type get_allocator() const noexcept { return get_allocator_ref(); }

		[[deprecated("Access via operator* or operator-> is preferred.")]]
		constexpr pointer get() noexcept { return p_; }

		[[deprecated("Access via operator* or operator-> is preferred.")]]
		constexpr const_pointer get() const noexcept { return p_; }

		[[deprecated("Value types typically use assignment for replacement. This deviates from P3019.")]]
		constexpr void reset_public() noexcept { reset(); }

		[[deprecated("Value types manage their own resources; releasing breaks ownership. This deviates from P3019.")]]
		[[nodiscard]] constexpr pointer release_public() noexcept {
			cb_ptr_.reset();
			return std::exchange(p_, nullptr);
		}

		constexpr void swap(polymorphic &other) noexcept(
		BaseAllocTraits::propagate_on_container_swap::value ||
		BaseAllocTraits::is_always_equal::value) {
			if constexpr (BaseAllocTraits::propagate_on_container_swap::value) {
				std::swap(p_, other.p_);
				std::swap(cb_ptr_, other.cb_ptr_);
			}
			else {
				if (get_allocator_ref() != other.get_allocator_ref()) {
					assert(false && "polymorphic::swap requires allocators to be equal when POCS is false");
					return;
				}
				std::swap(p_, other.p_);
				std::swap(cb_ptr_, other.cb_ptr_);
			}
		}

		friend constexpr void swap(polymorphic &lhs, polymorphic &rhs) noexcept(noexcept(lhs.swap(rhs))) {
			lhs.swap(rhs);
		}

		/*
		 * Creates a refcounted shared copy
		 */
		polymorphic share() const noexcept {
			polymorphic result;
			if (cb_ptr_) {
				cb_ptr_->increment_ref_count();
				result.cb_ptr_ = ControlBlockPtr(cb_ptr_.get(), ControlBlockDeleter{cb_ptr_.get_deleter().alloc});
				result.p_ = p_;
			}
			return result;
		}
		/*
		 * Creates a refcounted shared copy but casts to <typename To>
		 */
		template<typename To>
		requires std::is_base_of_v<T, To> || std::is_base_of_v<To, T>
		polymorphic<To> share() const noexcept {
			polymorphic<To> result = share();
			return result;
		}

		template<typename Target>
		[[nodiscard]] bool is_type() const noexcept {
			return has_value() && (cb_ptr_->target_type_info() == typeid(Target));
		}

		template<typename Target>
		[[nodiscard]] polymorphic<Target> dynamic_cast_to() const {
			if (!is_type<Target>()) return nullptr;
			return polymorphic<Target>(*this);
		}

		template<typename Target>
		[[nodiscard]] Target *dynamic_get() const noexcept {
			if constexpr (std::is_base_of_v<T, Target>) {
				auto *result = static_cast<Target *>(std::to_address(p_));
				assert(typeid(*result) == typeid(Target) && "Static cast violated");
				return result;
			}
			else {
				return dynamic_cast<Target *>(std::to_address(p_));
			}
		}

		template<typename Interface>
		std::optional<Interface *> try_as_interface() const {
			//So much extra safety because of a Segfault that wasn't even this funcs fault //to be deleted
			if (!p_) return std::nullopt;
			auto *ptr = std::to_address(p_);
			if (!ptr) return std::nullopt;
#if DEBUG
			std::cout << "Attempting cast to Interface, ptr: " << ptr << std::endl;
#endif
			if (cb_ptr_) {
				const auto &target_type = typeid(Interface);
				const auto &stored_type = cb_ptr_->target_type_info();
#if DEBUG
    std::cout << "Checking type compatibility before cast" << std::endl;
#endif
				if (stored_type != target_type && !std::is_base_of_v<Interface, std::remove_pointer_t<decltype(ptr)>>) {
#if DEBUG
    std::cout << "Type mismatch detected, skipping cast" << std::endl;
#endif
					return std::nullopt;
				}
			}
			try {
				return dynamic_cast<Interface *>(ptr);
			} catch (...) {
#if DEBUG
    std::cout << "Exception caught during cast to Interface" << std::endl;
#endif
				return std::nullopt;
			}
		}

		template<typename To>
		[[nodiscard]] polymorphic<To> cast() &&{
			if (!has_value()) return nullptr;
			if (cb_ptr_->target_type_info() != typeid(To)) {
				throw std::bad_cast();
			}
			return polymorphic<To>(std::move(*this));
		}

		template<typename To>
		[[nodiscard]] polymorphic<To> cast() const &{
			if (!has_value()) return nullptr;
			if (cb_ptr_->target_type_info() != typeid(To)) {
				throw std::bad_cast();
			}
			return polymorphic<To>(*this);
		}

		template <typename To>
		[[nodiscard]] polymorphic<To> unchecked_cast() &&  {
			if (!has_value()) return nullptr;

			polymorphic<To> result(std::allocator_arg, get_allocator_ref());

			if (p_ && cb_ptr_) {
				using SourceCB = detail::ControlBlockBase<T, Allocator>;
				using DelegatingCB = detail::DelegatingControlBlock<To, T, typename polymorphic<To>::allocator_type, Allocator>;
				using CBAlloc = typename std::allocator_traits<typename polymorphic<To>::allocator_type>::template rebind_alloc<DelegatingCB>;
				using CBAllocTraits = std::allocator_traits<CBAlloc>;

				CBAlloc cb_alloc(result.get_allocator_ref());
				auto* new_cb = CBAllocTraits::allocate(cb_alloc, 1);

				try {
					new(new_cb) DelegatingCB(
							std::unique_ptr<SourceCB>(cb_ptr_.release()),
							p_,
							get_allocator_ref(),
							result.get_allocator_ref()
					);
					result.cb_ptr_ = typename polymorphic<To>::ControlBlockPtr(new_cb, typename polymorphic<To>::ControlBlockDeleter{result.get_allocator_ref()});
					result.p_ = std::pointer_traits<typename polymorphic<To>::pointer>::pointer_to(*static_cast<To*>(std::to_address(p_)));
					p_ = nullptr;
				} catch (...) {
					CBAllocTraits::deallocate(cb_alloc, new_cb, 1);
					throw;
				}
			}

			return result;
		}

		template <typename To>
		[[nodiscard]] polymorphic<To> static_cast_wrapper() && {
			static_assert(std::is_base_of_v<T, To> || std::is_base_of_v<To, T>,
			              "Types must be part of the same hierarchy");

			if (!has_value()) return {};

			polymorphic<To> result(std::allocator_arg, get_allocator_ref());

			if (cb_ptr_) {
				using SourceCB = detail::ControlBlockBase<T, Allocator>;
				using TargetCB = detail::ControlBlockBase<To, typename polymorphic<To>::allocator_type>;
				using DelegatingCB = detail::DelegatingControlBlock<To, T, typename polymorphic<To>::allocator_type, Allocator>;

				using CBAlloc = typename std::allocator_traits<typename polymorphic<To>::allocator_type>::template rebind_alloc<DelegatingCB>;
				using CBAllocTraits = std::allocator_traits<CBAlloc>;

				CBAlloc cb_alloc(result.get_allocator_ref());
				auto* new_cb = CBAllocTraits::allocate(cb_alloc, 1);

				try {
					new (new_cb) DelegatingCB(
							std::unique_ptr<SourceCB>(cb_ptr_.release()),
							p_,
							get_allocator_ref(),
							result.get_allocator_ref()
					);

					result.cb_ptr_.reset(static_cast<TargetCB*>(new_cb));
					result.p_ = std::pointer_traits<typename polymorphic<To>::pointer>::pointer_to(
							*static_cast<To*>(std::to_address(p_))
					);
					p_ = nullptr; // Mark moved-from state
				} catch (...) {
					CBAllocTraits::deallocate(cb_alloc, new_cb, 1);
					throw;
				}
			}

			return result;
		}
		template<typename To>
		[[nodiscard]] polymorphic<To> share_static() const {
			using ToAlloc = typename polymorphic<To>::allocator_type;
			using DelegatingCB = detail::DelegatingControlBlock<To, T, ToAlloc, Allocator>;

			polymorphic<To> result(std::allocator_arg, get_allocator_ref());

			if (cb_ptr_) {
				auto* new_cb = new DelegatingCB(
						std::unique_ptr<ControlBlock>(cb_ptr_.get()),
						p_,
						get_allocator_ref(),
						result.get_allocator_ref()
				);

				cb_ptr_->increment_ref_count();
				result.cb_ptr_.reset(new_cb);

				result.p_ = static_cast<typename polymorphic<To>::pointer>(
						std::pointer_traits<pointer>::pointer_to(
								*static_cast<To*>(std::to_address(p_))
						));
			}
			return result;
		}

	};

	template <typename T>
	concept HasBaseType = requires {
		typename T::base_type;
	};


	template <typename ConcreteDerived, typename Base, typename BaseAlloc, typename... Args>
	requires std::derived_from<ConcreteDerived, Base> &&
	         std::constructible_from<ConcreteDerived, Args...> &&
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
	         std::default_initializable<std::allocator<Base>>
	[[nodiscard]] constexpr polymorphic<Base, std::allocator<Base>>
	make_polymorphic(Args&&... args) {
		return polymorphic<Base, std::allocator<Base>>(std::in_place_type<ConcreteDerived>,
		                                               std::forward<Args>(args)...);
	}

	template <typename T, typename Alloc, typename... Args>
	requires std::constructible_from<T, Args...> &&
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

	template <typename T, template <typename...> class Template>
	struct is_specialization_of : std::false_type {};

	template <template <typename...> class Template, typename... Args>
	struct is_specialization_of<Template<Args...>, Template> : std::true_type {};

	template <typename T, template <typename...> class Template>
	inline constexpr bool is_specialization_of_v =
			is_specialization_of<T, Template>::value;
	template <typename T>
	concept general_polymorphic = is_specialization_of_v<T, polymorphic>;

} // namespace std_P3019_modified
