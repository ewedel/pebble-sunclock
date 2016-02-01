/**
 *  @file
 *  
 */

#include  "ConfigData.h"

#include  "testing.h"


///  Version of code's current ConfigDataCurLocation structure layout.
#define CONFIG_DATA_CUR_VERSION 1

/**
 *  All data we persist to flash for current location.
 */
typedef struct
{
   ///  Version of this struct.  Always CONFIG_DATA_CUR_VERSION now.
   uint16_t usVersion;

   ///  Set to zero.
   uint16_t usReserved;

   ///  Degrees from equator: positive for North, negative for South.
   float    fLatitude;

   ///  Degrees from Greenwich: positive for Each, negative for West.
   float    fLongitude;

   ///  Time offset from UTC to local time.
   int32_t  iUtcOffset;

   /**
    *  Time this struct's values were last changed in flash.
    *  NB: this comes from PebbleOS' time() call, so may be local time
    *      instead of the customary UTC.
    *  
    *  A zero value here means we have no location config data.
    */
   time_t   timeLastUpdate;

} __attribute__((__packed__))  ConfigDataCurLocation;


///  PebbleOS persist_* config item key for ConfigDataCurLocation.
#define  CONFIG_DATA_KEY_CUR_LOCATION  1

///  Cached copy of watch flash.  Valid after config_data_init() is called.
static ConfigDataCurLocation  curLocationCache;

///  Cached copy of timezone-in-hours.  Valid after config_data_init() is called.
static float curTimezoneInHours = 0;

#ifndef PBL_SDK_2
/**
 *  For SDK 3 and later, we monitor Pebble's own understanding of timezone.
 *  This is the Pebble's local offset from UTC in seconds, last time we
 *  checked (typically via config_has_tz_offset_changed()).
 *  
 *  Uses normal UTC-relative sign, so PST is -(8 * 3600).
 */
static int curTimezoneInSeconds = 0;
#endif


/**
 *  Silly little helper because we persist a "UTC offset" (from local) in seconds,
 *  but the app wants local offset from UTC (traditional tz info) expressed in hours
 *  and fractions of an hour.
 *
 *  Since this is floating point, we calculate it at config updates rather
 *  than on the fly.
 */
static void  compute_tz_in_hours()
{
#ifdef PBL_SDK_2
   //  our persisted offset is from local, so sign-reverse to get normal convention.
   curTimezoneInHours = -(curLocationCache.iUtcOffset / 3600.0);
#else
   //  Ignore UTC config explicitly fed us by phone, always use Pebble's own offset.
   curTimezoneInHours = curTimezoneInSeconds / 3600.0;
#endif
}


#ifndef PBL_SDK_2
///  Return Pebble watch's effective local time offset from UTC, factoring in DST.
static int  get_pebble_tz_secs()
{
   time_t   tmNow = time(NULL);
   struct tm * pTm = localtime(&tmNow);

   int gmtoff = pTm->tm_gmtoff;
   if (pTm->tm_isdst > 0)
   {
      gmtoff += 3600;
   }
   else if (pTm->tm_isdst < 0)
   {
      //  tm_isdst < 0 means "unknown", so we fall back to our config value.
      gmtoff = -curLocationCache.iUtcOffset; 
   }

   return gmtoff;
}
#endif  // #ifndef PBL_SDK_2


void  config_data_init()
{

#ifndef PBL_SDK_2
   ///  Initialize our reference copy of Pebble's own TZ offset
   curTimezoneInSeconds = get_pebble_tz_secs();
#endif

   int iRet;

   iRet = persist_read_data(CONFIG_DATA_KEY_CUR_LOCATION,
                            &curLocationCache, sizeof(curLocationCache));

#if TESTING_USE_DUMMY_COORDS
   curLocationCache.usVersion  = CONFIG_DATA_CUR_VERSION;
   curLocationCache.usReserved = 0;
   curLocationCache.fLatitude  = TESTING_DUMMY_LATITUDE;
   curLocationCache.fLongitude = TESTING_DUMMY_LONGITUDE;
   curLocationCache.iUtcOffset = TESTING_DUMMY_UTC_OFFSET;
   curLocationCache.timeLastUpdate = time(NULL);
   iRet = sizeof(curLocationCache);
#endif

#if TESTING_DISABLE_CACHE_READ
   iRet = 0;
#endif
   if ((iRet < (int) sizeof(curLocationCache)) ||
       (curLocationCache.usVersion != CONFIG_DATA_CUR_VERSION))
   {
      //  no (usable) persisted data.
      memset(&curLocationCache, 0, sizeof(curLocationCache));
      // (Zeroing the timeLastUpdate field marks cache as invalid.)
      APP_LOG(APP_LOG_LEVEL_WARNING, "* config_data_init(): no usable data");
   }
   else
   {
      APP_LOG(APP_LOG_LEVEL_WARNING, "* config_data_init(): have usable data");
      compute_tz_in_hours();
   }

}


bool  config_data_location_avail()
{
   //  Note that this reflects cache, not the raw state of watch flash.
   return (curLocationCache.timeLastUpdate != 0);
}


#ifndef PBL_SDK_2
bool  config_has_tz_offset_changed()
{

   int latestTimezoneInSeconds = get_pebble_tz_secs();
   if (latestTimezoneInSeconds != curTimezoneInSeconds)
   {
      curTimezoneInSeconds = latestTimezoneInSeconds;
      compute_tz_in_hours();
      return true;
   }

   return false;

}
#endif  // #ifndef PBL_SDK_2


static
bool  config_data_location_get(float* pLat, float *pLong, int32_t *pUtcOffset,
                               time_t* pLastUpdateTime)
{

   if (curLocationCache.timeLastUpdate == 0)
   {
      //  no config data to return.
      return false;
   }

   if (pLat != 0)
      *pLat = curLocationCache.fLatitude; 

   if (pLong != 0)
      *pLong = curLocationCache.fLongitude; 

   if (pUtcOffset != 0)
      *pUtcOffset = curLocationCache.iUtcOffset; 

   if (pLastUpdateTime != 0)
      *pLastUpdateTime = curLocationCache.timeLastUpdate; 

   return true;

}  /* end of config_data_location_get */

float  config_data_get_latitude()
{
   return curLocationCache.fLatitude;
}

float  config_data_get_longitude()
{
   return curLocationCache.fLongitude;
}

float  config_data_get_tz_in_hours()
{
   return curTimezoneInHours;
}


bool  config_data_is_different(float latitude, float longitude, int32_t utcOffset)
{
   float tmpLat, tmpLong;
   int32_t tmp_utcOffset;

   return ! (config_data_location_get(&tmpLat, &tmpLong, &tmp_utcOffset, NULL) &&
             (latitude == tmpLat) && (longitude == tmpLong) && (utcOffset == tmp_utcOffset));

}  /* end of config_data_is_different */


///  Do the geolocation parts of two location structs match?
static bool locations_equiv(ConfigDataCurLocation* pLoc1, ConfigDataCurLocation* pLoc2)
{
   return (pLoc1->fLatitude == pLoc2->fLatitude) &&
          (pLoc1->fLongitude == pLoc2->fLongitude) &&
          (pLoc1->iUtcOffset == pLoc2->iUtcOffset);
}


bool  config_data_location_set(float fLat, float fLong, int32_t iUtcOffset)
{

ConfigDataCurLocation   newLocation;


   newLocation.usVersion      = CONFIG_DATA_CUR_VERSION;
   newLocation.usReserved     = 0;
   newLocation.fLatitude      = fLat;
   newLocation.fLongitude     = fLong;
   newLocation.iUtcOffset     = iUtcOffset;
   newLocation.timeLastUpdate = time(NULL);

   int iRet;

   if (locations_equiv(&newLocation, &curLocationCache))
   {
      //  we want to leave curLocationCache.timeLastUpdate undisturbed.
      return true;
   }

   //  Started seeing E_INTERNAL from persist_write_data, so make a best effort
   //  to remove the old value, if any, first.
   iRet = persist_delete(CONFIG_DATA_KEY_CUR_LOCATION);
   APP_LOG(APP_LOG_LEVEL_DEBUG, "persist_delete ret =  %d", iRet); 

   iRet = persist_write_data(CONFIG_DATA_KEY_CUR_LOCATION, 
                             &newLocation, sizeof(newLocation));
   if (iRet == sizeof(newLocation))
   {
      curLocationCache = newLocation;
      compute_tz_in_hours();

      return true;
   }

   APP_LOG(APP_LOG_LEVEL_DEBUG, "persist_write_data failed, ret =  %d", iRet);

   //BUGBUG - should we flag cached copy as invalid to match?
   return false;

}  /* end of config_data_location_set */


void  config_data_location_erase(void)
{
   persist_delete(CONFIG_DATA_KEY_CUR_LOCATION);

   //  clear cache to match:
   memset(&curLocationCache, 0, sizeof(curLocationCache));
}

