#ifndef ROB_DRAWING_H_INCLUDED
#define ROB_DRAWING_H_INCLUDED

#include <cstddef>

class QTextEdit;
class QXmlStreamReader;
class QXmlStreamWriter;
class Room;
class Texture;

class Drawing
{
public:
	Drawing(QTextEdit *statusText, QTextEdit *helpText);
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
		ShowRoomTriangulation,
		ShowWaypoints,
		ShowPath,
		ShowNeighbours
	};

	void fromImage(const char *name);

	void setNodes(int amount);
	void setWaypointModification(WaypointModification modification);
	void setOption(Option option, bool enabled);
	void mouseClick(int x, int y);

	std::size_t countWaypoints() const;

	void initialize();
	void paint();
	void resize(int width, int height);

	bool loadRoom(const char *name);
	bool loadProject(QXmlStreamReader *reader);
	bool saveProject(QXmlStreamWriter *writer) const;

private:
	class DrawingImpl;
	DrawingImpl *p;
};

#endif // ROB_DRAWING_H_INCLUDED
