#ifndef ROB_TRIANGULATION_H_INCLUDED
#define ROB_TRIANGULATION_H_INCLUDED

#include "coord.h"
#include "edge.h"
#include "neighbours.h"
#include "triangle.h"

#include <vector>
#include <set>

class DelaunayTriangulation
{
public:
	DelaunayTriangulation();
	~DelaunayTriangulation();

	NeighboursMap getNeighbours() const;
	std::vector<Triangle> getTriangulation() const;

	void insert(Coord2D const &coord);
	void remove(Coord2D const &coord);
	std::set<Coord2D> list() const;
	void clear();

	// check before with new algorithm
	//bool inDomain(Coord2D const &coord);
	bool pointIsVertex(Coord2D const &coord);

private:
	class DelaunayTriangulationImpl;
	DelaunayTriangulationImpl *p;
};

class ConstrainedDelaunayTriangulation
{
public:
	ConstrainedDelaunayTriangulation();
	~ConstrainedDelaunayTriangulation();

	NeighboursMap getNeighbours() const;
	std::vector<Triangle> getTriangulation() const;

	void insert(Coord2D const &coord);
	void remove(Coord2D const &coord);
	std::set<Coord2D> list() const;
	void clear();

	bool inDomain(float x, float y) const;
	bool pointIsVertex(Coord2D const &coord) const;

	void insertConstraints(std::vector<Coord2D> const &points);
	std::vector<Edge> getConstrainedEdges() const;

private:
	class ConstrainedDelaunayTriangulationImpl;
	ConstrainedDelaunayTriangulationImpl *p;
};

#endif // ROB_TRIANGULATION_H_INCLUDED
