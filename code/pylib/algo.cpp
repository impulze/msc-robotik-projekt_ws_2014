#include "algo.h"
#include "edge.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <limits>
#include <list>
#include <set>
#include <utility>

namespace
{
	Coord2DTemplate<float> catmullRomImpl(double matrix[4][4], float t, Coord2D const &c1, Coord2D const &c2, Coord2D const &c3, Coord2D const &c4);
	Coord2DTemplate<float> catmullRomFirst(float t, Coord2D const &c1, Coord2D const &c2, Coord2D const &c3);
	Coord2DTemplate<float> catmullRomMiddle(float t, Coord2D const &c1, Coord2D const &c2, Coord2D const &c3, Coord2D const &c4);
	Coord2DTemplate<float> catmullRomLast(float t, Coord2D const &c1, Coord2D const &c2, Coord2D const &c3);

	const double max_weight = std::numeric_limits<double>::infinity();

	struct neighbour
	{
		neighbour(int target, double weight);

		int target;
		double weight;
	};

	typedef std::vector< std::vector<neighbour> > adjacency_list_t;

	// dijkstra
	void DijkstraComputePaths(int source,
	                          const adjacency_list_t &adjacency_list,
	                          std::vector<double> &min_distance,
	                          std::vector<int> &previous);
	std::list<int> DijkstraGetShortestPathTo(int vertex, const std::vector<int> &previous);

	// a-star
}

namespace
{

Coord2DTemplate<float> catmullRomImpl(double matrix[4][4], float t, Coord2D const &c1, Coord2D const &c2, Coord2D const &c3, Coord2D const &c4)
{
	double first = (1 * matrix[0][0] + t * matrix[1][0] + t * t * matrix[2][0] + t * t * t * matrix[3][0]) / 2;
	double second = (1 * matrix[0][1] + t * matrix[1][1] + t * t * matrix[2][1] + t * t * t * matrix[3][1]) / 2;
	double third = (1 * matrix[0][2] + t * matrix[1][2] + t * t * matrix[2][2] + t * t * t * matrix[3][2]) / 2;
	double fourth = (1 * matrix[0][3] + t * matrix[1][3] + t * t * matrix[2][3] + t * t * t * matrix[3][3]) / 2;

	Coord2DTemplate<float> result;

	result.x = c1.x * first + c2.x * second + c3.x * third + c4.x * fourth;
	result.y = c1.y * first + c2.y * second + c3.y * third + c4.y * fourth;

	return result;
}

Coord2DTemplate<float> catmullRomFirst(float t, Coord2D const &c1, Coord2D const &c2, Coord2D const &c3)
{
	double matrix[4][4] = {
		{ 0, 2, 0, 0 },
		{ 2, 0, 0, 0 },
		{ -4, -5, 6, -1 },
		{ 2, 3, -4, 1 }
	};

	Coord2D orientation(1, 1);

	return catmullRomImpl(matrix, t, orientation, c1, c2, c3);
}

Coord2DTemplate<float> catmullRomMiddle(float t, Coord2D const &c1, Coord2D const &c2, Coord2D const &c3, Coord2D const &c4)
{
	double a = 1;

	double matrix[4][4] = {
		{ 0, 2, 0, 0 },
		{ -a, 0, a, 0 },
		{ 2 * a, -6 + a, 6 - 2 * a, -a },
		{ -a, 4 - a, -4 + a, a }
	};

	return catmullRomImpl(matrix, t, c1, c2, c3, c4);
}

Coord2DTemplate<float> catmullRomLast(float t, Coord2D const &c1, Coord2D const &c2, Coord2D const &c3)
{
	double matrix[4][4] = {
		{ 0, 2, 0, 0 },
		{ -1, 0, 1, 0 },
		{ 2, -6, 4, -2 },
		{ -1, 4, -3, 2 }
	};

	Coord2D orientation(1, 1);

	return catmullRomImpl(matrix, t, c1, c2, c3, orientation);
}

}

std::vector< Coord2DTemplate<float> > catmullRom(std::vector<Coord2D> const &waypoints, unsigned int steps)
{
	std::vector< Coord2DTemplate<float> > pathPoints;

	for (std::vector<Coord2D>::size_type i = 0; i < waypoints.size() - 1; i++) {
		Coord2D c1;
		Coord2D c2;
		Coord2D c3;
		Coord2D c4;
		Coord2DTemplate<float> result;

		for (float t = 0.0f; t < 1.0f; t += 1.0f / steps) {
			if (i == 0) {
				c1 = waypoints[i];
				c2 = waypoints[i + 1];
				c3 = waypoints[i + 2];
				result = catmullRomFirst(t, c1, c2, c3);
			} else if (i == waypoints.size() - 2) {
				c1 = waypoints[i - 1];
				c2 = waypoints[i];
				c3 = waypoints[i + 1];
				result = catmullRomLast(t, c1, c2, c3);
			} else {
				c1 = waypoints[i - 1];
				c2 = waypoints[i];
				c3 = waypoints[i + 1];
				c4 = waypoints[i + 2];
				result = catmullRomMiddle(t, c1, c2, c3, c4);
			}

			pathPoints.push_back(result);
		}
	}

	return pathPoints;
}

namespace
{

neighbour::neighbour(int target, double weight)
	: target(target),
	  weight(weight)
{
}

void DijkstraComputePaths(int source,
                          const adjacency_list_t &adjacency_list,
                          std::vector<double> &min_distance,
                          std::vector<int> &previous)
{
	adjacency_list_t::size_type n = adjacency_list.size();
	min_distance.clear();
	min_distance.resize(n, max_weight);
	min_distance[source] = 0;
	previous.clear();
	previous.resize(n, -1);
	std::set< std::pair<double, int> > vertex_queue;
	vertex_queue.insert(std::make_pair(min_distance[source], source));
 
	while (!vertex_queue.empty()) {
		double dist = vertex_queue.begin()->first;
		int u = vertex_queue.begin()->second;
		vertex_queue.erase(vertex_queue.begin());
 
		// Visit each edge exiting u
		const std::vector<neighbour> &neighbours = adjacency_list[u];

		for (std::vector<neighbour>::const_iterator neighbour_iter = neighbours.begin();
		     neighbour_iter != neighbours.end();
		     neighbour_iter++) {
			int v = neighbour_iter->target;
			double weight = neighbour_iter->weight;
			double distance_through_u = dist + weight;

			if (distance_through_u < min_distance[v]) {
				vertex_queue.erase(std::make_pair(min_distance[v], v));

				min_distance[v] = distance_through_u;
				previous[v] = u;

				vertex_queue.insert(std::make_pair(min_distance[v], v));
			}
		}
	}
}

std::list<int> DijkstraGetShortestPathTo(int vertex, const std::vector<int> &previous)
{
	std::list<int> path;

	for (; vertex != -1; vertex = previous[vertex]) {
		path.push_front(vertex);
	}

	return path;
}

}

std::vector<Coord2D> dijkstra(NeighboursMap const &neighbours,
                              Coord2D const &startpoint, Coord2D const &endpoint,
                              boost::function<bool(Edge const &, double &)> edgeAdder)
{
	std::vector<Coord2D> generatedPath;
	std::map<Coord2D, int> neighbourToIndexMap;

	int i = 0;

	// every coordinate gets an id
	for (NeighboursMap::const_iterator it = neighbours.begin(); it != neighbours.end(); it++) {
		Coord2D coord = it->first;

		if (neighbourToIndexMap.find(coord) == neighbourToIndexMap.end()) {
			neighbourToIndexMap[coord] = i++;
		}
	}

	// check that every neighbour of a coordinate (which is also a coordinate) has an id
	for (NeighboursMap::const_iterator it = neighbours.begin(); it != neighbours.end(); it++) {
		for (std::set<Coord2D>::const_iterator cit = it->second.begin(); cit != it->second.end(); cit++) {
			Coord2D checkCoord = *cit;
			assert(neighbourToIndexMap.find(checkCoord) != neighbourToIndexMap.end());
		}
	}

	adjacency_list_t adjacency_list(neighbours.size());

	for (NeighboursMap::const_iterator it = neighbours.begin(); it != neighbours.end(); it++) {
		Coord2D thisCoord = it->first;

		for (std::set<Coord2D>::const_iterator cit = it->second.begin(); cit != it->second.end(); cit++) {
			Coord2D thatCoord = *cit;

			// now check that the neighbour coordinate can be reached
			Edge checkEdge(thisCoord, thatCoord);
			double distance;

			if (edgeAdder(checkEdge, distance)) {
				adjacency_list[neighbourToIndexMap[thisCoord]].push_back(neighbour(neighbourToIndexMap[thatCoord], distance));
			}
		}
	}

	assert(neighbourToIndexMap.find(startpoint) != neighbourToIndexMap.end());
	assert(neighbourToIndexMap.find(endpoint) != neighbourToIndexMap.end());

	std::vector<double> min_distance;
	std::vector<int> previous;
	DijkstraComputePaths(neighbourToIndexMap[startpoint], adjacency_list, min_distance, previous);
	std::list<int> path = DijkstraGetShortestPathTo(neighbourToIndexMap[endpoint], previous);

	for (std::list<int>::const_iterator it = path.begin(); it != path.end(); it++) {
		int thisIndex = *it;
		std::map<Coord2D, int>::const_iterator found = neighbourToIndexMap.end();

		for (std::map<Coord2D, int>::const_iterator nit = neighbourToIndexMap.begin(); nit != neighbourToIndexMap.end(); nit++) {
			if (nit->second == thisIndex) {
				found = nit;
				break;
			}
		}

		assert(found != neighbourToIndexMap.end());

		generatedPath.push_back(found->first);
	}

	return generatedPath;
}
