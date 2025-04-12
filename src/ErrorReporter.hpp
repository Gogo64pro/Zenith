#pragma once

#include <ostream>
#include <unordered_map>
#include <vector>
#include "ast/Node.hpp"
#include "utils/hash.hpp"

namespace zenith {

class ErrorReporter {
private:
	std::ostream& errStream;
	StringHashMap<std::string> fileCache;
	StringHashMap<std::vector<std::string>> fileLineCache;
	std::string getSourceLine(std::string_view file, const ast::SourceLocation& loc);
public:
	explicit ErrorReporter(std::ostream& errStream) : errStream(errStream) {}
	void report(std::string_view file, const ast::SourceLocation& loc,std::string_view message,std::string_view errorType = "error");
	void clearCache();
};

} // zenith