/**
 *  @file
 *  
 */


#include  <pebble.h>

#include  "ConfigData.h"
#include  "messaging.h"
#include  "MessageWindow.h"
#include  "platform.h"
#include  "sunclock.h"



/**
 *  Where coords data initially comes when received from the phone.
 *  We check system state and update with coords as needed.
 * 
 * @param latitude 
 * @param longitude 
 * @param utcOffset Seconds to add to watch's time() value in order to obtain UTC.
 */
void coords_recvd_callback(float latitude, float longitude, int32_t utcOffset)
{

   //  Got data now, so if we had an error / search message window up,
   //  can get rid of it.
   message_window_close();

   //  Pass data on to main watchface window.
   sunclock_coords_recvd(latitude, longitude, utcOffset);

}  /* end of coords_recvd_callback */


void coords_failed_callback (FailureSource eErrSrc,
                             int32_t errCode, const char *pszErrMsg)
{

   MY_APP_LOG(APP_LOG_LEVEL_DEBUG, "coords failure, src=%d, err=%d, msg=\"%s\"",
           (int) eErrSrc, (int) errCode, pszErrMsg);

   if (config_data_location_avail())
   {
      //  already have locally persisted location info, so silently ignore this.
      return;
   }

   message_window_show_error(eErrSrc, errCode, pszErrMsg);

}  /* end of coords_failed_callback */


int  main()
{

   //  make sure config data can be read before setting up main window
   config_data_init();

   //  want to have messaging up for whichever window needs it.
   app_msg_init(coords_recvd_callback, coords_failed_callback);

   sunclock_handle_init();

   //  NB: for iOS it may be important to not block application execution
   //      before hitting the main event loop.  So we now defer the check
   //      for available location data until we hit our first ("nighttime")
   //      layer's drawing routine.  No better "message pump running" proxy
   //      that I'm aware of.
   app_event_loop(); 

   app_msg_deinit();

   sunclock_handle_deinit();

}  /* end of main() */

