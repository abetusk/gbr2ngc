gbl2ngc
=======

Open source no frills gerber to gcode converter, using (a slightly modified) [Clipper Lib](http://www.angusj.com/delphi/clipper.php).  Produces an isolation routing gcode file for the given gerber file.

gbl2ngc will convert a gerber file like this:


![gerber example](https://raw2.github.com/abetusk/gbl2ngc/master/example/gerbExample.png)


to a gcode file like this:


![gcode example](https://raw2.github.com/abetusk/gbl2ngc/master/example/gcodeExample.png)


to compile:
-----------

    cd src
    ./cmp.sh

example usage:
--------------

    gbl2ngc --input example/example.gbl --radius 0.0025 --output example.ngc

Current version is in an alpha state, so use at your own risk.


TODO:
-----

  - allow for metric (right now it's imperial only)
  - allow for other options in terms of seek rate, feed rate, etc.
  - optimizing air drill paths




