#include "roomimage.h"

#include <cassert>
#include <cstdio>
#include <map>
#include <set>


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

struct RoomImage::RIPIMPL
{
	RIPIMPL(unsigned char const *bytes, unsigned int width, unsigned int height, unsigned char stride)
		: bytes(bytes),
		  width(width),
		  height(height),
		  stride(stride)
	{
		printf("PIMPL: w: %d h: %d s: %d\n", width, height, stride);
	}

	unsigned char const *bytes;
	unsigned int width;
	unsigned int height;
	unsigned char stride;
	_CDT::CDT cdt;

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

	std::vector<Coord2D> createNeighbours(Coord2D const &coord)
	{
		std::vector<Coord2D> neighbours;
		bool addNorth = coord.y > 0;
		bool addEast = coord.x < width - 1;
		bool addSouth = coord.y < height - 1;
		bool addWest = coord.x > 0;

		if (addNorth) {
			Coord2D north(coord.x, coord.y - 1);
			neighbours.push_back(north);

			if (addEast) {
				Coord2D northEast(coord.x + 1, coord.y - 1);
				neighbours.push_back(northEast);
			}

			if (addWest) {
				Coord2D northWest(coord.x - 1, coord.y - 1);
				neighbours.push_back(northWest);
			}
		}

		if (addSouth) {
			Coord2D south(coord.x, coord.y + 1);
			neighbours.push_back(south);

			if (addEast) {
				Coord2D southEast(coord.x + 1, coord.y + 1);
				neighbours.push_back(southEast);
			}

			if (addWest) {
				Coord2D southWest(coord.x - 1, coord.y + 1);
				neighbours.push_back(southWest);
			}
		}

		if (addEast) {
			Coord2D east(coord.x + 1, coord.y);
			neighbours.push_back(east);
		}

		if (addWest) {
			Coord2D west(coord.x - 1, coord.y);
			neighbours.push_back(west);
		}

		return neighbours;
	}

	void createPolygons(ConvexCCWRoomPolygons &convexCCWRoomPolygons)
	{
		convexCCWRoomPolygons.clear();

		_CDT::initializeID(cdt);
		_CDT::discoverComponents(cdt);

		for (_CDT::CDT::Finite_faces_iterator fit = cdt.finite_faces_begin();
		     fit != cdt.finite_faces_end();
		     ++fit) {
			ConvexPolygon2D polygon;

			if (fit->is_in_domain()) {
				_CDT::Point_2 p0 = fit->vertex(0)->point();
				_CDT::Point_2 p1 = fit->vertex(1)->point();
				_CDT::Point_2 p2 = fit->vertex(2)->point();

				polygon.insert(Coord2D(p0.x(), p0.y()));
				polygon.insert(Coord2D(p1.x(), p1.y()));
				polygon.insert(Coord2D(p2.x(), p2.y()));
			}

			convexCCWRoomPolygons.push_back(polygon);
		}
	}

	bool check(Coord2D const &coord)
	{
		cdt.insert(_CDT::Point_2(coord.x, coord.y));

		_CDT::initializeID(cdt);
		_CDT::discoverComponents(cdt);

		_CDT::Vertex_handle vh;
		bool in_domain = false;

		for (_CDT::CDT::Finite_faces_iterator fit = cdt.finite_faces_begin();
		     fit != cdt.finite_faces_end();
		     ++fit) {
			for (int i = 0; i < 3; i++) {
				_CDT::Point_2 p(coord.x, coord.y);

				if (fit->vertex(i)->point() == p) {
					vh = fit->vertex(i);

					printf("vertex (%d/%d) found, finite: %d\n", coord.x, coord.y, !cdt.is_infinite(vh));

					if (fit->is_in_domain()) {
						in_domain = true;
					}

					break;
				}

				if (vh != _CDT::Vertex_handle()) {
					break;
				}
			}
		}

		assert(vh != _CDT::Vertex_handle());

		// TODO: why?
		//cdt.remove_incident_constraints(vh);
		cdt.remove(vh);

		_CDT::initializeID(cdt);
		_CDT::discoverComponents(cdt);

		return in_domain;
	}

	void insert(Coord2D const &coord)
	{
		cdt.insert(_CDT::Point_2(coord.x, coord.y));

		_CDT::initializeID(cdt);
		_CDT::discoverComponents(cdt);
	}

	void remove(Coord2D const &coord)
	{
		_CDT::Vertex_handle vh;
		bool in_domain = false;

		for (_CDT::CDT::Finite_faces_iterator fit = cdt.finite_faces_begin();
		     fit != cdt.finite_faces_end();
		     ++fit) {

			for (int i = 0; i < 3; i++) {
				_CDT::Point_2 p = fit->vertex(i)->point();
				Coord2D c(p.x(), p.y());

				if (c == coord) {
					vh = fit->vertex(i);
					in_domain = fit->is_in_domain();
					break;
				}
			}

			if (vh != _CDT::Vertex_handle()) {
				break;
			}
		}

		if (vh != _CDT::Vertex_handle()) {
			printf("point to be removed was found in CDT (in domain: %d)\n", in_domain);
			cdt.remove(vh);

			_CDT::initializeID(cdt);
			_CDT::discoverComponents(cdt);
		} else {
			printf("point to be removed was NOT found in CDT\n");
		}
	}
};

RoomImage::RoomImage(std::string const &filename)
	: Image(filename),
	  p(new RIPIMPL(data().data(), width(), height(), type() == IMAGE_TYPE_RGB ? 3 : 4))
{
	enum CoordType {
		OUTSIDE,
		WALL_OR_OBJECT,
		INSIDE
	};

	typedef std::map<Coord2D, int> CoordTypesMap;

	CoordTypesMap coordTypes;

	// stamp all coordinates with the appropriate type
	for (unsigned int y = 0; y < p->height; y++) {
		for (unsigned int x = 0; x < p->width; x++) {
			Coord2D coord(x, y);

			if (p->isWhite(coord)) {
				coordTypes[coord] = OUTSIDE;
			} else if (p->isBlack(coord)) {
				coordTypes[coord] = WALL_OR_OBJECT;
			} else {
				coordTypes[coord] = INSIDE;
			}
		}
	}

	std::set<Coord2D> insideCoords;

	// first find all coordinates inside the room
	for (CoordTypesMap::const_iterator it = coordTypes.begin(); it != coordTypes.end(); it++) {
		if (it->second == INSIDE) {
			std::vector<Coord2D> neighbours = p->createNeighbours(it->first);

			bool neighboursInside = true;

			// expand neighbours
			for (std::vector<Coord2D>::const_iterator nit = neighbours.begin(); nit != neighbours.end(); nit++) {
				// find the type of the neighbour coordinate
				CoordTypesMap::const_iterator foundCoordType = coordTypes.find(*nit);

				assert(foundCoordType != coordTypes.end());

				if (foundCoordType->second != INSIDE) {
					neighboursInside = false;
					break;
				}
			}

			if (!neighboursInside) {
				// not all neighbours are inside, this one is an edge vertex
				insideCoords.insert(it->first);
			}
		}
	}

	enum DirectionType {
		WEST,
		SOUTH,
		EAST,
		NORTH,
	};

	ConvexPolygon2D currentPolygon;
	int currentDirection = WEST;
	Coord2D coord = *insideCoords.begin();
	currentPolygon.insert(coord);

	while (!insideCoords.empty()) {
		bool hasWestNeighbours = coord.x > 0;
		bool hasSouthNeighbours = coord.y < p->height - 1;
		bool hasEastNeighbours = coord.x < p->width - 1;
		bool hasNorthNeighbours = coord.y > 0;

		std::set<Coord2D>::const_iterator insideNeighbours[8];

		for (int i = 0; i < 8; i++) {
			insideNeighbours[0] == insideCoords.end();
		}

		if (hasWestNeighbours) {
			insideNeighbours[0] = insideCoords.find(Coord2D(coord.x - 1, coord.y));
		}

		if (hasSouthNeighbours) {
			insideNeighbours[1] = insideCoords.find(Coord2D(coord.x, coord.y + 1));
		}

		if (hasEastNeighbours) {
			insideNeighbours[2] = insideCoords.find(Coord2D(coord.x + 1, coord.y));
		}


		if (hasNorthNeighbours) {
			insideNeighbours[3] = insideCoords.find(Coord2D(coord.x, coord.y - 1));
		}

		std::set<Coord2D>::const_iterator newCoord = insideCoords.end();
		bool switchedDirection = false;

		// now do the work
		switch (currentDirection) {
			case WEST: {
				if (insideNeighbours[WEST] != insideCoords.end()) {
					newCoord = insideNeighbours[WEST];
					break;
				}

				int possibleDirections[2] = { NORTH, SOUTH };

				for (int i = 0; i < 2; i++) {
					int direction = possibleDirections[i];

					if (insideNeighbours[direction] != insideCoords.end()) {
						newCoord = insideNeighbours[direction];
						switchedDirection = true;
						currentDirection = direction;
						break;
					}
				}

				break;
			}

			case SOUTH: {
				if (insideNeighbours[SOUTH] != insideCoords.end()) {
					newCoord = insideNeighbours[SOUTH];
					break;
				}

				int possibleDirections[2] = { WEST, EAST };

				for (int i = 0; i < 2; i++) {
					int direction = possibleDirections[i];

					if (insideNeighbours[direction] != insideCoords.end()) {
						newCoord = insideNeighbours[direction];
						switchedDirection = true;
						currentDirection = direction;
						break;
					}
				}

				break;
			}

			case EAST: {
				if (insideNeighbours[EAST] != insideCoords.end()) {
					newCoord = insideNeighbours[EAST];
					break;
				}

				int possibleDirections[2] = { SOUTH, NORTH };

				for (int i = 0; i < 2; i++) {
					int direction = possibleDirections[i];

					if (insideNeighbours[direction] != insideCoords.end()) {
						newCoord = insideNeighbours[direction];
						switchedDirection = true;
						currentDirection = direction;
						break;
					}
				}

				break;
			}

			case NORTH: {
				if (insideNeighbours[NORTH] != insideCoords.end()) {
					newCoord = insideNeighbours[NORTH];
					break;
				}

				int possibleDirections[2] = { WEST, EAST };

				for (int i = 0; i < 2; i++) {
					int direction = possibleDirections[i];

					if (insideNeighbours[direction] != insideCoords.end()) {
						newCoord = insideNeighbours[direction];
						switchedDirection = true;
						currentDirection = direction;
						break;
					}
				}

				break;
			}

		}

		insideCoords.erase(coord);

		if (switchedDirection) {
			currentPolygon.insert(coord);
		}

		if (newCoord == insideCoords.end()) {
			roomBoundaries_.push_back(currentPolygon);
			currentPolygon = ConvexPolygon2D();

			if (insideCoords.empty()) {
				break;
			}

			coord = *insideCoords.begin();
		} else {
			coord = *newCoord;
		}
	}

	recreateConvexCCWRoomPolygons();
}

bool RoomImage::checkWaypoint(Coord2D const &coord) const
{
	return p->check(coord);
}

void RoomImage::insertWaypoint(Coord2D const &coord)
{
	p->insert(coord);
	p->createPolygons(convexCCWRoomPolygons_);
}

void RoomImage::removeWaypoint(Coord2D const &coord)
{
	p->remove(coord);
	p->createPolygons(convexCCWRoomPolygons_);
}

RoomImage::ConvexCCWRoomPolygons const &RoomImage::convexCCWRoomPolygons() const
{
	return convexCCWRoomPolygons_;
}

void RoomImage::recreateConvexCCWRoomPolygons()
{
	convexCCWRoomPolygons_.clear();
	p->cdt.clear();

	for (ConvexCCWRoomPolygons::const_iterator it = roomBoundaries_.begin();
	     it != roomBoundaries_.end();
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

	p->createPolygons(convexCCWRoomPolygons_);
}
