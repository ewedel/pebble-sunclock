This is a Pebble watch face, originally created (to the best of my understanding) by MichaelEhrmann, and then updated by KarbonPebbler, Boldo, Chad Harp, Bob TheMadCow and orgemd.  (My apologies if I have left out any other contributors.  If I have, please let me know.)

# Instructions

Please refer to the [Twilight Sunclock webpage](http://ewedel.github.io/pebble-sunclock/) for usage information.

# Obtaining the Software

The best way to install the Twilight Sunclock is from the Pebble store, so that it is properly tracked by your phone's Pebble application. To obtain it, search the Pebble app store for Twilight Sunclock or [click on this link](http://pblweb.com/appstore/52cbea986e5f7003cf000134/ "Twilight SunClock on the Pebble appstore").

If for some reason that doesn't work, you can also [download Twilight Sunclock from My Pebble Faces](http://www.mypebblefaces.com/apps/21510/9960/).

# History

3.5

[Pebble SDK 3.14]

Fix another high-latitude bug: handle case when edges of a twilight band are both on the same side of midnight.

3.4

[Pebble SDK 3.13.1]

Add Pebble Time Round support, and temporarily (I hope!) remove Classic support.

Correct display issues when latitude is towards either pole (reported case was near Bremen, Germany, but it should work further North / South now).

Fix "missing config" and other error windows which had been malfunctioning for the last few releases.

3.3

[Pebble SDK 3.6]

For SDK v3 and higher, add support for watch's now-built-in timezone info.  This lets the watchface update for daylight savings time entry/exit without phone involvement.

Refine location auto-request code so secondary requests are sent sooner (a minute apart), and if a change in built-in timezone offset is seen trigger a location request also in case the offset change is due to physical relocation instead of DST entry/exit.

On color Pebbles change the hour hand / gnomon shadow so it is more visible: during night-time it now renders as grey rather than black.  In daytime it is black as before.  Thanks to Andrea Carlo Butti for suggesting this.

3.2

Restore auto-request of location data on watchface startup. Had unintentionally removed this when chasing an iOS-specific issue.

It still asked when location was not cached on the watch, but a forced location query is helpful when changing locations / timezones. Now it once again asks at each startup regardless of whether location is cached, and will of course use the cached value until anything newer comes along.

3.1

[Pebble SDK 3.2]

In Pebble Time variant, draw hour hand using path primitives instead of a bitmap. Pebble Time's antialiasing makes this look smoother.

Also add a simple low-charge watch battery indicator: the pivot point of the hour hand changes color as follows:

| Color   | Charge Remaining |
| ------- |:----------------:|
| (none)  |       > 30%      |
| Yellow  |      <= 30%      |
| Orange  |      <= 20%      |
| Red     |      <= 10%      |

Thanks to Andrea Carlo Butti for suggesting this type of indicator, though I morphed it from BT-connected to low-battery. And mistakes of course are mine.

3.0

Add support for Pebble Time watch.

2.4

Remove sample JSON encodings from header comment of reverseGeoCode() in pebble-js-app.js.  These comments (!) were preventing the iOS Pebble app from loading / running the Sunclock's Pebblekit JS, including its access to location / timezone data.

Config page behavior is still incorrect under iOS, but at least the base watchface now works.

2.3

Fix a missing semi-colon flagged by jslint in pebble-js-app.js.  Switch build from Pebble SDK v2.8 to v2.9.

2.2

Speculative fix for iOS: delay initial location request send from watch until after watchface's message pump is running.

2.1

Add configuration pages.  Also switch to two-part version numbers so the Pebble application can (theoretically) auto-upgrade us.  Switch build from SDK v2.0 beta 6 to SDK v2.8 (yeah, it had been a while).

2.0.1

Fully initialize text fields before displaying the watchface. Had avoided this due to display hesitations in Pebble v1.x, but Pebble 2.x doesn't have the same difficulty.

2.0.0

Initial port to Pebble version 2 environment.

# Lineage

This version is a tweak of the updates posted by orgemd.

The following changes have been made to orgemd's version:

1.  Source code converted to use Pebble's SDK version 2 APIs.
2.  A considerable amount of code refactoring was done, both to reduce code space and hopefully to clarify the code.
3.  Some comments were added, at least for the parts I deciphered.
4.  A separate message window was added, along with comms code to query a new phone-based javascript component for location and timezone offset data.  To keep from eating the battery alive the location query is run only once, each time the sunclock watchface is started.
5.  A couple routines were set up to provide a standardized watch-based persistence interface.  This is used to save the returned location data and supply it onsubsequent restarts.
6.  Due to the above, there is no longer a compile-time setting for location. The watchface always obtains location dynamically, and will display an error message if it is not (ever) able to obtain one.  But if it had previously saved a location in flash, it will use that value instead.

-----------------------

The following is a list of orgemd's changes from Bob TheMadCow's version:

1.  Moon phase is fixed so that it shows correctly.
2.  The watch hand has been slimmed down so that it obscures less.
3.  The day/date information has been changed to a single line such as "Mon Oct 8, 2013" rather than four separate pieces.
4.  The watch face has been shifted down to allow the single day/date line.
5.  The digital time on the watch face has been made much larger to be easier to read.
6.  The numbers 6,12,18,24 have been removed from the watch face to make it less cluttered.
7.  The hourly tick marks on the face have been made slightly larger.
8.  Some tweaks to the sunrise/sunset text display.

There is both a compiled version included, as well as the src and resources stuff needed to compile it yourself.


