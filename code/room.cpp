#include "algo.h"
#include "room.h"
#include "roomimage.h"
#include "stats.h"
#include "triangulation.h"

#include <set>

#include <cassert>
#include <cmath>
#include <cstdlib>

#include <QtCore/QElapsedTimer>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>
#include <QtWidgets/QTextEdit>

#include <stdio.h>

namespace
{

long randomAtMost(long max);

long randomAtMost(long max)
{
	unsigned long num_bins = static_cast<unsigned long>(max) + 1;
	unsigned long num_rand = static_cast<unsigned long>(RAND_MAX) + 1;
	unsigned long bin_size = num_rand / num_bins;
	unsigned long defect = num_rand % num_bins;

	int x;

	while (true) {
		x = std::rand();

		if (num_rand - defect > static_cast<unsigned long>(x)) {
			break;
		}
	}

	return x / bin_size;
}

} // end of private namespace

struct Room::RoomImpl
{
	RoomImpl(std::string const &filename, unsigned char distance, Stats *stats, QTextEdit *statusText, QTextEdit *helpText)
		: image(new RoomImage(filename)),
		  statusText_(statusText),
		  helpText_(helpText),
		  stats(stats),
		  algorithm(Room::Dijkstra)
	{
		bytes = image->data().data();
		width = image->width();
		height = image->height();
		stride = image->type() == Image::IMAGE_TYPE_RGB ? 3 : 4;

		this->distance = distance;

		// this array below distinguishes between big room (first) and holes
		std::vector<Polygon2D> borderPolygons;

		image->getBorderPolygons(distance, borderPolygons, doorPolygons_);

		// this array doesn't distinguish between big room and holes
		//std::vector<Edge> constraints = roomTriangulation.getConstrainedEdges();

		QElapsedTimer timer;
		timer.start();

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

		stats->lastRoomTriangulationCalculation = timer.elapsed();

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

		setDoorWaypoints();

		while (true) {
			int randX = randomAtMost(width - 1);
			int randY = randomAtMost(height - 1);

			if (setStartpoint(Coord2D(randX, randY))) {
				break;
			}
		}

		while (true) {
			int randX = randomAtMost(width - 1);
			int randY = randomAtMost(height - 1);

			if (setEndpoint(Coord2D(randX, randY))) {
				break;
			}
		}
	}

	~RoomImpl()
	{
		delete image;
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
	std::set<Coord2D> waypoints;
	QTextEdit *statusText_;
	QTextEdit *helpText_;
	std::vector<Polygon2D> doorPolygons_;
	Stats *stats;
	Room::Algorithm algorithm;

	bool insert(Coord2D const &coord)
	{
		if (coord == startpoint || coord == endpoint) {
			statusText_->setText(statusText_->tr("Waypoint (%1/%2) is startpoint or endpoint, can't insert.\n").arg(coord.x).arg(coord.y));
			return false;
		}

		if (triangulation.pointIsVertex(coord)) {
			statusText_->setText(statusText_->tr("Waypoint (%1/%2) already inserted, can't insert.\n").arg(coord.x).arg(coord.y));
			return false;
		}

		if (!roomTriangulation.inDomain(coord.x, coord.y)) {
			statusText_->setText(statusText_->tr("Waypoint (%1/%2) outside domain, can't insert.\n").arg(coord.x).arg(coord.y));
			return false;
		}

		triangulation.insert(coord);
		waypoints.insert(coord);
		assert(triangulation.pointIsVertex(coord));

		return true;
	}

	bool remove(Coord2D const &coord)
	{
		if (coord == startpoint || coord == endpoint) {
			statusText_->setText(statusText_->tr("Waypoint (%1/%2) is startpoint or endpoint, can't remove.\n").arg(coord.x).arg(coord.y));
			return false;
		}

		if (!triangulation.pointIsVertex(coord)) {
			statusText_->setText(statusText_->tr("Waypoint (%1/%2) not inserted, can't remove.\n").arg(coord.x).arg(coord.y));
			return false;
		}

		triangulation.remove(coord);
		waypoints.erase(coord);
		assert(!triangulation.pointIsVertex(coord));

		return true;
	}

	bool setStartpoint(Coord2D const &coord)
	{
		if (startpoint == coord) {
			return true;
		}

		if (endpoint == coord) {
			statusText_->setText(statusText_->tr("Startpoint (%1/%2) is endpoint, can't insert.\n").arg(coord.x).arg(coord.y));
			return false;
		}

		if (!roomTriangulation.inDomain(coord.x, coord.y)) {
			statusText_->setText(statusText_->tr("Startpoint (%1/%2) outside domain, can't insert.\n").arg(coord.x).arg(coord.y));
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
			statusText_->setText(statusText_->tr("Endpoint (%1/%2) is startpoint, can't insert.\n").arg(coord.x).arg(coord.y));
			return false;
		}

		if (!roomTriangulation.inDomain(coord.x, coord.y)) {
			statusText_->setText(statusText_->tr("Endpoint (%1/%2) outside domain, can't insert.\n").arg(coord.x).arg(coord.y));
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

	void setNodes(int amount)
	{
		for (int i = 0; i < amount; i++) {
			int randX = randomAtMost(width - 1);
			int randY = randomAtMost(height - 1);

			if (!insert(Coord2D(randX, randY))) {
				i--;
				continue;
			}
		}

		setDoorWaypoints();
	}

	void setDoorWaypoints()
	{
		for (std::vector<Polygon2D>::const_iterator it = doorPolygons_.begin();
		     it != doorPolygons_.end();
		     it++) {
			assert(it->size() == 2);

			Coord2D first = (*it)[0];
			Coord2D second = (*it)[1];

			int diffX = static_cast<int>(first.x) - static_cast<int>(second.x);
			int diffY = static_cast<int>(first.y) - static_cast<int>(second.y);

			if (diffX < 0) {
				diffX *= -1;
			}

			if (diffY < 0) {
				diffY *= -1;
			}

			unsigned int newX = std::min(first.x, second.x) + diffX / 2;
			unsigned int newY = std::min(first.y, second.y) + diffY / 2;

			insert(Coord2D(newX, newY));
		}
	}

	bool pointInside(float x, float y) const
	{
		return roomTriangulation.inDomain(x, y);
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

				continue;
			}

			if (!intersectionPoints.empty()) {
				float x = intersectionPoints[0].x;
				float y = intersectionPoints[0].y;

				if ((x == checkEdge.start.x && y == checkEdge.start.y) ||
				    (x == checkEdge.end.x && y == checkEdge.end.y)) {
					continue;
				}

				return true;
			}
		}

		return false;
	}

	void setAlgorithm(Room::Algorithm algorithm)
	{
		this->algorithm = algorithm;
	}

	Room::Algorithm getAlgorithm() const
	{
		return algorithm;
	}

	std::vector<Coord2D> generatePath()
	{
		NeighboursMap neighbours = triangulation.getNeighbours();

		for (NeighboursMap::iterator it = neighbours.begin(); it != neighbours.end(); it++) {
			for (std::set<Coord2D>::iterator nit = it->second.begin(); nit != it->second.end();) {
				Edge checkEdge(it->first, *nit);

				if (intersectsEdges(checkEdge)) {
					it->second.erase(nit++);
				} else {
					++nit;
				}
			}
		}

		std::vector<Coord2D> generatedPath;

		stats->lastUsedAlgorithm = algorithm;

		QElapsedTimer timer;

		timer.start();

		if (algorithm == Room::Dijkstra) {
			generatedPath = dijkstra(neighbours, startpoint, endpoint);
		} else {
			generatedPath = astar(neighbours, startpoint, endpoint);
		}

		stats->lastPathCalculation = timer.elapsed();

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

	bool loadProject(QXmlStreamReader *reader)
	{
		if (!reader->isStartElement() || reader->name().toString() != "room") {
			return false;
		}

		reader->readNextStartElement();

		while (true) {
			if (reader->isStartElement()) {
				QString name = reader->name().toString();
				QXmlStreamAttributes attributes = reader->attributes();
				unsigned int xValue = attributes.value("x").toString().toUInt();
				unsigned int yValue = attributes.value("y").toString().toUInt();

				if (name == "startpoint") {
					setStartpoint(Coord2D(xValue, yValue));
				} else if (name == "endpoint") {
					setEndpoint(Coord2D(xValue, yValue));
				} else if (name == "waypoint") {
					insert(Coord2D(xValue, yValue));
				}
			} else if (reader->isEndElement() && reader->name().toString() == "room") {
				return true;
			}

			reader->readNext();

			if (!reader->isEndElement()) {
				return false;
			}

			reader->readNextStartElement();
		}

		return false;
	}

	bool saveProject(QXmlStreamWriter *writer) const
	{
		writer->writeStartElement("", "room");

		writer->writeEmptyElement("", "startpoint");
		writer->writeAttribute("", "x", QString::number(startpoint.x));
		writer->writeAttribute("", "y", QString::number(startpoint.y));

		writer->writeEmptyElement("", "endpoint");
		writer->writeAttribute("", "x", QString::number(endpoint.x));
		writer->writeAttribute("", "y", QString::number(endpoint.y));

		for (std::set<Coord2D>::const_iterator it = waypoints.begin();
		     it != waypoints.end();
		     ++it) {
			writer->writeEmptyElement("", "waypoint");
			writer->writeAttribute("", "x", QString::number(it->x));
			writer->writeAttribute("", "y", QString::number(it->y));
		}

		writer->writeEndElement();

		return !writer->hasError();
	}
};


















Room::Room(std::string const &filename, unsigned char distance, Stats *stats, QTextEdit *statusText, QTextEdit *helpText)
	: p(new RoomImpl(filename, distance, stats, statusText, helpText))
{
}

Room::~Room()
{
	delete p;
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

void Room::setNodes(int amount)
{
	p->setNodes(amount);
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
	p->waypoints.clear();
}

bool Room::hasWaypoint(Coord2D const &coord) const
{
	return p->triangulation.pointIsVertex(coord);
}

std::set<Coord2D> const &Room::getWaypoints() const
{
	return p->waypoints;
}

void Room::setAlgorithm(Algorithm algorithm)
{
	p->setAlgorithm(algorithm);
}

Room::Algorithm Room::getAlgorithm() const
{
	return p->getAlgorithm();
}

NeighboursMap Room::getNeighbours() const
{
	return p->triangulation.getNeighbours();
}

std::vector< std::vector<Edge> > Room::getEdges() const
{
	return p->edges;
}

bool Room::pointInside(float x, float y) const
{
	return p->pointInside(x, y);
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

bool Room::loadProject(QXmlStreamReader *reader)
{
	return p->loadProject(reader);
}

bool Room::saveProject(QXmlStreamWriter *writer) const
{
	return p->saveProject(writer);
}
