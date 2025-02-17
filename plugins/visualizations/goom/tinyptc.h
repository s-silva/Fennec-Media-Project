//
// TinyPTC by Gaffer
// www.gaffer.org/tinyptc
//

#include <windows.h>

#ifndef __TINYPTC_WINDOWS_H
#define __TINYPTC_WINDOWS_H


// integer types
typedef unsigned __int32 int32;
typedef unsigned __int16 short16;
typedef unsigned __int8 char8;

// tinyptc api
extern int ptc_open(char *title,int width,int height);
int ptc_open_ex(char *title,int width,int height, HWND hwnd, HDC hdc);
extern int ptc_update(void *buffer);
extern void ptc_close();
int ptc_draw(void);
HWND ptc_get_wnd(void);

// display target
#define __PTC_DDRAW__
//#define __PTC_GDI__
//#define __PTC_VFW__

// configuration
#ifndef GOOM_FULLSCREEN
#define __PTC_WINDOWED__
#endif
#define __PTC_CENTER_WINDOW__
#define __PTC_RESIZE_WINDOW__
#define __PTC_SYSTEM_MENU__
#define __PTC_ICON__ "IDI_MAIN"
#define __PTC_ALLOW_CLOSE__
#define __PTC_CLOSE_ON_ESCAPE__
#define __PTC_DISABLE_SCREENSAVER__

// converter configuration
#define __PTC_CONVERTER_32_TO_32_RGB888
#define __PTC_CONVERTER_32_TO_32_BGR888
#define __PTC_CONVERTER_32_TO_24_RGB888
#define __PTC_CONVERTER_32_TO_24_BGR888
#define __PTC_CONVERTER_32_TO_16_RGB565
#define __PTC_CONVERTER_32_TO_16_BGR565
#define __PTC_CONVERTER_32_TO_16_RGB555
#define __PTC_CONVERTER_32_TO_16_BGR555

// endian configuration
#define __PTC_LITTLE_ENDIAN__

// optimization
#define __PTC_MMX__
#define __PTC_MEMCPY__

// linker configuration
#define __PTC_WINMAIN_CRT__
//#define __PTC_MAIN_CRT__


#endif
