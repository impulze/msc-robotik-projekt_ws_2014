#define GL_GLEXT_PROTOTYPES

#include "algo.h"
#include "coord.h"
#include "drawing.h"
#include "room.h"
#include "roomimage.h"
#include "stats.h"
#include "texture.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>

#ifdef WIN32
#include <GL/glew.h>
#endif
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
#include <QtWidgets/QTextEdit>

#if MEASURE
#include <sys/time.h>
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
	DrawingImpl(Drawing *parent, Stats *stats, QTextEdit *statusText, QTextEdit *helpText);
	~DrawingImpl();

	void fromImage(const char *name);

	void updateRoom();

	void setNodes(int amount);
	void setWaypointModification(Drawing::WaypointModification modification);
	void setOption(Drawing::Option option, bool enabled);
	bool getOption(Drawing::Option option) const;
	void setAlgorithm(Room::Algorithm algorithm);
	Room::Algorithm getAlgorithm() const;
	void mouseClick(int x, int y);

	std::size_t countWaypoints() const;

	void initialize();
	void paint();
	void resize(int width, int height);
	void animate();

	void freeTexture();
	bool checkNode(int x, int y);
	bool delNode(int x, int y);
	void drawPoint(int x, int y);
	void drawCross(int x, int y);
	bool getCoordFromMouseClick(int x, int y, Coord2D &coord);
	void showNeighbours(Coord2D const &coord);

	bool loadRoom(const char *name);
	bool loadProject(QXmlStreamReader *reader);
	bool saveProject(QXmlStreamWriter *writer) const;

	void animationForward();

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
	QTextEdit *statusText_;
	QTextEdit *helpText_;
	QTimer *animationTimer;
	bool animated;
	std::vector< Coord2DTemplate<float> > animationPoints;
	std::vector< Coord2DTemplate<float> >::const_iterator animationPosition;
	Stats *stats;
};

Drawing::DrawingImpl::DrawingImpl(Drawing *parent, Stats *stats, QTextEdit *statusText, QTextEdit *helpText)
	: waypointModification(Drawing::WaypointNoMod),
	  room(0),
	  texture(0),
	  statusText_(statusText),
	  helpText_(helpText),
	  animationTimer(new QTimer(parent)),
	  animated(false),
	  stats(stats)
{
	std::srand(std::time(0));

	for (size_t i = 0; i < sizeof show_ / sizeof *show_; i++) {
		show_[i] = false;
	}

	connect(animationTimer, SIGNAL(timeout()), parent, SLOT(animationForward()));
}

Drawing::DrawingImpl::~DrawingImpl()
{
	freeTexture();
	glDeleteBuffers(1, &circleVBO);
}

void Drawing::DrawingImpl::fromImage(const char *name)
{
	room = new Room(name, ROBOT_DIAMETER, stats, statusText_, helpText_);

	if (room->image().width() > static_cast<unsigned int>(std::numeric_limits<int>::max()) ||
	    room->image().height() > static_cast<unsigned int>(std::numeric_limits<int>::min())) {
		delete room;
		statusText_->setText(statusText_->tr("OpenGL cannot draw this texture."));
		throw std::runtime_error("OpenGL cannot draw this texture.");
	}

	roomTriangulation = room->getRoomTriangulation();
}

void Drawing::DrawingImpl::updateRoom()
{
	QElapsedTimer timer;

	triangulation = room->getTriangulation();

	path = room->generatePath();

	if (path.begin()->x > path.rbegin()->x) {
		std::reverse(path.begin(), path.end());
	}

	timer.start();

	pathPoints = catmullRom(path, 150);

	stats->lastCatmullRomCalculation = timer.elapsed();

	pathCollisions.clear();

	timer.start();

	for (std::vector< Coord2DTemplate<float> >::const_iterator it = pathPoints.begin(); it != pathPoints.end(); ++it) {
		if (!room->pointInside(it->x, it->y)) {
			pathCollisions.insert(*it);
		}
	}

	stats->lastPathCollisionCalculation = timer.elapsed();
}

void Drawing::DrawingImpl::setNodes(int amount)
{
	room->clearWaypoints();

	QElapsedTimer timer;
	timer.start();

	room->setNodes(amount);

	stats->lastSetNodes = timer.elapsed();

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

bool Drawing::DrawingImpl::getOption(Drawing::Option option) const
{
	return show_[option];
}

void Drawing::DrawingImpl::setAlgorithm(Room::Algorithm algorithm)
{
	room->setAlgorithm(algorithm);

	updateRoom();
}

Room::Algorithm Drawing::DrawingImpl::getAlgorithm() const
{
	return room->getAlgorithm();
}

void Drawing::DrawingImpl::mouseClick(int x, int y)
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
			if (show_[ShowNeighbours]) {
				Coord2D coord;

				if (!getCoordFromMouseClick(x, y, coord)) {
					return;
				}

				showNeighbours(coord);
			}
			break;
	}

	if (changed) {
		updateRoom();
	}
}

std::size_t Drawing::DrawingImpl::countWaypoints() const
{
	return room->getWaypoints().size();
}

void Drawing::DrawingImpl::initialize()
{
#ifdef WIN32
    GLenum result = glewInit();

    std::string glewVersionString;
    std::string vertexBufferString;

    if (result != GLEW_OK) {
        glewVersionString = "GLEW not correctly supported.\n" + std::string(reinterpret_cast<const char *>(glewGetErrorString(result)));
    } else {
        glewVersionString = "GLEW supported.\n" + std::string(reinterpret_cast<const char *>(glewGetString(GLEW_VERSION)));
    }

    if (!GLEW_ARB_vertex_buffer_object) {
        vertexBufferString = "OpenGL VBO not supported.";
    } else {
        vertexBufferString = "OpenGL VBO are supported.";
    }

    statusText_->setText(QString::fromStdString(glewVersionString + "\n" + vertexBufferString));

    if (result != GLEW_OK) {
        return;
    }
#endif

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
		throw;
	}

	{
		vertices[0] = -3; vertices[1] = -3;
		vertices[2] = 3; vertices[3] = 3;
		vertices[4] = -3; vertices[5] = 3;
		vertices[6] = 3; vertices[7] = -3;
	}

	glGenBuffersARB(1, &crossVBO);
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
		throw;
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

	glColor3f(0.0f, 1.0f, 0.0f);
	drawPoint(room->getStartpoint().x, room->getStartpoint().y);

	// Collisions
	if (show_[ShowPath] && !pathCollisions.empty()) {
		glBindBuffer(GL_ARRAY_BUFFER, crossVBO);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(2, GL_DOUBLE, 0, 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glColor3f(1.0f, 0.0f, 0.0f);

		for (std::set< Coord2DTemplate<float> >::const_iterator it = pathCollisions.begin(); it != pathCollisions.end(); ++it) {
			drawCross(it->x, it->y);
		}
	}

	if (animated) {
		glColor3f(0.63f, 0.13f, 0.94f);
		drawPoint(animationPosition->x, animationPosition->y);
	}

	glDisableClientState(GL_VERTEX_ARRAY);

#if MEASURE
	struct timeval sec; gettimeofday(&sec, NULL);
	struct timeval res; timersub(&sec, &fir, &res);
	statusText_->setText(statusText_->tr("rendering: %1 sec %2 usec\n").arg(res.tv_sec).arg(res.tv_usec));
#endif
}

void Drawing::DrawingImpl::resize(int width, int height)
{
	viewport[0] = 0;
	viewport[1] = 0;
	viewport[2] = width;
	viewport[3] = height;
}

void Drawing::DrawingImpl::animate()
{
	if (pathPoints.empty()) {
		statusText_->setText(statusText_->tr("There's no path yet, that can be animated."));
		return;
	}

	if (!pathCollisions.empty()) {
		statusText_->setText(statusText_->tr("The path collides, no animation possible."));
		return;
	}

	animated = true;
	animationTimer->start(16);
	animationPoints = pathPoints;

	if (!animationPoints.empty()) {
		Coord2DTemplate<float> firstPosition = *animationPoints.begin();

		if (firstPosition.x == room->getEndpoint().x && firstPosition.y == room->getEndpoint().y) {
			std::reverse(animationPoints.begin(), animationPoints.end());
		}
	}

	animationPosition = animationPoints.begin();
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
		}
	}
}

bool Drawing::DrawingImpl::loadRoom(const char *name)
{
	fromImage(name);
	return true;
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

			if (name == "algorithm") {
				reader->readNext();

				if (!reader->isCharacters()) {
					return false;
				}

				if (reader->text().toString() == "Dijkstra") {
					room->setAlgorithm(Room::Dijkstra);
				} else if (reader->text().toString() == "A*") {
					room->setAlgorithm(Room::AStar);
				}

				reader->readNext();

				if (!reader->isEndElement() || reader->name().toString() != "algorithm") {
					return false;
				}

				reader->readNextStartElement();

				continue;
			}

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

void Drawing::DrawingImpl::animationForward()
{
	if (animationPosition == animationPoints.end()) {
		animationPoints.clear();
		animated = false;
		animationTimer->stop();
	}

	++animationPosition;
}

Drawing::Drawing(Stats *stats, QTextEdit *statusText, QTextEdit *helpText)
	: p(new DrawingImpl(this, stats, statusText, helpText))
{
}

Drawing::~Drawing()
{
	delete p;
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

bool Drawing::getOption(Option option) const
{
	return p->getOption(option);
}

void Drawing::setAlgorithm(Room::Algorithm algorithm)
{
	p->setAlgorithm(algorithm);
}

Room::Algorithm Drawing::getAlgorithm() const
{
	return p->getAlgorithm();
}

void Drawing::mouseClick(int x, int y)
{
	p->mouseClick(x, y);
}

std::size_t Drawing::countWaypoints() const
{
	return p->countWaypoints();
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

void Drawing::animate()
{
	p->animate();
}

bool Drawing::loadRoom(char const *name)
{
	return p->loadRoom(name);
}

bool Drawing::loadProject(QXmlStreamReader *reader)
{
	return p->loadProject(reader);
}

bool Drawing::saveProject(QXmlStreamWriter *writer) const
{
	return p->saveProject(writer);
}

void Drawing::animationForward()
{
	p->animationForward();
}
