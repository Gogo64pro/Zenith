#pragma once
#include <string>
#include <utility>
#include <vector>
#include <sstream>
#include "acceptMethods.hpp"
#include "../core/polymorphic.hpp"
#include "ASTNode.hpp"
#include "TypeNodes.hpp"

namespace zenith{
	struct AnnotationNode : ASTNode {
		std::string name;
		std::vector<std::pair<std::string, polymorphic<ExprNode>>> arguments;

		AnnotationNode(SourceLocation loc,
		               std::string name,
		               std::vector<std::pair<std::string, polymorphic<ExprNode>>> args)
				: name(std::move(name)), arguments(std::move(args)) {
			this->loc = std::move(loc);
		}
		std::string toString(int indent = 0) const override{
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "Annotation @" << name;
			if (!arguments.empty()) {
				ss << "(";
				bool first = true;
				for (const auto& argument : arguments) {
					if (!first) {
						ss << ", ";
					}
					ss << argument.first;
					if (!argument.first.empty()) { // Only show '=' if arg has a name
						ss << "=";
					}
					ss << argument.second->toString(indent + 2); // Assuming ExprNode has toString()
					first = false;
				}
				ss << ")";
			}

			return ss.str();
		}
		ACCEPT_METHODS
	};
	struct ErrorNode : ASTNode {
		ErrorNode(const SourceLocation &loc){
			this->loc=loc;
		}

		std::string toString(int indent = 0) const override {
			return std::string(indent, ' ') + "<PARSE ERROR>";
		}

		ACCEPT_METHODS

	};
	struct TemplateParameter : ASTNode{
		enum class Kind { TYPE, NON_TYPE, TEMPLATE } kind;
		std::string name;
		bool isVariadic = false;

		// For type parameters
		polymorphic<TypeNode> defaultType;

		// For non-type parameters - changed from Token to TypeNode
		polymorphic<TypeNode> type;
		polymorphic<ExprNode> defaultValue;

		// For template template parameters
		std::vector<TemplateParameter> templateParams;

		explicit TemplateParameter(std::string name,
					  polymorphic<TypeNode> defaultType = nullptr,
					  bool isVariadic = false)
		: kind(Kind::TYPE), name(std::move(name)), isVariadic(isVariadic), defaultType(std::move(defaultType)) {}

		// Constructor for NON_TYPE parameters
		TemplateParameter(std::string name,
						  polymorphic<TypeNode> type,
						  polymorphic<ExprNode> defaultValue = nullptr,
						  bool isVariadic = false)
			: kind(Kind::NON_TYPE), name(std::move(name)), isVariadic(isVariadic),
			  type(std::move(type)), defaultValue(std::move(defaultValue)) {}

		// Constructor for TEMPLATE parameters
		TemplateParameter(std::string name,
						  std::vector<TemplateParameter> templateParams,
						  bool isVariadic = false)
			: kind(Kind::TEMPLATE), name(std::move(name)), isVariadic(isVariadic),
			  templateParams(std::move(templateParams)) {}
		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string result = std::string(indent, ' ') + "TemplateParameter(" + name;
			switch (kind) {
				case Kind::TYPE:
					result += ", TYPE";
					if (defaultType) {
						result += ", default=" + defaultType->toString(indent + 2);
					}
					break;
				case Kind::NON_TYPE:
					result += ", NON_TYPE";
					if (type) {
						result += ", type=" + type->toString(indent + 2);
					}
					if (defaultValue) {
						result += ", default=" + defaultValue->toString(indent + 2);
					}
					break;
				case Kind::TEMPLATE:
					result += ", TEMPLATE";
					if (!templateParams.empty()) {
						result += ", params=[\n";
						for (const auto& param : templateParams) {
							result += param.toString(indent + 2) + ",\n";
						}
						result += std::string(indent, ' ') + "]";
					}
					break;
			}
			if (isVariadic) {
				result += ", variadic";
			}
			result += ")";
			return result;
		}
		ACCEPT_METHODS
	};
	struct TemplateDeclNode : ASTNode {
		std::vector<TemplateParameter> parameters;
		polymorphic<ASTNode> declaration;

		TemplateDeclNode(SourceLocation loc,
		                 std::vector<TemplateParameter> params,
		                 polymorphic<ASTNode>& decl)
				: parameters(std::move(params)),
				  declaration(std::move(decl)) {
			this->loc = std::move(loc);
		}

		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "template<";

			bool first = true;
			for (const auto& param : parameters) {
				if (!first) ss << ", ";
				first = false;

				if (param.isVariadic) ss << "...";

				switch (param.kind) {
					case TemplateParameter::Kind::TYPE:
						ss << "typename " << param.name;
						if (param.defaultType) {
							ss << " = " << param.defaultType->toString();
						}
						break;
					case TemplateParameter::Kind::NON_TYPE:
						ss << param.type->toString() << " " << param.name;
						if (param.defaultValue) {
							ss << " = " << param.defaultValue->toString();
						}
						break;
					case TemplateParameter::Kind::TEMPLATE:
						ss << "template<...> typename " << param.name;
						break;
				}
			}

			ss << ">\n";
			ss << declaration->toString(indent);
			return ss.str();
		}
		ACCEPT_METHODS
	};

} // namespace zenith
