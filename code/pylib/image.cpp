#include "il.h"
#include "image.h"

#include <IL/il.h>
#include <IL/ilu.h>

#include <stdexcept>

Image::Image(std::string const &filename)
{
	ilInit();
	check_il_error();

	ILuint handle;
	ilGenImages(1, &handle);

	try {
		ilBindImage(handle);
		ilLoad(IL_PNG, filename.c_str());
		check_il_error();

		ILenum format;

		switch (ilGetInteger(IL_IMAGE_FORMAT)) {
			case IL_RGB:
				format = IL_RGB;
				type_ = IMAGE_TYPE_RGB;
				break;

			case IL_RGBA:
				format = IL_RGBA;
				type_ = IMAGE_TYPE_RGBA;
				break;

			default:
				throw std::runtime_error("Only RGB and RBGA images supported.");
		}

		// possibly always convert and accept all formats?
		//ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE);
		//check_il_error();

		ILubyte *data = ilGetData();
		width_ = static_cast<unsigned int>(ilGetInteger(IL_IMAGE_WIDTH));
		height_ = static_cast<unsigned int>(ilGetInteger(IL_IMAGE_HEIGHT));

		if (format == IL_RGB) {
			data_.reserve(width_ * height_ * 3);
		} else {
			data_.reserve(width_ * height_ * 4);
		}

		for (unsigned int y = 0; y < height_; y++) {
			for (unsigned int x = 0; x < width_; x++) {
				unsigned char *dataPointer;

				if (format == IL_RGB) {
					dataPointer = data + (y * width_ + x) * 3;
				} else {
					dataPointer = data + (y * width_ + x) * 4;
				}

				data_.push_back(dataPointer[0]);
				data_.push_back(dataPointer[1]);
				data_.push_back(dataPointer[2]);

				if (format == IL_RGBA) {
					data_.push_back(dataPointer[3]);
				}
			}
		}

		ilDeleteImages(1, &handle);
	} catch (...) {
		ilDeleteImages(1, &handle);
		throw;
	}
}

unsigned int Image::width() const
{
	return static_cast<unsigned int>(width_);
}

unsigned int Image::height() const
{
	return static_cast<unsigned int>(height_);
}

std::vector<unsigned char> const &Image::data() const
{
	return data_;
}

Image::ImageType Image::type() const
{
	return type_;
}
