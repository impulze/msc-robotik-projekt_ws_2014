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

	enum Option
	{
		ShowTriangulation,
		ShowWaypoints,
		ShowPath
	};

	void fromImage(const char *name);
	void toImage(const char *name);

	void setNodes(int amount);
	void setWaypointModification(WaypointModification modification);
	void setOption(Option option, bool enabled);
	void mouseClick(int x, int y);

	void initialize();
	void paint();
	void resize(int width, int height);

private:
	void freeTexture();
	bool checkNode(int x, int y);
	bool delNode(int x, int y);
	void drawPoint(int x, int y);

	WaypointModification waypointModification_;
	GLint viewport_[4];
	GLuint circleVBO_;
	Room *room_;
	Texture *texture_;
	bool showTriangulation_;
	bool showWaypoints_;
	bool showPath_;
};

#endif // DRAWING_H_INCLUDED
