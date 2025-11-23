module;
#include "../exceptions/ErrorReporter.hpp"
#include <string>
export module zenith.semantic;
export import :symbolTable;
import zenith.ast;
import zenith.core.polymorphic_ref;

export namespace zenith {
	class SemanticAnalyzer : public Visitor {
		ErrorReporter& errorReporter;
		SymbolTable symbolTable;

		// Context information
		polymorphic_ref<FunctionDeclNode> currentFunction;
		polymorphic_ref<ObjectDeclNode> currentClass;
		bool inLoop = false;

		// Expression result type
		polymorphic_ref<TypeNode> exprVR = nullptr;

		// Type system helpers
		bool areTypesCompatible(polymorphic_ref<TypeNode> targetType, polymorphic_ref<TypeNode> valueType);

		polymorphic_ref<TypeNode> resolveType(polymorphic_ref<TypeNode> typeNode);

		[[nodiscard]] std::string typeToString(polymorphic_ref<TypeNode> type);

		// Visitor methods
		void visit(ASTNode& node) override;
		polymorphic_ref<TypeNode> visitExpression(polymorphic_ref<ExprNode> expr) override;

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
		void visit(UnionDeclNode& node) override;
		void visit(ActorDeclNode& node) override;
		void visit(IfNode& node) override;
		void visit(WhileNode& node) override;
		void visit(DoWhileNode& node) override;
		void visit(ForNode& node) override;
		void visit(ExprStmtNode& node) override;
		void visit(EmptyStmtNode& node) override;
		void visit(AnnotationNode& node) override;
		void visit(TemplateDeclNode& node) override;
		void visit(OperatorOverloadNode& node) override;
		void visit(MemberDeclNode& node) override;

		// Expression visitors
		void visit(LiteralNode& node) override;               // polymorphic<TypeNode> -> exprVR
		void visit(VarNode& node) override;                   // polymorphic<TypeNode> -> exprVR
		void visit(BinaryOpNode& node) override;              // polymorphic<TypeNode> -> exprVR
		void visit(UnaryOpNode& node) override;               // polymorphic<TypeNode> -> exprVR
		void visit(CallNode& node) override;                  // polymorphic<TypeNode> -> exprVR
		void visit(MemberAccessNode& node) override;          // polymorphic<TypeNode> -> exprVR
		void visit(ArrayAccessNode& node) override;           // polymorphic<TypeNode> -> exprVR
		void visit(NewExprNode& node) override;               // polymorphic<TypeNode> -> exprVR
		void visit(ThisNode& node) override;                  // polymorphic<TypeNode> -> exprVR
		void visit(FreeObjectNode& node) override;            // polymorphic<TypeNode> -> exprVR
		void visit(TemplateStringNode& node) override;        // polymorphic<TypeNode> -> exprVR
		void visit(StructInitializerNode& node) override;     // polymorphic<TypeNode> -> exprVR

	public:
		explicit SemanticAnalyzer(ErrorReporter& errorReporter)
				: errorReporter(errorReporter), symbolTable(errorReporter) {}

		SymbolTable&& analyze(ProgramNode &program);
	};

} // namespace zenith