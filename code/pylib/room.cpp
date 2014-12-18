#include "dijkstra.h"
#include "mycdt.h"
#include "room.h"
#include "roomimage.h"

#include <cmath>

namespace
{
	bool isBlack(unsigned char const *bytes);
	bool isWhite(unsigned char const *bytes);

	template <class GeomTraits, class FaceBase>
	class CDTFaceBase
		: public FaceBase
	{
	public:
		typedef typename FaceBase::Face_handle FaceHandle;
		typedef typename FaceBase::Vertex_handle VertexHandle;

		// required for CGAL
		template <class TDS2>
		struct Rebind_TDS
		{
			typedef typename FaceBase::template Rebind_TDS<TDS2>::Other FaceBase2;
			typedef CDTFaceBase<GeomTraits, FaceBase2> Other;
		};

		CDTFaceBase();
		CDTFaceBase(VertexHandle vh0, VertexHandle vh1, VertexHandle vh2);
		CDTFaceBase(VertexHandle vh0, VertexHandle vh1, VertexHandle vh2,
		            FaceHandle fh0, FaceHandle fh1, FaceHandle fh2);

		bool getInDomain() const;
		void setInDomain(const bool set);

		int getCounter() const;
		void setCounter(int i);

	private:
		int status_;
	};

	bool isBlack(unsigned char const *bytes)
	{
		return bytes[0] == 0 && bytes[1] == 0 && bytes[2] == 0;
	}

	bool isWhite(unsigned char const *bytes)
	{
		return bytes[0] == 255 && bytes[1] == 255 && bytes[2] == 255;
	}
} // end of private namespace

struct Room::RoomImpl
{
	RoomImpl(std::string const &filename, unsigned char distance)
		: image(new RoomImage(filename))
	{
		bytes = image->data().data();
		width = image->width();
		height = image->height();
		stride = image->type() == Image::IMAGE_TYPE_RGB ? 3 : 4;

		std::vector<Polygon2D> const &borderPolygons = image->getBorderPolygons(distance);

		for (std::vector<Polygon2D>::const_iterator it = borderPolygons.begin();
		     it != borderPolygons.end();
		     it++) {
			std::list<CDT::Point> points;

			for (std::size_t i = 0; i < it->size(); i++) {
				Coord2D coord = (*it)[i];
				points.push_back(CDT::Point(coord.x, coord.y));
			}

			Coord2D coord = (*it)[0];
			points.push_back(CDT::Point(coord.x, coord.y));
		
			cdt.insertConstraints(points.begin(), points.end());
		}

		triangulation = cdt.triangulate();
	}

	RoomImage *image;
	unsigned char const *bytes;
	unsigned int width;
	unsigned int height;
	unsigned char stride;
	MyCDT cdt;
	std::vector<Triangle> triangulation;
	std::vector<Coord2D> generatedPath;
	NeighboursMap neighbours;
	std::set<Coord2D> waypoints;
	Coord2D startpoint;
	Coord2D endpoint;

	bool isBlack(Coord2D const &coord)
	{
		return ::isBlack(bytes + (coord.y * width + coord.x) * stride);
	}

	bool isWhite(Coord2D const &coord)
	{
		return ::isWhite(bytes + (coord.y * width + coord.x) * stride);
	}

	int _sign(Coord2D const &p0, CDT::Point const &p1, CDT::Point const &p2)
	{
		return (p0.x - p2.x()) * (p1.y() - p2.y()) - (p1.x() - p2.x()) * (p0.y - p2.y());
	}

	bool insert(Coord2D const &coord)
	{
		if (coord == startpoint || coord == endpoint) {
			std::fprintf(stderr, "Waypoint (%d/%d) is startpoint or endpoint, can't insert.\n", coord.x, coord.y);
			return false;
		}

		if (std::find(waypoints.begin(), waypoints.end(), coord) != waypoints.end()) {
			assert(cdt.pointIsVertex(coord));
			return true;
		}

		// TOP PRIO TODO: find out why this sometimes failed when generating 2000+ waypoints
		assert(!cdt.pointIsVertex(coord));

		if (!cdt.inDomain(coord)) {
			std::fprintf(stderr, "Waypoint (%d/%d) outside domain.\n", coord.x, coord.y);
			return false;
		}

		waypoints.insert(coord);
		cdt.insert(coord);

		triangulation = cdt.triangulate();

		return true;
	}

	bool remove(std::set<Coord2D>::iterator waypointIterator)
	{
		Coord2D coord = *waypointIterator;

		if (waypointIterator == waypoints.end()) {
			assert(!cdt.pointIsVertex(coord));
			std::fprintf(stderr, "Waypoint (%d/%d) not inserted yet, can't remove.\n", coord.x, coord.y);
			return false;
		}

		assert(cdt.pointIsVertex(coord));

		waypoints.erase(waypointIterator);
		cdt.remove(coord);

		triangulation = cdt.triangulate();

		return true;
	}

	bool setStartpoint(Coord2D const &coord)
	{
		CDT::Vertex_handle vh;
		CDT::Face_handle fh;

		if (startpoint == coord) {
			return true;
		}

		if (!cdt.inDomain(coord)) {
			std::printf("Startpoint (%d/%d) outside domain.\n", coord.x, coord.y);
			return false;
		}

		if (std::find(waypoints.begin(), waypoints.end(), coord) != waypoints.end()) {
			std::fprintf(stderr, "Can't set startpoint (%d/%d), it's a waypoint.\n", coord.x, coord.y);
			return false;
		}

		if (coord == endpoint) {
			std::fprintf(stderr, "Can't set startpoint (%d/%d), it's the endpoint.\n", coord.x, coord.y);
			return false;
		}

		if (startpoint != Coord2D(0, 0)) {
			assert(cdt.pointIsVertex(startpoint));
			cdt.remove(startpoint);
		}

		assert(!cdt.pointIsVertex(startpoint));

		cdt.insert(coord);
		startpoint = coord;

		triangulation = cdt.triangulate();

		return true;
	}

	bool setEndpoint(Coord2D const &coord)
	{
		CDT::Vertex_handle vh;
		CDT::Face_handle fh;

		if (endpoint == coord) {
			return true;
		}

		if (!cdt.inDomain(coord)) {
			std::printf("Endpoint (%d/%d) outside domain.\n", coord.x, coord.y);
			return false;
		}

		if (std::find(waypoints.begin(), waypoints.end(), coord) != waypoints.end()) {
			std::fprintf(stderr, "Can't set endpoint (%d/%d), it's a waypoint.\n", coord.x, coord.y);
			return false;
		}

		if (coord == startpoint) {
			std::fprintf(stderr, "Can't set endpoint (%d/%d), it's the startpoint.\n", coord.x, coord.y);
			return false;
		}

		if (endpoint != Coord2D(0, 0)) {
			assert(cdt.pointIsVertex(endpoint));
			cdt.remove(endpoint);
		}

		assert(!cdt.pointIsVertex(endpoint));

		cdt.insert(coord);
		endpoint = coord;

		triangulation = cdt.triangulate();

		return true;
	}

	void generatePath()
	{
		calculateNeighbours();

		generatedPath.clear();

		std::map<Coord2D, int> neighbourToIndexMap;

		int i = 0;

		for (Room::NeighboursMap::const_iterator it = neighbours.begin(); it != neighbours.end(); it++) {
			Coord2D coord = it->first;

			if (neighbourToIndexMap.find(coord) == neighbourToIndexMap.end()) {
				neighbourToIndexMap[coord] = i++;
			}
		}

		for (Room::NeighboursMap::const_iterator it = neighbours.begin(); it != neighbours.end(); it++) {
			for (std::set<Coord2D>::const_iterator cit = it->second.begin(); cit != it->second.end(); cit++) {
				Coord2D checkCoord = *cit;
				assert(neighbourToIndexMap.find(checkCoord) != neighbourToIndexMap.end());
			}
		}

		adjacency_list_t adjacency_list(neighbours.size());

		for (Room::NeighboursMap::const_iterator it = neighbours.begin(); it != neighbours.end(); it++) {
			Coord2D thisCoord = it->first;

			for (std::set<Coord2D>::const_iterator cit = it->second.begin(); cit != it->second.end(); cit++) {
				Coord2D thatCoord = *cit;

				double xDistance = thatCoord.x - thisCoord.x;
				double yDistance = thatCoord.y - thisCoord.y;
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
	}

	void calculateNeighbours()
	{
		neighbours.clear();

		for (CDT::Finite_vertices_iterator vi = cdt.cdt.finite_vertices_begin(); vi != cdt.cdt.finite_vertices_end(); vi++) {
			Coord2D c = Coord2D(vi->point().x(), vi->point().y());

			if (waypoints.find(c) == waypoints.end()) {
				if (c != startpoint && c != endpoint) {
					continue;
				}
			}

			CDT::Edge_circulator ec = cdt.cdt.incident_edges(vi);
			CDT::Edge_circulator ec_done = ec;
			std::set<Coord2D> thisNeighbours;

			do {
				bool addNeighbour = false;

				if (cdt.cdt.is_constrained(*ec)) {
					addNeighbour = true;
				} else if (!cdt.cdt.is_infinite(ec)) {
					if (ec->first->getInDomain()) {
						addNeighbour = true;
					}
				}

				if (addNeighbour) {
					CDT::Segment segment = cdt.cdt.segment(ec);
					Coord2D c0(segment.point(0).x(), segment.point(0).y());
					Coord2D c1(segment.point(1).x(), segment.point(1).y());


					if (c0 == c) {
						if (waypoints.find(c1) != waypoints.end() ||
						    c1 == startpoint ||
						    c1 == endpoint) {
							thisNeighbours.insert(c1);
						}
					} else {
						if (waypoints.find(c0) != waypoints.end() ||
						    c0 == startpoint ||
						    c0 == endpoint) {
							thisNeighbours.insert(c0);
						}
					}
				}

				ec++;
			} while (ec != ec_done);

			neighbours[c] = thisNeighbours;
		}
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

bool Room::removeWaypoint(std::set<Coord2D>::iterator waypointIterator)
{
	return p->remove(waypointIterator);
}

std::set<Coord2D> const &Room::getWaypoints() const
{
	return p->waypoints;
}

std::vector<Triangle> const &Room::getTriangulation() const
{
	return p->triangulation;
}

void Room::generatePath()
{
	p->generatePath();
}

std::vector<Coord2D> const &Room::getGeneratedPath() const
{
	return p->generatedPath;
}
