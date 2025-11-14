module;
#include "../exceptions/ErrorReporter.hpp"
import zenith.ast;
import zenith.core.polymorphic;
export module zenith.semantic:symbolTable;
export namespace zenith{
	struct SymbolInfo {
		enum Kind {
			VARIABLE, FUNCTION, OBJECT, ACTOR, TYPE_ALIAS, TEMPLATE_PARAM, UNKNOWN
		} kind = UNKNOWN;
		std_P3019_modified::polymorphic<TypeNode> type = nullptr;
		std_P3019_modified::polymorphic<ASTNode> declarationNode = nullptr;
		bool isConst = false;
		bool isStatic = false;

		SymbolInfo(Kind k, std_P3019_modified::polymorphic<TypeNode> t, std_P3019_modified::polymorphic<ASTNode> node, bool isConst = false, bool isStatic = false);

		SymbolInfo() = default;

		SymbolInfo(SymbolInfo&& other) noexcept
				: kind(other.kind),
				  type(std::move(other.type)),
				  declarationNode(std::move(other.declarationNode)),
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
				type = std::move(other.type);
				declarationNode = std::move(other.declarationNode);
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

	class SymbolTable {
	private:
		std::vector<Scope> scopeStack;
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