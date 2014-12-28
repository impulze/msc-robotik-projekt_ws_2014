#ifndef ROB_ALGO_H_INCLUDED
#define ROB_ALGO_H_INCLUDED

#include "coord.h"

Coord2DTemplate<float> catmullRomImpl(double matrix[4][4], float t, Coord2D const &c1, Coord2D const &c2, Coord2D const &c3, Coord2D const &c4);
Coord2DTemplate<float> catmullRomFirst(float t, Coord2D const &c1, Coord2D const &c2, Coord2D const &c3);
Coord2DTemplate<float> catmullRom(float t, Coord2D const &c1, Coord2D const &c2, Coord2D const &c3, Coord2D const &c4);
Coord2DTemplate<float> catmullRomLast(float t, Coord2D const &c1, Coord2D const &c2, Coord2D const &c3);

#endif // ROB_ALGO_H_INCLUDED
