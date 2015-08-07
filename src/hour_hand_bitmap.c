/**
 *  @file
 *  
 *  Provides bitmap-based display of hour hand.
 *  
 *  This is the approach used since Pebble version 1.x, but in version 3.x
 *  appears quite aliased when compared to anti-aliasing on the rest of
 *  the display.
 */


#include  <pebble.h>

#include  "hour_hand.h"


#if HOUR_HAND_USE_BITMAP

#include  "TransRotBmp.h"


///  Hour hand bitmap, a transparent png which can rotate to any angle.
static TransRotBmp* pTransRotBmpHourHand = 0;


void  hour_hand_set_angle(int32_t hour_angle)
{

   transrotbmp_set_angle(pTransRotBmpHourHand, hour_angle);
   transrotbmp_set_pos_centered(pTransRotBmpHourHand, 0, 9 + 2);

}


void  hour_hand_init (const Window *window)
{

   Layer *window_layer = window_get_root_layer(window);

   pTransRotBmpHourHand = transrotbmp_create_with_resource_prefix(RESOURCE_ID_IMAGE_HOUR);
   if (pTransRotBmpHourHand == NULL)
   {
      return;
   }

   transrotbmp_set_src_ic(pTransRotBmpHourHand, GPoint(9, 56));
   transrotbmp_add_to_layer(pTransRotBmpHourHand, window_layer);

}


void  hour_hand_deinit()
{


   //  undo everything init() did

   transrotbmp_destroy(pTransRotBmpHourHand);
   pTransRotBmpHourHand = 0;

   return;

}


#endif  // #if HOUR_HAND_USE_BITMAP

