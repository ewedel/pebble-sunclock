/**
 *  @file
 *  
 *  Definitions relating to the Pebble platform our current build is targeting.
 *  
 *  These defs are more-or-less for routine use.  See also geometry.h and testing.h.
 */


#pragma once


#ifdef PBL_PLATFORM_APLITE

//# define LOW_MEMORY_DEVICE  1
# define MY_APP_LOG(...)

#else

//# define LOW_MEMORY_DEVICE  0
# define MY_APP_LOG  APP_LOG

#endif

