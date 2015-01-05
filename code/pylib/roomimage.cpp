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

	bool isGray(unsigned char const *bytes)
	{
		return bytes[0] == 200 && bytes[1] == 200 && bytes[2] == 200;
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

		long checkX = coord.x;
		long checkY = coord.y;

		for (unsigned char i = 0; i < distance; i++) {
			for (unsigned char j = 0; j < distance; j++) {
				checkX = coord.x;
				checkY = coord.y;

				checkX -= (distance / 2) - j;
				checkY -= (distance / 2) - i;

				if (checkX < width && checkY < height) {
					if (checkX >= 0 && checkY >= 0) {
						checks.insert(Coord2D(checkX, checkY));
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

std::vector<Polygon2D> RoomImage::expandPolygon(std::set<Coord2D> &coords) const
{
	std::vector<Polygon2D> borderPolygons;

	enum DirectionType {
		WEST,
		SOUTH,
		EAST,
		NORTH
	};

	Polygon2D currentPolygon;
	// walk east, sets store the coords low -> high
	int currentDirection = EAST;
	Coord2D coord = *coords.begin();
	currentPolygon.push_back(coord);

	while (!coords.empty()) {
		bool hasWestNeighbours = coord.x > 0;
		bool hasSouthNeighbours = coord.y < height() - 1;
		bool hasEastNeighbours = coord.x < width() - 1;
		bool hasNorthNeighbours = coord.y > 0;

		std::set<Coord2D>::const_iterator insideNeighbours[4];

		if (hasWestNeighbours) {
			insideNeighbours[0] = coords.find(Coord2D(coord.x - 1, coord.y));
		}

		if (hasSouthNeighbours) {
			insideNeighbours[1] = coords.find(Coord2D(coord.x, coord.y + 1));
		}

		if (hasEastNeighbours) {
			insideNeighbours[2] = coords.find(Coord2D(coord.x + 1, coord.y));
		}

		if (hasNorthNeighbours) {
			insideNeighbours[3] = coords.find(Coord2D(coord.x, coord.y - 1));
		}

		std::set<Coord2D>::const_iterator newCoord = coords.end();
		bool switchedDirection = false;

		// now do the work
		switch (currentDirection) {
			case WEST: {
				if (insideNeighbours[WEST] != coords.end()) {
					newCoord = insideNeighbours[WEST];
					break;
				}

				int possibleDirections[2] = { NORTH, SOUTH };

				for (int i = 0; i < 2; i++) {
					int direction = possibleDirections[i];

					if (insideNeighbours[direction] != coords.end()) {
						newCoord = insideNeighbours[direction];
						switchedDirection = true;
						currentDirection = direction;
						break;
					}
				}

				break;
			}

			case SOUTH: {
				if (insideNeighbours[SOUTH] != coords.end()) {
					newCoord = insideNeighbours[SOUTH];
					break;
				}

				int possibleDirections[2] = { WEST, EAST };

				for (int i = 0; i < 2; i++) {
					int direction = possibleDirections[i];

					if (insideNeighbours[direction] != coords.end()) {
						newCoord = insideNeighbours[direction];
						switchedDirection = true;
						currentDirection = direction;
						break;
					}
				}

				break;
			}

			case EAST: {
				if (insideNeighbours[EAST] != coords.end()) {
					newCoord = insideNeighbours[EAST];
					break;
				}

				int possibleDirections[2] = { SOUTH, NORTH };

				for (int i = 0; i < 2; i++) {
					int direction = possibleDirections[i];

					if (insideNeighbours[direction] != coords.end()) {
						newCoord = insideNeighbours[direction];
						switchedDirection = true;
						currentDirection = direction;
						break;
					}
				}

				break;
			}

			case NORTH: {
				if (insideNeighbours[NORTH] != coords.end()) {
					newCoord = insideNeighbours[NORTH];
					break;
				}

				int possibleDirections[2] = { WEST, EAST };

				for (int i = 0; i < 2; i++) {
					int direction = possibleDirections[i];

					if (insideNeighbours[direction] != coords.end()) {
						newCoord = insideNeighbours[direction];
						switchedDirection = true;
						currentDirection = direction;
						break;
					}
				}

				break;
			}

		}

		coords.erase(coord);

		if (switchedDirection) {
			currentPolygon.push_back(coord);
		}

		if (newCoord == coords.end()) {
			// support lines
			if (currentPolygon.size() == 1) {
				currentPolygon.push_back(coord);
			}

			borderPolygons.push_back(currentPolygon);
			currentPolygon = Polygon2D();

			if (coords.empty()) {
				break;
			}

			coord = *coords.begin();
		} else {
			coord = *newCoord;
		}
	}

	return borderPolygons;
}

void RoomImage::getBorderPolygons(unsigned char distance, std::vector<Polygon2D> &borderPolygons,
                                  std::vector<Polygon2D> &doorPolygons) const
{
	enum CoordType {
		OUTSIDE,
		WALL_OR_OBJECT_OUTLINE,
		DOOR,
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
			} else if (isGray(bytes)) {
				coordTypes[coord] = DOOR;
			} else {
				coordTypes[coord] = INSIDE;
			}
		}
	}

	// mark points as collision which can't be passed by the moving object
	CoordTypesMap copyCoordTypes = coordTypes;

	for (CoordTypesMap::const_iterator it = copyCoordTypes.begin(); it != copyCoordTypes.end(); it++) {
		if (it->second != INSIDE && it->second != DOOR) {
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

	std::set<Coord2D> insideCoords;
	std::set<Coord2D> doorCoords;

	// first find all coordinates inside the room
	for (CoordTypesMap::const_iterator it = coordTypes.begin(); it != coordTypes.end(); it++) {
		bool insideCoord = it->second == INSIDE;
		bool doorCoord = it->second == DOOR;

		if (!insideCoord && !doorCoord) {
			continue;
		}

		std::vector<Coord2D> neighbours = createNeighbours(it->first, width(), height());

		// expand neighbours
		for (std::vector<Coord2D>::const_iterator nit = neighbours.begin(); nit != neighbours.end(); nit++) {
			// find the type of the neighbour coordinate
			CoordTypesMap::const_iterator foundCoordType = coordTypes.find(*nit);

			assert(foundCoordType != coordTypes.end());

			if (doorCoord && foundCoordType->second != DOOR) {
				doorCoords.insert(it->first);
			}

			if (foundCoordType->second != INSIDE && foundCoordType->second != DOOR) {
				insideCoords.insert(it->first);
				break;
			}
		}
	}

	borderPolygons = expandPolygon(insideCoords);
	doorPolygons = expandPolygon(doorCoords);
}
