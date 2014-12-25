#ifndef ROB_TRIANGULATION_H_INCLUDED
#define ROB_TRIANGULATION_H_INCLUDED

#include "coord.h"
#include "neighbours.h"
#include "triangle.h"

#include <vector>
#include <set>

class ConstrainedDelaunayTriangulation
{
public:
	ConstrainedDelaunayTriangulation();

	NeighboursMap getNeighbours() const;
	std::vector<Triangle> getTriangulation() const;

	void insert(Coord2D const &coord);
	void remove(Coord2D const &coord);
	std::set<Coord2D> list() const;
	void clear();

	bool inDomain(Coord2D const &coord);
	bool pointIsVertex(Coord2D const &coord);

	void insertConstraints(std::vector<Coord2D> const &points);

private:
	class ConstrainedDelaunayTriangulationImpl;
	ConstrainedDelaunayTriangulationImpl *p;
};

#endif // ROB_TRIANGULATION_H_INCLUDED
