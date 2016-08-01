/**
 *  @file
 *  
 */


#include  <pebble.h>

#include  "MessageWindow.h"

#include  "geometry.h"
#include  "platform.h"
#include  "sunclock.h"        // for shared font resources


static Window * pMsgWindow = 0;
static TextLayer * pMsgText = 0;
static TextLayer * pCaption = 0;

static char achTextBuf[128];

#define CAPTION_Y_ORIGIN  (20)
#define TEXT_Y_ORIGIN     (DISP_HEIGHT / 3)


static void  message_window_load(Window *pWindow)
{

   pMsgText = text_layer_create((GRect) {.origin = { 0, TEXT_Y_ORIGIN },
                                           .size = { DISP_WIDTH, DISP_HEIGHT-TEXT_Y_ORIGIN } });
   pCaption = text_layer_create((GRect) {.origin = { 0, CAPTION_Y_ORIGIN },
                                           .size = { DISP_WIDTH, TEXT_Y_ORIGIN - CAPTION_Y_ORIGIN } });

   if ((pMsgText == NULL) || (pCaption == NULL))
   {
      return;
   }

   text_layer_set_font(pCaption, pFontMediumText);
   text_layer_set_font(pMsgText, pFontSmallText);
   
   text_layer_set_text_alignment(pCaption, GTextAlignmentCenter);
   text_layer_set_text_alignment(pMsgText, GTextAlignmentCenter);

   Layer *pRootLayer = window_get_root_layer(pWindow);

   layer_add_child (pRootLayer, text_layer_get_layer(pMsgText));
   layer_add_child (pRootLayer, text_layer_get_layer(pCaption));

#ifdef PBL_ROUND
   MY_APP_LOG(APP_LOG_LEVEL_DEBUG, "doing Round-specific text_flow settings");

   text_layer_enable_screen_text_flow_and_paging(pMsgText, 5);
   text_layer_enable_screen_text_flow_and_paging(pCaption, 5);
#endif

}  /* end of message_window_load */


static void  message_window_unload(Window *pWindow)
{

   layer_remove_child_layers(window_get_root_layer(pMsgWindow));

   text_layer_destroy(pMsgText);
   text_layer_destroy(pCaption);

}


static bool  ensure_message_window_pushed()
{

   if (pMsgWindow != 0)
   {
      //  we assume that any time our message window exists it is already displayed
      return true;
   }

   pMsgWindow = window_create(); 
   if (pMsgWindow == NULL)
   {
      return false;
   }

   window_set_window_handlers(pMsgWindow, (WindowHandlers) {
                                             .load = message_window_load,
                                             .unload = message_window_unload
                                          } );

   window_stack_push(pMsgWindow, false /* animated */);

   return true;

}  /* end of ensure_message_window_pushed() */


void  message_window_close()
{

   if (pMsgWindow == 0)
   {
      return;
   }

   if (pMsgWindow == window_stack_get_top_window())
   {
      window_stack_pop (false /* animated	*/);
   }

   window_destroy(pMsgWindow);
   pMsgWindow = 0;

   return;

}  /* end of message_window_close */


void  message_window_show_status (const char *pszCaption, const char *pszText)
{

   ensure_message_window_pushed();

   text_layer_set_text(pCaption, pszCaption);
   text_layer_set_text(pMsgText, pszText);

}  /* end of message_window_show_status */


void  message_window_show_error (FailureSource eErrSrc,
                                 int32_t errCode, const char *pszErrMsg)
{

   const char * pszCaption;

   ensure_message_window_pushed();

   if (eErrSrc == FAIL_SRC_APP_MSG)
      pszCaption = "Watch/Phone Comms Error";
   else
      pszCaption = "Location Query Error";

   snprintf(achTextBuf, sizeof(achTextBuf), 
            "Code (%d) : %s", (int) errCode, pszErrMsg);

   text_layer_set_text(pCaption, pszCaption);
   text_layer_set_text(pMsgText, achTextBuf);

}  /* end of message_window_show_error */

