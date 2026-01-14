module;
#include "../exceptions/ErrorReporter.hpp"
#include <string>
export module zenith.semantic;
import zenith.core.polymorphic_ref;
import zenith.ast;
export import :symbolTable;

export namespace zenith {
	class SemanticAnalyzer : public Visitor {
		struct ExpressionInfo {
			polymorphic_variant<TypeNode> type;
			bool isLvalue;
			bool isConst;
			[[nodiscard]] bool isModifiable() const {return isLvalue && !isConst;}
		};
		ErrorReporter& errorReporter;
		SymbolTable symbolTable;

		// Context information
		polymorphic_ref<FunctionDeclNode> currentFunction;
		polymorphic_ref<ObjectDeclNode> currentClass;
		bool inLoop = false;

		// Expression result type
		ExpressionInfo exprVR;

		// Type system helpers
		bool areTypesCompatible(polymorphic_ref<TypeNode> targetType, polymorphic_ref<TypeNode> valueType);

		polymorphic_variant<TypeNode> resolveType(polymorphic_ref<TypeNode> typeNode);

		[[nodiscard]] static std::string typeToString(polymorphic_ref<TypeNode> type);

		// Visitor methods
		void visit(ASTNode& node) override;
		ExpressionInfo visitExpression(polymorphic_ref<ExprNode> expr);

		// Declaration visitors
		void visit(ProgramNode& node) override;
		void visit(ImportNode& node) override;
		void visit(BlockNode& node) override;
		void visit(VarDeclNode& node) override;
		void visit(MultiVarDeclNode& node) override;
		void visit(FunctionDeclNode& node) override;
		void visit(LambdaExprNode& node) override;
		void visit(ReturnStmtNode& node) override;
		void visit(ObjectDeclNode& node) override;
		// void visit(UnionDeclNode& node) override;
		// void visit(ActorDeclNode& node) override;
		// Stmt
		void visit(IfNode& node) override;
		void visit(WhileNode& node) override;
		void visit(DoWhileNode& node) override;
		void visit(ForNode& node) override;
		void visit(ExprStmtNode& node) override;
		void visit(EmptyStmtNode& node) override {}
		// void visit(AnnotationNode& node) override;
		// void visit(TemplateDeclNode& node) override;
		// void visit(OperatorOverloadNode& node) override;
		// void visit(MemberDeclNode& node) override;

		// Expression visitors
		void visit(LiteralNode& node) override;               // polymorphic<TypeNode> -> exprVR
		void visit(VarNode& node) override;                   // polymorphic<TypeNode> -> exprVR
		void visit(BinaryOpNode& node) override;              // polymorphic<TypeNode> -> exprVR
		void visit(UnaryOpNode& node) override;               // polymorphic<TypeNode> -> exprVR
		void visit(CallNode& node) override;                  // polymorphic<TypeNode> -> exprVR
		void visit(MemberAccessNode& node) override;          // polymorphic<TypeNode> -> exprVR
		void visit(ArrayAccessNode& node) override;           // polymorphic<TypeNode> -> exprVR
		void visit(NewExprNode& node) override;               // polymorphic<TypeNode> -> exprVR
		// void visit(ThisNode& node) override;                  // polymorphic<TypeNode> -> exprVR
		// void visit(FreeObjectNode& node) override;            // polymorphic<TypeNode> -> exprVR
		// void visit(TemplateStringNode& node) override;        // polymorphic<TypeNode> -> exprVR
		// void visit(StructInitializerNode& node) override;     // polymorphic<TypeNode> -> exprVR

	public:
		explicit SemanticAnalyzer(ErrorReporter& errorReporter)
				: errorReporter(errorReporter), symbolTable(errorReporter) {}

		SymbolTable&& analyze(polymorphic_ref<ProgramNode> program);
	};

} // namespace zenith