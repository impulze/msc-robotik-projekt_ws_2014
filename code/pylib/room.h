#ifndef ROB_ROOM_H_INCLUDED
#define ROB_ROOM_H_INCLUDED

#include "coord.h"
#include "roomimage.h"

#include <string>
#include <vector>

class Room
{
public:
	Room(std::string const &filename);

	RoomImage const &image() const;

	bool checkWaypoint(Coord2D const &coord) const;
	void insertWaypoint(Coord2D const &coord);
	void removeWaypoint(Coord2D const &coord);

	std::vector<Polygon2D> const &convexCCWRoomPolygons() const;
	void recreateConvexCCWRoomPolygons();

private:
	RoomImage image_;
	struct RIMPL;
	RIMPL *p;
	std::vector<Polygon2D> convexCCWRoomPolygons_;
};

#endif // ROB_ROOM_H_INCLUDED
