#ifndef ROB_ROOMIMAGE_H_INCLUDED
#define ROB_ROOMIMAGE_H_INCLUDED

#include "image.h"
#include "polygon.h"

#include <vector>

class RoomImage
	: public Image
{
public:
	RoomImage(std::string const &filename);

	typedef std::vector<ConvexPolygon2D> ConvexCCWRoomPolygons;

	bool checkWaypoint(Coord2D const &coord) const;
	void insertWaypoint(Coord2D const &coord);
	void removeWaypoint(Coord2D const &coord);

	ConvexCCWRoomPolygons const &convexCCWRoomPolygons() const;
	void recreateConvexCCWRoomPolygons();

private:
	struct RIPIMPL;
	RIPIMPL *p;
	ConvexCCWRoomPolygons roomBoundaries_;
	ConvexCCWRoomPolygons convexCCWRoomPolygons_;
};

#endif // ROB_ROOMIMAGE_H_INCLUDED
