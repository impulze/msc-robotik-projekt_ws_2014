#ifndef ROB_ROOM_H_INCLUDED
#define ROB_ROOM_H_INCLUDED

#include "coord.h"
#include "edge.h"
#include "neighbours.h"
#include "triangle.h"

#include <set>
#include <string>
#include <vector>

class QTextEdit;
class QXmlStreamReader;
class QXmlStreamWriter;
class RoomImage;
class Stats;

class Room
{
public:
	enum Algorithm
	{
		Dijkstra,
		AStar
	};

	Room(std::string const &filename, unsigned char distance, Stats *stats, QTextEdit *statusText, QTextEdit *helpText);
	~Room();

	RoomImage const &image() const;

	bool setStartpoint(Coord2D const &coord);
	Coord2D getStartpoint() const;

	bool setEndpoint(Coord2D const &coord);
	Coord2D getEndpoint() const;

	void setNodes(int amount);

	bool insertWaypoint(Coord2D const &coord);
	bool removeWaypoint(Coord2D const &coord);
	void clearWaypoints();
	bool hasWaypoint(Coord2D const &coord) const;
	std::set<Coord2D> const &getWaypoints() const;

	void setAlgorithm(Algorithm algorithm);
	Algorithm getAlgorithm() const;

	NeighboursMap getNeighbours() const;
	std::vector< std::vector<Edge> > getEdges() const;
	bool pointInside(float x, float y) const;
	bool intersectsEdges(Edge const &checkEdge) const;
	std::vector<Triangle> getTriangulation() const;
	std::vector<Triangle> getRoomTriangulation() const;
	std::vector<Coord2D> generatePath() const;

	bool loadProject(QXmlStreamReader *reader);
	bool saveProject(QXmlStreamWriter *writer) const;

private:
	class RoomImpl;
	RoomImpl *p;
};

#endif // ROB_ROOM_H_INCLUDED
