#include <fstream>
#include <string>
#include "ErrorReporter.hpp"

namespace zenith {

struct Rep {
	std::size_t n;
	char ch;

	friend std::ostream& operator<<(std::ostream& out, Rep r) {
		while (r.n--)
			out.put(r.ch);
		return out;
	}
};

template <std::unsigned_integral T>
constexpr T digit10count(T v) noexcept {
	static_assert(sizeof(T) <= 8, "type not supported");

	if constexpr (sizeof(T) >= 8) {
		if (v >= 10000000000000000000u) { return 20; }
		if (v >= 1000000000000000000  ) { return 19; }
		if (v >= 100000000000000000   ) { return 18; }
		if (v >= 10000000000000000    ) { return 17; }
		if (v >= 1000000000000000     ) { return 16; }
		if (v >= 100000000000000      ) { return 15; }
		if (v >= 10000000000000       ) { return 14; }
		if (v >= 1000000000000        ) { return 13; }
		if (v >= 100000000000         ) { return 12; }
		if (v >= 10000000000          ) { return 11; }
	}

	if constexpr (sizeof(T) >= 4) {
		if (v >= 1000000000) { return 10; }
		if (v >= 100000000 ) { return 9; }
		if (v >= 10000000  ) { return 8; }
		if (v >= 1000000   ) { return 7; }
		if (v >= 100000    ) { return 6; }
	}

	if constexpr (sizeof(T) >= 2) {
		if (v >= 10000) { return 5; }
		if (v >= 1000 ) { return 4; }
	}

	if (v >= 100) { return 3; }
	if (v >= 10 ) { return 2; }
	return 1;
}

void ErrorReporter::report(std::string_view file, const ast::SourceLocation &loc, std::string_view message, std::string_view errorType) {
	std::string line = getSourceLine(file, loc);

	// Format the error message
	errStream << "\033[1m" << file << ":" << loc.line << ":" << loc.column << ": "
	          << "\033[1;31m" << errorType << ": \033[0m"
	          << message << "\n";

	// Show the problematic line
	errStream << "  " << loc.line << " | " << line << "\n";

	// Add the underline marker
	errStream << "  " << Rep(digit10count(loc.line), ' ')
		<< " | "
		<< Rep(std::max(1uz, loc.column) - 1, ' ') << "\033[1;31m^" << Rep(std::max(1uz, loc.length) - 1, '~') << "\033[0m\n";
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