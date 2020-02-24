#include <esl/module/Library.h>
#include <mhd4esl/Module.h>

esl::module::Library::GetModule esl__module__library__getModule = &mhd4esl::getModule;
