#ifndef ROB_ALGO_H_INCLUDED
#define ROB_ALGO_H_INCLUDED

#include "coord.h"
#include "neighbours.h"

#include <boost/function.hpp>

#include <vector>

class Edge;

std::vector< Coord2DTemplate<float> > catmullRom(std::vector<Coord2D> const &waypoints, unsigned int steps);

std::vector<Coord2D> dijkstra(NeighboursMap const &neighbours,
                              Coord2D const &startpoint, Coord2D const &endpoint,
                              boost::function<bool(Edge const &, double &)> edgeAdder);

std::vector<Coord2D> astar(NeighboursMap const &neighbours);

#endif // ROB_ALGO_H_INCLUDED
