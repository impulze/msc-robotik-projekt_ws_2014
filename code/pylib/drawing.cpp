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
#include <cstdlib>
#include <ctime>
#include <limits>
#include <set>
#include <stdexcept>
#include <vector>

#if MEASURE
#include <sys/time.h>
#endif

#define ROBOT_DIAMETER 5

// private namespace
namespace
{

void throwErrorFromGLError();
long randomAtMost(long max);

void throwErrorFromGLError()
{
	GLenum glError = glGetError();

	if (glError != GL_NO_ERROR) {
		const char *string = reinterpret_cast<const char *>(gluErrorString(glError));
		throw std::runtime_error(string);
	}
}

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

class Drawing::DrawingImpl
{
public:
	DrawingImpl();
	~DrawingImpl();

	void fromImage(const char *name);

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
	bool getCoordFromMouseClick(int x, int y, Coord2D &coord);

	Drawing::WaypointModification waypointModification;
	GLint viewport[4];
	GLuint circleVBO;
	Room *room;
	Texture *texture;
	bool showTriangulation;
	bool showWaypoints;
	bool showPath;
	std::vector<Triangle> triangulation;
	std::vector<Coord2D> path;
	Coord2D neighbourToShow;
	// mapping of a coord and intersection value
	std::map<Coord2D, bool> neighbourToShowNeighbours;
};

Drawing::DrawingImpl::DrawingImpl()
	: waypointModification(Drawing::WaypointNoMod),
	  room(0),
	  texture(0),
	  showTriangulation(false),
	  showWaypoints(false),
	  showPath(false)
{
	std::srand(std::time(0));
}

Drawing::DrawingImpl::~DrawingImpl()
{
	freeTexture();
	glDeleteBuffers(1, &circleVBO);
}

void Drawing::DrawingImpl::fromImage(const char *name)
{
	freeTexture();

	room = new Room(name, ROBOT_DIAMETER);

	if (room->image().width() > static_cast<unsigned int>(std::numeric_limits<int>::max()) ||
	    room->image().height() > static_cast<unsigned int>(std::numeric_limits<int>::min())) {
		delete room;
		throw std::runtime_error("OpenGL cannot draw this texture.");
	}

	try {
		texture = new Texture(room->image());
	} catch (...) {
		delete texture;
		throw;
	}
}

void Drawing::DrawingImpl::setNodes(int amount)
{
	room->clearWaypoints();

	for (int i = 0; i < amount; i++) {
		int randX = randomAtMost(texture->width() - 1);
		int randY = randomAtMost(texture->height() - 1);

		if (!room->insertWaypoint(Coord2D(randX, randY))) {
			i--;
			continue;
		}
	}

	triangulation = room->triangulate();
	path = room->generatePath();
}

void Drawing::DrawingImpl::setWaypointModification(Drawing::WaypointModification modification)
{
	waypointModification = modification;
}

void Drawing::DrawingImpl::setOption(Drawing::Option option, bool enabled)
{
	switch (option) {
		case Drawing::ShowTriangulation:
			showTriangulation = enabled;
			break;

		case Drawing::ShowWaypoints:
			showWaypoints = enabled;
			break;

		case Drawing::ShowPath:
			showPath = enabled;
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

		if (changed) {
			triangulation = room->triangulate();
			path = room->generatePath();
		}
	} else if (button == Drawing::RightMouseButton) {
		Coord2D coord;

		if (!getCoordFromMouseClick(x, y, coord)) {
			return;
		}

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
						printf("intersects %d\n", neighbourToShowNeighbours[*sit]);
					}
				} else {
					neighbourToShowNeighbours.clear();
				}

				for (std::set<Coord2D>::const_iterator cit = it->second.begin(); cit != it->second.end(); cit++) {
					Coord2D thatCoord(cit->x, cit->y);

					double xDistance = static_cast<double>(thatCoord.x) - thisCoord.x;
					double yDistance = static_cast<double>(thatCoord.y) - thisCoord.y;
					double distance = std::sqrt(xDistance * xDistance + yDistance * yDistance);

					printf("distances: %f, %f, %f\n", xDistance, yDistance, distance);
					printf("neighbour distances to (%d/%d): %g\n", cit->x, cit->y, distance);
				}
			}
		}
	}
}

void Drawing::DrawingImpl::initialize()
{
	fromImage("/home/impulze/Diagramm2.png");
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

	while (true) {
		int randX = randomAtMost(texture->width() - 1);
		int randY = randomAtMost(texture->height() - 1);

		if (room->setStartpoint(Coord2D(randX, randY))) {
			break;
		}
	}

	while (true) {
		int randX = randomAtMost(texture->width() - 1);
		int randY = randomAtMost(texture->height() - 1);

		if (room->setEndpoint(Coord2D(randX, randY))) {
			break;
		}
	}

	room->insertWaypoint(Coord2D(352, 277));
	room->insertWaypoint(Coord2D(401, 271));
	room->setStartpoint(Coord2D(410, 255));
	//room->setStartpoint(Coord2D(557, 317));
	room->setEndpoint(Coord2D(330, 287));

	triangulation = room->triangulate();
	path = room->generatePath();
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

	// Waypoints
	glBindBuffer(GL_ARRAY_BUFFER, circleVBO);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_DOUBLE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (showTriangulation) {
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

	if (showWaypoints) {
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
			unsigned char c1, c2, c3;

			c1 = rand() % 128;
			c2 = rand() % 128;
			c3 = rand() % 128;

			glColor3b(c1, c2, c3);
		} else {
			glColor3b(127, 0 ,0);
			glLineWidth(2.0f);
		}

		glBegin(GL_LINES);
		glVertex2f(neighbourToShow.x, texture->height() - 1 - neighbourToShow.y);
		glVertex2f(it->first.x, texture->height() - 1 - it->first.y);
		glEnd();
	}

	if (showPath && path.size() > 0) {
		glTranslatef(0.0f, 0.0f, 0.2f);
		// dark-yellow
		//glColor3f(0.7f, 0.7f, 0.0f);
		glColor3f(0.2f, 0.2f, 0.2f);
		glLineWidth(2.0f);
		glBegin(GL_LINE_STRIP);
		for (std::vector<Coord2D>::size_type i = 0; i < path.size() - 1; i++) {
			Coord2D c1;
			Coord2D c2;
			Coord2D c3;
			Coord2D c4;
			Coord2DTemplate<float> result;

			for (float t = 0.0f; t < 1.0f; t += 0.02f) {
				if (i == 0) {
					c1 = path[i];
					c2 = path[i + 1];
					c3 = path[i + 2];
					result = catmullRomFirst(t, c1, c2, c3);
				} else if (i == path.size() - 2) {
					c1 = path[i - 1];
					c2 = path[i];
					c3 = path[i + 1];
					result = catmullRomLast(t, c1, c2, c3);
				} else {
					c1 = path[i - 1];
					c2 = path[i];
					c3 = path[i + 1];
					c4 = path[i + 2];
					result = catmullRom(t, c1, c2, c3, c4);
				}

				glVertex2f(result.x, texture->height() - 1 - result.y);
			}
		}
		glEnd();
	}

	glTranslatef(0.0f, 0.0f, 0.2f);
	glColor3f(1.0f, 0.0f, 0.0f);
	drawPoint(room->getEndpoint().x, room->getEndpoint().y);

	glTranslatef(0.0f, 0.0f, 0.2f);
	glColor3f(0.0f, 1.0f, 0.0f);
	drawPoint(room->getStartpoint().x, room->getStartpoint().y);

	glDisableClientState(GL_VERTEX_ARRAY);

#if MEASURE
	struct timeval sec; gettimeofday(&sec, NULL);
	struct timeval res; timersub(&sec, &fir, &res);
	printf("rendering: %lu sec %lu usec\n", res.tv_sec, res.tv_usec);
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
		bool result = room->removeWaypoint(coord);

		assert(result);

		return result;
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

Drawing::Drawing()
	: p(new DrawingImpl)
{
}

void Drawing::fromImage(const char *name)
{
	p->fromImage(name);
}

void Drawing::toImage(const char *name)
{
	static_cast<void>(name);

	return;
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
