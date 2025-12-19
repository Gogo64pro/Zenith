//
// Created by gogop on 11/28/2025.
//
module;
#include <variant>
import zenith.core.polymorphic;
import zenith.core.polymorphic_ref;
export module zenith.core.polymorphic_variant;

export namespace zenith {
	template<typename T>
	class polymorphic_variant {

	public:
		std::variant<polymorphic<T>, polymorphic_ref<T> > v_;

		template<typename U>
		polymorphic_variant &operator=(U &&value) {
			v_ = std::forward<U>(value);
			return *this;
		}

		polymorphic_variant(const polymorphic_variant &) = delete;

		polymorphic_variant &operator=(const polymorphic_variant &) = delete;

		polymorphic_variant(std::nullptr_t = nullptr) noexcept
		{ std::visit([](auto& elem){elem = nullptr;}, v_); }

		polymorphic_variant& operator=(std::nullptr_t) noexcept {
			std::visit([](auto& elem){elem = nullptr;}, v_);
			return *this;
		}

		polymorphic_variant(polymorphic_variant &&other) noexcept
			: v_(std::move(other.v_)) {
		}

		template<typename U>
		polymorphic_variant(const polymorphic<U>& p)
		: v_(polymorphic_ref<U>(p)) {}

		template<typename U>
		polymorphic_variant(polymorphic<U>&& p)
			: v_(std::move(p)) {}

		template<typename U>
		polymorphic_variant(const polymorphic_ref<U>& r)
			: v_(r) {}

		polymorphic_variant &operator=(polymorphic_variant &&other) noexcept {
			v_ = std::move(other.v_);
			return *this;
		}

		explicit operator bool() const {
			return std::visit([](auto const& elem) -> bool {
				return static_cast<bool>(elem);
			}, v_);
		}

		T &operator*() {
			return std::visit([](auto &elem) -> auto & {
				return elem;
			}, v_);
		}

		const T &operator*() const {
			return std::visit([](auto const &elem) -> const auto & {
				return elem;
			}, v_);
		}

		T *operator->() {
			return std::visit([](auto &elem) {
				return elem.operator->();
			}, v_);
		}

		const T *operator->() const {
			return std::visit([](auto const &elem) {
				return elem.operator->();
			}, v_);
		}

		polymorphic_variant copy_or_share() const {
			return std::visit([]<typename U>(U const& x) -> polymorphic_variant {
				using X = std::decay_t<U>;

				if constexpr (std::is_same_v<X, polymorphic_ref<typename X::value_type>>) {
					return x;
				} else {
					return x.share();
				}
			}, v_);
		}

		polymorphic_ref<T> get_ref() {
			return std::visit([]<typename U>(U& x) -> polymorphic_ref<T> {
				return x;
			}, v_);
		}
	};
}
