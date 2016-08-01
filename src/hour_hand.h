/**
 *  @file
 *  
 *  Public hour hand methods.  Separate implementations exist
 *  for bitmap- and path-based approaches.
 */

#ifndef hour_hand_h__
#define hour_hand_h__



void  hour_hand_init (const Window *window);

void  hour_hand_deinit();

void  hour_hand_set_angle(int32_t hour_angle);


//  For now, we use the original bitmap approach on aplite,
//  and path on basalt (anti-aliasing is nice there).

#ifdef PBL_PLATFORM_APLITE

# define HOUR_HAND_USE_PATH 0
# define HOUR_HAND_USE_BITMAP 1

#elif PBL_PLATFORM_BASALT || PBL_PLATFORM_CHALK

# define HOUR_HAND_USE_PATH 1
# define HOUR_HAND_USE_BITMAP 0

#endif


#if HOUR_HAND_USE_PATH
/**
 *  Set hour hand's appearance based on whether it is over the night part
 *  of the background.
 */
void  hour_hand_set_is_night (bool fIsNightNow);
#endif


#endif  // #ifndef hour_hand_h__

