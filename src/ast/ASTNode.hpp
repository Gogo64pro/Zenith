#pragma once
#include <string>
export import zenith.sourceLocation;
import zenith.core.polymorphic;
namespace zenith {
	class Visitor;
}
export namespace zenith {
	struct ASTNode {
		virtual ~ASTNode() = default;
		SourceLocation loc;
		[[nodiscard]] virtual std::string toString(int indent = 0) const = 0;
		virtual void accept(Visitor& visitor) = 0;
	};

	struct ExprNode : ASTNode {
		[[nodiscard]] virtual bool isConstructorCall() const { return false; }
	};
	struct StmtNode : ASTNode {};
}