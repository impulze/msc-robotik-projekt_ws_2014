#ifndef ROB_ROOMIMAGE_H_INCLUDED
#define ROB_ROOMIMAGE_H_INCLUDED

#include "image.h"
#include "polygon.h"

#include <string>
#include <vector>

class RoomImage
	: public Image
{
public:
	RoomImage(std::string const &filename);

	std::vector<Polygon2D> triangulate(unsigned char distance) const;
};

#endif // ROB_ROOMIMAGE_H_INCLUDED
