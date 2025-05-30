//
// Created by gogop on 4/25/2025.
//

#include "Visitor.hpp"
#include "../core/ASTNode.hpp"
#include "../ast/MainNodes.hpp"
#include "../ast/Declarations.hpp"
#include "../ast/Expressions.hpp"

namespace zenith {

	void Visitor::visit(ASTNode &node) {
		throw std::runtime_error("Unhandled AST node type");
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

	std::pair<TypeNode *, std::unique_ptr<TypeNode>> Visitor::visitExpression(ExprNode &expr) {
		return {nullptr, nullptr};
	}

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

	// Default fallback (optional)
	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<ASTNode> node) {
		throw std::runtime_error("Unhandled AST node type");
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<ExprNode> node) {
		visit(node.share<ASTNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<StmtNode> node) {
		visit(node.share<ASTNode>());
	}

	std_P3019_modified::polymorphic<TypeNode> PolymorphicVisitor::visitExpression(std_P3019_modified::polymorphic<ExprNode> expr) {
		return std_P3019_modified::polymorphic<TypeNode>();
	}

	// Specific visitors
	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<ProgramNode> node) {
		visit(node.share<ASTNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<ImportNode> node) {
		visit(node.share<ASTNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<BlockNode> node) {
		visit(node.share<StmtNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<VarDeclNode> node) {
		visit(node.share<StmtNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<MultiVarDeclNode> node) {
		visit(node.share<StmtNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<FunctionDeclNode> node) {
		visit(node.share<ASTNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<ObjectDeclNode> node) {
		visit(node.share<ASTNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<UnionDeclNode> node) {
		visit(node.share<ASTNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<ActorDeclNode> node) {
		visit(node.share<ObjectDeclNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<IfNode> node) {
		visit(node.share<StmtNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<WhileNode> node) {
		visit(node.share<StmtNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<DoWhileNode> node) {
		visit(node.share<StmtNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<ForNode> node) {
		visit(node.share<StmtNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<ExprStmtNode> node) {
		visit(node.share<StmtNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<EmptyStmtNode> node) {
		visit(node.share<StmtNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<AnnotationNode> node) {
		visit(node.share<ASTNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<TemplateDeclNode> node) {
		visit(node.share<ASTNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<LambdaNode> node) {
		visit(node.share<FunctionDeclNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<MemberDeclNode> node) {
		visit(node.share<ASTNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<OperatorOverloadNode> node) {
		visit(node.share<ASTNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<UnsafeNode> node) {
		visit(node.share<BlockNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<CompoundStmtNode> node) {
		visit(node.share<StmtNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<ReturnStmtNode> node) {
		visit(node.share<StmtNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<ScopeBlockNode> node) {
		visit(node.share<BlockNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<TemplateParameter> node) {
		visit(node.share<ASTNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<LiteralNode> node) {
		visit(node.share<ExprNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<VarNode> node) {
		visit(node.share<ExprNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<BinaryOpNode> node) {
		visit(node.share<ExprNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<UnaryOpNode> node) {
		visit(node.share<ExprNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<CallNode> node) {
		visit(node.share<ExprNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<MemberAccessNode> node) {
		visit(node.share<ExprNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<FreeObjectNode> node) {
		visit(node.share<ExprNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<ArrayAccessNode> node) {
		visit(node.share<ExprNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<NewExprNode> node) {
		visit(node.share<ExprNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<TemplateStringNode> node) {
		visit(node.share<ExprNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<ThisNode> node) {
		visit(node.share<ExprNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<StructInitializerNode> node) {
		visit(node.share<ExprNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<LambdaExprNode> node) {
		visit(node.share<ExprNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<TypeNode> node) {
		visit(node.share<ASTNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<PrimitiveTypeNode> node) {
		visit(node.share<TypeNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<NamedTypeNode> node) {
		visit(node.share<TypeNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<ArrayTypeNode> node) {
		visit(node.share<TypeNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<TemplateTypeNode> node) {
		visit(node.share<TypeNode>());
	}

	void PolymorphicVisitor::visit(std_P3019_modified::polymorphic<ErrorNode> node) {
		visit(node.share<ASTNode>());
	}

} // zenith