#ifndef DRAWING_H_INCLUDED
#define DRAWING_H_INCLUDED

#include "coord.h"

#include <GL/gl.h>
#include <IL/il.h>

#include <map>
#include <set>
#include <vector>

class Room;
class Texture;

class Drawing
{
public:
	Drawing();
	~Drawing();

	enum WaypointModification
	{
		WaypointAdd,
		WaypointDelete,
		WaypointStart,
		WaypointEnd,
		WaypointNoMod
	};

	void fromImage(const char *name);
	void toImage(const char *name);

	void setNodes(int amount);
	void setWaypointModification(WaypointModification modification);
	void mouseClick(int x, int y);

	void initialize();
	void paint();
	void resize(int width, int height);

private:
	void freeTexture();
	bool checkSurrounding(int x, int y);
	bool checkNode(int x, int y);
	bool addNode(int x, int y);
	bool delNode(int x, int y);
	void drawPoint(int x, int y);

	std::set<Coord2D> collideNodes_;
	std::set<Coord2D> outsideNodes_;
	std::set<Coord2D> waypointNodes_;
	std::set<Coord2D> edges_;
	std::map<Coord2D *, std::vector<Coord2D  *> > vertices_;
	Coord2D startNode_;
	Coord2D endNode_;
	WaypointModification waypointModification_;
	GLint viewport_[4];
	GLuint circleVBO_;
	Room *room_;
	Texture *texture_;
};

#endif // DRAWING_H_INCLUDED
