//
// Created by gogop on 7/29/2025.
//

#include <gtest/gtest.h>
#include "../core/polymorphic.cppm"
#include "../lexer/lexer.hpp"
#include "../parser/parser.hpp"



class ParserTest : public ::testing::Test {
protected:
	[[nodiscard]] static auto parse(const std::string& source){
		zenith::Lexer lexer(source, "Test");
		auto tokens = std::move(lexer).tokenize();
		zenith::Parser parser(std::move(tokens), Flags{
			.bracesRequired = true,
			.target = Target::native
		});
		return parser.parse();
	}
};
class VarTest : public ParserTest {

};

TEST_F(VarTest, GlobalStaticVarDecl) {
	auto parsed = parse(R"(
        int x = 42;
    )");

	ASSERT_EQ(parsed->declarations.size(), 1);
	auto varDecl = std::move(parsed->declarations[0]).unchecked_cast<VarDeclNode>();
	ASSERT_TRUE(varDecl.has_value());
	EXPECT_EQ(varDecl->kind, VarDeclNode::STATIC);
	EXPECT_EQ(varDecl->name, "x");
	EXPECT_FALSE(varDecl->isHoisted);
	ASSERT_TRUE(varDecl->initializer.has_value());
}

TEST_F(VarTest, GlobalDynamicLetDecl) {
	auto parsed = parse(R"(
        let y = "hello";
    )");

	ASSERT_EQ(parsed->declarations.size(), 1);
	auto varDecl = std::move(parsed->declarations[0]).unchecked_cast<VarDeclNode>();
	ASSERT_TRUE(varDecl.has_value());
	EXPECT_EQ(varDecl->kind, VarDeclNode::DYNAMIC);
	EXPECT_EQ(varDecl->name, "y");
	ASSERT_TRUE(varDecl->initializer.has_value());
}

TEST_F(VarTest, GlobalHoistedVarDecl) {
	auto parsed = parse(R"(
        hoist var z;
    )");

	ASSERT_EQ(parsed->declarations.size(), 1);
	auto varDecl = std::move(parsed->declarations[0]).unchecked_cast<VarDeclNode>();
	ASSERT_TRUE(varDecl.has_value());
	EXPECT_EQ(varDecl->kind, VarDeclNode::DYNAMIC);
	EXPECT_EQ(varDecl->name, "z");
	EXPECT_TRUE(varDecl->isHoisted);
	EXPECT_EQ(varDecl->initializer.has_value(), false);
}

TEST_F(VarTest, GlobalArrayDecl) {
	auto parsed = parse(R"(
        int arr[10];
    )");

	ASSERT_EQ(parsed->declarations.size(), 1);
	auto varDecl = std::move(parsed->declarations[0]).unchecked_cast<VarDeclNode>();
	ASSERT_TRUE(varDecl.has_value());
	EXPECT_EQ(varDecl->kind, VarDeclNode::STATIC);
	EXPECT_EQ(varDecl->name, "arr");
	ASSERT_TRUE(varDecl->type.has_value());

	// Check if typeNode is ArrayTypeNode
	auto arrayType = std::move(varDecl->type).unchecked_cast<ArrayTypeNode>();
	ASSERT_TRUE(arrayType.has_value());
}

TEST_F(VarTest, GlobalTypedDynamicWithAnnotation) {
	auto parsed = parse(R"(
        let name: string = "world";
    )");

	ASSERT_EQ(parsed->declarations.size(), 1);
	auto varDecl = std::move(parsed->declarations[0]).unchecked_cast<VarDeclNode>();
	ASSERT_TRUE(varDecl.has_value());
	EXPECT_EQ(varDecl->kind, VarDeclNode::DYNAMIC);
	EXPECT_EQ(varDecl->name, "name");
	ASSERT_TRUE(varDecl->type.has_value());
	ASSERT_TRUE(varDecl->initializer.has_value());
}

TEST_F(VarTest, GlobalClassInitialization) {
	auto parsed = parse(R"(
        MyClass obj = new MyClass();
    )");

	ASSERT_EQ(parsed->declarations.size(), 1);
	auto varDecl = std::move(parsed->declarations[0]).unchecked_cast<VarDeclNode>();
	ASSERT_TRUE(varDecl.has_value());
	EXPECT_EQ(varDecl->kind, VarDeclNode::CLASS_INIT);
	EXPECT_EQ(varDecl->name, "obj");
	ASSERT_TRUE(varDecl->initializer.has_value());
}

TEST_F(VarTest, MultipleGlobalVarDecls) {
	auto parsed = parse(R"(
        int a = 1;
        let b = 2;
        hoist var c;
        string d[5];
    )");

	ASSERT_EQ(parsed->declarations.size(), 4);

	// First declaration
	auto varDecl1 = std::move(parsed->declarations[0]).unchecked_cast<VarDeclNode>();
	ASSERT_TRUE(varDecl1.has_value());
	EXPECT_EQ(varDecl1->name, "a");
	EXPECT_EQ(varDecl1->kind, VarDeclNode::STATIC);

	// Second declaration
	auto varDecl2 = std::move(parsed->declarations[1]).unchecked_cast<VarDeclNode>();
	ASSERT_TRUE(varDecl2.has_value());
	EXPECT_EQ(varDecl2->name, "b");
	EXPECT_EQ(varDecl2->kind, VarDeclNode::DYNAMIC);

	// Third declaration
	auto varDecl3 = std::move(parsed->declarations[2]).unchecked_cast<VarDeclNode>();
	ASSERT_TRUE(varDecl3.has_value());
	EXPECT_EQ(varDecl3->name, "c");
	EXPECT_TRUE(varDecl3->isHoisted);

	// Fourth declaration (array)
	auto varDecl4 = std::move(parsed->declarations[3]).unchecked_cast<VarDeclNode>();
	ASSERT_TRUE(varDecl4.has_value());
	EXPECT_EQ(varDecl4->name, "d");
	auto arrayType = std::move(varDecl4->type).unchecked_cast<ArrayTypeNode>();
	ASSERT_TRUE(arrayType.has_value());
}

int main(int argc, char **argv) {
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}