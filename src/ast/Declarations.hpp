#pragma once

#include <string>
#include <utility>
#include <vector>
#include <memory>
#include <sstream>
#include <algorithm>
#include "../core/ASTNode.hpp"
#include "TypeNodes.hpp"
#include "Statements.hpp"
#include "Other.hpp"
#include "../utils/RemovePadding.hpp"
#include "../utils/small_vector.hpp"
#include "../core/IAnnotatable.hpp"
#include "../core/indirect_polymorphic.hpp"

namespace zenith {
	struct VarDeclNode : StmtNode {
		enum Kind { STATIC, DYNAMIC, CLASS_INIT } kind;
		std::string name;
		std_P3019_modified::polymorphic<TypeNode> type;
		std_P3019_modified::polymorphic<ExprNode> initializer;
		bool isHoisted : 1;
		bool isConst : 1;

		VarDeclNode(SourceLocation loc, Kind k, std::string n,
		            std_P3019_modified::polymorphic<TypeNode> t, std_P3019_modified::polymorphic<ExprNode> i,
		            bool hoisted = false, bool isConst = false)
				: StmtNode(), kind(k), name(std::move(n)), type(std::move(t)),
				  initializer(std::move(i)), isHoisted(hoisted), isConst(isConst) {
			this->loc = std::move(loc);
		}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			static const char* kindNames[] = {"STATIC", "DYNAMIC", "CLASS_INIT"};
			std::stringstream ss;
			ss << pad;
			if (isConst) ss << "CONST ";
			if (isHoisted) ss << "HOIST ";
			ss << kindNames[kind] << " " << name;
			if (type) ss << " : " << removePadUntilNewLine(type->toString(indent+2));
			if (initializer) ss << " = " << removePadUntilNewLine(initializer->toString(indent+2));
			return ss.str();
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }
	};

	struct FunctionDeclNode : ASTNode, IAnnotatable {
		virtual bool isLambda() const { return false; }
		std::string name;
		std::vector<std::pair<std::string, std_P3019_modified::polymorphic<TypeNode>>> params;
		std_P3019_modified::polymorphic<TypeNode> returnType;
		std_P3019_modified::polymorphic<BlockNode> body;
		std::vector<std_P3019_modified::polymorphic<ExprNode>> defaultValues;
		bool isAsync;
		bool usingStructSugar;

		FunctionDeclNode(SourceLocation loc, std::string name,
		                 std::vector<std::pair<std::string, std_P3019_modified::polymorphic<TypeNode>>> params,
		                 std_P3019_modified::polymorphic<TypeNode> returnType, std_P3019_modified::polymorphic<BlockNode> body, bool async, bool structSugar = false, std::vector<std_P3019_modified::polymorphic<AnnotationNode>> ann = {})
				: name(std::move(name)), params(std::move(params)), returnType(std::move(returnType)),
				  body(std::move(body)), isAsync(async), usingStructSugar(structSugar) {
			this->loc = std::move(loc);
			this->annotations = std::move(ann);
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
		void accept(Visitor& visitor) override { visitor.visit(*this); }
	};

	struct MemberDeclNode : ASTNode, IAnnotatable{
		enum Kind : uint8_t { FIELD, METHOD, METHOD_CONSTRUCTOR, MESSAGE_HANDLER};
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
		std_P3019_modified::polymorphic<TypeNode> type;
		small_vector<std::pair<std::string, std_P3019_modified::polymorphic<ExprNode>>, 1> initializers;
		std_P3019_modified::polymorphic<BlockNode> body;

		MemberDeclNode(
				SourceLocation loc,
				Kind kind,
				Access access,
				bool isConst,
				std::string&& name,  // Take ownership of strings
				std_P3019_modified::polymorphic<TypeNode> type,
				std_P3019_modified::polymorphic<ExprNode> initializer = nullptr,  // Unified initializer
				std::vector<std::pair<std::string, std_P3019_modified::polymorphic<ExprNode>>> ctor_inits = {},
				std_P3019_modified::polymorphic<BlockNode> body = nullptr,
				std::vector<std_P3019_modified::polymorphic<AnnotationNode>> ann = {},
				bool isStatic = false
		) : ASTNode(),
		    name(name),  // Implicitly converts to string_view (removed it anyway)
		    type(std::move(type)),
		    body(std::move(body))
		{
			this->loc = std::move(loc);
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
			static const char* kindNames[] = {"FIELD", "METHOD", "METHOD_CONSTRUCTOR", "MESSAGE_HANDLER"};
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
		void accept(Visitor& visitor) override { visitor.visit(*this); }
	};
// Lambda
	struct LambdaNode : FunctionDeclNode{
		bool isLambda() const override { return true; }
		LambdaNode(SourceLocation loc,
		           std::vector<std::pair<std::string, std_P3019_modified::polymorphic<TypeNode>>> params,
		           std_P3019_modified::polymorphic<TypeNode> returnType = nullptr,
		           std_P3019_modified::polymorphic<BlockNode> body = nullptr,
		           bool async = false)
				: FunctionDeclNode(std::move(loc), "", std::move(params), std::move(returnType), std::move(body), async, false, {}) {
			// Additional lambda-specific initialization
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }
	};

	struct OperatorOverloadNode : ASTNode {
		std::string op;  // Store the operator as a string literal (e.g., "+", "==")
		std::vector<std::pair<std::string, std_P3019_modified::polymorphic<TypeNode>>> params;
		std_P3019_modified::polymorphic<TypeNode> returnType;
		std_P3019_modified::polymorphic<BlockNode> body;

		OperatorOverloadNode(SourceLocation loc, std::string op,
		                     std::vector<std::pair<std::string, std_P3019_modified::polymorphic<TypeNode>>> params,
		                     std_P3019_modified::polymorphic<TypeNode> returnType,
		                     std_P3019_modified::polymorphic<BlockNode> body)
				: op(op), params(std::move(params)),
				  returnType(std::move(returnType)), body(std::move(body)) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "operator " << op << "(";
			// Print parameters
			for (size_t i = 0; i < params.size(); ++i) {
				if (i > 0) ss << ", ";
				ss << params[i].first << ": " << params[i].second->toString();
			}
			ss << ") -> " << returnType->toString() << " {\n";
			ss << body->toString(indent + 2) << "\n";
			ss << pad << "}";
			return ss.str();
		}

		static bool isValidOp(std::string op){
			if(op.size()>=4) return false;
			std::string allowedChars = "+-*/%!>=<~";
			return std::all_of(op.begin(), op.end(), [&allowedChars](char c) {
				return allowedChars.find(c) != std::string::npos;
			});
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }
	};
	struct ObjectDeclNode : ASTNode {
		enum class Kind {CLASS, STRUCT, ACTOR} kind;
		std::string name;
		std::string base;
		std::vector<std_P3019_modified::polymorphic<MemberDeclNode>> members;
		std::vector<std_P3019_modified::polymorphic<OperatorOverloadNode>> operators;
		bool autoGettersSetters;

		ObjectDeclNode(SourceLocation loc, Kind kind, std::string name, std::string base,
		               std::vector<std_P3019_modified::polymorphic<MemberDeclNode>> memb,
		               std::vector<std_P3019_modified::polymorphic<OperatorOverloadNode>> ops = {},
		               bool autoGS = true)
				: name(std::move(name)), members(std::move(memb)),
				  base(std::move(base)), operators(std::move(ops)),
				  autoGettersSetters(autoGS) {
			this->loc = std::move(loc);
			this->kind = kind;
		}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << (kind == Kind::CLASS ? "CLASS " :
			              kind == Kind::STRUCT ? "STRUCT " : "ACTOR ")
			   << name;
			if(!base.empty())
				ss << " : " << base;
			ss << " {\n";
			if (autoGettersSetters) {
				ss << pad << "  // Auto-generated getters/setters enabled\n";
			}

			for (const auto& member : members) {
				ss << member->toString(indent + 2) << "\n";
			}

			for (const auto& op : operators) {
				ss << op->toString(indent + 2) << "\n";
			}

			ss << pad << "}";
			return ss.str();
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }
	};

	struct UnionDeclNode : ASTNode{
		std::string name;
		std::vector<std_P3019_modified::polymorphic<TypeNode>> types;
		UnionDeclNode(SourceLocation loc, std::string name, std::vector<std_P3019_modified::polymorphic<TypeNode>> types)
		: name(std::move(name)), types(std::move(types)){
			this->loc = std::move(loc);
		}
		std::string toString(int indent = 0) const override{
			std::string pad(indent,' ');
			std::stringstream ss;
			ss << pad << "UNION" << name << "{\n";
			for(size_t i = 0;i<types.size();++i){
				ss << types[i]->toString(indent+2) << '\n';
				if(i!= types.size()-1 ) ss << ", ";
			}
			ss << "\n}";
			return ss.str();
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }
	};

	class ActorDeclNode : public ObjectDeclNode {
	public:
		explicit ActorDeclNode(
				SourceLocation loc,
				std::string name,
				std::vector<std_P3019_modified::polymorphic<MemberDeclNode>> members,
				std::string baseActor = ""
		) : ObjectDeclNode(std::move(loc), ObjectDeclNode::Kind::ACTOR, std::move(name), std::move(baseActor), std::move(members)) {}
		void accept(Visitor& visitor) override { visitor.visit(*this); }
	};

//	struct StructDeclNode : ASTNode {
//		std::string name;
//		std::vector<std::pair<std::string, std::unique_ptr<TypeNode>>> fields;
//		bool isUnion;
//
//		StructDeclNode(SourceLocation loc, std::string n,
//		               std::vector<std::pair<std::string, std::unique_ptr<TypeNode>>> f,
//		               bool unionFlag)
//				: name(std::move(n)), fields(std::move(f)), isUnion(unionFlag) {
//			this->loc = loc;
//		}
//
//		std::string toString(int indent = 0) const {
//			std::string pad(indent, ' ');
//			std::stringstream ss;
//			ss << pad << (isUnion ? "UNION " : "STRUCT ") << name << " {\n";
//			for (const auto& field : fields) {
//				ss << pad << "  " << field.first;
//				if (field.second) ss << " : " << field.second->toString(indent+2);
//				ss << "\n";
//			}
//			ss << pad << "}";
//			return ss.str();
//		}
//	};

	//Multi-variables
	struct MultiVarDeclNode : StmtNode {
		std::vector<std_P3019_modified::polymorphic<VarDeclNode>> vars;

		explicit MultiVarDeclNode(SourceLocation loc, std::vector<std_P3019_modified::polymorphic<VarDeclNode>>&& vars ) : vars(std::move(vars)){
			loc = std::move(loc);
		}
		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "Multi-vars\n";
			for(auto &x : vars)
				ss << x->toString(indent + 2);
			return ss.str();
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }
	};

}