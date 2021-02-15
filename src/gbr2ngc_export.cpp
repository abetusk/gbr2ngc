/*
*    This program is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
* This program was written while working at Bright Works Computer Consulting
* and allowed to be GPL'd under express permission of the current president
* John Guttridge
* Dated May 20th 2013
*/

#include "gbr2ngc.hpp"

// How many digits after the decimal point?
// Yes, it should be a string :)
#define GCODE_LENGTH_PRECISION "6"

static double unit_mm2in(double v) {
  return v/25.4;
}

static double unit_in2mm(double v) {
  return v*25.4;
}

static double unit_identity(double v) {
  return v;
}

// Export a rapid (G0) movement to file.
// `axes` is a string containing the letters of the axes to move.
// Up to three axis coordinates can be provided.
// Ex: rapid(f, "xz", 3, -5) will move X3 and Y-5
//
void rapid(FILE* file, const char* axes, double one, double two=0, double three=0) {
  int i;
  double coords[] = {one, two, three};

  if (gHumanReadable) {
    fprintf(file, "g0");
    for (i = 0; axes[i] != '\0'; i++) {
      fprintf(file, " %c%." GCODE_LENGTH_PRECISION "f", tolower(axes[i]), coords[i]);
    }
  } else {
    fprintf(file, "G00");
    for (i = 0; axes[i] != '\0'; i++) {
      fprintf(file, "%c%." GCODE_LENGTH_PRECISION "f", toupper(axes[i]), coords[i]);
    }
  }

  if (gSeekRateSet) {
    if (gHumanReadable) {
      fprintf(file, " f%" GCODE_LENGTH_PRECISION "f", (double)gSeekRate);
    }
    else {
      fprintf(file, " F%" GCODE_LENGTH_PRECISION "f", (double)gSeekRate);
    }
  }

  fprintf(file, "\n");
}

// G1 equiv. of rapid()
//
void cut(FILE* file, const char* axes, double one, double two=0, double three=0) {
  int i;
  double coords[] = {one, two, three};

  if (gHumanReadable) {
    fprintf(file, "g1");
    for (i = 0; axes[i] != '\0'; i++) {
      fprintf(file, " %c%." GCODE_LENGTH_PRECISION "f", tolower(axes[i]), coords[i]);
    }
  } else {
    fprintf(file, "G01");
    for (i = 0; axes[i] != '\0'; i++) {
      fprintf(file, "%c%." GCODE_LENGTH_PRECISION "f", toupper(axes[i]), coords[i]);
    }
  }

  if (gFeedRateSet) {
    if (gHumanReadable) {
      fprintf(file, " f%" GCODE_LENGTH_PRECISION "f", (double)gFeedRate);
    }
    else {
      fprintf(file, " F%" GCODE_LENGTH_PRECISION "f", (double)gFeedRate);
    }
  }


  fprintf(file, "\n");
}

int export_paths_to_polygon_unit( FILE *ofp, Paths &paths, int src_units_0in_1mm, int dst_units_0in_1mm, double ds) {
  int i, j, n, m;

  double x, y, start_x, start_y;
  double _ds;

  double (*f)(double);

  _ds = ds;
  if (_ds == 0.0) {
    _ds = ( dst_units_0in_1mm ? 0.125 : 1.0/1024.0 );
  }

  f = unit_identity;
  if (src_units_0in_1mm != dst_units_0in_1mm) {
    if ((src_units_0in_1mm == 1) && (dst_units_0in_1mm == 0)) {
      f = unit_mm2in;
    }
    else {
      f = unit_in2mm;
    }
  }

  n = paths.size();
  for (i=0; i<n; i++) {

    fprintf(ofp, "\n");

    if (gShowComments)  { fprintf(ofp, "( path %i )\n", i); }

    m = paths[i].size();

    if (m<1) { continue; }
    start_x = f(ctod( paths[i][0].X ));
    start_y = f(ctod( paths[i][0].Y ));

    for (j=0; j<m; j++) {
      x = f(ctod( paths[i][j].X ));
      y = f(ctod( paths[i][j].Y ));

      fprintf(ofp, "%0.8f %0.8f\n", x, y);
    }

    fprintf(ofp,  "%0.8f %0.8f\n", start_x, start_y);
  }

  return 0;
}

int export_paths_to_gcode_unit( FILE *ofp, Paths &paths, int src_units_0in_1mm, int dst_units_0in_1mm, double ds) {
  int i, j, n, m, n_t;
  int first;
  int ret;

  double (*f)(double);

  double x, y, start_x, start_y;

  double _zcut, len_d, t;
  double _x, _y, _prev_x, _prev_y;
  double _ds;

  double tx, ty, tz;

  _ds = ds;
  if (_ds == 0.0) {
    _ds = ( dst_units_0in_1mm ? 0.125 : 1.0/1024.0 );
  }

  f = unit_identity;
  if (src_units_0in_1mm != dst_units_0in_1mm) {
    if ((src_units_0in_1mm == 1) && (dst_units_0in_1mm == 0)) {
      f = unit_mm2in;
    }
    else {
      f = unit_in2mm;
    }
  }

  if (gGCodeHeader)   { fprintf(ofp, "%s\n", gGCodeHeader); }

  /*
  if (gHumanReadable) { fprintf(ofp, "f%i\n", gFeedRate); }
  else                { fprintf(ofp, "F%i\n", gFeedRate); }

  if (gSeekRateSet) {
    if (gHumanReadable) { fprintf(ofp, "g0 f%i\n", gSeekRate); }
    else                { fprintf(ofp, "G0 F%i\n", gSeekRate); }
  }
  */

  cut(ofp, "z", gZSafe);

  if (gHumanReadable) { fprintf(ofp, "\n"); }
  if (gShowComments)  { fprintf(ofp, "\n( feed %i seek %i zsafe %f, zcut %f )\n", gFeedRate, gSeekRate, gZSafe, gZCut ); }

  n = paths.size();
  for (i=0; i<n; i++) {

    if (gHumanReadable) { fprintf(ofp, "\n\n"); }
    if (gShowComments)  { fprintf(ofp, "( path %i )\n", i); }

    first = 1;
    m = paths[i].size();

    if (m<1) { continue; }
    start_x = f(ctod( paths[i][0].X ));
    start_y = f(ctod( paths[i][0].Y ));

    _prev_x = start_x;
    _prev_y = start_y;


    for (j=0; j<m; j++) {
      x = f(ctod( paths[i][j].X ));
      y = f(ctod( paths[i][j].Y ));

      if (first) {
        rapid(ofp, "xy", x, y);

        if (!gHeightOffset) {
          cut(ofp, "z", gZCut);
        }
        else {
          ret = gHeightMap.zOffset(_zcut, x,y);
          if (ret!=0) { return -2; }
          cut(ofp, "xyz", x,y,_zcut);
        }

        first = 0;
      }
      else {

        if (!gHeightOffset) {
          cut(ofp, "xy", x, y);
        }
        else {

          len_d = sqrt( (_prev_y - y)*(_prev_y - y) + (_prev_x - x)*(_prev_x - x) );
          n_t = (int)(len_d/_ds);
          for (int __i=1; __i<n_t; __i++) {
            t = (double)__i/(double)n_t;
            _x = _prev_x + t*(x - _prev_x);
            _y = _prev_y + t*(y - _prev_y);

            ret = gHeightMap.zOffset(_zcut, _x, _y);
            if (ret!=0) { return -2; }

            cut(ofp, "xyz", _x, _y, _zcut);
          }
          ret = gHeightMap.zOffset(_zcut, x,y);
          if (ret!=0) { return -2; }

          cut(ofp, "xyz", x, y, _zcut);

        }


      }

      _prev_x = x;
      _prev_y = y;

    }

    // go back to start
    //

    if (!gHeightOffset) {
      cut(ofp, "xy", start_x, start_y);
    }
    else {
      ret = gHeightMap.zOffset(_zcut, start_x,start_y);

      x = start_x;
      y = start_y;

      len_d = sqrt( (_prev_y - y)*(_prev_y - y) + (_prev_x - x)*(_prev_x - x) );
      n_t = (int)(len_d/_ds);
      for (int __i=1; __i<n_t; __i++) {
        t = (double)__i/(double)n_t;
        _x = _prev_x + t*(x - _prev_x);
        _y = _prev_y + t*(y - _prev_y);

        ret = gHeightMap.zOffset(_zcut, _x, _y);
        if (ret!=0) { return -2; }

        cut(ofp, "xyz", _x, _y, _zcut);
      }

      ret = gHeightMap.zOffset(_zcut, start_x, start_y);
      if (ret!=0) { return -2; }

      cut(ofp, "xyz", start_x, start_y, _zcut);
    }

    cut(ofp, "z", gZSafe);
  }

  if (gHumanReadable) { fprintf(ofp, "\n\n"); }
  if (gGCodeFooter)   { fprintf(ofp, "%s\n", gGCodeFooter); }

}
