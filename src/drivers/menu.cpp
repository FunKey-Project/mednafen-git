/* Mednafen - Multi-system Emulator
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "main.h"
#include "video.h"
#include "menu.h"
#include "../general.h"

#ifdef WIN32
#include <windows.h>
#endif

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <trio/trio.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_image.h>


/*#ifdef HAVE_GETPWUID
#include <pwd.h>
#endif

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

#include <atomic>

#include "input.h"
#include "Joystick.h"
#include "video.h"
#include "opengl.h"
#include "shader.h"
#include "sound.h"
#include "netplay.h"
#include "cheat.h"
#include "fps.h"
#include "debugger.h"
#include "help.h"
#include "video-state.h"
#include "remote.h"
#include "ers.h"
#include "rmdui.h"
#include <mednafen/qtrecord.h>
#include <mednafen/tests.h>
#include <mednafen/MemoryStream.h>
#include <mednafen/file.h>*/



/// -------------- DEFINES --------------
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define ABS(x) (((x) < 0) ? (-x) : (x))

//#define MENU_DEBUG
#define MENU_ERROR

#ifdef MENU_DEBUG
#define MENU_DEBUG_PRINTF(...)   printf(__VA_ARGS__);
#else
#define MENU_DEBUG_PRINTF(...)
#endif //MENU_DEBUG

#ifdef MENU_ERROR
#define MENU_ERROR_PRINTF(...)   printf(__VA_ARGS__);
#else
#define MENU_ERROR_PRINTF(...)
#endif //MENU_ERROR

#define SCREEN_HORIZONTAL_SIZE      RES_HW_SCREEN_HORIZONTAL
#define SCREEN_VERTICAL_SIZE        RES_HW_SCREEN_VERTICAL

#define SCROLL_SPEED_PX             30
#define FPS_MENU                    50
#define ARROWS_PADDING              8

#define MENU_ZONE_WIDTH             SCREEN_HORIZONTAL_SIZE
#define MENU_ZONE_HEIGHT            SCREEN_VERTICAL_SIZE
#define MENU_BG_SQUARE_WIDTH        180
#define MENU_BG_SQUARE_HEIGHT       140

#define MENU_FONT_NAME_TITLE        "/usr/games/menu_resources/OpenSans-Bold.ttf"
#define MENU_FONT_SIZE_TITLE        22
#define MENU_FONT_NAME_INFO         "/usr/games/menu_resources/OpenSans-Bold.ttf"
#define MENU_FONT_SIZE_INFO         16
#define MENU_FONT_NAME_SMALL_INFO   "/usr/games/menu_resources/OpenSans-Regular.ttf"
#define MENU_FONT_SIZE_SMALL_INFO   13
#define MENU_PNG_BG_PATH            "/usr/games/menu_resources/zone_bg.png"
#define MENU_PNG_ARROW_TOP_PATH     "/usr/games/menu_resources/arrow_top.png"
#define MENU_PNG_ARROW_BOTTOM_PATH  "/usr/games/menu_resources/arrow_bottom.png"

#define GRAY_MAIN_R                 85
#define GRAY_MAIN_G                 85
#define GRAY_MAIN_B                 85
#define WHITE_MAIN_R                236
#define WHITE_MAIN_G                236
#define WHITE_MAIN_B                236

#define MAX_SAVE_SLOTS              9

#define MAXPATHLEN                  512

/// -------------- STATIC VARIABLES --------------
extern SDL_Surface *screen;
extern SDL_Surface *hw_screen;
SDL_Surface * draw_screen;
extern int volatile pumpWrap_disabled;
extern int volatile NeedExitNow;


static int backup_key_repeat_delay, backup_key_repeat_interval;
static SDL_Surface * backup_hw_screen = NULL;

static TTF_Font *menu_title_font = NULL;
static TTF_Font *menu_info_font = NULL;
static TTF_Font *menu_small_info_font = NULL;
static SDL_Surface *img_arrow_top = NULL;
static SDL_Surface *img_arrow_bottom = NULL;
static SDL_Surface ** menu_zone_surfaces = NULL;
static int * idx_menus = NULL;
static int nb_menu_zones = 0;
static int menuItem = 0;
int stop_menu_loop = 0;
static int menu_init_ok = 0;

static SDL_Color text_color = {GRAY_MAIN_R, GRAY_MAIN_G, GRAY_MAIN_B};
static int padding_y_from_center_menu_zone = 18;
static uint16_t width_progress_bar = 100;
static uint16_t height_progress_bar = 20;
static uint16_t x_volume_bar = 0;
static uint16_t y_volume_bar = 0;
static uint16_t x_brightness_bar = 0;
static uint16_t y_brightness_bar = 0;

int volume_percentage = 0;
int brightness_percentage = 0;

#undef X
#define X(a, b) b,
const char *aspect_ratio_name[] = {ASPECT_RATIOS};
int aspect_ratio = ASPECT_RATIOS_TYPE_STRETCHED;
int aspect_ratio_factor_percent = 50;
int aspect_ratio_factor_step = 10;
int need_screen_cleared = 0;

#undef X
#define X(a, b) b,
const char *resume_options_str[] = {RESUME_OPTIONS};

extern int SaveStateStatus[10];
extern int CurrentState;
static int quick_load_slot_chosen = 0;

/// -------------- FUNCTIONS DECLARATION --------------

/// -------------- FUNCTIONS IMPLEMENTATION --------------
void init_menu_SDL(){
    if(menu_init_ok){
      MENU_DEBUG_PRINTF("Menu already initiated\n");
      return;
    }

    MENU_DEBUG_PRINTF("Init Menu\n");

    /// ----- Loading the fonts -----
    menu_title_font = TTF_OpenFont(MENU_FONT_NAME_TITLE, MENU_FONT_SIZE_TITLE);
    if(!menu_title_font){
        MENU_ERROR_PRINTF("ERROR in init_menu_SDL: Could not open menu font %s, %s\n", MENU_FONT_NAME_TITLE, SDL_GetError());
    }
    menu_info_font = TTF_OpenFont(MENU_FONT_NAME_INFO, MENU_FONT_SIZE_INFO);
    if(!menu_info_font){
        MENU_ERROR_PRINTF("ERROR in init_menu_SDL: Could not open menu font %s, %s\n", MENU_FONT_NAME_INFO, SDL_GetError());
    }
    menu_small_info_font = TTF_OpenFont(MENU_FONT_NAME_SMALL_INFO, MENU_FONT_SIZE_SMALL_INFO);
    if(!menu_small_info_font){
        MENU_ERROR_PRINTF("ERROR in init_menu_SDL: Could not open menu font %s, %s\n", MENU_FONT_NAME_SMALL_INFO, SDL_GetError());
    }

    /// ----- Create SDL surfaces ------
    backup_hw_screen = SDL_CreateRGBSurface(SDL_SWSURFACE,
        hw_screen->w, hw_screen->h, 32, 0, 0, 0, 0);
    if(backup_hw_screen == NULL){
        MENU_ERROR_PRINTF("ERROR Could not create backup_hw_screen: %s\n", SDL_GetError());
    }
    draw_screen = SDL_CreateRGBSurface(SDL_SWSURFACE,
        hw_screen->w, hw_screen->h, 32, 0, 0, 0, 0);
    if(draw_screen == NULL){
        MENU_ERROR_PRINTF("ERROR Could not create draw_screen: %s\n", SDL_GetError());
    }

    /// ------ Load arrows imgs -------
    img_arrow_top = IMG_Load(MENU_PNG_ARROW_TOP_PATH);
    if(!img_arrow_top) {
        MENU_ERROR_PRINTF("ERROR IMG_Load: %s\n", IMG_GetError());
    }
    img_arrow_bottom = IMG_Load(MENU_PNG_ARROW_BOTTOM_PATH);
    if(!img_arrow_bottom) {
        MENU_ERROR_PRINTF("ERROR IMG_Load: %s\n", IMG_GetError());
    }

    /// ------ Init menu zones ------
    init_menu_zones();

    menu_init_ok = 1;
}

void deinit_menu_SDL(){
    /// ------ Close font -------
    TTF_CloseFont(menu_title_font);
    TTF_CloseFont(menu_info_font);
    TTF_CloseFont(menu_small_info_font);

    /// ------ Free Surfaces -------
    for(int i=0; i < nb_menu_zones; i++){
        SDL_FreeSurface(menu_zone_surfaces[i]);
    }
    if(backup_hw_screen != NULL){
        SDL_FreeSurface(backup_hw_screen);
    }
    if(draw_screen != NULL){
        SDL_FreeSurface(draw_screen);
    }

    SDL_FreeSurface(img_arrow_top);
    SDL_FreeSurface(img_arrow_bottom);

    /// ------ Free Menu memory and reset vars -----
    if(idx_menus){
        free(idx_menus);
    }
    idx_menus=NULL;
    nb_menu_zones = 0;
}


void draw_progress_bar(SDL_Surface * surface, uint16_t x, uint16_t y, uint16_t width,
                        uint16_t height, uint8_t percentage, uint16_t nb_bars){
    /// ------ Init Variables ------
    uint16_t line_width = 1; //px
    uint16_t padding_bars_ratio = 3;
    uint16_t nb_full_bars = 0;

    /// ------ Check values ------
    percentage = (percentage > 100)?100:percentage;
    x = (x > (surface->w-1))?(surface->w-1):x;
    y = (y > surface->h-1)?(surface->h-1):y;
    width = (width < line_width*2+1)?(line_width*2+1):width;
    width = (width > surface->w-x-1)?(surface->w-x-1):width;
    height = (height < line_width*2+1)?(line_width*2+1):height;
    height = (height > surface->h-y-1)?(surface->h-y-1):height;
    uint16_t nb_bars_max = ( width * padding_bars_ratio  /  (line_width*2+1) + 1 ) / (padding_bars_ratio+1);
    nb_bars = (nb_bars > nb_bars_max)?nb_bars_max:nb_bars;
    uint16_t bar_width = (width / nb_bars)*padding_bars_ratio/(padding_bars_ratio+1)+1;
    uint16_t bar_padding_x = bar_width/padding_bars_ratio;
    nb_full_bars = nb_bars*percentage/100;

    /// ------ draw full bars ------
    for (int i = 0; i < nb_full_bars; ++i)
    {
        /// ---- draw one bar ----
        //MENU_DEBUG_PRINTF("Drawing filled bar %d\n", i);
        SDL_Rect rect = {x+ i*(bar_width +bar_padding_x),
            y, bar_width, height};
        SDL_FillRect(surface, &rect, SDL_MapRGB(surface->format, GRAY_MAIN_R, GRAY_MAIN_G, GRAY_MAIN_B));
    }

    /// ------ draw full bars ------
    for (int i = 0; i < (nb_bars-nb_full_bars); ++i)
    {
        /// ---- draw one bar ----
        //MENU_DEBUG_PRINTF("Drawing empty bar %d\n", i);
        SDL_Rect rect = {x+ i*(bar_width +bar_padding_x) + nb_full_bars*(bar_width +bar_padding_x),
            y, bar_width, height};
        SDL_FillRect(surface, &rect, SDL_MapRGB(surface->format, GRAY_MAIN_R, GRAY_MAIN_G, GRAY_MAIN_B));

        SDL_Rect rect2 = {x+ i*(bar_width +bar_padding_x) + line_width + nb_full_bars*(bar_width +bar_padding_x),
            y + line_width, bar_width - line_width*2, height - line_width*2};
        SDL_FillRect(surface, &rect2, SDL_MapRGB(surface->format, WHITE_MAIN_R, WHITE_MAIN_R, WHITE_MAIN_R));
    }


}


void add_menu_zone(ENUM_MENU_TYPE menu_type){
    /// ------ Increase nb of menu zones -------
    nb_menu_zones++;

    /// ------ Realoc idx Menus array -------
    if(!idx_menus){
        idx_menus = (int*) malloc(nb_menu_zones*sizeof(int));
        menu_zone_surfaces = (SDL_Surface**) malloc(nb_menu_zones*sizeof(SDL_Surface*));
    }
    else{
        int *temp = (int*) realloc(idx_menus, nb_menu_zones*sizeof(int));
        idx_menus = temp;
        menu_zone_surfaces = (SDL_Surface**) realloc(menu_zone_surfaces, nb_menu_zones*sizeof(SDL_Surface*));
    }
    idx_menus[nb_menu_zones-1] = menu_type;

    /// ------ Reinit menu surface with height increased -------
    menu_zone_surfaces[nb_menu_zones-1] = IMG_Load(MENU_PNG_BG_PATH);
    if(!menu_zone_surfaces[nb_menu_zones-1]) {
        MENU_ERROR_PRINTF("ERROR IMG_Load: %s\n", IMG_GetError());
    }
    /// --------- Init Common Variables --------
    SDL_Surface *text_surface = NULL;
    SDL_Surface *surface = menu_zone_surfaces[nb_menu_zones-1];
    SDL_Rect text_pos;

    /// --------- Add new zone ---------
    switch(menu_type){
    case MENU_TYPE_VOLUME:
        MENU_DEBUG_PRINTF("Init MENU_TYPE_VOLUME\n");
        /// ------ Text ------
        text_surface = TTF_RenderText_Blended(menu_title_font, "VOLUME", text_color);
        text_pos.x = (surface->w - MENU_ZONE_WIDTH)/2 + (MENU_ZONE_WIDTH - text_surface->w)/2;
        text_pos.y = surface->h - MENU_ZONE_HEIGHT/2 - text_surface->h/2 - padding_y_from_center_menu_zone;
        SDL_BlitSurface(text_surface, NULL, surface, &text_pos);

        x_volume_bar = (surface->w - MENU_ZONE_WIDTH)/2 + (MENU_ZONE_WIDTH - width_progress_bar)/2;
        y_volume_bar = surface->h - MENU_ZONE_HEIGHT/2 - height_progress_bar/2 + padding_y_from_center_menu_zone;
        draw_progress_bar(surface, x_volume_bar, y_volume_bar,
            width_progress_bar, height_progress_bar, 0, 100/STEP_CHANGE_VOLUME);
        break;
    case MENU_TYPE_BRIGHTNESS:
        MENU_DEBUG_PRINTF("Init MENU_TYPE_BRIGHTNESS\n");
        /// ------ Text ------
        text_surface = TTF_RenderText_Blended(menu_title_font, "BRIGHTNESS", text_color);
        text_pos.x = (surface->w - MENU_ZONE_WIDTH)/2 + (MENU_ZONE_WIDTH - text_surface->w)/2;
        text_pos.y = surface->h - MENU_ZONE_HEIGHT/2 - text_surface->h/2 - padding_y_from_center_menu_zone;
        SDL_BlitSurface(text_surface, NULL, surface, &text_pos);

        x_brightness_bar = (surface->w - MENU_ZONE_WIDTH)/2 + (MENU_ZONE_WIDTH - width_progress_bar)/2;
        y_brightness_bar = surface->h - MENU_ZONE_HEIGHT/2 - height_progress_bar/2 + padding_y_from_center_menu_zone;
        draw_progress_bar(surface, x_brightness_bar, y_brightness_bar,
            width_progress_bar, height_progress_bar, 0, 100/STEP_CHANGE_BRIGHTNESS);
        break;
    case MENU_TYPE_SAVE:
        MENU_DEBUG_PRINTF("Init MENU_TYPE_SAVE\n");
        /// ------ Text ------
        text_surface = TTF_RenderText_Blended(menu_title_font, "SAVE", text_color);
        text_pos.x = (surface->w - MENU_ZONE_WIDTH)/2 + (MENU_ZONE_WIDTH - text_surface->w)/2;
        text_pos.y = surface->h - MENU_ZONE_HEIGHT/2 - text_surface->h/2 - padding_y_from_center_menu_zone*2;
        SDL_BlitSurface(text_surface, NULL, surface, &text_pos);
        break;
    case MENU_TYPE_LOAD:
        MENU_DEBUG_PRINTF("Init MENU_TYPE_LOAD\n");
        /// ------ Text ------
        text_surface = TTF_RenderText_Blended(menu_title_font, "LOAD", text_color);
        text_pos.x = (surface->w - MENU_ZONE_WIDTH)/2 + (MENU_ZONE_WIDTH - text_surface->w)/2;
        text_pos.y = surface->h - MENU_ZONE_HEIGHT/2 - text_surface->h/2 - padding_y_from_center_menu_zone*2;
        SDL_BlitSurface(text_surface, NULL, surface, &text_pos);
        break;
    case MENU_TYPE_ASPECT_RATIO:
        MENU_DEBUG_PRINTF("Init MENU_TYPE_ASPECT_RATIO\n");
        /// ------ Text ------
        text_surface = TTF_RenderText_Blended(menu_title_font, "ASPECT RATIO", text_color);
        text_pos.x = (surface->w - MENU_ZONE_WIDTH)/2 + (MENU_ZONE_WIDTH - text_surface->w)/2;
        text_pos.y = surface->h - MENU_ZONE_HEIGHT/2 - text_surface->h/2 - padding_y_from_center_menu_zone;
        SDL_BlitSurface(text_surface, NULL, surface, &text_pos);
        break;
    case MENU_TYPE_EXIT:
        MENU_DEBUG_PRINTF("Init MENU_TYPE_EXIT\n");
        /// ------ Text ------
        text_surface = TTF_RenderText_Blended(menu_title_font, "EXIT GAME", text_color);
        text_pos.x = (surface->w - MENU_ZONE_WIDTH)/2 + (MENU_ZONE_WIDTH - text_surface->w)/2;
        text_pos.y = surface->h - MENU_ZONE_HEIGHT/2 - text_surface->h/2;
        SDL_BlitSurface(text_surface, NULL, surface, &text_pos);
        break;
    case MENU_TYPE_POWERDOWN:
        MENU_DEBUG_PRINTF("Init MENU_TYPE_POWERDOWN\n");
        /// ------ Text ------
        text_surface = TTF_RenderText_Blended(menu_title_font, "POWERDOWN", text_color);
        text_pos.x = (surface->w - MENU_ZONE_WIDTH)/2 + (MENU_ZONE_WIDTH - text_surface->w)/2;
        text_pos.y = surface->h - MENU_ZONE_HEIGHT/2 - text_surface->h/2;
        SDL_BlitSurface(text_surface, NULL, surface, &text_pos);
        break;
    default:
        MENU_DEBUG_PRINTF("Warning - In add_menu_zone, unknown MENU_TYPE: %d\n", menu_type);
        break;
    }

    /// ------ Free Surfaces -------
    SDL_FreeSurface(text_surface);
}

void init_menu_zones(){
    /// Init Volume Menu
    add_menu_zone(MENU_TYPE_VOLUME);
    /// Init Brightness Menu
    add_menu_zone(MENU_TYPE_BRIGHTNESS);
    /// Init Save Menu
    add_menu_zone(MENU_TYPE_SAVE);
    /// Init Load Menu
    add_menu_zone(MENU_TYPE_LOAD);
    /// Init Aspect Ratio Menu
    add_menu_zone(MENU_TYPE_ASPECT_RATIO);
    /// Init Exit Menu
    add_menu_zone(MENU_TYPE_EXIT);
    /// Init Powerdown Menu
    //add_menu_zone(MENU_TYPE_POWERDOWN);
}


void init_menu_system_values(){
    FILE *fp;
    char res[100];

    /// ------- Get system volume percentage --------
    fp = popen(SHELL_CMD_VOLUME_GET, "r");
    if (fp == NULL) {
        MENU_ERROR_PRINTF("Failed to run command %s\n", SHELL_CMD_VOLUME_GET );
        volume_percentage = 50; ///wrong value: setting default to 50
    }
    else{
        fgets(res, sizeof(res)-1, fp);

        /// Check if Volume is a number (at least the first char)
        if(res[0] < '0' || res[0] > '9'){
            MENU_ERROR_PRINTF("Wrong return value: %s for volume cmd: %s\n",res, SHELL_CMD_VOLUME_GET);
            volume_percentage = 50; ///wrong value: setting default to 50
        }
        else{
            volume_percentage = atoi(res);
            MENU_DEBUG_PRINTF("System volume = %d%%\n", volume_percentage);
        }
    }

    /// ------- Get system brightness percentage -------
    fp = popen(SHELL_CMD_BRIGHTNESS_GET, "r");
    if (fp == NULL) {
        MENU_ERROR_PRINTF("Failed to run command %s\n", SHELL_CMD_BRIGHTNESS_GET );
        brightness_percentage = 50; ///wrong value: setting default to 50
    }
    else{
        fgets(res, sizeof(res)-1, fp);

        /// Check if brightness is a number (at least the first char)
        if(res[0] < '0' || res[0] > '9'){
            MENU_ERROR_PRINTF("Wrong return value: %s for volume cmd: %s\n",res, SHELL_CMD_BRIGHTNESS_GET);
            brightness_percentage = 50; ///wrong value: setting default to 50
        }
        else{
            brightness_percentage = atoi(res);
            MENU_DEBUG_PRINTF("System brightness = %d%%\n", brightness_percentage);
        }
    }

    /// ------ Save prev key repeat params and set new Key repeat -------
    SDL_GetKeyRepeat(&backup_key_repeat_delay, &backup_key_repeat_interval);
    if(SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL)){
        MENU_ERROR_PRINTF("ERROR with SDL_EnableKeyRepeat: %s\n", SDL_GetError());
    }

    /// Get save slot from game
    CurrentState = (CurrentState%MAX_SAVE_SLOTS); // security
}

void menu_screen_refresh(int menuItem, int prevItem, int scroll, uint8_t menu_confirmation, uint8_t menu_action){
    /// --------- Vars ---------
    int print_arrows = (scroll==0)?1:0;

    /// --------- Clear HW screen ----------
    if(SDL_BlitSurface(backup_hw_screen, NULL, draw_screen, NULL)){
        MENU_ERROR_PRINTF("ERROR Could not Clear draw_screen: %s\n", SDL_GetError());
    }

    /// --------- Setup Blit Window ----------
    SDL_Rect menu_blit_window;
    menu_blit_window.x = 0;
    menu_blit_window.w = SCREEN_HORIZONTAL_SIZE;

    /// --------- Blit prev menu Zone going away ----------
    menu_blit_window.y = scroll;
    menu_blit_window.h = SCREEN_VERTICAL_SIZE;
    if(SDL_BlitSurface(menu_zone_surfaces[prevItem], &menu_blit_window, draw_screen, NULL)){
        MENU_ERROR_PRINTF("ERROR Could not Blit surface on draw_screen: %s\n", SDL_GetError());
    }

    /// --------- Blit new menu Zone going in (only during animations) ----------
    if(scroll>0){
        menu_blit_window.y = SCREEN_VERTICAL_SIZE-scroll;
        menu_blit_window.h = SCREEN_VERTICAL_SIZE;
        if(SDL_BlitSurface(menu_zone_surfaces[menuItem], NULL, draw_screen, &menu_blit_window)){
            MENU_ERROR_PRINTF("ERROR Could not Blit surface on draw_screen: %s\n", SDL_GetError());
        }
    }
    else if(scroll<0){
        menu_blit_window.y = SCREEN_VERTICAL_SIZE+scroll;
        menu_blit_window.h = SCREEN_VERTICAL_SIZE;
        if(SDL_BlitSurface(menu_zone_surfaces[menuItem], &menu_blit_window, draw_screen, NULL)){
            MENU_ERROR_PRINTF("ERROR Could not Blit surface on draw_screen: %s\n", SDL_GetError());
        }
    }
    /// --------- No Scroll ? Blitting menu-specific info
    else{
        SDL_Surface * text_surface = NULL;
        char text_tmp[40];
        SDL_Rect text_pos;
        char fname[MAXPATHLEN];
        uint16_t limit_filename_size = 14;
        memset(fname, 0, MAXPATHLEN);

        switch(idx_menus[menuItem]){
        case MENU_TYPE_VOLUME:
            draw_progress_bar(draw_screen, x_volume_bar, y_volume_bar,
                            width_progress_bar, height_progress_bar, volume_percentage, 100/STEP_CHANGE_VOLUME);
            break;

        case MENU_TYPE_BRIGHTNESS:
            draw_progress_bar(draw_screen, x_volume_bar, y_volume_bar,
                            width_progress_bar, height_progress_bar, brightness_percentage, 100/STEP_CHANGE_BRIGHTNESS);
            break;

        case MENU_TYPE_SAVE:
            /// ---- Write slot -----
            sprintf(text_tmp, "IN SLOT   < %d >", CurrentState+1);
            text_surface = TTF_RenderText_Blended(menu_info_font, text_tmp, text_color);
            text_pos.x = (draw_screen->w - MENU_ZONE_WIDTH)/2 + (MENU_ZONE_WIDTH - text_surface->w)/2;
            text_pos.y = draw_screen->h - MENU_ZONE_HEIGHT/2 - text_surface->h/2;
            SDL_BlitSurface(text_surface, NULL, draw_screen, &text_pos);

            if(menu_action){
                sprintf(text_tmp, "Saving...");
                text_surface = TTF_RenderText_Blended(menu_info_font, text_tmp, text_color);
            }
            else{
                if(menu_confirmation){
                    sprintf(text_tmp, "Are you sure ?");
                    text_surface = TTF_RenderText_Blended(menu_info_font, text_tmp, text_color);
                }
                else{
                    /// ---- Write current Save state ----
                    if(SaveStateStatus[CurrentState])
                    {
                        strcpy(fname, MDFN_MakeFName(MDFNMKF_STATE, CurrentState, NULL).c_str());
                        printf("Found Save slot: %s\n", fname);
                        char *p = strrchr (fname, '/');
                        char *basename = p ? p + 1 : (char *) fname;
                        p = strchr (basename, '.'); *p = p ? 0 : *p;
                        if(strlen(basename) > limit_filename_size){basename[limit_filename_size]=0;} //limiting size
                        sprintf(text_tmp, "%s - %d", basename, CurrentState);
                        text_surface = TTF_RenderText_Blended(menu_small_info_font, text_tmp, text_color);
                    }
                    else{
                        text_surface = TTF_RenderText_Blended(menu_info_font, "Free", text_color);
                    }
                }
            }
            text_pos.x = (draw_screen->w - MENU_ZONE_WIDTH)/2 + (MENU_ZONE_WIDTH - text_surface->w)/2;
            text_pos.y = draw_screen->h - MENU_ZONE_HEIGHT/2 - text_surface->h/2 + 2*padding_y_from_center_menu_zone;
            SDL_BlitSurface(text_surface, NULL, draw_screen, &text_pos);
            break;

        case MENU_TYPE_LOAD:
            /// ---- Write slot -----
            if(quick_load_slot_chosen){
                sprintf(text_tmp, "FROM AUTO SAVE");
            }
            else{
                sprintf(text_tmp, "FROM SLOT   < %d >", CurrentState+1);
            }
            text_surface = TTF_RenderText_Blended(menu_info_font, text_tmp, text_color);
            text_pos.x = (draw_screen->w - MENU_ZONE_WIDTH)/2 + (MENU_ZONE_WIDTH - text_surface->w)/2;
            text_pos.y = draw_screen->h - MENU_ZONE_HEIGHT/2 - text_surface->h/2;
            SDL_BlitSurface(text_surface, NULL, draw_screen, &text_pos);

            if(menu_action){
                sprintf(text_tmp, "Loading...");
                text_surface = TTF_RenderText_Blended(menu_info_font, text_tmp, text_color);
            }
            else{
                if(menu_confirmation){
                    sprintf(text_tmp, "Are you sure ?");
                    text_surface = TTF_RenderText_Blended(menu_info_font, text_tmp, text_color);
                }
                else{
                    if(quick_load_slot_chosen){
                        text_surface = TTF_RenderText_Blended(menu_info_font, " ", text_color);
                    }
                    else{
                        /// ---- Get current Load state ----
                        if(SaveStateStatus[CurrentState])
                        {
                            strcpy(fname, MDFN_MakeFName(MDFNMKF_STATE, CurrentState, NULL).c_str());
                            printf("Found Load slot: %s\n", fname);
                            char *p = strrchr (fname, '/');
                            char *basename = p ? p + 1 : (char *) fname;
                            p = strchr (basename, '.'); *p = p ? 0 : *p;
                            if(strlen(basename) > limit_filename_size){basename[limit_filename_size]=0;} //limiting size
                            sprintf(text_tmp, "%s - %d", basename, CurrentState);
                            text_surface = TTF_RenderText_Blended(menu_small_info_font, text_tmp, text_color);
                        }
                        else{
                            text_surface = TTF_RenderText_Blended(menu_info_font, "Free", text_color);
                        }
                    }
                }
            }
            text_pos.x = (draw_screen->w - MENU_ZONE_WIDTH)/2 + (MENU_ZONE_WIDTH - text_surface->w)/2;
            text_pos.y = draw_screen->h - MENU_ZONE_HEIGHT/2 - text_surface->h/2 + 2*padding_y_from_center_menu_zone;
            SDL_BlitSurface(text_surface, NULL, draw_screen, &text_pos);
            break;

        case MENU_TYPE_ASPECT_RATIO:
            sprintf(text_tmp, "<   %s   >", aspect_ratio_name[aspect_ratio]);
            text_surface = TTF_RenderText_Blended(menu_info_font, text_tmp, text_color);
            text_pos.x = (draw_screen->w - MENU_ZONE_WIDTH)/2 + (MENU_ZONE_WIDTH - text_surface->w)/2;
            text_pos.y = draw_screen->h - MENU_ZONE_HEIGHT/2 - text_surface->h/2 + padding_y_from_center_menu_zone;
            SDL_BlitSurface(text_surface, NULL, draw_screen, &text_pos);
            break;

        case MENU_TYPE_EXIT:
        case MENU_TYPE_POWERDOWN:
            if(menu_confirmation){
                sprintf(text_tmp, "Are you sure ?");
                text_surface = TTF_RenderText_Blended(menu_info_font, text_tmp, text_color);
                text_pos.x = (draw_screen->w - MENU_ZONE_WIDTH)/2 + (MENU_ZONE_WIDTH - text_surface->w)/2;
                text_pos.y = draw_screen->h - MENU_ZONE_HEIGHT/2 - text_surface->h/2 + 2*padding_y_from_center_menu_zone;
                SDL_BlitSurface(text_surface, NULL, draw_screen, &text_pos);
            }
            break;
        default:
            break;
        }

        /// ------ Free Surfaces -------
        if(text_surface)
             SDL_FreeSurface(text_surface);
    }

    /// --------- Print arrows --------
    if(print_arrows){
        /// Top arrow
        SDL_Rect pos_arrow_top;
        pos_arrow_top.x = (draw_screen->w - img_arrow_top->w)/2;
        pos_arrow_top.y = (draw_screen->h - MENU_BG_SQUARE_HEIGHT)/4 - img_arrow_top->h/2;
        SDL_BlitSurface(img_arrow_top, NULL, draw_screen, &pos_arrow_top);

        /// Bottom arrow
        SDL_Rect pos_arrow_bottom;
        pos_arrow_bottom.x = (draw_screen->w - img_arrow_bottom->w)/2;
        pos_arrow_bottom.y = draw_screen->h -
            (draw_screen->h - MENU_BG_SQUARE_HEIGHT)/4 - img_arrow_bottom->h/2;
        SDL_BlitSurface(img_arrow_bottom, NULL, draw_screen, &pos_arrow_bottom);
    }

    /// --------- Screen Rotate --------
    //SDL_Rotate_270(draw_screen, hw_screen);
    SDL_BlitSurface(draw_screen, NULL, hw_screen, NULL);

    /// --------- Flip Screen ----------
    SDL_Flip(hw_screen);
}


void run_menu_loop()
{
    MENU_DEBUG_PRINTF("Launch Menu\n");

    SDL_Event event;
    uint32_t prev_ms = SDL_GetTicks();
    uint32_t cur_ms = SDL_GetTicks();
    int scroll=0;
    int start_scroll=0;
    uint8_t screen_refresh = 1;
    char shell_cmd[100];
    FILE *fp;
    uint8_t menu_confirmation = 0;
    stop_menu_loop = 0;
    char fname[MAXPATHLEN];
    pumpWrap_disabled = 1;

    /// ------ Get init values -------
    init_menu_system_values();
    int prevItem=menuItem;

    /// ------ Copy currently displayed screen -------
    if(SDL_BlitSurface(hw_screen, NULL, backup_hw_screen, NULL)){
        MENU_ERROR_PRINTF("ERROR Could not copy hw_screen: %s\n", SDL_GetError());
    }
    /*uint16_t *dst_virtual = (uint16_t*) sal_VirtualVideoGetBuffer();
    memcpy(backup_hw_screen->pixels, dst_virtual,
      RES_HW_SCREEN_HORIZONTAL * RES_HW_SCREEN_VERTICAL * sizeof(uint16_t));*/

    /* Stop Ampli */
    popen(SHELL_CMD_TURN_AMPLI_OFF, "r");

    /// -------- Main loop ---------
    while (!stop_menu_loop)
    {
        /// -------- Handle Keyboard Events ---------
        if(!scroll){
            while (SDL_PollEvent(&event))
            switch(event.type)
            {
                case SDL_QUIT:
                    NeedExitNow = 1;
                    stop_menu_loop = 1;
                    break;
                case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_b:
                        if(menu_confirmation){
                            /// ------ Reset menu confirmation ------
                            menu_confirmation = 0;
                            /// ------ Refresh screen ------
                            screen_refresh = 1;
                        }
                        /*else{
                            stop_menu_loop = 1;
                        }*/
                        break;

                    case SDLK_q:
                    case SDLK_ESCAPE:
                        stop_menu_loop = 1;
                        break;

                    case SDLK_d:
                    case SDLK_DOWN:
                        MENU_DEBUG_PRINTF("DOWN\n");
                        /// ------ Start scrolling to new menu -------
                        menuItem++;
                        if (menuItem>=nb_menu_zones) menuItem=0;
                        start_scroll=1;

                        /// ------ Reset menu confirmation ------
                        menu_confirmation = 0;

                        /// ------ Refresh screen ------
                        screen_refresh = 1;
                        break;

                    case SDLK_u:
                    case SDLK_UP:
                        MENU_DEBUG_PRINTF("UP\n");
                        /// ------ Start scrolling to new menu -------
                        menuItem--;
                        if (menuItem<0) menuItem=nb_menu_zones-1;
                        start_scroll=-1;

                        /// ------ Reset menu confirmation ------
                        menu_confirmation = 0;

                        /// ------ Refresh screen ------
                        screen_refresh = 1;
                        break;

                    case SDLK_l:
                    case SDLK_LEFT:
                        //MENU_DEBUG_PRINTF("LEFT\n");
                        if(idx_menus[menuItem] == MENU_TYPE_VOLUME){
                            MENU_DEBUG_PRINTF("Volume DOWN\n");
                            /// ----- Compute new value -----
                            volume_percentage = (volume_percentage < STEP_CHANGE_VOLUME)?
                                                    0:(volume_percentage-STEP_CHANGE_VOLUME);

                            /// ----- Shell cmd ----
                            sprintf(shell_cmd, "%s %d", SHELL_CMD_VOLUME_SET, volume_percentage);
                            fp = popen(shell_cmd, "r");
                            if (fp == NULL) {
                                MENU_ERROR_PRINTF("Failed to run command %s\n", shell_cmd);
                            }

                            /// ------ Refresh screen ------
                            screen_refresh = 1;
                        }
                        else if(idx_menus[menuItem] == MENU_TYPE_BRIGHTNESS){
                            MENU_DEBUG_PRINTF("Brightness DOWN\n");
                            /// ----- Compute new value -----
                            brightness_percentage = (brightness_percentage < STEP_CHANGE_BRIGHTNESS)?
                                                    0:(brightness_percentage-STEP_CHANGE_BRIGHTNESS);

                            /// ----- Shell cmd ----
                            sprintf(shell_cmd, "%s %d", SHELL_CMD_BRIGHTNESS_SET, brightness_percentage);
                            fp = popen(shell_cmd, "r");
                            if (fp == NULL) {
                                MENU_ERROR_PRINTF("Failed to run command %s\n", shell_cmd);
                            }
                        /// ------ Refresh screen ------
                            screen_refresh = 1;
                        }
                        else if(idx_menus[menuItem] == MENU_TYPE_SAVE){
                            MENU_DEBUG_PRINTF("Save Slot DOWN\n");
                            CurrentState = (!CurrentState)?(MAX_SAVE_SLOTS-1):(CurrentState-1);
                            MDFNI_SelectState(CurrentState);

                            /// ------ Refresh screen ------
                            screen_refresh = 1;
                        }
                        else if(idx_menus[menuItem] == MENU_TYPE_LOAD){
                            MENU_DEBUG_PRINTF("Load Slot DOWN\n");

                            /** Choose quick save file or standard saveslot for loading */
                            if(!quick_load_slot_chosen &&
                                CurrentState == 0 &&
                                access(quick_save_file, F_OK ) != -1){
                                quick_load_slot_chosen = 1;
                            }
                            else if(quick_load_slot_chosen){
                                quick_load_slot_chosen = 0;
                                CurrentState = MAX_SAVE_SLOTS-1;
                                MDFNI_SelectState(CurrentState);
                            }
                            else{
                                CurrentState = (!CurrentState)?(MAX_SAVE_SLOTS-1):(CurrentState-1);
                                MDFNI_SelectState(CurrentState);
                            }

                            /// ------ Refresh screen ------
                            screen_refresh = 1;
                        }
                        else if(idx_menus[menuItem] == MENU_TYPE_ASPECT_RATIO){
                            MENU_DEBUG_PRINTF("Aspect Ratio DOWN\n");
                            aspect_ratio = (!aspect_ratio)?(NB_ASPECT_RATIOS_TYPES-1):(aspect_ratio-1);
                            /// ------ Refresh screen ------
                            screen_refresh = 1;
                        }
                        break;

                    case SDLK_r:
                    case SDLK_RIGHT:
                        //MENU_DEBUG_PRINTF("RIGHT\n");
                        if(idx_menus[menuItem] == MENU_TYPE_VOLUME){
                            MENU_DEBUG_PRINTF("Volume UP\n");
                            /// ----- Compute new value -----
                            volume_percentage = (volume_percentage > 100 - STEP_CHANGE_VOLUME)?
                                                    100:(volume_percentage+STEP_CHANGE_VOLUME);

                            /// ----- Shell cmd ----
                            sprintf(shell_cmd, "%s %d", SHELL_CMD_VOLUME_SET, volume_percentage);
                            fp = popen(shell_cmd, "r");
                            if (fp == NULL) {
                                MENU_ERROR_PRINTF("Failed to run command %s\n", shell_cmd);
                            }
                            /// ------ Refresh screen ------
                            screen_refresh = 1;
                        }
                        else if(idx_menus[menuItem] == MENU_TYPE_BRIGHTNESS){
                            MENU_DEBUG_PRINTF("Brightness UP\n");
                            /// ----- Compute new value -----
                            brightness_percentage = (brightness_percentage > 100 - STEP_CHANGE_BRIGHTNESS)?
                                                    100:(brightness_percentage+STEP_CHANGE_BRIGHTNESS);

                            /// ----- Shell cmd ----
                            sprintf(shell_cmd, "%s %d", SHELL_CMD_BRIGHTNESS_SET, brightness_percentage);
                            fp = popen(shell_cmd, "r");
                            if (fp == NULL) {
                                MENU_ERROR_PRINTF("Failed to run command %s\n", shell_cmd);
                            }
                            /// ------ Refresh screen ------
                            screen_refresh = 1;
                        }
                        else if(idx_menus[menuItem] == MENU_TYPE_SAVE){
                            MENU_DEBUG_PRINTF("Save Slot UP\n");
                            CurrentState = (CurrentState+1)%MAX_SAVE_SLOTS;
                            /// ------ Refresh screen ------
                            screen_refresh = 1;
                        }
                        else if(idx_menus[menuItem] == MENU_TYPE_LOAD){
                            MENU_DEBUG_PRINTF("Load Slot UP\n");

                            /** Choose quick save file or standard saveslot for loading */
                            if(!quick_load_slot_chosen &&
                                CurrentState == MAX_SAVE_SLOTS-1 &&
                                access(quick_save_file, F_OK ) != -1){
                                quick_load_slot_chosen = 1;
                            }
                            else if(quick_load_slot_chosen){
                                quick_load_slot_chosen = 0;
                                CurrentState = 0;
                                MDFNI_SelectState(CurrentState);
                            }
                            else{
                                CurrentState = (CurrentState+1)%MAX_SAVE_SLOTS;
                                MDFNI_SelectState(CurrentState);
                            }

                            /// ------ Refresh screen ------
                            screen_refresh = 1;
                        }
                        else if(idx_menus[menuItem] == MENU_TYPE_ASPECT_RATIO){
                            MENU_DEBUG_PRINTF("Aspect Ratio UP\n");
                            aspect_ratio = (aspect_ratio+1)%NB_ASPECT_RATIOS_TYPES;
                            /// ------ Refresh screen ------
                            screen_refresh = 1;
                        }
                        break;

                    case SDLK_a:
                    case SDLK_RETURN:
                        if(idx_menus[menuItem] == MENU_TYPE_SAVE){
                            if(menu_confirmation){
                                MENU_DEBUG_PRINTF("Saving in slot %d\n", CurrentState);
                                /// ------ Refresh Screen -------
                                menu_screen_refresh(menuItem, prevItem, scroll, menu_confirmation, 1);

                                /// ------ Save game ------
				MDFNI_SaveState(NULL, NULL, NULL, NULL, NULL);

                                /// ----- Hud Msg -----
                                sprintf(shell_cmd, "%s %d \"        SAVED IN SLOT %d\"",
                                    SHELL_CMD_NOTIF, NOTIF_SECONDS_DISP, CurrentState+1);
                                fp = popen(shell_cmd, "r");
                                if (fp == NULL) {
                                    MENU_ERROR_PRINTF("Failed to run command %s\n", shell_cmd);
                                }

                                stop_menu_loop = 1;
                            }
                            else{
                                MENU_DEBUG_PRINTF("Save game - asking confirmation\n");
                                menu_confirmation = 1;
                                /// ------ Refresh screen ------
                                screen_refresh = 1;
                            }
                        }
                        else if(idx_menus[menuItem] == MENU_TYPE_LOAD){
                            if(menu_confirmation){
                                MENU_DEBUG_PRINTF("Loading in slot %d\n", CurrentState);
                                /// ------ Refresh Screen -------
                                menu_screen_refresh(menuItem, prevItem, scroll, menu_confirmation, 1);

                                /// ------ Load game ------
                                if(quick_load_slot_chosen){
                                    MDFNI_LoadState(quick_save_file, NULL);
                                }
                                else{
                                    MDFNI_LoadState(NULL, NULL);
                                }

                                /// ----- Hud Msg -----
                                if(quick_load_slot_chosen){
                                    sprintf(shell_cmd, "%s %d \"     LOADED FROM AUTO SAVE\"",
                                        SHELL_CMD_NOTIF, NOTIF_SECONDS_DISP);
                                }
                                else{
                                    sprintf(shell_cmd, "%s %d \"      LOADED FROM SLOT %d\"",
                                        SHELL_CMD_NOTIF, NOTIF_SECONDS_DISP, CurrentState+1);
                                }
                                fp = popen(shell_cmd, "r");
                                if (fp == NULL) {
                                    MENU_ERROR_PRINTF("Failed to run command %s\n", shell_cmd);
                                }

                                stop_menu_loop = 1;
                            }
                            else{
                                MENU_DEBUG_PRINTF("Save game - asking confirmation\n");
                                menu_confirmation = 1;
                                /// ------ Refresh screen ------
                                screen_refresh = 1;
                            }
                        }
                        else if(idx_menus[menuItem] == MENU_TYPE_EXIT){
                            MENU_DEBUG_PRINTF("Exit game\n");
                            if(menu_confirmation){
                                MENU_DEBUG_PRINTF("Exit game - confirmed\n");

                                /// ----- The game should be saved here ----
                                MDFNI_SaveState(quick_save_file, NULL, NULL, NULL, NULL);

                                /// ----- Exit game and back to launcher ----
                                NeedExitNow = 1;
                                stop_menu_loop = 1;
                            }
                            else{
                                MENU_DEBUG_PRINTF("Exit game - asking confirmation\n");
                                menu_confirmation = 1;
                                /// ------ Refresh screen ------
                                screen_refresh = 1;
                            }
                        }
                        else if(idx_menus[menuItem] == MENU_TYPE_POWERDOWN){
                            if(menu_confirmation){
                                MENU_DEBUG_PRINTF("Powerdown - confirmed\n");
                                /// ----- Shell cmd ----
                                sprintf(shell_cmd, "%s", SHELL_CMD_POWERDOWN);
                                fp = popen(shell_cmd, "r");
                                if (fp == NULL) {
                                    MENU_ERROR_PRINTF("Failed to run command %s\n", shell_cmd);
                                }
                            }
                            else{
                                MENU_DEBUG_PRINTF("Powerdown - asking confirmation\n");
                                menu_confirmation = 1;
                                /// ------ Refresh screen ------
                                screen_refresh = 1;
                            }
                        }
                        break;

                    default:
                        //MENU_DEBUG_PRINTF("Keydown: %d\n", event.key.keysym.sym);
                        break;
                }
                break;
            }
        }

        /// --------- Handle Scroll effect ---------
        if ((scroll>0) || (start_scroll>0)){
            scroll+=MIN(SCROLL_SPEED_PX, MENU_ZONE_HEIGHT-scroll);
            start_scroll = 0;
            screen_refresh = 1;
        }
        else if ((scroll<0) || (start_scroll<0)){
            scroll-=MIN(SCROLL_SPEED_PX, MENU_ZONE_HEIGHT+scroll);
            start_scroll = 0;
            screen_refresh = 1;
        }
        if (scroll>=MENU_ZONE_HEIGHT || scroll<=-MENU_ZONE_HEIGHT) {
            prevItem=menuItem;
            scroll=0;
            screen_refresh = 1;
        }

        /// --------- Handle FPS ---------
        cur_ms = SDL_GetTicks();
        if(cur_ms-prev_ms < 1000/FPS_MENU){
            SDL_Delay(1000/FPS_MENU - (cur_ms-prev_ms));
        }
        prev_ms = SDL_GetTicks();


        /// --------- Refresh screen
        if(screen_refresh){
            menu_screen_refresh(menuItem, prevItem, scroll, menu_confirmation, 0);
        }

        /// --------- reset screen refresh ---------
        screen_refresh = 0;

        /// --------- re-enable pumpwrap if exiting menu -------
        if(stop_menu_loop){
          pumpWrap_disabled = 0;
        }
    }

    /// ------ Reset prev key repeat params -------
    if(SDL_EnableKeyRepeat(backup_key_repeat_delay, backup_key_repeat_interval)){
         MENU_ERROR_PRINTF("ERROR with SDL_EnableKeyRepeat: %s\n", SDL_GetError());
    }

    /* Start Ampli */
    popen(SHELL_CMD_TURN_AMPLI_ON, "r");
}



/****************************/
/*    Quick Resume Menu     */
/****************************/
int launch_resume_menu_loop()
{
    MENU_DEBUG_PRINTF("Init resume menu\n");

    /* Decare vars */
    SDL_Surface *text_surface = NULL;
    char text_tmp[40];
    SDL_Rect text_pos;
    SDL_Event event;
    uint32_t prev_ms = SDL_GetTicks();
    uint32_t cur_ms = SDL_GetTicks();
    stop_menu_loop = 0;
    uint8_t screen_refresh = 1;
    uint8_t menu_confirmation = 0;
    int option_idx=RESUME_YES;
    pumpWrap_disabled = 1;

    /* Stop Ampli */
    popen(SHELL_CMD_TURN_AMPLI_OFF, "r");

    /* Save prev key repeat params and set new Key repeat */
    SDL_GetKeyRepeat(&backup_key_repeat_delay, &backup_key_repeat_interval);
    if(SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL)){
        MENU_ERROR_PRINTF("ERROR with SDL_EnableKeyRepeat: %s\n", SDL_GetError());
    }

    /* Load BG */
    SDL_Surface *img_square_bg = IMG_Load(MENU_PNG_BG_PATH);
    if(!img_square_bg) {
        MENU_ERROR_PRINTF("ERROR IMG_Load: %s\n", IMG_GetError());
    }

    SDL_Surface *bg_surface = SDL_CreateRGBSurface(SDL_SWSURFACE, hw_screen->w, hw_screen->h, 16, 0, 0, 0, 0);


    SDL_BlitSurface(img_square_bg, NULL, bg_surface, NULL);
    SDL_FreeSurface(img_square_bg);

    /*  Print top arrow */
    SDL_Rect pos_arrow_top;
    pos_arrow_top.x = (bg_surface->w - img_arrow_top->w)/2;
    pos_arrow_top.y = (bg_surface->h - MENU_BG_SQUARE_HEIGHT)/4 - img_arrow_top->h/2;
    SDL_BlitSurface(img_arrow_top, NULL, bg_surface, &pos_arrow_top);

    /*  Print bottom arrow */
    SDL_Rect pos_arrow_bottom;
    pos_arrow_bottom.x = (bg_surface->w - img_arrow_bottom->w)/2;
    pos_arrow_bottom.y = bg_surface->h -
            (bg_surface->h - MENU_BG_SQUARE_HEIGHT)/4 - img_arrow_bottom->h/2;
    SDL_BlitSurface(img_arrow_bottom, NULL, bg_surface, &pos_arrow_bottom);

    if (text_surface)
        SDL_FreeSurface(text_surface);

    /* Main loop */
    while (!stop_menu_loop)
    {
        /* Handle keyboard events */
        while (SDL_PollEvent(&event))
        switch(event.type)
        {
            case SDL_QUIT:
                stop_menu_loop = 1;
                NeedExitNow = 1;
                break;
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_b:
                        if(menu_confirmation){
                            /// ------ Reset menu confirmation ------
                            menu_confirmation = 0;

                            /// ------ Refresh screen ------
                            screen_refresh = 1;
                        }
                        /*else{
                            stop_menu_loop = 1;
                        }*/
                        break;

                    case SDLK_q:
                    case SDLK_ESCAPE:
                        /*stop_menu_loop = 1;*/
                        break;

                    case SDLK_u:
                    case SDLK_UP:
                        MENU_DEBUG_PRINTF("Option UP\n");
                        option_idx = (!option_idx)?(NB_RESUME_OPTIONS-1):(option_idx-1);

                        /// ------ Reset menu confirmation ------
                        menu_confirmation = 0;

                        /// ------ Refresh screen ------
                        screen_refresh = 1;
                        break;

                    case SDLK_d:
                    case SDLK_DOWN:
                        MENU_DEBUG_PRINTF("Option DWON\n");
                        option_idx = (option_idx+1)%NB_RESUME_OPTIONS;

                        /// ------ Reset menu confirmation ------
                        menu_confirmation = 0;

                        /// ------ Refresh screen ------
                        screen_refresh = 1;
                        break;

                    case SDLK_a:
                    case SDLK_RETURN:
                        MENU_DEBUG_PRINTF("Pressed A\n");
                        if(menu_confirmation){
                            MENU_DEBUG_PRINTF("Confirmed\n");

                            /// ----- exit menu  ----
                            stop_menu_loop = 1;
                        }
                        else{
                            MENU_DEBUG_PRINTF("Asking confirmation\n");
                            menu_confirmation = 1;

                            /// ------ Refresh screen ------
                            screen_refresh = 1;
                        }
                        break;

                    default:
                        //MENU_DEBUG_PRINTF("Keydown: %d\n", event.key.keysym.sym);
                        break;
            }
            break;
        }

        /* Handle FPS */
        cur_ms = SDL_GetTicks();
        if(cur_ms-prev_ms < 1000/FPS_MENU){
            SDL_Delay(1000/FPS_MENU - (cur_ms-prev_ms));
        }
        prev_ms = SDL_GetTicks();

        /* Refresh screen */
        if(screen_refresh){
            /* Clear and draw BG */
            SDL_FillRect(hw_screen, NULL, 0);
            if(SDL_BlitSurface(bg_surface, NULL, hw_screen, NULL)){
                MENU_ERROR_PRINTF("ERROR Could not draw background: %s\n", SDL_GetError());
            }

            /* Draw resume or reset option */
            text_surface = TTF_RenderText_Blended(menu_title_font, resume_options_str[option_idx], text_color);
            text_pos.x = (hw_screen->w - MENU_ZONE_WIDTH)/2 + (MENU_ZONE_WIDTH - text_surface->w)/2;
            text_pos.y = hw_screen->h - MENU_ZONE_HEIGHT/2 - text_surface->h/2;
            SDL_BlitSurface(text_surface, NULL, hw_screen, &text_pos);

            /* Draw confirmation */
            if(menu_confirmation){
                sprintf(text_tmp, "Are you sure ?");
                text_surface = TTF_RenderText_Blended(menu_info_font, text_tmp, text_color);
                text_pos.x = (hw_screen->w - MENU_ZONE_WIDTH)/2 + (MENU_ZONE_WIDTH - text_surface->w)/2;
                text_pos.y = hw_screen->h - MENU_ZONE_HEIGHT/2 - text_surface->h/2 + 2*padding_y_from_center_menu_zone;
                SDL_BlitSurface(text_surface, NULL, hw_screen, &text_pos);
            }

            /* Flip Screen */
            SDL_Flip(hw_screen);
        }

        /* reset screen refresh */
        screen_refresh = 0;
    }

    /* Free SDL Surfaces */
    if(bg_surface)
        SDL_FreeSurface(bg_surface);
    if(text_surface)
        SDL_FreeSurface(text_surface);

    /* Reset prev key repeat params */
    if(SDL_EnableKeyRepeat(backup_key_repeat_delay, backup_key_repeat_interval)){
        MENU_ERROR_PRINTF("ERROR with SDL_EnableKeyRepeat: %s\n", SDL_GetError());
    }

    /* re-enable pumpwrap if exiting menu */
    pumpWrap_disabled = 0;

    /* Start Ampli */
    popen(SHELL_CMD_TURN_AMPLI_ON, "r");

    return option_idx;
}
