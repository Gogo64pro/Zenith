//
// Created by gogop on 4/29/2025.
//

#pragma once

#include <memory>
#include <iostream>
#include <cassert>

namespace zenith {
//
//	struct Cloneable{
//		virtual ~Cloneable() = default;
//		[[nodiscard]] virtual std::unique_ptr<Cloneable> clone() const{
//			return do_clone();
//		};
//
//	protected:
//		[[nodiscard]] virtual std::unique_ptr<Cloneable> do_clone() const = 0; //OOOOooh scary, inconsistent camel and snake case
//	};
//
//template<typename Base>
//	class polymorphic {
//		template<typename T>
//		static constexpr bool is_valid_base_v = std::is_base_of_v<Cloneable, Base>;
//		static_assert(is_valid_base_v<Base>, "Base type must inherit from Cloneable");
//
//		template<typename T>
//		static constexpr bool is_valid_derived_type_v = std::is_base_of_v<Base, T>;
//	public:
//		std::unique_ptr<Base> ptr_; // The managed pointer
//
//		// --- Static Factory ---
//		template<typename T, typename... Args>
//		[[nodiscard("make creates a new object, assign the result.")]]
//		static polymorphic make(Args&&... args) {
//			static_assert(is_valid_derived_type_v<T>, "T must derive from Base");
//			// Directly construct polymorphic via the explicit unique_ptr constructor
//			return polymorphic(std::make_unique<T>(std::forward<Args>(args)...));
//		}
//
//		// --- Constructors ---
//
//		// Default constructor (empty)
//		polymorphic() = default;
//
//		//Nullptr ctor
//		polymorphic(std::nullptr_t) noexcept : ptr_(nullptr) {
//			// std::cout << "DEBUG: polymorphic nullptr constructor invoked!" << std::endl;
//		}
//
//		// Constructor taking ownership of a pointer
//		explicit polymorphic(std::unique_ptr<Base> p) : ptr_(std::move(p)) {
//			// Optional: Assertion can be helpful during debugging
//			// assert(!ptr_ || dynamic_cast<const Cloneable*>(ptr_.get()) && "Internal check failed: Base instance doesn't seem to implement Cloneable");
//		}
//
//		// --- Move Semantics (Cheap) ---
//		polymorphic(polymorphic&& other) noexcept = default;
//		polymorphic& operator=(polymorphic&& other) noexcept = default;
//
//		// --- Copy Semantics (Potentially Expensive - Deep Copy) ---
//		polymorphic(const polymorphic& other) {
//			// std::cout << "DEBUG: polymorphic copy constructor invoked!" << std::endl;
//			if (other.ptr_) {
//				// Cloneable interface guarantees clone() exists.
//				// No dynamic_cast needed here if Base *is* Cloneable or directly inherits.
//				// But if Base is intermediate, dynamic_cast might be safer. Let's assume Base inherits Cloneable.
//				auto* cloneable_ptr = static_cast<const Cloneable*>(other.ptr_.get()); // Safe static_cast if Base inherits Cloneable
//				ptr_ = std::unique_ptr<Base>(static_cast<Base*>(cloneable_ptr->clone().release()));
//				if (!ptr_) { /* Handle cloning failure? Throw? */ }
//			} else {
//				ptr_.reset();
//			}
//		}
//
//		polymorphic& operator=(const polymorphic& other) {
//			if (this == &other) {
//				return *this;
//			}
//			// std::cout << "DEBUG: polymorphic copy assignment invoked!" << std::endl;
//			// Use copy-and-swap idiom for exception safety and code reuse
//			polymorphic temp(other); // Calls copy constructor
//			*this = std::move(temp); // Calls move assignment
//			return *this;
//		}
//
//		//     This allows assigning from std::make_unique<Derived>(),
//		//     as unique_ptr<Derived> converts implicitly to unique_ptr<Base>.
//		polymorphic& operator=(std::unique_ptr<Base> p) {
//			// std::cout << "DEBUG: polymorphic unique_ptr assignment invoked!" << std::endl;
//			ptr_ = std::move(p); // Take ownership by moving from the parameter
//			// Optional: Add assertion back if desired
//			// assert(!ptr_ || dynamic_cast<const Cloneable*>(ptr_.get()) && "Internal check failed: Assigned pointer's type doesn't seem to implement Cloneable");
//			return *this; // Return *this to allow chaining
//		}
//
//
//		polymorphic& operator=(std::nullptr_t) noexcept {
//			// std::cout << "DEBUG: polymorphic nullptr assignment invoked!" << std::endl;
//			ptr_.reset(); // Reset the unique_ptr, making it null
//			return *this;
//		}
//
//		// --- Accessors ---
//		explicit operator bool() const noexcept { return static_cast<bool>(ptr_); }
//		Base* operator->() { return ptr_.get(); }
//		const Base* operator->() const { return ptr_.get(); }
//		Base& operator*() { assert(ptr_); return *ptr_; } // Add assert for safety
//		const Base& operator*() const { assert(ptr_); return *ptr_; } // Add assert for safety
//		Base* get() { return ptr_.get(); }
//		const Base* get() const { return ptr_.get(); }
//		std::unique_ptr<Base> release() { return std::move(ptr_); } // Give up ownership
//
//
//		// Optional: Provide an explicit clone method if you want a non-deprecated way
//		[[nodiscard("Clone creates a new object, assign the result.")]]
//		polymorphic clone() const {
//			// std::cout << "DEBUG: polymorphic explicit clone() invoked!" << std::endl;
//			// Delegate to the copy constructor which does the actual work
//			return polymorphic(*this);
//		}
//	};

} // zenith
