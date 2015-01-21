#include "gl.h"

#include <GL/glu.h>

#include <stdexcept>

void checkGLError()
{
	GLenum glError = glGetError();

	if (glError != GL_NO_ERROR) {
		GLubyte const *gluString = gluErrorString(glError);
		const char *cString = reinterpret_cast<const char *>(gluString);
		throw std::runtime_error(cString);
	}
}
