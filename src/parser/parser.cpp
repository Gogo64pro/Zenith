#include "parser.hpp"
#include <memory>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include "../ast/Expressions.hpp"
#include "ParseError.hpp"

namespace zenith {
	// Fixed parseVarDecl to return the node
	std::unique_ptr<ast::VarDeclNode> Parser::parseVarDecl() {
		ast::SourceLocation loc = currentToken.loc;
		bool isHoisted = match(lexer::TokenType::HOIST);
		ast::VarDeclNode::Kind kind = ast::VarDeclNode::DYNAMIC; // Default to dynamic
		std::unique_ptr<ast::TypeNode> typeNode;

		// Case 1: Static typed declaration (int a = 12)
		if (isBuiltInType(currentToken.type) || currentToken.type == lexer::TokenType::IDENTIFIER) {
			kind = ast::VarDeclNode::STATIC;
			typeNode = parseType();
		}
			// Case 2: Dynamic declaration (let/var/dynamic)
		else if (match(lexer::TokenType::LET) || match(lexer::TokenType::VAR) || match(lexer::TokenType::DYNAMIC)) {
			kind = ast::VarDeclNode::DYNAMIC;
			advance();
		}

		std::string name = consume(lexer::TokenType::IDENTIFIER).lexeme;

		// Handle type annotation for dynamic variables (let a: int = 10)
		if (kind == ast::VarDeclNode::DYNAMIC && match(lexer::TokenType::COLON)) {
			typeNode = parseType();
		}

		// Handle initialization
		std::unique_ptr<ast::ExprNode> initializer;
		if (match(lexer::TokenType::EQUAL)) {
			consume(lexer::TokenType::EQUAL);
			initializer = parseExpression();

			// Special case: Class initialization (SomeClassName a = new SomeClassName())
			if (kind == ast::VarDeclNode::STATIC && initializer->isConstructorCall()) {
				kind = ast::VarDeclNode::CLASS_INIT;
			}
		}

		return std::make_unique<ast::VarDeclNode>(
				loc, kind, std::move(name),
				std::move(typeNode), std::move(initializer),
				isHoisted
		);
	}

	bool Parser::match(lexer::TokenType type) const {
		if (isAtEnd()) return false;
		return currentToken.type == type;
	}

	bool Parser::match(std::initializer_list<lexer::TokenType> types) const {
		if (isAtEnd()) return false;
		return std::find(types.begin(), types.end(), currentToken.type) != types.end();
	}

	lexer::Token Parser::consume(lexer::TokenType type, const std::string& errorMessage) {
		if (match(type)) {
			return advance();
		}
		throw ParseError(currentToken.loc, errorMessage);
	}

	lexer::Token Parser::consume(lexer::TokenType type) {
		return consume(type, "Expected " + lexer::Lexer::tokenToString(type));
	}

	std::unique_ptr<ast::ExprNode> Parser::parseExpression(int precedence) { // Remove default arg here
		auto expr = parsePrimary();

		while (true) {
			lexer::Token op = currentToken;
			int opPrecedence = getPrecedence(op.type);

			if (opPrecedence <= precedence) break;

			advance();
			auto right = parseExpression(opPrecedence);
			expr = std::make_unique<ast::BinaryOpNode>(
					op.loc, // Pass operator location
					op.type,
					std::move(expr),
					std::move(right)
			);
		}

		return expr;
	}

	std::unique_ptr<ast::ExprNode> Parser::parsePrimary() {
		ast::SourceLocation startLoc = currentToken.loc;

		// Handle all the simple cases first
		if (match(lexer::TokenType::NEW)) {
			return parseNewExpression();
		}
		else if (match({lexer::TokenType::NUMBER,lexer::TokenType::INTEGER,lexer::TokenType::FLOAT_LIT})) {
			lexer::Token numToken = advance();
			return std::make_unique<ast::LiteralNode>(startLoc, ast::LiteralNode::NUMBER, numToken.lexeme);
		}
		else if (match(lexer::TokenType::STRING_LIT)) {
			lexer::Token strToken = advance();
			return std::make_unique<ast::LiteralNode>(startLoc, ast::LiteralNode::STRING, strToken.lexeme);
		} else if (match(lexer::TokenType::TRUE) || match(lexer::TokenType::FALSE)) {
			lexer::Token boolToken = advance();
			return std::make_unique<ast::LiteralNode>(startLoc, ast::LiteralNode::BOOL, boolToken.lexeme);
		}
		else if (match(lexer::TokenType::NULL_LIT)) {
			advance();
			return std::make_unique<ast::LiteralNode>(startLoc, ast::LiteralNode::NIL, "nil");
		}
		else if (match(lexer::TokenType::LBRACE)) {
			if (isInStructInitializerContext()) {
				return parseStructInitializer();
			}
			return parseFreeObject();
		} else if (match(lexer::TokenType::FREEOBJ)) {
			advance(); // consume 'freeobj'
			if (match(lexer::TokenType::LBRACE)) {
				return parseFreeObject();
			}
			throw ParseError(startLoc,"Expected '{' after free obj, Achievement unlocked: How did we get here?");
		}
		else if (match(lexer::TokenType::LPAREN)) {
				auto params = parseArrowFunctionParams();
				if (match(lexer::TokenType::LAMBARROW)) {
					return parseArrowFunction(std::move(params));
				}
				// else treat as normal parenthesized expression
			}
		else if (match({lexer::TokenType::IDENTIFIER, lexer::TokenType::THIS})) {
			lexer::Token identToken = advance();
			std::unique_ptr<ast::ExprNode> expr;

			if (identToken.type == lexer::TokenType::THIS) {
				expr = std::make_unique<ast::ThisNode>(startLoc);
			} else {
				expr = std::make_unique<ast::VarNode>(startLoc, identToken.lexeme);
			}

			// Handle chained operations
			while (true) {
				if (match(lexer::TokenType::LPAREN)) {
					expr = parseFunctionCall(std::move(expr));  // std::move here
				}
				else if (match(lexer::TokenType::DOT)) {
					expr = parseMemberAccess(std::move(expr));  // std::move here
				}
				else if (match(lexer::TokenType::LBRACKET)) {
					expr = parseArrayAccess(std::move(expr));   // std::move here
				}
				else {
					break;
				}
			}

			return expr;  // No move needed here (RVO applies)
		}

		throw ParseError(currentToken.loc,
		                 "Expected primary expression, got " +
		                 lexer::Lexer::tokenToString(currentToken.type));
	}

	lexer::Token Parser::advance() {
		if (isAtEnd()) {
			return lexer::Token{lexer::TokenType::EOF_TOKEN, "",
			             tokens.empty() ? ast::SourceLocation{1,1,0, 0} : tokens.back().loc};
		}

		lexer::Token result = tokens[current];  // 1. Get current token first
		previous = current;

		// 2. Only advance if not already at end
		if (current + 1 < tokens.size()) {
			current++;
			currentToken = tokens[current];
		} else {
			current = tokens.size();  // Mark as ended
			currentToken = lexer::Token{lexer::TokenType::EOF_TOKEN, "", result.loc};
		}

		return result;  // Return what was current when we entered
	}

	Parser::Parser(std::vector<lexer::Token> tokens, const Flags& flags, std::ostream& errStream)
			: tokens(std::move(tokens)),
			  currentToken(this->tokens.empty() ?
			               lexer::Token{lexer::TokenType::EOF_TOKEN, "", {1, 1, 0}} :
			               this->tokens[0]) , flags(flags), errStream(errStream), errorReporter(std::cout) {
		current = 0;
	}

	bool Parser::isAtEnd() const {
		return current >= tokens.size();
	}
	std::unique_ptr<ast::TypeNode> Parser::parseType() {
		ast::SourceLocation startLoc = currentToken.loc;

		// Handle built-in types (from lexer::TokenType)
		if (match({lexer::TokenType::INT, lexer::TokenType::LONG, lexer::TokenType::SHORT,
				   lexer::TokenType::BYTE, lexer::TokenType::FLOAT, lexer::TokenType::DOUBLE,
				   lexer::TokenType::STRING, lexer::TokenType::NUMBER, lexer::TokenType::BIGINT,
				   lexer::TokenType::BIGNUMBER, lexer::TokenType::FREEOBJ})) {

			lexer::Token typeToken = advance();

			ast::PrimitiveTypeNode::Type kind;
			     if (typeToken.type == lexer::TokenType::INT   ) kind = ast::PrimitiveTypeNode::INT;
			else if (typeToken.type == lexer::TokenType::LONG  ) kind = ast::PrimitiveTypeNode::LONG;
			else if (typeToken.type == lexer::TokenType::SHORT ) kind = ast::PrimitiveTypeNode::SHORT;
			else if (typeToken.type == lexer::TokenType::BYTE  ) kind = ast::PrimitiveTypeNode::BYTE;
			else if (typeToken.type == lexer::TokenType::FLOAT ) kind = ast::PrimitiveTypeNode::FLOAT;
			else if (typeToken.type == lexer::TokenType::DOUBLE) kind = ast::PrimitiveTypeNode::DOUBLE;
			else if (typeToken.type == lexer::TokenType::STRING) kind = ast::PrimitiveTypeNode::STRING;
			else if (typeToken.type == lexer::TokenType::NUMBER) kind = ast::PrimitiveTypeNode::NUMBER;
			else if (typeToken.type == lexer::TokenType::BIGINT) kind = ast::PrimitiveTypeNode::BIGINT;
			else if (typeToken.type == lexer::TokenType::FREEOBJ) {
				// For freeobj, we'll return a TypeNode with DYNAMIC kind since it's a special case
				return std::make_unique<ast::TypeNode>(startLoc, ast::TypeNode::DYNAMIC);
			}
			else kind = ast::PrimitiveTypeNode::BIGNUMBER;

			return std::make_unique<ast::PrimitiveTypeNode>(startLoc, kind);
		}
		else if (match(lexer::TokenType::LBRACKET)) {
			// Array type (e.g., [int] or [MyClass])
			advance(); // Consume '['
			auto elementType = parseType();
			consume(lexer::TokenType::RBRACKET, "Expected ']' after array type");
			return std::make_unique<ast::ArrayTypeNode>(startLoc, std::move(elementType));
		}
		else if (match(lexer::TokenType::IDENTIFIER)) {
			// User-defined type (class/struct/type alias)
			lexer::Token typeToken = advance();
			return std::make_unique<ast::NamedTypeNode>(startLoc, typeToken.lexeme);
		}

		throw ParseError(
				currentToken.loc,
				"Expected type name, got " + lexer::Lexer::tokenToString(currentToken.type)
		);
	}

	void Parser::synchronize() {
		// 1. Always consume the current problematic token first
		advance();

		// 2. Skip tokens until we find a synchronization point
		while (!isAtEnd()) {
			// 3. Check for statement/declaration starters
			switch (currentToken.type) {
				// Declaration starters
				case lexer::TokenType::CLASS:
				case lexer::TokenType::FUN:
				case lexer::TokenType::LET:
				case lexer::TokenType::VAR:
				case lexer::TokenType::STRUCT:
				case lexer::TokenType::IMPORT:
					return;

					// Statement terminators
				case lexer::TokenType::SEMICOLON:
				case lexer::TokenType::RBRACE:
					advance(); // Consume the terminator
					return;

					// Expression-level recovery points
				case lexer::TokenType::IF:
				case lexer::TokenType::FOR:
				case lexer::TokenType::WHILE:
				case lexer::TokenType::RETURN:
					return;
			}

			// 4. Skip all other tokens
			advance();
		}
	}

	const lexer::Token &Parser::previousToken() const {
		if (previous >= tokens.size()) {
			static lexer::Token eof{lexer::TokenType::EOF_TOKEN, "", {0,0,0}};
			return eof;
		}
		return tokens[previous];
	}

	int Parser::getPrecedence(lexer::TokenType type) {
		static const std::unordered_map<lexer::TokenType, int> precedences = {
				// Assignment
				{lexer::TokenType::EQUAL, 1},
				{lexer::TokenType::PLUS_EQUALS, 1},
				{lexer::TokenType::MINUS_EQUALS, 1},
				{lexer::TokenType::STAR_EQUALS, 1},
				{lexer::TokenType::SLASH_EQUALS, 1},
				{lexer::TokenType::PERCENT_EQUALS, 1},

				// Logical
				{lexer::TokenType::OR, 2},
				{lexer::TokenType::AND, 3},

				// Equality
				{lexer::TokenType::BANG_EQUAL, 4},
				{lexer::TokenType::EQUAL_EQUAL, 4},

				// Comparison
				{lexer::TokenType::LESS, 5},
				{lexer::TokenType::LESS_EQUAL, 5},
				{lexer::TokenType::GREATER, 5},
				{lexer::TokenType::GREATER_EQUAL, 5},

				// Arithmetic
				{lexer::TokenType::PLUS, 6},
				{lexer::TokenType::MINUS, 6},
				{lexer::TokenType::STAR, 7},
				{lexer::TokenType::SLASH, 7},
				{lexer::TokenType::PERCENT, 7}

		};

		auto it = precedences.find(type);
		return it != precedences.end() ? it->second : 0;
	}

	std::unique_ptr<ast::ProgramNode> Parser::parse() {
		ast::SourceLocation startLoc = currentToken.loc;
		std::vector<std::unique_ptr<ast::ASTNode>> declarations;
		while (!isAtEnd()) {
			try {
				if (match(lexer::TokenType::IMPORT)) {
					declarations.push_back(parseImport());
				}
				else if (match(lexer::TokenType::CLASS)) {
					declarations.push_back(parseClass());
				}
				else if (match(lexer::TokenType::FUN)) {
					declarations.push_back(parseFunction());
				}
				else if (isBuiltInType(currentToken.type) || currentToken.type == lexer::TokenType::IDENTIFIER) {
					// Handle both built-in types and user-defined types
					if (isPotentialMethod()) {
						declarations.push_back(parseFunction());
					} else {
						// It's a variable declaration
						declarations.push_back(parseVarDecl());
					}
				}
				else if (match({lexer::TokenType::LET, lexer::TokenType::VAR, lexer::TokenType::DYNAMIC})) {
					declarations.push_back(parseVarDecl());
				}
				else {
					// Handle other top-level constructs
					advance();
				}
			} catch (const ParseError& e) {
				errorReporter.report(e.location,e.format());
				errStream << e.what() << std::endl;
				synchronize();
			}
		}
		auto program = std::make_unique<ast::ProgramNode>(startLoc,std::move(declarations));

		return program;
	}

	bool Parser::isBuiltInType(lexer::TokenType type) {
		static const std::unordered_set<lexer::TokenType> builtInTypes = {
				lexer::TokenType::INT, lexer::TokenType::LONG, lexer::TokenType::SHORT, lexer::TokenType::BYTE, lexer::TokenType::FLOAT, lexer::TokenType::DOUBLE, lexer::TokenType::STRING, lexer::TokenType::FREEOBJ
		};
		return builtInTypes.count(type) > 0;
	}

	std::unique_ptr<ast::NewExprNode> Parser::parseNewExpression() {
		ast::SourceLocation location = consume(lexer::TokenType::NEW).loc; // Eat 'new' keyword
		std::string className = consume(lexer::TokenType::IDENTIFIER).lexeme;

		consume(lexer::TokenType::LPAREN);
		std::vector<std::unique_ptr<ast::ExprNode>> args;
		if (!match(lexer::TokenType::RPAREN)) {
			do {
				args.push_back(parseExpression());
			} while (match(lexer::TokenType::COMMA));
			consume(lexer::TokenType::RPAREN);
		}

		return std::make_unique<ast::NewExprNode>(location,className, std::move(args));
	}

	std::unique_ptr<ast::FunctionDeclNode> Parser::parseFunction() {
		ast::SourceLocation loc = currentToken.loc;
		bool hasFunKeyword = false;

		// Handle annotations
		bool isAsync = false;
		while (match(lexer::TokenType::AT)) {
			if (peek().lexeme == "Async") {
				isAsync = true;
				advance(); // Skip @
				advance(); // Skip Async
			} else {
				// Handle other annotations if needed
				advance();
			}
		}

		// Parse return type (optional)
		std::unique_ptr<ast::TypeNode> returnType;
		if (match(lexer::TokenType::FUN)) {
			hasFunKeyword = true;
			advance();

			// Only parse return type if followed by type specifier
			if (isBuiltInType(currentToken.type) || currentToken.type == lexer::TokenType::IDENTIFIER) {
				// Peek ahead to distinguish:
				// 1) 'fun int foo' -> returnType=int
				// 2) 'fun foo'     -> function name
				if (peek(1).type == lexer::TokenType::IDENTIFIER && peek(2).type == lexer::TokenType::LPAREN) {
					returnType = parseType();
				}
			}
		}
			// Handle C-style declarations (e.g. "int foo()")
		else if (isBuiltInType(currentToken.type) || currentToken.type == lexer::TokenType::IDENTIFIER) {
			returnType = parseType();
		}
		std::string name = consume(lexer::TokenType::IDENTIFIER).lexeme;

		// Get both params and structSugar flag
		auto [params, structSugar] = parseParameters();

		auto body = parseBlock();

		return std::make_unique<ast::FunctionDeclNode>(
				loc,
				std::move(name),
				std::move(params),
				std::move(returnType),
				std::move(body),
				isAsync,
				structSugar
		);
	}

	std::unique_ptr<ast::BlockNode> Parser::parseBlock() {
		ast::SourceLocation startLoc = currentToken.loc;
		consume(lexer::TokenType::LBRACE);

		std::vector<std::unique_ptr<ast::ASTNode>> statements;
		try {
			while (!match(lexer::TokenType::RBRACE) && !isAtEnd()) {
				statements.push_back(parseStatement());
			}
			consume(lexer::TokenType::RBRACE,"Expected '}' after block");
		} catch (const ParseError&) {
			synchronize(); // Your error recovery method
			if (!match(lexer::TokenType::RBRACE)) {
				// Insert synthetic '}' if missing
				statements.push_back(createErrorNode());
			}
		}

		return std::make_unique<ast::BlockNode>(startLoc, std::move(statements));
	}

	std::unique_ptr<ast::StmtNode> Parser::parseStatement() {
		ast::SourceLocation loc = currentToken.loc;

		// Declaration statements
		if (match({lexer::TokenType::LET, lexer::TokenType::VAR, lexer::TokenType::DYNAMIC})) {
			return parseVarDecl();
		}

		// Static variable declarations (primitive or class type)
		if (isBuiltInType(currentToken.type) || currentToken.type == lexer::TokenType::IDENTIFIER) {
			// Check if this is actually a declaration by looking ahead
			if (peek(1).type == lexer::TokenType::IDENTIFIER &&
			    (peek(2).type == lexer::TokenType::EQUAL || peek(2).type == lexer::TokenType::SEMICOLON)) {
				return parseVarDecl();
			}
		}

		if (match(lexer::TokenType::THIS)) {
			auto expr = parseExpression();
			if (match(lexer::TokenType::SEMICOLON)) advance();
			return std::make_unique<ast::ExprStmtNode>(loc, std::move(expr));
		}

		// Control flow statements
		if (match(lexer::TokenType::IF)) return parseIfStmt();
		if (match(lexer::TokenType::FOR)) return parseForStmt();
		if (match(lexer::TokenType::WHILE)) return parseWhileStmt();
		if (match(lexer::TokenType::DO)) return parseDoWhileStmt();
		if (match(lexer::TokenType::RETURN)) return parseReturnStmt();

		// Unsafe blocks
		if (match(lexer::TokenType::UNSAFE)) {
			advance(); // Consume 'unsafe'
			return parseBlock();
		}

		// Block statements
		if (match(lexer::TokenType::LBRACE)) {
			return parseBlock();
		}

		// Expression statements
		if (peekIsExpressionStart()) {
			auto expr = parseExpression();
			// Optional semicolon (Zenith allows brace-optional syntax)
			if (match(lexer::TokenType::SEMICOLON)) {
				advance();
			}
			return std::make_unique<ast::ExprStmtNode>(loc, std::move(expr));
		}

		// Empty statement (just a semicolon)
		if (match(lexer::TokenType::SEMICOLON)) {
			advance();
			return std::make_unique<ast::EmptyStmtNode>(loc);
		}

		// Error recovery
		throw ParseError(currentToken.loc, "Unexpected token in statement: " + currentToken.lexeme);
	}

	bool Parser::peekIsExpressionStart() const {
		switch (currentToken.type) {
			case lexer::TokenType::IDENTIFIER:
			case lexer::TokenType::INTEGER:
			case lexer::TokenType::FLOAT_LIT:
			case lexer::TokenType::STRING_LIT:
			case lexer::TokenType::LPAREN: //Maybe also arrow lambda
			case lexer::TokenType::LBRACE:       // For object literals
			case lexer::TokenType::NEW:          // Constructor calls
			case lexer::TokenType::BANG:         // Unary operators
			case lexer::TokenType::MINUS:
			case lexer::TokenType::PLUS:
			case lexer::TokenType::THIS:

				return true;
			default:
				return false;
		}
	}

	lexer::Token Parser::peek(size_t offset) const {
		size_t idx = current + offset;
		if (idx >= tokens.size()) {
			return lexer::Token{lexer::TokenType::EOF_TOKEN, "",
			             tokens.empty() ? ast::SourceLocation{1,1,0} : tokens.back().loc};
		}
		return tokens[idx];

	}

	std::unique_ptr<ast::IfNode> Parser::parseIfStmt() {
		ast::SourceLocation loc = consume(lexer::TokenType::IF).loc;
		consume(lexer::TokenType::LPAREN, "Expected '(' after 'if'");
		auto condition = parseExpression();
		consume(lexer::TokenType::RPAREN, "Expected ')' after if condition");

		// Parse 'then' branch (enforce braces if required)
		std::unique_ptr<ast::StmtNode> thenBranch;
		try {
			if (flags.bracesRequired && !match(lexer::TokenType::LBRACE)) {
				throw ParseError(currentToken.loc, "Expected '{' after 'if'");
			}
			thenBranch = parseStatement();
		} catch (const ParseError& e) {
			errorReporter.report(e.location,"Error in if body " + e.format());
			errStream << "Error in if body: " << e.what() << std::endl;
			synchronize(); // Skip to next statement
			auto errorNode = createErrorNode();
			thenBranch = std::unique_ptr<ast::StmtNode>(dynamic_cast<ast::StmtNode*>(errorNode.release()));
			if (!thenBranch) {  // Fallback if cast fails
				thenBranch = std::make_unique<ast::EmptyStmtNode>(errorNode->loc);
			}
		}

		// Parse 'else' branch (also enforce braces if required)
		std::unique_ptr<ast::ASTNode> elseBranch;
		if (match(lexer::TokenType::ELSE)) {
			advance();
			if (flags.bracesRequired) {
				if (!match(lexer::TokenType::LBRACE)) {
					throw ParseError(currentToken.loc,
					                 "Expected '{' after 'else' (braces are required)");
				}
				elseBranch = parseBlock();
			} else {
				elseBranch = parseStatement();
			}
		}

		return std::make_unique<ast::IfNode>(
				loc,
				std::move(condition),
				std::move(thenBranch),
				std::move(elseBranch)
		);
	}

	std::unique_ptr<ast::ForNode> Parser::parseForStmt() {
		ast::SourceLocation loc = consume(lexer::TokenType::FOR).loc;
		consume(lexer::TokenType::LPAREN);

		// Parse init/condition/increment (unchanged)
		std::unique_ptr<ast::ASTNode> init;
		if (match(lexer::TokenType::SEMICOLON)) {
			advance(); // Empty initializer
		}
		else if (match({lexer::TokenType::LET, lexer::TokenType::VAR, lexer::TokenType::DYNAMIC}) ||
		         isBuiltInType(currentToken.type)) {
			init = parseVarDecl();
		}
		else {
			init = std::make_unique<ast::ExprStmtNode>(currentToken.loc, parseExpression());
		}
		consume(lexer::TokenType::SEMICOLON);

		std::unique_ptr<ast::ExprNode> condition;
		if (!match(lexer::TokenType::SEMICOLON)) {
			condition = parseExpression();
		}
		consume(lexer::TokenType::SEMICOLON);

		std::unique_ptr<ast::ExprNode> increment;
		if (!match(lexer::TokenType::RPAREN)) {
			increment = parseExpression();
		}
		consume(lexer::TokenType::RPAREN);

		// Enforce braces if required
		if (flags.bracesRequired && !match(lexer::TokenType::LBRACE)) {
			throw ParseError(currentToken.loc,
			                 "Expected '{' after 'for' (braces are required)");
		}

		auto body = parseStatement(); // parseBlock() if braces are required
		return std::make_unique<ast::ForNode>(loc, std::move(init), std::move(condition),
		                                 std::move(increment), std::move(body));
	}

	std::unique_ptr<ast::WhileNode> Parser::parseWhileStmt() {
		ast::SourceLocation loc = consume(lexer::TokenType::WHILE).loc;
		consume(lexer::TokenType::LPAREN, "Expected '(' after 'while'");
		auto condition = parseExpression();
		consume(lexer::TokenType::RPAREN, "Expected ')' after while condition");

		// Enforce braces if required
		if (flags.bracesRequired && !match(lexer::TokenType::LBRACE)) {
			throw ParseError(currentToken.loc,
			                 "Expected '{' after 'while' (braces are required)");
		}

		auto body = parseStatement(); // parseBlock() if braces are required
		return std::make_unique<ast::WhileNode>(loc, std::move(condition), std::move(body));
	}

	std::unique_ptr<ast::DoWhileNode> Parser::parseDoWhileStmt() {
		ast::SourceLocation loc = consume(lexer::TokenType::DO).loc;

		// Enforce braces if required
		if (flags.bracesRequired && !match(lexer::TokenType::LBRACE)) {
			throw ParseError(currentToken.loc,
			                 "Expected '{' after 'do' (braces are required)");
		}

		auto body = parseStatement(); // parseBlock() if braces are required

		consume(lexer::TokenType::WHILE, "Expected 'while' after do body");
		consume(lexer::TokenType::LPAREN, "Expected '(' after 'while'");
		auto condition = parseExpression();
		consume(lexer::TokenType::RPAREN, "Expected ')' after while condition");

		// Optional semicolon (Zenith allows it)
		if (match(lexer::TokenType::SEMICOLON)) {
			advance();
		}

		return std::make_unique<ast::DoWhileNode>(loc, std::move(condition), std::move(body));
	}

	std::unique_ptr<ast::ReturnStmtNode> Parser::parseReturnStmt() {
		ast::SourceLocation loc = consume(lexer::TokenType::RETURN).loc;

		// Handle empty return (no expression)
		if (match(lexer::TokenType::SEMICOLON)) {
			advance();  // Consume the semicolon
			return std::make_unique<ast::ReturnStmtNode>(loc, nullptr);
		}

		// Parse return value (optional in Zenith)
		std::unique_ptr<ast::ExprNode> value;
		if (!peekIsStatementTerminator()) {
			value = parseExpression();
		}

		// Zenith allows optional semicolon after return
		if (match(lexer::TokenType::SEMICOLON)) {
			advance();
		}

		// Zenith-specific: Check for annotations
		while (match(lexer::TokenType::AT)) {
			parseAnnotation();
		}

		return std::make_unique<ast::ReturnStmtNode>(loc, std::move(value));
	}

	bool Parser::peekIsStatementTerminator() const {
		return match({lexer::TokenType::SEMICOLON, lexer::TokenType::RBRACE, lexer::TokenType::EOF_TOKEN});
	}

	std::unique_ptr<ast::FreeObjectNode> Parser::parseFreeObject() {
		ast::SourceLocation loc = consume(lexer::TokenType::LBRACE).loc;
		std::vector<std::pair<std::string, std::unique_ptr<ast::ExprNode>>> properties;

		if (!match(lexer::TokenType::RBRACE)) {
			do {
				std::string name = consume(lexer::TokenType::IDENTIFIER).lexeme;
				consume(lexer::TokenType::COLON);
				auto value = parseExpression();
				properties.emplace_back(name, std::move(value));
				if (match(lexer::TokenType::COMMA)) {
					consume(lexer::TokenType::COMMA);  // Actually consume the comma
				} else {
					break;  // No more properties
				}
			} while (true);
		}

		consume(lexer::TokenType::RBRACE);
		return std::make_unique<ast::FreeObjectNode>(loc, std::move(properties));
	}

	std::unique_ptr<ast::CallNode> Parser::parseFunctionCall(std::unique_ptr<ast::ExprNode> callee) {
		ast::SourceLocation loc = callee->loc;
		consume(lexer::TokenType::LPAREN);

		std::vector<std::unique_ptr<ast::ExprNode>> args;
		if (!match(lexer::TokenType::RPAREN)) {
			do {
				args.push_back(parseExpression());
			} while (match(lexer::TokenType::COMMA));
		}

		consume(lexer::TokenType::RPAREN);
		return std::make_unique<ast::CallNode>(loc, std::move(callee), std::move(args));
	}

	std::unique_ptr<ast::MemberAccessNode> Parser::parseMemberAccess(std::unique_ptr<ast::ExprNode> object) {
		// Horse guarantee: We only get called when we see a DOT, if not I'm a horse or tramvai
		ast::SourceLocation loc = object->loc;

		// First access (guaranteed to exist)
		advance(); // Consume '.'
		std::string member = consume(lexer::TokenType::IDENTIFIER).lexeme;
		auto result = std::make_unique<ast::MemberAccessNode>(loc, std::move(object), member);

		// Handle additional accesses or calls
		while (match(lexer::TokenType::DOT)) {
			advance();
			member = consume(lexer::TokenType::IDENTIFIER).lexeme;
			result = std::make_unique<ast::MemberAccessNode>(loc, std::move(result), member);

			if (match(lexer::TokenType::LPAREN)) {
				// Temporarily degrade to ast::ExprNode for the call
				auto temp = std::unique_ptr<ast::ExprNode>(result.release());
				temp = parseFunctionCall(std::move(temp));
				// Then immediately cast back - if this fails, we deserve to be horses
				result.reset(dynamic_cast<ast::MemberAccessNode*>(temp.release()));
			}
		}

		return result;
	}

	std::unique_ptr<ast::ExprNode> Parser::parseArrayAccess(std::unique_ptr<ast::ExprNode> arrayExpr) {
		ast::SourceLocation loc = currentToken.loc;

		// Keep processing chained array accesses (e.g., arr[1][2][3])
		while (match(lexer::TokenType::LBRACKET)) {
			advance(); // Consume '['

			auto indexExpr = parseExpression();
			consume(lexer::TokenType::RBRACKET, "Expected ']' after array index");

			// Create new array access node
			arrayExpr = std::make_unique<ast::ArrayAccessNode>(
					loc,
					std::move(arrayExpr),  // The array being accessed
					std::move(indexExpr)   // The index expression
			);
		}

		return arrayExpr;
	}

	std::unique_ptr<ast::AnnotationNode> Parser::parseAnnotation() {
		ast::SourceLocation loc = consume(lexer::TokenType::AT).loc; // Eat '@' symbol

		// Parse annotation name
		std::string name = consume(lexer::TokenType::IDENTIFIER).lexeme;

		// Parse optional annotation arguments
		std::vector<std::pair<std::string, std::unique_ptr<ast::ExprNode>>> arguments;

		if (match(lexer::TokenType::LPAREN)) {
			advance(); // Consume '('

			if (!match(lexer::TokenType::RPAREN)) {
				do {
					// Parse argument name (optional) or just value
					std::string argName;
					if (peek(1).type == lexer::TokenType::EQUAL) {
						argName = consume(lexer::TokenType::IDENTIFIER).lexeme;
						consume(lexer::TokenType::EQUAL);
					}

					// Parse argument value
					auto value = parseExpression();

					arguments.emplace_back(argName, std::move(value));
				} while (match(lexer::TokenType::COMMA));
			}

			consume(lexer::TokenType::RPAREN); // Consume ')'
		}

		return std::make_unique<ast::AnnotationNode>(loc, name, std::move(arguments));
	}

	std::unique_ptr<ast::ErrorNode> Parser::createErrorNode() {
		return std::make_unique<ast::ErrorNode>(currentToken.loc);
	}

	std::unique_ptr<ast::ImportNode> Parser::parseImport() {
		ast::SourceLocation loc = consume(lexer::TokenType::IMPORT).loc;
		std::string importPath;
		bool isJavaImport = false;

		// Handle Java imports (keyword 'java' followed by package path)
		if (match(lexer::TokenType::JAVA)) {
			advance(); // Consume the 'java' keyword
			isJavaImport = true;

			// Now parse the package path (which may contain 'java' as identifier)
			while (true) {
				importPath += consume(lexer::TokenType::IDENTIFIER).lexeme;
				if (!match(lexer::TokenType::DOT)) break;
				importPath += ".";
				advance(); // Consume '.'
			}
		}
			// Handle quoted path imports
		else if (match(lexer::TokenType::STRING_LIT)) {
			importPath = consume(lexer::TokenType::STRING_LIT).lexeme;
			// Remove surrounding quotes
			if (importPath.size() >= 2 &&
			    importPath.front() == '"' &&
			    importPath.back() == '"') {
				importPath = importPath.substr(1, importPath.size() - 2);
			}
		}
			// Handle normal imports
		else {
			while (true) {
				importPath += consume(lexer::TokenType::IDENTIFIER).lexeme;
				if (!match(lexer::TokenType::DOT)) break;
				importPath += ".";
				advance(); // Consume '.'
			}
		}

		// Optional semicolon
		if (match(lexer::TokenType::SEMICOLON)) {
			advance();
		}

		return std::make_unique<ast::ImportNode>(loc, importPath, isJavaImport);
	}

	std::unique_ptr<ast::ClassDeclNode> Parser::parseClass() {
		ast::SourceLocation classLoc = consume(lexer::TokenType::CLASS).loc;
		std::string className = consume(lexer::TokenType::IDENTIFIER, "Expected class name").lexeme;

		// Parse inheritance
		std::string baseClass;
		if(match(lexer::TokenType::COLON)) {
			consume(lexer::TokenType::COLON);
			baseClass = consume(lexer::TokenType::IDENTIFIER, "Expected base class name").lexeme;
		}

		// Parse class body
		consume(lexer::TokenType::LBRACE, "Expected '{' after class declaration");
		std::vector<std::unique_ptr<ast::MemberDeclNode>> members;

		while (!match(lexer::TokenType::RBRACE) && !isAtEnd()) {
			try {
				// Parse annotations
				std::vector<std::unique_ptr<ast::AnnotationNode>> annotations;
				while (match(lexer::TokenType::AT)) {
					annotations.push_back(parseAnnotation());
				}

				// Parse access modifier
				ast::MemberDeclNode::Access access = ast::MemberDeclNode::PUBLIC;
				if (match(lexer::TokenType::PUBLIC)) {
					advance();
					access = ast::MemberDeclNode::PUBLIC;
				} else if (match(lexer::TokenType::PROTECTED)) {
					advance();
					access = ast::MemberDeclNode::PROTECTED;
				} else if (match(lexer::TokenType::PRIVATE)) {
					advance();
					access = ast::MemberDeclNode::PRIVATE;
				} else if (match(lexer::TokenType::PRIVATEW)) {
					advance();
					access = ast::MemberDeclNode::PRIVATEW;
				} else if (match(lexer::TokenType::PROTECTEDW)) {
					advance();
					access = ast::MemberDeclNode::PROTECTEDW;
				}

				// Parse const modifier
				bool isConst = match(lexer::TokenType::CONST);
				if (isConst) advance();

				// Check if it's a constructor
				if (match(lexer::TokenType::IDENTIFIER) && currentToken.lexeme == className) {
					// Constructor
					ast::SourceLocation loc = advance().loc;
					std::vector<std::pair<std::string, std::unique_ptr<ast::ExprNode>>> initializers;
					auto params = parseParameters();
					if (match(lexer::TokenType::COLON)) {
						advance(); // Consume the ':'

						do {
							std::string memberName = consume(lexer::TokenType::IDENTIFIER, "Expected member name in initializer list").lexeme;
							consume(lexer::TokenType::LPAREN, "Expected '(' after member name");
							auto expr = parseExpression();
							consume(lexer::TokenType::RPAREN, "Expected ')' after initializer expression");

							initializers.emplace_back(memberName, std::move(expr));

						} while (match(lexer::TokenType::COMMA) && (advance(), true));
					}
					auto body = parseBlock();

					members.push_back(std::make_unique<ast::MemberDeclNode>(
							loc,
							ast::MemberDeclNode::METHOD_CONSTRUCTOR,
							access,
							isConst,
							std::move(className),  // Move the string
							nullptr,
							nullptr,  // No field init
							std::move(initializers),  // Ctor inits
							std::move(body),
							std::move(annotations)
					));
				}
					// Check if it's a method (reuse parseFunctionDecl)
				else if (isPotentialMethod()) {
					auto funcDecl = parseFunction();
					// Convert FunctionDeclNode to MemberDeclNode
					members.push_back(std::make_unique<ast::MemberDeclNode>(
							funcDecl->loc,
							ast::MemberDeclNode::METHOD,
							access,
							isConst,
							std::move(funcDecl->name),    // std::string&&
							std::move(funcDecl->returnType), // unique_ptr<TypeNode>
							nullptr,                      // No field init
							std::vector<std::pair<std::string, std::unique_ptr<ast::ExprNode>>>{},// Empty ctor inits
							std::move(funcDecl->body),    // unique_ptr<BlockNode>
							std::move(annotations)        // vector<unique_ptr<AnnotationNode>>
					));
				}
				else {
					// Field declaration
					std::unique_ptr<ast::TypeNode> type;
					if (isBuiltInType(currentToken.type) || currentToken.type == lexer::TokenType::IDENTIFIER) {
						type = parseType();
					}

					std::string name = consume(lexer::TokenType::IDENTIFIER, "Expected type after declaration").lexeme;

					std::unique_ptr<ast::ExprNode> initializer;
					if (match(lexer::TokenType::EQUAL)) {
						advance();
						initializer = parseExpression();
					}

					consume(lexer::TokenType::SEMICOLON, "Expected ';' after field declaration");

					members.push_back(std::make_unique<ast::MemberDeclNode>(
							currentToken.loc,
							ast::MemberDeclNode::FIELD,
							access,
							isConst,
							std::move(name),          // std::string&&
							std::move(type),          // unique_ptr<TypeNode>
							std::move(initializer),   // unique_ptr<ast::ExprNode> (field init)
							std::vector<std::pair<std::string, std::unique_ptr<ast::ExprNode>>>{},// Empty ctor inits
							nullptr,                  // No body
							std::move(annotations)    // vector<unique_ptr<AnnotationNode>>
					));
				}
			} catch (const ParseError& e) {
				errorReporter.report(e.location, e.format());
				errStream << e.what() << std::endl;
				synchronize();
			}
		}

		consume(lexer::TokenType::RBRACE, "Expected '}' after class body");

		return std::make_unique<ast::ClassDeclNode>(
				classLoc,
				std::move(className),
				std::move(baseClass),
				std::move(members)
		);
	}

	bool Parser::peekIsBlockEnd() const {
		return match(lexer::TokenType::RBRACE) || isAtEnd();
	}

	std::pair<std::vector<std::pair<std::string, std::unique_ptr<ast::TypeNode>>>, bool>  Parser::parseParameters() {
		std::vector<std::pair<std::string, std::unique_ptr<ast::TypeNode>>> params;

		consume(lexer::TokenType::LPAREN, "Expected '(' after function declaration");

		bool inStructSyntax = match(lexer::TokenType::LBRACE);

		if (inStructSyntax) {
			consume(lexer::TokenType::LBRACE);
		}
		std::unique_ptr<ast::TypeNode> currentType = nullptr;
		bool firstParam = true;
		while (!(inStructSyntax ? match(lexer::TokenType::RBRACE) : match(lexer::TokenType::RPAREN))) {
			if (!firstParam) consume(lexer::TokenType::COMMA);
			firstParam = false;

			std::unique_ptr<ast::TypeNode> paramType;
			if(isBuiltInType(currentToken.type) || (currentToken.type == lexer::TokenType::IDENTIFIER)){
				paramType = parseType();
			}else if(match({lexer::TokenType::LET,lexer::TokenType::VAR,lexer::TokenType::DYNAMIC})){
				advance();
				paramType = std::make_unique<ast::TypeNode>(currentToken.loc, ast::TypeNode::DYNAMIC);
			}/*else if (lastExplicitType) {
            paramType = lastExplicitType->clone();
			}*/else{
				paramType = std::make_unique<ast::TypeNode>(currentToken.loc, ast::TypeNode::DYNAMIC);
			}
			std::string name = consume(lexer::TokenType::IDENTIFIER, "Expected parameter name").lexeme;
			params.emplace_back(name, std::move(paramType));

		}
		if (inStructSyntax) {
			consume(lexer::TokenType::RBRACE, "Expected '}' to close parameter struct");
			// Now consume the closing paren that wraps the struct
			consume(lexer::TokenType::RPAREN, "Expected ')' after parameter struct");
		}
		else {
			consume(lexer::TokenType::RPAREN, "Expected ')' to close parameter list");
		}

		return {std::move(params), inStructSyntax};
	}

	bool Parser::isPotentialMethod() const {
		// Look ahead to see if this is a method declaration
		size_t lookahead = current;
		while (lookahead < tokens.size()) {
			const lexer::Token& tok = tokens[lookahead];

			// Skip over type parameters or array dimensions
			if (tok.type == lexer::TokenType::LBRACKET) {
				lookahead++;
				continue;
			}

			// If we find '(' before ';' or '=', it's a method
			if (tok.type == lexer::TokenType::LPAREN) {
				return true;
			}

			// If we find these tokens before '(', it's not a method
			if (tok.type == lexer::TokenType::SEMICOLON ||
			    tok.type == lexer::TokenType::EQUAL ||
			    tok.type == lexer::TokenType::LBRACE) {
				return false;
			}

			lookahead++;
		}
		return false;
	}
	bool Parser::isInStructInitializerContext() const {
		// Look back to see if we're after an equals sign following a type name
		if (previous > 0 && tokens[previous].type == lexer::TokenType::EQUAL) {
			// Check if before the equals sign we have a type name
			if (previous > 1 &&
			    (isBuiltInType(tokens[previous-2].type) || tokens[previous-2].type == lexer::TokenType::IDENTIFIER) && tokens[previous-2].type != lexer::TokenType::FREEOBJ) {
				return true;
			}
		}
		return false;
	}

	std::unique_ptr<ast::StructInitializerNode> Parser::parseStructInitializer() {
		ast::SourceLocation loc = consume(lexer::TokenType::LBRACE).loc;
		std::vector<ast::StructInitializerNode::StructFieldInitializer> fields;

		if (!match(lexer::TokenType::RBRACE)) {
			do {
				// Check for designated initializer (C-style .field = value)
				if (match(lexer::TokenType::DOT)) {
					advance(); // Consume '.'
					std::string name = consume(lexer::TokenType::IDENTIFIER).lexeme;
					consume(lexer::TokenType::EQUAL);
					auto value = parseExpression();
					fields.push_back({name, std::move(value)});
				}
					// Check for named field (field: value)
				else if (peek(1).type == lexer::TokenType::COLON) {
					std::string name = consume(lexer::TokenType::IDENTIFIER).lexeme;
					consume(lexer::TokenType::COLON);
					auto value = parseExpression();
					fields.push_back({name, std::move(value)});
				}
					// Positional initialization (just value)
				else {
					auto value = parseExpression();
					fields.push_back({"", std::move(value)}); // Empty name indicates positional
				}
			} while (match(lexer::TokenType::COMMA) && (advance(), true));
		}

		consume(lexer::TokenType::RBRACE);
		return std::make_unique<ast::StructInitializerNode>(loc, std::move(fields));
	}

	std::vector<std::string> Parser::parseArrowFunctionParams() {
		std::vector<std::string> params;
		consume(lexer::TokenType::LPAREN, "Expected '(' before arrow function parameters");

		if (!match(lexer::TokenType::RPAREN)) {
			do {
				params.push_back(consume(lexer::TokenType::IDENTIFIER, "Expected parameter name").lexeme);
			} while (match(lexer::TokenType::COMMA) && (advance(), true));
		}

		consume(lexer::TokenType::RPAREN, "Expected ')' after arrow function parameters");
		return params;
	}

	std::unique_ptr<ast::LambdaExprNode> Parser::parseArrowFunction(std::vector<std::string>&& params) {
		ast::SourceLocation loc = consume(lexer::TokenType::LAMBARROW).loc;

		// Convert string params to typed params (with null types for lambdas)
		std::vector<std::pair<std::string, std::unique_ptr<ast::TypeNode>>> typedParams;
		typedParams.reserve(params.size());
		for (auto& param : params) {
			typedParams.emplace_back(std::move(param), nullptr);
		}
		std::unique_ptr<ast::LambdaNode> lambda;
		// Handle single-expression body
		if (!match(lexer::TokenType::LBRACE)) {
			auto expr = parseExpression();
			auto stmts = std::vector<std::unique_ptr<ast::ASTNode>>();
			stmts.push_back(std::make_unique<ast::ReturnStmtNode>(loc, std::move(expr)));
			auto body = std::make_unique<ast::BlockNode>(loc, std::move(stmts));

			lambda = std::make_unique<ast::LambdaNode>(loc,std::move(typedParams),nullptr, std::move(body),false);
			return std::make_unique<ast::LambdaExprNode>(loc,std::move(lambda));
		}

		// Handle block body
		auto body = parseBlock();
		lambda = std::make_unique<ast::LambdaNode>(loc,std::move(typedParams),nullptr,std::move(body),false);

		return std::make_unique<ast::LambdaExprNode>(loc,std::move(lambda));
	}
}