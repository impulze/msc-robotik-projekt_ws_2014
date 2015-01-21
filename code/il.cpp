#include "il.h"

#include <IL/ilu.h>

#include <stdexcept>

void checkILError()
{
	ILenum ilError = ilGetError();

	if (ilError != IL_NO_ERROR) {
		const char *cString = iluErrorString(ilError);
		throw std::runtime_error(cString);
	}
}
