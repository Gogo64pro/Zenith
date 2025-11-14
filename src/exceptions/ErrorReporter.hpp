#pragma once

#include <ostream>
#include <unordered_map>
#include <vector>
#include "../utils/Colorize.hpp"
import zenith.ast;

namespace zenith{
	class ErrorReporter{
		typedef std::pair<std::string, std::string> errType;
	private:
		std::ostream& errStream;
		std::unordered_map<std::string, std::string> fileCache;
		std::unordered_map<std::string, std::vector<std::string>> fileLineCache;
		std::string getSourceLine(const SourceLocation& loc);
	public:
		explicit ErrorReporter(std::ostream& errStream) : errStream(errStream) {}
		void report(const SourceLocation& loc,const std::string& message,const errType& errorType = {"error", RED_TEXT});
		void clearCache();
	};
}