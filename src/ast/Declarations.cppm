module;
#include "fmt/format.h"
#include "../utils/RemovePadding.hpp"
#include "../utils/Colorize.hpp"
#include "fmt/ranges.h"
#include "utils/small_vector.hpp"
#include "acceptMethods.hpp"
import zenith.core.polymorphic;
export module zenith.ast:declarations;
import :ASTNode;
import :typeNodes;
import :statements;
import :IAnnotatable;
import :other;

export namespace zenith {
	struct VarDeclNode : StmtNode {
		enum Kind { STATIC, DYNAMIC, CLASS_INIT } kind;
		std::string name;
		polymorphic<TypeNode> type;
		polymorphic<ExprNode> initializer;
		bool isHoisted : 1;
		bool isConst : 1;

		VarDeclNode(SourceLocation loc, Kind k, std::string n,
					polymorphic<TypeNode> t, polymorphic<ExprNode> i,
					bool hoisted = false, bool isConst = false)
				: StmtNode(), kind(k), name(std::move(n)), type(std::move(t)),
				  initializer(std::move(i)), isHoisted(hoisted), isConst(isConst) {
			this->loc = std::move(loc);
		}

		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			static const char* kindNames[] = {"STATIC", "DYNAMIC", "CLASS_INIT"};
			return fmt::format("{pad}{orange}{isConst} {isHoisted} {light_purple}{kind} {yellow}{name}{init}{reset}",
							   fmt::arg("pad", std::string(indent, ' ')),
							   fmt::arg("isConst", isConst ? "const" : ""),
							   fmt::arg("isHoisted", isHoisted ? "hoist" : ""),
							   fmt::arg("kind", kindNames[kind]),
							   fmt::arg("name", name),
							   fmt::arg("init", initializer ? fmt::format(" {white}= {init}", fmt::arg("init", removePadUntilNewLine(initializer->toString(indent+2))),fmt::arg("white", CL_WHITE)) : ""),

							   fmt::arg("orange", CL_ORANGE),
							   fmt::arg("yellow", CL_YELLOW),
							   fmt::arg("light_purple", CL_LIGHT_PURPLE),
							   fmt::arg("white", CL_WHITE),
							   fmt::arg("reset", RESET_COLOR)
			);
		}
		ACCEPT_METHODS
	};

	struct FunctionDeclNode : ASTNode, IAnnotatable {
		[[nodiscard]] virtual bool isLambda() const { return false; }
		std::string name;
		std::vector<std::pair<std::string, polymorphic<TypeNode>>> params;
		polymorphic<TypeNode> returnType;
		polymorphic<BlockNode> body;
		std::vector<polymorphic<ExprNode>> defaultValues;
		bool isAsync;
		bool usingStructSugar;

		FunctionDeclNode(SourceLocation loc, std::string name,
		                 std::vector<std::pair<std::string, polymorphic<TypeNode>>> params,
		                 polymorphic<TypeNode> returnType, polymorphic<BlockNode> body, bool async, bool structSugar = false, std::vector<polymorphic<AnnotationNode>> ann = {})
				: name(std::move(name)), params(std::move(params)), returnType(std::move(returnType)),
				  body(std::move(body)), isAsync(async), usingStructSugar(structSugar) {
			this->loc = std::move(loc);
			this->annotations = std::move(ann);
		}

		[[nodiscard]] std::string toString(int indent = 0) const override {
			return fmt::format(
					"{pad} {async}{returnType}{name}{white}{paren_open}{args}{paren_close}{body}",
					fmt::arg("pad", std::string(indent, ' ')),
					fmt::arg("async", isAsync ? fmt::format("{}async{} ", CL_ORANGE, RESET_COLOR) : ""),
					fmt::arg("returnType", returnType ? returnType->toString() + " " : ""),
					fmt::arg("name", fmt::format("{}{}{}",
					                             CL_YELLOW,
					                             isLambda() ? "[LAMBDA]" : name,
					                             RESET_COLOR)),
					fmt::arg("white", CL_WHITE),
					fmt::arg("paren_open", usingStructSugar ? "({" : "("),
					fmt::arg("paren_close", usingStructSugar ? "}) " : ") "),
					fmt::arg("args", [&]() -> std::string {
						if (params.empty()) return "";

						std::vector<std::string> formatted_params;
						formatted_params.reserve(params.size());

						for (size_t i = 0; i < params.size(); ++i) {
							// Start with parameter type (if exists)
							std::string param_part = params[i].second
							                         ? fmt::format("{}{} ", RESET_COLOR, params[i].second->toString())
							                         : "";

							// Add parameter name
							param_part += fmt::format("{}{}{}", CL_WHITE, params[i].first, RESET_COLOR);

							if (i < defaultValues.size() && defaultValues[i]) {
								param_part += fmt::format("{} = {}{}",
								                          CL_WHITE,
								                          defaultValues[i]->toString(indent + 2),
								                          RESET_COLOR);
							}

							formatted_params.push_back(std::move(param_part));
						}

						return fmt::format("{}", fmt::join(formatted_params,
						                                   fmt::format("{}, {}", CL_WHITE, RESET_COLOR)));
					}()),
					fmt::arg("body", removePadUntilNewLine(body->toString(indent + 2))),
					fmt::arg("reset", RESET_COLOR)
			);
		}
		ACCEPT_METHODS
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
		polymorphic<TypeNode> type;
		small_vector<std::pair<std::string, polymorphic<ExprNode>>, 1> initializers;
		polymorphic<BlockNode> body;

		MemberDeclNode(
				SourceLocation loc,
				Kind kind,
				Access access,
				bool isConst,
				std::string&& name,
				polymorphic<TypeNode> type,
				polymorphic<ExprNode> initializer = nullptr,
				std::vector<std::pair<std::string, polymorphic<ExprNode>>> ctor_inits = {},
				polymorphic<BlockNode> body = nullptr,
				std::vector<polymorphic<AnnotationNode>> ann = {},
				bool isStatic = false
		) : ASTNode(),
		    name(name),  // Implicitly converts to string_view (removed it anyway)
		    type(std::move(type)),
		    body(std::move(body)),
		    flags{kind, access, isConst, isStatic, 0}
		{
			this->loc = std::move(loc);


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

		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			static const char* kindNames[] = {"FIELD", "METHOD", "METHOD_CONSTRUCTOR", "MESSAGE_HANDLER"};
			static const char* accessNames[] = {"PUBLIC", "PROTECTED", "PRIVATE", "PRIVATEW", "PROTECTEDW"};

			//fmt::format("{ann}{pad}{orange}{access} {isConst} {isStatic} {kind} {name} {type}");

			std::stringstream ss;
			for (const auto& ann : annotations) {
				ss << pad << ann->toString() << "\n";
			}
			ss << pad << accessNames[flags.access]
			   << (flags.isConst ? " CONST" : "")
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
		ACCEPT_METHODS
	};

	struct LambdaNode : FunctionDeclNode{
		[[nodiscard]] bool isLambda() const override { return true; }
		LambdaNode(SourceLocation loc,
		           std::vector<std::pair<std::string, polymorphic<TypeNode>>> params,
		           polymorphic<TypeNode> returnType = nullptr,
		           polymorphic<BlockNode> body = nullptr,
		           bool async = false)
				: FunctionDeclNode(std::move(loc), "", std::move(params), std::move(returnType), std::move(body), async, false, {}) {
		}
		ACCEPT_METHODS
	};

	struct OperatorOverloadNode : ASTNode {
		std::string op;  // Store the operator as a string literal (e.g., "+", "==")
		std::vector<std::pair<std::string, polymorphic<TypeNode>>> params;
		polymorphic<TypeNode> returnType;
		polymorphic<BlockNode> body;

		OperatorOverloadNode(SourceLocation loc, std::string op,
		                     std::vector<std::pair<std::string, polymorphic<TypeNode>>> params,
		                     polymorphic<TypeNode> returnType,
		                     polymorphic<BlockNode> body)
				: op(std::move(op)), params(std::move(params)),
				  returnType(std::move(returnType)), body(std::move(body)) {
			this->loc = std::move(loc);
		}

		[[nodiscard]] std::string toString(int indent = 0) const override {
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
		ACCEPT_METHODS
	};
	struct ObjectDeclNode : ASTNode {
		enum class Kind {CLASS, STRUCT, ACTOR} kind;
		std::string name;
		std::string base;
		std::vector<polymorphic<MemberDeclNode>> members;
		std::vector<polymorphic<OperatorOverloadNode>> operators;
		bool autoGettersSetters;

		ObjectDeclNode(SourceLocation loc, Kind kind, std::string name, std::string base,
		               std::vector<polymorphic<MemberDeclNode>> memb,
		               std::vector<polymorphic<OperatorOverloadNode>> ops = {},
		               bool autoGS = true)
				: name(std::move(name)), members(std::move(memb)),
				  base(std::move(base)), operators(std::move(ops)),
				  autoGettersSetters(autoGS) {
			this->loc = std::move(loc);
			this->kind = kind;
		}

		[[nodiscard]] std::string toString(int indent = 0) const override {
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
		ACCEPT_METHODS
	};

	struct UnionDeclNode : ASTNode{
		std::string name;
		std::vector<polymorphic<TypeNode>> types;
		UnionDeclNode(SourceLocation loc, std::string name, std::vector<polymorphic<TypeNode>> types)
				: name(std::move(name)), types(std::move(types)){
			this->loc = std::move(loc);
		}
		[[nodiscard]] std::string toString(int indent = 0) const override{
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
		ACCEPT_METHODS
	};

	struct ActorDeclNode : ObjectDeclNode {
		explicit ActorDeclNode(
				SourceLocation loc,
				std::string name,
				std::vector<polymorphic<MemberDeclNode>> members,
				std::string baseActor = ""
		) : ObjectDeclNode(std::move(loc), ObjectDeclNode::Kind::ACTOR, std::move(name), std::move(baseActor), std::move(members)) {}
		ACCEPT_METHODS
	};


	//Multi-variables
	struct MultiVarDeclNode : StmtNode {
		std::vector<polymorphic<VarDeclNode>> vars;

		explicit MultiVarDeclNode(SourceLocation loc, std::vector<polymorphic<VarDeclNode>>&& vars ) : vars(std::move(vars)){
			loc = std::move(loc);
		}
		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "Multi-vars\n";
			for(auto &x : vars)
				ss << x->toString(indent + 2);
			return ss.str();
		}
		ACCEPT_METHODS
	};

}