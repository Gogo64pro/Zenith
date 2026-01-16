#pragma once
#include <utility>
#include <vector>
#include <sstream>
#include"acceptMethods.hpp"
#include "../core/polymorphic.hpp"
#include "ASTNode.hpp"


namespace zenith {
	struct ProgramNode : ASTNode {
		std::vector<polymorphic<ASTNode>> declarations;

		explicit ProgramNode(SourceLocation loc,
		                     std::vector<polymorphic<ASTNode>> decls)
				: declarations(std::move(decls)) {
			this->loc = std::move(loc);
		}

		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "Program {\n";
			for (const auto& decl : declarations) {
				ss << decl->toString(indent + 2) << "\n";
			}
			ss << pad << "}";
			return ss.str();
		}
		ACCEPT_METHODS
	};

	struct ImportNode : ASTNode {
		std::string path;
		bool isJavaImport;

		ImportNode(SourceLocation loc, std::string p, bool java)
				: path(std::move(p)), isJavaImport(java) {
			this->loc = std::move(loc);
		}

		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			return pad + "Import " + (isJavaImport ? "Java: " : "") + "\"" + path + "\"";
		}
		ACCEPT_METHODS
	};
}
