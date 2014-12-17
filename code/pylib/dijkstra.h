#ifndef ROB_DIJKSTRA_H_INCLUDED
#define ROB_DIJKSTRA_H_INCLUDED

#include <list>
#include <vector>

typedef int vertex_t;
typedef double weight_t;
 
struct neighbor {
    vertex_t target;
    weight_t weight;
    neighbor(vertex_t arg_target, weight_t arg_weight);
};
 
typedef std::vector<std::vector<neighbor> > adjacency_list_t;
 
void DijkstraComputePaths(vertex_t source,
                          const adjacency_list_t &adjacency_list,
                          std::vector<weight_t> &min_distance,
                          std::vector<vertex_t> &previous);

std::list<vertex_t> DijkstraGetShortestPathTo(
    vertex_t vertex, const std::vector<vertex_t> &previous);

#endif // ROB_DIJKSTRA_H_INCLUDED
