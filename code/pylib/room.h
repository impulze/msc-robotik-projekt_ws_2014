#ifndef ROB_ROOM_H_INCLUDED
#define ROB_ROOM_H_INCLUDED

#include "coord.h"
#include "triangle.h"

#include <set>
#include <string>
#include <vector>

class RoomImage;

class Room
{
public:
	Room(std::string const &filename, unsigned char distance);

	RoomImage const &image() const;

	bool setStartpoint(Coord2D const &coord);
	Coord2D getStartpoint() const;

	bool setEndpoint(Coord2D const &coord);
	Coord2D getEndpoint() const;

	bool insertWaypoint(Coord2D const &coord);
	bool removeWaypoint(std::set<Coord2D>::iterator waypointIterator);
	std::set<Coord2D> const &getWaypoints() const;

	std::vector<Triangle> triangulate() const;
	std::vector<Coord2D> generatePath() const;

private:
	class RoomImpl;
	RoomImpl *p;
};

#endif // ROB_ROOM_H_INCLUDED
