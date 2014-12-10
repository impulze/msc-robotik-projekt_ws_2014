#ifndef DRAWING_H_INCLUDED
#define DRAWING_H_INCLUDED

#include <GL/gl.h>
#include <IL/il.h>

#include <set>

class Drawing
{
public:
	Drawing();
	~Drawing();

	void fromImage(const char *name);
	void toImage(const char *name);

	void setNodes(int amount);
	void setOrigin(int x, int y);

	void initialize();
	void paint();
	void resize(int width, int height);

private:
	void freeTexture();
	bool checkSurrounding(int x, int y);

	struct Coord
	{
		int x;
		int y;

		bool operator<(Coord const &other) const;
	};

	GLuint texture_;
	GLuint textureWidth_;
	GLuint textureHeight_;
	ILuint image_;
	ILubyte *imageData_;
	std::set<Coord> collideNodes_;
	std::set<Coord> outsideNodes_;
	std::set<Coord> waypointNodes_;
};

#endif // DRAWING_H_INCLUDED
