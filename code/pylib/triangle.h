#ifndef ROB_TRIANGLE_H_INCLUDED
#define ROB_TRIANGLE_H_INCLUDED

#include "coord.h"

struct Triangle
{
	Coord2D &operator[](int index);
	Coord2D const &operator[](int index) const;

	Coord2D coords[3];
};

#endif // ROB_TRIANGLE_H_INCLUDED
