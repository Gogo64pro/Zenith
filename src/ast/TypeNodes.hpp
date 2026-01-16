#pragma once

#include "acceptMethods.hpp"
#include <algorithm>
#include <sstream>
#include <unordered_set>
#include <utility>
#include <vector>
#include "../core/polymorphic_variant.hpp"
#include "ASTNode.hpp"


namespace zenith {
	struct TypeNode : ASTNode {
		enum class Kind { PRIMITIVE, OBJECT, ARRAY, FUNCTION, DYNAMIC, TEMPLATE, ERROR } kind;

		explicit TypeNode(SourceLocation loc, const Kind k) : kind(k) {
			this->loc = std::move(loc);
		}

		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			static const char* kindNames[] = {"PRIMITIVE", "CLASS", "Object", "FUNCTION", "DYNAMIC", "TEMPLATE", "ERROR"};
			return pad + "Type(" + kindNames[static_cast<int>(kind)] + ")";
		}
		[[nodiscard]] virtual bool isDynamic() const {
			return kind== Kind::DYNAMIC;
		}

		ACCEPT_METHODS
	};


	struct PrimitiveTypeNode : TypeNode {
		enum class Type { INT, FLOAT, DOUBLE, STRING, BOOL, NUMBER, BIGINT, BIGNUMBER, SHORT, LONG, BYTE, VOID, NIL } type;

		PrimitiveTypeNode(SourceLocation loc, Type t)
				: TypeNode(std::move(loc), Kind::PRIMITIVE), type(t) {}

		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			static const char* typeNames[] = {"INT", "FLOAT", "DOUBLE", "STRING",
			                                  "BOOL", "NUMBER", "BIGINT", "BIGNUMBER", "SHORT", "LONG", "BYTE", "VOID", "NIL"};
			return pad + "PrimitiveType(" + typeNames[static_cast<int>(type)] + ")";
		}
		[[nodiscard]] bool isDynamic() const override {
			static const std::unordered_set basic_types = {
				Type::INT, Type::FLOAT, Type::DOUBLE, Type::BOOL, Type::SHORT, Type::LONG, Type::BYTE, Type::VOID, Type::NIL
			};

			return !basic_types.contains(type);
		}
		ACCEPT_METHODS
	};

	// Class/struct types
	struct NamedTypeNode : TypeNode {
		std::string name;

		NamedTypeNode(SourceLocation loc, std::string n)
				: TypeNode(std::move(loc), Kind::OBJECT), name(std::move(n)) {}

		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			return pad + "NamedType(" + name + ")";
		}
		ACCEPT_METHODS
	};

	// Array types
	struct ArrayTypeNode : TypeNode {
		polymorphic_variant<TypeNode> elementType;
		polymorphic_variant<ExprNode> sizeExpr;

		ArrayTypeNode(SourceLocation loc, polymorphic_variant<TypeNode> elemType, polymorphic_variant<ExprNode> sizeExpr = nullptr)
				: TypeNode(std::move(loc), Kind::ARRAY), elementType(std::move(elemType)), sizeExpr(std::move(sizeExpr)) {}

		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			return pad + "ArrayType\n" + elementType->toString();
		}
		ACCEPT_METHODS
	};
	struct TemplateTypeNode : TypeNode {
		std::string baseName;
		std::vector<polymorphic_variant<TypeNode>> templateArgs;

		TemplateTypeNode(SourceLocation loc,
		                 std::string baseName,
		                 std::vector<polymorphic_variant<TypeNode>> templateArgs)
				: TypeNode(std::move(loc), Kind::TEMPLATE),
				  baseName(std::move(baseName)),
				  templateArgs(std::move(templateArgs)) {}

		[[nodiscard]] std::string toString(int indent = 0) const override {
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

		[[nodiscard]] bool isDynamic() const override {
			// A template type is dynamic if any of its arguments are dynamic
			return std::ranges::any_of(templateArgs, [](const auto& arg){return arg->isDynamic();});
		}

		ACCEPT_METHODS
	};
	//Node only used in semantic analyzer
	struct FunctionTypeNode : TypeNode{
		std::vector<polymorphic_variant<TypeNode>> parameterTypes;
		polymorphic_variant<TypeNode> returnType;

		FunctionTypeNode(SourceLocation loc,
		                 std::vector<polymorphic_variant<TypeNode>> params,
		                 polymorphic_variant<TypeNode> retType)
				: TypeNode(std::move(loc), Kind::FUNCTION),
				  parameterTypes(std::move(params)),
				  returnType(std::move(retType)) {};

		[[nodiscard]] std::string toString(int indent = 0) const override {
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
		[[nodiscard]] bool isDynamic() const override { return false; }

		ACCEPT_METHODS
	};
}