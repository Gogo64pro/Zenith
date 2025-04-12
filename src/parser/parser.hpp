#pragma once

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include "../lexer/lexer.hpp"
#include "../ast/Declarations.hpp"
#include "../ast/Expressions.hpp"
#include "../ast/Statements.hpp"
#include "../utils/mainargs.hpp"
#include "../ast/MainNodes.hpp"
#include "../ErrorReporter.hpp"

namespace zenith {
	class Parser {
	private:
		std::vector<Token> tokens;
		size_t current = 0;
		size_t previous = 0;
		Token currentToken;
		const Flags& flags;
		std::ostream& errStream;
		ErrorReporter errorReporter;

		// Helper methods
		bool isAtEnd() const;
		bool match(TokenType type) const;
		bool match(std::initializer_list<TokenType> types) const;
		Token advance();
		Token consume(TokenType type, const std::string& errorMessage);
		Token consume(TokenType type);
		Token peek(size_t offset = 1) const;
		const Token& previousToken() const;
		void synchronize();
		bool isPotentialMethod() const;

		// Type checking
		static bool isBuiltInType(TokenType type);
		static int getPrecedence(TokenType type);
		bool peekIsExpressionStart() const;
		bool peekIsStatementTerminator() const;
		bool peekIsBlockEnd() const;
		bool isInStructInitializerContext() const;

		// Parsing methods
		std::unique_ptr<ExprNode> parseExpression(int precedence = 0);
		std::unique_ptr<ExprNode> parsePrimary();
		std::unique_ptr<StructInitializerNode> parseStructInitializer();
		std::unique_ptr<TypeNode> parseType();
		std::unique_ptr<StmtNode> parseStatement();
		std::pair<std::vector<std::pair<std::string, std::unique_ptr<TypeNode>>>, bool> parseParameters();
		std::vector<std::string> parseArrowFunctionParams();

		// Declaration parsers
		std::unique_ptr<VarDeclNode> parseVarDecl();
		std::unique_ptr<FunctionDeclNode> parseFunction();
		std::unique_ptr<ClassDeclNode> parseClass();
		std::unique_ptr<ImportNode> parseImport();
		std::unique_ptr<AnnotationNode> parseAnnotation();

		// Statement parsers
		std::unique_ptr<IfNode> parseIfStmt();
		std::unique_ptr<ForNode> parseForStmt();
		std::unique_ptr<WhileNode> parseWhileStmt();
		std::unique_ptr<DoWhileNode> parseDoWhileStmt();
		std::unique_ptr<ReturnStmtNode> parseReturnStmt();
		std::unique_ptr<BlockNode> parseBlock();

		// Expression parsers
		std::unique_ptr<NewExprNode> parseNewExpression();
		std::unique_ptr<FreeObjectNode> parseFreeObject();
		std::unique_ptr<CallNode> parseFunctionCall(std::unique_ptr<ExprNode> callee);
		std::unique_ptr<MemberAccessNode> parseMemberAccess(std::unique_ptr<ExprNode> object);
		std::unique_ptr<ExprNode> parseArrayAccess(std::unique_ptr<ExprNode> arrayExpr);
		std::unique_ptr<LambdaExprNode> parseArrowFunction(std::vector<std::string>&& params);
		// Error handling
		std::unique_ptr<ErrorNode> createErrorNode();

	public:
		explicit Parser(std::vector<Token> tokens, const Flags& flags, std::ostream& errStream = std::cerr);
		std::unique_ptr<ProgramNode> parse();

	};
}