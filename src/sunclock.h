/**
 *  @file
 *  
 *  Sundry public definitions for code accessing the sunclock watchface
 *  window code in sunclock.c.
 */

#pragma once

#include  <pebble.h>


void  sunclock_handle_init(void);
void  sunclock_handle_deinit(void);

void sunclock_coords_recvd(float latitude, float longitude, int32_t utcOffset);

#if ! PBL_PLATFORM_APLITE
//  Does given time fall over dark or light portion of watchface?
bool  is_dark_time(int localHour, int localMinute);
#endif

//  Some resources loaded by sunclock_handle_init() which might be useful elsewhere:

///  Roboto Condensed 19: "[a-zA-Z, :0-9]" chars only.  Used for face's date text.
extern GFont pFontMediumText;

// System font (Raster Gothic 18), doesn't need unloading.
extern GFont pFontSmallText;

