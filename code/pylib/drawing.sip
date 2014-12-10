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

	Drawing();
	~Drawing();

	void fromImage(const char *name);
	void toImage(const char *name);

	void setNodes(int amount);
	void setWaypointModification(WaypointModification modification);
	void mouseClick(int x, int y);
};
