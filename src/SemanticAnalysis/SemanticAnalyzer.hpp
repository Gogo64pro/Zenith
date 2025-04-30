#include <memory>
#include "../ast/MainNodes.hpp"
#include "../exceptions/ErrorReporter.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include "../ast/Expressions.hpp"
#include "../ast/Declarations.hpp"
#include "../ast/Statements.hpp"
#include "../ast/MainNodes.hpp"
#include "../ast/TypeNodes.hpp"
#include "../ast/Other.hpp"
#include "../exceptions/ErrorReporter.hpp"
#include "../utils/small_vector.hpp"
#include "SymbolTable.hpp"
#include "../visitor/Visitor.hpp"

namespace zenith {
	class SemanticAnalyzer : public Visitor {

	private:
		ErrorReporter& errorReporter;
		SymbolTable symbolTable;

		// Context information
		FunctionDeclNode* currentFunction = nullptr;
		ObjectDeclNode* currentClass = nullptr;
		bool inLoop = false;

		//Return type workaround
		std::unique_ptr<TypeNode> exprVR;

		// Type system helpers
		bool areTypesCompatible(const TypeNode* targetType, const TypeNode* valueType);
		std::unique_ptr<TypeNode> resolveType(TypeNode* typeNode);
		std::string typeToString(const TypeNode* type);
		static std::unique_ptr<TypeNode> getErrorTypeNode(const SourceLocation& loc);

		// Visitor methods
		void visit(ASTNode& node) override;
		std::unique_ptr<TypeNode> visitExpression(ExprNode& expr) override;

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
		void visit(LiteralNode& node) override;           //std::unique_ptr<TypeNode> -> exprVR
		void visit(VarNode& node) override;               //std::unique_ptr<TypeNode> -> exprVR
		void visit(BinaryOpNode& node) override;          //std::unique_ptr<TypeNode> -> exprVR
		void visit(UnaryOpNode& node) override;           //std::unique_ptr<TypeNode> -> exprVR
		void visit(CallNode& node) override;              //std::unique_ptr<TypeNode> -> exprVR
		void visit(MemberAccessNode& node) override;      //std::unique_ptr<TypeNode> -> exprVR
		void visit(ArrayAccessNode& node) override;       //std::unique_ptr<TypeNode> -> exprVR
		void visit(NewExprNode& node) override;           //std::unique_ptr<TypeNode> -> exprVR
		void visit(ThisNode& node) override;              //std::unique_ptr<TypeNode> -> exprVR
		void visit(FreeObjectNode& node) override;        //std::unique_ptr<TypeNode> -> exprVR
		void visit(TemplateStringNode& node) override;    //std::unique_ptr<TypeNode> -> exprVR
		void visit(StructInitializerNode& node) override; //std::unique_ptr<TypeNode> -> exprVR

	public:
		explicit SemanticAnalyzer(ErrorReporter& errorReporter) : symbolTable(errorReporter), errorReporter(errorReporter){};

		SymbolTable&& analyze(ProgramNode& program);
	};

} // namespace zenith
