module;
#include <utility>
#include <typeinfo>
#include <optional>
import zenith.core.polymorphic;
export module zenith.core.polymorphic_ref;

export namespace zenith {
    template<typename T>
    class polymorphic_ref {
        T* ptr_ = nullptr;
        bool owns = false;
    public:
        using value_type = T;

        template<typename U>
        friend class polymorphic_ref;

        polymorphic_ref(T& ref) noexcept : ptr_(&ref) {}
        template<typename U>
        requires std::derived_from<U, T> || std::derived_from<T, U>
        polymorphic_ref(polymorphic<U>& other) noexcept : ptr_(other.get()) {}

        ~polymorphic_ref() {
            if (owns && ptr_) delete ptr_;
        }

        // Construct from pointer
        explicit polymorphic_ref(T* ptr, const bool owns = false) noexcept : ptr_(ptr), owns(owns) {}

        polymorphic_ref() = default;

        polymorphic_ref(std::nullptr_t) {}

        polymorphic_ref(const polymorphic_ref& other)
        : ptr_(other.ptr_) {}

        polymorphic_ref(polymorphic_ref&& other) noexcept
            : ptr_(other.ptr_), owns(other.owns) {
            other.ptr_ = nullptr;
            other.owns = false;
        }

        polymorphic_ref& operator=(const polymorphic_ref& other) {
            if (this != &other) {
                if (owns && ptr_) {
                    delete ptr_;
                }
                ptr_  = other.ptr_;
                owns = false;
            }
            return *this;
        }
        polymorphic_ref& operator=(polymorphic_ref&& other) noexcept {
            if (this != &other) {
                if (owns && ptr_) {
                    delete ptr_;
                }
                ptr_  = other.ptr_;
                owns = other.owns;
                other.ptr_ = nullptr;
                other.owns = false;
            }
            return *this;
        }

        template<typename U>
        requires (std::derived_from<U, T> || std::derived_from<T, U>)
        polymorphic_ref(polymorphic_ref<U>&& other) noexcept
            : ptr_(other.get()), owns(other.owns) {
            other.ptr_ = nullptr;
            other.owns = false;
        }
        template<typename U>
        requires (std::derived_from<U, T> || std::derived_from<T, U>)
        polymorphic_ref& operator=(polymorphic_ref<U>&& other) noexcept {
            if (this->ptr_ != other.ptr_) {
                if (owns && ptr_) delete ptr_;

                ptr_  = other.get();
                owns = other.owns;

                other.ptr_ = nullptr;
                other.owns = false;
            }
            return *this;
        }

        template<typename U>
        requires (std::derived_from<U, T> || std::derived_from<T, U>)
        polymorphic_ref(const polymorphic_ref<U>& other) noexcept
            : ptr_(other.get()) {}

        // Observers
        explicit operator bool() const noexcept { return ptr_ != nullptr; }
        [[nodiscard]] bool has_value() const noexcept { return ptr_ != nullptr; }

        explicit operator T&() const noexcept { return *ptr_; }


        T* get() noexcept { return ptr_; }
        const T* get() const noexcept { return ptr_; }

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
            return ptr_;
        }

        const T* operator->() const {
            if (!ptr_) throw std::bad_optional_access();
            return ptr_;
        }

        // Type checking
        template<typename U>
        [[nodiscard]] bool is_type() const noexcept {
            return ptr_ && typeid(*ptr_) == typeid(U);
        }

        [[nodiscard]] const std::type_info& type() const noexcept {
            return ptr_ ? typeid(*ptr_) : typeid(void);
        }

        class cast_builder {
            polymorphic_ref& self_;
            const polymorphic_ref& const_self_;
            bool is_const_;
            bool checked_ = true;
            bool throw_on_fail_ = true;

        public:
            explicit cast_builder(polymorphic_ref& self)
                : self_(self), const_self_(self), is_const_(false) {}

            explicit cast_builder(const polymorphic_ref& self)
                : self_(const_cast<polymorphic_ref&>(self)), const_self_(self), is_const_(true) {}

            cast_builder&& unchecked() && {
                checked_ = false;
                return std::move(*this);
            }

            cast_builder&& non_throwing() && {
                throw_on_fail_ = false;
                return std::move(*this);
            }

            template<typename To>
            polymorphic_ref<To> to() && {
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

                return static_cast<To*>(current_self.ptr_);
            }

            template<typename To>
            std::optional<polymorphic_ref<To>> as_optional() && {
                try {
                    if (is_const_) {
                        return do_const_cast<To>(false);
                    }
                    return do_non_const_cast<To>(false);
                } catch (...) {
                    return std::nullopt;
                }
            }

        private:
            template<typename To>
            polymorphic_ref<To> do_const_cast(const bool throw_on_fail = true) {
                if (!const_self_.ptr_) {
                    if (throw_on_fail) throw std::bad_cast();
                    return nullptr;
                }
                To* casted = nullptr;
                if (checked_) {
                    casted = dynamic_cast<To*>(const_self_.ptr_);
                    if (!casted) {
                        if (throw_on_fail) throw std::bad_cast();
                        return nullptr;
                    }
                } else {
                    casted = static_cast<To*>(const_cast<T*>(const_self_.ptr_));
                }
                return polymorphic_ref<To>(casted);
            }

            template<typename To>
            polymorphic_ref<To> do_non_const_cast(const bool throw_on_fail = true) {
                if (!self_.ptr_) {
                    if (throw_on_fail) throw std::bad_cast();
                    return nullptr;
                }
                To* casted = nullptr;
                if (checked_) {
                    casted = dynamic_cast<To*>(self_.ptr_);
                    if (!casted) {
                        if (throw_on_fail) throw std::bad_cast();
                        return nullptr;
                    }
                } else {
                    casted = static_cast<To*>(self_.ptr_);
                }

                return polymorphic_ref<To>(casted);
            }
        };

        cast_builder cast() & { return cast_builder(*this); }
        cast_builder cast() const & { return cast_builder(*this); }
        cast_builder cast() && { return cast_builder(*this); }
    };

    template<typename T>
    polymorphic_ref<T> make_polymorphic_ref(T& ref) noexcept {
        return polymorphic_ref<T>(ref);
    }

    template<typename T>
    polymorphic_ref<const T> make_polymorphic_ref(const T& ref) noexcept {
        return polymorphic_ref<const T>(ref);
    }

    template<typename T, typename... Args>
    polymorphic_ref<T> make_polymorphic_ref_owns(Args&& ...args) noexcept {
        return polymorphic_ref<T>(new T(std::forward<Args>(args)...), true);
    }

    template<typename To, typename From>
    polymorphic_ref<To> polymorphic_cast(polymorphic_ref<From>&& p) {
        return std::move(p).template cast<To>().template to<To>();
    }

    template<typename To, typename From>
    polymorphic_ref<To> polymorphic_cast(const polymorphic_ref<From>& p) {
        return p.template cast<To>().template to<To>();
    }
}