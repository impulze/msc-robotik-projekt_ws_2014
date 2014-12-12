#ifndef ROB_POLYGON_H_INCLUDED
#define ROB_POLYGON_H_INCLUDED

#include "coord.h"

#include <cstdlib>
#include <vector>

class ConvexPolygon2D
{
public:
	void insert(Coord2D const &coord);
	std::size_t size() const;
	Coord2D const &operator[](int index) const;

private:
	std::vector<Coord2D> coords_;
};

#endif // ROB_POLYGON_H_INCLUDED
