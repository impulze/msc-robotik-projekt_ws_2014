#ifndef DRAWING_H_INCLUDED
#define DRAWING_H_INCLUDED

#include <string>

class Drawing
{
public:
	Drawing();
	~Drawing();

	void fromImage(const char *name);
	void toImage(const char *name);

	void setNodes(int amount);
	void setOrigin(int x, int y);
};

#endif // DRAWING_H_INCLUDED
