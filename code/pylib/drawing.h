#ifndef DRAWING_H_INCLUDED
#define DRAWING_H_INCLUDED

class Room;
class Texture;

class Drawing
{
public:
	Drawing();

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
	class DrawingImpl;
	DrawingImpl *p;
};

#endif // DRAWING_H_INCLUDED
