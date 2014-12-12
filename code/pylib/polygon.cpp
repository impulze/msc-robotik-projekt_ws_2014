#include "polygon.h"

void ConvexPolygon2D::insert(Coord2D const &coord)
{
	for (std::vector<Coord2D>::const_iterator it = coords_.begin(); it != coords_.end(); it++) {
		if (*it == coord) {
			return;
		}
	}

	coords_.push_back(coord);
}

size_t ConvexPolygon2D::size() const
{
	return coords_.size();
}

Coord2D const &ConvexPolygon2D::operator[](int index) const
{
	return coords_[index];
}
