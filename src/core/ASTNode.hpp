#pragma once

#include <cstdlib>
#include <memory>
#include "../visitor/Visitor.hpp"
#include "polymorphic.hpp"

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
		virtual void accept(Visitor& visitor) = 0;
		virtual void accept(PolymorphicVisitor& visitor, const std_P3019_modified::polymorphic<ASTNode> &x) = 0;
	};

	struct ExprNode : ASTNode {virtual bool isConstructorCall() const { return false; }};
	struct StmtNode : ASTNode {};


} // zenith
