#include "triangulation.h"

#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>

#include <list>

namespace {
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

	typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
	typedef Kernel::Point_2 Point;
	typedef CGAL::Triangulation_vertex_base_2<Kernel> VertexBase;
	typedef CGAL::Constrained_triangulation_face_base_2<Kernel> CTFaceBase;
	typedef CDTFaceBase<Kernel, CTFaceBase> TDSFaceBase;
	typedef CGAL::Triangulation_data_structure_2<VertexBase, TDSFaceBase> TDS;
	typedef CGAL::Exact_predicates_tag CDTIntersectionTag;
	typedef CGAL::Constrained_Delaunay_triangulation_2<Kernel, TDS, CDTIntersectionTag> CDT;
	typedef CGAL::Delaunay_triangulation_2<Kernel> DT;
} // end of private namespace

class DelaunayTriangulation::DelaunayTriangulationImpl
{
public:
	DT dt;

	void clear()
	{
		dt.clear();
	}

	std::set<Coord2D> list() const
	{
		std::set<Coord2D> waypoints;

		for (DT::Finite_vertices_iterator vi = dt.finite_vertices_begin(); vi != dt.finite_vertices_end(); vi++) {
			Coord2D c = Coord2D(vi->point().x(), vi->point().y());

			waypoints.insert(c);
		}

		return waypoints;
	}

	NeighboursMap getNeighbours() const
	{
		NeighboursMap neighbours;

		for (DT::Finite_vertices_iterator vi = dt.finite_vertices_begin(); vi != dt.finite_vertices_end(); vi++) {
			Coord2D c = Coord2D(vi->point().x(), vi->point().y());

			DT::Edge_circulator ec = dt.incident_edges(vi);
			DT::Edge_circulator ec_done = ec;
			std::set<Coord2D> thisNeighbours;

			do {
				bool addNeighbour = false;

				if (!dt.is_infinite(ec)) {
					addNeighbour = true;
				}

				if (addNeighbour) {
					DT::Segment segment = dt.segment(ec);
					Coord2D c0(segment.point(0).x(), segment.point(0).y());
					Coord2D c1(segment.point(1).x(), segment.point(1).y());


					thisNeighbours.insert(c0 == c ? c1 : c0);
				}

				ec++;
			} while (ec != ec_done);

			neighbours[c] = thisNeighbours;
		}

		return neighbours;
	}

	void insert(Coord2D const &coord)
	{
		dt.insert(DT::Point(coord.x, coord.y));
	}

	void remove(Coord2D const &coord)
	{
		DT::Point p(coord.x, coord.y);

		for (DT::Finite_faces_iterator fit = dt.finite_faces_begin();
		     fit != dt.finite_faces_end();
		     ++fit) {
			for (int i = 0; i < 3; i++) {
				if (fit->vertex(i)->point() == p) {
					dt.remove(fit->vertex(i));
					return;
				}
			}
		}

		assert(false);
	}

	bool pointIsVertex(Coord2D const &coord)
	{
		DT::Point p(coord.x, coord.y);

		for (DT::Finite_faces_iterator fit = dt.finite_faces_begin();
		     fit != dt.finite_faces_end();
		     ++fit) {
			for (int i = 0; i < 3; i++) {
				if (fit->vertex(i)->point() == p) {
					return true;
				}
			}
		}

		return false;
	}

	std::vector<Triangle> getTriangulation()
	{
		std::vector<Triangle> triangulation;

		for (DT::Finite_faces_iterator fit = dt.finite_faces_begin();
		     fit != dt.finite_faces_end();
		     ++fit) {
			Triangle triangle;

			DT::Point p0 = fit->vertex(0)->point();
			DT::Point p1 = fit->vertex(1)->point();
			DT::Point p2 = fit->vertex(2)->point();

			triangle[0] = Coord2D(p0.x(), p0.y());
			triangle[1] = Coord2D(p1.x(), p1.y());
			triangle[2] = Coord2D(p2.x(), p2.y());

			triangulation.push_back(triangle);
		}

		return triangulation;
	}
};

class ConstrainedDelaunayTriangulation::ConstrainedDelaunayTriangulationImpl
{
public:
	CDT cdt;

	void clear()
	{
		cdt.clear();
	}

	std::set<Coord2D> list() const
	{
		std::set<Coord2D> waypoints;

		for (CDT::Finite_vertices_iterator vi = cdt.finite_vertices_begin(); vi != cdt.finite_vertices_end(); vi++) {
			Coord2D c = Coord2D(vi->point().x(), vi->point().y());

			waypoints.insert(c);
		}

		return waypoints;
	}

	NeighboursMap getNeighbours() const
	{
		NeighboursMap neighbours;

		for (CDT::Finite_vertices_iterator vi = cdt.finite_vertices_begin(); vi != cdt.finite_vertices_end(); vi++) {
			Coord2D c = Coord2D(vi->point().x(), vi->point().y());

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


					thisNeighbours.insert(c0 == c ? c1 : c0);
				}

				ec++;
			} while (ec != ec_done);

			neighbours[c] = thisNeighbours;
		}

		return neighbours;
	}

	void insert(Coord2D const &coord)
	{
		cdt.insert(CDT::Point(coord.x, coord.y));

		mark();
	}

	void remove(Coord2D const &coord)
	{
		CDT::Point p(coord.x, coord.y);

		for (CDT::Finite_faces_iterator fit = cdt.finite_faces_begin();
		     fit != cdt.finite_faces_end();
		     ++fit) {
			for (int i = 0; i < 3; i++) {
				if (fit->vertex(i)->point() == p) {
					cdt.remove(fit->vertex(i));
					mark();
					return;
				}
			}
		}

		assert(false);
	}

	bool pointIsVertex(Coord2D const &coord)
	{
		CDT::Point p(coord.x, coord.y);

		for (CDT::Finite_faces_iterator fit = cdt.finite_faces_begin();
		     fit != cdt.finite_faces_end();
		     ++fit) {
			for (int i = 0; i < 3; i++) {
				if (fit->vertex(i)->point() == p) {
					return true;
				}
			}
		}

		return false;
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

	std::vector<Triangle> getTriangulation()
	{
		std::vector<Triangle> triangulation;

		for (CDT::Finite_faces_iterator fit = cdt.finite_faces_begin();
		     fit != cdt.finite_faces_end();
		     ++fit) {
			Triangle triangle;

			if (fit->getInDomain()) {
				CDT::Point p0 = fit->vertex(0)->point();
				CDT::Point p1 = fit->vertex(1)->point();
				CDT::Point p2 = fit->vertex(2)->point();

				triangle[0] = Coord2D(p0.x(), p0.y());
				triangle[1] = Coord2D(p1.x(), p1.y());
				triangle[2] = Coord2D(p2.x(), p2.y());

				triangulation.push_back(triangle);
			}
		}

		return triangulation;
	}

	void insertConstraints(std::vector<Coord2D> const &points)
	{
		Coord2D p, q;
		CDT::Vertex_handle vh, wh;
		std::vector<Coord2D>::const_iterator it = points.begin();

		vh = cdt.insert(CDT::Point(it->x, it->y));
		p = *it;
		++it;

		for(; it != points.end(); ++it) {
			q = *it;
			if (p != q) {
				wh = cdt.insert(CDT::Point(it->x, it->y));
				cdt.insert_constraint(vh, wh);
				vh = wh;
				p = q;
			} else {
				std::cout << "duplicate point: " << p.x << '/' << p.y << std::endl;
			}
		}

		mark();
	}

	void mark()
	{
		// mark faces in/out of domain
		for (CDT::All_faces_iterator it = cdt.all_faces_begin(); it != cdt.all_faces_end(); ++it) {
			it->setCounter(-1);
		}

		discoverComponents();
	}

	void discoverComponents()
	{
		if (cdt.dimension() != 2) {
			return;
		}

		int index = 0;
		std::list<CDT::Edge> border;
		discoverComponent(cdt.infinite_face(), index++, border);

		while (!border.empty()) {
			CDT::Edge edge = border.front();
			border.pop_front();
			CDT::Face_handle neigbor = edge.first->neighbor(edge.second);

			if (neigbor->getCounter() == -1) {
				discoverComponent(neigbor, edge.first->getCounter() + 1, border);
			}
		}
	}

	void discoverComponent(CDT::Face_handle start, int index, std::list<CDT::Edge>& border)
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
};

DelaunayTriangulation::DelaunayTriangulation()
	: p(new DelaunayTriangulationImpl)
{
}

NeighboursMap DelaunayTriangulation::getNeighbours() const
{
	return p->getNeighbours();
}

std::vector<Triangle> DelaunayTriangulation::getTriangulation() const
{
	return p->getTriangulation();
}

void DelaunayTriangulation::insert(Coord2D const &coord)
{
	p->insert(coord);
}

void DelaunayTriangulation::remove(Coord2D const &coord)
{
	p->remove(coord);
}

std::set<Coord2D> DelaunayTriangulation::list() const
{
	return p->list();
}

void DelaunayTriangulation::clear()
{
	p->clear();
}

bool DelaunayTriangulation::pointIsVertex(Coord2D const &coord)
{
	return p->pointIsVertex(coord);
}

ConstrainedDelaunayTriangulation::ConstrainedDelaunayTriangulation()
	: p(new ConstrainedDelaunayTriangulationImpl)
{
}

NeighboursMap ConstrainedDelaunayTriangulation::getNeighbours() const
{
	return p->getNeighbours();
}

std::vector<Triangle> ConstrainedDelaunayTriangulation::getTriangulation() const
{
	return p->getTriangulation();
}

void ConstrainedDelaunayTriangulation::insert(Coord2D const &coord)
{
	p->insert(coord);
}

void ConstrainedDelaunayTriangulation::remove(Coord2D const &coord)
{
	p->remove(coord);
}

std::set<Coord2D> ConstrainedDelaunayTriangulation::list() const
{
	return p->list();
}

void ConstrainedDelaunayTriangulation::clear()
{
	p->clear();
}

bool ConstrainedDelaunayTriangulation::inDomain(Coord2D const &coord)
{
	return p->inDomain(coord);
}

bool ConstrainedDelaunayTriangulation::pointIsVertex(Coord2D const &coord)
{
	return p->pointIsVertex(coord);
}

void ConstrainedDelaunayTriangulation::insertConstraints(std::vector<Coord2D> const &points)
{
	p->insertConstraints(points);
}
