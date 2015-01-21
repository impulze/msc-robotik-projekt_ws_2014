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

	struct NeighbourWithDistance
	{
		NeighbourWithDistance(Coord2D const &target, double weight);

		Coord2D target;
		double weight;
	};

	typedef std::map< Coord2D, std::vector<NeighbourWithDistance> > adjacency_list_t;

	// dijkstra
	void DijkstraComputePaths(Coord2D const &source,
	                          adjacency_list_t const &adjacency_list,
	                          std::map<Coord2D, Coord2D> &previous);
	std::list<Coord2D> DijkstraGetShortestPathTo(Coord2D const &vertex,
	                                             std::map<Coord2D, Coord2D> const &previous);

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

	Coord2D orientationPoint = c1;

	return catmullRomImpl(matrix, t, orientationPoint, c1, c2, c3);
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

	Coord2D orientationPoint = c3;

	return catmullRomImpl(matrix, t, c1, c2, c3, orientationPoint);
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

		for (float t = 0.0f; t <= 1.0f; t += 1.0f / steps) {
			if (i == 0) {
				c1 = waypoints[i];
			} else {
				c1 = waypoints[i - 1];
			}

			c2 = waypoints[i];
			c3 = waypoints[i + 1];

			if (i == waypoints.size() - 2) {
				c4 = c3;
			} else {
				c4 = waypoints[i + 2];
			}

			result = catmullRomMiddle(t, c1, c2, c3, c4);
#if 0
			if (i == 0) {
				c1 = waypoints[i];
				c2 = waypoints[i + 1];

				if (i + 2 == waypoints.size()) {
					c3 = c2;
				} else {
					c3 = waypoints[i + 2];
				}

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

				if (i + 2 == waypoints.size()) {
					c4 = c3;
				} else {
					c4 = waypoints[i + 2];
				}

				result = catmullRomMiddle(t, c1, c2, c3, c4);
			}
#endif

			pathPoints.push_back(result);
		}
	}

	return pathPoints;
}

namespace
{

NeighbourWithDistance::NeighbourWithDistance(Coord2D const &target, double weight)
	: target(target),
	  weight(weight)
{
}

void DijkstraComputePaths(Coord2D const &source,
                          adjacency_list_t const &adjacency_list,
                          std::map<Coord2D, Coord2D> &previous)
{
	std::map<Coord2D, double> min_distance;
	std::set< std::pair<double, Coord2D> > vertex_queue;

	min_distance[source] = 0;
	vertex_queue.insert(std::make_pair(0, source));
 
	while (!vertex_queue.empty()) {
		double dist = vertex_queue.begin()->first;
		Coord2D u = vertex_queue.begin()->second;
		vertex_queue.erase(vertex_queue.begin());
 
		// Visit each edge exiting u
		adjacency_list_t::const_iterator alIt = adjacency_list.find(u);

		if (alIt == adjacency_list.end()) {
			continue;
		}

		const std::vector<NeighbourWithDistance> &neighbours = alIt->second;

		for (std::vector<NeighbourWithDistance>::const_iterator neighbour_iter = neighbours.begin();
		     neighbour_iter != neighbours.end();
		     neighbour_iter++) {
			Coord2D v = neighbour_iter->target;
			double weight = neighbour_iter->weight;
			double distance_through_u = dist + weight;

			std::map<Coord2D, double>::iterator it = min_distance.find(v);

			if (it == min_distance.end() || distance_through_u < it->second) {
				if (it != min_distance.end()) {
					vertex_queue.erase(std::make_pair(it->second, v));
				}

				min_distance[v] = distance_through_u;
				previous[v] = u;

				vertex_queue.insert(std::make_pair(distance_through_u, v));
			}
		}
	}
}

std::list<Coord2D> DijkstraGetShortestPathTo(Coord2D const &vertex, std::map<Coord2D, Coord2D> const &previous)
{
	std::list<Coord2D> path;
	Coord2D current = vertex;

	while (true) {
		path.push_front(current);
		std::map<Coord2D, Coord2D>::const_iterator it = previous.find(current);

		if (it != previous.end()) {
			current = it->second;
		} else {
			break;
		}
	}

	return path;
}

}

std::vector<Coord2D> dijkstra(NeighboursMap const &neighbours,
                              Coord2D const &startpoint, Coord2D const &endpoint)
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

	adjacency_list_t adjacency_list;

	for (NeighboursMap::const_iterator it = neighbours.begin(); it != neighbours.end(); it++) {
		Coord2D thisCoord = it->first;

		for (std::set<Coord2D>::const_iterator cit = it->second.begin(); cit != it->second.end(); cit++) {
			Coord2D thatCoord = *cit;

			double xDistance = static_cast<double>(thatCoord.x) - thisCoord.x;
			double yDistance = static_cast<double>(thatCoord.y) - thisCoord.y;
			double distance = std::sqrt(xDistance * xDistance + yDistance * yDistance);

			adjacency_list[thisCoord].push_back(NeighbourWithDistance(thatCoord, distance));
		}
	}

	assert(neighbourToIndexMap.find(startpoint) != neighbourToIndexMap.end());
	assert(neighbourToIndexMap.find(endpoint) != neighbourToIndexMap.end());

	std::map<Coord2D, Coord2D> previous;
	DijkstraComputePaths(startpoint, adjacency_list, previous);
	std::list<Coord2D> path = DijkstraGetShortestPathTo(endpoint, previous);

	std::copy(path.begin(), path.end(), std::back_inserter(generatedPath));

	return generatedPath;
}

Coord2D const &aStarLowestFScore(std::set<Coord2D> const &searchSet, std::map<Coord2D, float> const &fScore)
{
	float lowest = 0;
	std::set<Coord2D>::const_iterator lowestMemberIt;

	assert(searchSet.begin() != searchSet.end());

	for (std::set<Coord2D>::const_iterator searchIt = searchSet.begin(); searchIt != searchSet.end(); searchIt++) {
		std::map<Coord2D, float>::const_iterator candidate = fScore.find(*searchIt);

		assert(candidate != fScore.end());

		if (lowest) {
			if (candidate->second < lowest) {
				lowest = candidate->second;
				lowestMemberIt = searchIt;
			}
		} else {
			lowest = candidate->second;
			lowestMemberIt = searchIt;
		}
	}

	return *lowestMemberIt;
}

float aStarHeuristicCostEstimate(Coord2D const &start, Coord2D const &end)
{
	unsigned int xDist = std::max(start.x, end.x) - std::min(start.x, end.x);
	unsigned int yDist = std::max(start.y, end.y) - std::min(start.y, end.y);

	return std::sqrt(xDist * xDist + yDist * yDist);
}

std::vector<Coord2D> aStarReconstructPath(std::map<Coord2D, Coord2D> &cameFrom, Coord2D const &end)
{
	Coord2D current = end;
	std::vector<Coord2D> path;

	path.push_back(current);

	std::map<Coord2D, Coord2D>::iterator it;

	for (it = cameFrom.find(current); it != cameFrom.end(); it = cameFrom.find(current)) {
		current = it->second;
		cameFrom.erase(it);
		path.push_back(current);
	}

	return path;
}

float aStarDistanceBetween(Coord2D const &start, Coord2D const &end)
{
	unsigned int xDist = std::max(start.x, end.x) - std::min(start.x, end.x);
	unsigned int yDist = std::max(start.y, end.y) - std::min(start.y, end.y);

	return std::sqrt(xDist * xDist + yDist * yDist);
}

std::vector<Coord2D> astar(NeighboursMap const &neighbours,
                           Coord2D const &startpoint, Coord2D const &endpoint)
{
	std::vector<Coord2D> generatedPath;

	std::set<Coord2D> closedSet;
	std::set<Coord2D> openSet;
	std::map<Coord2D, Coord2D> cameFrom;

	std::map<Coord2D, float> gScore;
	std::map<Coord2D, float> fScore;

	openSet.insert(startpoint);

	gScore[startpoint] = 0;
	fScore[startpoint] = 0 + aStarHeuristicCostEstimate(startpoint, endpoint);

	while (!openSet.empty()) {
		Coord2D current = aStarLowestFScore(openSet, fScore);

		if (current == endpoint) {
			return aStarReconstructPath(cameFrom, endpoint);
		}

		openSet.erase(current);
		closedSet.insert(current);

		NeighboursMap::const_iterator neighboursIt = neighbours.find(current);

		assert(neighboursIt != neighbours.end());

		std::set<Coord2D> const &currentNeighbours = neighboursIt->second;

		for (std::set<Coord2D>::const_iterator neighbourIt = currentNeighbours.begin(); neighbourIt != currentNeighbours.end(); neighbourIt++) {
			Coord2D neighbour = *neighbourIt;

			if (closedSet.find(neighbour) != closedSet.end()) {
				continue;
			}

			float tentativeGScore = gScore[current] + aStarDistanceBetween(current, neighbour);

			if (openSet.find(neighbour) == openSet.end() || tentativeGScore < gScore[neighbour]) {
				cameFrom[neighbour] = current;
				gScore[neighbour] = tentativeGScore;
				fScore[neighbour] = gScore[neighbour] + aStarHeuristicCostEstimate(neighbour, endpoint);

				openSet.insert(neighbour);
			}
		}
	}

	assert(false);

	return generatedPath;
}
