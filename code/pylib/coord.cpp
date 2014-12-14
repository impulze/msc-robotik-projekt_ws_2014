#include "coord.h"

Coord2D::Coord2D()
	: x(0),
	  y(0)
{
}

Coord2D::Coord2D(unsigned int x, unsigned int y)
	: x(x),
	  y(y)
{
}

bool Coord2D::operator<(Coord2D const &other) const
{
	if (x != other.x) {
		return x < other.x;
	}

	return y < other.y;
}

bool Coord2D::operator==(Coord2D const &other) const
{
	return x == other.x && y == other.y;
}

bool Coord2D::operator!=(Coord2D const &other) const
{
	return !operator==(other);
}
