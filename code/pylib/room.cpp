#include "room.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Triangulation_conformer_2.h>
#include <CGAL/Random.h>
#include <CGAL/point_generators_2.h>
#include <CGAL/Timer.h>

namespace _CDT
{
	template<class GeomTraits, class FaceBase>
	class Enriched_face_base_2
		: public FaceBase
	{
	public:
		typedef GeomTraits Geom_traits;
		typedef typename FaceBase::Vertex_handle Vertex_handle;
		typedef typename FaceBase::Face_handle Face_handle;

		template<class TDS2>
		struct Rebind_TDS
		{
			typedef typename FaceBase::template Rebind_TDS<TDS2>::Other FaceBase2;
			typedef Enriched_face_base_2<GeomTraits, FaceBase2> Other;
		};

		Enriched_face_base_2();
		Enriched_face_base_2(Vertex_handle vh0, Vertex_handle vh1, Vertex_handle vh2);
		Enriched_face_base_2(Vertex_handle vh0, Vertex_handle vh1, Vertex_handle vh2,
		                     Face_handle fh0, Face_handle fh1, Face_handle fh2);

		bool is_in_domain() const;
		void set_in_domain(const bool set);

		void set_counter(int i);
		int counter() const;
		int& counter();

	private:
		int status_;
	};

	template<class GeomTraits, class FaceBase>
	Enriched_face_base_2<GeomTraits, FaceBase>::Enriched_face_base_2()
		: FaceBase(),
		  status_(-1)
	{
	}

	template<class GeomTraits, class FaceBase>
	Enriched_face_base_2<GeomTraits, FaceBase>::Enriched_face_base_2(Vertex_handle vh0, Vertex_handle vh1, Vertex_handle vh2)
		: FaceBase(vh0, vh1, vh2),
		  status_(-1)
	{
	}

	template<class GeomTraits, class FaceBase>
	Enriched_face_base_2<GeomTraits, FaceBase>::Enriched_face_base_2(Vertex_handle vh0, Vertex_handle vh1, Vertex_handle vh2,
	                                                                 Face_handle fh0, Face_handle fh1, Face_handle fh2)
		: FaceBase(vh0, vh1, vh2, fh0, fh1, fh2),
		  status_(-1)
	{
	}

	template<class GeomTraits, class FaceBase>
	bool Enriched_face_base_2<GeomTraits, FaceBase>::is_in_domain() const
	{
		return status_ % 2 == 1;
	}

	template<class GeomTraits, class FaceBase>
	void Enriched_face_base_2<GeomTraits, FaceBase>::set_in_domain(const bool set)
	{
		status_ = set ? 1 : 0;
	}

	template<class GeomTraits, class FaceBase>
	void Enriched_face_base_2<GeomTraits, FaceBase>::set_counter(int i)
	{
		status_ = i;
	}

	template<class GeomTraits, class FaceBase>
	int Enriched_face_base_2<GeomTraits, FaceBase>::counter() const
	{
		return status_;
	}

	template<class GeomTraits, class FaceBase>
	int& Enriched_face_base_2<GeomTraits, FaceBase>::Enriched_face_base_2::counter()
	{
		return status_;
	}

	typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
	typedef K::Point_2 Point_2;
	typedef K::Segment_2 Segment_2;
	typedef K::Iso_rectangle_2 Iso_rectangle_2;
	typedef CGAL::Triangulation_vertex_base_2<K>  Vertex_base;
	typedef CGAL::Constrained_triangulation_face_base_2<K> Face_base;
	typedef Enriched_face_base_2<K, Face_base> FaceBase;
	typedef CGAL::Triangulation_data_structure_2<Vertex_base, FaceBase> TDS;
	typedef CGAL::Exact_predicates_tag Itag;
	typedef CGAL::Constrained_Delaunay_triangulation_2<K, TDS, Itag> CDT;
	typedef CDT::Vertex_handle Vertex_handle;
	typedef CDT::Face_handle Face_handle;
	typedef CDT::All_faces_iterator All_faces_iterator;

	void discoverComponent(const CDT &cdt, Face_handle start, int index, std::list<CDT::Edge>& border)
	{
		if (start->counter() != -1) {
			return;
		}

		std::list<Face_handle> queue;
		queue.push_back(start);

		while (!queue.empty()) {
			Face_handle fh = queue.front();
			queue.pop_front();

			if (fh->counter() == -1) {
				fh->counter() = index;
				fh->set_in_domain(index % 2 == 1);

				for (int i = 0; i < 3; i++) {
					CDT::Edge edge(fh, i);
					Face_handle neighbor = fh->neighbor(i);

					if (neighbor->counter() == -1) {
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
			Face_handle neigbor = edge.first->neighbor(edge.second);

			if (neigbor->counter() == -1) {
				discoverComponent(cdt, neigbor, edge.first->counter() + 1, border);
			}
		}
	}

	template<class Iterator>
	void insertPolyline(CDT &cdt, Iterator begin, Iterator end)
	{
		Point_2 p, q;
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
		for (All_faces_iterator it = cdt.all_faces_begin(); it != cdt.all_faces_end(); ++it) {
			it->set_counter(-1);
		}
	}
}

struct Room::RIMPL
{
	RIMPL(unsigned char const *bytes, unsigned int width, unsigned int height, unsigned char stride)
		: bytes(bytes),
		  width(width),
		  height(height),
		  stride(stride)
	{
	}

	unsigned char const *bytes;
	unsigned int width;
	unsigned int height;
	unsigned char stride;
	_CDT::CDT cdt;
	std::vector<Polygon2D> triangulatedPolygons;
	std::vector<Coord2D> calculatedPath;
	std::set<Coord2D> waypoints;
	Coord2D startpoint;
	Coord2D endpoint;

	bool isBlack(unsigned char const *bytes)
	{
		return bytes[0] == 0 && bytes[1] == 0 && bytes[2] == 0;
	}

	bool isWhite(unsigned char const *bytes)
	{
		return bytes[0] == 255 && bytes[1] == 255 && bytes[2] == 255;
	}

	bool isBlack(Coord2D const &coord)
	{
		return isBlack(bytes + (coord.y * width + coord.x) * stride);
	}

	bool isWhite(Coord2D const &coord)
	{
		return isWhite(bytes + (coord.y * width + coord.x) * stride);
	}

	void createPolygons()
	{
		triangulatedPolygons.clear();

		// mark faces in/out of domain
		_CDT::initializeID(cdt);
		_CDT::discoverComponents(cdt);

		for (_CDT::CDT::Finite_faces_iterator fit = cdt.finite_faces_begin();
		     fit != cdt.finite_faces_end();
		     ++fit) {
			Polygon2D polygon;

			if (fit->is_in_domain()) {
				_CDT::Point_2 p0 = fit->vertex(0)->point();
				_CDT::Point_2 p1 = fit->vertex(1)->point();
				_CDT::Point_2 p2 = fit->vertex(2)->point();

				polygon.push_back(Coord2D(p0.x(), p0.y()));
				polygon.push_back(Coord2D(p1.x(), p1.y()));
				polygon.push_back(Coord2D(p2.x(), p2.y()));
			}

			triangulatedPolygons.push_back(polygon);
		}
	}

	int _sign(Coord2D const &p0, _CDT::Point_2 const &p1, _CDT::Point_2 const &p2)
	{
		return (p0.x - p2.x()) * (p1.y() - p2.y()) - (p1.x() - p2.x()) * (p0.y - p2.y());
	}

	bool inDomain(Coord2D const &coord)
	{
		_CDT::Point_2 p(coord.x, coord.y);

		for (_CDT::CDT::Finite_faces_iterator fit = cdt.finite_faces_begin();
		     fit != cdt.finite_faces_end();
		     ++fit) {
			for (int i = 0; i < 3; i++) {
				if (fit->is_in_domain()) {
					_CDT::CDT::Ctr::Triangulation::Triangle triangle = cdt.triangle(fit);

					if (triangle.has_on_positive_side(p)) {
						return true;
					}
				}
			}
		}

		return false;
	}

	bool findCDTVertex(Coord2D const &coord, _CDT::Vertex_handle &vh, _CDT::Face_handle &fh)
	{
		_CDT::Point_2 p(coord.x, coord.y);

		for (_CDT::CDT::Finite_faces_iterator fit = cdt.finite_faces_begin();
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
		_CDT::Vertex_handle vh;
		_CDT::Face_handle fh;

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
		cdt.insert(_CDT::Point_2(coord.x, coord.y));

		createPolygons();

		return true;
	}

	bool remove(Coord2D const &coord)
	{
		_CDT::Vertex_handle vh;
		_CDT::Face_handle fh;

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
		assert(fh->is_in_domain());

		waypoints.erase(waypointIterator);
		cdt.remove(vh);

		createPolygons();

		return true;
	}

	bool setStartpoint(Coord2D const &coord)
	{
		_CDT::Vertex_handle vh;
		_CDT::Face_handle fh;

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

		cdt.insert(_CDT::Point_2(coord.x, coord.y));
		startpoint = coord;

		createPolygons();

		return true;
	}

	bool setEndpoint(Coord2D const &coord)
	{
		_CDT::Vertex_handle vh;
		_CDT::Face_handle fh;

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

		cdt.insert(_CDT::Point_2(coord.x, coord.y));
		endpoint = coord;

		createPolygons();

		return true;
	}

	void calculatePath()
	{
	}

	std::vector<Coord2D> const &getCalculatedPath()
	{
		return calculatedPath;
	}
};


















Room::Room(std::string const &filename, unsigned char distance)
	: image_(filename),
	  p(new RIMPL(image_.data().data(), image_.width(), image_.height(), image_.type() == Image::IMAGE_TYPE_RGB ? 3 : 4))
{
	triangulate(distance);
}

RoomImage const &Room::image() const
{
	return image_;
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
	p->waypoints.clear();
	p->triangulatedPolygons.clear();
	p->cdt.clear();
	p->startpoint = Coord2D();
	p->endpoint = Coord2D();

	std::vector<Polygon2D> const &innerPolygons = image_.triangulate(distance);

	for (std::vector<Polygon2D>::const_iterator it = innerPolygons.begin();
	     it != innerPolygons.end();
	     it++) {
		std::list<_CDT::Point_2> points;

		for (std::size_t i = 0; i < it->size(); i++) {
			Coord2D coord = (*it)[i];
			points.push_back(_CDT::Point_2(coord.x, coord.y));
		}

		Coord2D coord = (*it)[0];
		points.push_back(_CDT::Point_2(coord.x, coord.y));
		
		_CDT::insertPolyline(p->cdt, points.begin(), points.end());
	}

	p->createPolygons();
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
	return p->getCalculatedPath();
}
