#ifndef _ROLLER_VIEW_H
#define _ROLLER_VIEW_H
//-------------------------------------------------------------------------------------------------
#include "types.h"
//-------------------------------------------------------------------------------------------------

typedef struct
{
  float fChaseDistance;
  float fChaseMinDistance;
  float fChasePullNormal;
  float fChasePullCrash;
  float fChasePullDefault;
  float fChaseLookAhead;
} tViewData;

//-------------------------------------------------------------------------------------------------

typedef struct
{
  tVec3 pos;
  float fDistance;
} tCameraPos;

//-------------------------------------------------------------------------------------------------

extern tViewData viewdata[2];
extern int chaseview[2];
extern float CHASE_DIST[2];
extern float CHASE_MIN[2];
extern float PULLZ[2];
extern float LOOKZ[2];
extern int nextpoint[2];
extern tCameraPos lastpos[2][64];
extern int lastcamelevation;
extern int lastcamdirection;
extern int NearTow;
extern float chase_x;
extern float chase_y;
extern float chase_z;

//-------------------------------------------------------------------------------------------------

int calculateview(int a1, int a2, int a3);
void initcarview(int iCarIdx, int iPlayer);
void newchaseview(int iCarIdx, int iChaseCamIdx);

//-------------------------------------------------------------------------------------------------
#endif