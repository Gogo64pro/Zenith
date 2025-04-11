//
// Created by gogop on 3/30/2025.
//

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <sstream>
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

} // namespace zenith
