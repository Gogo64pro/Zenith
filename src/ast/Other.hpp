//
// Created by gogop on 3/30/2025.
//

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <sstream>
#include <variant>
#include "../core/ASTNode.hpp"

namespace zenith{
	struct AnnotationNode : ASTNode {
		std::string name;
		std::vector<std::pair<std::string, std::unique_ptr<ExprNode>>> arguments;

		AnnotationNode(SourceLocation loc,
		               std::string name,
		               std::vector<std::pair<std::string, std::unique_ptr<ExprNode>>> args)
		: name(std::move(name)), arguments(std::move(args)) {
			this->loc = loc;
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
	};
	struct ErrorNode : ASTNode {
	public:
		ErrorNode(SourceLocation loc){
			this->loc=loc;
		}

		std::string toString(int indent = 0) const override {
			return std::string(indent, ' ') + "<PARSE ERROR>";
		}

	};
	struct TemplateParameter {
		enum Kind { TYPE, NON_TYPE, TEMPLATE } kind;
		std::string name;
		bool isVariadic = false;

		// For type parameters
		std::unique_ptr<TypeNode> defaultType;

		// For non-type parameters - changed from Token to TypeNode
		std::unique_ptr<TypeNode> type;
		std::unique_ptr<ExprNode> defaultValue;

		// For template template parameters
		std::vector<TemplateParameter> templateParams;

		TemplateParameter(Kind kind,
		                  std::string name,
		                  bool isVariadic,
		                  std::variant<
				                  std::monostate,
				                  std::unique_ptr<TypeNode>, // For TYPE parameters (default type)
				                  std::pair<std::unique_ptr<TypeNode>, std::unique_ptr<ExprNode>>, // For NON_TYPE (type + default)
				                  std::vector<TemplateParameter> // For TEMPLATE parameters
		                  > params = std::monostate{})
				: kind(kind), name(std::move(name)), isVariadic(isVariadic) {

			switch (kind) {
				case TYPE:
					if (auto* type = std::get_if<std::unique_ptr<TypeNode>>(&params)) {
						defaultType = std::move(*type);
					}
					break;
				case NON_TYPE:
					if (auto* pair = std::get_if<std::pair<std::unique_ptr<TypeNode>, std::unique_ptr<ExprNode>>>(&params)) {
						type = std::move(pair->first);
						defaultValue = std::move(pair->second);
					}
					break;
				case TEMPLATE:
					if (auto* tparams = std::get_if<std::vector<TemplateParameter>>(&params)) {
						templateParams = std::move(*tparams);
					}
					break;
			}
		}
	};
	struct TemplateDeclNode : ASTNode {
		std::vector<TemplateParameter> parameters;
		std::unique_ptr<ASTNode> declaration;

		TemplateDeclNode(SourceLocation loc,
		                 std::vector<TemplateParameter> params,
		                 std::unique_ptr<ASTNode> decl)
				: parameters(std::move(params)),
				  declaration(std::move(decl)) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "template<";

			bool first = true;
			for (const auto& param : parameters) {
				if (!first) ss << ", ";
				first = false;

				if (param.isVariadic) ss << "...";

				switch (param.kind) {
					case TemplateParameter::TYPE:
						ss << "typename " << param.name;
						if (param.defaultType) {
							ss << " = " << param.defaultType->toString();
						}
						break;
					case TemplateParameter::NON_TYPE:
						ss << param.type->toString() << " " << param.name;
						if (param.defaultValue) {
							ss << " = " << param.defaultValue->toString();
						}
						break;
					case TemplateParameter::TEMPLATE:
						ss << "template<...> typename " << param.name;
						break;
				}
			}

			ss << ">\n";
			ss << declaration->toString(indent);
			return ss.str();
		}
	};

} // namespace zenith
