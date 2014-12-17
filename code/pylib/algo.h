#ifndef ROB_ALGO_H_INCLUDED
#define ROB_ALGO_H_INCLUDED

#include "coord.h"

Coord2D catmullRomImpl(double matrix[4][4], float t, Coord2D const &c1, Coord2D const &c2, Coord2D const &c3, Coord2D const &c4);
Coord2D catmullRomFirst(float t, Coord2D const &c1, Coord2D const &c2, Coord2D const &c3);
Coord2D catmullRom(float t, Coord2D const &c1, Coord2D const &c2, Coord2D const &c3, Coord2D const &c4);
Coord2D catmullRomLast(float t, Coord2D const &c1, Coord2D const &c2, Coord2D const &c3);

#endif // ROB_ALGO_H_INCLUDED
