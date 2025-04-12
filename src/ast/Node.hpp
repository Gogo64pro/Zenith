#pragma once

#include <cstdlib>
#include <string>

namespace zenith::ast {

struct SourceLocation {
	size_t line;
	size_t column;
	size_t length;
	size_t fileOffset;
};

struct Node {
	virtual ~Node() = default;
	SourceLocation loc;
	virtual std::string toString(int indent = 0) const = 0;
};

struct ExprNode : Node {virtual bool isConstructorCall() const { return false; }};
struct StmtNode : Node {};

} // zenith::ast