#pragma once

#include <string>
#include <utility>
#include <vector>
#include <memory>
#include <sstream>
#include "ASTNode.hpp"
#include "TypeNodes.hpp"
#include "Statements.hpp"
#include "Other.hpp"
#include "../utils/RemovePadding.hpp"
#include "../utils/small_vector.hpp"

namespace zenith::ast {
	struct VarDeclNode : StmtNode {
		enum Kind { STATIC, DYNAMIC, CLASS_INIT } kind;
		std::string name;
		std::unique_ptr<TypeNode> type;
		std::unique_ptr<ExprNode> initializer;
		bool isHoisted : 1;
		bool isConst : 1;

		VarDeclNode(SourceLocation loc, Kind k, std::string n,
		            std::unique_ptr<TypeNode> t, std::unique_ptr<ExprNode> i,
		            bool hoisted = false, bool isConst = false)
				: StmtNode(), kind(k), name(std::move(n)), type(std::move(t)),
				  initializer(std::move(i)), isHoisted(hoisted), isConst(isConst) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			static const char* kindNames[] = {"STATIC", "DYNAMIC", "CLASS_INIT"};
			std::stringstream ss;
			if (isConst) ss << "CONST ";
			if (isHoisted) ss << "HOIST ";
			ss << kindNames[kind] << " " << name;
			if (type) ss << " : " << type->toString();
			if (initializer) ss << " = " << initializer->toString();
			return ss.str();
		}
	};

	struct FunctionDeclNode : ASTNode {
		virtual bool isLambda() const { return false; }
		std::string name;
		std::vector<std::pair<std::string, std::unique_ptr<TypeNode>>> params;
		std::unique_ptr<TypeNode> returnType;
		std::unique_ptr<BlockNode> body;
		std::vector<std::unique_ptr<ExprNode>> defaultValues;
		bool isAsync;
		bool usingStructSugar;

		FunctionDeclNode(SourceLocation loc, std::string name,
		                 std::vector<std::pair<std::string, std::unique_ptr<TypeNode>>> params,
		                 std::unique_ptr<TypeNode> returnType, std::unique_ptr<BlockNode> body, bool async, bool structSugar = false)
				: name(std::move(name)), params(std::move(params)), returnType(std::move(returnType)),
				  body(std::move(body)), isAsync(async), usingStructSugar(structSugar) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad;
			if(isAsync) ss << "ASYNC";
			if (returnType) ss << returnType->toString() << " ";
			ss << name << "(";
			if(usingStructSugar) ss << "{";
			for (size_t i = 0; i < params.size(); ++i) {
				if (i > 0) ss << ", ";
				if (params[i].second) ss << params[i].second->toString() << " ";
				ss << params[i].first;
				if (i < defaultValues.size() && defaultValues[i]) {
					ss << " = " << defaultValues[i]->toString(indent+2);
				}
			}
			if(usingStructSugar) ss << "}";
			ss << ") " << removePadUntilNewLine(body->toString(indent+2));
			return ss.str();
		}
	};

	struct MemberDeclNode : ASTNode {
		enum Kind : uint8_t { FIELD, METHOD, METHOD_CONSTRUCTOR };
		enum Access : uint8_t { PUBLIC, PROTECTED, PRIVATE, PRIVATEW, PROTECTEDW };

#pragma pack(push, 1)
		struct Flags {
			Kind kind : 2;
			Access access : 3;
			bool isConst : 1;
			bool isStatic : 1;
			uint8_t reserved : 1; // For future use
		};
#pragma pack(pop)

		Flags flags;
		std::string name;
		std::unique_ptr<TypeNode> type;
		small_vector<std::pair<std::string, std::unique_ptr<ExprNode>>, 1> initializers;
		std::unique_ptr<BlockNode> body;
		small_vector<std::unique_ptr<AnnotationNode>, 2> annotations;

		MemberDeclNode(
				SourceLocation loc,
				Kind kind,
				Access access,
				bool isConst,
				std::string&& name,  // Take ownership of strings
				std::unique_ptr<TypeNode> type,
				std::unique_ptr<ExprNode> initializer = nullptr,  // Unified initializer
				std::vector<std::pair<std::string, std::unique_ptr<ExprNode>>> ctor_inits = {},
				std::unique_ptr<BlockNode> body = nullptr,
				std::vector<std::unique_ptr<AnnotationNode>> ann = {},
				bool isStatic = false
		) : ASTNode(),
		    name(name),  // Implicitly converts to string_view (removed it anyway)
		    type(std::move(type)),
		    body(std::move(body))
		{
			this->loc = loc;
			flags = {kind, access, isConst, isStatic, 0};

			// Handle initializers
			if (initializer)
				initializers.emplace_back("", std::move(initializer));

			for (auto& init : ctor_inits) {
				initializers.emplace_back(init.first, std::move(init.second));
			}

			// Move annotations
			annotations.reserve(ann.size());
			for (auto& a : ann) {
				annotations.push_back(std::move(a));
			}
		}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			static const char* kindNames[] = {"FIELD", "METHOD", "METHOD_CONSTRUCTOR"};
			static const char* accessNames[] = {"PUBLIC", "PROTECTED", "PRIVATE", "PRIVATEW", "PROTECTEDW"};

			std::stringstream ss;
			for (const auto& ann : annotations) {
				ss << pad << ann->toString() << "\n";
			}
			ss << pad << accessNames[flags.access]
			   << (flags.isConst ? " CONST " : " ")
			   << (flags.isStatic ? " STATIC " : " ")
			   << kindNames[flags.kind] << " " << name;
			if (type) ss << " : " << type->toString();
			for (size_t i = 0; i < initializers.size(); ++i) {
				if (i > 0) ss << ", ";
				ss << initializers[i].first;
				if (!initializers[i].first.empty()) ss << " = ";
				ss << initializers[i].second->toString();
			}
			if (body) {
				ss << " " << removePadUntilNewLine(body->toString(indent));
			} else if (flags.kind == FIELD) {
				ss << ";";
			}
			return ss.str();
		}
	};
// Lambda
	struct LambdaNode : FunctionDeclNode{
		bool isLambda() const override { return true; }
		LambdaNode(SourceLocation loc,
		           std::vector<std::pair<std::string, std::unique_ptr<TypeNode>>> params,
		           std::unique_ptr<TypeNode> returnType = nullptr,
		           std::unique_ptr<BlockNode> body = nullptr,
		           bool async = false)
				: FunctionDeclNode(std::move(loc), "", std::move(params), std::move(returnType), std::move(body), async, false) {
			// Additional lambda-specific initialization
		}
	};


	struct ClassDeclNode : ASTNode {
		std::string name;
		std::string base;
		std::vector<std::unique_ptr<MemberDeclNode>> members;

		ClassDeclNode(SourceLocation loc, std::string name, std::string base,
		              std::vector<std::unique_ptr<MemberDeclNode>> memb)
				: name(std::move(name)), members(std::move(memb)), base(std::move(base)) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "CLASS " << name << " {\n";
			for (const auto& member : members) {
				ss << member->toString(indent + 2) << "\n";
			}
			ss << pad << "}";
			return ss.str();
		}
	};

	struct StructDeclNode : ASTNode {
		std::string name;
		std::vector<std::pair<std::string, std::unique_ptr<TypeNode>>> fields;
		bool isUnion;

		StructDeclNode(SourceLocation loc, std::string n,
		               std::vector<std::pair<std::string, std::unique_ptr<TypeNode>>> f,
		               bool unionFlag)
				: name(std::move(n)), fields(std::move(f)), isUnion(unionFlag) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << (isUnion ? "UNION " : "STRUCT ") << name << " {\n";
			for (const auto& field : fields) {
				ss << pad << "  " << field.first;
				if (field.second) ss << " : " << field.second->toString(indent+2);
				ss << "\n";
			}
			ss << pad << "}";
			return ss.str();
		}
	};


}