//
// Created by gogop on 11/28/2025.
//
#pragma once

#include <optional>
#include <variant>
#include "polymorphic.hpp"
#include "polymorphic_ref.hpp"

namespace zenith {
	template<typename T>
	class polymorphic_variant {
	public:
		std::variant<polymorphic<T>, polymorphic_ref<T> > v_;

		template<typename U>
		polymorphic_variant &operator=(U &&value)
		requires (!std::same_as<std::remove_cvref_t<U>, polymorphic_variant> &&
			  (std::convertible_to<U&&, polymorphic<T>> ||
			   std::convertible_to<U&&, polymorphic_ref<T>>)) {
			if constexpr (std::is_convertible_v<U&&, polymorphic<T>>) {
				v_.template emplace<0>(std::forward<U>(value));
			} else if constexpr (std::is_convertible_v<U&&, polymorphic_ref<T>>) {
				v_.template emplace<1>(std::forward<U>(value));
			} else {
				static_assert(sizeof(U) == 0, "Cannot assign this type to polymorphic_variant");
			}
			return *this;
		}

		polymorphic_variant(const polymorphic_variant &) = delete;

		polymorphic_variant &operator=(const polymorphic_variant &) = delete;

		polymorphic_variant(std::nullptr_t = nullptr) noexcept { std::visit([](auto &elem) { elem = nullptr; }, v_); }

		polymorphic_variant &operator=(std::nullptr_t) noexcept {
			std::visit([](auto &elem) { elem = nullptr; }, v_);
			return *this;
		}

		polymorphic_variant(polymorphic_variant &&other) noexcept
			: v_(std::move(other.v_)) {
		}

		template<typename U>
		polymorphic_variant(const polymorphic<U> &p)
			: v_(polymorphic_ref<U>(p)) {
		}

		template<typename U>
		polymorphic_variant(polymorphic<U> &&p)
			: v_(std::in_place_type<polymorphic<T>>, std::move(p)) {
		}

		template<typename U>
		polymorphic_variant(const polymorphic_ref<U> &r)
			: v_(r) {
		}

		polymorphic_variant &operator=(polymorphic_variant &&other) noexcept {
			v_ = std::move(other.v_);
			return *this;
		}

		explicit operator bool() const {
			return std::visit([](auto const &elem) -> bool {
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
			return std::visit([]<typename U>(U const &x) -> polymorphic_variant {
				using X = std::decay_t<U>;

				if constexpr (std::is_same_v<X, polymorphic_ref<typename X::value_type> >) {
					return x;
				}
				else {
					return x.share();
				}
			}, v_);
		}

		operator const polymorphic_ref<T>() const & {
			return std::visit([](auto &x) -> auto{
				return polymorphic_ref<T>(x);
			}, v_);
		}

		polymorphic_ref<T> get_ref() {
			return std::visit([]<typename U>(U &x) -> polymorphic_ref<T> {
				return x;
			}, v_);
		}

		polymorphic_ref<T> get_ref() const {
			return std::visit([]<typename U>(U const &x) -> polymorphic_ref<T> {
				return x;
			}, v_);
		}

		template<typename V>
		class variant_cast_builder {
		public:
			using polymorphic_builder = polymorphic<V>::cast_builder;
			using ref_builder = polymorphic_ref<V>::cast_builder;

			std::variant<polymorphic_builder, ref_builder> builder_;

			explicit variant_cast_builder(polymorphic_builder &&b)
				: builder_(std::move(b)) {
			}

			explicit variant_cast_builder(ref_builder &&b)
				: builder_(std::move(b)) {
			}

			template<typename To>
			polymorphic_variant<To> to() && {
				return std::visit([](auto &&b) -> polymorphic_variant<To> {
					auto result = std::forward<decltype(b)>(b).template to<To>();


					return polymorphic_variant<To>{std::move(result)};
				}, std::move(builder_));
			}

			template<typename To>
			To *as_ptr() && {
				return std::visit([](auto &&b) -> To * {
					return std::forward<decltype(b)>(b).template as_ptr<To>();
				}, std::move(builder_));
			}

			template<typename To>
			std::optional<polymorphic<To> > as_optional() && {
				return std::visit([](auto &&b) -> std::optional<polymorphic<To> > {
					return std::forward<decltype(b)>(b).template as_optional<To>();
				}, std::move(builder_));
			}

			variant_cast_builder &&unchecked() && {
				std::visit([](auto &&b) {
					std::forward<decltype(b)>(b).unchecked();
				}, std::move(builder_));
				return std::move(*this);
			}

			variant_cast_builder &&non_throwing() && {
				std::visit([](auto &&b) {
					std::forward<decltype(b)>(b).non_throwing();
				}, std::move(builder_));
				return std::move(*this);
			}
		};


		auto cast() {
			return std::visit([](auto &elem) -> decltype(auto) {
				return variant_cast_builder<T>(elem.cast());
			}, v_);
		}

		auto cast() const {
			return std::visit([](auto const &elem) -> decltype(auto) {
				return variant_cast_builder<T>(elem.cast());
			}, v_);
		}

		template<typename U>
		[[nodiscard]] bool is_type() const {
			return std::visit([](auto const &elem) -> bool {
				return elem.template is_type<U>();
			}, v_);
		}
	};
}
