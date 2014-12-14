#include "roomimage.h"

#include <cassert>
#include <map>
#include <set>

#include <stdio.h>

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

	std::vector<Coord2D> createNeighbours(Coord2D const &coord, unsigned int width, unsigned int height)
	{
		std::vector<Coord2D> neighbours;
		bool addNorth = coord.y > 0;
		bool addEast = coord.x < width - 1;
		bool addSouth = coord.y < height - 1;
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

	std::set<Coord2D> checkNeighbourCollision(Coord2D const &coord, unsigned int width, unsigned int height, unsigned char distance)
	{
		// check a quad of (distance * 2 + 1)^2
		std::set<Coord2D> checks;

		for (unsigned char i = 0; i < distance * 2 + 1; i++) {
			for (unsigned char j = 0; j < distance * 2 + 1; j++) {
				// check if resulting pixel leaves left or top boundary
				if (coord.y >= i && coord.x >= j && coord.y) {
					// check if resulting pixel leaves right or bottom boundary
					if (coord.y + i < height - 1 && coord.x + j < width - 1) {
						checks.insert(coord);
					}
				}
			}
		}

		return checks;
	}
}

RoomImage::RoomImage(std::string const &filename)
	: Image(filename)
{
}

std::vector<Polygon2D> RoomImage::triangulate(unsigned char distance) const
{
	std::vector<Polygon2D> innerPolygons;

	enum CoordType {
		OUTSIDE,
		WALL_OR_OBJECT_OUTLINE,
		INSIDE,
		COLLISION
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

	// mark points as collision which can't be passed by the moving object
	CoordTypesMap copyCoordTypes = coordTypes;

	for (CoordTypesMap::const_iterator it = copyCoordTypes.begin(); it != copyCoordTypes.end(); it++) {
		if (it->second != INSIDE) {
			coordTypes[it->first] = it->second;
			continue;
		}

		Coord2D coord(it->first.x, it->first.y);
		std::set<Coord2D> checks = checkNeighbourCollision(coord, width(), height(), distance);

		for (std::set<Coord2D>::const_iterator cit = checks.begin(); cit != checks.end(); cit++) {
			unsigned char const *bytes = data().data() + (cit->y * width() + cit->x) * stride;

			if (isBlack(bytes)) {
				coordTypes[coord] = COLLISION;
			}
		}
	}

	int count = 0;

	for (CoordTypesMap::const_iterator it = coordTypes.begin(); it != coordTypes.end(); it++) {
		if (it->second == COLLISION) {
			count++;
		}
	}

	printf("%d collisions\n", count);
	
	std::set<Coord2D> insideCoords;

	// first find all coordinates inside the room
	for (CoordTypesMap::const_iterator it = coordTypes.begin(); it != coordTypes.end(); it++) {
		if (it->second != INSIDE) {
			continue;
		}

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

	enum DirectionType {
		WEST,
		SOUTH,
		EAST,
		NORTH
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
			innerPolygons.push_back(currentPolygon);
			currentPolygon = Polygon2D();

			if (insideCoords.empty()) {
				break;
			}

			coord = *insideCoords.begin();
		} else {
			coord = *newCoord;
		}
	}

	return innerPolygons;
}
