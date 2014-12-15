#define GL_GLEXT_PROTOTYPES
#include "drawing.h"
#include "room.h"
#include "texture.h"

#include <GL/glu.h>
#include <IL/ilu.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

#if MEASURE
#include <sys/time.h>
#endif

#define ROBOT_DIAMETER 5

#include "algo.h"

namespace
{

	void throw_error_from_gl_error()
	{
		GLenum glError = glGetError();

		if (glError != GL_NO_ERROR) {
			const char *string = reinterpret_cast<const char *>(gluErrorString(glError));
			throw std::runtime_error(string);
		}
	}

	long random_at_most(long max)
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
}

Drawing::Drawing()
	: waypointModification_(WaypointNoMod),
	  room_(0),
	  texture_(0)
{
	std::srand(std::time(0));
}

Drawing::~Drawing()
{
	freeTexture();
	glDeleteBuffers(1, &circleVBO_);
}

void Drawing::fromImage(const char *name)
{
	freeTexture();

	room_ = new Room(name, ROBOT_DIAMETER);

	if (room_->image().width() > static_cast<unsigned int>(std::numeric_limits<int>::max()) ||
	    room_->image().height() > static_cast<unsigned int>(std::numeric_limits<int>::min())) {
		delete room_;
		throw std::runtime_error("OpenGL cannot draw this texture.");
	}

	try {
		texture_ = new Texture(room_->image());
	} catch (...) {
		delete texture_;
		throw;
	}
}

void Drawing::setNodes(int amount)
{
	room_->triangulate(ROBOT_DIAMETER);

	for (int i = 0; i < amount; i++) {
		int randX = random_at_most(texture_->width() - 1);
		int randY = random_at_most(texture_->height() - 1);

		if (!room_->insertWaypoint(Coord2D(randX, randY))) {
			i--;
			continue;
		}
	}
}

void Drawing::setWaypointModification(WaypointModification modification)
{
	waypointModification_ = modification;
}

void Drawing::mouseClick(int x, int y)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, texture_->width(), 0, texture_->height(), -1.0f, 4.0f);
	throw_error_from_gl_error();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Texture
	glTranslatef(0.0f, 0.0f, -3.5f);

	GLdouble modelview[16], projection[16];
	glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	glGetDoublev(GL_PROJECTION_MATRIX, projection);

	GLfloat z;
	glReadPixels(x, viewport_[3] - y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);

	GLdouble posX, posY, posZ;
	gluUnProject(x, y, z, modelview, projection, viewport_, &posX, &posY, &posZ);
	throw_error_from_gl_error();

	x = posX;
	y = posY;

	switch (waypointModification_) {
		case WaypointAdd:
			room_->insertWaypoint(Coord2D(x, y));
			break;

		case WaypointDelete:
			delNode(x, y);
			break;

		case WaypointStart:
			room_->setStartpoint(Coord2D(x, y));
			break;

		case WaypointEnd:
			room_->setEndpoint(Coord2D(x, y));
			break;

		default:
			break;
	}
}

GLuint triangleVBO;

void Drawing::initialize()
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

	glGenBuffersARB(1, &circleVBO_);
	throw_error_from_gl_error();

	try {
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, circleVBO_);
		throw_error_from_gl_error();
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof vertices, vertices, GL_STATIC_DRAW);
		throw_error_from_gl_error();
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		throw_error_from_gl_error();
	} catch (...) {
		glDeleteBuffers(1, &circleVBO_);
	}

	{
		GLdouble tVertices[6];
		tVertices[0] = 0.0; tVertices[1] = 0.0;
		tVertices[2] = 1.0; tVertices[2] = 0.0;
		tVertices[3] = 0.5; tVertices[4] = 0.5;

		glGenBuffersARB(1, &triangleVBO);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, triangleVBO);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, sizeof tVertices, tVertices, GL_STATIC_DRAW);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
	}

	while (true) {
		int randX = random_at_most(texture_->width() - 1);
		int randY = random_at_most(texture_->height() - 1);

		if (room_->setStartpoint(Coord2D(randX, randY))) {
			break;
		}
	}

	while (true) {
		int randX = random_at_most(texture_->width() - 1);
		int randY = random_at_most(texture_->height() - 1);

		if (room_->setEndpoint(Coord2D(randX, randY))) {
			break;
		}
	}
}

void Drawing::paint()
{
	if (texture_ == 0) {
		return;
	}

#if MEASURE
	struct timeval fir; gettimeofday(&fir, NULL);
#endif

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// default projection: glOrtho(l=-1,r=1,b=-1,t=1,n=1,f=-1)
	glOrtho(0, texture_->width(), 0, texture_->height(), -1.0f, 4.0f);
	throw_error_from_gl_error();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Texture
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -3.5f);

	// OpenGL origin = bottom-left
	// DevIL origin = top-left
	// => swap y coordinates in glTexCoord2f

	glEnable(GL_TEXTURE_2D);
	texture_->bind();
	glBegin(GL_POLYGON);
	glTexCoord2i(0, 1);
	glVertex2i(0, 0);
	glTexCoord2i(0, 0);
	glVertex2i(0, texture_->height());
	glTexCoord2i(1, 0);
	glVertex2i(texture_->width(), texture_->height());
	glTexCoord2i(1, 1);
	glVertex2i(texture_->width(), 0);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	glPopMatrix();

	// Waypoints
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -3.0f);

	glBindBuffer(GL_ARRAY_BUFFER, circleVBO_);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_DOUBLE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glColor3f(1.0f, 1.0f, 0.0f);

	std::set<Coord2D> const &waypoints = room_->getWaypoints();

	for (std::set<Coord2D>::const_iterator it = waypoints.begin(); it != waypoints.end(); it++) {
		drawPoint(it->x, it->y);
	}

	std::vector<Polygon2D> const &triangulatedPolygons = room_->triangulatedPolygons();

	glColor3f(0.5f, 0.8f, 1.0f);

	glPushMatrix();
	glLineWidth(2.0f);
	//glEnable(GL_POINT_SMOOTH);
	//glPointSize(3.0f);
	for (std::vector<Polygon2D>::const_iterator it = triangulatedPolygons.begin();
	     it != triangulatedPolygons.end();
	     it++) {
		glBegin(GL_LINE_LOOP);

		for (std::size_t i = 0; i < it->size(); i++) {
			unsigned int x = (*it)[i].x;
			unsigned int y = texture_->height() - 1 - (*it)[i].y;
			glVertex2i(x, y);
		}

		glEnd();
	}
	glPopMatrix();

	glTranslatef(0.0f, 0.0f, 0.5f);
	glColor3f(1.0f, 0.0f, 0.0f);
	drawPoint(room_->getEndpoint().x, room_->getEndpoint().y);

	glTranslatef(0.0f, 0.0f, 0.5f);
	glColor3f(0.0f, 1.0f, 0.0f);
	drawPoint(room_->getStartpoint().x, room_->getStartpoint().y);

	glDisableClientState(GL_VERTEX_ARRAY);

	glPopMatrix();

#if MEASURE
	struct timeval sec; gettimeofday(&sec, NULL);
	struct timeval res; timersub(&sec, &fir, &res);
	printf("rendering: %lu sec %lu usec\n", res.tv_sec, res.tv_usec);
#endif
}

void Drawing::resize(int width, int height)
{
	viewport_[0] = 0;
	viewport_[1] = 0;
	viewport_[2] = width;
	viewport_[3] = height;
}

void Drawing::freeTexture()
{
	delete texture_;
	delete room_;

	room_ = 0;
	texture_ = 0;
}

bool Drawing::delNode(int x, int y)
{
	// the user clicked near or directly into the waypoint
	int left = std::max(0, x - ROBOT_DIAMETER / 2);
	int right = std::min(static_cast<int>(texture_->width()) - 1, x + ROBOT_DIAMETER / 2);
	int bottom = std::max(0, y - ROBOT_DIAMETER / 2);
	int top = std::min(static_cast<int>(texture_->height()) - 1, y + ROBOT_DIAMETER / 2);
	bool deleted = false;

	for (int i = bottom; i <= top; i++) {
		for (int j = left; j <= right; j++) {
			Coord2D coord(j, i);

			if (room_->removeWaypoint(coord)) {
				deleted = true;
				break;
			}
		}

		if (deleted) {
			break;
		}
	}

	if (!deleted) {
		std::printf("Unable to remove waypoint (%d/%d), not present yet.\n", x, y);
	}

	return deleted;
}

void Drawing::drawPoint(int x, int y)
{
#if COLLISION_DETECTION
		int left = std::max(0, x - ROBOT_DIAMETER / 2);
		int right = std::min(static_cast<int>(texture_->width()) - 1, x + ROBOT_DIAMETER / 2);
		int bottom = std::max(0, y - ROBOT_DIAMETER / 2);
		int top = std::min(static_cast<int>(texture_->height()) - 1, y + ROBOT_DIAMETER / 2);

		glRecti(left, bottom, right, top);
#else
		int offsetX = x;
		int offsetY = texture_->height() - 1 - y;
		glTranslatef(offsetX, offsetY, 0.0f);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 301);
		glTranslatef(-offsetX, -offsetY, 0.0f);
#endif
}

void Drawing::toImage(const char *name)
{
	return;
}
