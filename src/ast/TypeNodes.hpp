#pragma once

#include <memory>
#include <utility>
#include <vector>
#include <sstream>
#include <unordered_set>
#include "../core/ASTNode.hpp"
#include "../core/indirect_polymorphic.hpp"


namespace zenith {
	// Base type node
	struct TypeNode : ASTNode {
		enum Kind { PRIMITIVE, OBJECT, ARRAY, FUNCTION, DYNAMIC, TEMPLATE, ERROR } kind;

		explicit TypeNode(SourceLocation loc, Kind k) : kind(k) {
			this->loc = std::move(loc);
		}

		virtual std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			static const char* kindNames[] = {"PRIMITIVE", "CLASS", "Object", "FUNCTION", "DYNAMIC", "TEMPLATE", "ERROR"};
			return pad + "Type(" + kindNames[kind] + ")";
		}
		virtual bool isDynamic() const {
			return kind==DYNAMIC;
		}
		virtual std::unique_ptr<TypeNode> clone() const {
			return std::unique_ptr<TypeNode>();
		}

		void accept(Visitor& visitor) override { visitor.visit(*this); }
	};


	// Primitive types (int, float, etc.)
	struct PrimitiveTypeNode : TypeNode {
		enum Type { INT, FLOAT, DOUBLE, STRING, BOOL, NUMBER, BIGINT, BIGNUMBER, SHORT, LONG, BYTE, VOID, NIL } type;

		PrimitiveTypeNode(SourceLocation loc, Type t)
				: TypeNode(std::move(loc), PRIMITIVE), type(t) {}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			static const char* typeNames[] = {"INT", "FLOAT", "DOUBLE", "STRING",
			                                  "BOOL", "NUMBER", "BIGINT", "BIGNUMBER", "SHORT", "LONG", "BYTE", "VOID", "NIL"};
			return pad + "PrimitiveType(" + typeNames[type] + ")";
		}
		bool isDynamic() const override {
			static const std::unordered_set<Type> basic_types = {
					INT, FLOAT, DOUBLE, BOOL, SHORT, LONG, BYTE, VOID, NIL
			};

			return !basic_types.contains(type);
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }
		std::unique_ptr<TypeNode> clone() const override {
			return std::make_unique<PrimitiveTypeNode>(loc, type);
		}
	};

	// Class/struct types
	struct NamedTypeNode : TypeNode {
		std::string name;

		NamedTypeNode(SourceLocation loc, std::string n)
				: TypeNode(loc, OBJECT), name(std::move(n)) {}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			return pad + "NamedType(" + name + ")";
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }
	};

	// Array types
	struct ArrayTypeNode : TypeNode {
		std::unique_ptr<TypeNode> elementType;
		std::unique_ptr<ExprNode> sizeExpr;

		ArrayTypeNode(SourceLocation loc, std::unique_ptr<TypeNode> elemType, std::unique_ptr<ExprNode> sizeExpr = nullptr)
				: TypeNode(std::move(loc), ARRAY), elementType(std::move(elemType)), sizeExpr(std::move(sizeExpr)) {}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			return pad + "ArrayType\n" +
			       elementType->toString(indent + 2);
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }
	};
	struct TemplateTypeNode : TypeNode {
		std::string baseName;
		std::vector<std::unique_ptr<TypeNode>> templateArgs;

		TemplateTypeNode(SourceLocation loc,
		                 std::string baseName,
		                 std::vector<std::unique_ptr<TypeNode>> templateArgs)
				: TypeNode(loc, TEMPLATE),
				  baseName(std::move(baseName)),
				  templateArgs(std::move(templateArgs)) {}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "TemplateType(" << baseName << "<";
			for (size_t i = 0; i < templateArgs.size(); ++i) {
				if (i > 0) ss << ", ";
				ss << templateArgs[i]->toString();
			}
			ss << ">)";
			return ss.str();
		}

		bool isDynamic() const override {
			// A template type is dynamic if any of its arguments are dynamic
			for (const auto& arg : templateArgs) {
				if (arg->isDynamic()) return true;
			}
			return false;
		}

		void accept(Visitor& visitor) override { visitor.visit(*this); }
	};
	struct FunctionTypeNode : TypeNode{
		std::vector<std::unique_ptr<TypeNode>> parameterTypes;
		std::unique_ptr<TypeNode> returnType;

		FunctionTypeNode(SourceLocation loc,
		                 std::vector<std::unique_ptr<TypeNode>> params,
		                 std::unique_ptr<TypeNode> retType)
				: TypeNode(std::move(loc), FUNCTION),
				  parameterTypes(std::move(params)),
				  returnType(std::move(retType)) {};

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "FunctionType(";
			// Parameter Types
			ss << "(";
			for (size_t i = 0; i < parameterTypes.size(); ++i) {
				if (i > 0) ss << ", ";
				ss << (parameterTypes[i] ? parameterTypes[i]->toString(0) : "<invalid>");
			}
			ss << ")";
			ss << " -> ";
			ss << (returnType ? returnType->toString(0) : "<void/inferred>");
			ss << ")";
			return ss.str();
		}
		bool isDynamic() const override { return false; }

		void accept(Visitor& visitor) override{
			visitor.visit(*this);
		}
	};
}