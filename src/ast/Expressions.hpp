//
// Created by gogop on 3/28/2025.
//

#pragma once

#include <string>
#include <memory>
#include <utility>
#include <vector>
#include "../utils/small_vector.hpp"
#include "../utils/RemovePadding.hpp"
#include "Declarations.hpp"
#include "../core/ASTNode.hpp"
#include "../lexer/lexer.hpp"

namespace zenith{
	// --- Literal Values ---
	struct LiteralNode : ExprNode {
		enum Type : uint8_t { NUMBER, STRING, BOOL, NIL } type;
		std::string value;

		LiteralNode(SourceLocation loc, Type t, std::string val)
				: ExprNode(), type(t), value(std::move(val)) {
			this->loc = std::move(loc);
		}

		std::string toString(int indent = 0) const override {
			static const char* typeNames[] = {"NUMBER", "STRING", "BOOL", "NIL"};
			return std::string(indent, ' ') + "Literal(" + typeNames[type] + ": " + value + ")";
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }
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
		void accept(Visitor& visitor) override { visitor.visit(*this); }
		
	};

	// --- Binary Operations ---
	struct BinaryOpNode : ExprNode {
		enum Op : uint8_t { ADD, SUB, MUL, DIV, EQ, NEQ, LT, GT, LTE, GTE, ASN, MOD, ADD_ASN, SUB_ASN, MUL_ASN, DIV_ASN, MOD_ASN} op;
		std_P3019_modified::polymorphic<ExprNode> left;
		std_P3019_modified::polymorphic<ExprNode> right;

		BinaryOpNode(SourceLocation loc, TokenType tokenType,
		             std_P3019_modified::polymorphic<ExprNode> leftExpr,
		             std_P3019_modified::polymorphic<ExprNode> rightExpr)
				: ExprNode(), op(convertTokenType(tokenType)),
				  left(std::move(leftExpr)),
				  right(std::move(rightExpr)) {
			this->loc = std::move(loc);
		}
		BinaryOpNode(SourceLocation loc, Op opType,
		             std_P3019_modified::polymorphic<ExprNode> leftExpr,
		             std_P3019_modified::polymorphic<ExprNode> rightExpr)
				: ExprNode(), op(opType),
				  left(std::move(leftExpr)),
				  right(std::move(rightExpr)) {
			this->loc = std::move(loc);
		}

		std::string toString(int indent = 0) const override {
			static const char* opNames[] = {"+", "-", "*", "/", "==", "!=", "<", ">", "<=", ">=", "=", "%", "+=", "-=", "*=", "/=", "%="};
			std::string pad(indent, ' ');
			return pad + "BinaryOp(" + opNames[op] + ")\n" +
			       left->toString(indent + 2) + "\n" +
			       right->toString(indent + 2);
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }

	private:
		static Op convertTokenType(TokenType type) {
			switch(type) {
				case TokenType::PLUS: return ADD;
				case TokenType::MINUS: return SUB;
				case TokenType::STAR: return MUL;
				case TokenType::SLASH: return DIV;
				case TokenType::EQUAL_EQUAL: return EQ;
				case TokenType::BANG_EQUAL: return NEQ;
				case TokenType::LESS: return LT;
				case TokenType::GREATER: return GT;
				case TokenType::LESS_EQUAL: return LTE;
				case TokenType::GREATER_EQUAL: return GTE;
				case TokenType::EQUAL: return ASN;
				case TokenType::PERCENT: return MOD;
				case TokenType::PLUS_EQUALS: return ADD_ASN;
				case TokenType::MINUS_EQUALS: return SUB_ASN;
				case TokenType::STAR_EQUALS: return MUL_ASN;
				case TokenType::SLASH_EQUALS: return DIV_ASN;
				case TokenType::PERCENT_EQUALS: return MOD_ASN;
				default: throw std::invalid_argument("Invalid binary operator");
			}
		}
	};

	// --- Unary Operations ---
	struct UnaryOpNode : ExprNode{
		enum Op : uint8_t {INC, DEC } op;
		std_P3019_modified::polymorphic<ExprNode> right;
		bool prefix = false;
		UnaryOpNode(SourceLocation loc, TokenType tokenType,
				std_P3019_modified::polymorphic<ExprNode> rightExpr, bool prefix)
		: ExprNode(), op(convertTokenType(tokenType)),
		  right(std::move(rightExpr)), prefix(prefix) {
			this->loc = std::move(loc);
		}
		UnaryOpNode(SourceLocation loc, Op op,
		            std_P3019_modified::polymorphic<ExprNode> rightExpr, bool prefix)
				: ExprNode(), op(op),
				  right(std::move(rightExpr)), prefix(prefix) {
			this->loc = std::move(loc);
		}

		std::string toString(int indent = 0) const override {
			static const char* opNames[] = {"++", "--"};
			std::string pad(indent, ' ');
			return pad + "UnaryOp(" + opNames[op] + ")\n" +
					right->toString(indent + 2) + "\n";
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }

	private:
		static Op convertTokenType(TokenType type) {
			switch(type) {
				case TokenType::INCREASE: return INC;
				case TokenType::DECREASE: return DEC;
				default: throw std::invalid_argument("Invalid binary operator");
			}
		}
	};

	// --- Function Calls ---
	struct CallNode : ExprNode {
		std_P3019_modified::polymorphic<ExprNode> callee;
		small_vector<std_P3019_modified::polymorphic<ExprNode>, 4> arguments;

		CallNode(SourceLocation loc, std_P3019_modified::polymorphic<ExprNode> c,
		         small_vector<std_P3019_modified::polymorphic<ExprNode>, 4> args)
				: ExprNode(), callee(std::move(c)), arguments(std::move(args)) {
			this->loc = std::move(loc);
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
		void accept(Visitor& visitor) override { visitor.visit(*this); }

	};

	// --- Member Access ---
	struct MemberAccessNode : ExprNode {
		std_P3019_modified::polymorphic<ExprNode> object;
		std::string member;

		MemberAccessNode(SourceLocation loc, std_P3019_modified::polymorphic<ExprNode> obj,
		                 std::string mem)
				: ExprNode(), object(std::move(obj)), member(std::move(mem)) {
			this->loc = std::move(loc);
		}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			return pad + "MemberAccess(.)\n" +
			       object->toString(indent + 2) + "\n" +
			       pad + "  " + member;
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }

	};

	// --- Free Objects ---
	struct FreeObjectNode : ExprNode {
		small_vector<std::pair<std::string, std_P3019_modified::polymorphic<ExprNode>>, 4> properties;

		FreeObjectNode(SourceLocation loc,
		               small_vector<std::pair<std::string, std_P3019_modified::polymorphic<ExprNode>>, 4> props)
				: ExprNode(), properties(std::move(props)) {
			this->loc = std::move(loc);
		}

		// --- Conversion from std::string -> std::string (and also vector) ---
		FreeObjectNode(SourceLocation loc,
		               std::vector<std::pair<std::string, std_P3019_modified::polymorphic<ExprNode>>>&& props)
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
		void accept(Visitor& visitor) override { visitor.visit(*this); }

	};

	// --- Array Access ---
	struct ArrayAccessNode : ExprNode {
		std_P3019_modified::polymorphic<ExprNode> array;
		std_P3019_modified::polymorphic<ExprNode> index;

		ArrayAccessNode(SourceLocation loc, std_P3019_modified::polymorphic<ExprNode> arr,
		                std_P3019_modified::polymorphic<ExprNode> idx)
				: ExprNode(), array(std::move(arr)), index(std::move(idx)) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			return pad + "ArrayAccess([])\n" +
			       array->toString(indent + 2) + "\n" +
			       index->toString(indent + 2);
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }

	};

	// --- Object Construction ---
	struct NewExprNode : ExprNode {
		std::string className;
		small_vector<std_P3019_modified::polymorphic<ExprNode>, 4> args;

		NewExprNode(SourceLocation loc, std::string _class,
		            small_vector<std_P3019_modified::polymorphic<ExprNode>, 4> args)
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
		void accept(Visitor& visitor) override { visitor.visit(*this); }
		bool isConstructorCall() const override { return true; }

	};

	// Expression statement wrapper
	struct ExprStmtNode : StmtNode {
		std_P3019_modified::polymorphic<ExprNode> expr;

		ExprStmtNode(SourceLocation loc, std_P3019_modified::polymorphic<ExprNode> e)
				: expr(std::move(e)) {
			this->loc = std::move(loc);
		}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			return pad + "ExprStmt\n" + expr->toString(indent + 2);
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }
	};

	struct EmptyStmtNode : StmtNode {
		explicit EmptyStmtNode(SourceLocation loc) {
			this->loc = std::move(loc);
		}

		std::string toString(int indent = 0) const override {
			return std::string(indent, ' ') + "EmptyStmt";
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }

	};

	// Return statement node
	struct ReturnStmtNode : StmtNode {
		std_P3019_modified::polymorphic<ExprNode> value;

		ReturnStmtNode(SourceLocation loc, std_P3019_modified::polymorphic<ExprNode> v = nullptr)
				: value(std::move(v)) { this->loc = loc; }

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			if (!value) {
				return pad + "return;";  // No value, simple case
			}

			// Get the value's string without forcing its first line to indent

			return pad + "return " + removePadUntilNewLine(value->toString(indent+2));
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }


	};
	// --- Template Strings ---
	struct TemplateStringNode : ExprNode {
		small_vector<std_P3019_modified::polymorphic<ExprNode>, 4> parts;

		TemplateStringNode(SourceLocation loc,
		                   small_vector<std_P3019_modified::polymorphic<ExprNode>, 4> parts)
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
		void accept(Visitor& visitor) override { visitor.visit(*this); }

	};
	// --- This Reference ---
	struct ThisNode : ExprNode {
		explicit ThisNode(SourceLocation loc) : ExprNode() {
			this->loc = std::move(loc);
		}

		std::string toString(int indent = 0) const override {
			return std::string(indent, ' ') + "This";
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }

	};

	struct StructInitializerNode : ExprNode{
		struct StructFieldInitializer {
			std::string name; // empty for positional
			std_P3019_modified::polymorphic<ExprNode> value;
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
				ss << fields[i].value->toString();
			}
			ss << "}";
			return ss.str();
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }

	};
	// -- Lambda Expression (Holder) Node --
	struct LambdaExprNode : ExprNode {
		std_P3019_modified::polymorphic<LambdaNode> lambda;

		LambdaExprNode(SourceLocation loc, std_P3019_modified::polymorphic<LambdaNode> l)
				: lambda(std::move(l)) {
			this->loc = std::move(loc);
		}

		std::string toString(int indent = 0) const override {
			return lambda->toString(indent);
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }

	};
}