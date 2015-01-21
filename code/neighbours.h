#ifndef ROB_NEIGHBOURS_H_INCLUDED
#define ROB_NEIGHBOURS_H_INCLUDED

#include "coord.h"

#include <map>
#include <set>

typedef std::map< Coord2D, std::set<Coord2D> > NeighboursMap;

#endif // ROB_NEIGHBOURS_H_INCLUDED
