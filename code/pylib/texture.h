#ifndef ROB_TEXTURE_H_INCLUDED
#define ROB_TEXTURE_H_INCLUDED

#include <GL/gl.h>

class Image;

class Texture
{
public:
	Texture(Image const &image);
	~Texture();

	unsigned int width() const;
	unsigned int height() const;
	void bind() const;

private:
	GLuint handle_;
	unsigned int width_;
	unsigned int height_;
};

#endif // ROB_TEXTURE_H_INCLUDED
