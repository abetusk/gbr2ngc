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


