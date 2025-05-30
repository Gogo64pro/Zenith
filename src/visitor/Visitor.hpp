//
// Created by gogop on 4/25/2025.
//

#pragma once

#include <memory>
#include "../core/indirect_polymorphic.hpp"

namespace zenith{
	struct ASTNode;
	struct ProgramNode;
	struct ImportNode;
	struct BlockNode;
	struct VarDeclNode;
	struct MultiVarDeclNode;
	struct FunctionDeclNode;
	struct ObjectDeclNode;
	struct UnionDeclNode;
	struct ActorDeclNode;
	struct ReturnStmtNode;
	struct IfNode;
	struct WhileNode;
	struct DoWhileNode;
	struct ForNode;
	struct ExprStmtNode;
	struct EmptyStmtNode;
	struct AnnotationNode;
	struct TemplateDeclNode;
	struct OperatorOverloadNode;
	struct MemberDeclNode;
	struct LambdaNode;
	struct UnsafeNode;
	struct CompoundStmtNode;
	struct ScopeBlockNode;
	struct ErrorNode;
	struct TemplateParameter;

	// Base Expression/Type
	struct ExprNode;
	struct StmtNode;
	struct TypeNode;

	// Specific Expression nodes ARE needed for visit overrides now
	struct LiteralNode;
	struct VarNode;
	struct BinaryOpNode;
	struct UnaryOpNode;
	struct CallNode;
	struct MemberAccessNode;
	struct ArrayAccessNode;
	struct NewExprNode;
	struct ThisNode;
	struct FreeObjectNode;
	struct TemplateStringNode;
	struct StructInitializerNode;
	struct LambdaExprNode;

	// Specific Type nodes might be needed if visiting them directly
	struct PrimitiveTypeNode;
	struct NamedTypeNode;
	struct ArrayTypeNode;
	struct TemplateTypeNode;
}

namespace zenith {
	class Visitor {
	public:
		virtual ~Visitor() = default;

		// Default fallback
		virtual void visit(ASTNode& node);
		virtual void visit(ExprNode& node);
		virtual void visit(StmtNode& node);
		virtual std::pair<TypeNode *, std::unique_ptr<TypeNode>> visitExpression(ExprNode& expr);

		// Specific visitors
		virtual void visit(ProgramNode& node);
		virtual void visit(ImportNode& node);
		virtual void visit(BlockNode& node);
		virtual void visit(VarDeclNode& node);
		virtual void visit(MultiVarDeclNode& node);
		virtual void visit(FunctionDeclNode& node);
		virtual void visit(ObjectDeclNode& node);
		virtual void visit(UnionDeclNode& node);
		virtual void visit(ActorDeclNode& node);
		virtual void visit(IfNode& node);
		virtual void visit(WhileNode& node);
		virtual void visit(DoWhileNode& node);
		virtual void visit(ForNode& node);
		virtual void visit(ExprStmtNode& node);
		virtual void visit(EmptyStmtNode& node);
		virtual void visit(AnnotationNode& node);
		virtual void visit(TemplateDeclNode& node);
		virtual void visit(LambdaNode& node);
		virtual void visit(MemberDeclNode& node);
		virtual void visit(OperatorOverloadNode& node);
		virtual void visit(UnsafeNode& node);
		virtual void visit(CompoundStmtNode& node);
		virtual void visit(ReturnStmtNode& node);
		virtual void visit(ScopeBlockNode& node);
		virtual void visit(TemplateParameter& node);

		virtual void visit(LiteralNode& node);
		virtual void visit(VarNode& node);
		virtual void visit(BinaryOpNode& node);
		virtual void visit(UnaryOpNode& node);
		virtual void visit(CallNode& node);
		virtual void visit(MemberAccessNode& node);
		virtual void visit(FreeObjectNode& node);
		virtual void visit(ArrayAccessNode& node);
		virtual void visit(NewExprNode& node);
		virtual void visit(TemplateStringNode& node);
		virtual void visit(ThisNode& node);
		virtual void visit(StructInitializerNode& node);
		virtual void visit(LambdaExprNode& node);

		virtual void visit(TypeNode& node);
		virtual void visit(PrimitiveTypeNode& node);
		virtual void visit(NamedTypeNode& node);
		virtual void visit(ArrayTypeNode& node);
		virtual void visit(TemplateTypeNode& node);

		virtual void visit(ErrorNode& node);
	};

	class PolymorphicVisitor{
	public:
		// Default fallback (optional)
		virtual void visit(std_P3019_modified::polymorphic<ASTNode> node);
		virtual void visit(std_P3019_modified::polymorphic<ExprNode> node);
		virtual void visit(std_P3019_modified::polymorphic<StmtNode> node);
		virtual std_P3019_modified::polymorphic<TypeNode> visitExpression(std_P3019_modified::polymorphic<ExprNode> expr);

		// Specific visitors
		virtual void visit(std_P3019_modified::polymorphic<ProgramNode> node);
		virtual void visit(std_P3019_modified::polymorphic<ImportNode> node);
		virtual void visit(std_P3019_modified::polymorphic<BlockNode> node);
		virtual void visit(std_P3019_modified::polymorphic<VarDeclNode> node);
		virtual void visit(std_P3019_modified::polymorphic<MultiVarDeclNode> node);
		virtual void visit(std_P3019_modified::polymorphic<FunctionDeclNode> node);
		virtual void visit(std_P3019_modified::polymorphic<ObjectDeclNode> node);
		virtual void visit(std_P3019_modified::polymorphic<UnionDeclNode> node);
		virtual void visit(std_P3019_modified::polymorphic<ActorDeclNode> node);
		virtual void visit(std_P3019_modified::polymorphic<IfNode> node);
		virtual void visit(std_P3019_modified::polymorphic<WhileNode> node);
		virtual void visit(std_P3019_modified::polymorphic<DoWhileNode> node);
		virtual void visit(std_P3019_modified::polymorphic<ForNode> node);
		virtual void visit(std_P3019_modified::polymorphic<ExprStmtNode> node);
		virtual void visit(std_P3019_modified::polymorphic<EmptyStmtNode> node);
		virtual void visit(std_P3019_modified::polymorphic<AnnotationNode> node);
		virtual void visit(std_P3019_modified::polymorphic<TemplateDeclNode> node);
		virtual void visit(std_P3019_modified::polymorphic<LambdaNode> node);
		virtual void visit(std_P3019_modified::polymorphic<MemberDeclNode> node);
		virtual void visit(std_P3019_modified::polymorphic<OperatorOverloadNode> node);
		virtual void visit(std_P3019_modified::polymorphic<UnsafeNode> node);
		virtual void visit(std_P3019_modified::polymorphic<CompoundStmtNode> node);
		virtual void visit(std_P3019_modified::polymorphic<ReturnStmtNode> node);
		virtual void visit(std_P3019_modified::polymorphic<ScopeBlockNode> node);
		virtual void visit(std_P3019_modified::polymorphic<TemplateParameter> node);

		virtual void visit(std_P3019_modified::polymorphic<LiteralNode> node);
		virtual void visit(std_P3019_modified::polymorphic<VarNode> node);
		virtual void visit(std_P3019_modified::polymorphic<BinaryOpNode> node);
		virtual void visit(std_P3019_modified::polymorphic<UnaryOpNode> node);
		virtual void visit(std_P3019_modified::polymorphic<CallNode> node);
		virtual void visit(std_P3019_modified::polymorphic<MemberAccessNode> node);
		virtual void visit(std_P3019_modified::polymorphic<FreeObjectNode> node);
		virtual void visit(std_P3019_modified::polymorphic<ArrayAccessNode> node);
		virtual void visit(std_P3019_modified::polymorphic<NewExprNode> node);
		virtual void visit(std_P3019_modified::polymorphic<TemplateStringNode> node);
		virtual void visit(std_P3019_modified::polymorphic<ThisNode> node);
		virtual void visit(std_P3019_modified::polymorphic<StructInitializerNode> node);
		virtual void visit(std_P3019_modified::polymorphic<LambdaExprNode> node);

		virtual void visit(std_P3019_modified::polymorphic<TypeNode> node);
		virtual void visit(std_P3019_modified::polymorphic<PrimitiveTypeNode> node);
		virtual void visit(std_P3019_modified::polymorphic<NamedTypeNode> node);
		virtual void visit(std_P3019_modified::polymorphic<ArrayTypeNode> node);
		virtual void visit(std_P3019_modified::polymorphic<TemplateTypeNode> node);

		virtual void visit(std_P3019_modified::polymorphic<ErrorNode> node);

	protected:
		virtual ~PolymorphicVisitor() = default;
	};

} // zenith
