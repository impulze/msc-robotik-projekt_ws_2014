#define GL_GLEXT_PROTOTYPES

#include "algo.h"
#include "coord.h"
#include "drawing.h"
#include "room.h"
#include "roomimage.h"
#include "texture.h"

#include <GL/gl.h>
#include <GL/glu.h>
#include <IL/il.h>
#include <IL/ilu.h>

#include <cassert>
#include <cmath>
#include <ctime>
#include <limits>
#include <set>
#include <stdexcept>
#include <vector>

#include <QtCore/QXmlStreamReader>
#include <QtCore/QXmlStreamWriter>

#if MEASURE
#include <sys/time.h>
#include <cstdio>
#endif

#define ROBOT_DIAMETER 5

// private namespace
namespace
{

void throwErrorFromGLError();

void throwErrorFromGLError()
{
	GLenum glError = glGetError();

	if (glError != GL_NO_ERROR) {
		const char *string = reinterpret_cast<const char *>(gluErrorString(glError));
		throw std::runtime_error(string);
	}
}

} // end of private namespace

class Drawing::DrawingImpl
{
public:
	DrawingImpl();
	~DrawingImpl();

	void fromImage(const char *name);

	void updateRoom();

	void setNodes(int amount);
	void setWaypointModification(Drawing::WaypointModification modification);
	void setOption(Drawing::Option option, bool enabled);
	void mouseClick(int x, int y, Drawing::MouseButton button);

	void initialize();
	void paint();
	void resize(int width, int height);

	void freeTexture();
	bool checkNode(int x, int y);
	bool delNode(int x, int y);
	void drawPoint(int x, int y);
	void drawCross(int x, int y);
	bool getCoordFromMouseClick(int x, int y, Coord2D &coord);
	void showNeighbours(Coord2D const &coord);

	bool loadProject(QXmlStreamReader *reader);
	bool saveProject(QXmlStreamWriter *writer) const;

	Drawing::WaypointModification waypointModification;
	GLint viewport[4];
	GLuint circleVBO;
	GLuint crossVBO;
	Room *room;
	Texture *texture;
	bool show_[5];
	std::vector<Triangle> triangulation;
	std::vector<Triangle> roomTriangulation;
	std::vector<Coord2D> path;
	std::vector< Coord2DTemplate<float> > pathPoints;
	std::set< Coord2DTemplate<float> > pathCollisions;
	Coord2D neighbourToShow;
	// mapping of a coord and intersection value
	std::map<Coord2D, bool> neighbourToShowNeighbours;
};

Drawing::DrawingImpl::DrawingImpl()
	: waypointModification(Drawing::WaypointNoMod),
	  room(0),
	  texture(0)
{
	std::srand(std::time(0));

	for (size_t i = 0; i < sizeof show_ / sizeof *show_; i++) {
		show_[i] = false;
	}
}

Drawing::DrawingImpl::~DrawingImpl()
{
	freeTexture();
	glDeleteBuffers(1, &circleVBO);
}

void Drawing::DrawingImpl::fromImage(const char *name)
{
	room = new Room(name, ROBOT_DIAMETER);

	if (room->image().width() > static_cast<unsigned int>(std::numeric_limits<int>::max()) ||
	    room->image().height() > static_cast<unsigned int>(std::numeric_limits<int>::min())) {
		delete room;
		throw std::runtime_error("OpenGL cannot draw this texture.");
	}

	roomTriangulation = room->getRoomTriangulation();
}

void Drawing::DrawingImpl::updateRoom()
{
	triangulation = room->getTriangulation();
	path = room->generatePath();

	if (path.begin()->x > path.rbegin()->x) {
		std::reverse(path.begin(), path.end());
	}

	pathPoints = catmullRom(path, 50);

	pathCollisions.clear();

	for (std::vector< Coord2DTemplate<float> >::const_iterator it = pathPoints.begin(); it != pathPoints.end(); ++it) {
		if (!room->pointInside(it->x, it->y)) {
			pathCollisions.insert(*it);
		}
	}
}

void Drawing::DrawingImpl::setNodes(int amount)
{
	room->clearWaypoints();
	room->setNodes(amount);

	updateRoom();
}

void Drawing::DrawingImpl::setWaypointModification(Drawing::WaypointModification modification)
{
	waypointModification = modification;
}

void Drawing::DrawingImpl::setOption(Drawing::Option option, bool enabled)
{
	switch (option) {
		case Drawing::ShowTriangulation:
		case Drawing::ShowRoomTriangulation:
		case Drawing::ShowWaypoints:
		case Drawing::ShowPath:
		case Drawing::ShowNeighbours:
			show_[option] = enabled;
			break;
	}
}

void Drawing::DrawingImpl::mouseClick(int x, int y, Drawing::MouseButton button)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, texture->width(), 0, texture->height(), -1.0f, 4.0f);
	throwErrorFromGLError();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Texture
	glTranslatef(0.0f, 0.0f, -3.5f);

	GLdouble modelview[16], projection[16];
	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);

	GLfloat z;
	glReadPixels(x, viewport[3] - y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);

	GLdouble posX, posY, posZ;
	gluUnProject(x, y, z, modelview, projection, viewport, &posX, &posY, &posZ);
	throwErrorFromGLError();

	x = posX;
	y = posY;

	if (button == Drawing::LeftMouseButton) {
		bool changed = false;

		switch (waypointModification) {
			case Drawing::WaypointAdd:
				changed = room->insertWaypoint(Coord2D(x, y));
				break;

			case Drawing::WaypointDelete:
				changed = delNode(x, y);
				break;

			case Drawing::WaypointStart:
				changed = room->setStartpoint(Coord2D(x, y));
				break;

			case Drawing::WaypointEnd:
				changed = room->setEndpoint(Coord2D(x, y));
				break;
		
			default:
				break;
		}

		if (show_[ShowNeighbours]) {
			Coord2D coord;

			if (!getCoordFromMouseClick(x, y, coord)) {
				return;
			}

			showNeighbours(coord);
		}

		if (changed) {
			updateRoom();
		}
	}
}

void Drawing::DrawingImpl::initialize()
{
	glEnable(GL_DEPTH_TEST);

	GLdouble vertices[(300 + 1) * 2];

	vertices[0] = 0; vertices[1] = 0;

	for (int i = 0; i < 300; i++) {
		double angle = 2.0 * M_PI * (1.0 * i) / 300;
		double drawX = (ROBOT_DIAMETER / 2.5) * std::cos(angle);
		double drawY = (ROBOT_DIAMETER / 2.5) * std::sin(angle);

		vertices[(i + 1) * 2] = drawX;
		vertices[(i + 1) * 2 + 1] = drawY;
	}

	glGenBuffersARB(1, &circleVBO);
	throwErrorFromGLError();

	try {
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, circleVBO);
		throwErrorFromGLError();
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof vertices, vertices, GL_STATIC_DRAW);
		throwErrorFromGLError();
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		throwErrorFromGLError();
	} catch (...) {
		glDeleteBuffers(1, &circleVBO);
	}

	{
		vertices[0] = -3; vertices[1] = -3;
		vertices[2] = 3; vertices[3] = 3;
		vertices[4] = -3; vertices[5] = 3;
		vertices[6] = 3; vertices[7] = -3;
	}

	glGenBuffersARB(2, &crossVBO);
	throwErrorFromGLError();

	try {
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, crossVBO);
		throwErrorFromGLError();
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof *vertices * 8, vertices, GL_STATIC_DRAW);
		throwErrorFromGLError();
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		throwErrorFromGLError();
	} catch (...) {
		glDeleteBuffers(2, &crossVBO);
	}

	texture = new Texture(room->image());

	updateRoom();
}

void Drawing::DrawingImpl::paint()
{
	if (texture == 0) {
		return;
	}

#if MEASURE
	struct timeval fir; gettimeofday(&fir, NULL);
#endif

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// default projection: glOrtho(l=-1,r=1,b=-1,t=1,n=1,f=-1)
	glOrtho(0, texture->width(), 0, texture->height(), -1.0f, 4.0f);
	throwErrorFromGLError();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0f, 0.0f, -3.5f);

	// OpenGL origin = bottom-left
	// DevIL origin = top-left
	// => swap y coordinates in glTexCoord2f

	// Texture
	glEnable(GL_TEXTURE_2D);
	texture->bind();
	glBegin(GL_POLYGON);
	glTexCoord2i(0, 1);
	glVertex2i(0, 0);
	glTexCoord2i(0, 0);
	glVertex2i(0, texture->height());
	glTexCoord2i(1, 0);
	glVertex2i(texture->width(), texture->height());
	glTexCoord2i(1, 1);
	glVertex2i(texture->width(), 0);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	if (show_[ShowTriangulation]) {
		glTranslatef(0.0f, 0.0f, 0.2f);
		// light blue
		glColor3f(0.5f, 0.8f, 1.0f);
		glLineWidth(2.0f);
		for (std::vector<Triangle>::const_iterator it = triangulation.begin();
		     it != triangulation.end();
		     it++) {
			glBegin(GL_LINE_LOOP);

			for (int i = 0; i < 3; i++) {
				unsigned int x = (*it)[i].x;
				unsigned int y = texture->height() - 1 - (*it)[i].y;
				glVertex2i(x, y);
			}

			glEnd();
		}
	}

	if (show_[ShowRoomTriangulation]) {
		glTranslatef(0.0f, 0.0f, 0.2f);
		// lighter blue
		glColor3f(0.6f, 0.9f, 1.0f);
		glLineWidth(2.0f);
		for (std::vector<Triangle>::const_iterator it = roomTriangulation.begin();
		     it != roomTriangulation.end();
		     it++) {
			glBegin(GL_LINE_LOOP);

			for (int i = 0; i < 3; i++) {
				unsigned int x = (*it)[i].x;
				unsigned int y = texture->height() - 1 - (*it)[i].y;
				glVertex2i(x, y);
			}

			glEnd();
		}
	}

	// Waypoints
	glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_DOUBLE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (show_[ShowWaypoints]) {
		glTranslatef(0.0f, 0.0f, 0.2f);
		// yellow
		glColor3f(1.0f, 1.0f, 0.0f);
		std::set<Coord2D> const &waypoints = room->getWaypoints();
		for (std::set<Coord2D>::const_iterator it = waypoints.begin(); it != waypoints.end(); it++) {
			drawPoint(it->x, it->y);
		}
	}

#if 0
	for (std::vector< std::vector<Edge> >::const_iterator it = edges.begin(); it != edges.end(); it++) {
		unsigned char c1, c2, c3;

		c1 = rand() % 256;
		c2 = rand() % 256;
		c3 = rand() % 256;

		glColor3b(c1, c2, c3);
		glBegin(GL_LINES);
		for (std::vector<Edge>::const_iterator eit = it->begin(); eit != it->end(); eit++) {
			Edge edge = *eit;
			glVertex2f(edge.start.x, texture->height() - 1 - edge.start.y);
			glVertex2f(edge.end.x, texture->height() - 1 - edge.end.y);
		}
		glEnd();
	}
#endif

	for (std::map<Coord2D, bool>::const_iterator it = neighbourToShowNeighbours.begin();
	     it != neighbourToShowNeighbours.end();
	     it++) {
		if (!it->second) {
			glColor3b(0, 127, 0);
		} else {
			glColor3b(127, 0 ,0);
			glLineWidth(2.0f);
		}

		glBegin(GL_LINES);
		glVertex2f(neighbourToShow.x, texture->height() - 1 - neighbourToShow.y);
		glVertex2f(it->first.x, texture->height() - 1 - it->first.y);
		glEnd();
	}

	if (show_[ShowPath] && path.size() > 0) {
		glTranslatef(0.0f, 0.0f, 0.2f);
		// dark-yellow
		//glColor3f(0.7f, 0.7f, 0.0f);
		glColor3f(0.2f, 0.2f, 0.2f);
		glLineWidth(2.0f);
		glBegin(GL_LINE_STRIP);
		for (std::vector< Coord2DTemplate<float> >::const_iterator it = pathPoints.begin();
		     it != pathPoints.end();
		     ++it) {
			glVertex2f(it->x, texture->height() - 1 - it->y);
		}
		glEnd();
	}

	glTranslatef(0.0f, 0.0f, 0.2f);
	glColor3f(1.0f, 0.0f, 0.0f);
	drawPoint(room->getEndpoint().x, room->getEndpoint().y);

	glTranslatef(0.0f, 0.0f, 0.2f);
	glColor3f(0.0f, 1.0f, 0.0f);
	drawPoint(room->getStartpoint().x, room->getStartpoint().y);

	// Collisions
	if (!pathCollisions.empty()) {
		glBindBuffer(GL_ARRAY_BUFFER, crossVBO);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_DOUBLE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glColor3f(1.0f, 0.0f, 0.0f);

		for (std::set< Coord2DTemplate<float> >::const_iterator it = pathCollisions.begin(); it != pathCollisions.end(); ++it) {
			drawCross(it->x, it->y);
		}
	}

	glDisableClientState(GL_VERTEX_ARRAY);

#if MEASURE
	struct timeval sec; gettimeofday(&sec, NULL);
	struct timeval res; timersub(&sec, &fir, &res);
	std::printf("rendering: %lu sec %lu usec\n", res.tv_sec, res.tv_usec);
#endif
}

void Drawing::DrawingImpl::resize(int width, int height)
{
	viewport[0] = 0;
	viewport[1] = 0;
	viewport[2] = width;
	viewport[3] = height;
}

void Drawing::DrawingImpl::freeTexture()
{
	delete texture;
	delete room;

	room = 0;
	texture = 0;
}

bool Drawing::DrawingImpl::delNode(int x, int y)
{
	Coord2D coord;

	if (getCoordFromMouseClick(x, y, coord)) {
		return room->removeWaypoint(coord);
	}

	return false;
}

void Drawing::DrawingImpl::drawPoint(int x, int y)
{
#if COLLISION_DETECTION
		int left = std::max(0, x - ROBOT_DIAMETER / 2);
		int right = std::min(static_cast<int>(texture->width()) - 1, x + ROBOT_DIAMETER / 2);
		int bottom = std::max(0, y - ROBOT_DIAMETER / 2);
		int top = std::min(static_cast<int>(texture->height()) - 1, y + ROBOT_DIAMETER / 2);

		glRecti(left, bottom, right, top);
#else
		int offsetX = x;
		int offsetY = texture->height() - 1 - y;
		glTranslatef(offsetX, offsetY, 0.0f);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 301);
		glTranslatef(-offsetX, -offsetY, 0.0f);
#endif
}

void Drawing::DrawingImpl::drawCross(int x, int y)
{
	int offsetX = x;
	int offsetY = texture->height() - 1 - y;
	glTranslatef(offsetX, offsetY, 0.0f);
	glDrawArrays(GL_LINES, 0, 8);
	glTranslatef(-offsetX, -offsetY, 0.0f);
}

bool Drawing::DrawingImpl::getCoordFromMouseClick(int x, int y, Coord2D &coord)
{
	// the user clicked near or directly into the waypoint
	int left = std::max(0, x - ROBOT_DIAMETER / 2);
	int right = std::min(static_cast<int>(texture->width()) - 1, x + ROBOT_DIAMETER / 2);
	int bottom = std::max(0, y - ROBOT_DIAMETER / 2);
	int top = std::min(static_cast<int>(texture->height()) - 1, y + ROBOT_DIAMETER / 2);

	for (int i = bottom; i <= top; i++) {
		for (int j = left; j <= right; j++) {
			coord = Coord2D(j, i);

			if (room->hasWaypoint(coord)) {
				return true;
			}
		}
	}

	return false;
}

void Drawing::DrawingImpl::showNeighbours(Coord2D const &coord)
{
	NeighboursMap neighbours = room->getNeighbours();

	for (NeighboursMap::const_iterator it = neighbours.begin(); it != neighbours.end(); it++) {
		Coord2D thisCoord(it->first.x, it->first.y);

		if (coord == thisCoord) {
			neighbourToShow = coord;
			if (neighbourToShowNeighbours.empty()) {
				for (std::set<Coord2D>::const_iterator sit = it->second.begin(); sit != it->second.end(); sit++) {
					Edge edge(neighbourToShow, *sit);
					// store if this edge intersects any polygon boundary edge
					neighbourToShowNeighbours[*sit] = room->intersectsEdges(edge);
				}
			} else {
				neighbourToShowNeighbours.clear();
			}

			for (std::set<Coord2D>::const_iterator cit = it->second.begin(); cit != it->second.end(); cit++) {
				Coord2D thatCoord(cit->x, cit->y);

				double xDistance = static_cast<double>(thatCoord.x) - thisCoord.x;
				double yDistance = static_cast<double>(thatCoord.y) - thisCoord.y;
				double distance = std::sqrt(xDistance * xDistance + yDistance * yDistance);
			}
		}
	}
}

bool Drawing::DrawingImpl::loadProject(QXmlStreamReader *reader)
{
	if (!reader->isStartElement()) {
		return false;
	}

	if (reader->name().toString() != "drawing") {
		return false;
	}

	reader->readNextStartElement();

	if (reader->name().toString() != "image") {
		return false;
	}

	reader->readNext();
	fromImage(reader->text().toString().toStdString().c_str());
	reader->readNext();

	if (!reader->isEndElement() || reader->name().toString() != "image") {
		return false;
	}

	reader->readNextStartElement();

	std::map<int, bool> showMap;

	while (true) {
		if (reader->isStartElement()) {
			QString name = reader->name().toString();
			QXmlStreamAttributes attributes = reader->attributes();
			QString showValue = attributes.value("show").toString();
			bool showValueInt = showValue.toInt();

			if (name == "show_triangulation") {
				showMap[ShowTriangulation] = showValueInt;
			} else if (name == "show_room_triangulation") {
				showMap[ShowRoomTriangulation] = showValueInt;
			} else if (name == "show_waypoints") {
				showMap[ShowWaypoints] = showValueInt;
			} else if (name == "show_path") {
				showMap[ShowPath] = showValueInt;
			} else if (name == "show_neighbours") {
				showMap[ShowNeighbours] = showValueInt;
			}
		} else if (reader->isEndElement() && reader->name().toString() == "drawing") {
			reader->readNextStartElement();
			bool result = room->loadProject(reader);

			if (result) {
				for (std::map<int, bool>::const_iterator i = showMap.begin(); i != showMap.end(); i++) {
					show_[i->first] = i->second;
				}
			}

			return result;
		}

		reader->readNext();

		if (!reader->isEndElement()) {
			return false;
		}

		reader->readNextStartElement();
	}

	return false;
}

bool Drawing::DrawingImpl::saveProject(QXmlStreamWriter *writer) const
{
	writer->writeStartElement("", "drawing");

	writer->writeTextElement("", "image", QString::fromStdString(room->image().filename()));

	writer->writeEmptyElement("", "show_triangulation");
	writer->writeAttribute("show", QString::number(show_[ShowTriangulation]));

	writer->writeEmptyElement("", "show_room_triangulation");
	writer->writeAttribute("show", QString::number(show_[ShowRoomTriangulation]));

	writer->writeEmptyElement("", "show_waypoints");
	writer->writeAttribute("show", QString::number(show_[ShowWaypoints]));

	writer->writeEmptyElement("", "show_path");
	writer->writeAttribute("show", QString::number(show_[ShowPath]));

	writer->writeEmptyElement("", "show_neighbours");
	writer->writeAttribute("show", QString::number(show_[ShowNeighbours]));

	writer->writeEndElement();

	room->saveProject(writer);

	return !writer->hasError();
}

Drawing::Drawing()
	: p(new DrawingImpl)
{
}

Drawing::~Drawing()
{
	delete p;
}

void Drawing::fromImage(const char *name)
{
	p->fromImage(name);
}

void Drawing::setNodes(int amount)
{
	p->setNodes(amount);
}

void Drawing::setWaypointModification(WaypointModification modification)
{
	p->setWaypointModification(modification);
}

void Drawing::setOption(Option option, bool enabled)
{
	p->setOption(option, enabled);
}

void Drawing::mouseClick(int x, int y, MouseButton button)
{
	p->mouseClick(x, y, button);
}

void Drawing::initialize()
{
	p->initialize();
}

void Drawing::paint()
{
	p->paint();
}

void Drawing::resize(int width, int height)
{
	p->resize(width, height);
}

bool Drawing::loadProject(QXmlStreamReader *reader)
{
	return p->loadProject(reader);
}

bool Drawing::saveProject(QXmlStreamWriter *writer) const
{
	return p->saveProject(writer);
}
