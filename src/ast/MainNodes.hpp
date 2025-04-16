//
// Created by gogop on 3/28/2025.
//

#pragma once

#include <vector>
#include <memory>
#include <sstream>
#include "Node.hpp"

namespace zenith::ast {

struct ProgramNode : Node {
	std::vector<std::unique_ptr<Node>> declarations;

	explicit ProgramNode(lexer::SourceSpan loc, std::vector<std::unique_ptr<Node>> decls)
			: Node(loc), declarations(std::move(decls)) {}

	std::string toString(int indent = 0) const {
		std::string pad(indent, ' ');
		std::stringstream ss;
		ss << pad << "Program {\n";
		for (const auto& decl : declarations) {
			ss << decl->toString(indent + 2) << "\n";
		}
		ss << pad << "}";
		return ss.str();
	}
};

struct ImportNode : Node {
	std::string path;
	bool isJavaImport;

	ImportNode(lexer::SourceSpan loc, std::string p, bool java)
		: Node(loc), path(std::move(p)), isJavaImport(java) {}

	std::string toString(int indent = 0) const {
		std::string pad(indent, ' ');
		return pad + "Import " + (isJavaImport ? "Java: " : "") + "\"" + path + "\"";
	}
};

} // zenith::ast