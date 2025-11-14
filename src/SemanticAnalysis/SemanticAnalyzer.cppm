module;
#include "../exceptions/ErrorReporter.hpp"
#include <string>
export module zenith.semantic;
import zenith.ast;
import zenith.core.polymorphic;
export import :symbolTable;

export namespace zenith {
	class SemanticAnalyzer : public PolymorphicVisitor {
		ErrorReporter& errorReporter;
		SymbolTable symbolTable;

		// Context information
		std_P3019_modified::polymorphic<FunctionDeclNode> currentFunction = nullptr;
		std_P3019_modified::polymorphic<ObjectDeclNode> currentClass = nullptr;
		bool inLoop = false;

		// Expression result type
		std_P3019_modified::polymorphic<TypeNode> exprVR = nullptr;

		// Type system helpers
		bool areTypesCompatible(std_P3019_modified::polymorphic<TypeNode>& targetType, std_P3019_modified::polymorphic<TypeNode>& valueType);
		std_P3019_modified::polymorphic<TypeNode> resolveType(std_P3019_modified::polymorphic<TypeNode>& typeNode);
		std::string typeToString(std_P3019_modified::polymorphic<TypeNode> type);
		std_P3019_modified::polymorphic<TypeNode> getErrorTypeNode(const SourceLocation& loc);

		// Visitor methods
		void visit(std_P3019_modified::polymorphic<ASTNode> node) override;
		std_P3019_modified::polymorphic<TypeNode> visitExpression(std_P3019_modified::polymorphic<ExprNode> expr) override;

		// Declaration visitors
		void visit(std_P3019_modified::polymorphic<ProgramNode> node) override;
		void visit(std_P3019_modified::polymorphic<ImportNode> node) override;
		void visit(std_P3019_modified::polymorphic<BlockNode> node) override;
		void visit(std_P3019_modified::polymorphic<VarDeclNode> node) override;
		void visit(std_P3019_modified::polymorphic<MultiVarDeclNode> node) override;
		void visit(std_P3019_modified::polymorphic<FunctionDeclNode> node) override;
		void visit(std_P3019_modified::polymorphic<LambdaExprNode> node) override;
		void visit(std_P3019_modified::polymorphic<ReturnStmtNode> node) override;
		void visit(std_P3019_modified::polymorphic<ObjectDeclNode> node) override;
		void visit(std_P3019_modified::polymorphic<UnionDeclNode> node) override;
		void visit(std_P3019_modified::polymorphic<ActorDeclNode> node) override;
		void visit(std_P3019_modified::polymorphic<IfNode> node) override;
		void visit(std_P3019_modified::polymorphic<WhileNode> node) override;
		void visit(std_P3019_modified::polymorphic<DoWhileNode> node) override;
		void visit(std_P3019_modified::polymorphic<ForNode> node) override;
		void visit(std_P3019_modified::polymorphic<ExprStmtNode> node) override;
		void visit(std_P3019_modified::polymorphic<EmptyStmtNode> node) override;
		void visit(std_P3019_modified::polymorphic<AnnotationNode> node) override;
		void visit(std_P3019_modified::polymorphic<TemplateDeclNode> node) override;
		void visit(std_P3019_modified::polymorphic<OperatorOverloadNode> node) override;
		void visit(std_P3019_modified::polymorphic<MemberDeclNode> node) override;

		// Expression visitors
		void visit(std_P3019_modified::polymorphic<LiteralNode> node) override;               // std_P3019_modified::polymorphic<TypeNode> -> exprVR
		void visit(std_P3019_modified::polymorphic<VarNode> node) override;                   // std_P3019_modified::polymorphic<TypeNode> -> exprVR
		void visit(std_P3019_modified::polymorphic<BinaryOpNode> node) override;              // std_P3019_modified::polymorphic<TypeNode> -> exprVR
		void visit(std_P3019_modified::polymorphic<UnaryOpNode> node) override;               // std_P3019_modified::polymorphic<TypeNode> -> exprVR
		void visit(std_P3019_modified::polymorphic<CallNode> node) override;                  // std_P3019_modified::polymorphic<TypeNode> -> exprVR
		void visit(std_P3019_modified::polymorphic<MemberAccessNode> node) override;          // std_P3019_modified::polymorphic<TypeNode> -> exprVR
		void visit(std_P3019_modified::polymorphic<ArrayAccessNode> node) override;           // std_P3019_modified::polymorphic<TypeNode> -> exprVR
		void visit(std_P3019_modified::polymorphic<NewExprNode> node) override;               // std_P3019_modified::polymorphic<TypeNode> -> exprVR
		void visit(std_P3019_modified::polymorphic<ThisNode> node) override;                  // std_P3019_modified::polymorphic<TypeNode> -> exprVR
		void visit(std_P3019_modified::polymorphic<FreeObjectNode> node) override;            // std_P3019_modified::polymorphic<TypeNode> -> exprVR
		void visit(std_P3019_modified::polymorphic<TemplateStringNode> node) override;        // std_P3019_modified::polymorphic<TypeNode> -> exprVR
		void visit(std_P3019_modified::polymorphic<StructInitializerNode> node) override;     // std_P3019_modified::polymorphic<TypeNode> -> exprVR

	public:
		explicit SemanticAnalyzer(ErrorReporter& errorReporter)
				: symbolTable(errorReporter), errorReporter(errorReporter) {}

		SymbolTable&& analyze(const std_P3019_modified::polymorphic<ProgramNode>& program);
	};

} // namespace zenith