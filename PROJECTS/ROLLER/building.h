#ifndef _ROLLER_BUILDING_H
#define _ROLLER_BUILDING_H
//-------------------------------------------------------------------------------------------------
#include "types.h"
//-------------------------------------------------------------------------------------------------

typedef struct
{
  int iBuildingIdx;
  float fDepth;
} tVisibleBuilding;

//-------------------------------------------------------------------------------------------------

typedef struct
{
  float fZDepth;
  int iPolygonLink;
  int iPolygonIndex;
} tBuildingZOrderEntry;

//-------------------------------------------------------------------------------------------------

extern int BuildingSect[MAX_TRACK_CHUNKS];
extern float BuildingAngles[768];
extern int BuildingBase[256][4];
extern tVec3 BuildingBox[256][8];
extern float BuildingBaseX[256];
extern float BuildingBaseY[256];
extern float BuildingBaseZ[256];
extern float BuildingX[256];
extern float BuildingY[256];
extern float BuildingZ[256];
extern tVisibleBuilding VisibleBuildings[256];
extern int16 advert_list[256];
extern int NumBuildings;
extern int NumVisibleBuildings;

//-------------------------------------------------------------------------------------------------

void InitBuildings();
void CalcVisibleBuildings();
void DrawBuilding(int a1, uint8 *a2);
void init_animate_ads();
int bldZcmp(const void *pBuilding1, const void *pBuilding2);

//-------------------------------------------------------------------------------------------------
#endif