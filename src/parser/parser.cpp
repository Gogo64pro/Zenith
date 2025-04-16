#include <algorithm>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "../ast/Expressions.hpp"
#include "error.hpp"
#include "parser.hpp"

namespace zenith::parser {

// Fixed parseVarDecl to return the node
std::unique_ptr<ast::VarDeclNode> Parser::parseVarDecl() {
	const auto loc = currentToken.span();
	bool isHoisted = match(lexer::TokenType::HOIST);
	ast::VarDeclNode::Kind kind = ast::VarDeclNode::DYNAMIC; // Default to dynamic
	std::unique_ptr<ast::TypeNode> typeNode;

	// Case 1: Static typed declaration (int a = 12)
	if (isBuiltInType(currentToken.type()) || currentToken.type() == lexer::TokenType::IDENTIFIER) {
		kind = ast::VarDeclNode::STATIC;
		typeNode = parseType();
	}
		// Case 2: Dynamic declaration (let/var/dynamic)
	else if (match(lexer::TokenType::LET) || match(lexer::TokenType::VAR) || match(lexer::TokenType::DYNAMIC)) {
		kind = ast::VarDeclNode::DYNAMIC;
		advance();
	}

	std::string name(mod.getLexeme(consume(lexer::TokenType::IDENTIFIER)));

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
	return currentToken.type() == type;
}

bool Parser::match(std::initializer_list<lexer::TokenType> types) const {
	if (isAtEnd()) return false;
	return std::find(types.begin(), types.end(), currentToken.type()) != types.end();
}

lexer::Token Parser::consume(lexer::TokenType type, const std::string& errorMessage) {
	if (match(type)) {
		return advance();
	}
	throw Error(currentToken.span(), errorMessage);
}

lexer::Token Parser::consume(lexer::TokenType type) {
	return consume(type, std::string("Expected ") + toString(type));
}

std::unique_ptr<ast::ExprNode> Parser::parseExpression(int precedence) { // Remove default arg here
	auto expr = parsePrimary();

	while (true) {
		lexer::Token op = currentToken;
		int opPrecedence = getPrecedence(op.type());

		if (opPrecedence <= precedence) break;

		advance();
		auto right = parseExpression(opPrecedence);
		expr = std::make_unique<ast::BinaryOpNode>(
			op.span(), // Pass operator location
			op.type(),
			std::move(expr),
			std::move(right)
		);
	}

	return expr;
}

std::unique_ptr<ast::ExprNode> Parser::parsePrimary() {
	const auto startLoc = currentToken.span();

	// Handle all the simple cases first
	if (match(lexer::TokenType::NEW)) {
		return parseNewExpression();
	}
	else if (match({lexer::TokenType::NUMBER,lexer::TokenType::INTEGER,lexer::TokenType::FLOAT_LIT})) {
		lexer::Token numToken = advance();
		return std::make_unique<ast::LiteralNode>(startLoc, ast::LiteralNode::NUMBER, std::string(mod.getLexeme(numToken)));
	}
	else if (match(lexer::TokenType::STRING_LIT)) {
		lexer::Token strToken = advance();
		return std::make_unique<ast::LiteralNode>(startLoc, ast::LiteralNode::STRING, std::string(mod.getLexeme(strToken)));
	} else if (match(lexer::TokenType::TRUE) || match(lexer::TokenType::FALSE)) {
		lexer::Token boolToken = advance();
		return std::make_unique<ast::LiteralNode>(startLoc, ast::LiteralNode::BOOL, std::string(mod.getLexeme(boolToken)));
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
	}
	else if (match(lexer::TokenType::FREEOBJ)) {
		advance(); // consume 'freeobj'
		if (match(lexer::TokenType::LBRACE)) {
			return parseFreeObject();
		}
		throw Error(startLoc,"Expected '{' after free obj, Achievement unlocked: How did we get here?");
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

		if (identToken.type() == lexer::TokenType::THIS) {
			expr = std::make_unique<ast::ThisNode>(startLoc);
		} else {
			expr = std::make_unique<ast::VarNode>(startLoc, std::string(mod.getLexeme(identToken)));
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

	throw Error(currentToken.span(),
		std::string("Expected primary expression, got ") +
		toString(currentToken.type()));
}

lexer::Token Parser::advance() {
	if (isAtEnd()) {
		return lexer::Token{lexer::TokenType::EOF_TOKEN, tokens.empty() ? lexer::SourceSpan{0, 0} : tokens.back().span()};
	}

	lexer::Token result = tokens[current];  // 1. Get current token first
	previous = current;

	// 2. Only advance if not already at end
	if (current + 1 < tokens.size()) {
		current++;
		currentToken = tokens[current];
	} else {
		current = tokens.size();  // Mark as ended
		currentToken = lexer::Token{lexer::TokenType::EOF_TOKEN, result.span()};
	}

	return result;  // Return what was current when we entered
}

Parser::Parser(Module& mod, std::vector<lexer::Token> tokens, const utils::Flags& flags, std::ostream& errStream)
	: tokens(std::move(tokens)),
		currentToken(this->tokens.empty() ?
			lexer::Token{lexer::TokenType::EOF_TOKEN, {0, 0}} :
			this->tokens[0]) , flags(flags), errStream(errStream), mod(mod) {
	current = 0;
}

bool Parser::isAtEnd() const {
	return current >= tokens.size();
}

std::unique_ptr<ast::TypeNode> Parser::parseType() {
	const auto startLoc = currentToken.span();

	// Handle built-in types (from lexer::TokenType)
	if (match({lexer::TokenType::INT, lexer::TokenType::LONG, lexer::TokenType::SHORT,
		lexer::TokenType::BYTE, lexer::TokenType::FLOAT, lexer::TokenType::DOUBLE,
		lexer::TokenType::STRING, lexer::TokenType::NUMBER, lexer::TokenType::BIGINT,
		lexer::TokenType::BIGNUMBER, lexer::TokenType::FREEOBJ})) {

		lexer::Token typeToken = advance();

		ast::PrimitiveTypeNode::Type kind;
		     if (typeToken.type() == lexer::TokenType::INT   ) kind = ast::PrimitiveTypeNode::INT;
		else if (typeToken.type() == lexer::TokenType::LONG  ) kind = ast::PrimitiveTypeNode::LONG;
		else if (typeToken.type() == lexer::TokenType::SHORT ) kind = ast::PrimitiveTypeNode::SHORT;
		else if (typeToken.type() == lexer::TokenType::BYTE  ) kind = ast::PrimitiveTypeNode::BYTE;
		else if (typeToken.type() == lexer::TokenType::FLOAT ) kind = ast::PrimitiveTypeNode::FLOAT;
		else if (typeToken.type() == lexer::TokenType::DOUBLE) kind = ast::PrimitiveTypeNode::DOUBLE;
		else if (typeToken.type() == lexer::TokenType::STRING) kind = ast::PrimitiveTypeNode::STRING;
		else if (typeToken.type() == lexer::TokenType::NUMBER) kind = ast::PrimitiveTypeNode::NUMBER;
		else if (typeToken.type() == lexer::TokenType::BIGINT) kind = ast::PrimitiveTypeNode::BIGINT;
		else if (typeToken.type() == lexer::TokenType::FREEOBJ) {
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
		return std::make_unique<ast::NamedTypeNode>(startLoc, std::string(mod.getLexeme(typeToken)));
	}

	throw Error(
		currentToken.span(),
		std::string("Expected type name, got ") + toString(currentToken.type())
	);
}

void Parser::synchronize() {
	// 1. Always consume the current problematic token first
	advance();

	// 2. Skip tokens until we find a synchronization point
	while (!isAtEnd()) {
		// 3. Check for statement/declaration starters
		switch (currentToken.type()) {
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
			
			default:
		}

		// 4. Skip all other tokens
		advance();
	}
}

const lexer::Token &Parser::previousToken() const {
	if (previous >= tokens.size()) {
		static lexer::Token eof{lexer::TokenType::EOF_TOKEN, {0,0}};
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
	const auto startLoc = currentToken.span();
	std::vector<std::unique_ptr<ast::Node>> declarations;
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
			else if (isBuiltInType(currentToken.type()) || currentToken.type() == lexer::TokenType::IDENTIFIER) {
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
		} catch (const Error& e) {
			mod.report(e.location, e.what());
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
	const auto location = consume(lexer::TokenType::NEW).span(); // Eat 'new' keyword
	std::string className(mod.getLexeme(consume(lexer::TokenType::IDENTIFIER)));

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
	const auto loc = currentToken.span();
	bool hasFunKeyword = false;

	// Handle annotations
	bool isAsync = false;
	while (match(lexer::TokenType::AT)) {
		if (mod.getLexeme(peek()) == "Async") {
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
		if (isBuiltInType(currentToken.type()) || currentToken.type() == lexer::TokenType::IDENTIFIER) {
			// Peek ahead to distinguish:
			// 1) 'fun int foo' -> returnType=int
			// 2) 'fun foo'     -> function name
			if (peek(1).type() == lexer::TokenType::IDENTIFIER && peek(2).type() == lexer::TokenType::LPAREN) {
				returnType = parseType();
			}
		}
	}
		// Handle C-style declarations (e.g. "int foo()")
	else if (isBuiltInType(currentToken.type()) || currentToken.type() == lexer::TokenType::IDENTIFIER) {
		returnType = parseType();
	}
	std::string name(mod.getLexeme(consume(lexer::TokenType::IDENTIFIER)));

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
	const auto startLoc = currentToken.span();
	consume(lexer::TokenType::LBRACE);

	std::vector<std::unique_ptr<ast::Node>> statements;
	try {
		while (!match(lexer::TokenType::RBRACE) && !isAtEnd()) {
			statements.push_back(parseStatement());
		}
		consume(lexer::TokenType::RBRACE,"Expected '}' after block");
	} catch (const Error&) {
		synchronize(); // Your error recovery method
		if (!match(lexer::TokenType::RBRACE)) {
			// Insert synthetic '}' if missing
			statements.push_back(createErrorNode());
		}
	}

	return std::make_unique<ast::BlockNode>(startLoc, std::move(statements));
}

std::unique_ptr<ast::StmtNode> Parser::parseStatement() {
	const auto loc = currentToken.span();

	// Declaration statements
	if (match({lexer::TokenType::LET, lexer::TokenType::VAR, lexer::TokenType::DYNAMIC})) {
		return parseVarDecl();
	}

	// Static variable declarations (primitive or class type)
	if (isBuiltInType(currentToken.type()) || currentToken.type() == lexer::TokenType::IDENTIFIER) {
		// Check if this is actually a declaration by looking ahead
		if (peek(1).type() == lexer::TokenType::IDENTIFIER &&
			(peek(2).type() == lexer::TokenType::EQUAL || peek(2).type() == lexer::TokenType::SEMICOLON)) {
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
	throw Error(currentToken.span(), "Unexpected token in statement: " + std::string(mod.getLexeme(currentToken)));
}

bool Parser::peekIsExpressionStart() const {
	switch (currentToken.type()) {
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
		return lexer::Token{lexer::TokenType::EOF_TOKEN, tokens.empty() ? lexer::SourceSpan{0,0} : tokens.back().span()};
	}
	return tokens[idx];
}

std::unique_ptr<ast::IfNode> Parser::parseIfStmt() {
	const auto loc = consume(lexer::TokenType::IF).span();
	consume(lexer::TokenType::LPAREN, "Expected '(' after 'if'");
	auto condition = parseExpression();
	consume(lexer::TokenType::RPAREN, "Expected ')' after if condition");

	// Parse 'then' branch (enforce braces if required)
	std::unique_ptr<ast::StmtNode> thenBranch;
	try {
		if (flags.bracesRequired && !match(lexer::TokenType::LBRACE)) {
			throw Error(currentToken.span(), "Expected '{' after 'if'");
		}
		thenBranch = parseStatement();
	} catch (const Error& e) {
		mod.report(e.location, "Error in if body " + std::string(e.what()));
		errStream << "Error in if body: " << e.what() << std::endl;
		synchronize(); // Skip to next statement
		auto errorNode = createErrorNode();
		thenBranch = std::unique_ptr<ast::StmtNode>(dynamic_cast<ast::StmtNode*>(errorNode.release()));
		if (!thenBranch) {  // Fallback if cast fails
			thenBranch = std::make_unique<ast::EmptyStmtNode>(errorNode->loc);
		}
	}

	// Parse 'else' branch (also enforce braces if required)
	std::unique_ptr<ast::Node> elseBranch;
	if (match(lexer::TokenType::ELSE)) {
		advance();
		if (flags.bracesRequired) {
			if (!match(lexer::TokenType::LBRACE)) {
				throw Error(currentToken.span(),
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
	const auto loc = consume(lexer::TokenType::FOR).span();
	consume(lexer::TokenType::LPAREN);

	// Parse init/condition/increment (unchanged)
	std::unique_ptr<ast::Node> init;
	if (match(lexer::TokenType::SEMICOLON)) {
		advance(); // Empty initializer
	}
	else if (match({lexer::TokenType::LET, lexer::TokenType::VAR, lexer::TokenType::DYNAMIC}) ||
		isBuiltInType(currentToken.type())) {
		init = parseVarDecl();
	}
	else {
		init = std::make_unique<ast::ExprStmtNode>(currentToken.span(), parseExpression());
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
		throw Error(currentToken.span(),
			"Expected '{' after 'for' (braces are required)");
	}

	auto body = parseStatement(); // parseBlock() if braces are required
	return std::make_unique<ast::ForNode>(loc, std::move(init), std::move(condition),
		std::move(increment), std::move(body));
}

std::unique_ptr<ast::WhileNode> Parser::parseWhileStmt() {
	const auto loc = consume(lexer::TokenType::WHILE).span();
	consume(lexer::TokenType::LPAREN, "Expected '(' after 'while'");
	auto condition = parseExpression();
	consume(lexer::TokenType::RPAREN, "Expected ')' after while condition");

	// Enforce braces if required
	if (flags.bracesRequired && !match(lexer::TokenType::LBRACE)) {
		throw Error(currentToken.span(),
			"Expected '{' after 'while' (braces are required)");
	}

	auto body = parseStatement(); // parseBlock() if braces are required
	return std::make_unique<ast::WhileNode>(loc, std::move(condition), std::move(body));
}

std::unique_ptr<ast::DoWhileNode> Parser::parseDoWhileStmt() {
	const auto loc = consume(lexer::TokenType::DO).span();

	// Enforce braces if required
	if (flags.bracesRequired && !match(lexer::TokenType::LBRACE)) {
		throw Error(currentToken.span(),
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
	const auto loc = consume(lexer::TokenType::RETURN).span();

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
	const auto loc = consume(lexer::TokenType::LBRACE).span();
	std::vector<std::pair<std::string, std::unique_ptr<ast::ExprNode>>> properties;

	if (!match(lexer::TokenType::RBRACE)) {
		do {
			std::string name(mod.getLexeme(consume(lexer::TokenType::IDENTIFIER)));
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
	const auto loc = callee->loc;
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
	const auto loc = object->loc;

	// First access (guaranteed to exist)
	advance(); // Consume '.'
	std::string member(mod.getLexeme(consume(lexer::TokenType::IDENTIFIER)));
	auto result = std::make_unique<ast::MemberAccessNode>(loc, std::move(object), member);

	// Handle additional accesses or calls
	while (match(lexer::TokenType::DOT)) {
		advance();
		member = std::string(mod.getLexeme(consume(lexer::TokenType::IDENTIFIER)));
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
	const auto loc = currentToken.span();

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
	const auto loc = consume(lexer::TokenType::AT).span(); // Eat '@' symbol

	// Parse annotation name
	std::string name(mod.getLexeme(consume(lexer::TokenType::IDENTIFIER)));

	// Parse optional annotation arguments
	std::vector<std::pair<std::string, std::unique_ptr<ast::ExprNode>>> arguments;

	if (match(lexer::TokenType::LPAREN)) {
		advance(); // Consume '('

		if (!match(lexer::TokenType::RPAREN)) {
			do {
				// Parse argument name (optional) or just value
				std::string argName;
				if (peek(1).type() == lexer::TokenType::EQUAL) {
					argName = std::string(mod.getLexeme(consume(lexer::TokenType::IDENTIFIER)));
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
	return std::make_unique<ast::ErrorNode>(currentToken.span());
}

std::unique_ptr<ast::ImportNode> Parser::parseImport() {
	const auto loc = consume(lexer::TokenType::IMPORT).span();
	std::string importPath;
	bool isJavaImport = false;

	// Handle Java imports (keyword 'java' followed by package path)
	if (match(lexer::TokenType::JAVA)) {
		advance(); // Consume the 'java' keyword
		isJavaImport = true;

		// Now parse the package path (which may contain 'java' as identifier)
		while (true) {
			importPath += mod.getLexeme(consume(lexer::TokenType::IDENTIFIER));
			if (!match(lexer::TokenType::DOT)) break;
			importPath += ".";
			advance(); // Consume '.'
		}
	}
		// Handle quoted path imports
	else if (match(lexer::TokenType::STRING_LIT)) {
		importPath = mod.getLexeme(consume(lexer::TokenType::STRING_LIT));
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
			importPath += mod.getLexeme(consume(lexer::TokenType::IDENTIFIER));
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
	const auto classLoc = consume(lexer::TokenType::CLASS).span();
	std::string className(mod.getLexeme(consume(lexer::TokenType::IDENTIFIER, "Expected class name")));

	// Parse inheritance
	std::string baseClass;
	if(match(lexer::TokenType::COLON)) {
		consume(lexer::TokenType::COLON);
		baseClass = std::string(mod.getLexeme(consume(lexer::TokenType::IDENTIFIER, "Expected base class name")));
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
			if (match(lexer::TokenType::IDENTIFIER) && mod.getLexeme(currentToken) == className) {
				// Constructor
				const auto loc = advance().span();
				std::vector<std::pair<std::string, std::unique_ptr<ast::ExprNode>>> initializers;
				auto params = parseParameters();
				if (match(lexer::TokenType::COLON)) {
					advance(); // Consume the ':'

					do {
						std::string memberName(mod.getLexeme(consume(lexer::TokenType::IDENTIFIER, "Expected member name in initializer list")));
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
				if (isBuiltInType(currentToken.type()) || currentToken.type() == lexer::TokenType::IDENTIFIER) {
					type = parseType();
				}

				std::string name(mod.getLexeme(consume(lexer::TokenType::IDENTIFIER, "Expected type after declaration")));

				std::unique_ptr<ast::ExprNode> initializer;
				if (match(lexer::TokenType::EQUAL)) {
					advance();
					initializer = parseExpression();
				}

				consume(lexer::TokenType::SEMICOLON, "Expected ';' after field declaration");

				members.push_back(std::make_unique<ast::MemberDeclNode>(
					currentToken.span(),
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
		} catch (const Error& e) {
			mod.report(e.location, e.what());
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
		if(isBuiltInType(currentToken.type()) || (currentToken.type() == lexer::TokenType::IDENTIFIER)){
			paramType = parseType();
		}else if(match({lexer::TokenType::LET,lexer::TokenType::VAR,lexer::TokenType::DYNAMIC})){
			advance();
			paramType = std::make_unique<ast::TypeNode>(currentToken.span(), ast::TypeNode::DYNAMIC);
		}/*else if (lastExplicitType) {
		paramType = lastExplicitType->clone();
		}*/else{
			paramType = std::make_unique<ast::TypeNode>(currentToken.span(), ast::TypeNode::DYNAMIC);
		}
		std::string name(mod.getLexeme(consume(lexer::TokenType::IDENTIFIER, "Expected parameter name")));
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
		if (tok.type() == lexer::TokenType::LBRACKET) {
			lookahead++;
			continue;
		}

		// If we find '(' before ';' or '=', it's a method
		if (tok.type() == lexer::TokenType::LPAREN) {
			return true;
		}

		// If we find these tokens before '(', it's not a method
		if (tok.type() == lexer::TokenType::SEMICOLON ||
			tok.type() == lexer::TokenType::EQUAL ||
			tok.type() == lexer::TokenType::LBRACE) {
			return false;
		}

		lookahead++;
	}
	return false;
}
bool Parser::isInStructInitializerContext() const {
	// Look back to see if we're after an equals sign following a type name
	if (previous > 0 && tokens[previous].type() == lexer::TokenType::EQUAL) {
		// Check if before the equals sign we have a type name
		if (previous > 1 &&
			(isBuiltInType(tokens[previous-2].type()) || tokens[previous-2].type() == lexer::TokenType::IDENTIFIER) && tokens[previous-2].type() != lexer::TokenType::FREEOBJ) {
			return true;
		}
	}
	return false;
}

std::unique_ptr<ast::StructInitializerNode> Parser::parseStructInitializer() {
	const auto loc = consume(lexer::TokenType::LBRACE).span();
	std::vector<ast::StructInitializerNode::StructFieldInitializer> fields;

	if (!match(lexer::TokenType::RBRACE)) {
		do {
			// Check for designated initializer (C-style .field = value)
			if (match(lexer::TokenType::DOT)) {
				advance(); // Consume '.'
				std::string name(mod.getLexeme(consume(lexer::TokenType::IDENTIFIER)));
				consume(lexer::TokenType::EQUAL);
				auto value = parseExpression();
				fields.push_back({name, std::move(value)});
			}
				// Check for named field (field: value)
			else if (peek(1).type() == lexer::TokenType::COLON) {
				std::string name(mod.getLexeme(consume(lexer::TokenType::IDENTIFIER)));
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
			params.push_back(std::string(mod.getLexeme(consume(lexer::TokenType::IDENTIFIER, "Expected parameter name"))));
		} while (match(lexer::TokenType::COMMA) && (advance(), true));
	}

	consume(lexer::TokenType::RPAREN, "Expected ')' after arrow function parameters");
	return params;
}

std::unique_ptr<ast::LambdaExprNode> Parser::parseArrowFunction(std::vector<std::string>&& params) {
	const auto loc = consume(lexer::TokenType::LAMBARROW).span();

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
		auto stmts = std::vector<std::unique_ptr<ast::Node>>();
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

} // zenith::parser