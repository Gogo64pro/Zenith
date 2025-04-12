#pragma once

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace zenith::utils {

enum Target {
	native,
	jvm
};

enum GC {
	generational,
	refcounting,
	none
};

struct Flags {
	bool bracesRequired = true;
	Target target = Target::native;
	GC gc = GC::generational;
	std::string inputFile;
};

class ArgumentParser {
public:
	static Flags parse(int argc, char *argv[]) {
		return parse(std::vector<std::string>(argv, argv + argc));
	}

	static Flags parse(const std::vector<std::string> &args) {
		Flags flags;
		bool foundInputFile = false;

		for (size_t i = 1; i < args.size(); ++i) {
			const std::string_view arg = args[i];

			try {
				if (arg == "--braces=optional") {
					flags.bracesRequired = false;
				}
				else if (arg == "--braces=required") {
					flags.bracesRequired = true;
				}
				else if (arg.starts_with("--target=")) {
					const auto value = arg.substr(9);
					if (value == "native") flags.target = Target::native;
					else if (value == "jvm") flags.target = Target::jvm;
					else throw std::runtime_error("Invalid target");
				}
				else if (arg.starts_with("--gc=")) {
					const auto value = arg.substr(5);
					if (value == "generational") flags.gc = GC::generational;
					else if (value == "refcounting") flags.gc = GC::refcounting;
					else if (value == "none") flags.gc = GC::none;
					else throw std::runtime_error("Invalid GC strategy");
				}
				else if (!arg.starts_with("-")) {
					if (!foundInputFile) {
						flags.inputFile = arg;
						foundInputFile = true;
					}
					else {
						throw std::runtime_error("Multiple input files specified");
					}
				}
				else {
					throw std::runtime_error("Unknown option: " + std::string(arg));
				}
			} catch (const std::exception &e) {
				throw std::runtime_error("Error processing argument '" + std::string(arg) + "': " + e.what());
			}
		}

		if (!foundInputFile) {
			throw std::runtime_error("No input file specified");
		}
		return flags;
	}
};

} // zenith::utils