/**
 *  @file
 *  
 *  Provides path-based display of hour hand.
 *  
 *  This looks considerably smoother in Pebble 3.x where anti-aliasing
 *  of drawn figures is provided by default.
 */

#include  <pebble.h>

#include  "hour_hand.h"

#include  "geometry.h"
#include  "testing.h"


#if HOUR_HAND_USE_PATH


//  base constructs follow
//    https://github.com/pebble/pebble-sdk-examples/blob/master/watchfaces/feature_compass/src/compass.c
//  and
//    http://developer.getpebble.com/docs/c/Foundation/Event_Service/BatteryStateService/


//  "winding" direction is clockwise, per note documented in TwilightPath.h.
//  We offset our points.txt values (pixel coords from hour.png) so that the origin (0,0)
//  is the hour hand's axis point.

static const GPathInfo HOUR_HAND_POINTS = {
  6,
  (GPoint[]) { { -4,  HOUR_HAND_S_RADIUS },    // lower left
               { -5,   0 },    // bulge to left of axis
               { -3, -HOUR_HAND_L_RADIUS },    // upper left
               {  3, -HOUR_HAND_L_RADIUS },    // upper right
               {  5,   0 },    // bulge to right of axis
               {  4,  HOUR_HAND_S_RADIUS }     // bottom right
             }
};


static GPath *s_hour_hand_path;

static Layer *s_path_layer;

static GPoint s_center;    // axis of our hour hand, _not_ center of screen



//  Color values fed variously into GColorFromRGBA() / GColorFromRGB().
//  Each value is a byte, of which only the two most-significant bits
//  are used.  GColorFrom... then folds all of RGBA into a single byte.
//  NB:  until graphics_context_set_compositing_mode() can enable alpha
//       for fill operations, we leave alpha at full opacity.

#define   HR_HND_RGB_RED     0x0
#define   HR_HND_RGB_GREEN   0x0
#define   HR_HND_RGB_BLUE    0x0
#define   HR_HND_RBG_ALPHA   0xff      /* (fully opaque until testable) */

static BatteryChargeState  s_battery_charge = { 10, false, false };

static int32_t  s_hour_angle;

static bool  s_is_night_now;


void  hour_hand_set_angle(int32_t hour_angle)
{
   s_hour_angle = hour_angle;
//BUGBUG - when forcing positions for graphics testing:
//   s_hour_angle = 3*(TRIG_MAX_ANGLE / 4);
}


void  hour_hand_set_is_night (bool fIsNightNow)
{
   s_is_night_now = fIsNightNow;
}


void battery_State_Handler(BatteryChargeState charge)
{
   s_battery_charge = charge;
}


static void path_layer_update_callback(Layer *path_layer, GContext *ctx)
{


   //BUGBUG - as of SDK 3.2, no alpha support for graphics fill operations
   //         (path, circle, whatever).  Only bitmaps.  :-(
   graphics_context_set_compositing_mode(ctx, GCompOpSet);

   gpath_rotate_to(s_hour_hand_path, s_hour_angle);    // angular units?

// TODO:  revisit once we can get alpha fills in a gpath!
//   graphics_context_set_fill_color(ctx, GColorFromRGBA(HR_HND_RGB_RED,
//                                                       HR_HND_RGB_GREEN,
//                                                       HR_HND_RGB_BLUE,
//                                                       HR_HND_RBG_ALPHA));

   if (s_is_night_now)
   {
      graphics_context_set_fill_color(ctx, GColorDarkGray);
   }
   else
   {
      graphics_context_set_fill_color(ctx, GColorBlack);
   }

   gpath_draw_filled(ctx, s_hour_hand_path); 

   //  white outline seems to look better with both fill colors
   graphics_context_set_stroke_color(ctx, GColorWhite);

   gpath_draw_outline(ctx, s_hour_hand_path);

   //  draw stock black hub over axis / hour hand, regardless of hour hand color

   //  (same fill color as hour hand, except 100% opaque)
   graphics_context_set_fill_color(ctx, GColorFromRGB(HR_HND_RGB_RED,
                                                      HR_HND_RGB_GREEN,
                                                      HR_HND_RGB_BLUE));
   graphics_fill_circle(ctx, s_center, HOUR_HAND_HUB_RADIUS);
   graphics_draw_circle(ctx, s_center, HOUR_HAND_HUB_RADIUS);

   //  optionally write a smaller circle in center of hub, whose color reflects battery charge.
   //  Range of light yellow .. dark red suggested somewhere online, for another face.

   //  somehow interpolate from charge % in s_battery_charge to get maybe ten color "levels"?
   uint8_t charge_percent = s_battery_charge.charge_percent;
   GColor charge_color;

   if ((charge_percent <= CHARGE_PCT_CRITICAL) || TESTING_SHOW_LOW_BATTERY)
   {
      charge_color = GColorRed;
   }
   else if (charge_percent <= CHARGE_PCT_WARN)
   {
      charge_color = GColorOrange;
   }
   else if (charge_percent <= CHARGE_PCT_NOTICE)
   {
      charge_color = GColorChromeYellow;     // more orange than orange?  :-)
   }
   else
   {
      //  anything above 30%, just use our default hour hand color
      charge_color = GColorFromRGB(HR_HND_RGB_RED, HR_HND_RGB_GREEN, HR_HND_RGB_BLUE);
   }

   graphics_context_set_fill_color(ctx, charge_color);

   graphics_fill_circle(ctx, s_center, HOUR_HAND_CHARGE_RADIUS);

}


void  hour_hand_init (const Window *window)
{

   Layer *window_layer = window_get_root_layer(window);
   GRect bounds = layer_get_frame(window_layer);

   s_path_layer = layer_create(bounds);
  
   //  Define the draw callback to use for this layer
   layer_set_update_proc(s_path_layer, path_layer_update_callback);
   layer_add_child(window_layer, s_path_layer);

   // Initialize and define the two paths used to draw the needle to north and to south
   s_hour_hand_path = gpath_create(&HOUR_HAND_POINTS );

   //  hour hand axis: not quite the center of the screen
   s_center = GPoint(FACE_CENTER_X, FACE_CENTER_Y);
   gpath_move_to(s_hour_hand_path, s_center);

   // track current battery charge level
   s_battery_charge = battery_state_service_peek();
   battery_state_service_subscribe(battery_State_Handler);

}


void  hour_hand_deinit()
{


   //  undo everything init() did

   battery_state_service_unsubscribe();

   gpath_destroy(s_hour_hand_path);
   s_hour_hand_path = 0;

   layer_remove_from_parent(s_path_layer);
   layer_destroy(s_path_layer);
   s_path_layer = 0;

   return;

}


#endif  // #if HOUR_HAND_USE_PATH

