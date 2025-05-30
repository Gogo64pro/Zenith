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
#include "../core/indirect_polymorphic.hpp"

namespace zenith{
	struct AnnotationNode : ASTNode {
		std::string name;
		std::vector<std::pair<std::string, std_P3019_modified::polymorphic<ExprNode>>> arguments;

		AnnotationNode(SourceLocation loc,
		               std::string name,
		               std::vector<std::pair<std::string, std_P3019_modified::polymorphic<ExprNode>>> args)
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
		void accept(Visitor& visitor) override { visitor.visit(*this); }
	};
	struct ErrorNode : ASTNode {
	public:
		ErrorNode(SourceLocation loc){
			this->loc=loc;
		}

		std::string toString(int indent = 0) const override {
			return std::string(indent, ' ') + "<PARSE ERROR>";
		}

		void accept(Visitor& visitor) override { visitor.visit(*this); }

	};
	struct TemplateParameter : ASTNode{
		enum Kind { TYPE, NON_TYPE, TEMPLATE } kind;
		std::string name;
		bool isVariadic = false;

		// For type parameters
		std_P3019_modified::polymorphic<TypeNode> defaultType;

		// For non-type parameters - changed from Token to TypeNode
		std_P3019_modified::polymorphic<TypeNode> type;
		std_P3019_modified::polymorphic<ExprNode> defaultValue;

		// For template template parameters
		std::vector<TemplateParameter> templateParams;

		TemplateParameter(Kind kind,
		                  std::string name,
		                  bool isVariadic,
		                  std::variant<
				                  std::monostate,
				                  std_P3019_modified::polymorphic<TypeNode>, // For TYPE parameters (default type)
				                  std::pair<std_P3019_modified::polymorphic<TypeNode>, std_P3019_modified::polymorphic<ExprNode>>, // For NON_TYPE (type + default)
				                  std::vector<TemplateParameter> // For TEMPLATE parameters
		                  > params = std::monostate{})
				: kind(kind), name(std::move(name)), isVariadic(isVariadic) {

			switch (kind) {
				case TYPE:
					if (auto* type = std::get_if<std_P3019_modified::polymorphic<TypeNode>>(&params)) {
						defaultType = std::move(*type);
					}
					break;
				case NON_TYPE:
					if (auto* pair = std::get_if<std::pair<std_P3019_modified::polymorphic<TypeNode>, std_P3019_modified::polymorphic<ExprNode>>>(&params)) {
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
		std::string toString(int indent = 0) const override {
			std::string result = std::string(indent, ' ') + "TemplateParameter(" + name;
			switch (kind) {
				case TYPE:
					result += ", TYPE";
					if (defaultType) {
						result += ", default=" + defaultType->toString(indent + 2);
					}
					break;
				case NON_TYPE:
					result += ", NON_TYPE";
					if (type) {
						result += ", type=" + type->toString(indent + 2);
					}
					if (defaultValue) {
						result += ", default=" + defaultValue->toString(indent + 2);
					}
					break;
				case TEMPLATE:
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
		void accept(Visitor& visitor) override { visitor.visit(*this); }
	};
	struct TemplateDeclNode : ASTNode {
		std::vector<TemplateParameter> parameters;
		std_P3019_modified::polymorphic<ASTNode> declaration;

		TemplateDeclNode(SourceLocation loc,
		                 std::vector<TemplateParameter> params,
		                 std_P3019_modified::polymorphic<ASTNode> decl)
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
		void accept(Visitor& visitor) override { visitor.visit(*this); }
	};

} // namespace zenith
