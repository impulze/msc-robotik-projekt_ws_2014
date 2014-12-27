#include "dijkstra.h"
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

		std::vector<Polygon2D> const &borderPolygons = image->getBorderPolygons(distance);

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

		edges = calculateEdges();

		reinitializeTriangulation();
	}

	RoomImage *image;
	unsigned char const *bytes;
	unsigned int width;
	unsigned int height;
	unsigned char stride;
	unsigned char distance;
	ConstrainedDelaunayTriangulation triangulation;
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

		if (!roomTriangulation.inDomain(coord)) {
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

		if (!roomTriangulation.inDomain(coord)) {
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


		if (!roomTriangulation.inDomain(coord)) {
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

	std::vector< std::vector<Edge> > calculateEdges() const
	{
		// this array below distinguishes between big room (first) and holes
		std::vector<Polygon2D> const &borderPolygons = image->getBorderPolygons(distance);

		// this array doesn't distinguish between big room and holes
		//std::vector<Edge> constraints = roomTriangulation.getConstrainedEdges();

		std::vector< std::vector<Edge> > edges;

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

		return edges;
	}

	bool intersectsEdges(Edge const &checkEdge_)
	{
		Edge checkEdge(checkEdge_);

		if (checkEdge.end.x < checkEdge.start.x) {
			std::swap(checkEdge.start.x, checkEdge.end.x);
		}

		if (checkEdge.end.y < checkEdge.start.y) {
			std::swap(checkEdge.start.y, checkEdge.end.y);
		}

		for (std::vector< std::vector<Edge> >::const_iterator it = edges.begin();
		     it != edges.end();
		     it++) {
			int x12 = checkEdge.start.x - checkEdge.end.x;
			int y12 = checkEdge.start.y - checkEdge.end.y;

			for (std::vector<Edge>::const_iterator eit = it->begin(); eit != it->end(); eit++) {
				Edge edge = *eit;

				if (edge.end.x < edge.start.x) {
					std::swap(edge.start.x, edge.end.x);
				}

				if (edge.end.y < edge.start.y) {
					std::swap(edge.start.y, edge.end.y);
				}

				int x34 = edge.start.x - edge.end.x;
				int y34 = edge.start.y - edge.end.y;
				int c = x12 * y34 - y12 * x34;

				if (c == 0) {
					continue;
				}

				int a = checkEdge.start.x * checkEdge.end.y - checkEdge.start.y * checkEdge.end.x;
				int b = edge.start.x * edge.end.y - edge.start.y * edge.end.x;

				float x = (a * x34 - b * x12) / (1.0 * c);
				float y = (a * y34 - b * y12) / (1.0 * c);

				if (x < checkEdge.start.x || x > checkEdge.end.x) {
					continue;
				}

				if (x < edge.start.x || x > edge.end.x) {
					continue;
				}

				if (y < checkEdge.start.y || y > checkEdge.end.y) {
					continue;
				}

				if (y < edge.start.y || y > edge.end.y) {
					continue;
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

		for (NeighboursMap::const_iterator it = neighbours.begin(); it != neighbours.end(); it++) {
			Coord2D coord = it->first;

			if (neighbourToIndexMap.find(coord) == neighbourToIndexMap.end()) {
				neighbourToIndexMap[coord] = i++;
			}
		}

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

				double xDistance = static_cast<double>(thatCoord.x) - thisCoord.x;
				double yDistance = static_cast<double>(thatCoord.y) - thisCoord.y;
				double distance = std::sqrt(xDistance * xDistance + yDistance * yDistance);

				adjacency_list[neighbourToIndexMap[thisCoord]].push_back(neighbor(neighbourToIndexMap[thatCoord], distance));
			}
		}

		assert(neighbourToIndexMap.find(startpoint) != neighbourToIndexMap.end());
		assert(neighbourToIndexMap.find(endpoint) != neighbourToIndexMap.end());

		std::vector<weight_t> min_distance;
		std::vector<vertex_t> previous;
		DijkstraComputePaths(neighbourToIndexMap[startpoint], adjacency_list, min_distance, previous);
		std::list<vertex_t> path = DijkstraGetShortestPathTo(neighbourToIndexMap[endpoint], previous);

		for (std::list<vertex_t>::const_iterator it = path.begin(); it != path.end(); it++) {
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

		std::vector<Polygon2D> const &borderPolygons = image->getBorderPolygons(distance);

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

			triangulation.insertConstraints(points);
		}

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

std::vector<Triangle> Room::triangulate() const
{
	return p->triangulation.getTriangulation();
}

std::vector<Coord2D> Room::generatePath() const
{
	return p->generatePath();
}
