#pragma once

#include <cstdlib>

namespace zenith {
	struct SourceLocation {
		size_t line;
		size_t column;
		size_t length;
		size_t fileOffset;
		std::string file;
	};

	struct ASTNode {
		virtual ~ASTNode() = default;
		SourceLocation loc;
		virtual std::string toString(int indent = 0) const = 0;
	};

	struct ExprNode : ASTNode {virtual bool isConstructorCall() const { return false; }};
	struct StmtNode : ASTNode {};

} // zenith
