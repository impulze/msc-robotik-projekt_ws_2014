#include "gl.h"
#include "image.h"
#include "texture.h"

#include <GL/gl.h>

#include <stdexcept>

#include <stdio.h>

class Texture::TextureImpl
{
public:
	TextureImpl(Image const &image);
	~TextureImpl();

	void bind();

	GLuint handle;
	unsigned int width;
	unsigned int height;
};

Texture::TextureImpl::TextureImpl(Image const &image)
	: width(image.width()),
	  height(image.height())
{
	try {
		glGenTextures(1, &handle);
		glBindTexture(GL_TEXTURE_2D, handle);

		GLsizei glWidth = static_cast<GLsizei>(width);
		GLsizei glHeight = static_cast<GLsizei>(height);
		GLvoid const *data = static_cast<GLvoid const *>(image.data().data());
		GLenum format;

		switch (image.type()) {
			case Image::IMAGE_TYPE_RGB:
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				format = GL_RGB;
				break;

			case Image::IMAGE_TYPE_RGBA:
				format = GL_RGBA;
				break;

			default:
				throw std::runtime_error("Only RGB and RBGA images supported in texture.");
		}

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, glWidth, glHeight, 0, format, GL_UNSIGNED_BYTE, data);
		checkGLError();

		// replace the actual drawing color
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

		// filtering
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	} catch (...) {
		glDeleteTextures(1, &handle);
		throw;
	}
}

Texture::TextureImpl::~TextureImpl()
{
	glDeleteTextures(1, &handle);
}

void Texture::TextureImpl::bind()
{
	glBindTexture(GL_TEXTURE_2D, handle);
}

Texture::Texture(Image const &image)
	: p(new TextureImpl(image))
{
}

Texture::~Texture()
{
	delete p;
}

unsigned int Texture::width() const
{
	return p->width;
}

unsigned int Texture::height() const
{
	return p->height;
}

void Texture::bind() const
{
	p->bind();
}
