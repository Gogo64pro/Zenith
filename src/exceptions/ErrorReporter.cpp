#include <fstream>
#include <string>
#include "ErrorReporter.hpp"

namespace zenith{

	void ErrorReporter::report(const SourceLocation &loc, const std::string &message, const errType& errorType) {
		std::string line = getSourceLine(loc);

		errStream << BOLD_TEXT << loc.file << ":" << loc.line << ":" << loc.column << ": "
		  << errorType.second << errorType.first << ": " << RESET_COLOR
		  << message << "\n";

		errStream << "  " << loc.line << " | " << line << "\n";

		errStream << "  " << std::string(std::to_string(loc.line).size(), ' ') << " | ";
		for (size_t i = 0; i < loc.column + std::to_string(loc.line).size() - loc.length; ++i) {
			errStream << " ";
		}
		errStream << errorType.second << "^";
		for (size_t i = 1; i < loc.length; ++i) {
			errStream << "~";
		}
		errStream << RESET_COLOR << '\n';
	}

	std::string ErrorReporter::getSourceLine(const SourceLocation &loc) {
		const auto fileIt = fileLineCache.find(loc.file);
		if (fileIt != fileLineCache.end()) {
			// File is cached, check if we have this line
			const auto& lineCache = fileIt->second;
			if (loc.line > 0 && loc.line <= lineCache.size()) {
				return lineCache[loc.line - 1]; // lines are 1-based
			}
		}

		std::ifstream file(loc.file);
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
				fileLineCache[loc.file] = std::move(newLineCache);
				return currentLine;
			}
		}

		if (!newLineCache.empty()) {
			fileLineCache[loc.file] = std::move(newLineCache);
		}
		return "[line number out of range]";
	}
	void ErrorReporter::clearCache() {
		fileCache.clear();
	}
}