#include <drawing.h>

#include <GL/glu.h>
#include <IL/ilu.h>

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <vector>

#include <unistd.h>

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

	void drawCircle(int x, int y)
	{
		glBegin(GL_TRIANGLE_FAN);
		glVertex2d(1.0 * x, 1.0 * y);

		for (int i = 0; i < 100; i++) {
			double angle = 2.0 * M_PI * (1.0 * i) / 100;
			double drawX = 1.0 * x + (ROBOT_DIAMETER / 2.0) * std::cos(angle);
			double drawY = 1.0 * y + (ROBOT_DIAMETER / 2.0) * std::sin(angle);
			glVertex3d(drawX, drawY, 0.0);
		}

		glEnd();
	}

	long random_at_most(long max)
	{
		unsigned long num_bins = static_cast<unsigned long>(max) + 1;
		unsigned long num_rand = static_cast<unsigned long>(RAND_MAX) + 1;
		unsigned long bin_size = num_rand / num_bins;
		unsigned long defect = num_rand % num_bins;

		int x;

		while (true) {
			//x = random();
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
{
	ilInit();
	throw_error_from_il_error();

	std::srand(std::time(0));
}

Drawing::~Drawing()
{
	freeTexture();
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
		throw_error_from_gl_error();
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

		ILubyte *byte = imageData_ + (randY * textureWidth_ + randX) * 3;

		if (isBlack(byte)) {
			i--;
			continue;
		}

		if (!checkSurrounding(randX, randY)) {
			i--;
			continue;
		}

		Coord coord;
		coord.x = randX;
		coord.y = textureHeight_ - 1 - randY;

		std::set<Coord>::iterator found = outsideNodes_.find(coord);

		if (found != outsideNodes_.end()) {
			i--;
			continue;
		}

		waypointNodes_.insert(coord);
	}
}

void Drawing::setOrigin(int x, int y)
{
	(void)x;
	(void)y;
}

void Drawing::initialize()
{
	fromImage("/home/impulze/Documents/example_room2.png");
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_DEPTH_TEST);
}

void Drawing::paint()
{
	if (glIsTexture(texture_) != GL_TRUE) {
		return;
	}

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
	//glColor3i(1, 0, 0);
	glBegin(GL_QUADS);
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

	// Walls
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -3.0f);

	glColor3f(0.0f, 0.0f, 0.0f);
	glBegin(GL_POINTS);

	for (std::set<Coord>::const_iterator it = collideNodes_.begin(); it != collideNodes_.end(); it++) {
		glVertex3i(it->x, it->y, 0.0f);
	}

	glEnd();

	glPopMatrix();

	// Waypoints
	glPushMatrix();
	glTranslatef(0.0f, 0.0f, -3.0f);

	glColor3f(1.0f, 0.0f, 0.0f);

	for (std::set<Coord>::const_iterator it = waypointNodes_.begin(); it != waypointNodes_.end(); it++) {
		drawCircle(it->x, it->y);
	}

	glPopMatrix();
}

void Drawing::resize(int width, int height)
{
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
	int bottom = std::max(0, y - ROBOT_DIAMETER /2);
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

bool Drawing::Coord::operator<(Coord const &other) const
{
	if (x != other.x) {
		return x < other.x;
	}

	return y < other.y;
}
