#ifndef _DRIVERS_MAIN_H
#define _DRIVERS_MAIN_H

#include <mednafen/driver.h>
#include <mednafen/mednafen.h>
#include <mednafen/settings.h>
#include <mednafen/Time.h>
#include "config.h"
#include "args.h"
#include "menu.h"

#include <SDL.h>
#include <SDL/SDL_ttf.h>
#include <SDL_thread.h>

#include <mednafen/gettext.h>

#define CEVT_TOGGLEGUI	1
#define CEVT_TOGGLEFS	2
#define CEVT_VIDEOSYNC	5
#define CEVT_SHOWCURSOR		0x0c
#define CEVT_CHEATTOGGLEVIEW	0x10


#define CEVT_DISP_MESSAGE	0x11

#define CEVT_SET_GRAB_INPUT	0x20

#define CEVT_SET_STATE_STATUS	0x40
#define CEVT_SET_MOVIE_STATUS	0x41

#define CEVT_WANT_EXIT		0x80 // Emulator exit or GUI exit or bust!


#define CEVT_NP_DISPLAY_TEXT	0x101
#define CEVT_NP_TOGGLE_TT	0x103
#define CEVT_NP_LINE            0x180
#define CEVT_NP_LINE_RESPONSE   0x181

#define CEVT_SET_INPUT_FOCUS	0x1000	// Main thread to game thread.

void SendCEvent(unsigned int code, void *data1, void *data2);

void PauseGameLoop(bool p);

void SDL_MDFN_ShowCursor(int toggle);

extern int NoWaiting;
extern bool MDFNDHaveFocus;

extern MDFNGI *CurGame;
int CloseGame(void);

void RefreshThrottleFPS(double);
void PumpWrap(void);
void MainRequestExit(void);
bool MainExitPending(void);

extern bool pending_save_state, pending_ssnapshot, pending_snapshot, pending_save_movie;

void DoRunNormal(void);
void DoFrameAdvance(void);
bool IsInFrameAdvance(void);

void DebuggerFudge(void);

extern volatile int GameThreadRun;

void GT_ToggleFS(void);
bool GT_ReinitVideo(void);
bool GT_ReinitSound(void);


void BuildSystemSetting(MDFNSetting *setting, const char *system_name, const char *name, const char *description, const char *description_extra, MDFNSettingType type,
	const char *default_value, const char *minimum = NULL, const char *maximum = NULL,
	bool (*validate_func)(const char *name, const char *value) = NULL, void (*ChangeNotification)(const char *name) = NULL,
        const MDFNSetting_EnumList *enum_list = NULL);













#if 0

typedef enum{
    MENU_TYPE_VOLUME,
    MENU_TYPE_BRIGHTNESS,
    MENU_TYPE_SAVE,
    MENU_TYPE_LOAD,
    MENU_TYPE_ASPECT_RATIO,
    MENU_TYPE_EXIT,
    MENU_TYPE_POWERDOWN,
    NB_MENU_TYPES,
} ENUM_MENU_TYPE;


///------ Definition of the different aspect ratios
#define ASPECT_RATIOS \
    X(ASPECT_RATIOS_TYPE_MANUAL, "MANUAL ZOOM") \
    X(ASPECT_RATIOS_TYPE_STRECHED, "STRECHED") \
    X(ASPECT_RATIOS_TYPE_CROPPED, "CROPPED") \
    X(ASPECT_RATIOS_TYPE_SCALED, "SCALED") \
    X(NB_ASPECT_RATIOS_TYPES, "")

////------ Enumeration of the different aspect ratios ------
#undef X
#define X(a, b) a,
typedef enum {ASPECT_RATIOS} ENUM_ASPECT_RATIOS_TYPES;

////------ Defines to be shared -------
#define STEP_CHANGE_VOLUME          10
#define STEP_CHANGE_BRIGHTNESS      10

////------ Menu commands -------
#define SHELL_CMD_VOLUME_GET        "/root/shell_cmds/volume_get.sh"
#define SHELL_CMD_VOLUME_SET        "/root/shell_cmds/volume_set.sh"
#define SHELL_CMD_BRIGHTNESS_GET    "/root/shell_cmds/brightness_get.sh"
#define SHELL_CMD_BRIGHTNESS_SET    "/root/shell_cmds/brightness_set.sh"
#define SHELL_CMD_POWERDOWN         "shutdown -h now"

////------ Global variables -------
extern int volume_percentage;
extern int brightness_percentage;

extern const char *aspect_ratio_name[];
extern int aspect_ratio;
extern int aspect_ratio_factor_percent;
extern int aspect_ratio_factor_step;

////------ Functions -------
void init_menu_SDL();
void deinit_menu_SDL();
void init_menu_zones();
void init_menu_system_values();
void run_menu_loop();

#endif





#endif
