//
// Created by gogop on 3/28/2025.
//

#pragma once

#include <utility>
#include <vector>
#include <memory>
#include <sstream>
#include "../core/ASTNode.hpp"
#include "../core/indirect_polymorphic.hpp"

namespace zenith {
	struct ProgramNode : ASTNode {
		std::vector<std_P3019_modified::polymorphic<ASTNode>> declarations;

		explicit ProgramNode(SourceLocation loc,
		                     std::vector<std_P3019_modified::polymorphic<ASTNode>> decls)
				: declarations(std::move(decls)) {
			this->loc = std::move(loc);
		}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "Program {\n";
			for (const auto& decl : declarations) {
				ss << decl->toString(indent + 2) << "\n";
			}
			ss << pad << "}";
			return ss.str();
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }
	};

	struct ImportNode : ASTNode {
		std::string path;
		bool isJavaImport;

		ImportNode(SourceLocation loc, std::string p, bool java)
				: path(std::move(p)), isJavaImport(java) {
			this->loc = std::move(loc);
		}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			return pad + "Import " + (isJavaImport ? "Java: " : "") + "\"" + path + "\"";
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }
	};
}