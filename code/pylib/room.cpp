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

		reinitializeCDT();
	}

	RoomImage *image;
	unsigned char const *bytes;
	unsigned int width;
	unsigned int height;
	unsigned char stride;
	unsigned char distance;
	ConstrainedDelaunayTriangulation cdt;
	Coord2D startpoint;
	Coord2D endpoint;

	bool insert(Coord2D const &coord)
	{
		if (coord == startpoint || coord == endpoint) {
			std::fprintf(stderr, "Waypoint (%d/%d) is startpoint or endpoint, can't insert.\n", coord.x, coord.y);
			return false;
		}

		if (cdt.pointIsVertex(coord)) {
			std::fprintf(stderr, "Waypoint (%d/%d) already inserted, can't insert.\n", coord.x, coord.y);
			return false;
		}

		if (!cdt.inDomain(coord)) {
			std::fprintf(stderr, "Waypoint (%d/%d) outside domain, can't insert.\n", coord.x, coord.y);
			return false;
		}

		cdt.insert(coord);
		assert(cdt.pointIsVertex(coord));

		return true;
	}

	bool remove(Coord2D const &coord)
	{
		if (coord == startpoint || coord == endpoint) {
			std::fprintf(stderr, "Waypoint (%d/%d) is startpoint or endpoint, can't remove.\n", coord.x, coord.y);
			return false;
		}

		if (!cdt.pointIsVertex(coord)) {
			std::fprintf(stderr, "Waypoint (%d/%d) not inserted, can't remove.\n", coord.x, coord.y);
			return false;
		}

		cdt.remove(coord);
		assert(!cdt.pointIsVertex(coord));

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

		if (!cdt.inDomain(coord)) {
			std::fprintf(stderr, "Startpoint (%d/%d) outside domain, can't insert.\n", coord.x, coord.y);
			return false;
		}

		if (startpoint != Coord2D(0, 0)) {
			assert(cdt.pointIsVertex(startpoint));
			cdt.remove(startpoint);
		}

		assert(!cdt.pointIsVertex(startpoint));

		cdt.insert(coord);
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


		if (!cdt.inDomain(coord)) {
			std::fprintf(stderr, "Endpoint (%d/%d) outside domain, can't insert.\n", coord.x, coord.y);
			return false;
		}

		if (endpoint != Coord2D(0, 0)) {
			assert(cdt.pointIsVertex(endpoint));
			cdt.remove(endpoint);
		}

		assert(!cdt.pointIsVertex(endpoint));

		cdt.insert(coord);
		endpoint = coord;

		return true;
	}

	std::vector<Coord2D> generatePath()
	{
		std::vector<Coord2D> generatedPath;

		NeighboursMap neighbours = cdt.getNeighbours();
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

	void reinitializeCDT()
	{
		cdt.clear();

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

			cdt.insertConstraints(points);
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
	p->reinitializeCDT();
}

bool Room::hasWaypoint(Coord2D const &coord) const
{
	return p->cdt.pointIsVertex(coord);
}

std::set<Coord2D> Room::getWaypoints() const
{
	return p->cdt.list();
}

NeighboursMap Room::getNeighbours() const
{
	return p->cdt.getNeighbours();
}

std::vector<Triangle> Room::triangulate() const
{
	return p->cdt.getTriangulation();
}

std::vector<Coord2D> Room::generatePath() const
{
	return p->generatePath();
}
