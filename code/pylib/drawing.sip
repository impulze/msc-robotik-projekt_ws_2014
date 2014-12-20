%DefaultSupertype sip.simplewrapper

class Drawing
{
%TypeHeaderCode
#include <drawing.h>
%End

public:
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

	enum MouseButton
	{
		LeftMouseButton,
		RightMouseButton
	};

	Drawing();
	~Drawing();

	void fromImage(const char *name);
	void toImage(const char *name);

	void setNodes(int amount);
	void setWaypointModification(WaypointModification modification);
	void setOption(Option option, bool enabled);
	void mouseClick(int x, int y, MouseButton button);
};
