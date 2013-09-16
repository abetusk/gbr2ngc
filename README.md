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
  - outline milling strategy (zen garden milling)
  
KNOWN BUGS
----------

Things fail or get wonky if the offsetting is too thick.   Right now it 
will segfault, with CGAL throwing some sort of error, if the tool radius
puts the polygon offset in a bad state (overlapping regions, etc.).
I'll have to work on it to make sure it catches these errors gracefully.

Update 2013-09-16:
I've decided that this should be the users responsibility.  The segfault
was being caused when the PolygonSet was being constructed with intersections.
Instead of using a PolygonSet, I use a stl vector to hole the disjoint polygons
(with holes) and the offset each individually.  This means the segfault is avoided,
but now there might be some self intersections.

