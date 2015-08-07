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

///  Set to true to use following dummy coords.
#define  TESTING_USE_DUMMY_COORDS    0

///  Values per test coord send log message:
//  sending lat/long 37.3763061 / -122.0918526, utcOff secs = 25200

#define  TESTING_DUMMY_LATITUDE       37.3763061
#define  TESTING_DUMMY_LONGITUDE    -122.0918526
#define  TESTING_DUMMY_UTC_OFFSET  25200


#endif  // #ifndef sunclock_testing_h__

