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
#include "../exceptions/ErrorReporter.hpp"

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
		std::vector<std::unique_ptr<AnnotationNode>> pendingAnnotations;

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
		bool peekIsTemplateStart() const;
		bool isInStructInitializerContext() const;

		//std::vector<std::unique_ptr<MemberDeclNode>> parseActorMembers(std::string& actorName);
		// Parsing methods
		std::unique_ptr<ExprNode> parseExpression(int precedence = 0);
		std::unique_ptr<ExprNode> parsePrimary();
		std::unique_ptr<StructInitializerNode> parseStructInitializer();
		std::unique_ptr<TypeNode> parseType();
		std::unique_ptr<StmtNode> parseStatement();
		std::pair<std::vector<std::pair<std::string, std::unique_ptr<TypeNode>>>, bool> parseParameters();
		std::vector<std::string> parseArrowFunctionParams();
		std::unique_ptr<MemberDeclNode> parseConstructor(const MemberDeclNode::Access &access, bool isConst, std::string &className, std::vector<std::unique_ptr<AnnotationNode>> &annotations);
		std::unique_ptr<MemberDeclNode> parseField(std::vector<std::unique_ptr<AnnotationNode>> &annotations, const MemberDeclNode::Access &access, bool isConst);
		std::unique_ptr<MemberDeclNode> parseMessageHandler(std::vector<std::unique_ptr<AnnotationNode>> annotations);
		std::unique_ptr<MemberDeclNode> parseObjectPrimary(std::string &name, std::vector<std::unique_ptr<AnnotationNode>> &annotations, MemberDeclNode::Access defaultLevel = MemberDeclNode::PUBLIC);

		// Declaration parsers
		std::unique_ptr<ImportNode> parseImport();
		std::unique_ptr<VarDeclNode> parseVarDecl();
		std::unique_ptr<UnionDeclNode> parseUnion();
		std::unique_ptr<ObjectDeclNode> parseObject();
		std::unique_ptr<ActorDeclNode> parseActorDecl();
		std::unique_ptr<FunctionDeclNode> parseFunction();
		std::unique_ptr<AnnotationNode> parseAnnotation();
		std::vector<std::unique_ptr<AnnotationNode>> parseAnnotations();

		// Statement parsers
		std::unique_ptr<IfNode> parseIfStmt();
		std::unique_ptr<ForNode> parseForStmt();
		std::unique_ptr<WhileNode> parseWhileStmt();
		std::unique_ptr<DoWhileNode> parseDoWhileStmt();
		std::unique_ptr<ReturnStmtNode> parseReturnStmt();
		std::unique_ptr<ScopeBlockNode> parseScopeBlock();
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
		std::unique_ptr<MemberDeclNode> createErrorNodeAsMember();

	public:
		explicit Parser(std::vector<Token> tokens, const Flags& flags, std::ostream& errStream = std::cerr);
		std::unique_ptr<ProgramNode> parse();

	};
}