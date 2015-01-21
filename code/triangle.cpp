#include "triangle.h"

Coord2D &Triangle::operator[](int index)
{
	return coords[index];
}

Coord2D const &Triangle::operator[](int index) const
{
	return coords[index];
}
