/**
 *  @file
 *  
 *  Contains routines to support drawing the watchface dial's mask
 *  (twilight band clipping, hour mark placement, etc.) using Pebble
 *  graphics routines instead of a bitmap overlap.
 *  
 *  Using graphics routines allows us to tolerate different display
 *  resoluions from the original Aplite, and it is hoped will also
 *  allow for some degree of anti-aliasing on > 1 bit pixel displays.
 */


#include "pebble.h"

#include "geometry.h"
#include "sunclock.h"


#if ! PBL_PLATFORM_APLITE


/**
 *  Visible radial length of large hour hand mark, for use at 90 degree positions.
 */
#define  HOUR_MARK1_RADIAL_LENGTH   (DISP_WIDTH / 11)

/**
 *  Half-thickness of large hour hand mark, for use at 90 degree positions.
 *  "Half-" so that mark's path is symmetrical about its radial centerline.
 */
#define  HOUR_MARK1_HALF_WIDTH      (DISP_WIDTH / 48)     /* 3 in basalt */

///  Starting (inner) position of large hour mark, distance from center of face.
#define  HOUR_MARK1_INNER_RADIUS    (USABLE_FACE_RADIUS - HOUR_MARK1_RADIAL_LENGTH)

/**
 *  Ending (outer) position of large hour mark, distance from center of face.
 *  Deliberately over-size to be smoothly clipped by face edge.
 */
#define  HOUR_MARK1_OUTER_RADIUS    (9 * DISP_WIDTH / 16)


#define  HOUR_MARK2_RADIAL_LENGTH   (DISP_WIDTH / 16)
#define  HOUR_MARK2_HALF_WIDTH      (DISP_WIDTH / 72)
#define  HOUR_MARK2_INNER_RADIUS    (USABLE_FACE_RADIUS - HOUR_MARK2_RADIAL_LENGTH)
#define  HOUR_MARK2_OUTER_RADIUS    (HOUR_MARK1_OUTER_RADIUS)


#define  HOUR_MARK3_HALF_WIDTH      (HOUR_MARK2_HALF_WIDTH)
#define  HOUR_MARK3_INNER_RADIUS    (USABLE_FACE_RADIUS - HOUR_MARK3_HALF_WIDTH)


///  Large hour hand mark, for use at 90 degree positions.  This one is naturally at zero deg.
///  Points given in order so winding direction is clockwise.
static const struct GPathInfo LargeHourMarkPathInfo =
  { 4,
    (GPoint []) {
      { HOUR_MARK1_INNER_RADIUS,  HOUR_MARK1_HALF_WIDTH },
      { HOUR_MARK1_INNER_RADIUS, -HOUR_MARK1_HALF_WIDTH },
      { HOUR_MARK1_OUTER_RADIUS, -HOUR_MARK1_HALF_WIDTH },
      { HOUR_MARK1_OUTER_RADIUS,  HOUR_MARK1_HALF_WIDTH }
    }
  };

///  Medium hour mark, for mod(3) hours which don't get a large mark
static const struct GPathInfo MediumHourMarkPathInfo =
  { 4,
    (GPoint []) {
      { HOUR_MARK2_INNER_RADIUS,  HOUR_MARK2_HALF_WIDTH },
      { HOUR_MARK2_INNER_RADIUS, -HOUR_MARK2_HALF_WIDTH },
      { HOUR_MARK2_OUTER_RADIUS, -HOUR_MARK2_HALF_WIDTH },
      { HOUR_MARK2_OUTER_RADIUS,  HOUR_MARK2_HALF_WIDTH }
    }
  };

///  grect_center_point() of watchface dial's enclosing square
GPoint dialHub;


void  get_contrasting_colors(int localHour, GColor *pColorFill, GColor *pColorOutline)
{

   //BUGBUG - this check is backwards but works, what's wrong where??
   //         (is_dark_time() works properly for hour hand)
   if (! is_dark_time(localHour, 0))
      {
      *pColorFill = GColorDarkGray;
      *pColorOutline = GColorWhite;
      }
   else
      {
      *pColorFill = GColorBlack;
      *pColorOutline = GColorBlack;
      }

}


/**
 *  Draw a rotated, filled path which contrasts with its background.
 *  Rotation angle is expressed as an hour (only) value, since this is
 *  intended for use with watchface hour marks.
 *  
 *  Path's rotation and context's stroke / fill values are altered.
 *  
 *  @param ctx Graphics context to draw into.
 *  @param localHour Integer hour value, 0 .. 23.
 *  @param path Path to rotate and fill / stroke.
 */
void  fill_contrasting_path(GContext *ctx, int localHour, GPath *pPath)
{

   //  correct from hour 0 == top to degree 0 == right:
   int32_t hourTrigAngle = (localHour - 6) * TRIG_MAX_ANGLE / 24;

   GColor  colorFill;
   GColor  colorOutline;

   get_contrasting_colors(localHour, &colorFill, &colorOutline);

   gpath_rotate_to(pPath, hourTrigAngle);

   graphics_context_set_fill_color(ctx, colorFill);
   gpath_draw_filled(ctx, pPath);

   graphics_context_set_stroke_color(ctx, colorOutline);
   gpath_draw_outline(ctx, pPath);

   return;

}  /* end of fill_contrasting_path() */


void  draw_small_hour_mark (GContext *ctx, int localHour)
{

   //  correct from hour 0 == top to degree 0 == right:
   int32_t hourTrigAngle = (localHour - 6) * TRIG_MAX_ANGLE / 24;

   GPoint hourMarkCenter;

   //  figure out point from hub and location radius.

   hourMarkCenter.x = (int16_t)(cos_lookup(hourTrigAngle) *
                                (int32_t)HOUR_MARK3_INNER_RADIUS / TRIG_MAX_RATIO) + dialHub.x;
   hourMarkCenter.y = (int16_t)(sin_lookup(hourTrigAngle) *
                                (int32_t)HOUR_MARK3_INNER_RADIUS / TRIG_MAX_RATIO) + dialHub.y;

   GColor  colorFill;
   GColor  colorOutline;

   get_contrasting_colors(localHour, &colorFill, &colorOutline);

   graphics_context_set_fill_color(ctx, colorFill);
   graphics_fill_circle(ctx, hourMarkCenter, HOUR_MARK3_HALF_WIDTH);

   graphics_context_set_stroke_color(ctx, colorOutline);
   graphics_draw_circle(ctx, hourMarkCenter, HOUR_MARK3_HALF_WIDTH);

}  /* end of draw_small_hour_mark */


/**
 *  Routine called after twilight bands have been rendered, to trim them
 *  and add a "watch face" circle & hour marks.
 *  
 *  In addition to giving us support for multiple display resolutions,
 *  this hopefully lets our face detail benefit from anti-aliasing.
 */
void  draw_watchface_mask(GContext *ctx, GRect layerFrame)
{

//void grect_align(GRect *rect, const GRect *inside_rect, const GAlign alignment, const bool clip);

   //  do radial fill into a deliberately over-sized rectangle to ensure that
   //  screen corners etc. are cleared as well.  Find a single dimension which
   //  is used as both x and y overlap size.
   int  maxSize = layerFrame.size.h;

   GRect maskRect = GRect(layerFrame.origin.x - maxSize,
                          layerFrame.origin.y - maxSize + FACE_VOFFSET,
                          layerFrame.size.w + 2*maxSize,
                          layerFrame.size.h + 2*maxSize);

   dialHub = grect_center_point(&maskRect);

   // . . . insert drawing of hour markers here, a little oversized so they are cropped
   // . . . by the mask we then draw.

   GPath * pLargeHourMarkPath  = gpath_create(&LargeHourMarkPathInfo);
   GPath * pMediumHourMarkPath = gpath_create(&MediumHourMarkPathInfo);
   gpath_move_to(pLargeHourMarkPath,  dialHub);
   gpath_move_to(pMediumHourMarkPath, dialHub);

   for (int hour = 0;  hour < 24;  hour += 6)
      {
      fill_contrasting_path(ctx,  hour,   pLargeHourMarkPath);
      draw_small_hour_mark (ctx,  hour+1);
      draw_small_hour_mark (ctx,  hour+2);
      fill_contrasting_path(ctx,  hour+3, pMediumHourMarkPath);
      draw_small_hour_mark (ctx,  hour+4);
      draw_small_hour_mark (ctx,  hour+5);
      }


   gpath_destroy(pLargeHourMarkPath);
   gpath_destroy(pMediumHourMarkPath);


   graphics_context_set_fill_color(ctx, GColorBlack);

   graphics_fill_radial(ctx, maskRect, GOvalScaleModeFitCircle,
                        maxSize + FACE_EDGE_INSET,  /* uint16_t inset_thickness */
                        DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));

   int minSize     = layerFrame.size.w;
   int minSizeHalf = minSize / 2;

   GRect dialRect = GRect(dialHub.x - minSizeHalf, dialHub.y - minSizeHalf,
                          minSize, minSize);

   graphics_context_set_fill_color(ctx, GColorWhite);

   graphics_fill_radial(ctx, dialRect, GOvalScaleModeFitCircle,
                        FACE_EDGE_INSET,  /* uint16_t inset_thickness */
                        DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));

   graphics_context_set_stroke_color(ctx, GColorBlack);
   graphics_draw_circle(ctx, dialHub, minSizeHalf - 1);

   // . . . when all done with masked twilight bands, use
   //         graphics_capture_frame_buffer()
   // . . . to capture to final base image and then at each remaining minute
   // . . . of the same day, can blit it back using
   //         graphics_draw_bitmap_in_rect()
   //   need to figure out how big captured frame_buffer's bitmap is
   //   in order to save a copy of it in malloc()ed memory!

   return;

}  /* end of draw_watchface_mask */

#endif  // #if ! PBL_PLATFORM_APLITE

