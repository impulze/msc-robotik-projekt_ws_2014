#ifndef ROB_TEXTURE_H_INCLUDED
#define ROB_TEXTURE_H_INCLUDED

class Image;

class Texture
{
public:
	Texture(Image const &image);

	unsigned int width() const;
	unsigned int height() const;
	void bind() const;

private:
	class TextureImpl;
	TextureImpl *p;
};

#endif // ROB_TEXTURE_H_INCLUDED
