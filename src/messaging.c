/**
 *  @file
 *  
 */


#include  "messaging.h"

#include  "platform.h"
#include  "testing.h"

#include  <pebble.h>


//  this is ridiculous, but it doesn't seem to be available otherwise:
#define	min(a,b) (((a)<(b))?(a):(b))


static app_msg_coords_recvd_callback  coords_recvd_callback = 0;

static app_msg_coords_failed_callback coords_failed_callback = 0;


///  When a request is already outstanding, another one will be ignored.
///  [Curiously, that volatile qualifier seems to be needed when replies
///   get stacked up.]
static volatile bool  fRequestOutstanding = false;

///  When the currently outstanding request was submitted.
static time_t timeRequestSubmitted;

///  Max elapsed time before we give up on retries for a request.
#define  RETRY_TIMEOUT  20  /* time_t == seconds */

///  "Safe" copy of error message in failure tuple received from phone.
static char achErrMessage[64];


static bool  app_msg_RequestLatLong_internal(void)
{

   bool fMyRet = true;

#if TESTING_DISABLE_LOCATION_REQUEST
   return true;
#endif

   Tuplet fetch_tuple = TupletInteger(MSG_KEY_GET_LAT_LONG, 1); 

   DictionaryIterator *iter;
   AppMessageResult amRet;

   amRet = app_message_outbox_begin(&iter);

   if (amRet != APP_MSG_OK)
   {
      MY_APP_LOG(APP_LOG_LEVEL_DEBUG, "app_message_outbox_begin failed, ret = %04X", amRet);
      return false;
   }

   if (iter == NULL)
   {
      MY_APP_LOG(APP_LOG_LEVEL_DEBUG, "app_message_outbox_begin returned null iter");
      return false;
   }

   DictionaryResult dRet;

   dRet = dict_write_tuplet(iter, &fetch_tuple);
   if (dRet != DICT_OK)
   {
      MY_APP_LOG(APP_LOG_LEVEL_DEBUG, "dict_write_tuplet failed, ret = %04X", dRet);
      fMyRet = false;
      //  fall through to call end anyway
   }

   unsigned uRet;

   uRet = dict_write_end(iter);
   if (uRet == 0)
   {
      fMyRet = false;
   }

   if (fMyRet)
   {
      amRet = app_message_outbox_send();
      if (amRet != APP_MSG_OK)
      {
         MY_APP_LOG(APP_LOG_LEVEL_DEBUG, "app_message_outbox_send failed, ret = %04X", amRet);
         fMyRet = false;
      }
   }

   return fMyRet;

}  /* end of app_msg_RequestLatLong_internal(void) */


bool  app_msg_RequestLatLong(void)
{
   if (fRequestOutstanding)
   {
      //  one still running, so pretend subsequent is submitted.
      return true;
   }

   fRequestOutstanding = true;
   timeRequestSubmitted = time(NULL);
   return app_msg_RequestLatLong_internal();
}


/** 
 *  Callback function notified by the app_message_* Pebble subsystem
 *  when the watch has received a message from the phone.
 *  See if the message has values which we are interested in, and if so
 *  pass them along to the callback supplied to us via app_msg_init().
 * 
 *  @param iter 
 *  @param context 
 */
static void in_received_handler(DictionaryIterator *iter, void *context)
{
   Tuple *lat_tuple = dict_find(iter, MSG_KEY_LATITUDE);
   Tuple *long_tuple = dict_find(iter, MSG_KEY_LONGITUDE);
   Tuple *utcOff_tuple = dict_find(iter, MSG_KEY_UTC_OFFSET);
   Tuple *errCode_tuple = 0;
   Tuple *errMsg_tuple = 0;

   if ((lat_tuple != 0) && (long_tuple != 0) && (utcOff_tuple != 0))
   {
      fRequestOutstanding = false;

      //  Our coordinates come back as scaled integer degrees, where the original
      //  float values have been multiplied by 1000000 and then rounded.
      //  We convert them back to float since that is what the app needs.

      if (coords_recvd_callback != 0)
      {
         float fLat = lat_tuple->value->int32;
         fLat /= 1000000;
         float fLong = long_tuple->value->int32;
         fLong /= 1000000;

         (*coords_recvd_callback)(fLat, fLong, utcOff_tuple->value->int32); 
      }
   }
   else
   {
      errCode_tuple = dict_find(iter, MSG_KEY_FAIL_CODE);
      errMsg_tuple = dict_find(iter, MSG_KEY_FAIL_MESSAGE);

      if ((errCode_tuple != 0) && (errMsg_tuple != 0))
      {
         strncpy(achErrMessage, errMsg_tuple->value->cstring, sizeof(achErrMessage));

         //  relay phone-reported error to requestor:
         (*coords_failed_callback)(FAIL_SRC_PHONE,
                                   errCode_tuple->value->uint32, achErrMessage);
      }
   }
}


static void in_dropped_handler(AppMessageResult reason, void *context)
{
   MY_APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Dropped!  reason = %04X", reason);
}


static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context)
{
   static int failCount = 0;
   failCount ++;
   MY_APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Failed to Send! [%d]  reason = %04X",
           failCount, reason);
   if (fRequestOutstanding)
   {
      if (reason == APP_MSG_SEND_TIMEOUT)
      {
         if ((time(NULL) - timeRequestSubmitted) < RETRY_TIMEOUT)
         {
            //  Worth trying again, at least a few times.  It seems that, perhaps
            //  especially during watch/phone app startup, app_message_* is lossy.
            //  [Then other times, we seem to get multiple (2?) requests stacked up
            //   and keep getting answers back after we stop asking.  This is tidied
            //   via fRequestOutstanding.]
            app_msg_RequestLatLong_internal();
         }
         else
         {
            fRequestOutstanding = false;

            //  Let requestor know about watch/phone comms failure:
            (*coords_failed_callback)(FAIL_SRC_APP_MSG,
                                      01, "Send retry timeout");
         }
      }
      else
      {
         const char * pszReason;

         //  Let requestor know about watch/phone comms failure:
         switch (reason)
         {
            //! The other end did not confirm receiving the sent data with an (n)ack in time.
            case APP_MSG_SEND_TIMEOUT:
               pszReason = "send timeout";
               break;

            //! The other end rejected the sent data, with a "nack" reply.
            case APP_MSG_SEND_REJECTED:
               pszReason = "send rejected";
               break;

            //! The other end was not connected.
            case APP_MSG_NOT_CONNECTED:
               pszReason = "not connected";
               break;

            //! The local application was not running.
            case APP_MSG_APP_NOT_RUNNING:
               pszReason = "app not running";
               break;

            //! The function was called with invalid arguments.
            case APP_MSG_INVALID_ARGS:
               pszReason = "invalid args";
               break;

            //! There are pending (in or outbound) messages that need to be processed first before
            //! new ones can be received or sent.
            case APP_MSG_BUSY:
               pszReason = "comms busy";
               break;

            //! The buffer was too small to contain the incoming message.
            case APP_MSG_BUFFER_OVERFLOW:
               pszReason = "rx buffer overflow";
               break;

            //! The resource had already been released.
            case APP_MSG_ALREADY_RELEASED:   // (does this ever apply here?)
               pszReason = "resource already released";
               break;

            //! The support library did not have sufficient application memory to perform the requested operation.
            case APP_MSG_OUT_OF_MEMORY:
               pszReason = "out of memory";
               break;

            //! App message was closed
            case APP_MSG_CLOSED:
               pszReason = "comms closed";
               break;

            //! An internal OS error prevented APP_MSG from completing an operation
            case APP_MSG_INTERNAL_ERROR:
               pszReason = "internal OS error";
               break;

            default:
               pszReason = "unknown, see int code";
         }

         (*coords_failed_callback)(FAIL_SRC_APP_MSG, reason, pszReason);
      }
   }
}


void  app_msg_init(app_msg_coords_recvd_callback successCallback,
                   app_msg_coords_failed_callback failureCallback)
{

   //  Hook in caller's callback, before it might possibly be called.
   coords_recvd_callback = successCallback;
   coords_failed_callback = failureCallback;

   fRequestOutstanding = false;

   // Register PebbleOS message handlers
   app_message_register_inbox_received(in_received_handler);
   app_message_register_inbox_dropped(in_dropped_handler);
   app_message_register_outbox_failed(out_failed_handler);

   // Init buffers

   //  Pebble's current minima are larger than we need, and using the larger
   //  values may cost heap we don't have.
   app_message_open(min(64, APP_MESSAGE_INBOX_SIZE_MINIMUM),
                    min(64, APP_MESSAGE_OUTBOX_SIZE_MINIMUM));

   //  too early here: better for caller to explicitly request from window_load()
//   app_msg_RequestLatLong();

}  /* end of app_msg_init */


void app_msg_deinit(void)
{
   //  not done in the quotes.c example, but seems like it might be worthwhile:
   app_message_deregister_callbacks();

   coords_recvd_callback = 0;

   fRequestOutstanding = false;

}
