#ifndef ROB_COORD_H_INCLUDED
#define ROB_COORD_H_INCLUDED

struct Coord2D
{
	Coord2D();
	Coord2D(unsigned int x, unsigned int y);

	bool operator<(Coord2D const &other) const;
	bool operator==(Coord2D const &other) const;

	unsigned int x;
	unsigned int y;
};

#endif // ROB_COORD_H_INCLUDED
