//
// Created by gogop on 7/29/2025.
//

#include "../core/indirect_polymorphic.hpp"
#include <gtest/gtest.h>

// Base and derived classes for testing
class Base {
public:
	virtual ~Base() = default;
	[[nodiscard]] virtual int value() const = 0;
	[[nodiscard]] virtual const char* name() const = 0;
};

class Derived1 : public Base {
	int val;
public:
	explicit Derived1(int v) : val(v) {}
	[[nodiscard]] int value() const override { return val; }
	[[nodiscard]] const char* name() const override { return "Derived1"; }
};

class Derived2 : public Base {
	int val;
public:
	explicit Derived2(int v) : val(v) {}
	[[nodiscard]] int value() const override { return val * 2; }
	[[nodiscard]] const char* name() const override { return "Derived2"; }
};

// Fixture class for common setup
class PolymorphicTest : public ::testing::Test {
protected:
	void SetUp() override {
		// Can initialize common test objects here
	}
};

// ----------------------------------------------------------------------------
// Construction Tests
// ----------------------------------------------------------------------------

TEST_F(PolymorphicTest, DefaultConstruction) {
std_P3019_modified::polymorphic<Base> p;
EXPECT_FALSE(p.has_value());
EXPECT_TRUE(p.valueless_after_move());
}

TEST_F(PolymorphicTest, InPlaceConstruction) {
auto p = std_P3019_modified::polymorphic<Base>(std::in_place_type<Derived1>, 42);
EXPECT_TRUE(p.has_value());
EXPECT_EQ(p->value(), 42);
EXPECT_STREQ(p->name(), "Derived1");
}

TEST_F(PolymorphicTest, AllocatorAwareConstruction) {
std::allocator<Base> alloc;
auto p = std_P3019_modified::polymorphic<Base>(
		std::allocator_arg, alloc,
		std::in_place_type<Derived2>, 10
);
EXPECT_TRUE(p.has_value());
EXPECT_EQ(p->value(), 20);  // Derived2 doubles the value
}

TEST_F(PolymorphicTest, MoveConstruction) {
auto p1 = std_P3019_modified::make_polymorphic<Derived1>(15);
auto p2 = std::move(p1);

EXPECT_FALSE(p1.has_value());  // NOLINT(bugprone-use-after-move)
EXPECT_TRUE(p2.has_value());
EXPECT_EQ(p2->value(), 15);
}

// ----------------------------------------------------------------------------
// Assignment and Move Tests
// ----------------------------------------------------------------------------

TEST_F(PolymorphicTest, MoveAssignment) {
auto p1 = std_P3019_modified::make_polymorphic<Derived1>(30);
std_P3019_modified::polymorphic<Base> p2;

p2 = std::move(p1);

EXPECT_FALSE(p1.has_value());  // NOLINT(bugprone-use-after-move)
EXPECT_TRUE(p2.has_value());
EXPECT_EQ(p2->value(), 30);
}

TEST_F(PolymorphicTest, NullAssignment) {
auto p = std_P3019_modified::make_polymorphic<Derived1>(5);
p = nullptr;

EXPECT_FALSE(p.has_value());
}

// ----------------------------------------------------------------------------
// Type Safety and Polymorphism Tests
// ----------------------------------------------------------------------------

TEST_F(PolymorphicTest, DynamicTypeChecking) {
auto p = std_P3019_modified::make_polymorphic<Derived1>(8);

EXPECT_TRUE(p.is_type<Derived1>());
EXPECT_FALSE(p.is_type<Derived2>());

auto p2 = std_P3019_modified::make_polymorphic<Derived2>(8);
EXPECT_TRUE(p2.is_type<Derived2>());
}

//TEST_F(PolymorphicTest, DynamicCast) {
//	auto p = std_P3019_modified::make_polymorphic<Derived1>(12);
//
//	auto derived = p.dynamic_cast_to<Derived1>();
//	EXPECT_TRUE(derived.has_value());
//	EXPECT_EQ(derived->value(), 12);
//
//	EXPECT_THROW(p.dynamic_cast_to<Derived2>(), std::bad_cast);
//}

TEST_F(PolymorphicTest, TryAsInterface) {
auto p = std_P3019_modified::make_polymorphic<Derived2>(7);

auto basePtr = p.try_as_interface<Base>();
ASSERT_TRUE(basePtr.has_value());
EXPECT_EQ((*basePtr)->value(), 14);  // Derived2 doubles

// Try with unrelated type
struct Unrelated {};
auto unrelated = p.try_as_interface<Unrelated>();
EXPECT_FALSE(unrelated.has_value());
}

// ----------------------------------------------------------------------------
// Memory Management Tests
// ----------------------------------------------------------------------------

TEST_F(PolymorphicTest, SharedBehavior) {
auto p1 = std_P3019_modified::make_polymorphic<Derived1>(100);
auto p2 = p1.share();

EXPECT_TRUE(p1.has_value());
EXPECT_TRUE(p2.has_value());
EXPECT_EQ(p1->value(), 100);
EXPECT_EQ(p2->value(), 100);

// Modify through one reference
(void)const_cast<Derived1&>(*p1).value();  // If mutable, could test modification

// Both should see the change (if mutable)
EXPECT_EQ(p1->value(), 100);
EXPECT_EQ(p2->value(), 100);
}

TEST_F(PolymorphicTest, Destruction) {
static int destruction_count = 0;

struct Tracked : Base {
	~Tracked() override { destruction_count++; }
	[[nodiscard]] int value() const override { return 0; }
	[[nodiscard]] const char* name() const override { return "Tracked"; }
};

{
auto p = std_P3019_modified::make_polymorphic<Tracked>();
EXPECT_EQ(destruction_count, 0);
}
EXPECT_EQ(destruction_count, 1);
}

// ----------------------------------------------------------------------------
// Advanced Type Conversion Tests
// ----------------------------------------------------------------------------

//TEST_F(PolymorphicTest, StaticCastWrapper) {
//	auto p1 = std_P3019_modified::make_polymorphic<Derived1>(25);
//	auto p2 = std::move(p1).static_cast_wrapper<Derived1>();
//
//	EXPECT_TRUE(p2.has_value());
//	EXPECT_EQ(p2->value(), 25);
//}

TEST_F(PolymorphicTest, UncheckedCast) {
auto p1 = std_P3019_modified::make_polymorphic<Derived1>(33);
auto p2 = std::move(p1).unchecked_cast<Derived1>();

EXPECT_TRUE(p2.has_value());
EXPECT_EQ(p2->value(), 33);
}

// ----------------------------------------------------------------------------
// Factory Function Tests
// ----------------------------------------------------------------------------

TEST_F(PolymorphicTest, MakePolymorphic) {
auto p = std_P3019_modified::make_polymorphic<Derived2>(9);
EXPECT_TRUE(p.has_value());
EXPECT_EQ(p->value(), 18);
}

//TEST_F(PolymorphicTest, MakePolymorphicAlloc) {
//	std::allocator<Base> alloc;
//	auto p = std_P3019_modified::make_polymorphic_alloc<Derived1>(
//			std::allocator_arg, alloc, 13
//	);
//	EXPECT_TRUE(p.has_value());
//	EXPECT_EQ(p->value(), 13);
//}

// ----------------------------------------------------------------------------
// Edge Cases and Error Handling
// ----------------------------------------------------------------------------

TEST_F(PolymorphicTest, NullptrHandling) {
std_P3019_modified::polymorphic<Base> p(nullptr);
EXPECT_FALSE(p.has_value());

p = std_P3019_modified::make_polymorphic<Derived1>(1);
p = nullptr;
EXPECT_FALSE(p.has_value());
}

TEST_F(PolymorphicTest, MoveFromNull) {
std_P3019_modified::polymorphic<Base> p1;
std_P3019_modified::polymorphic<Base> p2 = std::move(p1);

EXPECT_FALSE(p1.has_value());  // NOLINT(bugprone-use-after-move)
EXPECT_FALSE(p2.has_value());
}
