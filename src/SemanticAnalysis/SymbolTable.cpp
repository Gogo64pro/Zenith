#include "SymbolTable.hpp"

#include <ranges>

namespace zenith{
	SymbolInfo::SymbolInfo(Kind k, std::unique_ptr<TypeNode> t, ASTNode* node, bool isConst, bool isStatic)
			: kind(k), type(std::move(t)), declarationNode(node), isConst(isConst), isStatic(isStatic) {}

	SymbolTable::SymbolTable(ErrorReporter& reporter) : errorReporter(reporter) {
		enterScope(); // Global scope
	}

	void SymbolTable::enterScope() {
		scopeStack.emplace_back();
	}

	void SymbolTable::exitScope() {
		if (!scopeStack.empty()) {
			scopeStack.pop_back();
		} else {
			errorReporter.report(SourceLocation(), "Exiting non-existent scope.", {"Internal error", RED_TEXT});
		}
	}

	bool SymbolTable::declare(const std::string& name, SymbolInfo info) {
		if (scopeStack.empty()) {
			errorReporter.report(info.declarationNode ? info.declarationNode->loc : SourceLocation(), "Internal Error: No current scope for declaration.");
			return false;
		}
		auto& currentScope = scopeStack.back();
		auto [it, success] = currentScope.emplace(name, std::move(info));

		if (!success) {
			const auto& existingSymbol = it->second;
			errorReporter.report(
					info.declarationNode ? info.declarationNode->loc : SourceLocation(),
					"Redeclaration of '" + name + "'. Previously declared at line " +
					std::to_string(existingSymbol.declarationNode ? existingSymbol.declarationNode->loc.line : 0)
			);
			return false;
		}
		return true;
	}

	const SymbolInfo* SymbolTable::lookup(const std::string& name) {
		for (auto & scope : std::ranges::reverse_view(scopeStack)) {
				auto found = scope.find(name);
			if (found != scope.end()) {
				return &found->second;
			}
		}
		return nullptr;
	}

	const SymbolInfo* SymbolTable::lookupCurrentScope(const std::string& name) {
		if (scopeStack.empty()) return nullptr;
		auto& currentScope = scopeStack.back();
		auto found = currentScope.find(name);
		if (found != currentScope.end()) {
			return &found->second;
		}
		return nullptr;
	}

	const SymbolInfo *SymbolTable::lookup(const std::string &name, SymbolInfo::Kind kind) {
		for (auto & scope : std::ranges::reverse_view(scopeStack)) {
				auto found = scope.find(name);
			if (found != scope.end()) {
				if(found->second.kind == kind)
					return &found->second;
			}
		}
		return nullptr;
	}

	std::string SymbolTable::toString(int indent) const {
		std::string pad(indent, ' ');
		std::stringstream ss;
		ss << pad << "SymbolTable {\n";

		// Iterate through scopes (from global to innermost)
		for (size_t scopeIndex = 0; scopeIndex < scopeStack.size(); ++scopeIndex) {
			const auto& scope = scopeStack[scopeIndex];
			ss << pad << "  Scope " << scopeIndex << " {\n";

			// Iterate through symbols in the current scope
			for (const auto& [name, symbolInfo] : scope) {
				ss << pad << "    Symbol: " << name << "\n";
				ss << pad << "      Kind: ";
				switch (symbolInfo.kind) {
					case SymbolInfo::VARIABLE: ss << "VARIABLE"; break;
					case SymbolInfo::FUNCTION: ss << "FUNCTION"; break;
					case SymbolInfo::OBJECT: ss << "OBJECT"; break;
					case SymbolInfo::ACTOR: ss << "ACTOR"; break;
					case SymbolInfo::TYPE_ALIAS: ss << "TYPE_ALIAS"; break;
					case SymbolInfo::TEMPLATE_PARAM: ss << "TEMPLATE_PARAM"; break;
					case SymbolInfo::UNKNOWN: ss << "UNKNOWN"; break;
				}
				ss << "\n";

				// Type
				if (symbolInfo.type) {
					ss << pad << "      Type: " << symbolInfo.type->toString(indent + 6) << "\n";
				} else {
					ss << pad << "      Type: <none>\n";
				}

				// Flags
				ss << pad << "      Const: " << (symbolInfo.isConst ? "true" : "false") << "\n";
				ss << pad << "      Static: " << (symbolInfo.isStatic ? "true" : "false") << "\n";

				// Declaration Node (optional, only if non-null)
				if (symbolInfo.declarationNode) {
					ss << pad << "      Declaration: " << symbolInfo.declarationNode->toString(indent + 6) << "\n";
				} else {
					ss << pad << "      Declaration: <none>\n";
				}
			}

			ss << pad << "  }\n";
		}

		ss << pad << "}";
		return ss.str();

	}


} // namespace zenith