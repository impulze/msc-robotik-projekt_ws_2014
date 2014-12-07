%DefaultSupertype sip.simplewrapper

class Drawing
{
%TypeHeaderCode
#include <drawing.h>
%End

public:
	Drawing();
	~Drawing();

	void fromImage(const char *name);
	void toImage(const char *name);

	void setNodes(int amount);
	void setOrigin(int x, int y);
};
