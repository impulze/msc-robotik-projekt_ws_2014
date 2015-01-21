#ifndef ROB_COORD_H_INCLUDED
#define ROB_COORD_H_INCLUDED

template <class T = unsigned int>
struct Coord2DTemplate
{
	Coord2DTemplate();
	Coord2DTemplate(T x, T y);

	bool operator<(Coord2DTemplate const &other) const;
	bool operator==(Coord2DTemplate const &other) const;
	bool operator!=(Coord2DTemplate const &other) const;

	T x;
	T y;
};

typedef Coord2DTemplate<> Coord2D;

#include "coord.tcc"

#endif // ROB_COORD_H_INCLUDED
