#define GL_GLEXT_PROTOTYPES
#include <drawing.h>

#include <GL/glu.h>
#include <IL/ilu.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <vector>

#if MEASURE
#include <sys/time.h>
#endif

#define ROBOT_DIAMETER 5

namespace
{

	void throw_error_from_il_error()
	{
		ILenum ilError = ilGetError();

		if (ilError != IL_NO_ERROR) {
			throw std::runtime_error(iluErrorString(ilError));
		}
	}

	void throw_error_from_gl_error()
	{
		GLenum glError = glGetError();

		if (glError != GL_NO_ERROR) {
			const char *string = reinterpret_cast<const char *>(gluErrorString(glError));
			throw std::runtime_error(string);
		}
	}

	bool isBlack(ILubyte *byte)
	{
		ILubyte red = byte[0];
		ILubyte green = byte[1];
		ILubyte blue = byte[2];

		return red == 0 && green == 0 && blue == 0;
	}

	bool isWhite(ILubyte *byte)
	{
		ILubyte red = byte[0];
		ILubyte green = byte[1];
		ILubyte blue = byte[2];

		return red == 255 && green == 255 && blue == 255;
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

struct Coord
{
	int x;
	int y;
};

Drawing::Drawing()
	: waypointModification_(WaypointNoMod)
{
	ilInit();
	throw_error_from_il_error();

	std::srand(std::time(0));
}

Drawing::~Drawing()
{
	freeTexture();
	glDeleteBuffers(1, &circleVBO_);
}

void Drawing::fromImage(const char *name)
{
	ILint width;
	ILint height;

	freeTexture();
	ilDeleteImages(1, &image_);

	ilGenImages(1, &image_);

	try {
		ilBindImage(image_);
		ilLoad(IL_PNG, name);
		throw_error_from_il_error();
		ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE);
		throw_error_from_il_error();

		imageData_ = ilGetData();
		width = ilGetInteger(IL_IMAGE_WIDTH);
		height = ilGetInteger(IL_IMAGE_HEIGHT);
	} catch (...) {
		ilDeleteImages(1, &image_);
		throw;
	}

	try {
		glGenTextures(1, &texture_);
		textureWidth_ = static_cast<GLuint>(width);
		textureHeight_ = static_cast<GLuint>(height);
		glBindTexture(GL_TEXTURE_2D, texture_);
		throw_error_from_gl_error();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureWidth_, textureHeight_, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData_);
		throw_error_from_gl_error();
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	} catch (...) {
		glDeleteTextures(1, &texture_);
		ilDeleteImages(1, &image_);
		throw;
	}

	collideNodes_.clear();
	outsideNodes_.clear();

	for (int i = 0; i < static_cast<int>(textureHeight_); i++) {
		for (int j = 0; j < static_cast<int>(textureWidth_); j++) {
			ILubyte *b = imageData_ + (i * textureWidth_ + j) * 3;

			if (isWhite(b)) {
				Coord coord;
				coord.x = j;
				coord.y = textureHeight_ - 1 - i;
				outsideNodes_.insert(coord);
			}

			if (isBlack(b)) {
				Coord coord;
				coord.x = j;
				coord.y = textureHeight_ - 1 - i;
				collideNodes_.insert(coord);
			}
		}
	}
}

void Drawing::toImage(const char *name)
{
	std::vector<unsigned char> rawdata;
	GLint x;
	GLint y;

	glBindTexture(GL_TEXTURE_2D, texture_);
	throw_error_from_gl_error();
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &x);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &y);
	rawdata.resize(x * y * 3);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, &rawdata[0]);
	throw_error_from_gl_error();

	ILuint image;

	try {
		ilGenImages(1, &image);
		ilBindImage(image);
		ilTexImage(x, y, 1, 3, IL_RGB, IL_UNSIGNED_BYTE, &rawdata[0]);
		throw_error_from_il_error();
		ilEnable(IL_FILE_OVERWRITE);
		ilSaveImage(name);
		throw_error_from_il_error();
		ilDeleteImages(1, &image);
	} catch (...) {
		ilDeleteImages(1, &image);
		throw;
	}
}

void Drawing::setNodes(int amount)
{
	waypointNodes_.clear();

	for (int i = 0; i < amount; i++) {
		int randX = random_at_most(textureWidth_ - 1);
		int randY = random_at_most(textureHeight_ - 1);

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
	glOrtho(0, textureWidth_, 0, textureHeight_, -1.0f, 4.0f);
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
				startNode_.y = textureHeight_ - 1 - y;
			}

			break;

		case WaypointEnd:
			if (checkNode(x, y)) {
				printf("Setting endpoint (%d/%d).\n", x, y);
				endNode_.x = x;
				endNode_.y = textureHeight_ - 1 - y;
			}

			break;

		default:
			break;
	}
}

void Drawing::initialize()
{
	fromImage("/home/impulze/Documents/example_room2.png");
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

	while (true) {
		int randX = random_at_most(textureWidth_ - 1);
		int randY = random_at_most(textureHeight_ - 1);

		if (checkNode(randX, randY)) {
			printf("Setting startpoint (%d/%d).\n", randX, randY);
			startNode_.x = randX;
			startNode_.y = textureHeight_ - 1 - randY;
			break;
		}
	}

	while (true) {
		int randX = random_at_most(textureWidth_ - 1);
		int randY = random_at_most(textureHeight_ - 1);

		if (checkNode(randX, randY)) {
			printf("Setting endpoint (%d/%d).\n", randX, randY);
			endNode_.x = randX;
			endNode_.y = textureHeight_ - 1 - randY;
			break;
		}
	}
}

void Drawing::paint()
{
	if (glIsTexture(texture_) != GL_TRUE) {
		return;
	}

#if MEASURE
	struct timeval fir; gettimeofday(&fir, NULL);
#endif

	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	// default projection: glOrtho(l=-1,r=1,b=-1,t=1,n=1,f=-1)
	glOrtho(0, textureWidth_, 0, textureHeight_, -1.0f, 4.0f);
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
	glBindTexture(GL_TEXTURE_2D, texture_);
	glBegin(GL_POLYGON);
	glTexCoord2i(0, 1);
	glVertex2i(0, 0);
	glTexCoord2i(0, 0);
	glVertex2i(0, textureHeight_);
	glTexCoord2i(1, 0);
	glVertex2i(textureWidth_, textureHeight_);
	glTexCoord2i(1, 1);
	glVertex2i(textureWidth_, 0);
	glEnd();
	glDisable(GL_TEXTURE_2D);

	glPopMatrix();

#if COLLISION_CHECK
	// Walls
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -3.0f);

	glColor3f(0.0f, 1.0f, 0.0f);
	glBegin(GL_POINTS);

	for (std::set<Coord>::const_iterator it = collideNodes_.begin(); it != collideNodes_.end(); it++) {
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

	for (std::set<Coord>::const_iterator it = waypointNodes_.begin(); it != waypointNodes_.end(); it++) {
		drawPoint(it->x, it->y);
	}

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
	if (glIsTexture(texture_) == GL_TRUE) {
		glDeleteTextures(1, &texture_);
	}
}

bool Drawing::checkSurrounding(int x, int y)
{
	int left = std::max(0, x - ROBOT_DIAMETER / 2);
	int right = std::min(static_cast<int>(textureWidth_) - 1, x + ROBOT_DIAMETER / 2);
	int bottom = std::max(0, y - ROBOT_DIAMETER / 2);
	int top = std::min(static_cast<int>(textureHeight_) - 1, y + ROBOT_DIAMETER / 2);

	for (int i = bottom; i <= top; i++) {
		for (int j = left; j <= right; j++) {
			ILubyte *byte = imageData_ + (i * textureWidth_ + j) * 3;

			if (isBlack(byte)) {
				return false;
			}
		}
	}

	return true;
}

bool Drawing::checkNode(int x, int y)
{
	ILubyte *byte = imageData_ + (y * textureWidth_ + x) * 3;

	if (isBlack(byte)) {
		std::fprintf(stderr, "Point (%d/%d) collides with an object or wall.\n", x, y);
		return false;
	}

	if (!checkSurrounding(x, y)) {
		std::fprintf(stderr, "Due to roboter diameter the point (%d/%d) would collide with an object or wall.\n", x, y);
		return false;
	}

	Coord coord;
	coord.x = x;
	coord.y = textureHeight_ - 1 - y;

	std::set<Coord>::iterator found = outsideNodes_.find(coord);

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

	return true;
}

bool Drawing::addNode(int x, int y)
{
	if (checkNode(x, y)) {
		std::cout << "adding node: " << x << '/' << y << '\n';
		Coord coord;
		coord.x = x;
		coord.y = textureHeight_ - 1 - y;
		waypointNodes_.insert(coord);
		return true;
	}

	return false;
}

bool Drawing::delNode(int x, int y)
{
	int left = std::max(0, x - ROBOT_DIAMETER / 2);
	int right = std::min(static_cast<int>(textureWidth_) - 1, x + ROBOT_DIAMETER / 2);
	int bottom = std::max(0, y - ROBOT_DIAMETER / 2);
	int top = std::min(static_cast<int>(textureHeight_) - 1, y + ROBOT_DIAMETER / 2);
	bool deleted = false;

	for (int i = bottom; i <= top; i++) {
		for (int j = left; j <= right; j++) {
			Coord coord;

			coord.x = j;
			coord.y = textureHeight_ - 1 - i;

			std::set<Coord>::iterator found = waypointNodes_.find(coord);

			if (found != waypointNodes_.end()) {
				std::cout << "deleting node: " << x << '/' << y << '\n';
				waypointNodes_.erase(found);
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
		int right = std::min(static_cast<int>(textureWidth_) - 1, x + ROBOT_DIAMETER / 2);
		int bottom = std::max(0, y - ROBOT_DIAMETER / 2);
		int top = std::min(static_cast<int>(textureHeight_) - 1, y + ROBOT_DIAMETER / 2);

		glRecti(left, bottom, right, top);
#else
		glTranslatef(x, y, 0.0f);
		glDrawArrays(GL_TRIANGLE_FAN, 0, 301);
		glTranslatef(-x, -y, 0.0f);
#endif
}

bool Drawing::Coord::operator<(Coord const &other) const
{
	if (x != other.x) {
		return x < other.x;
	}

	return y < other.y;
}

bool Drawing::Coord::operator==(Coord const &other) const
{
	return x == other.x && y == other.y;
}
