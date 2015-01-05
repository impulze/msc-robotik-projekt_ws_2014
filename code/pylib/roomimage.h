#ifndef ROB_ROOMIMAGE_H_INCLUDED
#define ROB_ROOMIMAGE_H_INCLUDED

#include "image.h"
#include "polygon.h"

#include <set>
#include <string>
#include <vector>

class RoomImage
	: public Image
{
public:
	RoomImage(std::string const &filename);
	std::vector<Polygon2D> expandPolygon(std::set<Coord2D> &coords) const;
	void getBorderPolygons(unsigned char distance, std::vector<Polygon2D> &borderPolygons,
	                       std::vector<Polygon2D> &doorPolygons) const;
};

#endif // ROB_ROOMIMAGE_H_INCLUDED
