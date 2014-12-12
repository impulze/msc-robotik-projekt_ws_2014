#include "gl.h"
#include "image.h"
#include "texture.h"

#include <stdexcept>

Texture::Texture(Image const &image)
	: width_(image.width()),
	  height_(image.height())
{
	try {
		glGenTextures(1, &handle_);
		glBindTexture(GL_TEXTURE_2D, handle_);

		GLsizei glWidth = static_cast<GLsizei>(width_);
		GLsizei glHeight = static_cast<GLsizei>(height_);
		GLvoid const *data = static_cast<GLvoid const *>(image.data().data());
		GLenum format;

		switch (image.type()) {
			case Image::IMAGE_TYPE_RGB:
				format = GL_RGB;
				break;

			case Image::IMAGE_TYPE_RGBA:
				format = GL_RGBA;
				break;

			default:
				throw std::runtime_error("Only RGB and RBGA images supported in texture.");
		}

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, glWidth, glHeight, 0, format, GL_UNSIGNED_BYTE, data);
		check_gl_error();

		// replace the actual drawing color
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

		// filtering
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	} catch (...) {
		glDeleteTextures(1, &handle_);
		throw;
	}
}

Texture::~Texture()
{
	glDeleteTextures(1, &handle_);
}

unsigned int Texture::width() const
{
	return width_;
}

unsigned int Texture::height() const
{
	return height_;
}

void Texture::bind() const
{
	glBindTexture(GL_TEXTURE_2D, handle_);
}
