#include "parser.hpp"
#include <memory>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include "../ast/Expressions.hpp"
#include "../exceptions/ParseError.hpp"

namespace zenith {
	// Fixed parseVarDecl to return the node
	std::unique_ptr<VarDeclNode> Parser::parseVarDecl() {
		SourceLocation loc = currentToken.loc;
		bool isHoisted = match(TokenType::HOIST);
		VarDeclNode::Kind kind = VarDeclNode::DYNAMIC; // Default to dynamic
		std::unique_ptr<TypeNode> typeNode;

		// Case 1: Static typed declaration (int a = 12)
		if (isBuiltInType(currentToken.type) || currentToken.type == TokenType::IDENTIFIER) {
			kind = VarDeclNode::STATIC;
			typeNode = parseType();
		}
		// Case 2: Dynamic declaration (let/var/dynamic)
		else if (match(TokenType::LET) || match(TokenType::VAR) || match(TokenType::DYNAMIC)) {
			kind = VarDeclNode::DYNAMIC;
			advance();
		}

		std::string name = consume(TokenType::IDENTIFIER).lexeme;

		// Handle type annotation for dynamic variables (let a: int = 10)
		if (kind == VarDeclNode::DYNAMIC && match(TokenType::COLON)) {
			typeNode = parseType();
		}

		// Handle initialization
		std::unique_ptr<ExprNode> initializer;
		if (match(TokenType::EQUAL)) {
			consume(TokenType::EQUAL);
			initializer = parseExpression();

			// Special case: Class initialization (SomeClassName a = new SomeClassName())
			if (kind == VarDeclNode::STATIC && initializer->isConstructorCall()) {
				kind = VarDeclNode::CLASS_INIT;
			}
		}

		return std::make_unique<VarDeclNode>(
				loc, kind, std::move(name),
				std::move(typeNode), std::move(initializer),
				isHoisted
		);
	}

	bool Parser::match(TokenType type) const {
		if (isAtEnd()) return false;
		return currentToken.type == type;
	}

	bool Parser::match(std::initializer_list<TokenType> types) const {
		if (isAtEnd()) return false;
		return std::find(types.begin(), types.end(), currentToken.type) != types.end();
	}

	Token Parser::consume(TokenType type, const std::string& errorMessage) {
		if (match(type)) {
			return advance();
		}
		throw ParseError(currentToken.loc, errorMessage);
	}

	Token Parser::consume(TokenType type) {
		return consume(type, "Expected " + Lexer::tokenToString(type));
	}

	std::unique_ptr<ExprNode> Parser::parseExpression(int precedence) { // Remove default arg here
		auto expr = parsePrimary();

		while (true) {
			Token op = currentToken;
			int opPrecedence = getPrecedence(op.type);

			if (opPrecedence <= precedence) break;

			advance();
			auto right = parseExpression(opPrecedence);
			expr = std::make_unique<BinaryOpNode>(
					op.loc, // Pass operator location
					op.type,
					std::move(expr),
					std::move(right)
			);
		}

		return expr;
	}

	std::unique_ptr<ExprNode> Parser::parsePrimary() {
		SourceLocation startLoc = currentToken.loc;

		// Handle all the simple cases first
		if (match(TokenType::NEW)) {
			return parseNewExpression();
		}
		else if (match({TokenType::NUMBER,TokenType::INTEGER,TokenType::FLOAT_LIT})) {
			Token numToken = advance();
			return std::make_unique<LiteralNode>(startLoc, LiteralNode::NUMBER, numToken.lexeme);
		}
		else if (match(TokenType::STRING_LIT)) {
			Token strToken = advance();
			return std::make_unique<LiteralNode>(startLoc, LiteralNode::STRING, strToken.lexeme);
		} else if (match(TokenType::TRUE) || match(TokenType::FALSE)) {
			Token boolToken = advance();
			return std::make_unique<LiteralNode>(startLoc, LiteralNode::BOOL, boolToken.lexeme);
		}
		else if (match(TokenType::NULL_LIT)) {
			advance();
			return std::make_unique<LiteralNode>(startLoc, LiteralNode::NIL, "nil");
		}
		else if (match(TokenType::LBRACE)) {
			if (isInStructInitializerContext()) {
				return parseStructInitializer();
			}
			return parseFreeObject();
		} else if (match(TokenType::FREEOBJ)) {
			advance(); // consume 'freeobj'
			if (match(TokenType::LBRACE)) {
				return parseFreeObject();
			}
			throw ParseError(startLoc,"Expected '{' after free obj, Achievement unlocked: How did we get here?");
		}
		else if (match(TokenType::LPAREN)) {
				auto params = parseArrowFunctionParams();
				if (match(TokenType::LAMBARROW)) {
					return parseArrowFunction(std::move(params));
				}
				// else treat as normal parenthesized expression
			}
		else if (match({TokenType::IDENTIFIER, TokenType::THIS})) {
			Token identToken = advance();
			std::unique_ptr<ExprNode> expr;

			if (identToken.type == TokenType::THIS) {
				expr = std::make_unique<ThisNode>(startLoc);
			} else {
				expr = std::make_unique<VarNode>(startLoc, identToken.lexeme);
			}

			// Handle chained operations
			while (true) {
				if (match(TokenType::LPAREN)) {
					expr = parseFunctionCall(std::move(expr));  // std::move here
				}
				else if (match(TokenType::DOT)) {
					expr = parseMemberAccess(std::move(expr));  // std::move here
				}
				else if (match(TokenType::LBRACKET)) {
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
		                 Lexer::tokenToString(currentToken.type));
	}

	Token Parser::advance() {
		if (isAtEnd()) {
			return Token{TokenType::EOF_TOKEN, "",
			             tokens.empty() ? SourceLocation{1,1,0, 0} : tokens.back().loc};
		}

		Token result = tokens[current];  // 1. Get current token first
		previous = current;

		// 2. Only advance if not already at end
		if (current + 1 < tokens.size()) {
			current++;
			currentToken = tokens[current];
		} else {
			current = tokens.size();  // Mark as ended
			currentToken = Token{TokenType::EOF_TOKEN, "", result.loc};
		}

		return result;  // Return what was current when we entered
	}

	Parser::Parser(std::vector<Token> tokens, const Flags& flags, std::ostream& errStream)
			: tokens(std::move(tokens)),
			  currentToken(this->tokens.empty() ?
			               Token{TokenType::EOF_TOKEN, "", {1, 1, 0}} :
			               this->tokens[0]) , flags(flags), errStream(errStream), errorReporter(std::cout) {
		current = 0;
	}

	bool Parser::isAtEnd() const {
		return current >= tokens.size();
	}
	std::unique_ptr<TypeNode> Parser::parseType() {
		SourceLocation startLoc = currentToken.loc;

		// Handle built-in types (from TokenType)
		if (match({TokenType::INT, TokenType::LONG, TokenType::SHORT,
				   TokenType::BYTE, TokenType::FLOAT, TokenType::DOUBLE,
				   TokenType::STRING, TokenType::NUMBER, TokenType::BIGINT,
				   TokenType::BIGNUMBER, TokenType::FREEOBJ, TokenType::BOOL})) {

			Token typeToken = advance();

			PrimitiveTypeNode::Type kind;
			if (typeToken.type == TokenType::INT) kind = PrimitiveTypeNode::INT;
			else if (typeToken.type == TokenType::LONG) kind = PrimitiveTypeNode::LONG;
			else if (typeToken.type == TokenType::BOOL) kind = PrimitiveTypeNode::BOOL;
			else if (typeToken.type == TokenType::SHORT) kind = PrimitiveTypeNode::SHORT;
			else if (typeToken.type == TokenType::BYTE) kind = PrimitiveTypeNode::BYTE;
			else if (typeToken.type == TokenType::FLOAT) kind = PrimitiveTypeNode::FLOAT;
			else if (typeToken.type == TokenType::DOUBLE) kind = PrimitiveTypeNode::DOUBLE;
			else if (typeToken.type == TokenType::STRING) kind = PrimitiveTypeNode::STRING;
			else if (typeToken.type == TokenType::NUMBER) kind = PrimitiveTypeNode::NUMBER;
			else if (typeToken.type == TokenType::BIGINT) kind = PrimitiveTypeNode::BIGINT;
			else if (typeToken.type == TokenType::FREEOBJ) {
				// For freeobj, we'll return a TypeNode with DYNAMIC kind since it's a special case
				return std::make_unique<TypeNode>(startLoc, TypeNode::DYNAMIC);
			}
			else kind = PrimitiveTypeNode::BIGNUMBER;

			return std::make_unique<PrimitiveTypeNode>(startLoc, kind);
		}
		else if (match(TokenType::LBRACKET)) {
			// Array type (e.g., [int] or [MyClass])
			advance(); // Consume '['
			auto elementType = parseType();
			consume(TokenType::RBRACKET, "Expected ']' after array type");
			return std::make_unique<ArrayTypeNode>(startLoc, std::move(elementType));
		}
		else if (match(TokenType::IDENTIFIER)) {
			// User-defined type (class/struct/type alias) - now with template support
			Token typeToken = advance();
			std::string baseName = typeToken.lexeme;

			// Check for template arguments
			if (peekIsTemplateStart()) {
				consume(TokenType::LESS); // Eat '<'

				std::vector<std::unique_ptr<TypeNode>> templateArgs;
				if (!match(TokenType::GREATER)) {
					do {
						templateArgs.push_back(parseType());
					} while (match(TokenType::COMMA) && (advance(), true));
				}

				consume(TokenType::GREATER, "Expected '>' to close template arguments");

				return std::make_unique<TemplateTypeNode>(
						startLoc,
						baseName,
						std::move(templateArgs)
				);
			}

			// Non-templated case
			return std::make_unique<NamedTypeNode>(startLoc, baseName);
		}

		throw ParseError(
				currentToken.loc,
				"Expected type name, got " + Lexer::tokenToString(currentToken.type)
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
				case TokenType::CLASS:
				case TokenType::FUN:
				case TokenType::LET:
				case TokenType::VAR:
				case TokenType::STRUCT:
				case TokenType::IMPORT:
					return;

					// Statement terminators
				case TokenType::SEMICOLON:
				case TokenType::RBRACE:
					advance(); // Consume the terminator
					return;

					// Expression-level recovery points
				case TokenType::IF:
				case TokenType::FOR:
				case TokenType::WHILE:
				case TokenType::RETURN:
				case TokenType::LESS:
				case TokenType::GREATER:
					return;
			}

			// 4. Skip all other tokens
			advance();
		}
	}

	const Token &Parser::previousToken() const {
		if (previous >= tokens.size()) {
			static Token eof{TokenType::EOF_TOKEN, "", {0,0,0}};
			return eof;
		}
		return tokens[previous];
	}

	int Parser::getPrecedence(TokenType type) {
		static const std::unordered_map<TokenType, int> precedences = {
				// Assignment
				{TokenType::EQUAL, 1},
				{TokenType::PLUS_EQUALS, 1},
				{TokenType::MINUS_EQUALS, 1},
				{TokenType::STAR_EQUALS, 1},
				{TokenType::SLASH_EQUALS, 1},
				{TokenType::PERCENT_EQUALS, 1},

				// Logical
				{TokenType::OR, 2},
				{TokenType::AND, 3},

				// Equality
				{TokenType::BANG_EQUAL, 4},
				{TokenType::EQUAL_EQUAL, 4},

				// Comparison
				{TokenType::LESS, 5},
				{TokenType::LESS_EQUAL, 5},
				{TokenType::GREATER, 5},
				{TokenType::GREATER_EQUAL, 5},

				// Arithmetic
				{TokenType::PLUS, 6},
				{TokenType::MINUS, 6},
				{TokenType::STAR, 7},
				{TokenType::SLASH, 7},
				{TokenType::PERCENT, 7}

		};

		auto it = precedences.find(type);
		return it != precedences.end() ? it->second : 0;
	}

	std::unique_ptr<ProgramNode> Parser::parse() {
		SourceLocation startLoc = currentToken.loc;
		std::vector<std::unique_ptr<ASTNode>> declarations;
		while (!isAtEnd()) {
			pendingAnnotations.clear();
			pendingAnnotations = parseAnnotations();
			try {
				auto annotations = parseAnnotations();
				if (match(TokenType::IMPORT)) {
					declarations.push_back(parseImport());
				}
				else if (match({TokenType::CLASS,TokenType::STRUCT})) {
					declarations.push_back(parseObject());
				}
				else if (match(TokenType::UNION)) {
					declarations.push_back(parseUnion());
				}
				else if (match(TokenType::FUN)) {
					declarations.push_back(parseFunction());
				}
				else if (isBuiltInType(currentToken.type) || currentToken.type == TokenType::IDENTIFIER) {
					// Handle both built-in types and user-defined types
					if (isPotentialMethod()) {
						declarations.push_back(parseFunction());
					} else {
						// It's a variable declaration
						declarations.push_back(parseVarDecl());
					}
				}
				else if (match({TokenType::LET, TokenType::VAR, TokenType::DYNAMIC})) {
					declarations.push_back(parseVarDecl());
				}
				else if (match(TokenType::ACTOR)) {
					declarations.push_back(parseActorDecl());
				}
				else if (!annotations.empty()) {
					throw ParseError(currentToken.loc,
					                 "Annotations must precede a declaration");
				}
				else {
					// Handle other top-level constructs
					advance();
				}
				if(!declarations.empty()) {
					if (auto annotatable = dynamic_cast<IAnnotatable *>(declarations.back().get())) {
						annotatable->setAnnotations(std::move(pendingAnnotations));
					}
					else if (!pendingAnnotations.empty()) {
					throw ParseError(currentToken.loc,
					                 "Annotations cannot be applied to this declaration type");
					}
				}
			} catch (const ParseError& e) {
				errorReporter.report(e.location,e.format());
				errStream << e.what() << std::endl;
				synchronize();
			}
		}
		auto program = std::make_unique<ProgramNode>(startLoc,std::move(declarations));

		return program;
	}

	bool Parser::isBuiltInType(TokenType type) {
		static const std::unordered_set<TokenType> builtInTypes = {
				TokenType::INT, TokenType::LONG, TokenType::SHORT, TokenType::BYTE, TokenType::FLOAT, TokenType::DOUBLE, TokenType::STRING, TokenType::FREEOBJ, TokenType::BOOL
		};
		return builtInTypes.count(type) > 0;
	}

	std::unique_ptr<NewExprNode> Parser::parseNewExpression() {
		SourceLocation location = consume(TokenType::NEW).loc; // Eat 'new' keyword
		std::string className = consume(TokenType::IDENTIFIER).lexeme;

		consume(TokenType::LPAREN);
		std::vector<std::unique_ptr<ExprNode>> args;
		if (!match(TokenType::RPAREN)) {
			do {
				args.push_back(parseExpression());
			} while (match(TokenType::COMMA));
			consume(TokenType::RPAREN);
		}

		return std::make_unique<NewExprNode>(location,className, std::move(args));
	}

	std::unique_ptr<FunctionDeclNode> Parser::parseFunction() {
		SourceLocation loc = currentToken.loc;
		bool hasFunKeyword = false;

		// Handle annotations
		bool isAsync = std::find_if(pendingAnnotations.begin(), pendingAnnotations.end(), [](const std::unique_ptr<AnnotationNode>& ann) {
			return ann->name == "Async";
		}) != pendingAnnotations.end();


		// Parse return type (optional)
		std::unique_ptr<TypeNode> returnType;
		if (match(TokenType::FUN)) {
			hasFunKeyword = true;
			advance();

			// Only parse return type if followed by type specifier
			if (isBuiltInType(currentToken.type) || currentToken.type == TokenType::IDENTIFIER) {
				// Peek ahead to distinguish:
				// 1) 'fun int foo' -> returnType=int
				// 2) 'fun foo'     -> function name
				if (peek(1).type == TokenType::IDENTIFIER && peek(2).type == TokenType::LPAREN) {
					returnType = parseType();
				}
			}
		}
			// Handle C-style declarations (e.g. "int foo()")
		else if (isBuiltInType(currentToken.type) || currentToken.type == TokenType::IDENTIFIER) {
			returnType = parseType();
		}
		std::string name = consume(TokenType::IDENTIFIER).lexeme;

		// Get both params and structSugar flag
		auto [params, structSugar] = parseParameters();

		if(match(TokenType::ARROW)){
			advance();
			returnType = parseType();
		}
		auto body = parseBlock();

		return std::make_unique<FunctionDeclNode>(
				loc,
				std::move(name),
				std::move(params),
				std::move(returnType),
				std::move(body),
				isAsync,
				structSugar,
				std::move(pendingAnnotations)
		);
	}

	std::unique_ptr<BlockNode> Parser::parseBlock() {
		SourceLocation startLoc = currentToken.loc;
		consume(TokenType::LBRACE);

		std::vector<std::unique_ptr<ASTNode>> statements;
		try {
			while (!match(TokenType::RBRACE) && !isAtEnd()) {
				statements.push_back(parseStatement());
			}
			consume(TokenType::RBRACE,"Expected '}' after block");
		} catch (const ParseError&) {
			synchronize(); // Your error recovery method
			if (!match(TokenType::RBRACE)) {
				// Insert synthetic '}' if missing
				statements.push_back(createErrorNode());
			}
		}

		return std::make_unique<BlockNode>(startLoc, std::move(statements));
	}

	std::unique_ptr<StmtNode> Parser::parseStatement() {
		SourceLocation loc = currentToken.loc;

		// Declaration statements
		if (match({TokenType::LET, TokenType::VAR, TokenType::DYNAMIC})) {
			return parseVarDecl();
		}

		// Static variable declarations (primitive or class type)
		if (isBuiltInType(currentToken.type) || currentToken.type == TokenType::IDENTIFIER) {
			// Check if this is actually a declaration by looking ahead
			if (peek(1).type == TokenType::IDENTIFIER &&
			    (peek(2).type == TokenType::EQUAL || peek(2).type == TokenType::SEMICOLON)) {
				return parseVarDecl();
			}
		}

		if (match(TokenType::THIS)) {
			auto expr = parseExpression();
			if (match(TokenType::SEMICOLON)) advance();
			return std::make_unique<ExprStmtNode>(loc, std::move(expr));
		}

		// Control flow statements
		if (match(TokenType::IF)) return parseIfStmt();
		if (match(TokenType::FOR)) return parseForStmt();
		if (match(TokenType::WHILE)) return parseWhileStmt();
		if (match(TokenType::DO)) return parseDoWhileStmt();
		if (match(TokenType::RETURN)) return parseReturnStmt();

		// Unsafe blocks
		if (match(TokenType::UNSAFE)) {
			advance(); // Consume 'unsafe'
			return parseBlock();
		}
		if (match(TokenType::SCOPE)) {
			return parseScopeBlock();
		}
		// Block statements
		if (match(TokenType::LBRACE)) {
			return parseBlock();
		}

		// Expression statements
		if (peekIsExpressionStart()) {
			auto expr = parseExpression();
			// Optional semicolon (Zenith allows brace-optional syntax)
			if (match(TokenType::SEMICOLON)) {
				advance();
			}
			return std::make_unique<ExprStmtNode>(loc, std::move(expr));
		}

		// Empty statement (just a semicolon)
		if (match(TokenType::SEMICOLON)) {
			advance();
			return std::make_unique<EmptyStmtNode>(loc);
		}

		// Error recovery
		throw ParseError(currentToken.loc, "Unexpected token in statement: " + currentToken.lexeme);
	}

	bool Parser::peekIsExpressionStart() const {
		switch (currentToken.type) {
			case TokenType::SCOPE:
			case TokenType::IDENTIFIER:
			case TokenType::INTEGER:
			case TokenType::FLOAT_LIT:
			case TokenType::STRING_LIT:
			case TokenType::LPAREN:          //Maybe also arrow lambda
			case TokenType::LBRACE:         // For object literals
			case TokenType::NEW:           // Constructor calls
			case TokenType::BANG:         // Unary operators
			case TokenType::MINUS:
			case TokenType::PLUS:
			case TokenType::THIS:

				return true;
			default:
				return false;
		}
	}

	Token Parser::peek(size_t offset) const {
		size_t idx = current + offset;
		if (idx >= tokens.size()) {
			return Token{TokenType::EOF_TOKEN, "",
			             tokens.empty() ? SourceLocation{1,1,0} : tokens.back().loc};
		}
		return tokens[idx];

	}

	std::unique_ptr<IfNode> Parser::parseIfStmt() {
		SourceLocation loc = consume(TokenType::IF).loc;
		consume(TokenType::LPAREN, "Expected '(' after 'if'");
		auto condition = parseExpression();
		consume(TokenType::RPAREN, "Expected ')' after if condition");

		// Parse 'then' branch (enforce braces if required)
		std::unique_ptr<StmtNode> thenBranch;
		try {
			if (flags.bracesRequired && !match(TokenType::LBRACE)) {
				throw ParseError(currentToken.loc, "Expected '{' after 'if'");
			}
			thenBranch = parseStatement();
		} catch (const ParseError& e) {
			errorReporter.report(e.location,"Error in if body " + e.format());
			errStream << "Error in if body: " << e.what() << std::endl;
			synchronize(); // Skip to next statement
			auto errorNode = createErrorNode();
			thenBranch = std::unique_ptr<StmtNode>(dynamic_cast<StmtNode*>(errorNode.release()));
			if (!thenBranch) {  // Fallback if cast fails
				thenBranch = std::make_unique<EmptyStmtNode>(errorNode->loc);
			}
		}

		// Parse 'else' branch (also enforce braces if required)
		std::unique_ptr<ASTNode> elseBranch;
		if (match(TokenType::ELSE)) {
			advance();
			if (flags.bracesRequired) {
				if (!match(TokenType::LBRACE)) {
					throw ParseError(currentToken.loc,
					                 "Expected '{' after 'else' (braces are required)");
				}
				elseBranch = parseBlock();
			} else {
				elseBranch = parseStatement();
			}
		}

		return std::make_unique<IfNode>(
				loc,
				std::move(condition),
				std::move(thenBranch),
				std::move(elseBranch)
		);
	}

	std::unique_ptr<ForNode> Parser::parseForStmt() {
		SourceLocation loc = consume(TokenType::FOR).loc;
		consume(TokenType::LPAREN);

		// Parse init/condition/increment (unchanged)
		std::unique_ptr<ASTNode> init;
		if (match(TokenType::SEMICOLON)) {
			advance(); // Empty initializer
		}
		else if (match({TokenType::LET, TokenType::VAR, TokenType::DYNAMIC}) ||
		         isBuiltInType(currentToken.type)) {
			init = parseVarDecl();
		}
		else {
			init = std::make_unique<ExprStmtNode>(currentToken.loc, parseExpression());
		}
		consume(TokenType::SEMICOLON);

		std::unique_ptr<ExprNode> condition;
		if (!match(TokenType::SEMICOLON)) {
			condition = parseExpression();
		}
		consume(TokenType::SEMICOLON);

		std::unique_ptr<ExprNode> increment;
		if (!match(TokenType::RPAREN)) {
			increment = parseExpression();
		}
		consume(TokenType::RPAREN);

		// Enforce braces if required
		if (flags.bracesRequired && !match(TokenType::LBRACE)) {
			throw ParseError(currentToken.loc,
			                 "Expected '{' after 'for' (braces are required)");
		}

		auto body = parseStatement(); // parseBlock() if braces are required
		return std::make_unique<ForNode>(loc, std::move(init), std::move(condition),
		                                 std::move(increment), std::move(body));
	}

	std::unique_ptr<WhileNode> Parser::parseWhileStmt() {
		SourceLocation loc = consume(TokenType::WHILE).loc;
		consume(TokenType::LPAREN, "Expected '(' after 'while'");
		auto condition = parseExpression();
		consume(TokenType::RPAREN, "Expected ')' after while condition");

		// Enforce braces if required
		if (flags.bracesRequired && !match(TokenType::LBRACE)) {
			throw ParseError(currentToken.loc,
			                 "Expected '{' after 'while' (braces are required)");
		}

		auto body = parseStatement(); // parseBlock() if braces are required
		return std::make_unique<WhileNode>(loc, std::move(condition), std::move(body));
	}

	std::unique_ptr<DoWhileNode> Parser::parseDoWhileStmt() {
		SourceLocation loc = consume(TokenType::DO).loc;

		// Enforce braces if required
		if (flags.bracesRequired && !match(TokenType::LBRACE)) {
			throw ParseError(currentToken.loc,
			                 "Expected '{' after 'do' (braces are required)");
		}

		auto body = parseStatement(); // parseBlock() if braces are required

		consume(TokenType::WHILE, "Expected 'while' after do body");
		consume(TokenType::LPAREN, "Expected '(' after 'while'");
		auto condition = parseExpression();
		consume(TokenType::RPAREN, "Expected ')' after while condition");

		// Optional semicolon (Zenith allows it)
		if (match(TokenType::SEMICOLON)) {
			advance();
		}

		return std::make_unique<DoWhileNode>(loc, std::move(condition), std::move(body));
	}

	std::unique_ptr<ReturnStmtNode> Parser::parseReturnStmt() {
		SourceLocation loc = consume(TokenType::RETURN).loc;

		// Handle empty return (no expression)
		if (match(TokenType::SEMICOLON)) {
			advance();  // Consume the semicolon
			return std::make_unique<ReturnStmtNode>(loc, nullptr);
		}

		// Parse return value (optional in Zenith)
		std::unique_ptr<ExprNode> value;
		if (!peekIsStatementTerminator()) {
			value = parseExpression();
		}

		// Zenith allows optional semicolon after return
		if (match(TokenType::SEMICOLON)) {
			advance();
		}

		// Zenith-specific: Check for annotations
		while (match(TokenType::AT)) {
			parseAnnotation();
		}

		return std::make_unique<ReturnStmtNode>(loc, std::move(value));
	}

	bool Parser::peekIsStatementTerminator() const {
		return match({TokenType::SEMICOLON, TokenType::RBRACE, TokenType::EOF_TOKEN});
	}

	std::unique_ptr<FreeObjectNode> Parser::parseFreeObject() {
		SourceLocation loc = consume(TokenType::LBRACE).loc;
		std::vector<std::pair<std::string, std::unique_ptr<ExprNode>>> properties;

		if (!match(TokenType::RBRACE)) {
			do {
				std::string name = consume(TokenType::IDENTIFIER).lexeme;
				consume(TokenType::COLON);
				auto value = parseExpression();
				properties.emplace_back(name, std::move(value));
				if (match(TokenType::COMMA)) {
					consume(TokenType::COMMA);  // Actually consume the comma
				} else {
					break;  // No more properties
				}
			} while (true);
		}

		consume(TokenType::RBRACE);
		return std::make_unique<FreeObjectNode>(loc, std::move(properties));
	}

	std::unique_ptr<CallNode> Parser::parseFunctionCall(std::unique_ptr<ExprNode> callee) {
		SourceLocation loc = callee->loc;
		consume(TokenType::LPAREN);

		std::vector<std::unique_ptr<ExprNode>> args;
		if (!match(TokenType::RPAREN)) {
			do {
				args.push_back(parseExpression());
			} while (match(TokenType::COMMA));
		}

		consume(TokenType::RPAREN);
		return std::make_unique<CallNode>(loc, std::move(callee), std::move(args));
	}

	std::unique_ptr<MemberAccessNode> Parser::parseMemberAccess(std::unique_ptr<ExprNode> object) {
		// Horse guarantee: We only get called when we see a DOT, if not I'm a horse or tramvai
		SourceLocation loc = object->loc;

		// First access (guaranteed to exist)
		advance(); // Consume '.'
		std::string member = consume(TokenType::IDENTIFIER).lexeme;
		auto result = std::make_unique<MemberAccessNode>(loc, std::move(object), member);

		// Handle additional accesses or calls
		while (match(TokenType::DOT)) {
			advance();
			member = consume(TokenType::IDENTIFIER).lexeme;
			result = std::make_unique<MemberAccessNode>(loc, std::move(result), member);

			if (match(TokenType::LPAREN)) {
				// Temporarily degrade to ExprNode for the call
				auto temp = std::unique_ptr<ExprNode>(result.release());
				temp = parseFunctionCall(std::move(temp));
				// Then immediately cast back - if this fails, we deserve to be horses
				result.reset(dynamic_cast<MemberAccessNode*>(temp.release()));
			}
		}

		return result;
	}

	std::unique_ptr<ExprNode> Parser::parseArrayAccess(std::unique_ptr<ExprNode> arrayExpr) {
		SourceLocation loc = currentToken.loc;

		// Keep processing chained array accesses (e.g., arr[1][2][3])
		while (match(TokenType::LBRACKET)) {
			advance(); // Consume '['

			auto indexExpr = parseExpression();
			consume(TokenType::RBRACKET, "Expected ']' after array index");

			// Create new array access node
			arrayExpr = std::make_unique<ArrayAccessNode>(
					loc,
					std::move(arrayExpr),  // The array being accessed
					std::move(indexExpr)   // The index expression
			);
		}

		return arrayExpr;
	}

	std::unique_ptr<AnnotationNode> Parser::parseAnnotation() {
		SourceLocation loc = consume(TokenType::AT).loc; // Eat '@' symbol

		// Parse annotation name
		std::string name = consume(TokenType::IDENTIFIER).lexeme;

		// Parse optional annotation arguments
		std::vector<std::pair<std::string, std::unique_ptr<ExprNode>>> arguments;

		if (match(TokenType::LPAREN)) {
			advance(); // Consume '('

			if (!match(TokenType::RPAREN)) {
				do {
					// Parse argument name (optional) or just value
					std::string argName;
					if (peek(1).type == TokenType::EQUAL) {
						argName = consume(TokenType::IDENTIFIER).lexeme;
						consume(TokenType::EQUAL);
					}

					// Parse argument value
					auto value = parseExpression();

					arguments.emplace_back(argName, std::move(value));
				} while (match(TokenType::COMMA));
			}

			consume(TokenType::RPAREN); // Consume ')'
		}

		return std::make_unique<AnnotationNode>(loc, name, std::move(arguments));
	}

	std::unique_ptr<ErrorNode> Parser::createErrorNode() {
		return std::make_unique<ErrorNode>(currentToken.loc);
	}

	std::unique_ptr<ImportNode> Parser::parseImport() {
		SourceLocation loc = consume(TokenType::IMPORT).loc;
		std::string importPath;
		bool isJavaImport = false;

		// Handle Java imports (keyword 'java' followed by package path)
		if (match(TokenType::JAVA)) {
			advance(); // Consume the 'java' keyword
			isJavaImport = true;

			// Now parse the package path (which may contain 'java' as identifier)
			while (true) {
				importPath += consume(TokenType::IDENTIFIER).lexeme;
				if (!match(TokenType::DOT)) break;
				importPath += ".";
				advance(); // Consume '.'
			}
		}
			// Handle quoted path imports
		else if (match(TokenType::STRING_LIT)) {
			importPath = consume(TokenType::STRING_LIT).lexeme;
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
				importPath += consume(TokenType::IDENTIFIER).lexeme;
				if (!match(TokenType::DOT)) break;
				importPath += ".";
				advance(); // Consume '.'
			}
		}

		// Optional semicolon
		if (match(TokenType::SEMICOLON)) {
			advance();
		}

		return std::make_unique<ImportNode>(loc, importPath, isJavaImport);
	}

	std::unique_ptr<ObjectDeclNode> Parser::parseObject() {
		if(!match({TokenType::STRUCT,TokenType::CLASS})){
			errorReporter.report(currentToken.loc, "Uhhh, +1 the compiler fucked up point", "Internal Error");
		}
		bool isClass = match(TokenType::CLASS);
		TokenType objectType = isClass ? TokenType::CLASS : TokenType::STRUCT;
		ObjectDeclNode::Kind kind = isClass ? ObjectDeclNode::Kind::CLASS : ObjectDeclNode::Kind::STRUCT;

		SourceLocation classLoc;
		if(isClass)
			classLoc = consume(TokenType::CLASS).loc;
		else
			classLoc = consume(TokenType::STRUCT).loc;
		std::string className = consume(TokenType::IDENTIFIER, "Expected object name").lexeme;

		// Parse inheritance
		std::string baseClass;
		if(match(TokenType::COLON)) {
			consume(TokenType::COLON);
			baseClass = consume(TokenType::IDENTIFIER, "Expected base object name").lexeme;
		}

		// Parse class body
		consume(TokenType::LBRACE, "Expected '{' after object declaration");
		std::vector<std::unique_ptr<MemberDeclNode>> members;

		while (!match(TokenType::RBRACE) && !isAtEnd()) {
			try {
				// Parse annotations
				std::vector<std::unique_ptr<AnnotationNode>> annotations;
				while (match(TokenType::AT)) {
					annotations.push_back(parseAnnotation());
				}
				// Parse access modifier
				members.push_back(parseObjectPrimary(className, annotations));

			} catch (const ParseError& e) {
				errorReporter.report(e.location, e.format());
				errStream << e.what() << std::endl;
				synchronize();
			}
		}

		consume(TokenType::RBRACE, "Expected '}' after object body");

		return std::make_unique<ObjectDeclNode>(
				classLoc,
				kind,
				std::move(className),
				std::move(baseClass),
				std::move(members)
		);
	}

	std::unique_ptr<MemberDeclNode> Parser::parseObjectPrimary(std::string &name, std::vector<std::unique_ptr<AnnotationNode>> &annotations, MemberDeclNode::Access defaultLevel) {
		MemberDeclNode::Access access = defaultLevel;
		if (match(TokenType::PUBLIC)) {
			advance();
			access = MemberDeclNode::PUBLIC;
		} else if (match(TokenType::PROTECTED)) {
			advance();
			access = MemberDeclNode::PROTECTED;
		} else if (match(TokenType::PRIVATE)) {
			advance();
			access = MemberDeclNode::PRIVATE;
		} else if (match(TokenType::PRIVATEW)) {
			advance();
			access = MemberDeclNode::PRIVATEW;
		} else if (match(TokenType::PROTECTEDW)) {
			advance();
			access = MemberDeclNode::PROTECTEDW;
		}

		// Parse const modifier
		bool isConst = match(TokenType::CONST);
		if (isConst) advance();

		// Check if it's a constructor
		if (match(TokenType::IDENTIFIER) && currentToken.lexeme == name) {
			// Constructor
			return parseConstructor(access, isConst, name, annotations);
		}
			// Check if it's a method (reuse parseFunctionDecl)
		else if (isPotentialMethod()) {
			auto funcDecl = parseFunction();
			// Convert FunctionDeclNode to MemberDeclNode
			 return std::make_unique<MemberDeclNode>(
					funcDecl->loc,
					MemberDeclNode::METHOD,
					access,
					isConst,
					std::move(funcDecl->name),    // std::string&&
					std::move(funcDecl->returnType), // unique_ptr<TypeNode>
					nullptr,                      // No field init
					std::vector<std::pair<std::string, std::unique_ptr<ExprNode>>>{},// Empty ctor inits
					std::move(funcDecl->body),    // unique_ptr<BlockNode>
					std::move(annotations)        // vector<unique_ptr<AnnotationNode>>
			);
		}
		else {
			// Field declaration
			return parseField(annotations, access, isConst);
		}
	}

	std::unique_ptr<MemberDeclNode> Parser::parseField(std::vector<std::unique_ptr<AnnotationNode>> &annotations, const MemberDeclNode::Access &access, bool isConst) {
		std::unique_ptr<TypeNode> type;
		if (isBuiltInType(currentToken.type) || currentToken.type == TokenType::IDENTIFIER) {
			type = parseType();
		}

		std::string name = consume(TokenType::IDENTIFIER, "Expected type after declaration").lexeme;

		std::unique_ptr<ExprNode> initializer;
		if (match(TokenType::EQUAL)) {
			advance();
			initializer = parseExpression();
		}

		consume(TokenType::SEMICOLON, "Expected ';' after field declaration");

		return std::make_unique<MemberDeclNode>(
				currentToken.loc,
				MemberDeclNode::FIELD,
				access,
				isConst,
				std::move(name),
				std::move(type),
				std::move(initializer),
				std::vector<std::pair<std::string, std::unique_ptr<ExprNode>>>{},
				nullptr,
				std::move(annotations)
		);
	}

	std::unique_ptr<MemberDeclNode> Parser::parseConstructor(const MemberDeclNode::Access &access, bool isConst, std::string &className, std::vector<std::unique_ptr<AnnotationNode>> &annotations) {
		SourceLocation loc = advance().loc;
		std::vector<std::pair<std::string, std::unique_ptr<ExprNode>>> initializers;
		auto params = parseParameters();
		if (match(TokenType::COLON)) {
			advance(); // Consume the ':'

			do {
				std::string memberName = consume(TokenType::IDENTIFIER, "Expected member name in initializer list").lexeme;
				consume(TokenType::LPAREN, "Expected '(' after member name");
				auto expr = parseExpression();
				consume(TokenType::RPAREN, "Expected ')' after initializer expression");

				initializers.emplace_back(memberName, std::move(expr));

			} while (match(TokenType::COMMA) && (advance(), true));
		}
		auto body = parseBlock();

		return std::make_unique<MemberDeclNode>(
				loc,
				MemberDeclNode::METHOD_CONSTRUCTOR,
				access,
				isConst,
				std::move(className),  // Move the string
				nullptr,
				nullptr,  // No field init
				std::move(initializers),  // Ctor inits
				std::move(body),
				std::move(annotations)
		);
	}

	bool Parser::peekIsBlockEnd() const {
		return match(TokenType::RBRACE) || isAtEnd();
	}

	std::pair<std::vector<std::pair<std::string, std::unique_ptr<TypeNode>>>, bool>  Parser::parseParameters() {
		std::vector<std::pair<std::string, std::unique_ptr<TypeNode>>> params;

		consume(TokenType::LPAREN, "Expected '(' after function declaration");

		bool inStructSyntax = match(TokenType::LBRACE);

		if (inStructSyntax) {
			consume(TokenType::LBRACE);
		}
		std::unique_ptr<TypeNode> currentType = nullptr;
		bool firstParam = true;
		while (!(inStructSyntax ? match(TokenType::RBRACE) : match(TokenType::RPAREN))) {
			if (!firstParam) consume(TokenType::COMMA);
			firstParam = false;

			std::unique_ptr<TypeNode> paramType;
			if(isBuiltInType(currentToken.type) || (currentToken.type == TokenType::IDENTIFIER)){
				paramType = parseType();
			}else if(match({TokenType::LET,TokenType::VAR,TokenType::DYNAMIC})){
				advance();
				paramType = std::make_unique<TypeNode>(currentToken.loc, TypeNode::DYNAMIC);
			}/*else if (lastExplicitType) {
            paramType = lastExplicitType->clone();
			}*/else{
				paramType = std::make_unique<TypeNode>(currentToken.loc, TypeNode::DYNAMIC);
			}
			std::string name = consume(TokenType::IDENTIFIER, "Expected parameter name").lexeme;
			params.emplace_back(name, std::move(paramType));

		}
		if (inStructSyntax) {
			consume(TokenType::RBRACE, "Expected '}' to close parameter struct");
			// Now consume the closing paren that wraps the struct
			consume(TokenType::RPAREN, "Expected ')' after parameter struct");
		}
		else {
			consume(TokenType::RPAREN, "Expected ')' to close parameter list");
		}

		return {std::move(params), inStructSyntax};
	}

	bool Parser::isPotentialMethod() const {
		// Look ahead to see if this is a method declaration
		size_t lookahead = current;
		while (lookahead < tokens.size()) {
			const Token& tok = tokens[lookahead];

			// Skip over type parameters or array dimensions
			if (tok.type == TokenType::LBRACKET) {
				lookahead++;
				continue;
			}

			// If we find '(' before ';' or '=', it's a method
			if (tok.type == TokenType::LPAREN) {
				return true;
			}

			// If we find these tokens before '(', it's not a method
			if (tok.type == TokenType::SEMICOLON ||
			    tok.type == TokenType::EQUAL ||
			    tok.type == TokenType::LBRACE) {
				return false;
			}

			lookahead++;
		}
		return false;
	}

	bool Parser::isInStructInitializerContext() const {
		// Look back to see if we're after an equals sign following a type name
		if (previous > 0 && tokens[previous].type == TokenType::EQUAL) {
			// Check if before the equals sign we have a type name
			if (previous > 1 &&
			    (isBuiltInType(tokens[previous-2].type) || tokens[previous-2].type == TokenType::IDENTIFIER) && tokens[previous-2].type != TokenType::FREEOBJ) {
				return true;
			}
		}
		return false;
	}

	std::unique_ptr<StructInitializerNode> Parser::parseStructInitializer() {
		SourceLocation loc = consume(TokenType::LBRACE).loc;
		std::vector<StructInitializerNode::StructFieldInitializer> fields;

		if (!match(TokenType::RBRACE)) {
			do {
				// Check for designated initializer (C-style .field = value)
				if (match(TokenType::DOT)) {
					advance(); // Consume '.'
					std::string name = consume(TokenType::IDENTIFIER).lexeme;
					consume(TokenType::EQUAL);
					auto value = parseExpression();
					fields.push_back({name, std::move(value)});
				}
					// Check for named field (field: value)
				else if (peek(1).type == TokenType::COLON) {
					std::string name = consume(TokenType::IDENTIFIER).lexeme;
					consume(TokenType::COLON);
					auto value = parseExpression();
					fields.push_back({name, std::move(value)});
				}
					// Positional initialization (just value)
				else {
					auto value = parseExpression();
					fields.push_back({"", std::move(value)}); // Empty name indicates positional
				}
			} while (match(TokenType::COMMA) && (advance(), true));
		}

		consume(TokenType::RBRACE);
		return std::make_unique<StructInitializerNode>(loc, std::move(fields));
	}

	std::vector<std::string> Parser::parseArrowFunctionParams() {
		std::vector<std::string> params;
		consume(TokenType::LPAREN, "Expected '(' before arrow function parameters");

		if (!match(TokenType::RPAREN)) {
			do {
				params.push_back(consume(TokenType::IDENTIFIER, "Expected parameter name").lexeme);
			} while (match(TokenType::COMMA) && (advance(), true));
		}

		consume(TokenType::RPAREN, "Expected ')' after arrow function parameters");
		return params;
	}

	std::unique_ptr<LambdaExprNode> Parser::parseArrowFunction(std::vector<std::string>&& params) {
		SourceLocation loc = consume(TokenType::LAMBARROW).loc;

		// Convert string params to typed params (with null types for lambdas)
		std::vector<std::pair<std::string, std::unique_ptr<TypeNode>>> typedParams;
		typedParams.reserve(params.size());
		for (auto& param : params) {
			typedParams.emplace_back(std::move(param), nullptr);
		}
		std::unique_ptr<LambdaNode> lambda;
		// Handle single-expression body
		if (!match(TokenType::LBRACE)) {
			auto expr = parseExpression();
			auto stmts = std::vector<std::unique_ptr<ASTNode>>();
			stmts.push_back(std::make_unique<ReturnStmtNode>(loc, std::move(expr)));
			auto body = std::make_unique<BlockNode>(loc, std::move(stmts));

			lambda = std::make_unique<LambdaNode>(loc,std::move(typedParams),nullptr, std::move(body),false);
			return std::make_unique<LambdaExprNode>(loc,std::move(lambda));
		}

		// Handle block body
		auto body = parseBlock();
		lambda = std::make_unique<LambdaNode>(loc,std::move(typedParams),nullptr,std::move(body),false);

		return std::make_unique<LambdaExprNode>(loc,std::move(lambda));
	}

	std::unique_ptr<UnionDeclNode> Parser::parseUnion() {
		SourceLocation loc = consume(TokenType::UNION).loc;
		std::string name = consume(TokenType::IDENTIFIER, "Expected union name").lexeme;
		consume(TokenType::LBRACE, "Expected '{' after union declaration");

		std::vector<std::unique_ptr<TypeNode>> types;

		// Parse at least one type
		do {
			auto type = parseType();

			// Check for dynamic types (unions shouldn't allow them)
			if (type->isDynamic()) {
				errorReporter.report(type->loc,
				                     "Unions cannot contain dynamic types");
			}

			types.push_back(std::move(type));

			// Exit if no more commas
			if (!match(TokenType::COMMA)) break;
			advance();

		} while (!match(TokenType::RBRACE) && !isAtEnd());

		consume(TokenType::RBRACE, "Expected '}' after union body");
		return std::make_unique<UnionDeclNode>(loc, std::move(name), std::move(types));
	}

	std::unique_ptr<ActorDeclNode> Parser::parseActorDecl() {
		SourceLocation loc = consume(TokenType::ACTOR).loc;
		std::string name = consume(TokenType::IDENTIFIER, "Expected actor name").lexeme;

		// Optional inheritance
		std::string baseActor;
		if (match(TokenType::COLON)) {
			advance(); // Consume ':'
			baseActor = consume(TokenType::IDENTIFIER, "Expected base actor name").lexeme;
		}

		consume(TokenType::LBRACE, "Expected '{' after actor declaration");

		std::vector<std::unique_ptr<MemberDeclNode>> members;
		while (!match(TokenType::RBRACE) && !isAtEnd()) {
			try {
				// Parse annotations
				std::vector<std::unique_ptr<AnnotationNode>> annotations;
				while (match(TokenType::AT)) {
					annotations.push_back(parseAnnotation());
				}

				// Parse message handlers (start with "on") or regular members
				if (match(TokenType::ON)) {
					members.push_back(parseMessageHandler(std::move(annotations)));
				} else {
					// Parse regular members (fields, etc.)
					members.push_back(parseObjectPrimary(name, annotations));
				}
			} catch (const ParseError& e) {
				errorReporter.report(e.location, e.format());
				errStream << e.what() << std::endl;
				synchronize();
			}
		}

		consume(TokenType::RBRACE, "Expected '}' after actor body");

		return std::make_unique<ActorDeclNode>(
				loc,
				std::move(name),
				std::move(members),
				std::move(baseActor)
		);
	}

	//std::vector<std::unique_ptr<MemberDeclNode>> Parser::parseActorMembers(std::string& actorName) {
	//	std::vector<std::unique_ptr<MemberDeclNode>> members;
	//	while (!match(TokenType::RBRACE) && !isAtEnd()) {
	//		try {
	//			// Parse annotations
	//			std::vector<std::unique_ptr<AnnotationNode>> annotations;
	//			while (match(TokenType::AT)) {
	//				annotations.push_back(parseAnnotation());
	//			}
	//			// Parse access modifier
	//			members.push_back(parseObjectPrimary(actorName, annotations));
//
	//		} catch (const ParseError& e) {
	//			errorReporter.report(e.location, e.format());
	//			errStream << e.what() << std::endl;
	//			synchronize();
	//		}
	//	}
	//	return members;
	//}
//
	std::unique_ptr<MemberDeclNode> Parser::parseMessageHandler(
			std::vector<std::unique_ptr<AnnotationNode>> annotations
	) {
		SourceLocation loc = consume(TokenType::ON).loc;
		std::string messageType = consume(TokenType::IDENTIFIER, "Expected message type").lexeme;

		// Parse parameters
		auto [params, _] = parseParameters();

		// Parse optional return type
		std::unique_ptr<TypeNode> returnType;
		if (match(TokenType::ARROW)) {
			advance();
			returnType = parseType();
		}

		auto body = parseBlock();

		return std::make_unique<MemberDeclNode>(
				loc,
				MemberDeclNode::MESSAGE_HANDLER,
				MemberDeclNode::PUBLIC,
				false,
				std::move(messageType),
				std::move(returnType),
				nullptr,
				std::vector<std::pair<std::string, std::unique_ptr<ExprNode>>>{},
				std::move(body),
				std::move(annotations)
		);
	}

	// Helper function to create error nodes that can be used as members
	std::unique_ptr<MemberDeclNode> Parser::createErrorNodeAsMember() {
		return std::make_unique<MemberDeclNode>(
				currentToken.loc,
				MemberDeclNode::FIELD,  // Using FIELD as generic error type
				MemberDeclNode::PRIVATE,
				false,
				"",  // Empty name
				nullptr,  // No type
				nullptr,  // No initializer
				std::vector<std::pair<std::string, std::unique_ptr<ExprNode>>>{},  // No ctor initializers
				nullptr,  // No body
				std::vector<std::unique_ptr<AnnotationNode>>{}  // No annotations
		);
	}

	bool Parser::peekIsTemplateStart() const {
		// Check if next token is '<' and it's not part of an operator
		if (currentToken.type == TokenType::LESS) {
			// Make sure it's not part of a comparison operator
			size_t next = current + 1;
			return next < tokens.size() && tokens[next].type != TokenType::LESS;
		}
		return false;
	}

	std::unique_ptr<ScopeBlockNode> Parser::parseScopeBlock() {
		SourceLocation loc = consume(TokenType::SCOPE).loc;
		consume(TokenType::LBRACE, "Expected '{' after 'scope'");

		std::vector<std::unique_ptr<ASTNode>> statements;

		while (!match(TokenType::RBRACE) && !isAtEnd()) {
			statements.push_back(parseStatement());
		}

		consume(TokenType::RBRACE, "Expected '}' after scope block");

		return std::make_unique<ScopeBlockNode>(loc, std::move(statements));
	}

	std::vector<std::unique_ptr<AnnotationNode>> Parser::parseAnnotations() {
		std::vector<std::unique_ptr<AnnotationNode>> annotations;
		while (match({TokenType::AT/*,TokenType::DOUBLE_AT*/})) {
			annotations.push_back(parseAnnotation());
		}
		return annotations;
	}


}