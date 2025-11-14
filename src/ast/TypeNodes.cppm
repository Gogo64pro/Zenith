module;
#include <algorithm>
#include "acceptMethods.hpp"
#include <utility>
#include <vector>
#include <sstream>
#include <unordered_set>
import zenith.core.polymorphic;
export module zenith.ast:typeNodes;
import :visitor;
import :ASTNode;
import :visitor;


export namespace zenith {
	// Base type node
	struct TypeNode : ASTNode {
		enum Kind { PRIMITIVE, OBJECT, ARRAY, FUNCTION, DYNAMIC, TEMPLATE, ERROR } kind;

		explicit TypeNode(SourceLocation loc, Kind k) : kind(k) {
			this->loc = std::move(loc);
		}

		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			static const char* kindNames[] = {"PRIMITIVE", "CLASS", "Object", "FUNCTION", "DYNAMIC", "TEMPLATE", "ERROR"};
			return pad + "Type(" + kindNames[kind] + ")";
		}
		[[nodiscard]] virtual bool isDynamic() const {
			return kind==DYNAMIC;
		}

		ACCEPT_METHODS
	};


	// Primitive types (int, float, etc.)
	struct PrimitiveTypeNode : TypeNode {
		enum Type { INT, FLOAT, DOUBLE, STRING, BOOL, NUMBER, BIGINT, BIGNUMBER, SHORT, LONG, BYTE, VOID, NIL } type;

		PrimitiveTypeNode(SourceLocation loc, Type t)
				: TypeNode(std::move(loc), PRIMITIVE), type(t) {}

		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			static const char* typeNames[] = {"INT", "FLOAT", "DOUBLE", "STRING",
			                                  "BOOL", "NUMBER", "BIGINT", "BIGNUMBER", "SHORT", "LONG", "BYTE", "VOID", "NIL"};
			return pad + "PrimitiveType(" + typeNames[type] + ")";
		}
		[[nodiscard]] bool isDynamic() const override {
			static const std::unordered_set<Type> basic_types = {
					INT, FLOAT, DOUBLE, BOOL, SHORT, LONG, BYTE, VOID, NIL
			};

			return !basic_types.contains(type);
		}
		ACCEPT_METHODS
	};

	// Class/struct types
	struct NamedTypeNode : TypeNode {
		std::string name;

		NamedTypeNode(SourceLocation loc, std::string n)
				: TypeNode(std::move(loc), OBJECT), name(std::move(n)) {}

		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			return pad + "NamedType(" + name + ")";
		}
		ACCEPT_METHODS
	};

	// Array types
	struct ArrayTypeNode : TypeNode {
		std_P3019_modified::polymorphic<TypeNode> elementType;
		std_P3019_modified::polymorphic<ExprNode> sizeExpr;

		ArrayTypeNode(SourceLocation loc, std_P3019_modified::polymorphic<TypeNode> elemType, std_P3019_modified::polymorphic<ExprNode> sizeExpr = nullptr)
				: TypeNode(std::move(loc), ARRAY), elementType(std::move(elemType)), sizeExpr(std::move(sizeExpr)) {}

		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			return pad + "ArrayType\n" +
			       elementType->toString(indent + 2);
		}
		ACCEPT_METHODS
	};
	struct TemplateTypeNode : TypeNode {
		std::string baseName;
		std::vector<std_P3019_modified::polymorphic<TypeNode>> templateArgs;

		TemplateTypeNode(SourceLocation loc,
		                 std::string baseName,
		                 std::vector<std_P3019_modified::polymorphic<TypeNode>> templateArgs)
				: TypeNode(std::move(loc), TEMPLATE),
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
	struct FunctionTypeNode : TypeNode{
		std::vector<std_P3019_modified::polymorphic<TypeNode>> parameterTypes;
		std_P3019_modified::polymorphic<TypeNode> returnType;

		FunctionTypeNode(SourceLocation loc,
		                 std::vector<std_P3019_modified::polymorphic<TypeNode>> params,
		                 std_P3019_modified::polymorphic<TypeNode> retType)
				: TypeNode(std::move(loc), FUNCTION),
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