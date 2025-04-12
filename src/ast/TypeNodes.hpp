#pragma once

#include <memory>
#include "../ast/ASTNode.hpp"

namespace zenith {
	// Base type node
	struct TypeNode : ASTNode {
		enum Kind { PRIMITIVE, CLASS, STRUCT, ARRAY, FUNCTION, DYNAMIC } kind;

		explicit TypeNode(SourceLocation loc, Kind k) : kind(k) {
			this->loc = loc;
		}

		virtual std::string toString(int indent = 0) const {
			std::string pad(indent, ' ');
			static const char* kindNames[] = {"PRIMITIVE", "CLASS", "STRUCT", "ARRAY", "FUNCTION", "DYNAMIC"};
			return pad + "Type(" + kindNames[kind] + ")";
		}
	};

	// Primitive types (int, float, etc.)
	struct PrimitiveTypeNode : TypeNode {
		enum Type { INT, FLOAT, DOUBLE, STRING, BOOL, NUMBER, BIGINT, BIGNUMBER, SHORT, LONG, BYTE } type;

		PrimitiveTypeNode(SourceLocation loc, Type t)
				: TypeNode(loc, PRIMITIVE), type(t) {}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			static const char* typeNames[] = {"INT", "FLOAT", "DOUBLE", "STRING",
			                                  "BOOL", "NUMBER", "BIGINT", "BIGNUMBER", "SHORT", "LONG", "BYTE"};
			return pad + "PrimitiveType(" + typeNames[type] + ")";
		}
	};

	// Class/struct types
	struct NamedTypeNode : TypeNode {
		std::string name;

		NamedTypeNode(SourceLocation loc, std::string n)
				: TypeNode(loc, CLASS), name(std::move(n)) {}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			return pad + "NamedType(" + name + ")";
		}
	};

	// Array types
	struct ArrayTypeNode : TypeNode {
		std::unique_ptr<TypeNode> elementType;

		ArrayTypeNode(SourceLocation loc, std::unique_ptr<TypeNode> elemType)
				: TypeNode(loc, ARRAY), elementType(std::move(elemType)) {}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			return pad + "ArrayType\n" +
			       elementType->toString(indent + 2);
		}
	};
}