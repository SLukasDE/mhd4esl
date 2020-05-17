#include <esl/module/Library.h>
#include <mhd4esl/Module.h>

extern "C" esl::module::Module* esl__module__library__getModule(const std::string& moduleName) {
	return &mhd4esl::getModule();
}
