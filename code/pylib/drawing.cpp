#define GL_GLEXT_PROTOTYPES
#include "drawing.h"
#include "roomimage.h"
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

	bool isBlack(ILubyte const *byte)
	{
		ILubyte red = byte[0];
		ILubyte green = byte[1];
		ILubyte blue = byte[2];

		return red == 0 && green == 0 && blue == 0;
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
	  image_(0),
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

	image_ = new RoomImage(name);

	if (image_->width() > static_cast<unsigned int>(std::numeric_limits<int>::max()) ||
	    image_->height() > static_cast<unsigned int>(std::numeric_limits<int>::min())) {
		delete image_;
		throw std::runtime_error("OpenGL cannot draw this texture.");
	}

	try {
		texture_ = new Texture(*image_);
	} catch (...) {
		delete texture_;
		throw;
	}

	collideNodes_.clear();
	outsideNodes_.clear();
}

void Drawing::setNodes(int amount)
{
	waypointNodes_.clear();
	image_->recreateConvexCCWRoomPolygons();

	for (int i = 0; i < amount; i++) {
		int randX = random_at_most(texture_->width() - 1);
		int randY = random_at_most(texture_->height() - 1);

		bool added = addNode(randX, randY);

		if (!added) {
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
			addNode(x, y);
			break;

		case WaypointDelete:
			delNode(x, y);
			break;

		case WaypointStart:
			if (checkNode(x, y)) {
				printf("Setting startpoint (%d/%d).\n", x, y);
				startNode_.x = x;
				startNode_.y = texture_->height() - 1 - y;
			}

			break;

		case WaypointEnd:
			if (checkNode(x, y)) {
				printf("Setting endpoint (%d/%d).\n", x, y);
				endNode_.x = x;
				endNode_.y = texture_->height() - 1 - y;
			}

			break;

		default:
			break;
	}
}

GLuint triangleVBO;

void Drawing::initialize()
{
	fromImage("/home/impulze/Documents/example_room3.png");
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

		if (checkNode(randX, randY)) {
			printf("Setting startpoint (%d/%d).\n", randX, randY);
			startNode_.x = randX;
			startNode_.y = texture_->height() - 1 - randY;
			break;
		}
	}

	while (true) {
		int randX = random_at_most(texture_->width() - 1);
		int randY = random_at_most(texture_->height() - 1);

		if (checkNode(randX, randY)) {
			printf("Setting endpoint (%d/%d).\n", randX, randY);
			endNode_.x = randX;
			endNode_.y = texture_->height() - 1 - randY;
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

#if COLLISION_CHECK
	// Walls
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -3.0f);

	glColor3f(0.0f, 1.0f, 0.0f);
	glBegin(GL_POINTS);

	for (std::set<Coord2D>::const_iterator it = collideNodes_.begin(); it != collideNodes_.end(); it++) {
		glVertex3i(it->x, it->y, 0.0f);
	}

	glEnd();

	glPopMatrix();
#endif

	// Waypoints
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -3.0f);

	glBindBuffer(GL_ARRAY_BUFFER, circleVBO_);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_DOUBLE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glColor3f(1.0f, 1.0f, 0.0f);

	for (std::set<Coord2D>::const_iterator it = waypointNodes_.begin(); it != waypointNodes_.end(); it++) {
		drawPoint(it->x, it->y);
	}


	RoomImage::ConvexCCWRoomPolygons const &convexCCWRoomPolygons = image_->convexCCWRoomPolygons();

	glColor3f(0.5f, 0.8f, 1.0f);

	glPushMatrix();
	glLineWidth(2.0f);
	//glEnable(GL_POINT_SMOOTH);
	//glPointSize(3.0f);
	for (RoomImage::ConvexCCWRoomPolygons::const_iterator it = convexCCWRoomPolygons.begin();
	     it != convexCCWRoomPolygons.end();
	     it++) {
		glBegin(GL_LINE_LOOP);

		for (std::size_t i = 0; i < it->size(); i++) {
			unsigned int x = (*it)[i].x;
			unsigned int y = texture_->height() - 1 - (*it)[i].y;
			//printf("drawing at: %d/%d\n", x, y);
			glVertex2i(x, y);
		}

		glEnd();
	}
	glPopMatrix();

	glTranslatef(0.0f, 0.0f, 0.5f);
	glColor3f(1.0f, 0.0f, 0.0f);
	drawPoint(endNode_.x, endNode_.y);

	glTranslatef(0.0f, 0.0f, 0.5f);
	glColor3f(0.0f, 1.0f, 0.0f);
	drawPoint(startNode_.x, startNode_.y);

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
	delete image_;

	image_ = 0;
	texture_ = 0;
}

bool Drawing::checkSurrounding(int x, int y)
{
	unsigned char const *data = image_->data().data();
	int left = std::max(0, x - ROBOT_DIAMETER / 2);
	int right = std::min(static_cast<int>(texture_->width()) - 1, x + ROBOT_DIAMETER / 2);
	int bottom = std::max(0, y - ROBOT_DIAMETER / 2);
	int top = std::min(static_cast<int>(texture_->height()) - 1, y + ROBOT_DIAMETER / 2);

	for (int i = bottom; i <= top; i++) {
		for (int j = left; j <= right; j++) {
			ILubyte const *byte = data + (i * texture_->width() + j) * 3;

			if (isBlack(byte)) {
				return false;
			}
		}
	}

	return true;
}

bool Drawing::checkNode(int x, int y)
{
	unsigned char const *data = image_->data().data();
	ILubyte const *byte = data + (y * texture_->width() + x) * 3;

	if (isBlack(byte)) {
		std::fprintf(stderr, "Point (%d/%d) collides with an object or wall.\n", x, y);
		return false;
	}

	if (!checkSurrounding(x, y)) {
		std::fprintf(stderr, "Due to roboter diameter the point (%d/%d) would collide with an object or wall.\n", x, y);
		return false;
	}

	Coord2D coord;
	coord.x = x;
	coord.y = texture_->height() - 1 - y;

	std::set<Coord2D>::iterator found = outsideNodes_.find(coord);

	if (found != outsideNodes_.end()) {
		std::fprintf(stderr, "Point (%d/%d) is outside of the room.\n", x, y);
		return false;
	}

	found = waypointNodes_.find(coord);

	if (found != waypointNodes_.end()) {
		std::fprintf(stderr, "Point (%d/%d) is a waypoint.\n", x, y);
		return false;
	}

	if (startNode_ == coord) {
		std::fprintf(stderr, "Point (%d/%d) is startpoint.\n", x, y);
		return false;
	}

	if (endNode_ == coord) {
		std::fprintf(stderr, "Point (%d/%d) is endpoint.\n", x, y);
		return false;
	}

	if (!image_->checkWaypoint(Coord2D(x, y))) {
		std::fprintf(stderr, "Point (%d/%d) not in boundary.\n", x, y);
		return false;
	}

	return true;
}

bool Drawing::addNode(int x, int y)
{
	if (checkNode(x, y)) {
		std::cout << "adding node: " << x << '/' << y << '\n';
		Coord2D coord;
		coord.x = x;
		coord.y = texture_->height() - 1 - y;
		waypointNodes_.insert(coord);
		image_->insertWaypoint(Coord2D(x, y));

		return true;
	}

	return false;
}

bool Drawing::delNode(int x, int y)
{
	int left = std::max(0, x - ROBOT_DIAMETER / 2);
	int right = std::min(static_cast<int>(texture_->width()) - 1, x + ROBOT_DIAMETER / 2);
	int bottom = std::max(0, y - ROBOT_DIAMETER / 2);
	int top = std::min(static_cast<int>(texture_->height()) - 1, y + ROBOT_DIAMETER / 2);
	bool deleted = false;

	for (int i = bottom; i <= top; i++) {
		for (int j = left; j <= right; j++) {
			Coord2D coord;

			coord.x = j;
			coord.y = texture_->height() - 1 - i;

			std::set<Coord2D>::iterator found = waypointNodes_.find(coord);

			if (found != waypointNodes_.end()) {
				std::cout << "deleting node: " << j << '/' << i << '\n';
				waypointNodes_.erase(found);
				image_->removeWaypoint(Coord2D(j, i));
				deleted = true;
			}
		}
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
		glTranslatef(x, y, 0.0f);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 301);
		glTranslatef(-x, -y, 0.0f);
#endif
}

void Drawing::toImage(const char *name)
{
	return;
}
