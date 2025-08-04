//
// Created by gogop on 8/4/2025.
//

export module zenith.ast.ASTNode;

#include "../visitor/Visitor.hpp"

export namespace zenith {
	struct SourceLocation {
		size_t line;
		size_t column;
		size_t length;
		[[maybe_unused]] size_t fileOffset; // Could delete soon
		std::string file;
	};

	struct ASTNode {
		virtual ~ASTNode() = default;
		SourceLocation loc;
		[[nodiscard]] virtual std::string toString(int indent = 0) const = 0;
		virtual void accept(Visitor& visitor) = 0;
		virtual void accept(PolymorphicVisitor& visitor, std_P3019_modified::polymorphic<ASTNode> x) = 0;
	};

	struct ExprNode : ASTNode {[[nodiscard]] virtual bool isConstructorCall() const { return false; }};
	struct StmtNode : ASTNode {};


} // zenith
