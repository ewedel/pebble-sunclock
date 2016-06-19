/**
 *  @file
 *  
 */

#pragma once

#include  "messaging.h"


/**
 *  Write the supplied error info to our message window, and make sure
 *  it is visible.
 *  
 *  @param pszCaption Points to text to show in window caption.
 *                Supplied text must exist for life of message window.
 *  @param pszText Points to text to show in window body.
 *                Supplied text must exist for life of message window.
 */
void  message_window_show_status (const char *pszCaption, const char *pszText);

/**
 *  Write the supplied error info to our message window, and make sure
 *  it is visible.
 * 
 *  @param eErrSrc 
 *  @param errCode 
 *  @param pszErrMsg 
 */
void  message_window_show_error (FailureSource eErrSrc,
                                 int32_t errCode, const char *pszErrMsg);


/**
 *  Close our message window, if any.
 */
void  message_window_close();


