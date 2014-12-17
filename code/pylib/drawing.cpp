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
	void mouseClick(int x, int y);

	void initialize();
	void paint();
	void resize(int width, int height);

	void freeTexture();
	bool checkNode(int x, int y);
	bool delNode(int x, int y);
	void drawPoint(int x, int y);

	Drawing::WaypointModification waypointModification;
	GLint viewport[4];
	GLuint circleVBO;
	Room *room;
	Texture *texture;
	bool showTriangulation;
	bool showWaypoints;
	bool showPath;
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
	Coord2D startpoint = room->getStartpoint();
	Coord2D endpoint = room->getEndpoint();

	room->triangulate(ROBOT_DIAMETER);

	bool result = room->setStartpoint(startpoint);
	assert(result);

	result = room->setEndpoint(endpoint);
	assert(result);

	for (int i = 0; i < amount; i++) {
		int randX = randomAtMost(texture->width() - 1);
		int randY = randomAtMost(texture->height() - 1);

		if (!room->insertWaypoint(Coord2D(randX, randY))) {
			i--;
			continue;
		}
	}

	room->calculatePath();
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
			break;
	}

	if (changed) {
		room->calculatePath();
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

	room->calculatePath();
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
		std::vector<Polygon2D> const &triangulatedPolygons = room->getTriangulatedPolygons();
		for (std::vector<Polygon2D>::const_iterator it = triangulatedPolygons.begin();
		     it != triangulatedPolygons.end();
		     it++) {
			glBegin(GL_LINE_LOOP);

			for (std::size_t i = 0; i < it->size(); i++) {
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

	if (showPath) {
		glTranslatef(0.0f, 0.0f, 0.2f);
		// dark-yellow
		glColor3f(0.7f, 0.7f, 0.0f);
		glBegin(GL_LINE_STRIP);
		std::vector<Coord2D> const &calculatedPath = room->getCalculatedPath();
		for (std::vector<Coord2D>::size_type i = 0; i < calculatedPath.size() - 1; i++) {
			Coord2D c1;
			Coord2D c2;
			Coord2D c3;
			Coord2D c4;
			Coord2D result;

			if (i == 0) {
				c1 = calculatedPath[i];
				c2 = calculatedPath[i + 1];
				c3 = calculatedPath[i + 2];
				for (float t = 0.0f; t < 1.0f; t += 0.02f) {
					result = catmullRomFirst(t, c1, c2, c3);
					glVertex2f(result.x, texture->height() - 1 - result.y);
				}
			} else if (i == calculatedPath.size() - 2) {
				c1 = calculatedPath[i - 1];
				c2 = calculatedPath[i];
				c3 = calculatedPath[i + 1];
				for (float t = 0.0f; t < 1.0f; t += 0.02f) {
					result = catmullRomLast(t, c1, c2, c3);
					glVertex2f(result.x, texture->height() - 1 - result.y);
				}
			} else {
				c1 = calculatedPath[i - 1];
				c2 = calculatedPath[i];
				c3 = calculatedPath[i + 1];
				c4 = calculatedPath[i + 2];
				for (float t = 0.0f; t < 1.0f; t += 0.02f) {
					result = catmullRom(t, c1, c2, c3, c4);
					glVertex2f(result.x, texture->height() - 1 - result.y);
				}
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
	// the user clicked near or directly into the waypoint
	int left = std::max(0, x - ROBOT_DIAMETER / 2);
	int right = std::min(static_cast<int>(texture->width()) - 1, x + ROBOT_DIAMETER / 2);
	int bottom = std::max(0, y - ROBOT_DIAMETER / 2);
	int top = std::min(static_cast<int>(texture->height()) - 1, y + ROBOT_DIAMETER / 2);

	for (int i = bottom; i <= top; i++) {
		for (int j = left; j <= right; j++) {
			Coord2D coord(j, i);

			if (room->removeWaypoint(coord)) {
				return true;
			}
		}
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

void Drawing::mouseClick(int x, int y)
{
	p->mouseClick(x, y);
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
