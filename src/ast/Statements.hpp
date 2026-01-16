#pragma once
#include <utility>
#include <sstream>
#include <vector>
#include "acceptMethods.hpp"
#include "../core/polymorphic.hpp"
#include "ASTNode.hpp"


namespace zenith {
	// Block statements { ... }
	struct BlockNode : StmtNode {
		std::vector<polymorphic<ASTNode>> statements;

		explicit BlockNode(SourceLocation loc,
		                   std::vector<polymorphic<ASTNode>> stmts)
				: statements(std::move(stmts)) {
			this->loc = std::move(loc);
		}

		[[nodiscard]] std::string toString(int indent = 0) const override {
			const std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "Block {\n";
			for (const auto& stmt : statements) {
				ss << stmt->toString(indent + 2) << "\n";
			}
			ss << pad << "}";
			return ss.str();
		}
		ACCEPT_METHODS
	};

	struct ScopeBlockNode : BlockNode {

		explicit ScopeBlockNode(SourceLocation loc,
		                        std::vector<polymorphic<ASTNode>> stmts)
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
		ACCEPT_METHODS
	};

	// If statements
	struct IfNode : StmtNode {
		polymorphic<ExprNode> condition;
		polymorphic<ASTNode> thenBranch;
		polymorphic<ASTNode> elseBranch;

		IfNode(SourceLocation loc, polymorphic<ExprNode> cond,
		       polymorphic<ASTNode> thenBr,
		       polymorphic<ASTNode> elseBr)
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
		ACCEPT_METHODS
	};

	// Loops
	struct WhileNode : StmtNode {
		polymorphic<ExprNode> condition;
		polymorphic<ASTNode> body;

		WhileNode(SourceLocation loc, polymorphic<ExprNode> cond,
		          polymorphic<ASTNode> b)
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
		ACCEPT_METHODS
	};

	struct DoWhileNode : StmtNode {
		polymorphic<ExprNode> condition;
		polymorphic<ASTNode> body;

		DoWhileNode(SourceLocation loc, polymorphic<ExprNode> cond,
		            polymorphic<ASTNode> b)
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
		ACCEPT_METHODS
	};

	struct ForNode : StmtNode {
		polymorphic<ASTNode> initializer;
		polymorphic<ExprNode> condition;
		polymorphic<ExprNode> increment;
		polymorphic<ASTNode> body;

		ForNode(SourceLocation loc, polymorphic<ASTNode> init,
		        polymorphic<ExprNode> cond,
		        polymorphic<ExprNode> incr,
		        polymorphic<ASTNode> b)
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
		ACCEPT_METHODS
	};

	// Unsafe blocks
	struct UnsafeNode : BlockNode {

		explicit UnsafeNode(SourceLocation loc, std::vector<polymorphic<ASTNode>> stmts)
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
		ACCEPT_METHODS
	};
	//Multi-statements
	struct CompoundStmtNode : StmtNode {
		std::vector<polymorphic<StmtNode>> stmts;

		explicit CompoundStmtNode(SourceLocation loc, std::vector<polymorphic<StmtNode>>&& statements ) : stmts(std::move(statements)){
			this->loc = std::move(loc);
		}
		[[nodiscard]] std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "Compound statements\n";
			for(auto &x : stmts)
				ss << x->toString(indent + 2);
			return ss.str();
		}
		ACCEPT_METHODS
	};
}
