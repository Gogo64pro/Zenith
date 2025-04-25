#include <memory>
#include "../ast/MainNodes.hpp"
#include "../exceptions/ErrorReporter.hpp"

namespace zenith{
	class SemanticAnalyzer{
	public:
		explicit SemanticAnalyzer(ErrorReporter& errorReporter)
				: errorReporter(errorReporter) {}

		void analyze(std::unique_ptr<ProgramNode>& program);
	private:
		ErrorReporter& errorReporter;
	};
}