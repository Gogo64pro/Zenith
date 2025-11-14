module;
#include <string>
import zenith.core.polymorphic;
export module zenith.ast:ASTNode;
export import zenith.sourceLocation;
namespace zenith {
	class Visitor;
	class PolymorphicVisitor;
}
export namespace zenith {
	struct ASTNode {
		virtual ~ASTNode() = default;
		SourceLocation loc;
		[[nodiscard]] virtual std::string toString(int indent = 0) const = 0;
		virtual void accept(Visitor& visitor) = 0;
		virtual void accept(PolymorphicVisitor& visitor, std_P3019_modified::polymorphic<ASTNode> x) = 0;
	};

	struct ExprNode : ASTNode {
		[[nodiscard]] virtual bool isConstructorCall() const { return false; }
	};
	struct StmtNode : ASTNode {};
}