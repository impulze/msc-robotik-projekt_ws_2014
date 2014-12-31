#ifndef ROB_ALGO_H_INCLUDED
#define ROB_ALGO_H_INCLUDED

#include "coord.h"

#include <list>
#include <vector>

struct neighbour
{
	neighbour(int target, double weight);

	int target;
	double weight;
};
 
typedef std::vector< std::vector<neighbour> > adjacency_list_t;
 
void DijkstraComputePaths(int source,
                          adjacency_list_t const &adjacency_list,
                          std::vector<double> &min_distance,
                          std::vector<int> &previous);

std::list<int> DijkstraGetShortestPathTo(int vertex, std::vector<int> const &previous);
std::vector< Coord2DTemplate<float> > catmullRom(std::vector<Coord2D> const &waypoints, unsigned int steps);


#endif // ROB_ALGO_H_INCLUDED
