#ifndef ROB_COORD_TCC_INCLUDED
#define ROB_COORD_TCC_INCLUDED

#include "coord.h"

template <class T>
Coord2DTemplate<T>::Coord2DTemplate()
	: x(0),
	  y(0)
{
}

template <class T>
Coord2DTemplate<T>::Coord2DTemplate(T x, T y)
	: x(x),
	  y(y)
{
}

template <class T>
bool Coord2DTemplate<T>::operator<(Coord2DTemplate const &other) const
{
	if (x != other.x) {
		return x < other.x;
	}

	return y < other.y;
}

template <class T>
bool Coord2DTemplate<T>::operator==(Coord2DTemplate const &other) const
{
	return x == other.x && y == other.y;
}

template <class T>
bool Coord2DTemplate<T>::operator!=(Coord2DTemplate const &other) const
{
	return !operator==(other);
}

#endif // ROB_COORD_TCC_INCLUDED
