OMX2GPX - A simple Geonaute OnMove 220 GPS watch OMH/OMD to GPX converter.
Code: Clement CORDE - c1702@yahoo.com, started on 2016/10/08.


*** 2016/10/25 - v.1.03 ***

- Added average pace.

- Added target statistics. I did only one test for each type of target, maybe there will be some adjustments to do.


*** 2016/10/20 - v.1.02 ***

- Tested with cardio belt. Seems ok.

- Fixed one line in the header output, I got it wrong. Added additionnal control.

- Added an elevation feature (see option 'e'). It uses Google Maps Elevation API, and due to the limitation of requests in a day (2.500), it requires you to ask for an API key. This is free!
- If ever you don't want to be bothered with this, you can upload the GPX file to http://www.gpsvisualizer.com/elevation and download a new GPX with the elevation included. This is free as well. There is one problem though, gpsvisualizer gets rid of the heart rate info. You can fix this by merging the original GPX file (without elevation but with heart rate) and the gpsvisualizer GPX (with elevation but without heart rate) into a new one with the 'merge_ele.pl' script provided.

- Changed the options system (internal).


*** 2016/10/13 - v.1.01 ***

Minor modifications.


*** 2016/10/11 - v.1.00 ***

Hi there.

I recently bought an OnMove 220 GPS watch, which is cool, but with which I ran into a problem. I'm a Linux user and my smartphone runs with Android 4.1. Geonaute's OnConnect is compatible with Windows, Mac OS and Android 4.3+. So basically I couldn't do anything of the OMH/OMD files recorded in the watch, which was kind of a shame.

Unfortunately, information about OMH and OMD files is rather rare on the net. I finally found valuable info here:
https://github.com/rboninsegna/OnMove200
https://www.snip2code.com/Snippet/1057567/strava-onmove200        (complete OMH/OMD description!)
Many thanks to the authors. (^_^)

With the aforementioned info, I wrote this very simple program. It extracts info from the OMH/OMD files and generates a GPX file. I think the OnMove 200 and 220 watches are using the same data format, therefore the program should also work with the OnMove 200 files.

I uploaded the generated GPX files in Sports-Tracker (web version), it seems to work.

I'll update/fix the program if needed, if buggy, if I have some requests, aso...

From what I could gather/observe:
- The OMH file is a header file, containing the session statistics. It has a fixed size of 60 bytes.
- The OMD file contains the GPS records. It's a simple list of 20 bytes chunks, with no header, no checksum, anything. The info is split between GPS and CURVE records. A CURVE record contains additional info for the 2 previous GPS records. So usually you'll always find the following sequence: GPS/GPS/CURVE / GPS/GPS/CURVE / aso, with the last sequence being either GPS/GPS/CURVE or GPS/CURVE. In the last case, additional info for the second GPS point should be ignored (and is apparently set to 0).

Known problems:
- There is no "point zero" in the OMD file. The first GPS point is recorded 5 seconds after pressing the start button, enough to be a few meters away from your starting location. I don't know yet how to fix this. (The record in the OMD file includes the distance ran, but Sports-Tracker ignored the different "distance" tags I tried. I guess the software prefers to calculate the distance from the GPS coordinates, which is not a bad idea). The solution I thought of is to provide the program with GPS coordinates for the zero point, to be inserted at the first place in the GPX file (see option 'z'). Not that convenient but effective anyway, plus it fixes the missing first seconds' gap.
- The watch doesn't record the elevation, so for now there is no elevation in the GPX file either.
- I don't own a cardio belt yet. I did a few tests anyway and I think the heart rate values should go in the GPX file nicely.
- I haven't used the "target" feature of the watch by now, so I've only got blank data in my files. I'll see what I'll get if/when I'll use it.
- There are some mysterious fields remaining in the structures...

Unknown:
- I don't have a MS Windows machine nearby, so I don't know if the program will compile/run under Windows.

Additional note to programmers:
- Please do *NOT* come back to me to tell me that the goto statement should be avoided. I think it's an efficient way to deal with errors, and I won't change it.

--- EOF
