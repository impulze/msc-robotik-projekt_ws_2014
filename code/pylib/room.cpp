#include "algo.h"
#include "room.h"
#include "roomimage.h"
#include "triangulation.h"

#include <set>

#include <cassert>
#include <cmath>
#include <cstdio>

struct Room::RoomImpl
{
	RoomImpl(std::string const &filename, unsigned char distance)
		: image(new RoomImage(filename))
	{
		bytes = image->data().data();
		width = image->width();
		height = image->height();
		stride = image->type() == Image::IMAGE_TYPE_RGB ? 3 : 4;
		this->distance = distance;

		// this array below distinguishes between big room (first) and holes
		std::vector<Polygon2D> const &borderPolygons = image->getBorderPolygons(distance);

		// this array doesn't distinguish between big room and holes
		//std::vector<Edge> constraints = roomTriangulation.getConstrainedEdges();

		for (std::vector<Polygon2D>::const_iterator it = borderPolygons.begin();
		     it != borderPolygons.end();
		     it++) {
			std::vector<Coord2D> points;

			for (std::size_t i = 0; i < it->size(); i++) {
				Coord2D coord = (*it)[i];
				points.push_back(coord);
			}

			Coord2D coord = (*it)[0];
			points.push_back(coord);

			roomTriangulation.insertConstraints(points);
		}


		for (std::vector<Polygon2D>::const_iterator it = borderPolygons.begin();
		     it != borderPolygons.end();
		     it++) {
			std::vector<Edge> polygonEdges;
			std::vector<Coord2D>::const_iterator coordIterator = it->begin();
			std::vector<Coord2D>::const_iterator lastCoordIterator = coordIterator;

			++coordIterator;

			while (coordIterator != it->end()) {
				Edge edge(*lastCoordIterator, *coordIterator);
				polygonEdges.push_back(edge);
				lastCoordIterator = coordIterator;
				++coordIterator;
			}

			if (it->begin() != it->end()) {
				Edge lastEdge(*lastCoordIterator, *(it->begin()));
				polygonEdges.push_back(lastEdge);
			}

			edges.push_back(polygonEdges);
		}

		reinitializeTriangulation();
	}

	RoomImage *image;
	unsigned char const *bytes;
	unsigned int width;
	unsigned int height;
	unsigned char stride;
	unsigned char distance;
	DelaunayTriangulation triangulation;
	ConstrainedDelaunayTriangulation roomTriangulation;
	Coord2D startpoint;
	Coord2D endpoint;
	std::vector< std::vector<Edge> > edges;

	bool insert(Coord2D const &coord)
	{
		if (coord == startpoint || coord == endpoint) {
			std::fprintf(stderr, "Waypoint (%d/%d) is startpoint or endpoint, can't insert.\n", coord.x, coord.y);
			return false;
		}

		if (triangulation.pointIsVertex(coord)) {
			std::fprintf(stderr, "Waypoint (%d/%d) already inserted, can't insert.\n", coord.x, coord.y);
			return false;
		}

		if (!roomTriangulation.inDomain(coord.x, coord.y)) {
			std::fprintf(stderr, "Waypoint (%d/%d) outside domain, can't insert.\n", coord.x, coord.y);
			return false;
		}

		triangulation.insert(coord);
		assert(triangulation.pointIsVertex(coord));

		return true;
	}

	bool remove(Coord2D const &coord)
	{
		if (coord == startpoint || coord == endpoint) {
			std::fprintf(stderr, "Waypoint (%d/%d) is startpoint or endpoint, can't remove.\n", coord.x, coord.y);
			return false;
		}

		if (!triangulation.pointIsVertex(coord)) {
			std::fprintf(stderr, "Waypoint (%d/%d) not inserted, can't remove.\n", coord.x, coord.y);
			return false;
		}

		triangulation.remove(coord);
		assert(!triangulation.pointIsVertex(coord));

		return true;
	}

	bool setStartpoint(Coord2D const &coord)
	{
		if (startpoint == coord) {
			return true;
		}

		if (endpoint == coord) {
			std::fprintf(stderr, "Startpoint (%d/%d) is endpoint, can't insert.\n", coord.x, coord.y);
			return false;
		}

		if (!roomTriangulation.inDomain(coord.x, coord.y)) {
			std::fprintf(stderr, "Startpoint (%d/%d) outside domain, can't insert.\n", coord.x, coord.y);
			return false;
		}

		if (startpoint != Coord2D(0, 0)) {
			assert(triangulation.pointIsVertex(startpoint));
			triangulation.remove(startpoint);
		}

		assert(!triangulation.pointIsVertex(startpoint));

		triangulation.insert(coord);
		startpoint = coord;

		return true;
	}

	bool setEndpoint(Coord2D const &coord)
	{
		if (endpoint == coord) {
			return true;
		}

		if (startpoint == coord) {
			std::fprintf(stderr, "Endpoint (%d/%d) is startpoint, can't insert.\n", coord.x, coord.y);
			return false;
		}

		if (!roomTriangulation.inDomain(coord.x, coord.y)) {
			std::fprintf(stderr, "Endpoint (%d/%d) outside domain, can't insert.\n", coord.x, coord.y);
			return false;
		}

		if (endpoint != Coord2D(0, 0)) {
			assert(triangulation.pointIsVertex(endpoint));
			triangulation.remove(endpoint);
		}

		assert(!triangulation.pointIsVertex(endpoint));

		triangulation.insert(coord);
		endpoint = coord;

		return true;
	}

	bool intersectsEdges(Edge const &checkEdge_) const
	{
		Edge checkEdge(checkEdge_);

		int r[2] = {
			static_cast<int>(checkEdge.end.x) - static_cast<int>(checkEdge.start.x),
			static_cast<int>(checkEdge.end.y) - static_cast<int>(checkEdge.start.y)
		};

		for (std::vector< std::vector<Edge> >::const_iterator it = edges.begin();
		     it != edges.end();
		     it++) {
			std::vector< Coord2DTemplate<float> > intersectionPoints;

			for (std::vector<Edge>::const_iterator eit = it->begin(); eit != it->end(); eit++) {
				Edge edge = *eit;

				int s[2] = {
					static_cast<int>(edge.end.x) - static_cast<int>(edge.start.x),
					static_cast<int>(edge.end.y) - static_cast<int>(edge.start.y)
				};

				float x, y;

				Coord2D diffPoint(edge.start.x - checkEdge.start.x, edge.start.y - checkEdge.start.y);
				int diffPointCrossR = diffPoint.x * r[1] - diffPoint.y * r[0];
				int rCrossS = r[0] * s[1] - r[1] * s[0];

				if (rCrossS == 0) {
					if (diffPointCrossR == 0) {
						int diffPointMultR = diffPoint.x * r[0] + diffPoint.y * r[1];
						int diffPointMultS = diffPoint.x * s[0] + diffPoint.y * s[1];
						int rMultR = r[0] * r[0] + r[1] * r[1];
						int sMultS = s[0] * s[0] + s[1] * s[1];

						if ((0 <= diffPointMultR) && (diffPointMultR <= rMultR)) {
							return true;
						} else if ((0 <= diffPointMultS) && (diffPointMultS <= sMultS)) {
							return true;
						} else {
							continue;
						}
					} else {
						continue;
					}
				} else {
					int diffPointCrossS = diffPoint.x * s[1] - diffPoint.y * s[0];
					float u = diffPointCrossR / static_cast<float>(rCrossS);
					float t = diffPointCrossS / static_cast<float>(rCrossS);

					if ((0 <= u) && (u <= 1) && (0 <= t) && (t <= 1)) {
						x = checkEdge.start.x + t * r[0];
						y = checkEdge.start.y + t * r[1];
					} else {
						continue;
					}
				}

				int maxCheckX = std::max(checkEdge.start.x, checkEdge.end.x);
				int maxCheckY = std::max(checkEdge.start.y, checkEdge.end.y);
				int minCheckX = std::min(checkEdge.start.x, checkEdge.end.x);
				int minCheckY = std::min(checkEdge.start.y, checkEdge.end.y);

				int maxX = std::max(edge.start.x, edge.end.x);
				int maxY = std::max(edge.start.y, edge.end.y);
				int minX = std::min(edge.start.x, edge.end.x);
				int minY = std::min(edge.start.y, edge.end.y);

				if (x < minCheckX || x > maxCheckX) {
					continue;
				}

				if (x < minX || x > maxX) {
					continue;
				}

				if (y < minCheckY || y > maxCheckY) {
					continue;
				}

				if (y < minY || y > maxY) {
					continue;
				}

				Coord2DTemplate<float> intersectionPoint(x, y);

				intersectionPoints.push_back(intersectionPoint);
			}

			if (intersectionPoints.size() >= 2) {
				for (size_t i = 0; i < intersectionPoints.size(); i++) {
					size_t next = i + 1;

					if (next == intersectionPoints.size()) {
						next = 0;
					}

					Coord2DTemplate<float> start(intersectionPoints[i]);
					Coord2DTemplate<float> end(intersectionPoints[next]);

					float checkX = (start.x + end.x) / 2.0;
					float checkY = (start.y + end.y) / 2.0;

					if (!roomTriangulation.inDomain(checkX, checkY)) {
						return true;
					}
				}

				return false;
			}

			if (!intersectionPoints.empty()) {
				float x = intersectionPoints[0].x;
				float y = intersectionPoints[0].y;

				if ((x == checkEdge.start.x && y == checkEdge.start.y) ||
				    (x == checkEdge.end.x && y == checkEdge.end.y)) {
					return false;
				}

				return true;
			}
		}

		return false;
	}

	std::vector<Coord2D> generatePath()
	{
		std::vector<Coord2D> generatedPath;

		NeighboursMap neighbours = triangulation.getNeighbours();
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

				if (intersectsEdges(checkEdge)) {
					continue;
				}

				double xDistance = static_cast<double>(thatCoord.x) - thisCoord.x;
				double yDistance = static_cast<double>(thatCoord.y) - thisCoord.y;
				double distance = std::sqrt(xDistance * xDistance + yDistance * yDistance);

				adjacency_list[neighbourToIndexMap[thisCoord]].push_back(neighbour(neighbourToIndexMap[thatCoord], distance));
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

	void reinitializeTriangulation()
	{
		triangulation.clear();

		Coord2D newStartpoint = startpoint;
		startpoint = Coord2D(0, 0);
		Coord2D newEndpoint = endpoint;
		endpoint = Coord2D(0, 0);
		setStartpoint(newStartpoint);
		setEndpoint(newEndpoint);
	}
};


















Room::Room(std::string const &filename, unsigned char distance)
	: p(new RoomImpl(filename, distance))
{
}

RoomImage const &Room::image() const
{
	return *p->image;
}

bool Room::setStartpoint(Coord2D const &coord)
{
	return p->setStartpoint(coord);
}

Coord2D Room::getStartpoint() const
{
	return p->startpoint;
}

bool Room::setEndpoint(Coord2D const &coord)
{
	return p->setEndpoint(coord);
}

Coord2D Room::getEndpoint() const
{
	return p->endpoint;
}

bool Room::insertWaypoint(Coord2D const &coord)
{
	return p->insert(coord);
}

bool Room::removeWaypoint(Coord2D const &coord)
{
	return p->remove(coord);
}

void Room::clearWaypoints()
{
	p->reinitializeTriangulation();
}

bool Room::hasWaypoint(Coord2D const &coord) const
{
	return p->triangulation.pointIsVertex(coord);
}

std::set<Coord2D> Room::getWaypoints() const
{
	return p->triangulation.list();
}

NeighboursMap Room::getNeighbours() const
{
	return p->triangulation.getNeighbours();
}

std::vector< std::vector<Edge> > Room::getEdges() const
{
	return p->edges;
}

bool Room::intersectsEdges(Edge const &checkEdge) const
{
	return p->intersectsEdges(checkEdge);
}

std::vector<Triangle> Room::getTriangulation() const
{
	return p->triangulation.getTriangulation();
}

std::vector<Triangle> Room::getRoomTriangulation() const
{
	return p->roomTriangulation.getTriangulation();
}

std::vector<Coord2D> Room::generatePath() const
{
	return p->generatePath();
}
