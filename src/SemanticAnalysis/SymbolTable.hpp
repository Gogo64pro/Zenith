#pragma once

#include <memory>
#include "../ast/TypeNodes.hpp"
#include "../exceptions/ErrorReporter.hpp"

namespace zenith{
	struct SymbolInfo {
		enum Kind {
			VARIABLE, FUNCTION, OBJECT, ACTOR, TYPE_ALIAS, TEMPLATE_PARAM, UNKNOWN
		} kind = UNKNOWN;
		TypeNode* type = nullptr;
		ASTNode* declarationNode = nullptr;
		bool isConst = false;
		bool isStatic = false;

		SymbolInfo(Kind k, TypeNode* t, ASTNode* node, bool isConst = false, bool isStatic = false);

		SymbolInfo() = default;

		SymbolInfo(SymbolInfo&& other) noexcept
				: kind(other.kind),
				  type(other.type),
				  declarationNode(other.declarationNode),
				  isConst(other.isConst),
				  isStatic(other.isStatic) {
			other.kind = UNKNOWN;
			other.declarationNode = nullptr;
			other.isConst = false;
			other.isStatic = false;
		}
		SymbolInfo& operator=(SymbolInfo&& other) noexcept {
			if (this != &other) {
				kind = other.kind;
				type = other.type;
				declarationNode = other.declarationNode;
				isConst = other.isConst;
				isStatic = other.isStatic;
				other.kind = UNKNOWN;
				other.declarationNode = nullptr;
				other.isConst = false;
				other.isStatic = false;
			}
			return *this;
		}
		SymbolInfo(const SymbolInfo&) = delete;
		SymbolInfo& operator=(const SymbolInfo&) = delete;
	};

	using Scope = std::unordered_map<std::string, SymbolInfo>;
	struct ScopeF {
		Scope symbols;
		std::vector<std::unique_ptr<TypeNode>> dynamicNodes;
	};

	class SymbolTable {
	private:
		std::vector<ScopeF> scopeStack;
		ErrorReporter& errorReporter;

	public:
		explicit SymbolTable(ErrorReporter& reporter);

		void enterScope();

		void exitScope();

		bool declare(const std::string& name, SymbolInfo info);

		const SymbolInfo* lookup(const std::string& name);
		const SymbolInfo* lookup(const std::string& name, SymbolInfo::Kind kind);
		const SymbolInfo* lookupCurrentScope(const std::string& name);

		[[nodiscard]] std::string toString(int indent = 0) const;
	};
} // namespace zenith