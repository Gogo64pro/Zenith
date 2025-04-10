//
// Created by gogop on 3/28/2025.
//

#pragma once

#include <memory>
#include <vector>
#include "../core/ASTNode.hpp"

namespace zenith {
	// Block statements { ... }
	struct BlockNode : StmtNode {
		std::vector<std::unique_ptr<ASTNode>> statements;

		explicit BlockNode(SourceLocation loc,
		                   std::vector<std::unique_ptr<ASTNode>> stmts)
				: statements(std::move(stmts)) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const {
			std::string pad(indent, ' ');
			std::stringstream ss;
			ss << pad << "Block {\n";
			for (const auto& stmt : statements) {
				ss << stmt->toString(indent + 2) << "\n";
			}
			ss << pad << "}";
			return ss.str();
		}
	};

	// If statements
	struct IfNode : StmtNode {
		std::unique_ptr<ExprNode> condition;
		std::unique_ptr<ASTNode> thenBranch;
		std::unique_ptr<ASTNode> elseBranch;

		IfNode(SourceLocation loc, std::unique_ptr<ExprNode> cond,
		       std::unique_ptr<ASTNode> thenBr,
		       std::unique_ptr<ASTNode> elseBr)
				: condition(std::move(cond)),
				  thenBranch(std::move(thenBr)),
				  elseBranch(std::move(elseBr)) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const {
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
	};

	// Loops
	struct WhileNode : StmtNode {
		std::unique_ptr<ExprNode> condition;
		std::unique_ptr<ASTNode> body;

		WhileNode(SourceLocation loc, std::unique_ptr<ExprNode> cond,
		          std::unique_ptr<ASTNode> b)
				: condition(std::move(cond)), body(std::move(b)) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const override {
			std::string pad(indent, ' ');
			return pad + "While\n" +
			       condition->toString(indent + 2) + "\n" +
			       pad + "Body:\n" +
			       body->toString(indent + 2);
		}
	};

	struct DoWhileNode : StmtNode {
		std::unique_ptr<ExprNode> condition;
		std::unique_ptr<ASTNode> body;

		DoWhileNode(SourceLocation loc, std::unique_ptr<ExprNode> cond,
		          std::unique_ptr<ASTNode> b)
				: condition(std::move(cond)), body(std::move(b)) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const {
			std::string pad(indent, ' ');
			return pad + "DoWhile\n" +
			       condition->toString(indent + 2) + "\n" +
			       pad + "Body:\n" +
			       body->toString(indent + 2);
		}
	};

	struct ForNode : StmtNode {
		std::unique_ptr<ASTNode> initializer;
		std::unique_ptr<ExprNode> condition;
		std::unique_ptr<ExprNode> increment;
		std::unique_ptr<ASTNode> body;

		ForNode(SourceLocation loc, std::unique_ptr<ASTNode> init,
		        std::unique_ptr<ExprNode> cond,
		        std::unique_ptr<ExprNode> incr,
		        std::unique_ptr<ASTNode> b)
				: initializer(std::move(init)),
				  condition(std::move(cond)),
				  increment(std::move(incr)),
				  body(std::move(b)) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const {
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
	};

	// Unsafe blocks
	struct UnsafeNode : ASTNode {
		std::unique_ptr<BlockNode> body;

		explicit UnsafeNode(SourceLocation loc, std::unique_ptr<BlockNode> b)
				: body(std::move(b)) {
			this->loc = loc;
		}

		std::string toString(int indent = 0) const {
			std::string pad(indent, ' ');
			return pad + "Unsafe\n" + body->toString(indent + 2);
		}
	};
}