#pragma clang diagnostic push
#pragma ide diagnostic ignored "misc-no-recursion"
#include "parser.hpp"
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include "../exceptions/ParseError.hpp"

std::string debug_lexeme_string(const std::vector<Token>& tokens) {
	std::stringstream ss;
	for (const auto& item : tokens) ss << item.lexeme << " ";
	return ss.str();
}

namespace zenith {
	// Fixed parseVarDecl to return the node
	std_P3019_modified::polymorphic<VarDeclNode> Parser::parseVarDecl() {
		SourceLocation loc = currentToken.loc;
		bool isHoisted = match(TokenType::HOIST);
		VarDeclNode::Kind kind = VarDeclNode::DYNAMIC; // Default to dynamic
		std_P3019_modified::polymorphic<TypeNode> typeNode;

		// Case 1: Static typed declaration (int a = 12)
		if (isBuiltInType(currentToken.type) || currentToken.type == TokenType::IDENTIFIER) {
			kind = VarDeclNode::STATIC;
			typeNode = parseType();

		}
			// Case 2: Dynamic declaration (let/var/dynamic)
		else if (match({TokenType::LET, TokenType::VAR, TokenType::DYNAMIC})) {
			kind = VarDeclNode::DYNAMIC;
			advance();
		}

		std::string name = consume(TokenType::IDENTIFIER, "Expected name").lexeme;

		// Handle array size specification (e.g., int arr[10])
		if (match(TokenType::LBRACKET)) {
			advance(); // Consume '['

			// Parse the array size expression
			auto sizeExpr = parseExpression();

			// Create a new ArrayTypeNode with the size expression
			auto arrayType = std_P3019_modified::make_polymorphic<ArrayTypeNode>(
					loc,
					std::move(typeNode),
					std::move(sizeExpr)
			);
			typeNode = std::move(arrayType);

			consume(TokenType::RBRACKET, "Expected ']' after array size");
		}

		// Handle type annotation for dynamic variables (let a: int = 10)
		if (kind == VarDeclNode::DYNAMIC && match(TokenType::COLON)) {
			typeNode = parseType();
		}

		// Handle initialization
		std_P3019_modified::polymorphic<ExprNode> initializer = nullptr;
		if (match(TokenType::EQUAL)) {
			consume(TokenType::EQUAL);
			initializer = parseExpression();

			// Special case: Class initialization (SomeClassName a = new SomeClassName())
			if (kind == VarDeclNode::STATIC && initializer->isConstructorCall()) {
				kind = VarDeclNode::CLASS_INIT;
			}
		}

		return std_P3019_modified::make_polymorphic<VarDeclNode>(
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

	std_P3019_modified::polymorphic<ExprNode> Parser::parseExpression(int precedence) {

		if (match({TokenType::INCREASE, TokenType::DECREASE})) {
			Token op = advance();
			auto right = parseExpression(getPrecedence(op.type));
			return std_P3019_modified::make_polymorphic<UnaryOpNode>(
					op.loc,
					op.type,
					std::move(right),
					true
			);
		}

		auto expr = parsePrimary();

		while (true) {
			if (match({TokenType::INCREASE, TokenType::DECREASE})) {
				Token op = advance();
				return std_P3019_modified::make_polymorphic<UnaryOpNode>(
						op.loc,
						op.type,
						std::move(expr),
						false
				);
			}

			Token op = currentToken;
			int opPrecedence = getPrecedence(op.type);

			if (opPrecedence <= precedence) break;

			advance();
			auto right = parseExpression(opPrecedence);
			expr = std_P3019_modified::make_polymorphic<BinaryOpNode>(
					op.loc, // Pass operator location
					op.type,
					std::move(expr),
					std::move(right)
			);
		}

		return std::move(expr);
	}

	std_P3019_modified::polymorphic<ExprNode> Parser::parsePrimary() {
		SourceLocation startLoc = currentToken.loc;

		if (match(TokenType::NEW)) {
			return std::move(parseNewExpression());
		}
		else if (match({TokenType::NUMBER,TokenType::INTEGER,TokenType::FLOAT_LIT})) {
			Token numToken = advance();
			return std_P3019_modified::make_polymorphic<LiteralNode>(startLoc, LiteralNode::NUMBER, numToken.lexeme);
		}
		else if (match(TokenType::STRING_LIT)) {
			Token strToken = advance();
			return std_P3019_modified::make_polymorphic<LiteralNode>(startLoc, LiteralNode::STRING, strToken.lexeme);
		} else if (match(TokenType::TRUE) || match(TokenType::FALSE)) {
			Token boolToken = advance();
			return std_P3019_modified::make_polymorphic<LiteralNode>(startLoc, LiteralNode::BOOL, boolToken.lexeme);
		}
		else if (match(TokenType::NULL_LIT)) {
			advance();
			return std_P3019_modified::make_polymorphic<LiteralNode>(startLoc, LiteralNode::NIL, "nil");
		}
		else if (match(TokenType::LBRACE)) {
			if (isInStructInitializerContext()) {
				return std::move(parseStructInitializer());
			}
			return parseFreeObject();
		} else if (match(TokenType::FREEOBJ)) {
			advance();
			if (match(TokenType::LBRACE)) {
				return parseFreeObject();
			}
			throw ParseError(startLoc,"Expected '{' after free obj, Achievement unlocked: How did we get here?");
		}
		else if (match(TokenType::LPAREN)) {
				auto params = parseArrowFunctionParams();
				if (match(TokenType::LAMBARROW)) {
					return std::move(parseArrowFunction(std::move(params)));
				}
				// else treat as normal parenthesized expression
			}
		else if (match({TokenType::IDENTIFIER, TokenType::THIS})) {
			Token identToken = advance();
			std_P3019_modified::polymorphic<ExprNode> expr;

			if (identToken.type == TokenType::THIS) {
				expr = std_P3019_modified::make_polymorphic<ThisNode>(startLoc);
			} else {
				expr = std_P3019_modified::make_polymorphic<VarNode>(startLoc, identToken.lexeme);
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
	std_P3019_modified::polymorphic<TypeNode> Parser::parseType() {
		SourceLocation startLoc = currentToken.loc;

		// Handle built-in types (from TokenType)
		if (match({TokenType::INT, TokenType::LONG, TokenType::SHORT,
				   TokenType::BYTE, TokenType::FLOAT, TokenType::DOUBLE,
				   TokenType::STRING, TokenType::NUMBER, TokenType::BIGINT,
				   TokenType::BIGNUMBER, TokenType::FREEOBJ, TokenType::BOOL, TokenType::VOID})) {

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
			else if (typeToken.type == TokenType::VOID) kind = PrimitiveTypeNode::VOID;
			else if (typeToken.type == TokenType::FREEOBJ) {
				// For freeobj, we'll return a TypeNode with DYNAMIC kind since it's a special case
				return std_P3019_modified::make_polymorphic<TypeNode>(startLoc, TypeNode::DYNAMIC);
			}
			else kind = PrimitiveTypeNode::BIGNUMBER;

			return std_P3019_modified::make_polymorphic<PrimitiveTypeNode>(startLoc, kind);
		}
		else if (match(TokenType::LBRACKET)) {
			// Array type (e.g., [int] or [MyClass])
			advance(); // Consume '['
			auto elementType = parseType();
			consume(TokenType::RBRACKET, "Expected ']' after array type");
			return std_P3019_modified::make_polymorphic<ArrayTypeNode>(startLoc, std::move(elementType));
		}
		else if (match(TokenType::IDENTIFIER)) {
			// User-defined type (class/struct/type alias) - now with template support
			Token typeToken = advance();
			std::string baseName = typeToken.lexeme;

			//Todo change this
			if(baseName == "Function")
				return std_P3019_modified::make_polymorphic<TypeNode>(
						startLoc,
						TypeNode::FUNCTION
						);
			//End todo

			// Check for template arguments
			if (peekIsTemplateStart()) {
				consume(TokenType::LESS); // Eat '<'

				std::vector<std_P3019_modified::polymorphic<TypeNode>> templateArgs;
				if (!match(TokenType::GREATER)) {
					do {
						templateArgs.push_back(parseType());
					} while (match(TokenType::COMMA) && (advance(), true));
				}

				consume(TokenType::GREATER, "Expected '>' to close template arguments");

				return std_P3019_modified::make_polymorphic<TemplateTypeNode>(
						startLoc,
						baseName,
						std::move(templateArgs)
				);
			}

			// Non-templated case
			return std_P3019_modified::make_polymorphic<NamedTypeNode>(startLoc, baseName);
		}

		throw ParseError(
				currentToken.loc,
				"Expected type name, got " + Lexer::tokenToString(currentToken.type)
		);
	}

	void Parser::synchronize() {
		// 1. Always consume the current problematic token first
		advance();

		// 2. Loop until we find a suitable synchronization point or reach the end.
		while (!isAtEnd()) {
			// --- Strategy 1: Check if the PREVIOUS token ended a structure ---
			// If the token we just consumed was a statement terminator or block end,
			// the current token is likely the start of something new and safe.
			TokenType prevType = previousToken().type; // Get the type of the token consumed *before* current
			if (prevType == TokenType::SEMICOLON || prevType == TokenType::RBRACE) {
				// We've likely just finished a statement or block.
				// The current token should be the start of the next one.
#ifdef PARSER_DEBUG // Optional debug output
				std::cout << "[Sync] Resuming after previous token: " << Lexer::tokenToString(prevType) << std::endl;
#endif
				return;
			}

			// --- Strategy 2: Check if the CURRENT token starts a new major structure ---
			// Look for keywords that reliably begin common declarations or statements.
			switch (currentToken.type) {
				// Major Declaration Starters (High Confidence Recovery Points)
				case TokenType::CLASS:
				case TokenType::STRUCT:
				case TokenType::UNION:
				case TokenType::FUN:        // Includes methods and functions
				case TokenType::ACTOR:
				case TokenType::TEMPLATE:
				case TokenType::IMPORT:     // Import statements
				case TokenType::PACKAGE:    // Package declaration
				case TokenType::EXTERN:     // FFI blocks (assuming 'extern' starts it)
					// Statement Starters (Good Confidence Recovery Points)
				case TokenType::IF:
				case TokenType::WHILE:
				case TokenType::FOR:
				case TokenType::DO:
				case TokenType::RETURN:
				//case TokenType::SWITCH:     /
				//case TokenType::MATCH:      /
				//case TokenType::TRY:        /
				case TokenType::UNSAFE:
				case TokenType::SCOPE:
					// Variable Declaration Starters
				case TokenType::LET:
				case TokenType::VAR:
				case TokenType::DYNAMIC:
					// Potential Type Starters (Lower confidence, might be part of expression)
					// Add these cautiously if needed, or rely on keywords above.
					// case TokenType::INT:
					// case TokenType::FLOAT:
					// case TokenType::IDENTIFIER: // Risky - could be function call, variable use, etc.
					// Annotation marker (often precedes a declaration)
				case TokenType::AT:
					// Access Modifiers (likely start a class/struct member)
				case TokenType::PUBLIC:
				case TokenType::PRIVATE:
				case TokenType::PROTECTED:
				case TokenType::PRIVATEW:
				case TokenType::PROTECTEDW:
					// Found a likely synchronization point.
#ifdef PARSER_DEBUG
					std::cout << "[Sync] Resuming before current token: " << Lexer::tokenToString(currentToken.type) << std::endl;
#endif
					return; // Exit synchronize, ready to parse the new structure

				default:
					// This token doesn't look like a good starting point.
					break;
			}

			// 3. If not a synchronization point, consume the current token and continue.
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
				{TokenType::PERCENT, 7},

				// Unary operator
				{TokenType::INCREASE, 9},
				{TokenType::DECREASE, 9},

		};

		auto it = precedences.find(type);
		return it != precedences.end() ? it->second : 0;
	}

	std_P3019_modified::polymorphic<ProgramNode> Parser::parse() {
		SourceLocation startLoc = currentToken.loc;
		std::vector<std_P3019_modified::polymorphic<ASTNode>> declarations;
		while (!isAtEnd()) {
			pendingAnnotations.clear();
			pendingAnnotations = parseAnnotations();
			try {
				auto annotations = parseAnnotations();
				if (match(TokenType::IMPORT)) {
					declarations.emplace_back(std::move(parseImport()));
				}
				if (match(TokenType::TEMPLATE)) {
					declarations.emplace_back(std::move(parseTemplate()));
				}
				else if (match({TokenType::CLASS,TokenType::STRUCT})) {
					declarations.emplace_back(std::move(parseObject()));
				}
				else if (match(TokenType::UNION)) {
					declarations.emplace_back(std::move(parseUnion()));
				}
				else if (match(TokenType::FUN)) {
					declarations.emplace_back(std::move(parseFunction()));
				}
				else if (isBuiltInType(currentToken.type) || currentToken.type == TokenType::IDENTIFIER) {
					// Handle both built-in types and user-defined types
					if (isPotentialMethod()) {
						declarations.emplace_back(std::move(parseFunction()));
					} else {
						// It's a variable declaration
						declarations.emplace_back(std::move(parseVarDecl()));
					}
				}
				else if (match({TokenType::LET, TokenType::VAR, TokenType::DYNAMIC})) {
					declarations.emplace_back(std::move(parseVarDecl()));
				}
				else if (match(TokenType::ACTOR)) {
					declarations.emplace_back(std::move(parseActorDecl()));
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
					if (auto annotatable_opt = declarations.back().try_as_interface<IAnnotatable>()) {
						(*annotatable_opt)->setAnnotations(std::move(pendingAnnotations));
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
		auto program = std_P3019_modified::make_polymorphic<ProgramNode>(startLoc,std::move(declarations));

		return program;
	}

	bool Parser::isBuiltInType(TokenType type) {
		static const std::unordered_set<TokenType> builtInTypes = {
				TokenType::INT, TokenType::LONG, TokenType::SHORT, TokenType::BYTE, TokenType::FLOAT, TokenType::DOUBLE, TokenType::STRING, TokenType::FREEOBJ, TokenType::BOOL, TokenType::VOID
		};
		return builtInTypes.count(type) > 0;
	}

	std_P3019_modified::polymorphic<NewExprNode> Parser::parseNewExpression() {
		SourceLocation location = consume(TokenType::NEW).loc; // Eat 'new' keyword, nom nom nom
		std::string className = consume(TokenType::IDENTIFIER).lexeme;

		consume(TokenType::LPAREN);
		std::vector<std_P3019_modified::polymorphic<ExprNode>> args;
		if (!match(TokenType::RPAREN)) {
			do {
				args.push_back(parseExpression());
			} while (match(TokenType::COMMA));
			consume(TokenType::RPAREN);
		}

		return std_P3019_modified::make_polymorphic<NewExprNode>(location,className, std::move(args));
	}

	std_P3019_modified::polymorphic<FunctionDeclNode> Parser::parseFunction() {
		SourceLocation loc = currentToken.loc;
		// Handle annotations
		bool isAsync = std::find_if(pendingAnnotations.begin(), pendingAnnotations.end(), [](const std_P3019_modified::polymorphic<AnnotationNode>& ann) {
			return ann->name == "Async";
		}) != pendingAnnotations.end();


		// Parse return type (optional)
		std_P3019_modified::polymorphic<TypeNode> returnType;
		if (match(TokenType::FUN)) {
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

		return std_P3019_modified::make_polymorphic<FunctionDeclNode>(
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

	std_P3019_modified::polymorphic<BlockNode> Parser::parseBlock() {
		SourceLocation startLoc = currentToken.loc;
		consume(TokenType::LBRACE);

		std::vector<std_P3019_modified::polymorphic<ASTNode>> statements;
		try {
			while (!match(TokenType::RBRACE) && !isAtEnd()) {
				statements.emplace_back(std::move(parseStatement()));
			}
			consume(TokenType::RBRACE,"Expected '}' after block");
		} catch (const ParseError&) {
			synchronize(); // Your error recovery method
			if (!match(TokenType::RBRACE)) {
				// Insert synthetic '}' if missing
				statements.emplace_back(std::move(createErrorNode()));
			}
		}

		return std_P3019_modified::make_polymorphic<BlockNode>(startLoc, std::move(statements));
	}

	std_P3019_modified::polymorphic<StmtNode> Parser::parseStatement() {
		SourceLocation loc = currentToken.loc;

		//Maybe ex
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
			return std_P3019_modified::make_polymorphic<ExprStmtNode>(loc, std::move(expr));
		}

		// Control flow statements
		if (match(TokenType::IF)) return std::move(parseIfStmt());
		if (match(TokenType::FOR)) return std::move(parseForStmt());
		if (match(TokenType::WHILE)) return std::move(parseWhileStmt());
		if (match(TokenType::DO)) return std::move(parseDoWhileStmt());
		if (match(TokenType::RETURN)) return std::move(parseReturnStmt());

		// Unsafe blocks
		if (match(TokenType::UNSAFE)) {
			advance(); // Consume 'unsafe'
			return parseUnsafeBlock();
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
			return std_P3019_modified::make_polymorphic<ExprStmtNode>(loc, std::move(expr));
		}

		// Empty statement (just a semicolon)
		if (match(TokenType::SEMICOLON)) {
			advance();
			return std_P3019_modified::make_polymorphic<EmptyStmtNode>(loc);
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
			case TokenType::LPAREN:          // Maybe also arrow lambda
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

	std_P3019_modified::polymorphic<IfNode> Parser::parseIfStmt() {
		SourceLocation loc = consume(TokenType::IF).loc;
		consume(TokenType::LPAREN, "Expected '(' after 'if'");
		auto condition = parseExpression();
		consume(TokenType::RPAREN, "Expected ')' after if condition");

		// Parse 'then' branch (enforce braces if required)
		std_P3019_modified::polymorphic<StmtNode> thenBranch;
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
			thenBranch = std_P3019_modified::make_polymorphic<EmptyStmtNode>(errorNode->loc);
		}

		// Parse 'else' branch (also enforce braces if required)
		std_P3019_modified::polymorphic<ASTNode> elseBranch;
		if (match(TokenType::ELSE)) {
			advance();
			try {
				if (flags.bracesRequired) {
					if (!match(TokenType::LBRACE)) {
						throw ParseError(currentToken.loc,
						                 "Expected '{' after 'else' (braces are required)");
					}
					elseBranch = parseBlock();
				} else {
					elseBranch = parseStatement();
				}
			} catch (const ParseError& e) {
				errorReporter.report(e.location,"Error in else body " + e.format());
				errStream << "Error in else body: " << e.what() << std::endl;
				synchronize();
				// Create error node or empty node as fallback
				elseBranch = std_P3019_modified::make_polymorphic<EmptyStmtNode>(currentToken.loc); // Simplified
			}

		}

		return std_P3019_modified::make_polymorphic<IfNode>(
				loc,
				std::move(condition),
				std::move(thenBranch),
				std::move(elseBranch)
		);
	}

	std_P3019_modified::polymorphic<ForNode> Parser::parseForStmt() {
		SourceLocation loc = consume(TokenType::FOR).loc;
		consume(TokenType::LPAREN);

		// Parse init/condition/increment (unchanged)
		std_P3019_modified::polymorphic<ASTNode> init;
		if (match(TokenType::SEMICOLON)) {
			advance(); // Empty initializer
		}
		else if (match({TokenType::LET, TokenType::VAR, TokenType::DYNAMIC}) ||
		         isBuiltInType(currentToken.type)) {
			init = parseVarDecl();
		}
		else {
			init = std_P3019_modified::make_polymorphic<ExprStmtNode>(currentToken.loc, parseExpression());
		}
		consume(TokenType::SEMICOLON);

		std_P3019_modified::polymorphic<ExprNode> condition;
		if (!match(TokenType::SEMICOLON)) {
			condition = parseExpression();
		}
		consume(TokenType::SEMICOLON);

		std_P3019_modified::polymorphic<ExprNode> increment;
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
		return std_P3019_modified::make_polymorphic<ForNode>(loc, std::move(init), std::move(condition),
		                                 std::move(increment), std::move(body));
	}

	std_P3019_modified::polymorphic<WhileNode> Parser::parseWhileStmt() {
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
		return std_P3019_modified::make_polymorphic<WhileNode>(loc, std::move(condition), std::move(body));
	}

	std_P3019_modified::polymorphic<DoWhileNode> Parser::parseDoWhileStmt() {
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

		return std_P3019_modified::make_polymorphic<DoWhileNode>(loc, std::move(condition), std::move(body));
	}

	std_P3019_modified::polymorphic<ReturnStmtNode> Parser::parseReturnStmt() {
		SourceLocation loc = consume(TokenType::RETURN).loc;

		// Handle empty return (no expression)
		if (match(TokenType::SEMICOLON)) {
			advance();  // Consume the semicolon
			return std_P3019_modified::make_polymorphic<ReturnStmtNode>(loc, nullptr);
		}

		// Parse return value (optional in Zenith)
		std_P3019_modified::polymorphic<ExprNode> value;
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

		return std_P3019_modified::make_polymorphic<ReturnStmtNode>(loc, std::move(value));
	}

	bool Parser::peekIsStatementTerminator() const {
		return match({TokenType::SEMICOLON, TokenType::RBRACE, TokenType::EOF_TOKEN});
	}

	std_P3019_modified::polymorphic<FreeObjectNode> Parser::parseFreeObject() {
		SourceLocation loc = consume(TokenType::LBRACE).loc;
		std::vector<std::pair<std::string, std_P3019_modified::polymorphic<ExprNode>>> properties;

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
		return std_P3019_modified::make_polymorphic<FreeObjectNode>(loc, std::move(properties));
	}

	std_P3019_modified::polymorphic<CallNode> Parser::parseFunctionCall(std_P3019_modified::polymorphic<ExprNode> callee) {
		SourceLocation loc = callee->loc;
		consume(TokenType::LPAREN);

		std::vector<std_P3019_modified::polymorphic<ExprNode>> args;
		if (!match(TokenType::RPAREN)) {
			do {
				args.push_back(parseExpression());
			} while (match(TokenType::COMMA));
		}

		consume(TokenType::RPAREN);
		return std_P3019_modified::make_polymorphic<CallNode>(loc, std::move(callee), std::move(args));
	}

	std_P3019_modified::polymorphic<ExprNode> Parser::parseMemberAccess(std_P3019_modified::polymorphic<ExprNode> object) {
		// Horse guarantee: We only get called when we see a DOT, if not I'm a horse or tramvai
		SourceLocation loc = object->loc;

		// First access (guaranteed to exist)
		advance(); // Consume '.'
		std::string member = consume(TokenType::IDENTIFIER).lexeme;
		std_P3019_modified::polymorphic<ExprNode> result = std_P3019_modified::make_polymorphic<MemberAccessNode>(loc, std::move(object), member);

		// Handle additional accesses or calls
		while (match(TokenType::DOT)) {
			advance();
			member = consume(TokenType::IDENTIFIER).lexeme;
			result = std_P3019_modified::make_polymorphic<MemberAccessNode>(loc, std::move(result), member);

			if (match(TokenType::LPAREN)) {
				if (!result.is_type<MemberDeclNode>()) {
					throw ParseError(loc, "Only member accesses can be called");
				}
				result = parseFunctionCall(std::move(result));
			}
		}

		return result;
	}

	std_P3019_modified::polymorphic<ExprNode> Parser::parseArrayAccess(std_P3019_modified::polymorphic<ExprNode> arrayExpr) {
		SourceLocation loc = currentToken.loc;

		// Keep processing chained array accesses (e.g., arr[1][2][3])
		while (match(TokenType::LBRACKET)) {
			advance(); // Consume '['

			auto indexExpr = parseExpression();
			consume(TokenType::RBRACKET, "Expected ']' after array index");

			// Create new array access node
			arrayExpr = std_P3019_modified::make_polymorphic<ArrayAccessNode>(
					loc,
					std::move(arrayExpr),  // The array being accessed
					std::move(indexExpr)   // The index expression
			);
		}

		return arrayExpr;
	}

	std_P3019_modified::polymorphic<AnnotationNode> Parser::parseAnnotation() {
		SourceLocation loc = consume(TokenType::AT).loc; // Eat '@' symbol

		// Parse annotation name
		std::string name = consume(TokenType::IDENTIFIER).lexeme;

		// Parse optional annotation arguments
		std::vector<std::pair<std::string, std_P3019_modified::polymorphic<ExprNode>>> arguments;

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

		return std_P3019_modified::make_polymorphic<AnnotationNode>(loc, name, std::move(arguments));
	}

	std_P3019_modified::polymorphic<ErrorNode> Parser::createErrorNode() {
		return std_P3019_modified::make_polymorphic<ErrorNode>(currentToken.loc);
	}

	std_P3019_modified::polymorphic<ImportNode> Parser::parseImport() {
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

		return std_P3019_modified::make_polymorphic<ImportNode>(loc, importPath, isJavaImport);
	}

	std_P3019_modified::polymorphic<ObjectDeclNode> Parser::parseObject() {
		if(!match({TokenType::STRUCT,TokenType::CLASS})){
			errorReporter.report(currentToken.loc, "Uhhh, +1 the compiler fucked up point", {"Internal Error", RED_TEXT});
		}
		bool isClass = match(TokenType::CLASS);
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
		std::vector<std_P3019_modified::polymorphic<MemberDeclNode>> members;

		while (!match(TokenType::RBRACE) && !isAtEnd()) {
			try {
				// Parse annotations
				std::vector<std_P3019_modified::polymorphic<AnnotationNode>> annotations;
				while (match(TokenType::AT)) {
					annotations.push_back(parseAnnotation());
				}
				// Parse access modifier
				members.push_back(parseObjectPrimary(className, annotations, isClass ? MemberDeclNode::PRIVATE : MemberDeclNode::PUBLIC));

			} catch (const ParseError& e) {
				errorReporter.report(e.location, e.format());
				errStream << e.what() << std::endl;
				synchronize();
			}
		}

		consume(TokenType::RBRACE, "Expected '}' after object body");

		return std_P3019_modified::make_polymorphic<ObjectDeclNode>(
				classLoc,
				kind,
				std::move(className),
				std::move(baseClass),
				std::move(members)
		);
	}

	std_P3019_modified::polymorphic<MemberDeclNode> Parser::parseObjectPrimary(std::string &name, std::vector<std_P3019_modified::polymorphic<AnnotationNode>> &annotations, MemberDeclNode::Access defaultLevel) {
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
			 return std_P3019_modified::make_polymorphic<MemberDeclNode>(
					funcDecl->loc,
					MemberDeclNode::METHOD,
					access,
					isConst,
					std::move(funcDecl->name),    // std::string&&
					std::move(funcDecl->returnType), // unique_ptr<TypeNode>
					nullptr,                      // No field init
					std::vector<std::pair<std::string, std_P3019_modified::polymorphic<ExprNode>>>{},// Empty ctor inits
					std::move(funcDecl->body),    // unique_ptr<BlockNode>
					std::move(annotations)        // vector<unique_ptr<AnnotationNode>>
			);
		}
		else {
			// Field declaration
			return parseField(annotations, access, isConst);
		}
	}

	std_P3019_modified::polymorphic<MemberDeclNode> Parser::parseField(std::vector<std_P3019_modified::polymorphic<AnnotationNode>> &annotations, const MemberDeclNode::Access &access, bool isConst) {
		auto varDecl = parseVarDecl();

		consume(TokenType::SEMICOLON, "Expected ';' after field declaration");

		return std_P3019_modified::make_polymorphic<MemberDeclNode>(
				currentToken.loc,
				MemberDeclNode::FIELD,
				access,
				isConst,
				std::move(varDecl->name),
				std::move(varDecl->type),
				std::move(varDecl->initializer),
				std::vector<std::pair<std::string, std_P3019_modified::polymorphic<ExprNode>>>{},
				nullptr,
				std::move(annotations)
		);
	}

	std_P3019_modified::polymorphic<MemberDeclNode> Parser::parseConstructor(const MemberDeclNode::Access &access, bool isConst, std::string &className, std::vector<std_P3019_modified::polymorphic<AnnotationNode>> &annotations) {
		SourceLocation loc = advance().loc;
		std::vector<std::pair<std::string, std_P3019_modified::polymorphic<ExprNode>>> initializers;
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

		return std_P3019_modified::make_polymorphic<MemberDeclNode>(
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

	[[maybe_unused]] bool Parser::peekIsBlockEnd() const {
		return match(TokenType::RBRACE) || isAtEnd();
	}

	std::pair<std::vector<std::pair<std::string, std_P3019_modified::polymorphic<TypeNode>>>, bool>  Parser::parseParameters() {
		std::vector<std::pair<std::string, std_P3019_modified::polymorphic<TypeNode>>> params;

		consume(TokenType::LPAREN, "Expected '(' after function declaration");

		bool inStructSyntax = match(TokenType::LBRACE);

		if (inStructSyntax) {
			consume(TokenType::LBRACE);
		}
		std_P3019_modified::polymorphic<TypeNode> currentType = nullptr;
		bool firstParam = true;
		while (!(inStructSyntax ? match(TokenType::RBRACE) : match(TokenType::RPAREN))) {
			if (!firstParam) consume(TokenType::COMMA);
			firstParam = false;

			std_P3019_modified::polymorphic<TypeNode> paramType;
			if(isBuiltInType(currentToken.type) || (currentToken.type == TokenType::IDENTIFIER)){
				paramType = parseType();
			}else if(match({TokenType::LET,TokenType::VAR,TokenType::DYNAMIC})){
				advance();
				paramType = std_P3019_modified::make_polymorphic<TypeNode>(currentToken.loc, TypeNode::DYNAMIC);
			}/*else if (lastExplicitType) {
            paramType = lastExplicitType->clone()}
		    */else{
				paramType = std_P3019_modified::make_polymorphic<TypeNode>(currentToken.loc, TypeNode::DYNAMIC);
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

	std_P3019_modified::polymorphic<StructInitializerNode> Parser::parseStructInitializer() {
		SourceLocation loc = consume(TokenType::LBRACE).loc;
		std::vector<StructInitializerNode::StructFieldInitializer> fields;

		if (!match(TokenType::RBRACE)) {
			do {
				// (C-style .field = value)
				if (match(TokenType::DOT)) {
					advance(); // Consume '.'
					std::string name = consume(TokenType::IDENTIFIER).lexeme;
					consume(TokenType::EQUAL);
					auto value = parseExpression();
					fields.push_back({name, std::move(value)});
				}
				// JS-Style (field: value)
				else if (peek(1).type == TokenType::COLON) {
					std::string name = consume(TokenType::IDENTIFIER).lexeme;
					consume(TokenType::COLON);
					auto value = parseExpression();
					fields.push_back({name, std::move(value)});
				}
				// Positional initialization (just value) (value)
				else {
					auto value = parseExpression();
					fields.push_back({"", std::move(value)});
				}
			} while (match(TokenType::COMMA) && (advance(), true));
		}

		consume(TokenType::RBRACE);
		return std_P3019_modified::make_polymorphic<StructInitializerNode>(loc, std::move(fields));
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

	std_P3019_modified::polymorphic<LambdaExprNode> Parser::parseArrowFunction(std::vector<std::string>&& params) {
		SourceLocation loc = consume(TokenType::LAMBARROW).loc;

		// Convert string params to typed params (with null types for lambdas)
		std::vector<std::pair<std::string, std_P3019_modified::polymorphic<TypeNode>>> typedParams;
		typedParams.reserve(params.size());
		for (auto& param : params) {
			typedParams.emplace_back(std::move(param), nullptr);
		}
		std_P3019_modified::polymorphic<LambdaNode> lambda;
		// Handle single-expression body
		if (!match(TokenType::LBRACE)) {
			auto expr = parseExpression();
			auto stmts = std::vector<std_P3019_modified::polymorphic<ASTNode>>();
			stmts.emplace_back(std_P3019_modified::make_polymorphic<ReturnStmtNode>(loc, std::move(expr)));
			auto body = std_P3019_modified::make_polymorphic<BlockNode>(loc, std::move(stmts));

			lambda = std_P3019_modified::make_polymorphic<LambdaNode>(loc,std::move(typedParams),nullptr, std::move(body),false);
			return std_P3019_modified::make_polymorphic<LambdaExprNode>(loc,std::move(lambda));
		}

		// Handle block body
		auto body = parseBlock();
		lambda = std_P3019_modified::make_polymorphic<LambdaNode>(loc,std::move(typedParams),nullptr,std::move(body),false);

		return std_P3019_modified::make_polymorphic<LambdaExprNode>(loc,std::move(lambda));
	}

	std_P3019_modified::polymorphic<UnionDeclNode> Parser::parseUnion() {
		SourceLocation loc = consume(TokenType::UNION).loc;
		std::string name = consume(TokenType::IDENTIFIER, "Expected union name").lexeme;
		consume(TokenType::LBRACE, "Expected '{' after union declaration");

		std::vector<std_P3019_modified::polymorphic<TypeNode>> types;

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
		return std_P3019_modified::make_polymorphic<UnionDeclNode>(loc, std::move(name), std::move(types));
	}

	std_P3019_modified::polymorphic<ActorDeclNode> Parser::parseActorDecl() {
		SourceLocation loc = consume(TokenType::ACTOR).loc;
		std::string name = consume(TokenType::IDENTIFIER, "Expected actor name").lexeme;

		// Optional inheritance
		std::string baseActor;
		if (match(TokenType::COLON)) {
			advance(); // Consume ':'
			baseActor = consume(TokenType::IDENTIFIER, "Expected base actor name").lexeme;
		}

		consume(TokenType::LBRACE, "Expected '{' after actor declaration");

		std::vector<std_P3019_modified::polymorphic<MemberDeclNode>> members;
		while (!match(TokenType::RBRACE) && !isAtEnd()) {
			try {
				// Parse message handlers (start with "on") or regular members
				if (match(TokenType::ON)) {
					members.push_back(parseMessageHandler(std::move(pendingAnnotations)));
				} else {
					// Parse regular members (fields, etc.)
					members.push_back(parseObjectPrimary(name, pendingAnnotations));
				}
			} catch (const ParseError& e) {
				errorReporter.report(e.location, e.format());
				errStream << e.what() << std::endl;
				synchronize();
			}
		}

		consume(TokenType::RBRACE, "Expected '}' after actor body");

		return std_P3019_modified::make_polymorphic<ActorDeclNode>(
				loc,
				std::move(name),
				std::move(members),
				std::move(baseActor)
		);
	}

	//std::vector<std_P3019_modified::polymorphic<MemberDeclNode>> Parser::parseActorMembers(std::string& actorName) {
	//	std::vector<std_P3019_modified::polymorphic<MemberDeclNode>> members;
	//	while (!match(TokenType::RBRACE) && !isAtEnd()) {
	//		try {
	//			// Parse annotations
	//			std::vector<std_P3019_modified::polymorphic<AnnotationNode>> annotations;
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
	std_P3019_modified::polymorphic<MemberDeclNode> Parser::parseMessageHandler(std::vector<std_P3019_modified::polymorphic<AnnotationNode>> annotations) {
		SourceLocation loc = consume(TokenType::ON).loc;
		std::string messageType = consume(TokenType::IDENTIFIER, "Expected message type").lexeme;

		// Parse parameters
		auto [params, _] = parseParameters();

		// Parse optional return type
		std_P3019_modified::polymorphic<TypeNode> returnType;
		if (match(TokenType::ARROW)) {
			advance();
			returnType = parseType();
		}

		auto body = parseBlock();

		return std_P3019_modified::make_polymorphic<MemberDeclNode>(
				loc,
				MemberDeclNode::MESSAGE_HANDLER,
				MemberDeclNode::PUBLIC,
				false,
				std::move(messageType),
				std::move(returnType),
				nullptr,
				std::vector<std::pair<std::string, std_P3019_modified::polymorphic<ExprNode>>>{},
				std::move(body),
				std::move(annotations)
		);
	}

	// Helper function to create error nodes that can be used as members
	[[maybe_unused]] std_P3019_modified::polymorphic<MemberDeclNode> Parser::createErrorNodeAsMember() {
		return std_P3019_modified::make_polymorphic<MemberDeclNode>(
				currentToken.loc,
				MemberDeclNode::FIELD,  // Using FIELD as generic error type
				MemberDeclNode::PRIVATE,
				false,
				"",  // Empty name
				nullptr,  // No type
				nullptr,  // No initializer
				std::vector<std::pair<std::string, std_P3019_modified::polymorphic<ExprNode>>>{},  // No ctor initializers
				nullptr,  // No body
				std::vector<std_P3019_modified::polymorphic<AnnotationNode>>{}  // No annotations
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

	std_P3019_modified::polymorphic<ScopeBlockNode> Parser::parseScopeBlock() {
		SourceLocation loc = consume(TokenType::SCOPE).loc;
		consume(TokenType::LBRACE, "Expected '{' after 'scope'");

		std::vector<std_P3019_modified::polymorphic<ASTNode>> statements;

		while (!match(TokenType::RBRACE) && !isAtEnd()) {
			statements.emplace_back(std::move(parseStatement()));
		}

		consume(TokenType::RBRACE, "Expected '}' after scope block");

		return std_P3019_modified::make_polymorphic<ScopeBlockNode>(loc, std::move(statements));
	}

	std::vector<std_P3019_modified::polymorphic<AnnotationNode>> Parser::parseAnnotations() {
		std::vector<std_P3019_modified::polymorphic<AnnotationNode>> annotations;
		while (match({TokenType::AT/*,TokenType::DOUBLE_AT*/})) {
			annotations.push_back(parseAnnotation());
		}
		return annotations;
	}

	std_P3019_modified::polymorphic<TemplateDeclNode> Parser::parseTemplate() {
		SourceLocation loc = consume(TokenType::TEMPLATE).loc;
		consume(TokenType::LESS, "Expected '<' after 'template'");

		auto params = parseTemplateParameters();

		consume(TokenType::GREATER, "Expected '>' after template parameters");

		// Parse the templated declaration
		std_P3019_modified::polymorphic<ASTNode> declaration;
		if (match(TokenType::CLASS) || match(TokenType::STRUCT)) {
			declaration = parseObject();
		}
		else if (match(TokenType::FUN) || isPotentialMethod()) {
			declaration = parseFunction();
		}
		else if (match(TokenType::UNION)) {
			declaration = parseUnion();
		}
		else if (match(TokenType::ACTOR)) {
			declaration = parseActorDecl();
		}
		else {
			throw ParseError(currentToken.loc,
			                 "Expected class, struct, function, union or actor after template declaration");
		}

		return std_P3019_modified::make_polymorphic<TemplateDeclNode>(
				loc,
				std::move(params),
				declaration
		);
	}

// Helper function to parse template parameters (used for template template parameters)
	std::vector<TemplateParameter> Parser::parseTemplateParameters() {
		std::vector<TemplateParameter> params;
		bool hasVariadic = false;

		do {
			// Handle variadic parameter
			if (match(TokenType::ELLIPSIS)) {
				advance();
				hasVariadic = true;
			}

			if (match(TokenType::TYPENAME)) {
				// TYPE parameter
				advance(); // Consume 'typename'
				if (match(TokenType::ELLIPSIS)) { hasVariadic = true; advance(); }
				std::string name = consume(TokenType::IDENTIFIER,
				                           "Expected template parameter name").lexeme;

				// Parse optional default type
				std_P3019_modified::polymorphic<TypeNode> defaultType;
				if (match(TokenType::EQUAL)) {
					advance();
					defaultType = parseType();
				}

				params.emplace_back(
						TemplateParameter::TYPE,
						std::move(name),
						hasVariadic,
						std::move(defaultType)
				);
			}
			else if (isBuiltInType(currentToken.type) || currentToken.type == TokenType::IDENTIFIER) {
				// NON_TYPE parameter
				auto type = parseType();
				std::string name = consume(TokenType::IDENTIFIER,
				                           "Expected template parameter name").lexeme;

				// Parse optional default value
				std_P3019_modified::polymorphic<ExprNode> defaultValue;
				if (match(TokenType::EQUAL)) {
					advance();
					defaultValue = parsePrimary();
				}

				params.emplace_back(
						TemplateParameter::NON_TYPE,
						std::move(name),
						hasVariadic,
						std::make_pair(std::move(type), std::move(defaultValue))
				);
			}
				/*else if (match(TokenType::TEMPLATE)) {
					// TEMPLATE parameter
					SourceLocation templateLoc = advance().loc;

					// Parse template parameter list
					consume(TokenType::LESS, "Expected '<' after 'template'");
					auto innerParams = parseTemplateParameters(true);
					consume(TokenType::GREATER, "Expected '>' after template parameters");

					std::string name = consume(TokenType::IDENTIFIER,
											   "Expected template parameter name").lexeme;

					params.emplace_back(
							TemplateParameter::TEMPLATE,
							std::move(name),
							hasVariadic,
							std::move(innerParams)
					);
				}*/
			else {
				throw ParseError(currentToken.loc,
				                 "Expected 'typename', type, or 'template' in template parameter");
			}

			hasVariadic = false;

		} while (match(TokenType::COMMA) && (advance(), true));

		return params;
	}

	std_P3019_modified::polymorphic<UnsafeNode> Parser::parseUnsafeBlock() {
		SourceLocation startLoc = currentToken.loc;
		consume(TokenType::LBRACE);

		std::vector<std_P3019_modified::polymorphic<ASTNode>> statements;
		try {
			while (!match(TokenType::RBRACE) && !isAtEnd()) {
				statements.emplace_back(std::move(parseStatement()));
			}
			consume(TokenType::RBRACE,"Expected '}' after block");
		} catch (const ParseError&) {
			synchronize(); // Your error recovery method
			if (!match(TokenType::RBRACE)) {
				// Insert synthetic '}' if missing
				statements.emplace_back(std::move(createErrorNode()));
			}
		}
		return std_P3019_modified::make_polymorphic<UnsafeNode>(startLoc,  std::move(statements));
	}


//	std_P3019_modified::polymorphic<MultiVarDeclNode> Parser::parseVarDecls() {
//		std::vector<std_P3019_modified::polymorphic<VarDeclNode>> declarations;
//
//		// First parse the common declaration parts
//		SourceLocation loc = currentToken.loc;
//		bool isHoisted = match(TokenType::HOIST);
//		VarDeclNode::Kind kind = VarDeclNode::DYNAMIC;
//		std_P3019_modified::polymorphic<TypeNode> commonType;
//
//		if (isBuiltInType(currentToken.type) || currentToken.type == TokenType::IDENTIFIER) {
//			kind = VarDeclNode::STATIC;
//			commonType = parseType();
//		} else if (match({TokenType::LET, TokenType::VAR, TokenType::DYNAMIC})) {
//			kind = VarDeclNode::DYNAMIC;
//			advance();
//		}
//		do {
//			if (!declarations.empty()) {
//				advance();
//			}
//			std::string name = consume(TokenType::IDENTIFIER).lexeme;
//
//			std_P3019_modified::polymorphic<TypeNode> actualType;
//			if (commonType) {
//				if (match(TokenType::LBRACKET)) {
//					advance();
//					auto sizeExpr = parseExpression();
//					consume(TokenType::RBRACKET);
//					actualType = std_P3019_modified::make_polymorphic<ASTNode,ArrayTypeNode>(
//							loc,
//							commonType->clone(),
//							std::move(sizeExpr)
//					);
//				} else {
//					actualType = commonType->clone();
//				}
//			}
//
//			std_P3019_modified::polymorphic<ExprNode> initializer;
//			if (match(TokenType::EQUAL)) {
//				advance();
//				initializer = parseExpression();
//			}
//
//			declarations.push_back(std_P3019_modified::make_polymorphic<ASTNode,VarDeclNode>(
//					loc, kind, std::move(name),
//					actualType ? std::move(actualType) : commonType->clone(),
//					std::move(initializer),
//					isHoisted
//			));
//		} while (match(TokenType::COMMA));
//
//		consume(TokenType::SEMICOLON);
//		return std_P3019_modified::make_polymorphic<ASTNode,MultiVarDeclNode>(loc,std::move(declarations));
//	}
//

}
#pragma clang diagnostic pop