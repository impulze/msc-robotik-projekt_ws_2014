#ifndef ROB_ROOM_H_INCLUDED
#define ROB_ROOM_H_INCLUDED

#include "coord.h"
#include "roomimage.h"

#include <set>
#include <string>
#include <vector>

class Room
{
public:
	Room(std::string const &filename);

	RoomImage const &image() const;

	bool setStartpoint(Coord2D const &coord);
	Coord2D getStartpoint() const;

	bool setEndpoint(Coord2D const &coord);
	Coord2D getEndpoint() const;

	bool insertWaypoint(Coord2D const &coord);
	bool removeWaypoint(Coord2D const &coord);
	std::set<Coord2D> const &getWaypoints() const;

	std::vector<Polygon2D> const &convexCCWRoomPolygons() const;
	void recreateConvexCCWRoomPolygons();

private:
	RoomImage image_;
	struct RIMPL;
	RIMPL *p;
};

#endif // ROB_ROOM_H_INCLUDED
