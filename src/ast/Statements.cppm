//
// Created by gogop on 8/4/2025.
//

//
// Created by gogop on 3/28/2025.
//

#include <memory>
#include <utility>
#include <sstream>
#include <vector>
#include "../core/indirect_polymorphic.hpp"
export module zenith.ast.statements;
import zenith.ast.ASTNode;

export namespace zenith {
	// Block statements { ... }
	struct BlockNode : StmtNode {
		std::vector<std_P3019_modified::polymorphic<ASTNode>> statements;

		explicit BlockNode(SourceLocation loc,
		                   std::vector<std_P3019_modified::polymorphic<ASTNode>> stmts)
				: statements(std::move(stmts)) {
			this->loc = std::move(loc);
		}

		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "Block {\n";
			for (const auto& stmt : statements) {
				ss << stmt->toString(indent + 2) << "\n";
			}
			ss << pad << "}";
			return ss.str();
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }
		void accept(PolymorphicVisitor &visitor, std_P3019_modified::polymorphic<ASTNode> x) override {visitor.visit(std::move(x).unchecked_cast<std::remove_pointer_t<decltype(this)>>());}
	};

	struct ScopeBlockNode : BlockNode {

		explicit ScopeBlockNode(SourceLocation loc,
		                        std::vector<std_P3019_modified::polymorphic<ASTNode>> stmts)
				: BlockNode(std::move(loc), std::move(stmts)) {}

		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "Scope Block {\n";
			for (const auto& stmt : statements) {
				ss << stmt->toString(indent + 2) << "\n";
			}
			ss << pad << "}";
			return ss.str();
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }
		void accept(PolymorphicVisitor &visitor, std_P3019_modified::polymorphic<ASTNode> x) override {visitor.visit(std::move(x).unchecked_cast<std::remove_pointer_t<decltype(this)>>());}
	};

	// If statements
	struct IfNode : StmtNode {
		std_P3019_modified::polymorphic<ExprNode> condition;
		std_P3019_modified::polymorphic<ASTNode> thenBranch;
		std_P3019_modified::polymorphic<ASTNode> elseBranch;

		IfNode(SourceLocation loc, std_P3019_modified::polymorphic<ExprNode> cond,
		       std_P3019_modified::polymorphic<ASTNode> thenBr,
		       std_P3019_modified::polymorphic<ASTNode> elseBr)
				: condition(std::move(cond)),
				  thenBranch(std::move(thenBr)),
				  elseBranch(std::move(elseBr)) {
			this->loc = std::move(loc);
		}

		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "If\n"
			   << condition->toString(indent + 2) << "\n"
			   << pad << "Then:\n"
			   << thenBranch->toString(indent + 2);
			if (elseBranch) {
				ss << "\n" << pad << "Else:\n"
				   << elseBranch->toString(indent + 2);
			}
			return ss.str();
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }
		void accept(PolymorphicVisitor &visitor, std_P3019_modified::polymorphic<ASTNode> x) override {visitor.visit(std::move(x).unchecked_cast<std::remove_pointer_t<decltype(this)>>());}
	};

	// Loops
	struct WhileNode : StmtNode {
		std_P3019_modified::polymorphic<ExprNode> condition;
		std_P3019_modified::polymorphic<ASTNode> body;

		WhileNode(SourceLocation loc, std_P3019_modified::polymorphic<ExprNode> cond,
		          std_P3019_modified::polymorphic<ASTNode> b)
				: condition(std::move(cond)), body(std::move(b)) {
			this->loc = std::move(loc);
		}

		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			return pad + "While\n" +
			       condition->toString(indent + 2) + "\n" +
			       pad + "Body:\n" +
			       body->toString(indent + 2);
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }
		void accept(PolymorphicVisitor &visitor, std_P3019_modified::polymorphic<ASTNode> x) override {visitor.visit(std::move(x).unchecked_cast<std::remove_pointer_t<decltype(this)>>());}
	};

	struct DoWhileNode : StmtNode {
		std_P3019_modified::polymorphic<ExprNode> condition;
		std_P3019_modified::polymorphic<ASTNode> body;

		DoWhileNode(SourceLocation loc, std_P3019_modified::polymorphic<ExprNode> cond,
		            std_P3019_modified::polymorphic<ASTNode> b)
				: condition(std::move(cond)), body(std::move(b)) {
			this->loc = std::move(loc);
		}

		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			return pad + "DoWhile\n" +
			       condition->toString(indent + 2) + "\n" +
			       pad + "Body:\n" +
			       body->toString(indent + 2);
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }
		void accept(PolymorphicVisitor &visitor, std_P3019_modified::polymorphic<ASTNode> x) override {visitor.visit(std::move(x).unchecked_cast<std::remove_pointer_t<decltype(this)>>());}
	};

	struct ForNode : StmtNode {
		std_P3019_modified::polymorphic<ASTNode> initializer;
		std_P3019_modified::polymorphic<ExprNode> condition;
		std_P3019_modified::polymorphic<ExprNode> increment;
		std_P3019_modified::polymorphic<ASTNode> body;

		ForNode(SourceLocation loc, std_P3019_modified::polymorphic<ASTNode> init,
		        std_P3019_modified::polymorphic<ExprNode> cond,
		        std_P3019_modified::polymorphic<ExprNode> incr,
		        std_P3019_modified::polymorphic<ASTNode> b)
				: initializer(std::move(init)),
				  condition(std::move(cond)),
				  increment(std::move(incr)),
				  body(std::move(b)) {
			this->loc = std::move(loc);
		}

		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "For\n";
			if (initializer) {
				ss << pad << "Init:\n" << initializer->toString(indent + 2) << "\n";
			}
			ss << pad << "Cond:\n" << condition->toString(indent + 2) << "\n"
			   << pad << "Incr:\n" << increment->toString(indent + 2) << "\n"
			   << pad << "Body:\n" << body->toString(indent + 2);
			return ss.str();
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }
		void accept(PolymorphicVisitor &visitor, std_P3019_modified::polymorphic<ASTNode> x) override {visitor.visit(std::move(x).unchecked_cast<std::remove_pointer_t<decltype(this)>>());}
	};

	// Unsafe blocks
	struct UnsafeNode : BlockNode {

		explicit UnsafeNode(SourceLocation loc, std::vector<std_P3019_modified::polymorphic<ASTNode>> stmts)
				: BlockNode(std::move(loc), std::move(stmts)) {}

		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "Unsafe {\n";
			for (const auto& stmt : statements) {
				ss << stmt->toString(indent + 2) << "\n";
			}
			ss << pad << "}";
			return ss.str();
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }
		void accept(PolymorphicVisitor &visitor, std_P3019_modified::polymorphic<ASTNode> x) override {visitor.visit(std::move(x).unchecked_cast<std::remove_pointer_t<decltype(this)>>());}
	};
	//Multi-statements
	struct CompoundStmtNode : StmtNode {
		std::vector<std_P3019_modified::polymorphic<StmtNode>> stmts;

		explicit CompoundStmtNode(SourceLocation loc, std::vector<std_P3019_modified::polymorphic<StmtNode>>&& statements ) : stmts(std::move(statements)){
			loc = std::move(loc);
		}
		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "Compound statements\n";
			for(auto &x : stmts)
				ss << x->toString(indent + 2);
			return ss.str();
		}
		void accept(Visitor& visitor) override { visitor.visit(*this); }
		void accept(PolymorphicVisitor &visitor, std_P3019_modified::polymorphic<ASTNode> x) override {visitor.visit(std::move(x).unchecked_cast<std::remove_pointer_t<decltype(this)>>());}
	};
}
