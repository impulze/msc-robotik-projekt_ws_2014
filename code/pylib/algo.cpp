#include "algo.h"

Coord2D catmullRomImpl(double matrix[4][4], float t, Coord2D const &c1, Coord2D const &c2, Coord2D const &c3, Coord2D const &c4)
{
	double first = (1 * matrix[0][0] + t * matrix[1][0] + t * t * matrix[2][0] + t * t * t * matrix[3][0]) / 2;
	double second = (1 * matrix[0][1] + t * matrix[1][1] + t * t * matrix[2][1] + t * t * t * matrix[3][1]) / 2;
	double third = (1 * matrix[0][2] + t * matrix[1][2] + t * t * matrix[2][2] + t * t * t * matrix[3][2]) / 2;
	double fourth = (1 * matrix[0][3] + t * matrix[1][3] + t * t * matrix[2][3] + t * t * t * matrix[3][3]) / 2;

	Coord2D result;

	result.x = c1.x * first + c2.x * second + c3.x * third + c4.x * fourth;
	result.y = c1.y * first + c2.y * second + c3.y * third + c4.y * fourth;

	return result;
}

Coord2D catmullRomFirst(float t, Coord2D const &c1, Coord2D const &c2, Coord2D const &c3)
{
	double matrix[4][4] = {
		{ 0, 2, 0, 0 },
		{ 2, 0, 0, 0 },
		{ -4, -5, 6, -1 },
		{ 2, 3, -4, 1 }
	};

	Coord2D orientation(1, 1);

	return catmullRomImpl(matrix, t, orientation, c1, c2, c3);
}

Coord2D catmullRom(float t, Coord2D const &c1, Coord2D const &c2, Coord2D const &c3, Coord2D const &c4)
{
	double a = 1;

	double matrix[4][4] = {
		{ 0, 2, 0, 0 },
		{ -a, 0, a, 0 },
		{ 2 * a, -6 + a, 6 - 2 * a, -a },
		{ -a, 4 - a, -4 + a, a }
	};

	return catmullRomImpl(matrix, t, c1, c2, c3, c4);
}

Coord2D catmullRomLast(float t, Coord2D const &c1, Coord2D const &c2, Coord2D const &c3)
{
	double matrix[4][4] = {
		{ 0, 2, 0, 0 },
		{ -1, 0, 1, 0 },
		{ 2, -6, 4, -2 },
		{ -1, 4, -3, 2 }
	};

	Coord2D orientation(1, 1);

	return catmullRomImpl(matrix, t, c1, c2, c3, orientation);
}
