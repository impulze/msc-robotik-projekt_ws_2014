#include "roomimage.h"

#include <cassert>
#include <map>
#include <set>

namespace
{
	bool isBlack(unsigned char const *bytes)
	{
		return bytes[0] == 0 && bytes[1] == 0 && bytes[2] == 0;
	}

	bool isWhite(unsigned char const *bytes)
	{
		return bytes[0] == 255 && bytes[1] == 255 && bytes[2] == 255;
	}

	std::vector<Coord2D> createNeighbours(Coord2D const &coord, unsigned int maxWidth, unsigned int maxHeight)
	{
		std::vector<Coord2D> neighbours;
		bool addNorth = coord.y > 0;
		bool addEast = coord.x < maxWidth - 1;
		bool addSouth = coord.y < maxHeight - 1;
		bool addWest = coord.x > 0;

		if (addNorth) {
			Coord2D north(coord.x, coord.y - 1);
			neighbours.push_back(north);

			if (addEast) {
				Coord2D northEast(coord.x + 1, coord.y - 1);
				neighbours.push_back(northEast);
			}

			if (addWest) {
				Coord2D northWest(coord.x - 1, coord.y - 1);
				neighbours.push_back(northWest);
			}
		}

		if (addSouth) {
			Coord2D south(coord.x, coord.y + 1);
			neighbours.push_back(south);

			if (addEast) {
				Coord2D southEast(coord.x + 1, coord.y + 1);
				neighbours.push_back(southEast);
			}

			if (addWest) {
				Coord2D southWest(coord.x - 1, coord.y + 1);
				neighbours.push_back(southWest);
			}
		}

		if (addEast) {
			Coord2D east(coord.x + 1, coord.y);
			neighbours.push_back(east);
		}

		if (addWest) {
			Coord2D west(coord.x - 1, coord.y);
			neighbours.push_back(west);
		}

		return neighbours;
	}
}

RoomImage::RoomImage(std::string const &filename)
	: Image(filename)
{
	enum CoordType {
		OUTSIDE,
		WALL_OR_OBJECT_OUTLINE,
		INSIDE
	};

	typedef std::map<Coord2D, int> CoordTypesMap;

	CoordTypesMap coordTypes;
	unsigned char stride = type() == IMAGE_TYPE_RGB ? 3 : 4;

	// stamp all coordinates with the appropriate type
	for (unsigned int y = 0; y < height(); y++) {
		for (unsigned int x = 0; x < width(); x++) {
			Coord2D coord(x, y);
			unsigned char const *bytes = data().data() + (y * width() + x) * stride;

			if (isWhite(bytes)) {
				coordTypes[coord] = OUTSIDE;
			} else if (isBlack(bytes)) {
				coordTypes[coord] = WALL_OR_OBJECT_OUTLINE;
			} else {
				coordTypes[coord] = INSIDE;
			}
		}
	}

	std::set<Coord2D> insideCoords;

	// first find all coordinates inside the room
	for (CoordTypesMap::const_iterator it = coordTypes.begin(); it != coordTypes.end(); it++) {
		if (it->second == INSIDE) {
			std::vector<Coord2D> neighbours = createNeighbours(it->first, width(), height());

			bool neighboursInside = true;

			// expand neighbours
			for (std::vector<Coord2D>::const_iterator nit = neighbours.begin(); nit != neighbours.end(); nit++) {
				// find the type of the neighbour coordinate
				CoordTypesMap::const_iterator foundCoordType = coordTypes.find(*nit);

				assert(foundCoordType != coordTypes.end());

				if (foundCoordType->second != INSIDE) {
					neighboursInside = false;
					break;
				}
			}

			if (!neighboursInside) {
				// not all neighbours are inside, this one is an edge vertex
				insideCoords.insert(it->first);
			}
		}
	}

	enum DirectionType {
		WEST,
		SOUTH,
		EAST,
		NORTH,
	};

	Polygon2D currentPolygon;
	int currentDirection = WEST;
	Coord2D coord = *insideCoords.begin();
	currentPolygon.push_back(coord);

	while (!insideCoords.empty()) {
		bool hasWestNeighbours = coord.x > 0;
		bool hasSouthNeighbours = coord.y < height() - 1;
		bool hasEastNeighbours = coord.x < width() - 1;
		bool hasNorthNeighbours = coord.y > 0;

		std::set<Coord2D>::const_iterator insideNeighbours[8];

		for (int i = 0; i < 8; i++) {
			insideNeighbours[0] == insideCoords.end();
		}

		if (hasWestNeighbours) {
			insideNeighbours[0] = insideCoords.find(Coord2D(coord.x - 1, coord.y));
		}

		if (hasSouthNeighbours) {
			insideNeighbours[1] = insideCoords.find(Coord2D(coord.x, coord.y + 1));
		}

		if (hasEastNeighbours) {
			insideNeighbours[2] = insideCoords.find(Coord2D(coord.x + 1, coord.y));
		}


		if (hasNorthNeighbours) {
			insideNeighbours[3] = insideCoords.find(Coord2D(coord.x, coord.y - 1));
		}

		std::set<Coord2D>::const_iterator newCoord = insideCoords.end();
		bool switchedDirection = false;

		// now do the work
		switch (currentDirection) {
			case WEST: {
				if (insideNeighbours[WEST] != insideCoords.end()) {
					newCoord = insideNeighbours[WEST];
					break;
				}

				int possibleDirections[2] = { NORTH, SOUTH };

				for (int i = 0; i < 2; i++) {
					int direction = possibleDirections[i];

					if (insideNeighbours[direction] != insideCoords.end()) {
						newCoord = insideNeighbours[direction];
						switchedDirection = true;
						currentDirection = direction;
						break;
					}
				}

				break;
			}

			case SOUTH: {
				if (insideNeighbours[SOUTH] != insideCoords.end()) {
					newCoord = insideNeighbours[SOUTH];
					break;
				}

				int possibleDirections[2] = { WEST, EAST };

				for (int i = 0; i < 2; i++) {
					int direction = possibleDirections[i];

					if (insideNeighbours[direction] != insideCoords.end()) {
						newCoord = insideNeighbours[direction];
						switchedDirection = true;
						currentDirection = direction;
						break;
					}
				}

				break;
			}

			case EAST: {
				if (insideNeighbours[EAST] != insideCoords.end()) {
					newCoord = insideNeighbours[EAST];
					break;
				}

				int possibleDirections[2] = { SOUTH, NORTH };

				for (int i = 0; i < 2; i++) {
					int direction = possibleDirections[i];

					if (insideNeighbours[direction] != insideCoords.end()) {
						newCoord = insideNeighbours[direction];
						switchedDirection = true;
						currentDirection = direction;
						break;
					}
				}

				break;
			}

			case NORTH: {
				if (insideNeighbours[NORTH] != insideCoords.end()) {
					newCoord = insideNeighbours[NORTH];
					break;
				}

				int possibleDirections[2] = { WEST, EAST };

				for (int i = 0; i < 2; i++) {
					int direction = possibleDirections[i];

					if (insideNeighbours[direction] != insideCoords.end()) {
						newCoord = insideNeighbours[direction];
						switchedDirection = true;
						currentDirection = direction;
						break;
					}
				}

				break;
			}

		}

		insideCoords.erase(coord);

		if (switchedDirection) {
			currentPolygon.push_back(coord);
		}

		if (newCoord == insideCoords.end()) {
			innerPolygons_.push_back(currentPolygon);
			currentPolygon = Polygon2D();

			if (insideCoords.empty()) {
				break;
			}

			coord = *insideCoords.begin();
		} else {
			coord = *newCoord;
		}
	}
}

std::vector<Polygon2D> const &RoomImage::innerPolygons() const
{
	return innerPolygons_;
}
