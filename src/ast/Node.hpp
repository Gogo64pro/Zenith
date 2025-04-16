#pragma once

#include <cstdlib>
#include <string>

#include "../lexer/lexer.hpp"

namespace zenith::ast {

struct SourceLocation {
	size_t line;
	size_t column;
	size_t length;
	size_t fileOffset;
};

struct Node {
	virtual ~Node() = default;
	lexer::SourceSpan loc;
	Node(lexer::SourceSpan loc) : loc(loc) {}
	virtual std::string toString(int indent = 0) const = 0;
};

struct ExprNode : Node {
	using Node::Node;
	virtual bool isConstructorCall() const { return false; }
};

struct StmtNode : Node { using Node::Node; };

} // zenith::ast