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

namespace zenith::parser {

class Parser {
private:
	std::vector<lexer::Token> tokens;
	size_t current = 0;
	size_t previous = 0;
	lexer::Token currentToken;
	const utils::Flags& flags;
	std::ostream& errStream;
	ErrorReporter errorReporter;

	// Helper methods
	bool isAtEnd() const;
	bool match(lexer::TokenType type) const;
	bool match(std::initializer_list<lexer::TokenType> types) const;
	lexer::Token advance();
	lexer::Token consume(lexer::TokenType type, const std::string& errorMessage);
	lexer::Token consume(lexer::TokenType type);
	lexer::Token peek(size_t offset = 1) const;
	const lexer::Token& previousToken() const;
	void synchronize();
	bool isPotentialMethod() const;

	// Type checking
	static bool isBuiltInType(lexer::TokenType type);
	static int getPrecedence(lexer::TokenType type);
	bool peekIsExpressionStart() const;
	bool peekIsStatementTerminator() const;
	bool peekIsBlockEnd() const;
	bool isInStructInitializerContext() const;

	// Parsing methods
	std::unique_ptr<ast::ExprNode> parseExpression(int precedence = 0);
	std::unique_ptr<ast::ExprNode> parsePrimary();
	std::unique_ptr<ast::StructInitializerNode> parseStructInitializer();
	std::unique_ptr<ast::TypeNode> parseType();
	std::unique_ptr<ast::StmtNode> parseStatement();
	std::pair<std::vector<std::pair<std::string, std::unique_ptr<ast::TypeNode>>>, bool> parseParameters();
	std::vector<std::string> parseArrowFunctionParams();

	// Declaration parsers
	std::unique_ptr<ast::VarDeclNode> parseVarDecl();
	std::unique_ptr<ast::FunctionDeclNode> parseFunction();
	std::unique_ptr<ast::ClassDeclNode> parseClass();
	std::unique_ptr<ast::ImportNode> parseImport();
	std::unique_ptr<ast::AnnotationNode> parseAnnotation();

	// Statement parsers
	std::unique_ptr<ast::IfNode> parseIfStmt();
	std::unique_ptr<ast::ForNode> parseForStmt();
	std::unique_ptr<ast::WhileNode> parseWhileStmt();
	std::unique_ptr<ast::DoWhileNode> parseDoWhileStmt();
	std::unique_ptr<ast::ReturnStmtNode> parseReturnStmt();
	std::unique_ptr<ast::BlockNode> parseBlock();

	// Expression parsers
	std::unique_ptr<ast::NewExprNode> parseNewExpression();
	std::unique_ptr<ast::FreeObjectNode> parseFreeObject();
	std::unique_ptr<ast::CallNode> parseFunctionCall(std::unique_ptr<ast::ExprNode> callee);
	std::unique_ptr<ast::MemberAccessNode> parseMemberAccess(std::unique_ptr<ast::ExprNode> object);
	std::unique_ptr<ast::ExprNode> parseArrayAccess(std::unique_ptr<ast::ExprNode> arrayExpr);
	std::unique_ptr<ast::LambdaExprNode> parseArrowFunction(std::vector<std::string>&& params);
	// Error handling
	std::unique_ptr<ast::ErrorNode> createErrorNode();

public:
	explicit Parser(std::vector<lexer::Token> tokens, const utils::Flags& flags, std::ostream& errStream = std::cerr);
	std::unique_ptr<ast::ProgramNode> parse();

};

} // zenith::parser