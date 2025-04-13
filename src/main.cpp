#include <iostream>

#include "module.hpp"
#include "utils/mainargs.hpp"

int main(int argc, char *argv[]) try {
	using namespace zenith;

	auto flags = utils::ArgumentParser::parse(argc, argv);
	if (flags.target != utils::Target::native) {
		std::cerr << "Target not set to native\nNot implemented\n";
		return 0;
	}

	Module mod(flags, std::cerr);
	mod.load();
	mod.lex();
	mod.parse();

	return 0;
}
catch (const std::exception &e) {
	std::cerr << "Error: " << e.what() << '\n';
	return 1;
}
catch (...) {
	std::cerr << "unknown error\n";
	return 1;
}