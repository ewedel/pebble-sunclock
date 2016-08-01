/**
 *  @file
 *  
 *  
 *  Twilight clock watchface app.
 *  
 *  Present heap utilization, as reported by "pebble logs" at face exit:
 *  
 *    [INFO    ] I app_manager.c:134 Heap Usage for <Twilight-Clock>:
 *               Available <12608B> Used <8668B> Still allocated <0B>
 */

#include "pebble.h"

#include "config.h"
#include "ConfigData.h"
#include "dial_mask_path.h"
#include "geometry.h"
#include "helpers.h"
#include "hour_hand.h"
#include "MessageWindow.h"
#include "messaging.h"
#include "my_math.h"
#include "platform.h"
#include "suncalc.h"
#include "TransBitmap.h"
#include "TransRotBmp.h"
#include "TwilightPath.h"


/// Test whether using a built-in font is smaller than using a (subsetted) resource.
#define USE_FONT_RESOURCE 1   /* 0 == "use built-in font" */

///  Main watchface window.
Window *pWindow;

///  Failed initialization due to lack of heap?
bool  s_fHeapFailure = false;

///  Is watchface's message pump running?  Not true until our first window proc callback.
bool fMessagePumpRunning = false;

#if HOUR_VIBRATION
const VibePattern hour_pattern = {
   .durations = (uint32_t[]){ 200, 100, 200, 100, 200 },
   .num_segments = 5
};
#endif

#define INITIAL_LAT_LONG_REQUESTS_MAX  2

///  Minimum number of requests issued each time watchface is started.
///  We use more than one since sometimes the first is lost.
unsigned cInitialLatLongRequestsRemaining = INITIAL_LAT_LONG_REQUESTS_MAX;

TextLayer *pTextTimeLayer      = 0;
#ifndef PBL_ROUND
TextLayer *pTextSunriseLayer   = 0;
TextLayer *pTextSunsetLayer    = 0;
TextLayer *pDayOfWeekLayer     = 0;
#endif
TextLayer *pMonthLayer         = 0;
TextLayer *pMoonLayer          = 0;   // moon phase

///  Not a real layer, but the layer of the base window.
///  This is where our watch "dial" (twilight bands, etc.) is drawn.
Layer     *pGraphicsNightLayer = 0;

//Make fonts global so we can deinit later
GFont pFontCurTime  = 0;
GFont pFontMoon     = 0;

///  Roboto Condensed 19: "[a-zA-Z, :0-9]" chars only.  Used for face's date text.
GFont pFontMediumText = 0;

// system font (Raster Gothic 18), doesn't need unloading:
GFont pFontSmallText = 0;


#ifdef PBL_PLATFORM_APLITE
/**
 *  Watchface dial: a transparent png which supplies hour marks, a face
 *  outline, and masks everything outside the face to black.  Aside from
 *  the hour marks, the interior of the face is transparent to allow
 *  twilight bands to show through.
 */
TransBitmap* pTransBmpWatchface = 0;
#endif

///  Boundary between night and astronomical twilight.
TwilightPath* pTwiPathNight = 0;

///  Boundary between astronomical and nautical twilight.
TwilightPath* pTwiPathAstro = 0;

///  Boundary between nautical and civil twilight.
TwilightPath* pTwiPathNautical = 0;

///  Daylight edge of civil twilight (i.e., sun rise / set times).
TwilightPath* pTwiPathCivil = 0;



/**
 *  Based on our most recently calculated twilight bands, report whether
 *  the supplied time falls over "dark" (night or astro twilight) or "light"
 *  (nautical / civil twilight and daylight) part of the display.
 *  
 *  Note that the time is 0:00 - 23:59, since we have a 24 hour face.
 *  
 *  @param localHour Hour, in local time.
 *  @param localMin Minute
 *  
 *  @return \c true if the supplied time falls on a dark background (see description),
 *          else \c false.
 */

bool  is_dark_time(int localHour, int localMinute)
{

   float localTickTime = (float) localHour + localMinute / 60.0;

   return ((localTickTime < pTwiPathAstro->fDawnTime) ||
           (localTickTime > pTwiPathAstro->fDuskTime));

}  /* end of is_dark_time() */


/**
 *  Handler called when the "night layer" needs redrawing.
 *  
 *  Note that the heavy calculation has been done ahead of time by
 *  \href updateDayAndNightInfo(), so this callback should be a bit
 *  zippier than it otherwise might be.  (Not to say "fast"..)
 * 
 * @param me Night layer being updated.
 * @param ctx System-supplied context, presumably already set to defaults
 *             for *me. 
 */
void graphics_night_layer_update_callback(Layer *me, GContext *ctx)
{


   if (s_fHeapFailure)
   {
      //  attempt to put up a trivial screen message.
      GFont font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
      graphics_context_set_text_color(ctx, GColorBlack);
      graphics_context_set_compositing_mode(ctx, GCompOpAssign);  //needed?
      GRect layer_bounds = layer_get_bounds(me);

      graphics_draw_text(ctx, "Out of Memory", font, layer_bounds,
                         GTextOverflowModeWordWrap,
                         GTextAlignmentCenter, NULL);
      return;
   }

   fMessagePumpRunning = true;

   //  Don't do our display hold-off until our message pump is running.
   //  Calling back a window update handler, of which this is the first,
   //  is a good way to know.  So now we can check for available data
   //  and hold off proper clock display until it is found:
   if (! config_data_location_avail())
   {
      //  Probably initial program run: no config data persisted yet.
      //  Put up a special window informing the user of this.
      message_window_show_status ("Getting Location",
                                  "Obtaining initial location data.");

      app_msg_RequestLatLong();

      return;
   }
   else if (cInitialLatLongRequestsRemaining > 0)
   {
      cInitialLatLongRequestsRemaining --;
      app_msg_RequestLatLong();
   }

   GRect layerFrame = layer_get_frame(me);

   //BUGBUG: are these
   //  GRect(0, 0, 144, 168)
   //not equal to layerFrame?

   // ------------------------------------------------

   //  With basalt added, this is now even more confusing than before.
   //  For aplite, the first render carves (fills black) night region
   //  out of a white screen.
   //  For basalt, the first render carves (fills astro) astro region
   //  out of a night-colored screen.
   //  This difference is because aplite needs to support bitmap draws
   //  via OR, and relies on the bitmap draws to add twilight "color".

#ifdef PBL_COLOR
   graphics_context_set_fill_color(ctx, TWI_COLOR_NIGHT);
   graphics_fill_rect(ctx, layerFrame, 1, 0);
#endif

   //  aplite: start out with white screen, draw full-night black to bottom part
   //  basalt: start out with night screen, fill all above night with astro.
   twilight_path_render(pTwiPathNight, ctx, TWI_COLOR_ASTRO, layerFrame);

   //  turn all of white remainder (upper part of screen) into dark grey & then
   //  turn upper part of screen above astro twilight band back into white
   twilight_path_render(pTwiPathAstro, ctx, TWI_COLOR_NAUTICAL, layerFrame);

   //  turn all of white remainder (upper part of screen) into medium grey &
   //  turn upper part of screen above nautical twilight band back into white
   twilight_path_render(pTwiPathNautical, ctx, TWI_COLOR_CIVIL, layerFrame);

   //  turn all of white remainder (upper part of screen) into light grey &
   //  turn upper part of screen above civil twilight band back into white
   twilight_path_render(pTwiPathCivil, ctx, TWI_COLOR_DAYTIME, layerFrame);

   // ------------------------------------------------

#ifdef PBL_PLATFORM_APLITE
   //  place tidy watchface frame over accumulated render of twilight bands:
   transbitmap_draw_in_rect(pTransBmpWatchface, ctx, layerFrame);
#else
   //  post-aplite we have necessary path primitives to actively mask
   //  & decorate watchface.  This also supports alternate resolutions.
   draw_watchface_mask(ctx, layerFrame);
#endif

   //  not clear why this is done: perhaps the system needs it?
   graphics_context_set_compositing_mode(ctx, GCompOpAssign);

   return;

}  /* end of graphics_night_layer_update_callback() */

float get24HourAngle(int hours, int minutes)
{
   return (12.0f + hours + (minutes / 60.0f)) / 24.0f;
}

/** 
 *  Given a presumably UTC time, return the astronomical julian day.
 *  This is not day-of-year, but a much larger value.
 * 
 *  @param timeUTC UTC time, unpacked.
 */ 
int tm2jd(struct tm *timeUTC)
{
   int y, m, d, a, b, c, e, f;
   y = timeUTC->tm_year + 1900;
   m = timeUTC->tm_mon + 1;
   d = timeUTC->tm_mday;
   if (m < 3)
   {
      m += 12;
      y -= 1;
   }
   a = y / 100;
   b = a / 4;
   c = 2 - a + b;
   e = 365.25 * (y + 4716);
   f = 30.6001 * (m + 1);
   return c + d + e + f - 1524;
}


int moon_phase(int jdn)
{
   double jd;
   jd = jdn - 2451550.1;
   jd /= 29.530588853;
   jd -= (int)jd;
   return (int)(jd * 27 + 0.5); // scale fraction from 0-27 and round by adding 0.5
}


/**
 *  Update lunar phase.  Intended to be called once per day.
 */
void DisplayCurrentLunarPhase()
{

   static char moon[] = "m";
   int moonphase_number = 0;

   // moon
   time_t timeNow;
   timeNow = time(NULL);
   moonphase_number = moon_phase(tm2jd(gmtime(&timeNow)));
   // correct for southern hemisphere
   if ((moonphase_number > 0) && (config_data_get_latitude() < 0))
      moonphase_number = 28 - moonphase_number;

   // select correct font char
   if (moonphase_number == 14)
   {
      moon[0] = (unsigned char)(48);
   } else if (moonphase_number == 0)
   {
      moon[0] = (unsigned char)(49);
   } else if (moonphase_number < 14)
   {
      moon[0] = (unsigned char)(moonphase_number + 96);
   } else
   {
      moon[0] = (unsigned char)(moonphase_number + 95);
   }
//    moon[0] = (unsigned char)(moonphase_number);

   text_layer_set_text(pMoonLayer, moon);

}  /* end of DisplayCurrentLunarPhase */


/**
 *  Calculate sunrise, sunset, and all corresponding twilight
 *  times for current day.
 *  
 *  This only needs to be called once a day (aside from startup time).
 * 
 * @param update_everything True to update everything. False to
 *                          only update when the day has
 *                          changed.
 */
void updateDayAndNightInfo(bool update_everything)
{
#ifndef PBL_ROUND
   static char sunrise_text[] = "00:00";
   static char sunset_text[] = "00:00";
#endif

   ///  Localtime mday of most recent completed day/night update.
   ///  This means we normally update just after midnight, which
   ///  seems a good time.
   static short lastUpdateDay = -1;


   time_t timeNow;
   timeNow = time(NULL);
   struct tm tmNowLocal = *(localtime(&timeNow));

   if ((lastUpdateDay == tmNowLocal.tm_mday) && !update_everything)
   {
      return;
   }

   twilight_path_compute_current(pTwiPathNight,    &tmNowLocal);
   twilight_path_compute_current(pTwiPathAstro,    &tmNowLocal);
   twilight_path_compute_current(pTwiPathNautical, &tmNowLocal);
   twilight_path_compute_current(pTwiPathCivil,    &tmNowLocal);

#ifndef PBL_ROUND

   //  Want the user's default time format, but not for the current time.
   //  We can't use clock_copy_time_string(), so make an equivalent format:
   char *time_format;

   if (clock_is_24h_style())
   {
      time_format = "%R";
   } else
   {
      time_format = "%l:%M";
   }

   float sunriseTime = pTwiPathCivil->fDawnTime;
   float sunsetTime  = pTwiPathCivil->fDuskTime;

   //BUGBUG - need to round this
   tmNowLocal.tm_min = (int)(60 * (sunriseTime - ((int)(sunriseTime))));
   tmNowLocal.tm_hour = (int)sunriseTime;
   strftime(sunrise_text, sizeof(sunrise_text), time_format, &tmNowLocal);
   text_layer_set_text(pTextSunriseLayer, sunrise_text);

   tmNowLocal.tm_min = (int)(60 * (sunsetTime - ((int)(sunsetTime))));
   tmNowLocal.tm_hour = (int)sunsetTime;
   strftime(sunset_text, sizeof(sunset_text), time_format, &tmNowLocal);
   text_layer_set_text(pTextSunsetLayer, sunset_text);
   text_layer_set_text_alignment(pTextSunsetLayer, GTextAlignmentRight);

#endif  // #ifndef PBL_ROUND

   DisplayCurrentLunarPhase();

   lastUpdateDay = tmNowLocal.tm_mday;

   //  other layers should take care of themselves, but make sure our base
   //  "dial" bitmap is updated.
   layer_mark_dirty(pGraphicsNightLayer);

}  /* end of updateDayAndNightInfo() */


/**
 *  Once a minute, update textual time displays, and analog hour hand.
 *  
 *  Also calls out to daily updater, which limits itself to acting
 *  only when due.
 * 
 *  @param tick_time The time at which the tick event was triggered.
 *                At least in PebbleOS versions 2 and earlier, this
 *                is local time.
 *  @param units_changed Which unit change triggered this tick event.
 *                Usually always minutes as that's all we register for.
 */

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed)
{


   (void) units_changed;

   if (! config_data_location_avail())
   {
      return;
   }

#ifndef PBL_SDK_2
   if (config_has_tz_offset_changed())
   {
      // in case change is due to physical relocation (e.g., just landed
      // and turned phone back on after a flight), also request updated
      // location info from the phone.  Doesn't hurt to ask anyway. ;-)
      if (fMessagePumpRunning)
      {
         //  cause graphics_night_layer_update_callback(), whose invocation
         //  we trigger via updateDayAndNightInfo(), to ask phone for updated
         //  location info
         cInitialLatLongRequestsRemaining = INITIAL_LAT_LONG_REQUESTS_MAX;
      }

      updateDayAndNightInfo(true /* update_everything */);
   }
   else if (cInitialLatLongRequestsRemaining > 0)
   {
      //  graphics_night_layer_update_callback() will handle the first request
      //  but its up to us to send any additional ones.
      cInitialLatLongRequestsRemaining --;
      app_msg_RequestLatLong();
   }
#endif

   // Need to be static because they're used by the system later.
   static char time_text[] = "00:00";
   static char dow_text[] = "xxx";
   static char mon_text[14];

   strftime(dow_text, sizeof(dow_text), "%a", tick_time);
   strftime(mon_text, sizeof(mon_text), "%b %e, %Y", tick_time);

   clock_copy_time_string(time_text, sizeof(time_text));
   if (!clock_is_24h_style() && (time_text[0] == '0'))
   {
      memmove(time_text, &time_text[1], sizeof(time_text) - 1);
   }

#ifndef PBL_ROUND
   text_layer_set_text(pDayOfWeekLayer, dow_text);
#endif
   text_layer_set_text(pMonthLayer, mon_text);

   text_layer_set_background_color(pTextTimeLayer, GColorClear);
   text_layer_set_text(pTextTimeLayer, time_text);
   text_layer_set_text_alignment(pTextTimeLayer, GTextAlignmentCenter);

   //  update hour hand position
   int32_t hour_angle = TRIG_MAX_ANGLE * get24HourAngle(tick_time->tm_hour,
                                                        tick_time->tm_min);

   hour_hand_set_angle(hour_angle);

#if HOUR_HAND_USE_PATH
   //  set hour hand's appearance based on whether it is over the night
   //  part of the background
   hour_hand_set_is_night(is_dark_time(tick_time->tm_hour, tick_time->tm_min));
#endif

   //  oddly we seem to need to explicitly mark our base window layer dirty,
   //  or else old time values will stack on top each other
   layer_mark_dirty(pGraphicsNightLayer);

// Vibrate Every Hour
#if HOUR_VIBRATION

   if ((t->tick_time->tm_min == 0) && (t->tick_time->tm_sec == 0))
   {
      vibes_enqueue_custom_pattern(hour_pattern);
   }
#endif

   updateDayAndNightInfo(false);

}  /* end of handle_minute_tick() */


static void  sunclock_window_free_all_memory()
{


   tick_timer_service_unsubscribe();

#ifndef PBL_ROUND
   SAFE_DESTROY(text_layer, pTextSunsetLayer);
   SAFE_DESTROY(text_layer, pTextSunriseLayer);
   SAFE_DESTROY(text_layer, pDayOfWeekLayer);
#endif
   SAFE_DESTROY(text_layer, pMonthLayer);
   SAFE_DESTROY(text_layer, pMoonLayer);
   SAFE_DESTROY(text_layer, pTextTimeLayer);

   //  actually our main window's base layer, so don't nuke it.
//   SAFE_DESTROY(layer,      pGraphicsNightLayer);

#ifdef PBL_PLATFORM_APLITE
   transbitmap_destroy(pTransBmpWatchface);
   pTransBmpWatchface = 0;
#endif

   hour_hand_deinit();

   SAFE_DESTROY(twilight_path, pTwiPathNight);
   SAFE_DESTROY(twilight_path, pTwiPathAstro);
   SAFE_DESTROY(twilight_path, pTwiPathNautical);
   SAFE_DESTROY(twilight_path, pTwiPathCivil);

}  /* end of sunclock_window_free_all_memory() */


/**
 *  Report a heap failure, try to free enough memory to put up an error message.
 */
void  mark_heap_failure()
{

   s_fHeapFailure = true;

   layer_mark_dirty(pGraphicsNightLayer);

   sunclock_window_free_all_memory();

   return;

}

/**
 *  Do GUI layout for already-created window, and cache all needed resources.
 *  Also register a tick handler, initialize watch/phone messaging, and request
 *  current location data from the phone.
 */
static void  sunclock_window_load(Window * pMyWindow) 
{

   window_set_background_color(pWindow, GColorWhite);

   pFontMoon = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_MOON_PHASES_SUBSET_30));

#if USE_FONT_RESOURCE
   pFontCurTime = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_42));
#else
   pFontCurTime = fonts_get_system_font(FONT_KEY_DROID_SERIF_28_BOLD);
#endif

   pFontMediumText = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_19));

   //  used for main display on non-round watches, and message window on all watches:
   pFontSmallText = fonts_get_system_font(FONT_KEY_GOTHIC_18);


   //  The v2 SDK docs suggest that we should do our base bitmap
   //  graphics directly in the window root layer, rather than creating a
   //  separate layer just for the bitmaps (& not using the base window's layer).

#ifdef PBL_COLOR
   window_set_background_color(pWindow, TWI_COLOR_NIGHT);
#endif
   pGraphicsNightLayer = window_get_root_layer(pWindow);
   if (pGraphicsNightLayer == NULL)
   {
      return;
   }

   layer_set_update_proc(pGraphicsNightLayer, graphics_night_layer_update_callback); 


#ifdef PBL_PLATFORM_APLITE
   pTransBmpWatchface = transbitmap_create_with_resource_prefix(RESOURCE_ID_IMAGE_WATCHFACE);
   if (pTransBmpWatchface == NULL)
   {
      mark_heap_failure();
      return;
   }
#endif

   //  Yes, the apparent mismatch between ZENITH_ names and TwilightPath instance
   //  names is intended (if a bit unfortunate).
   pTwiPathNight    = twilight_path_create(ZENITH_ASTRONOMICAL, ENCLOSE_SCREEN_BOTTOM,
                                           TWI_APLITE_RES_ONLY(INVALID_RESOURCE));
   pTwiPathAstro    = twilight_path_create(ZENITH_NAUTICAL,     ENCLOSE_SCREEN_TOP,
                                           TWI_APLITE_RES_ONLY(RESOURCE_ID_IMAGE_DARK_GREY));
   pTwiPathNautical = twilight_path_create(ZENITH_CIVIL,        ENCLOSE_SCREEN_TOP,
                                           TWI_APLITE_RES_ONLY(RESOURCE_ID_IMAGE_GREY));
   pTwiPathCivil    = twilight_path_create(ZENITH_OFFICIAL,     ENCLOSE_SCREEN_TOP,
                                           TWI_APLITE_RES_ONLY(RESOURCE_ID_IMAGE_LIGHT_GREY));
   if ((pTwiPathNight == NULL) || (pTwiPathAstro == NULL) ||
       (pTwiPathNautical == NULL) || (pTwiPathCivil == NULL))
   {
      mark_heap_failure();
      return;
   }

   //  time of day text
   pTextTimeLayer = text_layer_create(GRect(0, TEXT_TIME_Y, DISP_WIDTH, 42));
   if (pTextTimeLayer == NULL)
   {
      mark_heap_failure();
      return;
   }
   text_layer_set_text_color(pTextTimeLayer, GColorBlack);
   text_layer_set_background_color(pTextTimeLayer, GColorClear);
   text_layer_set_font(pTextTimeLayer, pFontCurTime);
   layer_add_child(window_get_root_layer(pWindow),
                   text_layer_get_layer(pTextTimeLayer));

   //  (text grows down, so just make window run all the way to the bottom)
   pMoonLayer = text_layer_create(GRect(0, TEXT_MOON_Y, DISP_WIDTH, DISP_HEIGHT - TEXT_MOON_Y));
   if (pMoonLayer == NULL)
   {
      mark_heap_failure();
      return;
   }
   text_layer_set_text_color(pMoonLayer, GColorWhite);
   text_layer_set_background_color(pMoonLayer, GColorClear);
   text_layer_set_font(pMoonLayer, pFontMoon);
   text_layer_set_text_alignment(pMoonLayer, GTextAlignmentCenter);
   layer_add_child(window_get_root_layer(pWindow),
                   text_layer_get_layer(pMoonLayer));

   //  Add hour hand after moon phase:  looks weird (wrong) to see phase
   //  on top of the hour hand.
   hour_hand_init (pWindow);

   //  Same rectangle used for day of week and date text:
   //  text alignment avoids conflicts in the two layers.
   GRect DayDateTextRect;
   DayDateTextRect = GRect(0, TEXT_DATE_Y, DISP_WIDTH, 127 + 26);

#ifndef PBL_ROUND

   //Day of Week text
   pDayOfWeekLayer = text_layer_create(DayDateTextRect);
   if (pDayOfWeekLayer == NULL)
   {
      mark_heap_failure();
      return;
   }
   text_layer_set_text_color(pDayOfWeekLayer, GColorWhite);
   text_layer_set_background_color(pDayOfWeekLayer, GColorClear);
   text_layer_set_font(pDayOfWeekLayer, pFontMediumText);
   text_layer_set_text_alignment(pDayOfWeekLayer, GTextAlignmentLeft);
   text_layer_set_text(pDayOfWeekLayer, "xxx");
   layer_add_child(window_get_root_layer(pWindow),
                   text_layer_get_layer(pDayOfWeekLayer));

#endif  // #ifndef PBL_ROUND

   //Month Text
   pMonthLayer = text_layer_create(DayDateTextRect);
   if (pMonthLayer == NULL)
   {
      mark_heap_failure();
      return;
   }
#ifndef PBL_ROUND
   text_layer_set_text_color(pMonthLayer, GColorWhite);
   text_layer_set_text_alignment(pMonthLayer, GTextAlignmentRight);
#else
   text_layer_set_text_color(pMonthLayer, GColorBlack);
   //  no day-of-week, so center date
   text_layer_set_text_alignment(pMonthLayer, GTextAlignmentCenter);
#endif
   text_layer_set_background_color(pMonthLayer, GColorClear);
   text_layer_set_font(pMonthLayer, pFontMediumText);
   text_layer_set_text(pMonthLayer, "xxx");
   layer_add_child(window_get_root_layer(pWindow),
                   text_layer_get_layer(pMonthLayer));

#ifndef PBL_ROUND

   //  Same rectangle used for sunrise / sunset text layers: updateDayAndNightInfo()
   //  changes sunset text to right-aligned.
   GRect SunRiseSetTextRect;
   SunRiseSetTextRect = GRect(0, 147, DISP_WIDTH, 30);

   pTextSunriseLayer = text_layer_create(SunRiseSetTextRect);
   if (pTextSunriseLayer == NULL)
   {
      mark_heap_failure();
      return;
   }
   text_layer_set_text_color(pTextSunriseLayer, GColorWhite);
   text_layer_set_background_color(pTextSunriseLayer, GColorClear);
   text_layer_set_font(pTextSunriseLayer, pFontSmallText);
   layer_add_child(window_get_root_layer(pWindow),
                   text_layer_get_layer(pTextSunriseLayer));

   pTextSunsetLayer = text_layer_create(SunRiseSetTextRect);
   if (pTextSunsetLayer == NULL)
   {
      mark_heap_failure();
      return;
   }
   text_layer_set_text_color(pTextSunsetLayer, GColorWhite);
   text_layer_set_background_color(pTextSunsetLayer, GColorClear);
   text_layer_set_font(pTextSunsetLayer, pFontSmallText);
   layer_add_child(window_get_root_layer(pWindow),
                   text_layer_get_layer(pTextSunsetLayer));

#endif  // #ifndef PBL_ROUND

   //  Run initial tick processing before our window displays, so that all
   //  text fields are populated initially.
   time_t timeNow = time(NULL);
   struct tm * pLocalTime = localtime(&timeNow);

   handle_minute_tick(pLocalTime, MINUTE_UNIT);

   //  [Don't do location data load until our message pump is running.]
//   app_msg_RequestLatLong();

   tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);

}  /* end of sunclock_window_load */


/** 
 *  Release all resources and registrations allocated by
 *  \ref sunclock_window_load().
 */
static void  sunclock_window_unload(Window * pMyWindow)
{


   //BUGBUG: pWindow == pMyWindow ??

   fMessagePumpRunning = false;

   sunclock_window_free_all_memory();

}  /* end of sunclock_window_unload() */


void sunclock_coords_recvd(float latitude, float longitude, int32_t utcOffset)
{

   MY_APP_LOG(APP_LOG_LEVEL_DEBUG, "got coords, utcOff=%d", (int) utcOffset);

   if (config_data_is_different(latitude, longitude, utcOffset))
   {
      config_data_location_set(latitude, longitude, utcOffset); 

      time_t timeNow = time(NULL);
      struct tm * pLocalTime = localtime(&timeNow);
      handle_minute_tick(pLocalTime, MINUTE_UNIT);

      //  we likely also need force a full recompute, since handle_minute_tick()'s
      //  tz check may well not have fired.  Battery, ouch.  But only on initial
      //  config set from phone.
      updateDayAndNightInfo(true /* update_everything */);
   }

}  /* end of sunclock_coords_recvd */


/**
 *  Create base watchface window.  We're called outside of the event loop,
 *  so we do as little as possible here.
 */
void  sunclock_handle_init()
{


   pWindow = window_create();
   if (pWindow == NULL)
   {
      // oh well.
      return;
   }

   //  Defer the bulk of our start up a load handler.  In particular, it seems
   //  better to do app_message stuff there, once our event loop is running.
   window_set_window_handlers(pWindow,
                              (WindowHandlers) {
                                 .load = sunclock_window_load,
                                 .unload = sunclock_window_unload,
                               });

   window_stack_push(pWindow, true /* Animated */);

   if ((! config_data_location_avail()) && (! s_fHeapFailure))
   {
      //  get message window in front of ours right away, even though it has no content yet
      message_window_show_status("Missing Config", "No location data");
   }

}  /* end of sunclock_handle_init() */

/**
 *  Called at application shutdown / exit.  Releases all dynamic storage
 *  allocated by \href handle_init() et al.
 *  
 *  NB: all except for our main window, since deleting it seems to provoke
 *      a Pebble "crash" report when our face exits.
 */
void sunclock_handle_deinit()
{

//   It appears that, despite what Pebble's guide says
//     http://developer.getpebble.com/guides/pebble-apps/app-structure/windows
//   it is _still_ (in SDK v2.8) not safe to destroy our own main window at exit.
//   In fact, with SDK v2.8 (maybe starting with v2.1?) attempting to destroy
//   our window when leaving the watchface via (at least) an up/down "scroll"
//   button press results in an "app crashed" message.  Humbug.
// 
//   SAFE_DESTROY(window, pWindow);    // uncomment to cause app crash @ exit

   //  Do these here since they're shared with another app window.
   //  The SDK hints that the window unload function might be called
   //  before window destruction, in future SDK releases.
   fonts_unload_custom_font(pFontMediumText);
   fonts_unload_custom_font(pFontMoon);
#if USE_FONT_RESOURCE
   fonts_unload_custom_font(pFontCurTime);
#endif

}  /* end of sunclock_handle_deinit */

