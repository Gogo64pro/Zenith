//
// Created by gogop on 11/9/2025.
//
export module zenith.ast:visitor;
import zenith.core.polymorphic_ref;


export namespace zenith {
	class Visitor {
	public:
		virtual ~Visitor() = default;

		// Default fallback
		virtual void visit(struct ASTNode& node);
		virtual void visit(struct ExprNode& node);
		virtual void visit(struct StmtNode& node);

		// virtual polymorphic_ref<struct TypeNode> visitExpression(polymorphic_ref<ExprNode> expr);

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
}
