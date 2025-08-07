#ifndef _ROLLER_FUNC2_H
#define _ROLLER_FUNC2_H
//-------------------------------------------------------------------------------------------------
#include "types.h"
#include "func3.h"
//-------------------------------------------------------------------------------------------------

typedef enum
{
  USERKEY_P1LEFT = 0,
  USERKEY_P1RIGHT = 1,
  USERKEY_P1UP = 2,
  USERKEY_P1DOWN = 3,
  USERKEY_P1UPGEAR = 4,
  USERKEY_P1DOWNGEAR = 5,
  USERKEY_P1CHEAT = 6,
  USERKEY_P2LEFT = 7,
  USERKEY_P2RIGHT = 8,
  USERKEY_P2UP = 9,
  USERKEY_P2DOWN = 10,
  USERKEY_P2UPGEAR = 11,
  USERKEY_P2DOWNGEAR = 12,
  USERKEY_P2CHEAT = 13
} eUserKey;


//-------------------------------------------------------------------------------------------------

struct DPMI_MemInfo
{
  uint32 largestFreeBlock;
  uint32 maxUnlockedPages;
  uint32 maxLockedPages;
  uint32 totalLinearPages;
  uint32 totalUnlockedPages;
  uint32 totalFreePages;
  uint32 totalPhysicalPages;
  uint32 freeLinearPages;
};

//-------------------------------------------------------------------------------------------------

extern int write_key;
extern int read_key;
extern uint8 mapping[];
extern int twoparter;
extern int font6_offsets[88];
extern char font6_ascii[256];
extern char ascii_conv3[256];
extern char *keyname[140];
extern int userkey[14];
extern uint8 key_buffer[];
extern int new_zoom[2];
extern char config_buffer[8192];
extern char language_buffer[8192];
extern int no_mem;
extern uint8 *screen_pointer;
extern int lots_of_mem;

//-------------------------------------------------------------------------------------------------

int draw_smoke(int a1, int a2);
int test_panel(int a1, char *a2);
void ZoomString(char *a1, int a2, int a3, int a4, int a5);
void ZoomSub(char *a1, int a2, int a3, int a4, int a5);
int zoom_letter(int a1, uint8 a2, void *a3, void *a4, char *a5, float a6);
int print_block(char *a1, int a2, int a3);
int print_damage(uint8 *a1, int *a2, int a3);
void print_pos(int iX, int iY, int iDriverIdx);
void free_game_memory();
int readmode();
void key_handler(uint8 byScancode);
void claim_key_int();
int fatkbhit();
int fatgetch();
void release_key_int();
void clear_border(int x, int y, int iWidth, int iLines);
void DisplayFree();
void setdirectory(const char *szAppPath);
void FindShades();
int nearest_colour(int iR, int iB, int iG);
void select_view(int iPlayer);
void mini_prt_string(tBlockHeader *pBlockHeader, const char *szStr, int iX, int iY);
void mini_prt_string_rev(tBlockHeader *pBlockHeader, const char *szText, int iX, int iY);
void mini_prt_right(tBlockHeader *pBlockHeader, const char *szText, int iX, int iY);
void mini_prt_centre(tBlockHeader *pBlockHeader, const char *szStr, int iX, int iY);
void prt_right(tBlockHeader *pBlockHeader, const char *szStr, int iX, int iY);
void prt_string(tBlockHeader *pBlockHeader, const char *szStr, int iX, int iY);
void prt_letter(tBlockHeader *pBlockHeader, char byChar, int *piXPos, int *piYPos, int iFontType);
void prt_letter_rev(tBlockHeader *pBlockHeader, char byChar, int *piXPos, int *piYPos, int iFontType);
void prt_stringcol(tBlockHeader *pBlockHeader, const char *szStr, int iX, int iY, char byColor);
void prt_rightcol(tBlockHeader *pBlockHeader, const char *szStr, int iX, int iY, char byColor);
void prt_centrecol(tBlockHeader *pBlockHeader, const char *szStr, int iX, int iY, char byColor);
void prt_lettercol(tBlockHeader *pBlockHeader, char byChar, int *piXPos, int *piYPos, char byColor);
char display_paused();
void enable_keyboard();
void disable_keyboard();
void save_fatal_config();
void load_fatal_config();
void getconfigvalue(const char *szConfigText, const char *szVarName, int *piOutVal, int iMin, int iMax);
void getconfigvalueuc(const char *szConfigText, const char *szVarName, uint8 *pbyOutVal, int iMin, int iMax);
void displaycalibrationbar(int iX, int iY, int iValue);
void blankwindow(int iX1, int iY1, int iX2, int iY2);
void volumebar(int iX, int iVolume);
void remove_uncalibrated();
void LoadRecords();
void SaveRecords();
uint8 *copy_int(uint8 *pDest, uint32 uiValue);
void ShowATime(float fTime, int iX, int iY);
void setmodex();
void modexsethardware();
void modexclearscreen();
void copyscreenmodex(uint8 *pSrc, uint8 *pDest);
void start_zoom(const char *szStr, int iPlayerIdx);
char small_zoom(char *a1);
char subzoom(char *a1);
void make_time(char *szTimeStr, float fTime);
void check_machine_speed();
void load_language_file(char *szFilename, int iUseConfigBuffer);
void do_blip(int iCarIdx);

//-------------------------------------------------------------------------------------------------
#endif
