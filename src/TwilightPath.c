/**
 *  @file
 *  
 */


#include  "TwilightPath.h"

#include  "ConfigData.h"
#include  "geometry.h"
#include  "helpers.h"
#include  "my_math.h"
#include  "suncalc.h"


//  Values used in our static (non-computed) points to indicate a screen edge.
//  These are relative to the dial's center hub, to vertical values must account
//  for the face's screen offset if any.
#define X_RIGHT   (DISP_WIDTH / 2 - 1)
#define X_LEFT    (-DISP_WIDTH / 2)
#define Y_BOTTOM  (DISP_HEIGHT / 2 - FACE_VOFFSET)
#define Y_TOP     (-DISP_HEIGHT / 2 - FACE_VOFFSET)


TwilightPath * twilight_path_create(float zenithAngle, ScreenPartToEnclose toEnclose,
                                    uint32_t greyBitmapResourceId)
{

   TwilightPath * pMyRet = malloc(sizeof(TwilightPath));
   if (pMyRet == 0)
   {
      return pMyRet;
   }

   pMyRet->fZenith = zenithAngle;

   if (greyBitmapResourceId != INVALID_RESOURCE)
   {
      pMyRet->pBmpGrey = gbitmap_create_with_resource(greyBitmapResourceId);
      if (pMyRet->pBmpGrey == NULL)
      {
         free(pMyRet);
         return NULL;
      }
   }
   else
   {
      pMyRet->pBmpGrey = NULL;
   }

   //  until twilight_path_compute_current() is called:
   pMyRet->pPath = 0;
   pMyRet->pathInfo.num_points = 0;
   pMyRet->pathInfo.points = pMyRet->aPathPoints;

#ifdef PBL_PLATFORM_APLITE
   pMyRet->toEnclose = toEnclose;
#endif

   return pMyRet; 

}  /* end of twilight_path_create */


/**
 *  Adjust UTC hour + fraction to same in local time.
 *  
 *  Relies on config_data_get_tz_in_hours() correctly reflecting the current
 *  timezone + DST setting for the currently configured location.
 * 
 *  @param time Points to var holding a UTC hour + fraction (minutes etc.).
 *             We update the value of this var to the local time equivalent
 *             of its original value.
 *             Or, if input time is NO_RISE_SET_TIME then we return the same.
 */
static void adjustTimezone(float *time)
{
   float newTime;

   if (*time != NO_RISE_SET_TIME)
   {
      newTime = *time + config_data_get_tz_in_hours(); 
      if (newTime > 24)
         newTime -= 24;
      if (newTime < 0)
         newTime += 24;

      *time = newTime;
   }

}  /* end of adjustTimezone */


/**
 *  Calculate rise / set time pair for a given zenith value and UTC date.
 * 
 * @param riseTime Set to local time (hour + fraction) sun rises to specified 
 *                zenith on given date.
 * @param setTime Set to local time (hour + fraction) sun sets to specified 
 *                zenith on given date.
 * @param dateLocal Local date to find rise/set values for.
 * @param zenith Definition of "rise" / "set": used to select true rise / set,
 *                or various flavors of twilight. This is an unsigned deflection
 *                angle in degrees, with zero representing "directly overhead" (noon).
 */
static void  calcRiseAndSet(float *riseTime, float *setTime,
                            const struct tm* dateLocal, float zenith)
{

   float latitude = config_data_get_latitude();
   float longitude = config_data_get_longitude();

   //BUGBUG - date should be UTC!

   *riseTime = calcSunRise(dateLocal->tm_year, dateLocal->tm_mon + 1, dateLocal->tm_mday,
                           latitude, longitude, zenith);

   *setTime = calcSunSet(dateLocal->tm_year, dateLocal->tm_mon + 1, dateLocal->tm_mday,
                         latitude, longitude, zenith);

   if ((*riseTime == NO_RISE_SET_TIME) || (*setTime == NO_RISE_SET_TIME))
   {
      //  probably get get both or neither, but force the matter for ease of checking:
      *riseTime = *setTime = NO_RISE_SET_TIME;
   }
   else
   {
      //  convert UTC outputs to local time
      adjustTimezone(riseTime);
      adjustTimezone(setTime);
   }

} /* end of calcRiseAndSet() */


/**
 *  Find the location of a point in the twilight path, corresponding to some
 *  time expressed.  Since this is for drawing a twilight path, we attempt
 *  to ensure that the point is at the very edge of the screen.  This is
 *  expressed in the usual hub-relative twilight path form.
 *  
 *  We do not attempt to crop the point's coordinates to overall X/Y minima / maxima
 *  for the our host Pebble device's screen, since this would result in altering
 *  the effective hour angle (at least when done trivially, and cycles / RAM are
 *  both at premiums).
 * 
 *  @param fLocalHour Local time, in hours and fractions of an hour.
 *  @param pPoint Point where a ray drawn from the dial hub through the localHour
 *                dial mark strikes the edge of the Pebble display.  May be off
 *                the screen, works because our target layer does clipping.
 * 
 *  @return \c true if we produce a valid point, or \c false if the input hour 
 *           value is NO_RISE_SET_TIME (== "no such twilight band now"). 
 */
bool  find_time_path_point(float fLocalHour, struct GPoint *pPoint)
{


   if (fLocalHour == NO_RISE_SET_TIME)
   {
      //  no path for this phase of twilight
      return (false);
   }

   //  This is a little tricky since
   //    1) our time of 00:00 / 24:00 is down, not up, since our face shows midnight
   //       at the traditional "6:00" position (looks better this way), and
   //    2) our trig operations assume a zero angle means x=1 / y=0 as customary, and
   //    3) trig angle increases counter-clock while our hour angle increase clock-wise.
   //
   //  As observed in draw_small_hour_mark(), we can deal with these issues thusly:
   //  Since the Pebble's native Y axis values increase down the display, this causes a
   //  mirroring effect on sin/cos values so that angles increase in a clockwise direction.
   //  So we only need to offset our hour value so that 00:00 comes out 1/4 of the way
   //  around the dial from trig's natural 0 angle (18:00 on our face).

   fLocalHour += 6;

   const float fLocalHourRadians = fLocalHour / 24 * M_PI * 2;

   int16_t x = (int16_t)(my_cos(fLocalHourRadians) * FULL_DISP_RADIUS);
   int16_t y = (int16_t)(my_sin(fLocalHourRadians) * FULL_DISP_RADIUS);

   //  NB: tempting to clip x & y to the screen edge, but doing so changes
   //      the effective hour angle.  Fortunately, Pebble's default for
   //      the base layer which we're drawing into is clipping-enabled.

   *pPoint = GPoint(x, y);

   return (true);

}  /* end of find_time_path_point() */


void  twilight_path_compute_current(TwilightPath *pTwilightPath,
                                    struct tm * localTime)
{


   //  Find time of day for dawn and dusk times.  Results are expressed
   //  as local hour-of-day, with minutes as fraction of an hour.
   float fDawnTime;
   float fDuskTime;
   calcRiseAndSet(&fDawnTime, &fDuskTime, localTime, pTwilightPath->fZenith);

   //  save true dawn / dusk times
   pTwilightPath->fDawnTime = fDawnTime;
   pTwilightPath->fDuskTime = fDuskTime;

   GPoint dawnPoint;
   GPoint duskPoint;

   //  Update dawn / dusk points to reflect zenith at present location / date.
   //  We are computing coords of the proper points on the hour mark dial for
   //  each of dawn and dusk times for the given zenith: these are relative
   //  to the dial's center hub.

   if ((! find_time_path_point(fDawnTime, &dawnPoint)) ||
       (! find_time_path_point(fDuskTime, &duskPoint)))
   {
      //  this twilight path's zenith doesn't apply at this location / date
      return;
   }

   //  Number of path points varies with latitude: we only include display
   //  corner points when dawntime / dusktime values are themselves to the
   //  left or right of the 45 degree lines into said corners.
   //  Point order also varies depending on toEnclose (see aPathPoints
   //  declaration comment in header).

   pTwilightPath->aPathPoints[0] = GPoint(0, 0);    // always center hub

   ///  Next point to write in array.
   int iPt = 1;

#ifdef PBL_PLATFORM_APLITE
   if (pTwilightPath->toEnclose != ENCLOSE_SCREEN_TOP)
   {
      //  path encloses bottom part of screen
      pTwilightPath->aPathPoints[iPt++] = duskPoint;
      if (fDuskTime < 15)
      {
         //  fill to upper right corner
         pTwilightPath->aPathPoints[iPt++] = GPoint(X_RIGHT, Y_TOP);
      }
      if (fDuskTime < 21)
      {
         //  fill to lower right corner
         pTwilightPath->aPathPoints[iPt++] = GPoint(X_RIGHT, Y_BOTTOM);
      }
      if (fDawnTime > 3)
      {
         //  fill to lower left corner
         pTwilightPath->aPathPoints[iPt++] = GPoint(X_LEFT, Y_BOTTOM);
      }
      if (fDawnTime > 9)
      {
         //  fill to upper left corner
         pTwilightPath->aPathPoints[iPt++] = GPoint(X_LEFT, Y_TOP);
      }
      pTwilightPath->aPathPoints[iPt++] = dawnPoint;
   }
   else
#endif
   {
      //  path encloses top part of screen
      pTwilightPath->aPathPoints[iPt++] = dawnPoint;
      if (fDawnTime < 9)
      {
         //  fill to upper left corner
         pTwilightPath->aPathPoints[iPt++] = GPoint(X_LEFT, Y_TOP);
      }
      if (fDawnTime < 3)
      {
         //  fill to lower left corner
         pTwilightPath->aPathPoints[iPt++] = GPoint(X_LEFT, Y_BOTTOM);
      }
      if (fDuskTime > 15)
      {
         //  fill to upper right corner
         pTwilightPath->aPathPoints[iPt++] = GPoint(X_RIGHT, Y_TOP);
      }
      if (fDuskTime > 21)
      {
         //  fill to lower right corner
         pTwilightPath->aPathPoints[iPt++] = GPoint(X_RIGHT, Y_BOTTOM);
      }
      pTwilightPath->aPathPoints[iPt++] = duskPoint;
   }

   //  path descriptor, which is constant for life of this TwilightPath instance.
   pTwilightPath->pathInfo.num_points = iPt;
   pTwilightPath->pathInfo.points = pTwilightPath->aPathPoints;

   //  (Actual GPath creation is done in twilight_path_render().)

   return;

}  /* end of twilight_path_compute_current */


void  twilight_path_render(TwilightPath *pTwilightPath, GContext *ctx,
                           GColor color, GRect frameDst)
{


   //  Recreate path each time.  Not sure if this is strictly necessary
   //  but it would probably not be good to apply gpath_move_to() more
   //  than once for the same path instance.
   if (pTwilightPath->pPath != NULL)
      gpath_destroy(pTwilightPath->pPath);

   if ((pTwilightPath->fDawnTime == NO_RISE_SET_TIME) ||
       (pTwilightPath->fDuskTime == NO_RISE_SET_TIME))
   {
      //  sun either never sets or never rises at this location / time.
      //  For now, simply render nothing.
      pTwilightPath->pPath = 0;
      return;
   }

   pTwilightPath->pPath = gpath_create(&(pTwilightPath->pathInfo));
   if (pTwilightPath->pPath == NULL)
      return;

   GPoint  centerPoint = grect_center_point(&frameDst);
   centerPoint.y += FACE_VOFFSET;

   gpath_move_to(pTwilightPath->pPath, centerPoint);

   //  do rendering

   if (pTwilightPath->pBmpGrey != NULL)
   {
      graphics_context_set_compositing_mode(ctx, GCompOpAnd); 
      graphics_draw_bitmap_in_rect(ctx, pTwilightPath->pBmpGrey, frameDst);
   }

   graphics_context_set_fill_color(ctx, color);
   if ((pTwilightPath != NULL) && (pTwilightPath->pPath != NULL))
   {
      gpath_draw_filled(ctx, pTwilightPath->pPath); 
   }

}  /* end of twilight_path_render */


void  twilight_path_destroy(TwilightPath *pTwilightPath)
{

   if (pTwilightPath != 0)
   {
      SAFE_DESTROY(gpath, pTwilightPath->pPath);
      SAFE_DESTROY(gbitmap, pTwilightPath->pBmpGrey);

      free(pTwilightPath);
   }

   return;

}  /* end of twilight_path_destroy */

