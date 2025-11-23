module;
#include <memory>
#include <type_traits>
#include <concepts>
#include <utility>
#include <typeinfo>
#include <optional>

export module zenith.core.polymorphic;

export namespace zenith {
    template<typename T>
    class polymorphic {
        std::shared_ptr<T> ptr_;

        template<typename U>
        friend class polymorphic;

        // Private constructor for sharing with different type
        template<typename U>
            requires std::derived_from<U, T> || std::derived_from<T, U>
        polymorphic(std::shared_ptr<U> ptr) : ptr_(std::move(ptr)) {}

        // Private copy constructor - only used by share()
        polymorphic(const polymorphic& other) : ptr_(other.ptr_) {}

    public:
        using value_type = T;

        // Default constructor
        polymorphic() = default;

        // Nullptr constructor
        polymorphic(std::nullptr_t) {}

        // Construct from derived type
        template<typename U, typename... Args>
            requires std::derived_from<U, T> && std::constructible_from<U, Args...>
        explicit polymorphic(std::in_place_type_t<U>, Args&&... args)
            : ptr_(std::make_shared<U>(std::forward<Args>(args)...)) {}

        // Construct from U
        template<typename U>
            requires std::derived_from<U, T> &&
                     (!std::same_as<std::remove_cvref_t<U>, polymorphic>) &&
                     std::constructible_from<U, U&&>
        polymorphic(U&& value)
            : polymorphic(std::in_place_type<std::remove_cvref_t<U>>, std::forward<U>(value)) {}

        // Converting constructor from derived polymorphic (move only)
        template<typename U>
            requires std::derived_from<U, T>
        polymorphic(polymorphic<U>&& other) : ptr_(std::move(other.ptr_)) {}

        // Move constructor
        polymorphic(polymorphic&&) = default;

        // Move assignment
        polymorphic& operator=(polymorphic&&) = default;

        // Delete public copy assignment to prevent accidental sharing
        polymorphic& operator=(const polymorphic&) = delete;

        // Observers
        explicit operator bool() const noexcept { return static_cast<bool>(ptr_); }
        [[nodiscard]] bool has_value() const noexcept { return static_cast<bool>(ptr_); }

        T* get() noexcept { return ptr_.get(); }
        const T* get() const noexcept { return ptr_.get(); }

        T& operator*() {
            if (!ptr_) throw std::bad_optional_access();
            return *ptr_;
        }

        const T& operator*() const {
            if (!ptr_) throw std::bad_optional_access();
            return *ptr_;
        }

        T* operator->() {
            if (!ptr_) throw std::bad_optional_access();
            return ptr_.get();
        }

        const T* operator->() const {
            if (!ptr_) throw std::bad_optional_access();
            return ptr_.get();
        }

        // Reset
        void reset() noexcept {
            ptr_.reset();
        }

        // Swap
        void swap(polymorphic& other) noexcept {
            ptr_.swap(other.ptr_);
        }

        friend void swap(polymorphic& a, polymorphic& b) noexcept {
            a.swap(b);
        }

        // Type checking
        template<typename U>
        [[nodiscard]] bool is_type() const noexcept {
            return ptr_ && typeid(*ptr_) == typeid(U);
        }

        [[nodiscard]] const std::type_info& type() const noexcept {
            return ptr_ ? typeid(*ptr_) : typeid(void);
        }

        // Share - creates a new shared reference (explicit copy)
        polymorphic share() const {
            return polymorphic(*this); // Uses private copy constructor
        }

        template<typename U>
            requires std::derived_from<U, T> || std::derived_from<T, U>
        polymorphic<U> share() const {
            if (!ptr_) return nullptr;

            // Use shared_ptr's built-in casting
            if constexpr (std::derived_from<U, T>) {
                // U is derived from T - use dynamic_pointer_cast
                return polymorphic<U>(std::dynamic_pointer_cast<U>(ptr_));
            } else {
                // T is derived from U - static cast is safe
                return polymorphic<U>(std::static_pointer_cast<U>(ptr_));
            }
        }

        // Cast builder interface
        class cast_builder {
            polymorphic& self_;
            const polymorphic& const_self_;
            bool is_const_;
            bool checked_ = true;
            bool throw_on_fail_ = true;

        public:
            explicit cast_builder(polymorphic& self)
                : self_(self), const_self_(self), is_const_(false) {}

            explicit cast_builder(const polymorphic& self)
                : self_(const_cast<polymorphic&>(self)), const_self_(self), is_const_(true) {}

            cast_builder&& unchecked() && {
                checked_ = false;
                return std::move(*this);
            }

            cast_builder&& non_throwing() && {
                throw_on_fail_ = false;
                return std::move(*this);
            }

            template<typename To>
            polymorphic<To> to() && {
                if (is_const_) {
                    return do_const_cast<To>();
                }
                return do_non_const_cast<To>();
            }

            template<typename To>
            To* as_ptr() && {
                const auto& current_self = is_const_ ? const_self_ : self_;

                if (!current_self.has_value()) {
                    if (throw_on_fail_) throw std::bad_cast();
                    return nullptr;
                }

                if (checked_) {
                    if (!current_self.template is_type<To>()) {
                        if (throw_on_fail_) throw std::bad_cast();
                        return nullptr;
                    }
                }

                if constexpr (std::is_const_v<To>) {
                    return current_self.ptr_.get();
                } else {
                    return const_cast<To*>(current_self.ptr_.get());
                }
            }

            template<typename To>
            std::optional<polymorphic<To>> as_optional() && {
                try {
                    return std::move(*this).template to<To>();
                } catch (...) {
                    return std::nullopt;
                }
            }

        private:
            template<typename To>
            polymorphic<To> do_const_cast(const bool throw_on_fail = true) {
                if (!const_self_.has_value()) {
                    if (throw_on_fail) throw std::bad_cast();
                    return nullptr;
                }

                if (checked_ && !const_self_.is_type<To>()) {
                    if (throw_on_fail) throw std::bad_cast();
                    return nullptr;
                }

                return polymorphic<To>(std::dynamic_pointer_cast<To>(const_self_.ptr_));
            }

            template<typename To>
            polymorphic<To> do_non_const_cast(bool throw_on_fail = true) {
                if (!self_.has_value()) {
                    if (throw_on_fail) throw std::bad_cast();
                    return nullptr;
                }

                if (checked_ && !self_.is_type<To>()) {
                    if (throw_on_fail) throw std::bad_cast();
                    return nullptr;
                }

                return polymorphic<To>(std::dynamic_pointer_cast<To>(self_.ptr_));
            }
        };

        cast_builder cast() & { return cast_builder(*this); }
        cast_builder cast() const & { return cast_builder(*this); }
        cast_builder cast() && { return cast_builder(*this); }
    };

    // Factory functions
    template<typename Concrete, typename Base = Concrete, typename... Args>
    polymorphic<Base> make_polymorphic(Args&&... args) {
        return polymorphic<Base>(std::in_place_type<Concrete>, std::forward<Args>(args)...);
    }

    template<typename To, typename From>
    polymorphic<To> polymorphic_cast(polymorphic<From>&& p) {
        return std::move(p).template cast<To>().template to<To>();
    }

    template<typename To, typename From>
    polymorphic<To> polymorphic_cast(const polymorphic<From>& p) {
        return p.template cast<To>().template to<To>();
    }
}