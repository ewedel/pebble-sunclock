/**
 *  @file
 *  
 *  Screen size settings to accomodate traditional plus Round platforms.
 */

#ifndef geometry_h__
#define geometry_h__


#ifdef PBL_ROUND

//  Pebble Time Round ("chalk" has a circular display nominal diameter
//  of 180, but Pebble warns that the outer 2 pixels or so may be
//  obscured by the bezel (!).  So we mask them after writing the
//  twilight bands & other face background.

#define DISP_WIDTH   180
#define DISP_HEIGHT  180

///  Usable outer diameter of face display area, loses 2 pixels from radius.
#define FACE_OD      (DISP_WIDTH - 4)

#define FACE_VOFFSET  0

//  Distance from edge of screen to edge of dial outline, at 90 degree positions
#define FACE_EDGE_INSET  4

#define HOUR_HAND_HUB_RADIUS      9
#define HOUR_HAND_L_RADIUS       71
#define HOUR_HAND_S_RADIUS       15

#define TEXT_TIME_Y     16
#define TEXT_DATE_Y     56

#define TEXT_MOON_Y     117

//  also, since these vary with platform as well:

#define CHARGE_PCT_NOTICE         40
#define CHARGE_PCT_WARN           30
#define CHARGE_PCT_CRITICAL       20

#else  // #ifdef PBL_ROUND

//  At this time all other platforms use the original screen size
//     144 x 168
//  All pixels are visible / usable.

#define DISP_WIDTH   144
#define DISP_HEIGHT  168

///  Usable outer diameter of face display area.
#define FACE_OD      DISP_WIDTH

//  got extra vertical space, so leave room up top for date line
#define FACE_VOFFSET  11

#define FACE_EDGE_INSET  2

#define HOUR_HAND_HUB_RADIUS      7
#define HOUR_HAND_L_RADIUS       57
#define HOUR_HAND_S_RADIUS       12

#define TEXT_TIME_Y     36
#define TEXT_DATE_Y      0

#define TEXT_MOON_Y     113

#define CHARGE_PCT_NOTICE         30
#define CHARGE_PCT_WARN           20
#define CHARGE_PCT_CRITICAL       10

#endif  // #ifdef PBL_ROUND

#define FACE_CENTER_X  (DISP_WIDTH / 2)
#define FACE_CENTER_Y  (FACE_VOFFSET + DISP_HEIGHT / 2)

#define USABLE_FACE_RADIUS  (FACE_CENTER_X - FACE_EDGE_INSET)

///  Radius necessary to enclose even corners of square display, plus a little more.
#define FULL_DISP_RADIUS  (3 * DISP_WIDTH / 2)

#define HOUR_HAND_CHARGE_RADIUS   (HOUR_HAND_HUB_RADIUS - 2)


#endif  // #ifndef geometry_h__

