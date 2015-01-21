#ifndef ROB_EDGE_H_INCLUDED
#define ROB_EDGE_H_INCLUDED

#include "coord.h"

struct Edge
{
	Edge(Coord2D const &start, Coord2D const &end);

	Coord2D start;
	Coord2D end;
};

#endif // ROB_EDGE_H_INCLUDED
