#pragma once

#include <memory>
#include <vector>
#include <string>
#include <iostream>
#include "../lexer/lexer.hpp"
#include "../utils/mainargs.hpp"
#include "../exceptions/ErrorReporter.hpp"
import zenith.ast;
import zenith.core.polymorphic;

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
		std::vector<std_P3019_modified::polymorphic<AnnotationNode>> pendingAnnotations;

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

		//std::vector<std_P3019_modified::polymorphic<MemberDeclNode>> parseActorMembers(std::string& actorName);
		//std_P3019_modified::polymorphic<MultiVarDeclNode> parseVarDecls();
		// Parsing methods
		std_P3019_modified::polymorphic<TypeNode> parseType();
		std_P3019_modified::polymorphic<ExprNode> parsePrimary();
		std_P3019_modified::polymorphic<StmtNode> parseStatement();
		std::vector<std::string> parseArrowFunctionParams();
		std::vector<TemplateParameter> parseTemplateParameters();
		std_P3019_modified::polymorphic<ExprNode> parseExpression(int precedence = 0);
		std_P3019_modified::polymorphic<StructInitializerNode> parseStructInitializer();
		std::pair<std::vector<std::pair<std::string, std_P3019_modified::polymorphic<TypeNode>>>, bool> parseParameters();
		std_P3019_modified::polymorphic<MemberDeclNode> parseMessageHandler(std::vector<std_P3019_modified::polymorphic<AnnotationNode>> annotations);
		std_P3019_modified::polymorphic<MemberDeclNode> parseField(std::vector<std_P3019_modified::polymorphic<AnnotationNode>> &annotations, const MemberDeclNode::Access &access, bool isConst);
		std_P3019_modified::polymorphic<MemberDeclNode> parseConstructor(const MemberDeclNode::Access &access, bool isConst, std::string &className, std::vector<std_P3019_modified::polymorphic<AnnotationNode>> &annotations);
		std_P3019_modified::polymorphic<MemberDeclNode> parseObjectPrimary(std::string &name, std::vector<std_P3019_modified::polymorphic<AnnotationNode>> &annotations, MemberDeclNode::Access defaultLevel = MemberDeclNode::PUBLIC);

		// Declaration parsers
		std_P3019_modified::polymorphic<ImportNode> parseImport();
		std_P3019_modified::polymorphic<VarDeclNode> parseVarDecl();
		std_P3019_modified::polymorphic<UnionDeclNode> parseUnion();
		std_P3019_modified::polymorphic<ObjectDeclNode> parseObject();
		std_P3019_modified::polymorphic<ActorDeclNode> parseActorDecl();
		std_P3019_modified::polymorphic<FunctionDeclNode> parseFunction();
		std_P3019_modified::polymorphic<AnnotationNode> parseAnnotation();
		std::vector<std_P3019_modified::polymorphic<AnnotationNode>> parseAnnotations();

		// Statement parsers
		std_P3019_modified::polymorphic<IfNode> parseIfStmt();
		std_P3019_modified::polymorphic<ForNode> parseForStmt();
		std_P3019_modified::polymorphic<WhileNode> parseWhileStmt();
		std_P3019_modified::polymorphic<DoWhileNode> parseDoWhileStmt();
		std_P3019_modified::polymorphic<ReturnStmtNode> parseReturnStmt();
		std_P3019_modified::polymorphic<ScopeBlockNode> parseScopeBlock();
		std_P3019_modified::polymorphic<TemplateDeclNode> parseTemplate();
		std_P3019_modified::polymorphic<BlockNode> parseBlock();
		std_P3019_modified::polymorphic<UnsafeNode> parseUnsafeBlock();

		// Expression parsers
		std_P3019_modified::polymorphic<NewExprNode> parseNewExpression();
		std_P3019_modified::polymorphic<FreeObjectNode> parseFreeObject();
		std_P3019_modified::polymorphic<CallNode> parseFunctionCall(std_P3019_modified::polymorphic<ExprNode> callee);
		std_P3019_modified::polymorphic<ExprNode> parseArrayAccess(std_P3019_modified::polymorphic<ExprNode> arrayExpr);
		std_P3019_modified::polymorphic<LambdaExprNode> parseArrowFunction(std::vector<std::string>&& params);
		std_P3019_modified::polymorphic<ExprNode> parseMemberAccess(std_P3019_modified::polymorphic<ExprNode> object);
		// Error handling
		std_P3019_modified::polymorphic<ErrorNode> createErrorNode();

		[[maybe_unused]] std_P3019_modified::polymorphic<MemberDeclNode> createErrorNodeAsMember();

	public:
		explicit Parser(std::vector<Token> tokens, const Flags& flags, std::ostream& errStream = std::cerr);
		std_P3019_modified::polymorphic<ProgramNode> parse();

	};
}