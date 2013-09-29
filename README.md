gbl2ngc
=======

Open source no frills gerber to gcode converter, using CGAL.  Produces an isolation routing gcode file for the given gerber file.

example usage:
--------------

gbl2ngc --input example/example.gbl --radius 0.0025 --output example.ngc

Current version is in an alpha state, so use at your own risk.

TODO
----

  - scanline milling strategy 
  - outline milling strategy (zen garden milling) speedup.  Currently _very_ slow
  
KNOWN ISSUES
------------

  - Outline milling is so very slow.  On my system for the example gerber provided, it takes upwards of 15 minutes to render.
  - In fact, the whole thing is pretty slow.  10s (on my system) to just do the outline seems execessive.
  - I think the offsetting is pretty wonky, as in it creates self intersections that
    make CGAL bork if used improperly.  Should I give warnings to users about self intersections?
    what should I do about them in general?


