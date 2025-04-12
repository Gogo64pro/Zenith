#pragma once

#include <ostream>
#include <unordered_map>
#include <vector>
#include "ast/Node.hpp"

namespace zenith {

class ErrorReporter {
private:
	std::ostream& errStream;
	std::unordered_map<std::string, std::string> fileCache;
	std::unordered_map<std::string, std::vector<std::string>> fileLineCache;
	std::string getSourceLine(const ast::SourceLocation& loc);
public:
	explicit ErrorReporter(std::ostream& errStream) : errStream(errStream) {}
	void report(const ast::SourceLocation& loc,std::string_view message,std::string_view errorType = "error");
	void clearCache();
};

} // zenith