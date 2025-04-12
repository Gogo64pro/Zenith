#include <fstream>
#include <string>
#include "ErrorReporter.hpp"

namespace zenith {

void ErrorReporter::report(std::string_view file, const ast::SourceLocation &loc, std::string_view message, std::string_view errorType) {
	std::string line = getSourceLine(file, loc);

	// Format the error message
	errStream << "\033[1m" << file << ":" << loc.line << ":" << loc.column << ": "
	          << "\033[1;31m" << errorType << ": \033[0m"
	          << message << "\n";

	// Show the problematic line
	errStream << "  " << loc.line << " | " << line << "\n";

	// Add the underline marker
	errStream << "     | ";
	for (size_t i = 0; i < loc.column + std::to_string(loc.line).size() + 3; ++i) {
		errStream << " ";
	}
	errStream << "\033[1;31m^";
	for (size_t i = 1; i < loc.length; ++i) {
		errStream << "~";
	}
	errStream << "\033[0m\n";
}

std::string ErrorReporter::getSourceLine(std::string_view filepath, const ast::SourceLocation &loc) {
	// First check if we have this file in cache at all
	auto fileIt = fileLineCache.find(filepath);
	if (fileIt != fileLineCache.end()) {
		// File is cached, check if we have this line
		const auto& lineCache = fileIt->second;
		if (loc.line > 0 && loc.line <= lineCache.size()) {
			return lineCache[loc.line - 1]; // lines are 1-based
		}
	}

	// todo: wtf -- don't scan the file again
	// If not, read the file line by line
	std::ifstream file{std::string(filepath)};
	if (!file) {
		return "[could not open file]";
	}

	std::vector<std::string> newLineCache;
	std::string currentLine;
	size_t currentLineNum = 0;

	while (std::getline(file, currentLine)) {
		newLineCache.push_back(currentLine);
		currentLineNum++;

		if (currentLineNum == loc.line) {
			// Cache the lines we've read so far
			fileLineCache.insert({std::string(filepath), std::move(newLineCache)});
			return currentLine;
		}
	}

	// If we get here, the line number was too large
	// Cache whatever lines we did read
	if (!newLineCache.empty()) {
		fileLineCache.insert({std::string(filepath), std::move(newLineCache)});
	}
	return "[line number out of range]";
}

void ErrorReporter::clearCache() {
	fileCache.clear();
}

} // zenith