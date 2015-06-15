/**
 *  @file
 *  
 */


#include  "TwilightPath.h"

#include  "ConfigData.h"
#include  "helpers.h"
#include  "my_math.h"
#include  "suncalc.h"


//  Values used in our static (non-computed) points to indicate a screen edge.
//  These are a pixel over half of each screen dimension.  Why the extra pixel?
#define X_LEFT    (-73)
#define X_RIGHT   (73)
#define Y_TOP     (-84)
#define Y_BOTTOM  (84)


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

   //  Most path points are constant for life of this TwilightPath instance
   //  (they depend only on ScreenPartToEnclose).  But the point order varies
   //  depending on toEnclose (see aPathPoints declaration comment in header).
   pMyRet->aPathPoints[0] = GPoint(0, 9);    // always "center" hub
#ifdef PBL_PLATFORM_APLITE
   pMyRet->toEnclose = toEnclose;
   if (toEnclose != ENCLOSE_SCREEN_TOP)
   {
      // pMyRet->aPathPoints[1] = dusk zenith angle "time"
      pMyRet->aPathPoints[2] = GPoint(X_RIGHT, Y_BOTTOM);
      pMyRet->aPathPoints[3] = GPoint(X_LEFT, Y_BOTTOM);
      // pMyRet->aPathPoints[4] = dawn zenith angle "time"
   }
   else
#endif
   {
      // pMyRet->aPathPoints[1] = dawn zenith angle "time"
      pMyRet->aPathPoints[2] = GPoint(X_LEFT, Y_TOP);
      pMyRet->aPathPoints[3] = GPoint(X_RIGHT, Y_TOP);
      // pMyRet->aPathPoints[4] = dusk zenith angle "time"
   }

   //  path descriptor, which is constant for life of this TwilightPath instance.
   pMyRet->pathInfo.num_points = POINTS_IN_TWILIGHT_PATH;
   pMyRet->pathInfo.points = pMyRet->aPathPoints;

   //  until twilight_path_compute_current() is called:
   pMyRet->pPath = 0;

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

   //  convert UTC outputs to local time
   adjustTimezone(riseTime);
   adjustTimezone(setTime);

}


void  twilight_path_compute_current(TwilightPath *pTwilightPath,
                                    struct tm * localTime)
{

///BUGBUG: This is 12 hours, or 1/2 day.  Why is this value needed?
const float timeFudge = 12.0f;


   //  Find time of day for dawn and dusk times.  Results are expressed
   //  as local hour-of-day, with minutes as fraction of an hour.
   float fDawnTime;
   float fDuskTime;
   calcRiseAndSet(&fDawnTime, &fDuskTime, localTime, pTwilightPath->fZenith);

   //  save true dawn / dusk times
   pTwilightPath->fDawnTime = fDawnTime;
   pTwilightPath->fDuskTime = fDuskTime;

   //  Update dawn / dusk points to reflect zenith at present location / date.

   fDawnTime += timeFudge;
   GPoint dawnPoint = GPoint((int16_t)(my_sin(fDawnTime / 24 * M_PI * 2) * 120),
                             9 - (int16_t)(my_cos(fDawnTime / 24 * M_PI * 2) * 120));

   fDuskTime += timeFudge;
   GPoint duskPoint = GPoint((int16_t)(my_sin(fDuskTime / 24 * M_PI * 2) * 120),
                             9 - (int16_t)(my_cos(fDuskTime / 24 * M_PI * 2) * 120));

   //  do the point init which twilight_path_create() couldn't.
#ifdef PBL_PLATFORM_APLITE    // basalt always encloses screen top
   if (pTwilightPath->toEnclose != ENCLOSE_SCREEN_TOP)
   {
      pTwilightPath->aPathPoints[1] = duskPoint;
      pTwilightPath->aPathPoints[4] = dawnPoint;
   }
   else
#endif
   {
      pTwilightPath->aPathPoints[1] = dawnPoint;
      pTwilightPath->aPathPoints[4] = duskPoint;
   }

   //  (Actual GPath creation is done in twilight_path_render().)

   return;

}  /* end of twilight_path_compute_current */


void  twilight_path_render(TwilightPath *pTwilightPath, GContext *ctx,
                           GColor color, GRect frameDst)
{


   if ((pTwilightPath->fDawnTime == NO_RISE_SET_TIME) ||
       (pTwilightPath->fDuskTime == NO_RISE_SET_TIME))
   {
      //  sun either never sets or never rises at this location / time.
      //  For now, simply render nothing.
      return;
   }

   //  Recreate path each time.  Not sure if this is strictly necessary
   //  but it would probably not be good to apply gpath_move_to() more
   //  than once for the same path instance.
   if (pTwilightPath->pPath != NULL)
      gpath_destroy(pTwilightPath->pPath);

   pTwilightPath->pPath = gpath_create(&(pTwilightPath->pathInfo));
   if (pTwilightPath->pPath == NULL)
      return;
   gpath_move_to(pTwilightPath->pPath, grect_center_point(&frameDst));

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

