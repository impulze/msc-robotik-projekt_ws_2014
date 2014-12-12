#include "il.h"

#include <IL/ilu.h>

#include <stdexcept>

void check_il_error()
{
	ILenum ilError = ilGetError();

	if (ilError != IL_NO_ERROR) {
		const char *cString = iluErrorString(ilError);
		throw std::runtime_error(cString);
	}
}
