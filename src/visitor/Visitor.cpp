//
// Created by gogop on 4/25/2025.
//

module;
#include <stdexcept>
#if defined(__clang__) || defined(__GNUC__)
#include <cxxabi.h>
#include <cstdlib>
#endif
module zenith.ast;
import zenith.core.polymorphic_ref;
inline std::string type_name(const std::type_info& ti) {
#if (defined(__clang__) || defined(__GNUC__)) && !defined(_MSC_VER)
	// assume itanium abi idk if intel compiler defines this
	int status = 0;
	char* demangled = abi::__cxa_demangle(ti.name(), nullptr, nullptr, &status);
	std::string result = (status == 0 && demangled) ? demangled : ti.name();
	std::free(demangled);
	return result;
#else
	//assume msvc abi
	return ti.name();
#endif
}

namespace zenith{

	void Visitor::visit(ASTNode &node) {
		throw std::runtime_error("Unhandled AST node type " + type_name(typeid(node)));
	}

	void Visitor::visit(ImportNode& node) {
		visit(static_cast<ASTNode&>(node));
	}

	void Visitor::visit(VarDeclNode& node) {
		visit(static_cast<ASTNode&>(node));
	}

	void Visitor::visit(FunctionDeclNode& node) {
		visit(static_cast<ASTNode&>(node));
	}

	void Visitor::visit(LambdaNode& node) {
		visit(static_cast<FunctionDeclNode&>(node));
	}

	void Visitor::visit(MemberDeclNode& node) {
		visit(static_cast<ASTNode&>(node));
	}

	void Visitor::visit(OperatorOverloadNode& node) {
		visit(static_cast<ASTNode&>(node));
	}

	void Visitor::visit(ObjectDeclNode& node) {
		visit(static_cast<ASTNode&>(node));
	}

	void Visitor::visit(UnionDeclNode& node) {
		visit(static_cast<ASTNode&>(node));
	}

	void Visitor::visit(ActorDeclNode& node) {
		visit(static_cast<ObjectDeclNode&>(node));
	}

	void Visitor::visit(MultiVarDeclNode& node) {
		visit(static_cast<StmtNode&>(node));
	}

	void Visitor::visit(BlockNode& node) {
		visit(static_cast<StmtNode&>(node));
	}

	void Visitor::visit(IfNode& node) {
		visit(static_cast<StmtNode&>(node));
	}

	void Visitor::visit(WhileNode& node) {
		visit(static_cast<StmtNode&>(node));
	}

	void Visitor::visit(ForNode& node) {
		visit(static_cast<StmtNode&>(node));
	}

	void Visitor::visit(UnsafeNode& node) {
		visit(static_cast<BlockNode&>(node));
	}

	void Visitor::visit(CompoundStmtNode& node) {
		visit(static_cast<StmtNode&>(node));
	}

	void Visitor::visit(ExprStmtNode& node) {
		visit(static_cast<StmtNode&>(node));
	}

	void Visitor::visit(ReturnStmtNode& node) {
		visit(static_cast<StmtNode&>(node));
	}

	void Visitor::visit(LiteralNode& node) {
		visit(static_cast<ExprNode&>(node));
	}

	void Visitor::visit(VarNode& node) {
		visit(static_cast<ExprNode&>(node));
	}

	void Visitor::visit(BinaryOpNode& node) {
		visit(static_cast<ExprNode&>(node));
	}

	void Visitor::visit(UnaryOpNode& node) {
		visit(static_cast<ExprNode&>(node));
	}

	void Visitor::visit(CallNode& node) {
		visit(static_cast<ExprNode&>(node));
	}

	void Visitor::visit(MemberAccessNode& node) {
		visit(static_cast<ExprNode&>(node));
	}

	void Visitor::visit(FreeObjectNode& node) {
		visit(static_cast<ExprNode&>(node));
	}

	void Visitor::visit(ArrayAccessNode& node) {
		visit(static_cast<ExprNode&>(node));
	}

	void Visitor::visit(NewExprNode& node) {
		visit(static_cast<ExprNode&>(node));
	}

	void Visitor::visit(TemplateStringNode& node) {
		visit(static_cast<ExprNode&>(node));
	}

	void Visitor::visit(ThisNode& node) {
		visit(static_cast<ExprNode&>(node));
	}

	void Visitor::visit(StructInitializerNode& node) {
		visit(static_cast<ExprNode&>(node));
	}

	void Visitor::visit(LambdaExprNode& node) {
		visit(static_cast<ExprNode&>(node));
	}

	void Visitor::visit(AnnotationNode& node) {
		visit(static_cast<ASTNode&>(node));
	}

	void Visitor::visit(ErrorNode& node) {
		visit(static_cast<ASTNode&>(node));
	}

	void Visitor::visit(TemplateDeclNode& node) {
		visit(static_cast<ASTNode&>(node));
	}

	void Visitor::visit(TypeNode& node) {
		visit(static_cast<ASTNode&>(node));
	}

	void Visitor::visit(FunctionTypeNode& node) {
		visit(static_cast<TypeNode &>(node));
	}

	void Visitor::visit(PrimitiveTypeNode& node) {
		visit(static_cast<TypeNode&>(node));
	}

	void Visitor::visit(NamedTypeNode& node) {
		visit(static_cast<TypeNode&>(node));
	}

	void Visitor::visit(ArrayTypeNode& node) {
		visit(static_cast<TypeNode&>(node));
	}

	void Visitor::visit(TemplateTypeNode& node) {
		visit(static_cast<TypeNode&>(node));
	}

	void Visitor::visit(DoWhileNode &node) {
		visit(static_cast<StmtNode&>(node));
	}

	void Visitor::visit(ScopeBlockNode &node) {
		visit(static_cast<BlockNode&>(node));
	}

	void Visitor::visit(EmptyStmtNode &node) {
		visit(static_cast<ASTNode&>(node));
	}

	//polymorphic_ref<TypeNode> Visitor::visitExpression(polymorphic_ref<ExprNode> expr) {
	//	return nullptr;
	//}

	void Visitor::visit(ExprNode &node) {
		visit(static_cast<ASTNode&>(node));
	}
	void Visitor::visit(StmtNode &node) {
		visit(static_cast<ASTNode&>(node));
	}

	void Visitor::visit(TemplateParameter &node) {
		visit(static_cast<ASTNode&>(node));
	}

	void Visitor::visit(ProgramNode &node) {
		visit(static_cast<ASTNode&>(node));
	}
} // zenith