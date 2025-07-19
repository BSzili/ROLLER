#include "graphics.h"
#include "3d.h"
#include "transfrm.h"
#include "sound.h"
#include "roller.h"
#include "func2.h"
#include "car.h"
#include "polytex.h"
#include <stdbool.h>
#include <math.h>
#include <fcntl.h>
#ifdef IS_WINDOWS
#include <io.h>
#define open _open
#define close _close
#else
#include <inttypes.h>
#include <unistd.h>
#define O_BINARY 0 //linux does not differentiate between text and binary
#endif
//-------------------------------------------------------------------------------------------------

char revs_files1[6][13] = //000A41C0
{
  "minitext.bm",
  "font6.bm",
  "panel2.bm",
  "font3.bm",
  "pancar1.bm",
  ""
};
char revs_files2[6][13] = //000A420E
{
  "minitext.bm",
  "font6.bm",
  "panel2.bm",
  "font3.bm",
  "pancar2.bm",
  ""
};
char texture_file[12] = "texture.drh";  //000A425C
char bldtex_file[13] = "building.drh";  //000A4269
char gencartex_name[11] = "gentex.drh"; //000A4276
int car_remap[4096];    //001446C0
int cargen_remap[256];  //001486C0
int bld_remap[256];     //00148AC0
int num_textures[32];   //00148EC0
int remap_tex[256];     //00148F40
int mode_c[256];        //00149340
int gfx_size;           //00149740
int BldTextures;        //00149744
int NoOfTextures;       //00149748

//-------------------------------------------------------------------------------------------------

void plotxyz(float fWorldX, float fWorldY, float fWorldZ, char byColor)
{
  double dPerspectiveScale; // st6
  double dScreenX; // st5
  double dScreenY; // st7
  float fTransformedX; // [esp+4h] [ebp-24h]
  float fTransformedY; // [esp+8h] [ebp-20h]
  float fTransformedZ; // [esp+Ch] [ebp-1Ch]
  int iScreenXInt; // [esp+10h] [ebp-18h]
  float fViewDistance; // [esp+14h] [ebp-14h]

  // Transform world coords to camera-relative coords
  k1 = fWorldX - viewx;
  k2 = fWorldY - viewy;
  k3 = fWorldZ - viewz;

  // Frustum culling - only process points within max render distance (5600)
  if (fabs(k1) <= 5600.0 && fabs(k2) <= 5600.0 && fabs(k3) <= 5600.0) {
    // Apply camera transformation mat
    fTransformedY = k1 * vk2 + k2 * vk5 + k3 * vk8;// right vector proj
    fTransformedZ = k1 * vk3 + k2 * vk6 + k3 * vk9;// up vector proj

    fViewDistance = (float)VIEWDIST;

    // Only render points in front of camera (positive z)
    if (fTransformedZ >= (double)fViewDistance) {
      dPerspectiveScale = 1.0 / fTransformedZ;

      // Transform X to camera space using forward vec
      fTransformedX = k1 * vk1 + k2 * vk4 + k3 * vk7;
      dScreenX = fViewDistance * fTransformedX * dPerspectiveScale + (double)xbase;
      dScreenX = round(dScreenX); //_CHP
      iScreenXInt = (int)dScreenX;
      dScreenY = dPerspectiveScale * (fViewDistance * fTransformedY) + (double)ybase;
      dScreenY = round(dScreenY); //_CHP

      // Clip to screen bounds and plot pixel to 320x200 screen
      if ((int)dScreenX >= 0 && iScreenXInt <= 319 && (unsigned int)(int)dScreenY < 200)
        // plot pixel with y coord flipped (199-y for top-down screen layout)
        // screen buffer offset: (x + width*flippedY)
        scrbuf[iScreenXInt + winw * (199 - (int)dScreenY)] = byColor;
    }
  }
}

//-------------------------------------------------------------------------------------------------

void linexyz(uint8 *pScrBuf, float fX1, float fY1, float fZ1, float fX2, float fY2, float fZ2, uint8 byColor)
{
  double dViewDist; // st7
  double dClipInterpolationFactor; // st6
  double dScreenCenterX; // st5
  double dScrScale; // st4
  double dScrScale_1; // rt2
  double dScreenCenterY; // st4
  int iScreenX1; // edi
  int iScreenY1; // eax
  int iScreenX2; // eax
  float fClippedRightProj; // [esp+4h] [ebp-3Ch]
  float fRightProj_1; // [esp+8h] [ebp-38h]
  float fForwardProj; // [esp+Ch] [ebp-34h]
  float fClippedForwardProj; // [esp+10h] [ebp-30h]
  float fRightProj; // [esp+14h] [ebp-2Ch]
  float fViewDist; // [esp+18h] [ebp-28h]
  float fUpProj2; // [esp+1Ch] [ebp-24h]
  float fDepthOffset2; // [esp+1Ch] [ebp-24h]
  float fFinalDepth2; // [esp+1Ch] [ebp-24h]
  float fUpProj; // [esp+20h] [ebp-20h]
  float fDepthOffset1; // [esp+20h] [ebp-20h]
  float fFinalDepth1; // [esp+20h] [ebp-20h]
  int iScaledScreenX1; // [esp+24h] [ebp-1Ch]
  int iScaledScreenY2; // [esp+24h] [ebp-1Ch]

  // Transform both points from world space to camera-relative space
  k1 = fX1 - viewx; // Point 1 relative to camera
  k2 = fY1 - viewy;
  k3 = fZ1 - viewz;
  k4 = fX2 - viewx; // Point 2 relative to camera
  k5 = fY2 - viewy;
  k6 = fZ2 - viewz;

  // Apply camera transformation matrix to both points
  // Transform point 1 to camera space
  fForwardProj = k1 * vk1 + k2 * vk4 + k3 * vk7; // Forward (Z) projection
  fRightProj_1 = k1 * vk2 + k2 * vk5 + k3 * vk8; // Right (X) projection for point 1
  fUpProj = k1 * vk3 + k2 * vk6 + k3 * vk9;      // Up (Y) projection

  // Transform point 2 to camera space
  fClippedForwardProj = k4 * vk1 + k5 * vk4 + k6 * vk7; // Forward projection for point 2
  fRightProj = k4 * vk2 + k5 * vk5 + k6 * vk8;          // Right (X) projection for point 2
  fUpProj2 = k4 * vk3 + k5 * vk6 + k6 * vk9;            // Up (Y) projection for point 2

  dViewDist = (double)VIEWDIST;

  // Calculate distance from near clipping plane
  fDepthOffset1 = fUpProj - (float)dViewDist;
  fDepthOffset2 = fUpProj2 - (float)dViewDist;

  // Check if line crosses the near clipping plane and perform clipping
  if (fDepthOffset1 < 0.0 && fDepthOffset2 > 0.0 || fDepthOffset1 > 0.0 && fDepthOffset2 < 0.0) {
    // Line crosses near plane - interpolate to find clipping point
    dClipInterpolationFactor = 1.0 / (fDepthOffset1 - fDepthOffset2);
    fClippedRightProj = (float)(dClipInterpolationFactor * (fRightProj * fDepthOffset1 - fDepthOffset2 * fRightProj_1));

    if (fDepthOffset1 >= 0.0) {
      // Point 1 is behind near plane, clip it
      fDepthOffset2 = 0.0;
      fRightProj = fClippedRightProj;
      fForwardProj = (float)((fClippedForwardProj * fDepthOffset1 - fForwardProj * fDepthOffset2) * dClipInterpolationFactor);
    } else {
      // Point 2 is behind near plane, clip it
      fDepthOffset1 = 0.0;
      fRightProj_1 = fClippedRightProj;
      fClippedForwardProj = (float)((fClippedForwardProj * fDepthOffset1 - fForwardProj * fDepthOffset2) * dClipInterpolationFactor);
    }
  }

  fViewDist = (float)VIEWDIST;

  // Add view distance back to get final depth values
  fFinalDepth1 = fDepthOffset1 + fViewDist;
  fFinalDepth2 = fDepthOffset2 + fViewDist;

  // Only draw if both points are in front of the near plane
  if (fFinalDepth1 >= (double)fViewDist && fFinalDepth2 >= (double)fViewDist) {
    dScreenCenterX = (double)xbase;
    dScrScale = (double)scr_size;
    //_CHP();

    // Project point 1 to screen coordinates with perspective division
    // X coordinate for point 1
    iScaledScreenX1 = (int)((fViewDist * fForwardProj * (1.0 / fFinalDepth1) + dScreenCenterX) * dScrScale);
    dScrScale_1 = dScrScale;
    dScreenCenterY = (double)ybase;
    //_CHP();

    // Convert scaled X coordinate to pixel coordinate (divide by 64 for fixed-point)
    iScreenX1 = iScaledScreenX1 >> 6;

    // Y coordinate for point 1
    int iScaledScreenY1 = (int)((199.0 - 1.0 / fFinalDepth1 * (fViewDist * fRightProj_1) - dScreenCenterY) * dScrScale_1);
    iScreenY1 = iScaledScreenY1 >> 6;
    //_CHP();

    // X coordinate for point 2
    int iScaledScreenX2 = (int)((fViewDist * fClippedForwardProj * (1.0 / fFinalDepth2) + dScreenCenterX) * dScrScale_1);
    iScreenX2 = iScaledScreenX2 >> 6;
    //_CHP();

    // Y coordinate for point 2  
    iScaledScreenY2 = (int)((199.0 - 1.0 / fFinalDepth2 * (fViewDist * fRightProj) - dScreenCenterY) * dScrScale_1);

    // Draw line between the two projected points
    compout(
      pScrBuf,
      iScreenX1,                    // X coordinate of point 1
      iScreenY1,                    // Y coordinate of point 1  
      iScreenX2,                    // X coordinate of point 2
      iScaledScreenY2 >> 6,         // Y coordinate of point 2
      byColor);
  }
}

//-------------------------------------------------------------------------------------------------

void compout(uint8 *pScrBuf, int iX0, int iY0, int iX1, int iY1, uint8 byColor)
{
  int iNewX; // edi
  int iNewY; // esi
  int iX0_1; // ebp
  char byOutcode0; // cl
  char byOutcode1; // dl
  int iDy; // ecx
  int iYOffset; // eax
  char *p_byOutcode; // ecx
  char byOutcode; // dl
  int iDx; // [esp+8h] [ebp-28h]
  char byUseOutcode; // [esp+10h] [ebp-20h]
  char byOutcode1_1; // [esp+14h] [ebp-1Ch] BYREF
  char byOutcode0_1; // [esp+18h] [ebp-18h] BYREF
  char byDone; // [esp+1Ch] [ebp-14h]
  char byDraw; // [esp+20h] [ebp-10h]

  iX0_1 = iX0;
  byOutcode0 = 0;
  byDraw = 0;
  byDone = 0;

  // Get region code for point 0
  if (iY0 < winh) {
    if (iY0 < 0)
      byOutcode0 = 4;                           // bottom
  } else {
    byOutcode0 = 8;                             // top
  }
  if (iX0 < winw) {
    if (iX0 < 0)
      ++byOutcode0;                             // left
  } else {
    byOutcode0 += 2;                            // right
  }
  byOutcode0_1 = byOutcode0;
  byOutcode1 = 0;

  // Get region code for point 1
  if (iY1 < winh) {
    if (iY1 < 0)
      byOutcode1 = 4;                           // bottom
  } else {
    byOutcode1 = 8;                             // top
  }
  if (iX1 < winw) {
    if (iX1 < 0)
      ++byOutcode1;                             // left
  } else {
    byOutcode1 += 2;                            // right
  }
  byOutcode1_1 = byOutcode1;

  // Clipping loop (Cohen-Sutherland algorithm)
  do {
    // Both points inside screen, draw line
    if (!byOutcode0_1 && !byOutcode1_1) {
      byDraw = -1;
      byDone = -1;
      continue;
    }
    // Both points outside screen, don't draw line
    if (((uint8)byOutcode1_1 & (uint8)byOutcode0_1) != 0) {
      byDone = -1;
      continue;
    }

    // Select point to clip
    if (byOutcode0_1)
      byUseOutcode = byOutcode0_1;
    else
      byUseOutcode = byOutcode1_1;

    // Calculate intersection with window boundary
    iDx = iX1 - iX0_1;
    iDy = iY1 - iY0;
    if ((byUseOutcode & 8) != 0)              // top
    {
      iNewY = winh - 1;
      iNewX = iDx * (winh - 1 - iY0) / iDy + iX0_1;
    } else {
      if ((byUseOutcode & 4) == 0)            // bottom
      {
        if ((byUseOutcode & 2) != 0)          // right
        {
          iNewX = winw - 1;
          iYOffset = iDy * (winw - 1 - iX0_1) / iDx;
        } else {
          if ((byUseOutcode & 1) == 0)        // left
            goto LABEL_35;
          iYOffset = iDy * -iX0_1 / iDx;
          iNewX = 0;
        }
        iNewY = iY0 + iYOffset;
        goto LABEL_35;
      }
      iNewY = 0;
      iNewX = iDx * -iY0 / iDy + iX0_1;
    }
  LABEL_35:
      // Update clipped point and recompute outcode
    if (byUseOutcode == byOutcode0_1) {
      p_byOutcode = &byOutcode0_1;
      iX0_1 = iNewX;
      iY0 = iNewY;
      byOutcode = 0;
      if (iNewY < winh) {
        if (iNewY < 0)
          byOutcode = 4;
      } else {
        byOutcode = 8;
      }
      if (iNewX < winw) {
        if (iNewX < 0)
          ++byOutcode;
      } else {
        byOutcode += 2;
      }
    } else {
      p_byOutcode = &byOutcode1_1;
      iX1 = iNewX;
      iY1 = iNewY;
      byOutcode = 0;
      if (iNewY < winh) {
        if (iNewY < 0)
          byOutcode = 4;
      } else {
        byOutcode = 8;
      }
      if (iNewX < winw) {
        if (iNewX < 0)
          ++byOutcode;
      } else {
        byOutcode += 2;
      }
    }
    *p_byOutcode = byOutcode;
  } while (!byDone);
  if (byDraw)
    line(pScrBuf, iX0_1, iY0, iX1, iY1, byColor);
}

//-------------------------------------------------------------------------------------------------

void line(uint8 *pScrBuf, int iX0, int iY0, int iX1, int iY1, uint8 byColor)
{
  int iOldWinw; // ebp
  int iTempX; // esi
  int iTempY; // esi
  int iDx; // edi
  uint8 *pDest; // eax
  int iDy; // esi
  int iDy_1; // esi
  int iError_2; // ebx
  int iError_3; // edx
  bool bAboveTarget; // cc
  int iError_1; // ebx
  int iError; // edx
  int iTempX2; // [esp+4h] [ebp-10h]

  iOldWinw = winw;
  iTempX2 = iX1;

  // single point check
  if (iX0 == iX1 && iY0 == iY1) {
    pScrBuf[iX1 + winw * iY0] = byColor;        // draw pixel
  } else {
    // Swap points if needed to ensure left to right drawing
    if (iX0 > iX1) {
      iTempX = iX0;
      iX0 = iX1;
      iTempX2 = iTempX;
      iTempY = iY0;
      iY0 = iY1;
      iY1 = iTempY;
    }
    iDx = iTempX2 - iX0;
    pDest = &pScrBuf[iX0 + winw * iY0];
    iDy = iY1 - iY0;

    // positive slope
    if (iY1 - iY0 >= 0) {
      // steep slope (|m| > 1)
      if (iDx <= iDy) {
        iError = iDy >> 1;
        while (iY0 <= iY1) {
          ++iY0;
          iError -= iDx;
          *pDest = byColor;                     // draw pixel
          pDest += iOldWinw;                    // move down
          if (iError < 0) {
            ++pDest;                            // move right
            iError += iDy;
          }
        }
      } else                                      // gentle slope (|m| <= 1)
      {
        iError_1 = (iTempX2 - iX0) >> 1;
        while (iX0 <= iTempX2) {
          ++pDest;                              // move right
          ++iX0;
          iError_1 -= iDy;
          *(pDest - 1) = byColor;               // draw pixel
          if (iError_1 < 0) {
            iError_1 += iDx;
            pDest += iOldWinw;                  // move down
          }
        }
      }
    } else                                        // negative slope
    {
      iDy_1 = iY0 - iY1;
      // steep slope (|m| > 1)
      if (iDx <= iY0 - iY1) {
        iError_3 = iDy_1 >> 1;
        for (bAboveTarget = iY0 < iY1; !bAboveTarget; bAboveTarget = iY0 < iY1) {
          --iY0;
          iError_3 -= iDx;
          *pDest = byColor;                     // draw pixel
          pDest -= iOldWinw;                    // move up
          if (iError_3 < 0) {
            ++pDest;                            // move right
            iError_3 += iDy_1;
          }
        }
      } else                                      // gentle slope (|m| <= 1)
      {
        iError_2 = (iTempX2 - iX0) >> 1;
        while (iX0 <= iTempX2) {
          ++pDest;                              // move right
          ++iX0;
          iError_2 -= iDy_1;
          *(pDest - 1) = byColor;               // draw pixel
          if (iError_2 < 0) {
            iError_2 += iDx;
            pDest -= iOldWinw;                  // move up
          }
        }
      }
    }
  }
  winw = iOldWinw;
}

//-------------------------------------------------------------------------------------------------

void LoadPanel()
{
  int iRevIdx;
  char *szRevPtr;
  const char *szRevFile;
  int iFileHandle;
  uint32 uiFileLength;
  void *pBuf;

  iRevIdx = 0;

  if ((textures_off & 0x10000) != 0) {
    szRevPtr = revs_files2[0];
    while (revs_files2[iRevIdx][0]) {
      szRevFile = revs_files2[iRevIdx];

      // Check if file exists
      iFileHandle = ROLLERopen(szRevFile, O_RDONLY | O_BINARY); //0x200 is O_BINARY in WATCOM/h/fcntl.h
      if (iFileHandle == -1) {
        ErrorBoxExit("Unable to open %s", szRevFile);
        //printf("Unable to open %s\n\n", szRevFile);
        //doexit();
      }
      close(iFileHandle);

      // Load the compressed file
      uiFileLength = getcompactedfilelength(szRevPtr);
      pBuf = getbuffer(uiFileLength);
      rev_vga[iRevIdx] = pBuf;  // Store buffer pointer in array
      loadcompactedfile(szRevPtr, (uint8 *)pBuf);

      ++iRevIdx;
      szRevPtr += 13;  // Move to next filename
    }
  } else {
    szRevPtr = revs_files1[0];
    while (revs_files1[iRevIdx][0]) {
      szRevFile = revs_files1[iRevIdx];

      // Check if file exists
      iFileHandle = ROLLERopen(revs_files1[iRevIdx], O_RDONLY | O_BINARY); //0x200 is O_BINARY in WATCOM/h/fcntl.h
      if (iFileHandle == -1) {
        ErrorBoxExit("Unable to open %s", szRevFile);
        //printf("Unable to open %s\n\n", szRevFile);
        //doexit();
      }
      close(iFileHandle);

      // Load the compressed file
      uiFileLength = getcompactedfilelength(szRevPtr);
      pBuf = getbuffer(uiFileLength);
      rev_vga[iRevIdx] = pBuf;  // Store buffer pointer in array
      loadcompactedfile(szRevPtr, (uint8 *)pBuf);

      ++iRevIdx;
      szRevPtr += 13;  // Move to next filename
    }
  }
}

//-------------------------------------------------------------------------------------------------

void InitRemaps()
{
  int iCarIdx; // edi
  int iCartexIdx; // esi
  int iCarTexsLoaded; // ecx

  init_remap(cargen_vga, 18, num_textures[18], gfx_size);
  init_remap(building_vga, 17, num_textures[17], gfx_size);
  iCarIdx = 0;
  if (numcars > 0) {
    iCartexIdx = 0;
    do {
      iCarTexsLoaded = car_texs_loaded[iCartexIdx];
      if (iCarTexsLoaded != -1)
        init_remap(
          cartex_vga[iCarTexsLoaded + 1],
          car_texs_loaded[iCartexIdx] - 1,
          num_textures[iCarTexsLoaded + 1],
          gfx_size);
      ++iCarIdx;
      ++iCartexIdx;
    } while (iCarIdx < numcars);
  }
  init_remap(texture_vga, -1, num_textures[19], gfx_size);
}

//-------------------------------------------------------------------------------------------------

void LoadGenericCarTextures()
{
  int iFileHandle; // edx
  signed int iFileLength; // ecx
  int iNumTextures; // eax
  int iNumTextures_1; // esi
  int iFinalTexCount; // ecx
  int iMapSelMode; // ebx
  uint8 *pFileBuf; // [esp+0h] [ebp-14h] BYREF

  // Check if generic car texture file exists
  iFileHandle = ROLLERopen(gencartex_name, O_RDONLY | O_BINARY); //0x200 is O_BINARY in WATCOM/h/fcntl.h
  if (iFileHandle == -1) {
    ErrorBoxExit("Unable to open texture map data file <%s>", gencartex_name);
    //printf("Unable to open texture map data file <%s>\n\n", gencartex_name);
    //doexit();
  }
  close(iFileHandle);

  // Get compressed file size and calculate number of texture blocks
  iFileLength = getcompactedfilelength(gencartex_name);
  iNumTextures = iFileLength / 4096;
  iNumTextures_1 = iNumTextures;

  if (gfx_size == 1) {
    // Allocate buffer for processd textures (aligned to 8-tex boundaries)
    // Each texture 32x32 grouped in sets of 8
    cargen_vga = (uint8 *)getbuffer((((int16)iNumTextures + 7) & 0xFFF8) << 10);

    // Allocate temp buf
    pFileBuf = (uint8 *)getbuffer(iFileLength);

    // Load tex data
    loadcompactedfile(gencartex_name, pFileBuf);

    // Recalculate tex count
    iFinalTexCount = iFileLength / 4096;

    // Process 32x32 textures and reorganize
    sort_small_texture(cargen_vga, pFileBuf, iNumTextures_1);

    // setmapsel mode
    iMapSelMode = -1;

    // Cleanup
    fre((void **)&pFileBuf);
  } else {
    // Allocate buffer for processed textures (aligned to 4-texture boundaries)
    // Each texture 64x64 grouped in sets of 4
    cargen_vga = (uint8 *)getbuffer((((int16)iNumTextures + 3) & 0xFFFC) << 12);

    // Recalculate tex count
    iFinalTexCount = iFileLength / 4096;

    // Load tex data
    loadcompactedfile(gencartex_name, cargen_vga);

    // setmapsel mode
    iMapSelMode = 0;

    // Process 64x64 textures and reorganize
    sort_texture(cargen_vga, iNumTextures_1);
  }

  // Setup tex mapping selector
  setmapsel(cargen_vga, 18, iMapSelMode, iFinalTexCount);

  // Update count
  num_textures[18] = iNumTextures_1;
}

//-------------------------------------------------------------------------------------------------

void LoadCarTexture(int iCartexIdx, uint8 byTexSlotIdx)
{
  int iFileHandle; // edx
  int iCompressedFileLength; // ecx
  int iNumTexBlocks; // eax
  uint8 *pTexBuf; // ebx
  int iNumTexsToProcess; // ebp
  int iChunkDataSize; // edx
  uint8 *pChunkData; // eax
  int iTexBlockIdx; // edi
  uint8 *pSourcePixel; // eax
  int iPxRowIdx; // esi
  int iPixelGroupIdx; // edx
  uint8 *pDestPixel; // ebx
  uint8 byPx1; // cl
  uint8 *pNextSourcePixel; // eax
  uint8 byPx2; // cl
  uint8 byPx3; // cl
  uint8 byPx4; // cl
  int iChunkDataSize_1; // edx
  uint8 *pChunkData_1; // eax
  int iTexBlockIdx_1; // edi
  uint8 *pSourcePixel_1; // eax
  int iPxRowIdx_1; // esi
  int iPixelGroupIdx_1; // edx
  uint8 *pDestPixel_1; // ebx
  uint8 byPx1_1; // cl
  uint8 *pNextSourcePixel_1; // eax
  uint8 byPx2_1; // cl
  uint8 byPx3_1; // cl
  uint8 byPx4_1; // cl
  uint8 *pTexBuf_1; // eax
  int iTexSlotIdx; // [esp+0h] [ebp-28h]
  int iTotalFileLength; // [esp+8h] [ebp-20h]
  char *szTexFile; // [esp+Ch] [ebp-1Ch]
  int iRemainingFileLength; // [esp+10h] [ebp-18h]

  iTexSlotIdx = byTexSlotIdx;
  szTexFile = car_texture_names[iCartexIdx];
  if (!iCartexIdx) {
    printf("Exiting name is %s\n", szTexFile);
    doexit();
  }

  // Advanced cars
  if ((textures_off & 0x10000) != 0 && iCartexIdx >= 1 && iCartexIdx <= 8)
    *szTexFile = 'y';                           // load Y tex files

  // Check if file exists
  iFileHandle = ROLLERopen(szTexFile, O_RDONLY | O_BINARY); //0x200 is O_BINARY in WATCOM/h/fcntl.h
  if (iFileHandle == -1) {
    ErrorBoxExit("Unable to open texture map data file <%s>", szTexFile);
    //printf("Unable to open texture map data file <%s>\n\n", szTexFile);
    //doexit();
  }
  close(iFileHandle);

  // Get file size
  iCompressedFileLength = getcompactedfilelength(szTexFile);
  if (iCompressedFileLength == -1) {
    printf("File size error in texture map data file\n\n");
    doexit();
  }

  // Caclulate number of texture blocks (4096 bytes or 64x64 px)
  iNumTexBlocks = iCompressedFileLength / 4096;
  iTotalFileLength = iNumTexBlocks;

  // 32x32 textures
  if (gfx_size == 1) {
    // Init mangled file reading
    initmangle(szTexFile);
    iRemainingFileLength = iCompressedFileLength;

    // Allocate buffer for processed texture (aligned to 8-tex boundaries)
    pTexBuf = (uint8 *)getbuffer((((int16)iTotalFileLength + 7) & 0xFFF8) << 10);
    cartex_vga[iTexSlotIdx - 1] = pTexBuf;

    // Process file in chunks due to memory constraints
    while (iRemainingFileLength > 0) {
      iNumTexsToProcess = iRemainingFileLength / 4096;

        // Low memory mode
      if (no_mem) {
        if (iRemainingFileLength <= 20480)    // 5 blocks max (5 * 4096)
        {
          iChunkDataSize = iRemainingFileLength;
          pChunkData = scrbuf;
        } else {
          iChunkDataSize = 20480;
          pChunkData = scrbuf;
          iNumTexsToProcess = 5;
        }
        readmangled(pChunkData, iChunkDataSize);

        // Process each tex block in chunk
        iTexBlockIdx = 0;
        for (pSourcePixel = scrbuf + 40000; iTexBlockIdx < iNumTexsToProcess; ++iTexBlockIdx) {
          // Process each row of 32x32 tex
          for (iPxRowIdx = 0; iPxRowIdx < 32; ++iPxRowIdx) {
            iPixelGroupIdx = 0;

            // Process 32 pixels per row (8 groups of 4 pixels)
            do {
              pDestPixel = pTexBuf + 1;
              byPx1 = *pSourcePixel;
              pNextSourcePixel = pSourcePixel + 2;
              iPixelGroupIdx += 4;
              *(pDestPixel++ - 1) = byPx1;

              byPx2 = *pNextSourcePixel;
              pNextSourcePixel += 2;
              *(pDestPixel++ - 1) = byPx2;

              byPx3 = *pNextSourcePixel;
              pNextSourcePixel += 2;
              *(pDestPixel - 1) = byPx3;

              pTexBuf = pDestPixel + 1;
              byPx4 = *pNextSourcePixel;
              pSourcePixel = pNextSourcePixel + 2;
              *(pTexBuf - 1) = byPx4;
            } while (iPixelGroupIdx < 32);
            pSourcePixel += 64;                 // skip to next row
          }
        }
        iRemainingFileLength -= 20480;
      } else                                      // normal memory mode
      {
        if (iRemainingFileLength <= 196608)   // 48 blocks max (48 * 4096)
        {
          pChunkData_1 = scrbuf;
          iChunkDataSize_1 = iRemainingFileLength;
        } else {
          iChunkDataSize_1 = 196608;            // process 48 blocks at a time
          pChunkData_1 = scrbuf;
          iNumTexsToProcess = 48;
        }
        readmangled(pChunkData_1, iChunkDataSize_1);

        // Process each texture block in chunk
        iTexBlockIdx_1 = 0;
        for (pSourcePixel_1 = scrbuf + 40000; iTexBlockIdx_1 < iNumTexsToProcess; ++iTexBlockIdx_1) {
          // Process each row of 32x32 texture
          for (iPxRowIdx_1 = 0; iPxRowIdx_1 < 32; ++iPxRowIdx_1) {
            iPixelGroupIdx_1 = 0;
            do {
              pDestPixel_1 = pTexBuf + 1;

              byPx1_1 = *pSourcePixel_1;
              pNextSourcePixel_1 = pSourcePixel_1 + 2;
              iPixelGroupIdx_1 += 4;
              *(pDestPixel_1++ - 1) = byPx1_1;

              byPx2_1 = *pNextSourcePixel_1;
              pNextSourcePixel_1 += 2;
              *(pDestPixel_1++ - 1) = byPx2_1;

              byPx3_1 = *pNextSourcePixel_1;
              pNextSourcePixel_1 += 2;
              *(pDestPixel_1 - 1) = byPx3_1;
              pTexBuf = pDestPixel_1 + 1;

              byPx4_1 = *pNextSourcePixel_1;
              pSourcePixel_1 = pNextSourcePixel_1 + 2;
              *(pTexBuf - 1) = byPx4_1;
            } while (iPixelGroupIdx_1 < 32);
            pSourcePixel_1 += 64;               // skip to next row
          }
        }
        iRemainingFileLength -= 196608;
      }
    }

    // Cleanup
    uninitmangle();

    // Reorganize texture memory layout
    sort_mini_texture(cartex_vga[iTexSlotIdx - 1], iTotalFileLength);

    // Setup tex mapping selectors
    setmapsel(cartex_vga[iTexSlotIdx - 1], iTexSlotIdx, -1, iTotalFileLength);

    // Store num textures
    num_textures[iTexSlotIdx - 1] = iTotalFileLength;
  } else {
    // Allocate buffer for processed textures (aligned to 4-tex boundaries)
    pTexBuf_1 = (uint8 *)getbuffer((((int16)iNumTexBlocks + 3) & 0xFFFC) << 12);
    cartex_vga[iTexSlotIdx - 1] = pTexBuf_1;

    // Load directly into buffer
    loadcompactedfile(szTexFile, pTexBuf_1);

    // Reorganize tex memory layout
    sort_texture(cartex_vga[iTexSlotIdx - 1], iTotalFileLength);

    // Setup tex mapping selectors
    setmapsel(cartex_vga[iTexSlotIdx - 1], iTexSlotIdx, 0, iTotalFileLength);

    // Store num textures
    num_textures[iTexSlotIdx - 1] = iTotalFileLength;
  }

  // Restore original filename if modified
  if ((textures_off & 0x10000) != 0 && iCartexIdx >= 1 && iCartexIdx <= 8)
    *szTexFile = 'x';
}

//-------------------------------------------------------------------------------------------------

void LoadBldTextures()
{
  int iFileHandle; // edx
  signed int iCompressedFileLength; // ecx
  int iNumTextureBlocks; // eax
  int iTexCount; // esi
  int iFinalTexCount; // ecx
  int iMapSelMode; // ebx
  char *pTempBuf; // [esp+0h] [ebp-14h] BYREF

  // free existing bld
  fre((void **)&building_vga);

  // Check if bld file exists
  iFileHandle = ROLLERopen(bldtex_file, O_RDONLY | O_BINARY); //0x200 is O_BINARY in WATCOM/h/fcntl.h
  if (iFileHandle == -1) {
    ErrorBoxExit("Unable to open bld texture map data file");
    //printf("Unable to open bld texture map data file\n\n");
    //doexit();
  }
  close(iFileHandle);

  // Get compressed file size and calculate number of tex blocks
  iCompressedFileLength = getcompactedfilelength(bldtex_file);
  iNumTextureBlocks = iCompressedFileLength / 4096;
  iTexCount = iNumTextureBlocks;

  // 32x32 mode
  if (gfx_size == 1) {
    // Load and process 32x32 textures
    building_vga = (uint8 *)getbuffer((((int16)iNumTextureBlocks + 7) & 0xFFF8) << 10);
    pTempBuf = (char *)getbuffer(iCompressedFileLength);
    loadcompactedfile(bldtex_file, (uint8 *)pTempBuf);
    iFinalTexCount = iCompressedFileLength / 4096;
    sort_small_texture(building_vga, (uint8 *)pTempBuf, iTexCount);
    iMapSelMode = -1;
    fre((void **)&pTempBuf);
  } else                                          // 64x64 mode
  {
    // Load and process 64x64 textures
    building_vga = (uint8 *)getbuffer((((int16)iNumTextureBlocks + 3) & 0xFFFC) << 12);
    iFinalTexCount = iCompressedFileLength / 4096;
    loadcompactedfile(bldtex_file, building_vga);
    iMapSelMode = 0;
    sort_texture(building_vga, iTexCount);
  }

  // Store tex counts and setup mapping selector
  BldTextures = iTexCount;
  setmapsel(building_vga, 17, iMapSelMode, iFinalTexCount);
  num_textures[17] = iTexCount;
}

//-------------------------------------------------------------------------------------------------

void LoadTextures()
{
  int iCompressedFileLength; // ecx
  int iNumTextureBlocks; // eax
  uint8 *pTexBuf; // ebx
  int iNumTexsToProcess; // ebp
  int iChunkDataSize; // edx
  uint8 *pChunkData; // eax
  int iTexBlockIdx; // edi
  uint8 *pSourcePixel; // eax
  int j; // esi
  int iPxGroupIdx; // edx
  uint8 *pDestPixel; // ebx
  uint8 byPx1; // cl
  uint8 *pNextSourcePixel; // eax
  uint8 byPx2; // cl
  uint8 byPx3; // cl
  uint8 byPx4; // cl
  int iChunkDataSize_1; // edx
  uint8 *pChunkData_1; // eax
  int iTexBlockIdx_1; // edi
  uint8 *pSourcePixel_1; // eax
  int iPxRowIdx; // esi
  int iPxGroupIdx_1; // edx
  uint8 *pDestPixel_1; // ebx
  uint8 byPx1_1; // cl
  uint8 *pNextSourcePixel_1; // eax
  uint8 byPx2_1; // cl
  uint8 byPx3_1; // cl
  uint8 byPx4_1; // cl
  int iFileHandle; // [esp+0h] [ebp-24h]
  int iTotalTextureBlocks; // [esp+4h] [ebp-20h]
  int iRemainingFileLength; // [esp+8h] [ebp-1Ch]

  // Free existing texture
  fre((void **)&texture_vga);

  // Check if tex file exists
  iFileHandle = ROLLERopen(texture_file, O_RDONLY | O_BINARY); //0x200 is O_BINARY in WATCOM/h/fcntl.h
  if (iFileHandle == -1) {
    ErrorBoxExit("Unable to open texture map data file");
    //printf("Unable to open texture map data file\n\n");
    //doexit();
  }
  close(iFileHandle);

  // Get compressed file size and calculate number of texture blocks
  iCompressedFileLength = getcompactedfilelength(texture_file);
  iNumTextureBlocks = iCompressedFileLength / 4096;
  iTotalTextureBlocks = iNumTextureBlocks;

  // 32x32 tex mode
  if (gfx_size == 1) {
    // Init mangled reading
    initmangle(texture_file);
    iRemainingFileLength = iCompressedFileLength;

    // Allocate buf for processed textures (aligned to 8-tex boundaries)
    pTexBuf = (uint8 *)getbuffer((((int16)iTotalTextureBlocks + 7) & 0xFFF8) << 10);
    texture_vga = pTexBuf;

    // Process file in chunks due to memory constraints
    while (iRemainingFileLength > 0) {
      iNumTexsToProcess = iRemainingFileLength / 4096;
      if (no_mem)                             // low memory mode
      {
        if (iRemainingFileLength <= 20480)    // 5 blocks max (5 * 4096)
        {
          iChunkDataSize = iRemainingFileLength;
          pChunkData = scrbuf;
        } else {
          iChunkDataSize = 20480;               // process 5 blocks at a time
          pChunkData = scrbuf;
          iNumTexsToProcess = 5;
        }

        // Read mangled data
        readmangled(pChunkData, iChunkDataSize);

        // Process each tex block in chunk
        iTexBlockIdx = 0;
        for (pSourcePixel = scrbuf + 40000; iTexBlockIdx < iNumTexsToProcess; ++iTexBlockIdx) {
          for (j = 0; j < 32; ++j) {
            iPxGroupIdx = 0;

            // Process 32 px per row (8 groups of 4 px)
            do {
              pDestPixel = pTexBuf + 1;

              byPx1 = *pSourcePixel;
              pNextSourcePixel = pSourcePixel + 2;
              iPxGroupIdx += 4;
              *(pDestPixel++ - 1) = byPx1;

              byPx2 = *pNextSourcePixel;
              pNextSourcePixel += 2;
              *(pDestPixel++ - 1) = byPx2;

              byPx3 = *pNextSourcePixel;
              pNextSourcePixel += 2;
              *(pDestPixel - 1) = byPx3;
              pTexBuf = pDestPixel + 1;

              byPx4 = *pNextSourcePixel;
              pSourcePixel = pNextSourcePixel + 2;
              *(pTexBuf - 1) = byPx4;
            } while (iPxGroupIdx < 32);
            pSourcePixel += 64;                 // skip to next row
          }
        }
        iRemainingFileLength -= 20480;
      } else                                      // normal memory mode
      {
        if (iRemainingFileLength <= 196608)   // 48 blocks max (48 * 4096)
        {
          pChunkData_1 = scrbuf;
          iChunkDataSize_1 = iRemainingFileLength;
        } else {
          iChunkDataSize_1 = 196608;            // process 48 blocks at a time
          pChunkData_1 = scrbuf;
          iNumTexsToProcess = 48;
        }

        // Read mangled data
        readmangled(pChunkData_1, iChunkDataSize_1);

        // process each tex block in chunk
        iTexBlockIdx_1 = 0;
        for (pSourcePixel_1 = scrbuf + 40000; iTexBlockIdx_1 < iNumTexsToProcess; ++iTexBlockIdx_1) {
          // Process each row of 32x32 tex
          for (iPxRowIdx = 0; iPxRowIdx < 32; ++iPxRowIdx) {
            iPxGroupIdx_1 = 0;

            // Process 32 px per row (8 groups of 4 px)
            do {
              pDestPixel_1 = pTexBuf + 1;

              byPx1_1 = *pSourcePixel_1;
              pNextSourcePixel_1 = pSourcePixel_1 + 2;
              iPxGroupIdx_1 += 4;
              *(pDestPixel_1++ - 1) = byPx1_1;

              byPx2_1 = *pNextSourcePixel_1;
              pNextSourcePixel_1 += 2;
              *(pDestPixel_1++ - 1) = byPx2_1;

              byPx3_1 = *pNextSourcePixel_1;
              pNextSourcePixel_1 += 2;
              *(pDestPixel_1 - 1) = byPx3_1;
              pTexBuf = pDestPixel_1 + 1;

              byPx4_1 = *pNextSourcePixel_1;
              pSourcePixel_1 = pNextSourcePixel_1 + 2;
              *(pTexBuf - 1) = byPx4_1;
            } while (iPxGroupIdx_1 < 32);
            pSourcePixel_1 += 64;               // skip to next row
          }
        }
        iRemainingFileLength -= 196608;
      }
    }

    // Cleanup, reorganize, and setup mapsel
    uninitmangle();
    sort_mini_texture(texture_vga, iTotalTextureBlocks);
    setmapsel(texture_vga, 0, -1, iTotalTextureBlocks);
    NoOfTextures = iTotalTextureBlocks;
    close(iFileHandle);
    num_textures[19] = iTotalTextureBlocks;
  } else                                          // 64x64 mode
  {
    // Allocate buffer for processed textures (aligned to 4-tex boundaries)
    texture_vga = (uint8 *)getbuffer((((int16)iNumTextureBlocks + 3) & 0xFFFC) << 12);
    loadcompactedfile(texture_file, texture_vga);
    sort_texture(texture_vga, iTotalTextureBlocks);
    setmapsel(texture_vga, 0, 0, iTotalTextureBlocks);
    NoOfTextures = iCompressedFileLength / 4096;
    num_textures[19] = NoOfTextures;
  }
}

//-------------------------------------------------------------------------------------------------

void init_remap(uint8 *pTextureBaseAddr, int iRemapType, int iNumBlocks, int iIsLowRes)
{
  int iBlockSize; // edi
  uint8 *pBlockRow; // ebx
  int iTexRowBytes_1; // eax
  uint8 *pBlockData; // ebx
  int iTotalB; // ebp
  int i; // ecx
  int j; // eax
  int iMaxColorIdx; // edx
  int iCurrColorIdx; // edx
  int iSearchIdx; // eax
  int iMaxColorCount; // ebx
  int iColorCount; // ebx
  int iColorCount_1; // esi
  int iAvgB; // ebp
  int iAvgG; // eax
  int iDominantColorIdx; // edx
  int iTotalPxCount; // [esp+0h] [ebp-40h]
  int iTexRowBytes; // [esp+10h] [ebp-30h]
  int iRemapAyOffset; // [esp+18h] [ebp-28h]
  int iBlockIdx; // [esp+1Ch] [ebp-24h]
  int iTotalG; // [esp+28h] [ebp-18h]
  int iTotalR; // [esp+2Ch] [ebp-14h]
  int iAvgR; // [esp+2Ch] [ebp-14h]
  int iDominantColorSearch; // [esp+30h] [ebp-10h]

  // Determine block size based on resolution
  if (iIsLowRes)
    iBlockSize = 32;
  else
    iBlockSize = 64;

  iBlockIdx = 0;

  if (iNumBlocks > 0) {
    iRemapAyOffset = 0;
    iTexRowBytes = iRemapType << 10;
    do {
      // Calculate tex memory addr for current block
      if (iIsLowRes) {
        // 8 blocks per row, each block 32px wide
        pBlockRow = &pTextureBaseAddr[0x2000 * (iBlockIdx >> 3)];// row start addr
        iTexRowBytes_1 = 32 * (iBlockIdx & 7);  // column offset
      } else {
        // 4 blocks per row, 64px wide
        pBlockRow = &pTextureBaseAddr[0x4000 * (iBlockIdx >> 2)];// row start addr
        iTexRowBytes_1 = (iBlockIdx & 3) << 6;  // column offset
      }

      pBlockData = &pBlockRow[iTexRowBytes_1];

      // clear color freq histogram
      //_STOSD(mode_c, 0, (int)pBlockData, 0x100u);
      memset(mode_c, 0, 256 * sizeof(int));

      iTotalB = 0;
      iTotalG = 0;
      iTotalR = 0;

      // Analyze all pixels in current block
      for (i = 0; i < iBlockSize; ++i) {
        for (j = 0; j < iBlockSize; ++j) {
          iMaxColorIdx = *pBlockData++;         // get pixel color index
          ++mode_c[iMaxColorIdx];               // inc frequency counter for this color
        }
        pBlockData += 256 - iBlockSize;         // skip to next row
      }

      iTotalPxCount = 0;

      for (iDominantColorSearch = 0; iDominantColorSearch < 4; ++iDominantColorSearch) {
        iCurrColorIdx = 0;
        iSearchIdx = 0;
        iMaxColorCount = 0;

        // Find color with highest frequency
        do {
          if (mode_c[iMaxColorCount] > mode_c[iCurrColorIdx])
            iCurrColorIdx = iSearchIdx;
          ++iSearchIdx;
          ++iMaxColorCount;
        } while (iSearchIdx < 256);

        iColorCount = mode_c[iCurrColorIdx];

        // Only include colors that appear frequently enough (4 * blocksize)
        if (iColorCount > 4 * iBlockSize) {
          iTotalPxCount += iColorCount;
          iTotalR += mode_c[iCurrColorIdx] * palette[iCurrColorIdx].byR;
          iColorCount_1 = mode_c[iCurrColorIdx];
          iTotalB += iColorCount_1 * palette[iCurrColorIdx].byB;
          iTotalG += iColorCount_1 * palette[iCurrColorIdx].byG;
          mode_c[iCurrColorIdx] = 0;
        }
      }

      // Calcualte avg color from dominant colors
      if (iTotalPxCount <= 0) {
        // No dominant colors found, use last checked color
        iDominantColorIdx = iCurrColorIdx;
        iAvgR = palette[iDominantColorIdx].byR;
        iAvgG = palette[iDominantColorIdx].byG;
        iAvgB = palette[iDominantColorIdx].byB;
      } else {
        // Calculate weighted avg of dominant colors
        iAvgR = iTotalR / iTotalPxCount;
        iAvgB = iTotalB / iTotalPxCount;
        iAvgG = iTotalG / iTotalPxCount;
      }

      // Apply brightness clamping for certain remap types
      if (iRemapType < 17) {
        if (iRemapType == -1) {
          // special case: clamp bright colors
          if (iAvgR > 28 && (iAvgB <= iAvgR) >= iAvgG)
            iAvgR = 28;
          if (iAvgB > 28 && (iAvgB <= iAvgR) >= iAvgG)
            iAvgB = 28;
          remap_tex[iRemapAyOffset] = nearest_colour(iAvgR, iAvgB, iAvgG);
          goto STORE_COMPLETE;
        }
      } else {
        if (iRemapType <= 17) {
          // Building remap type
          if (iAvgR > 28 && (iAvgB <= iAvgR) >= iAvgG)
            iAvgR = 28;
          if (iAvgB > 28 && (iAvgB <= iAvgR) >= iAvgG)
            iAvgB = 28;
          bld_remap[iRemapAyOffset] = nearest_colour(iAvgR, iAvgB, iAvgG);
          goto STORE_COMPLETE;
        }
        if (iRemapType == 18) {
          // Car remap type
          if (iAvgR > 28 && (iAvgB <= iAvgR) >= iAvgG)
            iAvgR = 28;
          if (iAvgB > 28 && (iAvgB <= iAvgR) >= iAvgG)
            iAvgB = 28;
          cargen_remap[iRemapAyOffset] = nearest_colour(iAvgR, iAvgB, iAvgG);
          goto STORE_COMPLETE;
        }
      }

      // Default: store in car_remap
      *(int *)((char *)car_remap + iTexRowBytes) = nearest_colour(iAvgR, iAvgB, iAvgG);
    STORE_COMPLETE:
      iTexRowBytes += 4;                        // move to next pos in remap array
      ++iRemapAyOffset;
      ++iBlockIdx;
    } while (iBlockIdx < iNumBlocks);
  }
}

//-------------------------------------------------------------------------------------------------

void sort_small_texture(uint8 *pDest, uint8 *pSrc, int iNumBlocks)
{
  //ROLLER: we do not need interleaved texture data for this implementation
  return;

  int iNumLayers; // eax
  int iCurrLayerSize; // ebp
  int iLayerPixelHeight; // ebp
  int iBlockRowOffset; // edi
  int iPxRowInBlock; // esi
  uint8 *pDestPixel; // eax
  int iPixelColGroup; // edx
  uint8 *pDestAddr; // eax
  uint8 byPx1; // cl
  uint8 *pSrcPixel; // ebx
  uint8 byPx2; // cl
  uint8 byPx3; // cl
  uint8 byPx4; // cl
  int iNumLayers_1; // [esp+0h] [ebp-20h]
  int iLayerIdx; // [esp+4h] [ebp-1Ch]
  int iRemainingBlocks; // [esp+8h] [ebp-18h]

  // Calculate number of complete 8-block layers
  iNumLayers = iNumBlocks / 8;
  iRemainingBlocks = iNumBlocks;
  iLayerIdx = 0;

  // Process each layer (up to 8 blocks per layer)
  for (iNumLayers_1 = iNumLayers; iLayerIdx <= iNumLayers_1; ++iLayerIdx) {
    // Determine how many blocks in this layer (max 8)
    if (iRemainingBlocks <= 7)
      iCurrLayerSize = iRemainingBlocks;
    else
      iCurrLayerSize = 8;
    if (iCurrLayerSize > 0) {
      iLayerPixelHeight = 32 * iCurrLayerSize;  // each block is 32px wide
      iBlockRowOffset = 0;

      // Process each block column in layer
      do {
        iPxRowInBlock = 0;
        pDestPixel = &pDest[iBlockRowOffset];

        // Process each row of pixels in block (32 rows)
        do {
          iPixelColGroup = 0;

          // Process each group of 4 pixels in row (8 groups = 32 pixels)
          do {
            pDestAddr = pDestPixel + 1;

            // Copy 4 px from src to dest
            byPx1 = *pSrc;
            pSrcPixel = pSrc + 2;
            iPixelColGroup += 4;
            *(pDestAddr++ - 1) = byPx1;

            byPx2 = *pSrcPixel;
            pSrcPixel += 2;
            *(pDestAddr++ - 1) = byPx2;

            byPx3 = *pSrcPixel;
            pSrcPixel += 2;
            *(pDestAddr - 1) = byPx3;
            pDestPixel = pDestAddr + 1;

            byPx4 = *pSrcPixel;
            pSrc = pSrcPixel + 2;
            *(pDestPixel - 1) = byPx4;
          } while (iPixelColGroup < 32);

          pDestPixel += 224;                    // skip to next row (256-32 = 224 px)
          ++iPxRowInBlock;
          pSrc += 64;                           // advance src ptr (skip padding or next row data)
        } while (iPxRowInBlock < 32);

        // move to next block column
        iBlockRowOffset += 32;
      } while (iBlockRowOffset < iLayerPixelHeight);
    }

    iRemainingBlocks -= 8;                      // process next layer
    pDest += 8192;                              // move to next tex layer (8192 bytes)
  }
}

//-------------------------------------------------------------------------------------------------

void sort_texture(uint8 *pTexData, int iNumTextures)
{
  //ROLLER: we do not need interleaved texture data for this implementation
  return;

  int iTexturesInGroup; // ebp
  int iSourceRowOffset; // esi
  uint8 *pTempRow; // ecx
  int iTexIdx; // edi
  int iPixelGroupIdx; // edx
  uint8 *pSourcePx; // eax
  uint8 *pPx1; // ecx
  uint8 byPx0; // bl
  uint8 *pSourcePx_1; // eax
  uint8 byPx2; // bl
  uint8 byPx3; // bl
  uint8 byPx4; // bl
  uint8 *pTempBuf; // [esp+0h] [ebp-34h] BYREF
  int iNumGroups; // [esp+4h] [ebp-30h]
  int iGroupIdx; // [esp+8h] [ebp-2Ch]
  int iRemainingTextures; // [esp+Ch] [ebp-28h]
  uint8 *pCurrTex; // [esp+10h] [ebp-24h]
  int iTempRowOffset; // [esp+14h] [ebp-20h]
  int iRowIdx; // [esp+18h] [ebp-1Ch]
  uint8 *pGroupStart; // [esp+1Ch] [ebp-18h] SPLIT

  iRemainingTextures = iNumTextures;
  pTempBuf = (uint8 *)getbuffer(0x4000u);       // 16KB temp buffer
  pCurrTex = pTexData;
  iGroupIdx = 0;
  iNumGroups = iNumTextures / 4;

  if (iNumGroups >= 0) {
    do {
      // Determine how many textures in this group (max 4)
      pGroupStart = pCurrTex;
      if (iRemainingTextures <= 3)
        iTexturesInGroup = iRemainingTextures;
      else
        iTexturesInGroup = 4;

      iTempRowOffset = 0;

      if (iTexturesInGroup > 0) {
        // Process each row of the 64x64 blocks
        for (iRowIdx = 0; iRowIdx < 64; ++iRowIdx) {
          iSourceRowOffset = iRowIdx << 6;      // iRowIndex * 64, 64 px per row
          pTempRow = &pTempBuf[iTempRowOffset];

          // Process each texture in group
          for (iTexIdx = 0; iTexIdx < iTexturesInGroup; ++iTexIdx) {
            iPixelGroupIdx = 0;
            pSourcePx = &pGroupStart[iSourceRowOffset];

            // Copy 64 px from this texture's row (16 groups of 4 pixels)
            do {
              pPx1 = pTempRow + 1;
              byPx0 = *pSourcePx;
              pSourcePx_1 = pSourcePx + 1;
              iPixelGroupIdx += 4;
              *(pPx1++ - 1) = byPx0;
              byPx2 = *pSourcePx_1++;
              *(pPx1++ - 1) = byPx2;
              byPx3 = *pSourcePx_1++;
              *(pPx1 - 1) = byPx3;
              pTempRow = pPx1 + 1;
              byPx4 = *pSourcePx_1;
              pSourcePx = pSourcePx_1 + 1;
              *(pTempRow - 1) = byPx4;
            } while (iPixelGroupIdx < 64);
            iSourceRowOffset += 4096;           // move to next texture (4096 bytes = 64x64 px)
          }
          iTempRowOffset += 256;                // move to next row in temp buffer (4 tex * 64 px)
        }

        // Copy reorganized data back to original location
        memcpy(pCurrTex, pTempBuf, 0x4000u);
      }

      pCurrTex += 0x4000;                       // move to next group of textures
      iRemainingTextures -= 4;                  // process remaining textures
      ++iGroupIdx;
    } while (iGroupIdx <= iNumGroups);
  }

  // Cleanup
  fre((void **)&pTempBuf);
}

//-------------------------------------------------------------------------------------------------

void sort_mini_texture(uint8 *pTexData, int iNumTextures)
{
  //ROLLER: we do not need interleaved texture data for this implementation
  return;

  int iTexturesInGroup; // ebp
  int iTexIdx; // esi
  int iSourceRowOffset; // edi
  uint8 *pTempRow; // ecx
  int iPixelGroupIdx; // edx
  uint8 *pSourcePx; // eax
  uint8 *pDestPixel; // ecx
  uint8 byPx1; // bl
  uint8 *iPixelGroupIdx_1; // eax
  char byPx2; // bl
  char byPx3; // bl
  char byPx4; // bl
  uint8 *pTempBuf; // [esp+0h] [ebp-34h] BYREF
  int iNumGroups; // [esp+4h] [ebp-30h]
  int iGroupIdx; // [esp+8h] [ebp-2Ch]
  uint8 *pCurrTex; // [esp+Ch] [ebp-28h]
  int iRemainingTextures; // [esp+10h] [ebp-24h]
  int iTempRowOffset; // [esp+14h] [ebp-20h]
  int iRowIdx; // [esp+18h] [ebp-1Ch]
  uint8 *pGroupStart; // [esp+1Ch] [ebp-18h]

  iRemainingTextures = iNumTextures;
  pTempBuf = (uint8 *)getbuffer(0x2000u);       // 8KB temp buffer
  pCurrTex = pTexData;
  iGroupIdx = 0;
  iNumGroups = iNumTextures / 8;

  if (iNumGroups >= 0) {
    do {
      pGroupStart = pCurrTex;
      if (iRemainingTextures <= 7)
        iTexturesInGroup = iRemainingTextures;
      else
        iTexturesInGroup = 8;

      iTempRowOffset = 0;

      if (iTexturesInGroup > 0) {
        // Process each row of 32x32 blocks
        for (iRowIdx = 0; iRowIdx < 32; ++iRowIdx) {
          iTexIdx = 0;
          iSourceRowOffset = 32 * iRowIdx;
          pTempRow = &pTempBuf[iTempRowOffset];

          // Process each tex in group
          do {
            iPixelGroupIdx = 0;
            pSourcePx = &pGroupStart[iSourceRowOffset];

            // Copy 32 px from this texture's row (8 groups of 4 px)
            do {
              pDestPixel = pTempRow + 1;

              // Copy 4px at a time
              byPx1 = *pSourcePx;
              iPixelGroupIdx_1 = pSourcePx + 1;
              iPixelGroupIdx += 4;
              *(pDestPixel++ - 1) = byPx1;

              byPx2 = *iPixelGroupIdx_1++;
              *(pDestPixel++ - 1) = byPx2;

              byPx3 = *iPixelGroupIdx_1++;
              *(pDestPixel - 1) = byPx3;

              pTempRow = pDestPixel + 1;
              byPx4 = *iPixelGroupIdx_1;
              pSourcePx = iPixelGroupIdx_1 + 1;
              *(pTempRow - 1) = byPx4;
            } while (iPixelGroupIdx < 32);

            ++iTexIdx;
            iSourceRowOffset += 1024;           // Move to next texture (1024 bytes = 32x32 px)
          } while (iTexIdx < iTexturesInGroup);

          // Move to next row in temp buffer (8 tex * 32px)
          iTempRowOffset += 256;
        }

        // Coyp reorganized data back to original location
        memcpy(pCurrTex, pTempBuf, 0x2000u);
      }

      pCurrTex += 0x2000;                       // move to next group of textures
      iRemainingTextures -= 8;                  // update remaining textures
      ++iGroupIdx;
    } while (iGroupIdx <= iNumGroups);
  }

  // Cleanup
  fre((void **)&pTempBuf);
}

//-------------------------------------------------------------------------------------------------

void drbox(int iX, int iY, int iWidth, int iHeight, uint8 byColor)
{
  uint8 *pRowStart; // edi
  int i; // esi
  uint8 *pPixel; // eax
  int iPixelCol; // edx

  // Calculate starting position in screen buffer
  pRowStart = &scrbuf[iX + winw * (199 - iY)];

  // Draw rectangle row by row
  for (i = 0; i < iHeight; ++i) {
    pPixel = pRowStart;
    pRowStart += winw;

    // Fill row with color
    for (iPixelCol = 0; iPixelCol < iWidth; *(pPixel - 1) = byColor) {
      ++pPixel;
      ++iPixelCol;
    }
  }
}

//-------------------------------------------------------------------------------------------------

void box(int iX, int iY, int iWidth, int iHeight, uint8 byBorderColor)
{
  int iWinW; // esi
  int iTopRowY; // ebp
  int iHorizontalPixels; // edi
  uint8 *pTopRow; // eax
  uint8 *pBottomRow; // edx
  int i; // ebx
  uint8 *pLeftEdge; // eax
  uint8 *pRightEdge; // edx
  int j; // ebx

  iWinW = winw;
  iTopRowY = 199 - iY;
  iHorizontalPixels = iWidth - 2;

  // Draw top and bottom lines excluding corner pixels
  pTopRow = &scrbuf[iX + winw * (199 - iY)];
  pBottomRow = &scrbuf[iX + winw * (iHeight + 199 - iY - 1)];
  for (i = 0; i < iHorizontalPixels; *pBottomRow = byBorderColor) {
    ++pBottomRow;
    *++pTopRow = byBorderColor;
    ++i;
  }

  // Draw left and right vertical lines (including corners)
  pLeftEdge = &scrbuf[iX + iWinW * iTopRowY];
  pRightEdge = &scrbuf[iWidth - 1 + iX + iWinW * iTopRowY];
  for (j = 0; j <= iHeight - 1; pRightEdge += iWinW) {
    ++j;
    *pLeftEdge = byBorderColor;
    pLeftEdge += iWinW;
    *pRightEdge = byBorderColor;
  }
  winw = iWinW;
}

//-------------------------------------------------------------------------------------------------

void SetVideoMode(uint8 byVideoMode)
{
  //union REGS regs; // [esp+0h] [ebp-24h] BYREF
  //regs.w.ax = byVideoMode;
  //int386(0x10, &regs, &regs);
}

//-------------------------------------------------------------------------------------------------
