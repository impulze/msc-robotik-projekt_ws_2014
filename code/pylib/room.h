#ifndef ROB_ROOM_H_INCLUDED
#define ROB_ROOM_H_INCLUDED

#include "coord.h"
#include "triangle.h"

#include <map>
#include <set>
#include <string>
#include <vector>

class RoomImage;

class Room
{
public:
	typedef std::map< Coord2D, std::set<Coord2D> > NeighboursMap;

	Room(std::string const &filename, unsigned char distance);

	RoomImage const &image() const;

	bool setStartpoint(Coord2D const &coord);
	Coord2D getStartpoint() const;

	bool setEndpoint(Coord2D const &coord);
	Coord2D getEndpoint() const;

	bool insertWaypoint(Coord2D const &coord);
	bool removeWaypoint(Coord2D const &coord);
	std::set<Coord2D> const &getWaypoints() const;

	void triangulate(unsigned char distance);
	std::vector<Triangle> const &getTriangulation() const;

	void generatePath();
	std::vector<Coord2D> const &getGeneratedPath() const;

private:
	class RoomImpl;
	RoomImpl *p;
};

#endif // ROB_ROOM_H_INCLUDED
