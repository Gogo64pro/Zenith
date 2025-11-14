//
// Created by gogop on 11/9/2025.
//
module;
#include <memory>
#include <utility>
import zenith.core.polymorphic;
export module zenith.ast:visitor;


export namespace zenith {
	class Visitor {
	public:
		virtual ~Visitor() = default;

		// Default fallback
		virtual void visit(struct ASTNode& node);
		virtual void visit(struct ExprNode& node);
		virtual void visit(struct StmtNode& node);

		[[maybe_unused]] virtual std::pair<struct TypeNode *, std::unique_ptr<TypeNode>> visitExpression(ExprNode& expr);

		// Specific visitors
		virtual void visit(struct ProgramNode& node);
		virtual void visit(struct ImportNode& node);
		virtual void visit(struct BlockNode& node);
		virtual void visit(struct VarDeclNode& node);
		virtual void visit(struct MultiVarDeclNode& node);
		virtual void visit(struct FunctionDeclNode& node);
		virtual void visit(struct ObjectDeclNode& node);
		virtual void visit(struct UnionDeclNode& node);
		virtual void visit(struct ActorDeclNode& node);
		virtual void visit(struct IfNode& node);
		virtual void visit(struct WhileNode& node);
		virtual void visit(struct DoWhileNode& node);
		virtual void visit(struct ForNode& node);
		virtual void visit(struct ExprStmtNode& node);
		virtual void visit(struct EmptyStmtNode& node);
		virtual void visit(struct AnnotationNode& node);
		virtual void visit(struct TemplateDeclNode& node);
		virtual void visit(struct LambdaNode& node);
		virtual void visit(struct MemberDeclNode& node);
		virtual void visit(struct OperatorOverloadNode& node);
		virtual void visit(struct UnsafeNode& node);
		virtual void visit(struct CompoundStmtNode& node);
		virtual void visit(struct ReturnStmtNode& node);
		virtual void visit(struct ScopeBlockNode& node);
		virtual void visit(struct TemplateParameter& node);

		virtual void visit(struct LiteralNode& node);
		virtual void visit(struct VarNode& node);
		virtual void visit(struct BinaryOpNode& node);
		virtual void visit(struct UnaryOpNode& node);
		virtual void visit(struct CallNode& node);
		virtual void visit(struct MemberAccessNode& node);
		virtual void visit(struct FreeObjectNode& node);
		virtual void visit(struct ArrayAccessNode& node);
		virtual void visit(struct NewExprNode& node);
		virtual void visit(struct TemplateStringNode& node);
		virtual void visit(struct ThisNode& node);
		virtual void visit(struct StructInitializerNode& node);
		virtual void visit(struct LambdaExprNode& node);

		virtual void visit(struct TypeNode& node);
		virtual void visit(struct PrimitiveTypeNode& node);
		virtual void visit(struct NamedTypeNode& node);
		virtual void visit(struct ArrayTypeNode& node);
		virtual void visit(struct TemplateTypeNode& node);
		virtual void visit(struct FunctionTypeNode& node);

		virtual void visit(struct ErrorNode& node);
	};

	class PolymorphicVisitor {
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
		virtual void visit(std_P3019_modified::polymorphic<FunctionTypeNode> node);
		virtual void visit(std_P3019_modified::polymorphic<TemplateTypeNode> node);

		virtual void visit(std_P3019_modified::polymorphic<ErrorNode> node);

		virtual ~PolymorphicVisitor() = default;
	};
}
