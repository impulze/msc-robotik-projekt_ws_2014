#ifndef ROB_ROOM_H_INCLUDED
#define ROB_ROOM_H_INCLUDED

#include "coord.h"
#include "edge.h"
#include "neighbours.h"
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
	bool removeWaypoint(Coord2D const &coord);
	void clearWaypoints();
	bool hasWaypoint(Coord2D const &coord) const;
	std::set<Coord2D> getWaypoints() const;

	NeighboursMap getNeighbours() const;
	std::vector< std::vector<Edge> > getEdges() const;
	bool intersectsEdges(Edge const &checkEdge) const;
	std::vector<Triangle> getTriangulation() const;
	std::vector<Triangle> getRoomTriangulation() const;
	std::vector<Coord2D> generatePath() const;

private:
	class RoomImpl;
	RoomImpl *p;
};

#endif // ROB_ROOM_H_INCLUDED
