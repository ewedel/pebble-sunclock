/**
 *  @file
 *  
 */

#pragma once


#include  "pebble.h"


/**
 *  Read what configuration data we have from watch flash into RAM cache.
 *  Best called from program init, as this might be a lengthy operation.
 */
void  config_data_init();

/**
 *  Convenience function to simply check whether location data is persisted,
 *  without returning the values.
 *
 *  @return \c true if persisted location info is available, else \c false.
 */
bool  config_data_location_avail();

/**
 *  Has timezone offset changed since our last check.
 *  Intended to detect whether the Pebble has changed its DST flag, or been
 *  updated by the phone.
 *
 *  Only makes sense in SDK v3 and later, since that is when Pebble first (!)
 *  added true on-watch support for timezones and DST.
 */
#ifndef PBL_SDK_2
bool  config_has_tz_offset_changed();
#endif

/**
 *  Read location data from our watch-based persistent storage.
 *
 *  Any of the supplied output pointers may be NULL if the caller is not
 *  interested in the particular field.
 *
 *  @param pLat Points to var to receive latitude coord: degrees from equator,
 *             positive for North, negative for South.
 *  @param pLong Points to var to receive longitude coord: degrees from
 *             Greenwich, positive for East, negative for West.
 *  @param pUtcOffset Points to var to receive offset of local (watch) time
 *             from UTC, in seconds.  This is the reverse of the usual tz
 *             offset: *pUtcOffset is added to local time to obtain UTC.
 *  @param pLastUpdateTime Points to var to receive the time the returned values
 *             were last changed in flash.  This is relative to PebbleOS' time()
 *             value, so is local time rather than UTC.
 * 
 *  @return \c true if persisted location info is available, else \c false.
 *          In the latter case, all other returns are undefined.
 */
//bool  config_data_location_get(float* pLat, float *pLong, int32_t *pUtcOffset,
//                               time_t* pLastUpdateTime);

/** 
 *  Returns most recent latitude fed us by the phone.
 *  
 *  @return Degrees from equator.  Positive for North, negative for South.
 */
float  config_data_get_latitude();

/**
 *  Returns most recent longitude fed us by the phone.
 * 
 * @return Degrees from Greenwich.  Positive for East, negative for West.
 */
float  config_data_get_longitude();

/**
 *  Returns timezone offset.
 * 
 * @return Offset of time from UTC, in hours.  Add this to UTC to obtain local time.
 */
float  config_data_get_tz_in_hours();

/**
 *  Convenience to check if the caller-supplied values match our config values.
 *  
 *  @param fLat Latitude coord: degrees from equator, positive for North,
 *             negative for South.
 *  @param fLong Longitutde coord: degrees from Greenwich, positive for East
 *             and negative for West.
 *  @param pUtcOffset Offset of local (watch) time from UTC, in seconds.
 *             This is the reverse of the usual tz offset: *pUtcOffset
 *             is added to local time to obtain UTC.
 *  
 *  @return \c true of parameters match our config, else \c false.
 */
bool  config_data_is_different(float latitude, float longitude, int32_t utcOffset);


/**
 *  Save supplied location values in our cache and in watch flash.
 *  This is a blocking call and the flash write might take a noticable
 *  amount of time, so design accordingly.
 * 
 *  @param fLat Latitude coord: degrees from equator, positive for North,
 *             negative for South.
 *  @param fLong Longitutde coord: degrees from Greenwich, positive for East
 *             and negative for West.
 *  @param pUtcOffset Offset of local (watch) time from UTC, in seconds.
 *             This is the reverse of the usual tz offset: *pUtcOffset
 *             is added to local time to obtain UTC.
 *  
 *  @return \c true if write went ok, \c false if it failed.
 */
bool  config_data_location_set(float fLat, float fLong, int32_t iUtcOffset);


/**
 *  Remove location configuration data from watch flash.  Intended for testing,
 *  this is also a blocking call and likely as slow as flash write.
 */
void  config_data_location_erase(void);

