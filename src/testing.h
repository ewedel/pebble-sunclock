/**
 *  @file
 *  
 *  A few constants to simplify kicking the Sunclock into test modes for debug.
 */

#ifndef sunclock_testing_h__
#define sunclock_testing_h__


///  Set true to always fail to read from watch's location cache, even when valid.
#define  TESTING_DISABLE_CACHE_READ  0

///  Set true to disable normal load-time send of location update request to phone.
#define  TESTING_DISABLE_LOCATION_REQUEST  0

///  Set to true to force display of low-battery hour hand hub.
#define  TESTING_SHOW_LOW_BATTERY    0

///  Use dummy coords for Mountain View, CA
#define  TESTING_USE_DUMMY_COORDS_MV  0

///  Use dummy coords for Bremen, DEU
#define  TESTING_USE_DUMMY_COORDS_BREMEN  0

///  Use dummy coords for Machu Picchu Base, Antarctica
#define  TESTING_USE_DUMMY_COORDS_MPB  0

///  Setting any of the preceding enable dummy coord testing.
#define  TESTING_USE_DUMMY_COORDS    ( TESTING_USE_DUMMY_COORDS_MV |      \
                                       TESTING_USE_DUMMY_COORDS_BREMEN |  \
                                       TESTING_USE_DUMMY_COORDS_MPB )

///  Values per test coord send log message:
//  sending lat/long 37.3763061 / -122.0918526, utcOff secs = 25200

#if TESTING_USE_DUMMY_COORDS_MV
#define  TESTING_DUMMY_LATITUDE       37.3763061
#define  TESTING_DUMMY_LONGITUDE    -122.0918526
#define  TESTING_DUMMY_UTC_OFFSET  25200        /* when DST is in effect */
#define  TESTING_DUMMY_NAME            "Mountain View"

#elif TESTING_USE_DUMMY_COORDS_BREMEN
#define  TESTING_DUMMY_LATITUDE       53.0793
#define  TESTING_DUMMY_LONGITUDE       8.8017
#define  TESTING_DUMMY_UTC_OFFSET  -7200        /* when DST is in effect */
#define  TESTING_DUMMY_NAME            "Bremen"

#elif TESTING_USE_DUMMY_COORDS_MPB
#define  TESTING_DUMMY_LATITUDE      -62.09139
#define  TESTING_DUMMY_LONGITUDE      58.47111
#define  TESTING_DUMMY_UTC_OFFSET -14400        /* AST - no DST? */
#define  TESTING_DUMMY_NAME            "Machu Picchu Base"

#endif



#endif  // #ifndef sunclock_testing_h__

