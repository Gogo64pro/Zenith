#pragma once

#include <memory>
#include <unordered_set>
#include "../core/ASTNode.hpp"

namespace zenith {
	// Base type node
	struct TypeNode : ASTNode {
		enum Kind { PRIMITIVE, OBJECT, ARRAY, FUNCTION, DYNAMIC, TEMPLATE } kind;

		explicit TypeNode(SourceLocation loc, Kind k) : kind(k) {
			this->loc = loc;
		}

		virtual std::string toString(int indent = 0) const {
			std::string pad(indent, ' ');
			static const char* kindNames[] = {"PRIMITIVE", "CLASS", "Object", "FUNCTION", "DYNAMIC", "TEMPLATE"};
			return pad + "Type(" + kindNames[kind] + ")";
		}
		virtual bool isDynamic() const {
			return kind==DYNAMIC;
		}

		virtual std::unique_ptr<TypeNode> clone() const{
			return std::make_unique<TypeNode>(*this);
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
		bool isDynamic() const override {
			static const std::unordered_set<Type> basic_types = {
					INT, FLOAT, DOUBLE, BOOL, SHORT, LONG, BYTE
			};

			return !basic_types.contains(type);
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
	};

	// Array types
	struct ArrayTypeNode : TypeNode {
		std::unique_ptr<TypeNode> elementType;
		std::unique_ptr<ExprNode> sizeExpr;

		ArrayTypeNode(SourceLocation loc, std::unique_ptr<TypeNode> elemType, std::unique_ptr<ExprNode> sizeExpr = nullptr)
				: TypeNode(loc, ARRAY), elementType(std::move(elemType)), sizeExpr(std::move(sizeExpr)) {}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			return pad + "ArrayType\n" +
			       elementType->toString(indent + 2);
		}
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
		std::unique_ptr<TypeNode> clone() const override {
			std::vector<std::unique_ptr<TypeNode>> clonedArgs;
			clonedArgs.reserve(templateArgs.size());
			for (const auto& arg : templateArgs) {
				clonedArgs.push_back(arg->clone());  // Recursively clone each argument
			}
			return std::make_unique<TemplateTypeNode>(loc, baseName, std::move(clonedArgs));
		}
	};
}