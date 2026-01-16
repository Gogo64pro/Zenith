#include "SymbolTable.hpp"
#include <ranges>
#include <sstream>
#include "exceptions/ErrorReporter.hpp"
#include <vector>
namespace zenith{
	//SymbolInfo::SymbolInfo(const Kind k, polymorphic_ref<TypeNode> t, polymorphic_ref<ASTNode> node, const bool isConst, const bool isStatic)
	//		: kind(k), type(t), declarationNode(node), isConst(isConst), isStatic(isStatic) {}

	SymbolInfo::SymbolInfo(const Kind k, TypeNode& t, ASTNode& node, const bool isConst, const bool isStatic)
			: kind(k), type(make_polymorphic_ref(t)), declarationNode(make_polymorphic_ref(node)), isConst(isConst), isStatic(isStatic) {}

	//SymbolInfo::SymbolInfo(const Kind k, polymorphic<TypeNode> t, polymorphic_ref<ASTNode> node, const bool isConst, const bool isStatic)
	//		: kind(k), type(std::move(t)), declarationNode(node), isConst(isConst), isStatic(isStatic) {}

	SymbolInfo::SymbolInfo(const Kind k, polymorphic_variant<TypeNode> t, polymorphic_ref<ASTNode> node, const bool isConst, const bool isStatic)
			: kind(k), type(std::move(t)), declarationNode(node), isConst(isConst), isStatic(isStatic) {}

	SymbolTable::SymbolTable(ErrorReporter& reporter) : errorReporter(reporter) {
		enterScope();
	}

	void SymbolTable::enterScope() {
		scopeStack.emplace_back();
	}

	void SymbolTable::exitScope() {
		if (!scopeStack.empty()) {
			scopeStack.pop_back();
		} else {
			errorReporter.internalError(SourceLocation(), "Exiting non-existent scope.");
		}
	}

	void SymbolTable::declare(const std::string &name, SymbolInfo info) {
		if (scopeStack.empty()) {
			errorReporter.internalError(info.declarationNode ? info.declarationNode->loc : SourceLocation(), "No current scope for declaration");
			return;
		}

		auto& currentScope = scopeStack.back();

		if (auto [it, success] = currentScope.emplace(name, std::move(info)); !success) {
			const auto& existingSymbol = it->second;
			errorReporter.report(
					info.declarationNode ? info.declarationNode->loc : SourceLocation(),
					"Redeclaration of '" + name + "'. Previously declared at line " +
					std::to_string(existingSymbol.declarationNode ? existingSymbol.declarationNode->loc.line : 0)
			);
		}
	}

	polymorphic_ref<SymbolInfo> SymbolTable::lookup(const std::string &name) {
		for (auto & scope : std::ranges::reverse_view(scopeStack)) {
				auto found = scope.find(name);
			if (found != scope.end()) {
				return make_polymorphic_ref(found->second);
			}
		}
		return nullptr;
	}

	const SymbolInfo* SymbolTable::lookupCurrentScope(const std::string& name) {
		if (scopeStack.empty()) return nullptr;
		auto& currentScope = scopeStack.back();
		const auto found = currentScope.find(name);
		if (found != currentScope.end()) {
			return &found->second;
		}
		return nullptr;
	}

	polymorphic_ref<SymbolInfo> SymbolTable::lookup(const std::string &name, const SymbolInfo::Kind kind) {
		for (auto & scope : std::ranges::reverse_view(scopeStack)) {
				auto found = scope.find(name);
			if (found != scope.end()) {
				if(found->second.kind == kind)
					return make_polymorphic_ref(found->second);
			}
		}
		return nullptr;
	}

	std::string SymbolTable::toString(int indent) const {
		const std::string pad(indent, ' ');
		const std::string conv = "main";
		std::stringstream ss;
		ss << pad << "SymbolTable {\n";

		for (const auto& [scopeIndex, scope] : std::views::enumerate(scopeStack)) {
			ss << pad << "  Scope " << scopeIndex << " {\n";

			for (const auto& [name, symbolInfo] : scope) {
				ss << pad << "    Symbol: " << name << "\n";

				ss << pad << "    Kind: ";
				switch (symbolInfo.kind) {
					case SymbolInfo::VARIABLE:       ss << "VARIABLE";       break;
					case SymbolInfo::FUNCTION:       ss << "FUNCTION";       break;
					case SymbolInfo::OBJECT:         ss << "OBJECT";         break;
					case SymbolInfo::ACTOR:          ss << "ACTOR";          break;
					case SymbolInfo::TYPE_ALIAS:     ss << "TYPE_ALIAS";     break;
					case SymbolInfo::TEMPLATE_PARAM: ss << "TEMPLATE_PARAM"; break;
					case SymbolInfo::UNKNOWN:        ss << "UNKNOWN";        break;
				}
				ss << "\n";

				if (symbolInfo.type) {
					ss << pad << "    Type: " << symbolInfo.type->toString(indent + 6) << "\n";
				} else {
					ss << pad << "    Type: <none>\n";
				}

				ss << pad << "    Const: "  << (symbolInfo.isConst  ? "true" : "false") << "\n";
				ss << pad << "    Static: " << (symbolInfo.isStatic ? "true" : "false") << "\n";

				if (symbolInfo.declarationNode) {
					ss << pad << "    Declaration: " /*<< symbolInfo.declarationNode->toString(indent + 6)*/ << "\n";
				} else {
					ss << pad << "    Declaration: <none>\n";
				}
			}

			ss << pad << "  }\n";
		}

		ss << pad << "}";
		return ss.str();
	}


} // namespace zenith