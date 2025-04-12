//
// Created by gogop on 3/28/2025.
//

#pragma once

#include <string>
#include <memory>
#include <utility>
#include <vector>
#include "../ast/ASTNode.hpp"
#include "../lexer/lexer.hpp"

namespace zenith::ast {
	// --- Literal Values ---
	struct LiteralNode : ExprNode {
		enum Type : uint8_t { NUMBER, STRING, BOOL, NIL } type;
		std::string value;

		LiteralNode(SourceLocation loc, Type t, std::string val)
				: ExprNode(), type(t), value(std::move(val)) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const override {
			static const char* typeNames[] = {"NUMBER", "STRING", "BOOL", "NIL"};
			return std::string(indent, ' ') + "Literal(" + typeNames[type] + ": " + value + ")";
		}
	};

	// --- Variable References ---
	struct VarNode : ExprNode {
		std::string name;

		explicit VarNode(SourceLocation loc, std::string n)
				: ExprNode(), name(n) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const override {
			return std::string(indent, ' ') + "Var(" + name + ")";
		}
	};

	// --- Binary Operations ---
	struct BinaryOpNode : ExprNode {
		enum Op : uint8_t { ADD, SUB, MUL, DIV, EQ, NEQ, LT, GT, LTE, GTE, ASN, MOD, ADD_ASN, SUB_ASN, MUL_ASN, DIV_ASN, MOD_ASN  } op;
		std::unique_ptr<ExprNode> left;
		std::unique_ptr<ExprNode> right;

		BinaryOpNode(SourceLocation loc, lexer::TokenType tokenType,
		             std::unique_ptr<ExprNode> leftExpr,
		             std::unique_ptr<ExprNode> rightExpr)
				: ExprNode(), op(convertTokenType(tokenType)),
				  left(std::move(leftExpr)),
				  right(std::move(rightExpr)) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const override {
			static const char* opNames[] = {"+", "-", "*", "/", "==", "!=", "<", ">", "<=", ">=", "=", "%", "+=", "-=", "*=", "/=", "%=", };
			std::string pad(indent, ' ');
			return pad + "BinaryOp(" + opNames[op] + ")\n" +
			       left->toString(indent + 2) + "\n" +
			       right->toString(indent + 2);
		}

	private:
		static Op convertTokenType(lexer::TokenType type) {
			switch(type) {
				case lexer::TokenType::PLUS: return ADD;
				case lexer::TokenType::MINUS: return SUB;
				case lexer::TokenType::STAR: return MUL;
				case lexer::TokenType::SLASH: return DIV;
				case lexer::TokenType::EQUAL_EQUAL: return EQ;
				case lexer::TokenType::BANG_EQUAL: return NEQ;
				case lexer::TokenType::LESS: return LT;
				case lexer::TokenType::GREATER: return GT;
				case lexer::TokenType::LESS_EQUAL: return LTE;
				case lexer::TokenType::GREATER_EQUAL: return GTE;
				case lexer::TokenType::EQUAL: return ASN;
				case lexer::TokenType::PERCENT: return MOD;
				case lexer::TokenType::PLUS_EQUALS: return ADD_ASN;
				case lexer::TokenType::MINUS_EQUALS: return SUB_ASN;
				case lexer::TokenType::STAR_EQUALS: return MUL_ASN;
				case lexer::TokenType::SLASH_EQUALS: return DIV_ASN;
				case lexer::TokenType::PERCENT_EQUALS: return MOD_ASN;
				default: throw std::invalid_argument("Invalid binary operator");
			}
		}
	};

	// --- Function Calls ---
	struct CallNode : ExprNode {
		std::unique_ptr<ExprNode> callee;
		small_vector<std::unique_ptr<ExprNode>, 4> arguments;

		CallNode(SourceLocation loc, std::unique_ptr<ExprNode> c,
		         small_vector<std::unique_ptr<ExprNode>, 4> args)
				: ExprNode(), callee(std::move(c)), arguments(std::move(args)) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "Call\n"
			   << callee->toString(indent + 2) << "\n"
			   << pad << "  Arguments:";
			for (const auto& arg : arguments) {
				ss << "\n" << arg->toString(indent + 2);
			}
			return ss.str();
		}
	};

	// --- Member Access ---
	struct MemberAccessNode : ExprNode {
		std::unique_ptr<ExprNode> object;
		std::string member;

		MemberAccessNode(SourceLocation loc, std::unique_ptr<ExprNode> obj,
		                 std::string mem)
				: ExprNode(), object(std::move(obj)), member(mem) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			return pad + "MemberAccess(.)\n" +
			       object->toString(indent + 2) + "\n" +
			       pad + "  " + member;
		}
	};

	// --- Free Objects ---
	struct FreeObjectNode : ExprNode {
		small_vector<std::pair<std::string, std::unique_ptr<ExprNode>>, 4> properties;

		FreeObjectNode(SourceLocation loc,
		               small_vector<std::pair<std::string, std::unique_ptr<ExprNode>>, 4> props)
				: ExprNode(), properties(std::move(props)) {
			this->loc = loc;
		}

		// --- Conversion from std::string -> std::string (and also vector) ---
		FreeObjectNode(SourceLocation loc,
		               std::vector<std::pair<std::string, std::unique_ptr<ExprNode>>>&& props)
				: ExprNode() {
			this->loc = loc;
			properties.reserve(props.size());
			for (auto& p : props) {
				properties.emplace_back(
						p.first,
						std::move(p.second)         // Transfer ownership
				);
			}
		}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "FreeObject {\n";
			for (const auto& prop : properties) {
				ss << pad << "  " << prop.first << ": "
				   << prop.second->toString(0) << "\n";
			}
			ss << pad << "}";
			return ss.str();
		}
	};

	// --- Array Access ---
	struct ArrayAccessNode : ExprNode {
		std::unique_ptr<ExprNode> array;
		std::unique_ptr<ExprNode> index;

		ArrayAccessNode(SourceLocation loc, std::unique_ptr<ExprNode> arr,
		                std::unique_ptr<ExprNode> idx)
				: ExprNode(), array(std::move(arr)), index(std::move(idx)) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			return pad + "ArrayAccess([])\n" +
			       array->toString(indent + 2) + "\n" +
			       index->toString(indent + 2);
		}
	};

	// --- Object Construction ---
	struct NewExprNode : ExprNode {
		std::string className;
		small_vector<std::unique_ptr<ExprNode>, 4> args;

		NewExprNode(SourceLocation loc, std::string _class,
		            small_vector<std::unique_ptr<ExprNode>, 4> args)
				: ExprNode(), className(_class), args(std::move(args)) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "new " << className << "(";
			const char* separator = "";
			for (const auto& arg : args) {
				ss << separator << arg->toString();
				separator = ", ";
			}
			ss << ")";
			return ss.str();
		}

		bool isConstructorCall() const override { return true; }
	};

	// Expression statement wrapper
	struct ExprStmtNode : StmtNode {
		std::unique_ptr<ExprNode> expr;

		ExprStmtNode(SourceLocation loc, std::unique_ptr<ExprNode> e)
				: expr(std::move(e)) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			return pad + "ExprStmt\n" + expr->toString(indent + 2);
		}
	};

	struct EmptyStmtNode : StmtNode {
		explicit EmptyStmtNode(SourceLocation loc) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const override {
			return std::string(indent, ' ') + "EmptyStmt";
		}
	};

// Return statement node
	struct ReturnStmtNode : StmtNode {
		std::unique_ptr<ExprNode> value;

		ReturnStmtNode(SourceLocation loc, std::unique_ptr<ExprNode> v = nullptr)
				: value(std::move(v)) { this->loc = loc; }

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			if (!value) {
				return pad + "return;";  // No value, simple case
			}

			// Get the value's string without forcing its first line to indent

			return pad + "return " + removePadUntilNewLine(value->toString(indent+2));
		}
	};
	// --- Template Strings ---
	struct TemplateStringNode : ExprNode {
		small_vector<std::unique_ptr<ExprNode>, 4> parts;

		TemplateStringNode(SourceLocation loc,
		                   small_vector<std::unique_ptr<ExprNode>, 4> parts)
				: ExprNode(), parts(std::move(parts)) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::string result = pad + "TemplateString(\n";
			for (const auto& part : parts) {
				result += part->toString(indent + 2) + "\n";
			}
			return result + pad + ")";
		}
	};
	// --- This Reference ---
	struct ThisNode : ExprNode {
		explicit ThisNode(SourceLocation loc) : ExprNode() {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const override {
			return std::string(indent, ' ') + "This";
		}
	};

	struct StructInitializerNode : ExprNode{
		struct StructFieldInitializer {
			std::string name; // empty for positional
			std::unique_ptr<ExprNode> value;
		};

		std::vector<StructFieldInitializer> fields;
		bool isPositional; // true if all fields are positional

		StructInitializerNode(SourceLocation loc, std::vector<StructFieldInitializer> fields)
				: fields(std::move(fields)) {
			this->loc = loc;
			// Determine if this is purely positional
			isPositional = true;
			for (const auto& field : this->fields) {
				if (!field.name.empty()) {
					isPositional = false;
					break;
				}
			}
		}
		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			if(isPositional) ss << "Positional";
			ss << "{";
			for(size_t i=0; i<fields.size(); ++i){
				if(i!=0) ss << ", ";
				if(!fields[i].name.empty()) ss << fields[i].name << " : ";
				ss << fields[i].value->toString(indent + 2);
			}
			ss << "}";
			return ss.str();
		}
	};
	// -- Lambda Expression (Holder) Node --
	struct LambdaExprNode : ExprNode {
		std::unique_ptr<LambdaNode> lambda;

		LambdaExprNode(SourceLocation loc, std::unique_ptr<LambdaNode> l)
				: lambda(std::move(l)) {
			this->loc = std::move(loc);
		}

		std::string toString(int indent = 0) const override {
			return lambda->toString(indent);
		}
	};
}