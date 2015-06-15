/**
 *  @file
 *  
 */

#pragma once

#include  "pebble.h"


///  Should created path enclose top or bottom of screen?
typedef enum {
   ENCLOSE_SCREEN_TOP,
   ENCLOSE_SCREEN_BOTTOM
} ScreenPartToEnclose;


///  Four "corners" plus center point.
#define  POINTS_IN_TWILIGHT_PATH   5

/** 
 *  Carries data about a single path which includes two lines, roughly
 *  like hands of a clock, which show the specific times of the sun
 *  hitting a particular "zenith" value (e.g., rise / set times).
 *  These lines are calculated so their outer ends intercept our 24 hour
 *  watchface at the proper points to show their time values.  The lines'
 *  inner ends join with each other at the center of the watchface.
 *  
 *  The remainder of the path extends either up or down (caller selected)
 *  to encompass all of the watch screen above or below the zenith lines.
 *  
 *  The zenith line endpoints are calculated using the presently known
 *  user location, and the current date.
 *  
 *  Our computed path coords are relative, using as a zero-point the axis
 *  of the hour hand's rotation.
 *  
 *  This structure also includes an optional bitmap resource.  When
 *  present, the bitmap is rendered immediately before we fill our path.
 *  When a bitmap is present, our path fill typically is used to carve out
 *  part of the bitmap (which can only be rendered to a rectangle) and
 *  change it back to white.
 */
typedef struct {

   /**
    *  Collection of points comprising our path.  We don't explicitly close
    *  the path, but PebbleOS seems to infer that.
    *  
    *  NOTE: sample file
    *  
    *    PebbleSDK-2.0-BETA4/Examples/watchapps/feature_gpath/src/feature_gpath.c
    *  
    *  includes this comment:
    *  
    *    A path can be concave, but it should not twist on itself
    *    The points should be defined in clockwise order due to the rendering
    *    implementation. Counter-clockwise will work in older firmwares, but
    *    it is not officially supported
    *  
    *  So we change the ordering of our computed points depending on whether
    *  the path is to enclose the top or bottom of the screen, to ensure that
    *  our path goes in a clockwise direction.
    */
   GPoint aPathPoints[POINTS_IN_TWILIGHT_PATH];

   ///  Descriptor pointing to aPathPoints, used to create a GPath.
   GPathInfo pathInfo;

   ///  Derived from the above, and ready for use with Pebble graphics primitives.
   GPath *pPath;

   /**
    *  Bitmap resource to render to screen immediately before path fill.
    *  NULL if there is no bitmap to render (i.e., for our initial
    *  black-fill of the bottom part of the screen).
    *  Non-null only for aplite platforms.
    */
   GBitmap* pBmpGrey;   // some shade of gray

   /**
    *  Color value used to fill within twilight path.
    */

   /**
    *  Zenith value for our path.  This is the angle between the sun's zenith
    *  position ("high noon") and the position our twilight path represents.
    */
   float  fZenith;

#ifdef PBL_PLATFORM_APLITE
   ///  Does our path enclose the top or bottom part of the screen?
   ScreenPartToEnclose toEnclose;
#endif

   ///  For convenience, preserve the dawn and dusk times which we compute.

   /** 
    *  Time of zenith dawn at our current location, for the date supplied to
    *  twilight_path_compute_current
    */
   float  fDawnTime;

   /**
    *  Time of zenith dusk at our current location, for the date supplied to
    *  twilight_path_compute_current
    */
   float  fDuskTime;

} TwilightPath;

#ifdef PBL_PLATFORM_APLITE

//  Non-night colors are base for bitmap filling on top of.
#define  TWI_COLOR_NIGHT      GColorBlack
#define  TWI_COLOR_ASTRO      GColorBlack
#define  TWI_COLOR_NAUTICAL   GColorWhite
#define  TWI_COLOR_CIVIL      GColorWhite
#define  TWI_COLOR_DAYTIME    GColorWhite

#elif PBL_PLATFORM_BASALT
#define  TWI_COLOR_NIGHT      GColorOxfordBlue
#define  TWI_COLOR_ASTRO      GColorVividViolet   /*GColorPurple,     GColorDukeBlue*/
#define  TWI_COLOR_NAUTICAL   GColorFolly         /*GColorIndigo*/
#define  TWI_COLOR_CIVIL      GColorChromeYellow  /*GColorLavenderIndigo*/
#define  TWI_COLOR_DAYTIME    GColorPastelYellow

#endif


//  Simplify parameter passage into twilight_path_create().
#ifdef PBL_PLATFORM_APLITE
#define TWI_APLITE_RES_ONLY(bitmap_id)  bitmap_id
#elif PBL_PLATFORM_BASALT
#define TWI_APLITE_RES_ONLY(bitmap_id)  INVALID_RESOURCE
#endif

/**
 *  Allocate a TwilightPath instance, save the supplied parameters in it,
 *  and pre-populate as much "static" data in the instance as possible.
 *  
 *  To be usable, the returned TwilightPath instance must first be passed to
 *  \ref twilight_path_compute_current().
 * 
 *  @param zenithAngle Angle in degrees of sun position relative to zenith
 *             which we should use in calculating our graphics path.
 *  @param toEnclose Should graphics path enclose top or bottom of screen?
 *  @param bitmapResId Aplite:  Resource ID of bitmap to use when rendering.
 *                         Set to INVALID_RESOURCE for no bitmap.
 *                     Basalt:  Color value to fill with (no bitmap used!).
 */
TwilightPath * twilight_path_create(float zenithAngle, ScreenPartToEnclose toEnclose,
                                    uint32_t greyBitmapResourceId);


/**
 *  Compute dawn / dusk times for supplied twilight path instance, using given
 *  date and current (most recently read from phone) location values to
 *  complete the calculations.
 *  
 *  With the dawn / dusk times in hand, create a graphics path showing those
 *  times and enclosing either top or bottom of the watch screen, as requested
 *  when twilight_path_create() was called to create this instance.
 * 
 *  @param pTwilightPath Twilight path instance to update for present location/date.
 *  @param utcTime UTC time to compute dawn / dusk for.
 */
void  twilight_path_compute_current(TwilightPath *pTwilightPath,
                                    struct tm * localTime);


/**
 *  Render optional bitmap (specified during _create()) to full screen using
 *  GCompAnd compositing, and then fill our path with the specified color.
 *  Thus we write the bitmap and then carve out a chunk of it corresponding
 *  to the "daytime" part beyond our twilight range.
 * 
 *  @param pTwilightPath Contains path info for filling.
 *  @param ctx Graphics context to render to.  Iff we are supplied a bitmap
 *              then we change the context's compositing mode to GCompAnd.
 *  @param color Color to fill our path with.
 *  @param frameDst Frame to constrain rendering to (whole window).
 */
void  twilight_path_render(TwilightPath *pTwilightPath, GContext *ctx,
                           GColor color, GRect frameDst);


void  twilight_path_destroy(TwilightPath *pTwilightPath);

