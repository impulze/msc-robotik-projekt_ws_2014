#include "dijkstra.h"
#include "room.h"
#include "roomimage.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>

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

	template <class GeomTraits, class FaceBase>
	CDTFaceBase<GeomTraits, FaceBase>::CDTFaceBase()
		: FaceBase(),
		  status_(-1)
	{
	}

	template <class GeomTraits, class FaceBase>
	CDTFaceBase<GeomTraits, FaceBase>::CDTFaceBase(VertexHandle vh0, VertexHandle vh1, VertexHandle vh2)
		: FaceBase(vh0, vh1, vh2),
		  status_(-1)
	{
	}

	template <class GeomTraits, class FaceBase>
	CDTFaceBase<GeomTraits, FaceBase>::CDTFaceBase(VertexHandle vh0, VertexHandle vh1, VertexHandle vh2,
	                                               FaceHandle fh0, FaceHandle fh1, FaceHandle fh2)
		: FaceBase(vh0, vh1, vh2, fh0, fh1, fh2),
		  status_(-1)
	{
	}

	template <class GeomTraits, class FaceBase>
	bool CDTFaceBase<GeomTraits, FaceBase>::getInDomain() const
	{
		return status_ % 2 == 1;
	}

	template <class GeomTraits, class FaceBase>
	void CDTFaceBase<GeomTraits, FaceBase>::setInDomain(const bool set)
	{
		status_ = set ? 1 : 0;
	}

	template <class GeomTraits, class FaceBase>
	int CDTFaceBase<GeomTraits, FaceBase>::getCounter() const
	{
		return status_;
	}

	template <class GeomTraits, class FaceBase>
	void CDTFaceBase<GeomTraits, FaceBase>::setCounter(int i)
	{
		status_ = i;
	}
} // end of private namespace


typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef Kernel::Point_2 Point;
typedef CGAL::Triangulation_vertex_base_2<Kernel> VertexBase;
typedef CGAL::Constrained_triangulation_face_base_2<Kernel> CTFaceBase;
typedef CDTFaceBase<Kernel, CTFaceBase> TDSFaceBase;
typedef CGAL::Triangulation_data_structure_2<VertexBase, TDSFaceBase> TDS;
typedef CGAL::Exact_predicates_tag CDTIntersectionTag;
typedef CGAL::Constrained_Delaunay_triangulation_2<Kernel, TDS, CDTIntersectionTag> CDT;

	void discoverComponent(const CDT &cdt, CDT::Face_handle start, int index, std::list<CDT::Edge>& border)
	{
		if (start->getCounter() != -1) {
			return;
		}

		std::list<CDT::Face_handle> queue;
		queue.push_back(start);

		while (!queue.empty()) {
			CDT::Face_handle fh = queue.front();
			queue.pop_front();

			if (fh->getCounter() == -1) {
				fh->setCounter(index);
				fh->setInDomain(index % 2 == 1);

				for (int i = 0; i < 3; i++) {
					CDT::Edge edge(fh, i);
					CDT::Face_handle neighbor = fh->neighbor(i);

					if (neighbor->getCounter() == -1) {
						if (cdt.is_constrained(edge)) {
							border.push_back(edge);
						} else {
							queue.push_back(neighbor);
						}
					}
				}
			}
		}
	}

	void discoverComponents(const CDT &cdt)
	{
		if (cdt.dimension() != 2) {
			return;
		}

		int index = 0;
		std::list<CDT::Edge> border;
		discoverComponent(cdt, cdt.infinite_face(), index++, border);

		while (!border.empty()) {
			CDT::Edge edge = border.front();
			border.pop_front();
			CDT::Face_handle neigbor = edge.first->neighbor(edge.second);

			if (neigbor->getCounter() == -1) {
				discoverComponent(cdt, neigbor, edge.first->getCounter() + 1, border);
			}
		}
	}

	template<class Iterator>
	void insertPolyline(CDT &cdt, Iterator begin, Iterator end)
	{
		CDT::Point p, q;
		CDT::Vertex_handle vh, wh;
		Iterator it = begin;

		vh = cdt.insert(*it);
		p = *it;
		++it;

		for(; it != end; ++it) {
			q = *it;
			if (p != q) {
				wh = cdt.insert(*it);
				cdt.insert_constraint(vh, wh);
				vh = wh;
				p = q;
			} else {
				std::cout << "duplicate point: " << p << std::endl;
			}
		}
	}

	void initializeID(const CDT& cdt)
	{
		for (CDT::All_faces_iterator it = cdt.all_faces_begin(); it != cdt.all_faces_end(); ++it) {
			it->setCounter(-1);
		}
	}

struct Room::RoomImpl
{
	RoomImpl(std::string const &filename)
		: image(new RoomImage(filename))
	{
		bytes = image->data().data();
		width = image->width();
		height = image->height();
		stride = image->type() == Image::IMAGE_TYPE_RGB ? 3 : 4;
	}

	RoomImage *image;
	unsigned char const *bytes;
	unsigned int width;
	unsigned int height;
	unsigned char stride;
	CDT cdt;
	std::vector<Polygon2D> triangulatedPolygons;
	std::vector<Coord2D> calculatedPath;
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

	void createPolygons()
	{
		triangulatedPolygons.clear();

		// mark faces in/out of domain
		initializeID(cdt);
		discoverComponents(cdt);

		for (CDT::Finite_faces_iterator fit = cdt.finite_faces_begin();
		     fit != cdt.finite_faces_end();
		     ++fit) {
			Polygon2D polygon;

			if (fit->getInDomain()) {
				CDT::Point p0 = fit->vertex(0)->point();
				CDT::Point p1 = fit->vertex(1)->point();
				CDT::Point p2 = fit->vertex(2)->point();

				polygon.push_back(Coord2D(p0.x(), p0.y()));
				polygon.push_back(Coord2D(p1.x(), p1.y()));
				polygon.push_back(Coord2D(p2.x(), p2.y()));

				triangulatedPolygons.push_back(polygon);
			}
		}
	}

	int _sign(Coord2D const &p0, CDT::Point const &p1, CDT::Point const &p2)
	{
		return (p0.x - p2.x()) * (p1.y() - p2.y()) - (p1.x() - p2.x()) * (p0.y - p2.y());
	}

	bool inDomain(Coord2D const &coord)
	{
		CDT::Point p(coord.x, coord.y);

		for (CDT::Finite_faces_iterator fit = cdt.finite_faces_begin();
		     fit != cdt.finite_faces_end();
		     ++fit) {
			for (int i = 0; i < 3; i++) {
				if (fit->getInDomain()) {
					CDT::Ctr::Triangulation::Triangle triangle = cdt.triangle(fit);

					if (triangle.has_on_positive_side(p)) {
						return true;
					}
				}
			}
		}

		return false;
	}

	bool findCDTVertex(Coord2D const &coord, CDT::Vertex_handle &vh, CDT::Face_handle &fh)
	{
		CDT::Point p(coord.x, coord.y);

		for (CDT::Finite_faces_iterator fit = cdt.finite_faces_begin();
		     fit != cdt.finite_faces_end();
		     ++fit) {
			for (int i = 0; i < 3; i++) {
				if (fit->vertex(i)->point() == p) {
					fh = fit;
					vh = fit->vertex(i);
					return true;
				}
			}
		}

		return false;
	}

	bool insert(Coord2D const &coord)
	{
		CDT::Vertex_handle vh;
		CDT::Face_handle fh;

		if (coord == startpoint || coord == endpoint) {
			std::fprintf(stderr, "Waypoint (%d/%d) is startpoint or endpoint, can't insert.\n", coord.x, coord.y);
			return false;
		}

		if (std::find(waypoints.begin(), waypoints.end(), coord) != waypoints.end()) {
			assert(findCDTVertex(coord, vh, fh));
			return true;
		}

		// TOP PRIO TODO: find out why this sometimes failed when generating 2000+ waypoints
		assert(!findCDTVertex(coord, vh, fh));

		if (!inDomain(coord)) {
			std::fprintf(stderr, "Waypoint (%d/%d) outside domain.\n", coord.x, coord.y);
			return false;
		}

		waypoints.insert(coord);
		cdt.insert(CDT::Point(coord.x, coord.y));

		createPolygons();

		return true;
	}

	bool remove(Coord2D const &coord)
	{
		CDT::Vertex_handle vh;
		CDT::Face_handle fh;

		if (coord == startpoint || coord == endpoint) {
			std::fprintf(stderr, "Waypoint (%d/%d) is startpoint or endpoint, can't remove.\n", coord.x, coord.y);
			return false;
		}

		std::set<Coord2D>::iterator waypointIterator = std::find(waypoints.begin(), waypoints.end(), coord);

		if (waypointIterator == waypoints.end()) {
			assert(!findCDTVertex(coord, vh, fh));
			std::fprintf(stderr, "Waypoint (%d/%d) not inserted yet, can't remove.\n", coord.x, coord.y);
			return false;
		}

		bool found = findCDTVertex(coord, vh, fh);

		assert(found);
		assert(fh->getInDomain());

		waypoints.erase(waypointIterator);
		cdt.remove(vh);

		createPolygons();

		return true;
	}

	void triangulate(unsigned char distance)
	{
		waypoints.clear();
		triangulatedPolygons.clear();
		cdt.clear();
		startpoint = Coord2D();
		endpoint = Coord2D();

		std::vector<Polygon2D> const &borderPolygons = image->triangulate(distance);

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
		
			insertPolyline(cdt, points.begin(), points.end());
		}

		createPolygons();
	}

	bool setStartpoint(Coord2D const &coord)
	{
		CDT::Vertex_handle vh;
		CDT::Face_handle fh;

		if (startpoint == coord) {
			return true;
		}

		if (!inDomain(coord)) {
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
			bool found = findCDTVertex(startpoint, vh, fh);
			assert(found);
			cdt.remove(vh);
		}

		assert(!findCDTVertex(startpoint, vh, fh));

		cdt.insert(CDT::Point(coord.x, coord.y));
		startpoint = coord;

		createPolygons();

		return true;
	}

	bool setEndpoint(Coord2D const &coord)
	{
		CDT::Vertex_handle vh;
		CDT::Face_handle fh;

		if (endpoint == coord) {
			return true;
		}

		if (!inDomain(coord)) {
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
			bool found = findCDTVertex(endpoint, vh, fh);
			assert(found);
			cdt.remove(vh);
		}

		assert(!findCDTVertex(endpoint, vh, fh));

		cdt.insert(CDT::Point(coord.x, coord.y));
		endpoint = coord;

		createPolygons();

		return true;
	}

	void calculatePath()
	{
		calculateNeighbours();

		calculatedPath.clear();

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

			calculatedPath.push_back(found->first);
		}
	}

	void calculateNeighbours()
	{
		neighbours.clear();

		for (CDT::Finite_vertices_iterator vi = cdt.finite_vertices_begin(); vi != cdt.finite_vertices_end(); vi++) {
			Coord2D c = Coord2D(vi->point().x(), vi->point().y());

			if (waypoints.find(c) == waypoints.end()) {
				if (c != startpoint && c != endpoint) {
					continue;
				}
			}

			CDT::Edge_circulator ec = cdt.incident_edges(vi);
			CDT::Edge_circulator ec_done = ec;
			std::set<Coord2D> thisNeighbours;

			do {
				bool addNeighbour = false;

				if (cdt.is_constrained(*ec)) {
					addNeighbour = true;
				} else if (!cdt.is_infinite(ec)) {
					if (ec->first->getInDomain()) {
						addNeighbour = true;
					}
				}

				if (addNeighbour) {
					CDT::Segment segment = cdt.segment(ec);
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
	: p(new RoomImpl(filename))
{
	triangulate(distance);
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

std::set<Coord2D> const &Room::getWaypoints() const
{
	return p->waypoints;
}

void Room::triangulate(unsigned char distance)
{
	p->triangulate(distance);
}

std::vector<Polygon2D> const &Room::getTriangulatedPolygons() const
{
	return p->triangulatedPolygons;
}

void Room::calculatePath()
{
	p->calculatePath();
}

std::vector<Coord2D> const &Room::getCalculatedPath() const
{
	return p->calculatedPath;
}

void Room::calculateNeighbours()
{
	p->calculateNeighbours();
}

Room::NeighboursMap const &Room::getNeighbours() const
{
	return p->neighbours;
}
