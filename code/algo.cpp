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
	std::vector<Coord2D> reconstructPath(std::map<Coord2D, Coord2D> &cameFrom, Coord2D const &end);
	float distanceBetween(Coord2D const &start, Coord2D const &end);
	float aStarHeuristicCostEstimate(Coord2D const &start, Coord2D const &end);
}

namespace
{

std::vector<Coord2D> reconstructPath(std::map<Coord2D, Coord2D> &cameFrom, Coord2D const &end)
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

float distanceBetween(Coord2D const &start, Coord2D const &end)
{
	unsigned int xDist = static_cast<int>(end.x) - start.x;
	unsigned int yDist = static_cast<int>(end.y) - start.y;

	return std::sqrt(xDist * xDist + yDist * yDist);
}

float aStarHeuristicCostEstimate(Coord2D const &start, Coord2D const &end)
{
	unsigned int xDist = static_cast<int>(end.x) - start.x;
	unsigned int yDist = static_cast<int>(end.y) - start.y;

	return std::sqrt(xDist * xDist + yDist * yDist);
}

}

std::vector< Coord2DTemplate<float> > catmullRom(std::vector<Coord2D> const &waypoints, unsigned int steps)
{
	std::vector< Coord2DTemplate<float> > pathPoints;

	// from https://forum.libcinder.org/topic/creating-catmull-rom-spline-from-the-bspline-class

	for (std::vector<Coord2D>::size_type i = 0; i < waypoints.size() - 1; i++) {
		Coord2D c1;
		Coord2D c2;
		Coord2D c3;
		Coord2D c4;
		Coord2DTemplate<float> result;

		if (i == 0) {
			c1 = waypoints[i];
		} else {
			c1 = waypoints[i - 1];
		}

		c2 = waypoints[i];
		c3 = waypoints[i + 1];

		if (i == waypoints.size() - 2) {
			c4 = waypoints[i + 1];
		} else {
			c4 = waypoints[i + 2];
		}

		for (float t = 0.0f; t <= 1.0f; t += 1.0f / steps) {
			double a = 1;

			double matrix[4][4] = {
				{ 0, 2, 0, 0 },
				{ -a, 0, a, 0 },
				{ 2 * a, -6 + a, 6 - 2 * a, -a },
				{ -a, 4 - a, -4 + a, a }
			};

			double first = (1 * matrix[0][0] + t * matrix[1][0] + t * t * matrix[2][0] + t * t * t * matrix[3][0]) / 2;
			double second = (1 * matrix[0][1] + t * matrix[1][1] + t * t * matrix[2][1] + t * t * t * matrix[3][1]) / 2;
			double third = (1 * matrix[0][2] + t * matrix[1][2] + t * t * matrix[2][2] + t * t * t * matrix[3][2]) / 2;
			double fourth = (1 * matrix[0][3] + t * matrix[1][3] + t * t * matrix[2][3] + t * t * t * matrix[3][3]) / 2;

			result.x = c1.x * first + c2.x * second + c3.x * third + c4.x * fourth;
			result.y = c1.y * first + c2.y * second + c3.y * third + c4.y * fourth;

			pathPoints.push_back(result);
		}
	}

	return pathPoints;
}

std::vector<Coord2D> dijkstra(NeighboursMap const &neighbours,
                              Coord2D const &startpoint, Coord2D const &endpoint)
{
	std::set<Coord2D> closedSet;
	std::set<Coord2D> openSet;
	std::map<Coord2D, Coord2D> cameFrom;

	std::map<Coord2D, float> gScore;

	openSet.insert(startpoint);

	gScore[startpoint] = 0;

	while (!openSet.empty()) {
		Coord2D current;

		float const *lowest = 0;

		for (std::set<Coord2D>::const_iterator si = openSet.begin(); si != openSet.end(); si++) {
			std::map<Coord2D, float>::const_iterator i = gScore.find(*si);

			if (lowest) {
				if (i->second < *lowest) {
					lowest = &(i->second);
					current = i->first;
				}
			} else {
				lowest = &(i->second);
				current = i->first;
			}
		}

		if (current == endpoint) {
			return reconstructPath(cameFrom, endpoint);
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

			assert(gScore.find(current) != gScore.end());

			float tentativeGScore = gScore[current] + distanceBetween(current, neighbour);

			if (openSet.find(neighbour) == openSet.end() || tentativeGScore < gScore[neighbour]) {
				cameFrom[neighbour] = current;
				gScore[neighbour] = tentativeGScore;

				openSet.insert(neighbour);
			}
		}
	}

	// there is no path
	return reconstructPath(cameFrom, endpoint);
}

std::vector<Coord2D> astar(NeighboursMap const &neighbours,
                           Coord2D const &startpoint, Coord2D const &endpoint)
{
	std::set<Coord2D> closedSet;
	std::set<Coord2D> openSet;
	std::map<Coord2D, Coord2D> cameFrom;

	std::map<Coord2D, float> gScore;
	std::map<Coord2D, float> fScore;

	openSet.insert(startpoint);

	gScore[startpoint] = 0;
	fScore[startpoint] = 0 + aStarHeuristicCostEstimate(startpoint, endpoint);

	while (!openSet.empty()) {
		Coord2D current;

		float const *lowest = 0;

		for (std::set<Coord2D>::const_iterator si = openSet.begin(); si != openSet.end(); si++) {
			std::map<Coord2D, float>::const_iterator i = fScore.find(*si);

			if (lowest) {
				if (i->second < *lowest) {
					lowest = &(i->second);
					current = i->first;
				}
			} else {
				lowest = &(i->second);
				current = i->first;
			}
		}

		if (current == endpoint) {
			return reconstructPath(cameFrom, endpoint);
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

			assert(gScore.find(current) != gScore.end());

			float tentativeGScore = gScore[current] + distanceBetween(current, neighbour);

			if (openSet.find(neighbour) == openSet.end() || tentativeGScore < gScore[neighbour]) {
				cameFrom[neighbour] = current;
				gScore[neighbour] = tentativeGScore;
				fScore[neighbour] = gScore[neighbour] + aStarHeuristicCostEstimate(neighbour, endpoint);

				openSet.insert(neighbour);
			}
		}
	}

	// there is no path
	return reconstructPath(cameFrom, endpoint);
}
