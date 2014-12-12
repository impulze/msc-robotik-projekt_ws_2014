#ifndef ROB_IMAGE_H_INCLUDED
#define ROB_IMAGE_H_INCLUDED

#include <string>
#include <vector>

class Image
{
public:
	enum ImageType {
		IMAGE_TYPE_RGB,
		IMAGE_TYPE_RGBA
	};

	Image(std::string const &filename);

	unsigned int width() const;
	unsigned int height() const;
	std::vector<unsigned char> const &data() const;
	ImageType type() const;

private:
	unsigned int width_;
	unsigned int height_;
	std::vector<unsigned char> data_;
	ImageType type_;
};

#endif // ROB_IMAGE_H_INCLUDED
