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

#include "gbl2ngc.hpp"

static double unit_mm2in(double v) {
  return v/25.4;
}

static double unit_in2mm(double v) {
  return v*25.4;
}

static double unit_identity(double v) {
  return v;
}

void export_paths_to_gcode_unit( FILE *ofp, Paths &paths, int src_units_0in_1mm, int dst_units_0in_1mm)
{
  int i, j, n, m;
  int first;

  double (*f)(double);

  f = unit_identity;
  if (src_units_0in_1mm != dst_units_0in_1mm) {
    if ((src_units_0in_1mm == 1) && (dst_units_0in_1mm == 0)) {
      f = unit_mm2in;
    } else {
      f = unit_in2mm;
    }
  }

  if (gHumanReadable) {
    fprintf(ofp, "f%i\n", gFeedRate);
    fprintf(ofp, "g1 z%f", gZSafe);
  } else {
    fprintf(ofp, "F%i\n", gFeedRate);
    fprintf(ofp, "G01Z%f\n", gZSafe);
  }

  if (gHumanReadable) {
    fprintf(ofp, "\n");
  }

  if (gShowComments) {
    fprintf(ofp, "\n( feed %i zsafe %f, zcut %f )\n", gFeedRate, gZSafe, gZCut );
  }

  n = paths.size();
  for (i=0; i<n; i++)
  {

    if (gHumanReadable) {
      fprintf(ofp, "\n\n");
    }

    if (gShowComments) {
      fprintf(ofp, "( path %i )\n", i);
    }

    first = 1;
    m = paths[i].size();
    for (j=0; j<m; j++)
    {
      if (first)
      {

        if (gHumanReadable) {
          fprintf(ofp, "g0 x%f y%f\n", f(ctod( paths[i][j].X )), f(ctod( paths[i][j].Y )) );
          fprintf(ofp, "g1 z%f\n", gZCut);
        } else {
          fprintf(ofp, "G00X%fY%f\n", f(ctod( paths[i][j].X )), f(ctod( paths[i][j].Y )) );
          fprintf(ofp, "G01Z%f\n", gZCut);
        }

        first = 0;
      }
      else
      {

        if (gHumanReadable) {
          fprintf(ofp, "g1 x%f y%f\n", f(ctod( paths[i][j].X )), f(ctod( paths[i][j].Y )) );
        } else {
          fprintf(ofp, "G01X%fY%f\n", f(ctod( paths[i][j].X )), f(ctod( paths[i][j].Y )) );
        }

      }

    }

    // go back to start
    //
    if (gHumanReadable) {
      fprintf(ofp, "g1 x%f y%f\n", f(ctod( paths[i][0].X )), f(ctod( paths[i][0].Y )) );
      fprintf(ofp, "g1 z%f\n", gZSafe);
    } else {
      fprintf(ofp, "G01X%fY%f\n", f(ctod( paths[i][0].X )), f(ctod( paths[i][0].Y )) );
      fprintf(ofp, "G01Z%f\n", gZSafe);
    }

  }

  if (gHumanReadable) {
    fprintf(ofp, "\n\n");
  }

}


// deprecated
/*
void export_paths_to_gcode( FILE *ofp, Paths &paths)
{
  int i, j, n, m;
  int first;

  fprintf(ofp, "f%i\n", gFeedRate);
  fprintf(ofp, "g1 z%f", gZSafe);

  fprintf(ofp, "\n( feed %i zsafe %f, zcut %f )\n", gFeedRate, gZSafe, gZCut );

  n = paths.size();
  for (i=0; i<n; i++)
  {
    fprintf(ofp, "\n\n( path %i )\n", i);

    first = 1;
    m = paths[i].size();
    for (j=0; j<m; j++)
    {
      if (first)
      {
        fprintf(ofp, "g0 x%f y%f\n", ctod( paths[i][j].X ), ctod( paths[i][j].Y ) );
        fprintf(ofp, "g1 z%f\n", gZCut);
        first = 0;
      }
      else
      {
        fprintf(ofp, "g1 x%f y%f\n", ctod( paths[i][j].X ), ctod( paths[i][j].Y ) );
      }

    }

    // go back to start
    //
    fprintf(ofp, "g1 x%f y%f\n", ctod( paths[i][0].X ), ctod( paths[i][0].Y ) );
    fprintf(ofp, "g1 z%f\n", gZSafe);

  }

  fprintf(ofp, "\n\n");

}
*/
