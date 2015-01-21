#ifndef ROB_STATS_H_INCLUDED
#define ROB_STATS_H_INCLUDED

#include "room.h"

#include <stdint.h>

struct Stats
{
	uint64_t lastPathCalculation;
	uint64_t lastPathCollisionCalculation;
	uint64_t lastCatmullRomCalculation;
	uint64_t lastSetNodes;
	uint64_t lastRoomTriangulationCalculation;
	Room::Algorithm lastUsedAlgorithm;
};

#endif // ROB_STATS_H_INCLUDED
