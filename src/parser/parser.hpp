#pragma once

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include "../lexer/lexer.hpp"
#include "../utils/mainargs.hpp"
#include "../exceptions/ErrorReporter.hpp"
#include "../ast/AST.hpp"

namespace zenith {
	class Parser {
		std::vector<Token> tokens;
		size_t current = 0;
		size_t previous = 0;
		Token currentToken;
		const Flags& flags;
		std::ostream& errStream;
		ErrorReporter errorReporter;
		std::vector<polymorphic<AnnotationNode>> pendingAnnotations;

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

		[[maybe_unused]] bool peekIsBlockEnd() const;
		bool peekIsTemplateStart() const;
		bool isInStructInitializerContext() const;

		//std::vector<polymorphic<MemberDeclNode>> parseActorMembers(std::string& actorName);
		//polymorphic<MultiVarDeclNode> parseVarDecls();
		// Parsing methods
		polymorphic<TypeNode> parseType();
		polymorphic<ExprNode> parsePrimary();
		polymorphic<StmtNode> parseStatement();
		std::vector<std::string> parseArrowFunctionParams();
		std::vector<TemplateParameter> parseTemplateParameters();
		polymorphic<ExprNode> parseExpression(int precedence = 0);
		polymorphic<StructInitializerNode> parseStructInitializer();
		std::pair<std::vector<std::pair<std::string, polymorphic<TypeNode>>>, bool> parseParameters();
		polymorphic<MemberDeclNode> parseMessageHandler(std::vector<polymorphic<AnnotationNode>> annotations);
		polymorphic<MemberDeclNode> parseField(std::vector<polymorphic<AnnotationNode>> &annotations, const MemberDeclNode::Access &access, bool isConst);
		polymorphic<MemberDeclNode> parseConstructor(const MemberDeclNode::Access &access, bool isConst, std::string &className, std::vector<polymorphic<AnnotationNode>> &annotations);
		polymorphic<MemberDeclNode> parseObjectPrimary(std::string &name, std::vector<polymorphic<AnnotationNode>> &annotations, MemberDeclNode::Access defaultLevel = MemberDeclNode::Access::PUBLIC);

		// Declaration parsers
		polymorphic<ImportNode> parseImport();
		polymorphic<VarDeclNode> parseVarDecl();
		polymorphic<UnionDeclNode> parseUnion();
		polymorphic<ObjectDeclNode> parseObject();
		polymorphic<ActorDeclNode> parseActorDecl();
		polymorphic<FunctionDeclNode> parseFunction();
		polymorphic<AnnotationNode> parseAnnotation();
		std::vector<polymorphic<AnnotationNode>> parseAnnotations();

		// Statement parsers
		polymorphic<IfNode> parseIfStmt();
		polymorphic<ForNode> parseForStmt();
		polymorphic<WhileNode> parseWhileStmt();
		polymorphic<DoWhileNode> parseDoWhileStmt();
		polymorphic<ReturnStmtNode> parseReturnStmt();
		polymorphic<ScopeBlockNode> parseScopeBlock();
		polymorphic<TemplateDeclNode> parseTemplate();
		polymorphic<BlockNode> parseBlock();
		polymorphic<UnsafeNode> parseUnsafeBlock();

		bool isArrowFunctionStart() const;

		// Expression parsers
		polymorphic<NewExprNode> parseNewExpression();
		polymorphic<FreeObjectNode> parseFreeObject();
		polymorphic<CallNode> parseFunctionCall(polymorphic<ExprNode> callee);
		polymorphic<ExprNode> parseArrayAccess(polymorphic<ExprNode> arrayExpr);
		polymorphic<LambdaExprNode> parseArrowFunction(std::vector<std::string>&& params);
		polymorphic<ExprNode> parseMemberAccess(polymorphic<ExprNode> object);
		// Error handling
		polymorphic<ErrorNode> createErrorNode();

		[[maybe_unused]] polymorphic<MemberDeclNode> createErrorNodeAsMember();

	public:
		explicit Parser(std::vector<Token> tokens, const Flags& flags, std::ostream& errStream = std::cerr);
		polymorphic<ProgramNode> parse();

	};
}