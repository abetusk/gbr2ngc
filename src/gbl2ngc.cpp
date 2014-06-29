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


struct option gLongOption[] =
{
  {"radius" , required_argument , 0, 'r'},
  {"routeradius" , required_argument , 0, 'R'},

  {"input"  , required_argument , 0, 'i'},
  {"output" , required_argument , 0, 'o'},

  {"feed"   , required_argument , 0, 'f'},
  {"seek"   , required_argument , 0, 's'},

  {"zsafe"  , required_argument , 0, 'z'},
  {"zcut"   , required_argument , 0, 'Z'},

  {"metric" , no_argument       , 0, 'I'},
  {"inches" , no_argument       , 0, 'M'},
//  {"scan"   , no_argument       , 0, 'H'},
//  {"scanvert",no_argument       , 0, 'V'},

  {"horizontal", no_argument     , 0, 'H'},
  {"vertical", no_argument       , 0, 'V'},
  {"zengarden", no_argument     , 0, 'G'},

  {"verbose", no_argument       , 0, 'v'},
  {"version", no_argument       , 0, 'N'},
  {"help"   , no_argument       , 0, 'h'},

  {0, 0, 0, 0}
};


char gOptionDescription[][1024] =
{
  "radius (default 0)",
  "radius to be used for routing (default to radius above)",

  "input file (default stdin)",
  "output file (default stdout)",

  "feed rate (default 10)",
  "seek rate (default 100)",

  "z safe height (default 0.1 inches)",
  "z cut height (default -0.05 inches)",

  "units in metric",
  "units in inches (default)",

  "route out blank areas with a horizontal scan line technique",
  "route out blank areas with a vertical scan line technique",
  "route out blank areas with a 'zen garden' technique",

  "verbose",
  "display version information",
  "help (this screen)",

  "n/a"

};



void show_help(void)
{
  int i, j, k;
  int len;

  printf("a gerber to gcode converter\n");
  printf("Version %s\n", GBL2NGC_VERSION);

  for (i=0; gLongOption[i].name; i++)
  {
    len = strlen(gLongOption[i].name);

    printf("  -%c, --%s", gLongOption[i].val, gLongOption[i].name);
    if (gLongOption[i].has_arg)
    {
      printf(" %s", gLongOption[i].name);
      len = 2*len + 3;
    }
    else
    {
      len = len + 2;
    }
    for (j=0; j<(32-len); j++) printf(" ");

    printf("%s\n", gOptionDescription[i]);
  }

}

void process_command_line_options(int argc, char **argv)
{

  extern char *optarg;
  extern int optind;
  int option_index;

  char ch;

  gRouteRadius = -1.0;

  while ((ch = getopt_long(argc, argv, "i:o:r:s:z:Z:f:IMHVGvNh", gLongOption, &option_index)) > 0) switch(ch)
  {
    case 0:
      //printf("error, bad option '%s'", gLongOption[option_index].name);
      printf("error, bad option '%s'", optarg);
      if (optarg)
        printf(" with arg '%s'", optarg);
      printf("\n");
      break;
    case 'N':
    case 'h':
      show_help();
      exit(0);
      break;
    case 'r':
      gRadius = atof(optarg);
      break;
    case '$':
      gRouteRadius = atof(optarg);
      break;
    case 's':
      gSeekRate = atoi(optarg);
      break;
    case 'z':
      gZSafe = atof(optarg);
      break;
    case 'Z':
      gZCut = atof(optarg);
      break;
    case 'f':
      gFeedRate = atoi(optarg);
      break;
    case 'i':
      gInputFilename = strdup(optarg);
      break;
    case 'o':
      gOutputFilename = strdup(optarg);
      break;
    case 'I':
      gMetricUnits = 0;
      break;
    case 'M':
      gMetricUnits = 1;
      break;

    case 'H':
      gScanLineHorizontal = 1;
      break;
    case 'V':
      gScanLineVertical = 1;
      break;
    case 'G':
      gScanLineZenGarden = 1;
      break;

    case 'v':
      gVerboseFlag = 1;
      break;
    default:
      printf("bad option\n");
      show_help();
      exit(0);
      break;
  }

  if (gRouteRadius <= 0.0)
    gRouteRadius = gRadius;

  if (gOutputFilename)
  {
    if (!(gOutStream = fopen(gOutputFilename, "w")))
    {
      perror(gOutputFilename);
      exit(0);
    }
  }

  if (!gInputFilename)
  {
    printf("must provide input file\n");
    show_help();
    exit(0);
  }

  if (gVerboseFlag)
  {
    fprintf(gOutStream, "( radius %f )\n", gRadius);
  }

}


void cleanup(void)
{
  if (gOutStream != stdout)
    fclose(gOutStream);
  if (gOutputFilename)
    free(gOutputFilename);
  if (gInputFilename)
    free(gInputFilename);
}



void construct_polygon_offset( Paths &src, Paths &soln )
{

  ClipperOffset co;

  co.MiterLimit = 3.0;

  //co.AddPaths( src, jtRound, etClosedPolygon);
  co.AddPaths( src, jtMiter, etClosedPolygon);
  co.Execute( soln, g_scalefactor * gRadius );
}

void print_paths( Paths &paths )
{
  int i, j, k;

  for (i=0; i<paths.size(); i++)
  {
    for (j=0; j<paths[i].size(); j++)
    {
      printf("%lli %lli\n", paths[i][j].X, paths[i][j].Y );
    }
    printf("\n\n");
  }

}

void find_min_max( Paths &src, IntPoint &minp, IntPoint &maxp)
{
  int i, j, n, m;
  n = src.size();
  for (i=0; i<n; i++)
  {
    m = src[i].size();
    if ( m == 0 ) continue;
    if (i==0) 
    {
      minp.X = src[i][0].X;
      minp.Y = src[i][0].Y;
      maxp.X = src[i][0].X;
      maxp.Y = src[i][0].Y;
    }

    for (j=0; j<m; j++)
    {
      if (minp.X > src[i][j].X) minp.X = src[i][j].X;
      if (minp.Y > src[i][j].Y) minp.Y = src[i][j].Y;
      if (maxp.X < src[i][j].X) maxp.X = src[i][j].X;
      if (maxp.Y < src[i][j].Y) maxp.Y = src[i][j].Y;
    }
  }

  minp.X--;
  minp.Y--;
  maxp.X++;
  maxp.Y++;

}


void do_zen_r( Paths &paths, IntPoint &minp, IntPoint &maxp )
{
  static int recur_count=0;
  int i, j, n, m;
  ClipperOffset co;
  Paths soln;
  Paths tpath;

  // We assume at least an outer boundary.  If only
  // the outer boundary is left, we've finished
  if (paths.size() <= 1)
  {
    return;
  }

  co.MiterLimit = 3.0;
  recur_count++;
  if(recur_count==400)
    return;

  if (paths.size() == 0)
    return;

  co.AddPaths( paths, jtMiter, etClosedPolygon);
  co.Execute( soln, g_scalefactor * gRadius );

  do_zen_r(soln, minp, maxp);

  //paths.insert( paths.end(), soln.begin(), soln.end() );
  n = soln.size();
  for (i=0; i<n; i++)
  {
    m = soln[i].size();
    if (m <= 2 ) continue;

    if ( (soln[i][0].X < minp.X) ||
         (soln[i][0].Y < minp.Y) ||
         (soln[i][0].X > maxp.X) ||
         (soln[i][0].Y > maxp.Y) )
      continue;

    paths.push_back(soln[i]);
  }

}



// extends outwards.  Need to do a final intersect with final (rectangle) polygon
//
void do_zen( Paths &src, Paths &dst )
{
  int i, j, n, m;
  static int recur_count=0;
  ClipperOffset co;
  Paths soln;
  Paths tpath;

  IntPoint minp, maxp;

  co.MiterLimit = 3.0;

  recur_count++;
  if(recur_count==400)
    return;

  if (src.size() == 0)
    return;

  find_min_max( src, minp, maxp );

  co.AddPaths( src, jtMiter, etClosedPolygon);
  co.Execute( soln, g_scalefactor * gRadius );

  //print_paths( soln );
  do_zen_r(soln, minp, maxp);

  dst.insert( dst.end(), soln.begin(), soln.end() );
}

void do_horizontal( Paths &src, Paths &dst )
{
  int i, j, n, m;
  Paths line_collection;
  cInt dy, h;
  cInt cury;
  IntPoint minp, maxp;


  h = 2.0 * g_scalefactor * gRadius ;
  h++;

  find_min_max( src, minp, maxp );

  cury = minp.Y;
  
  while ( cury < maxp.Y )
  {
    Path line;
    Paths soln;
    Clipper clip;

    line.push_back( IntPoint( minp.X, cury ) );
    line.push_back( IntPoint( maxp.X, cury ) );
    line.push_back( IntPoint( maxp.X, cury + h ) );
    line.push_back( IntPoint( minp.X, cury + h ) );

    cury += 2*h;

    clip.AddPath( line, ptSubject, true );
    clip.AddPaths( src, ptClip, true );
    clip.Execute( ctDifference, soln, pftNonZero, pftNonZero );

    /*
    n = soln.size();
    for (i=0; i<n; i++)
    {
      m = soln[i].size();
      for (j=0; j<m; j++)
      {
        printf("%lli %lli\n", soln[i][j].X, soln[i][j].Y);
      }
      printf("\n\n");
    }
    */

    line_collection.insert( line_collection.end(), soln.begin(), soln.end() );
  }

  dst.insert( dst.end(), line_collection.begin(), line_collection.end() );
  
}


void do_vertical( Paths &src, Paths &dst  )
{
  int i, j, n, m;
  //Paths soln;
  Paths line_collection;
  cInt dx, w;
  cInt curx;
  IntPoint minp, maxp;

  w = 2.0 * g_scalefactor * gRadius ;
  w++;

  find_min_max( src, minp, maxp );

  curx = minp.X;
  while ( curx < maxp.X )
  {
    Path line;
    Paths soln;
    Clipper clip;

    line.push_back( IntPoint( curx, minp.Y ) );
    line.push_back( IntPoint( curx, maxp.Y ) );
    line.push_back( IntPoint( curx + w, maxp.Y ) );
    line.push_back( IntPoint( curx + w, minp.Y ) );

    curx += 2*w;

    clip.AddPath( line, ptSubject, true );
    clip.AddPaths( src, ptClip, true );
    clip.Execute( ctDifference, soln, pftNonZero, pftNonZero );

    /*
    n = soln.size();
    for (i=0; i<n; i++)
    {
      m = soln[i].size();
      for (j=0; j<m; j++)
      {
        printf("%lli %lli\n", soln[i][j].X, soln[i][j].Y);
      }
      printf("\n\n");
    }
    */

    line_collection.insert( line_collection.end(), soln.begin(), soln.end() );

  }
  
  dst.insert( dst.end(), line_collection.begin(), line_collection.end() );

}


int main(int argc, char **argv)
{
  int i, j, k;
  gerber_state_t  gs;

  Paths           offset_polygons;

  Paths pgn_union;
  Paths offset;

  process_command_line_options(argc, argv);

  // Initalize and load gerber file
  //
  gerber_state_init(&gs);
  k = gerber_state_load_file(&gs, gInputFilename);
  if (k < 0)
  {
    perror(argv[1]);
    exit(errno);
  }

  // Construct library of atomic shapes and create CGAL polygons
  //
  realize_apertures(&gs);

  join_polygon_set( pgn_union, &gs );

  fprintf( gOutStream, "( union path size %lu)\n", pgn_union.size());

  fprintf( gOutStream, "%s\ng90\n", ( gMetricUnits ? "g21" : "g20" ) );


  // Offsetting is enabled if the tool radius is specified
  //
  if (gRadius > eps)
  {
    construct_polygon_offset( pgn_union, offset_polygons );

    if ( gScanLineZenGarden )
    {
      //generate_zen_gcode( gOutStream, pgn_union );
      do_zen( offset_polygons, offset_polygons );

    }
    else if ( gScanLineVertical )
    {
      do_vertical( offset_polygons, offset_polygons );
    }
    else if ( gScanLineHorizontal )
    {
      do_horizontal( offset_polygons, offset_polygons );
    }
    //else if ( gScanLineAngle ) { }

    export_paths_to_gcode( gOutStream, offset_polygons);
  }

  else 
  {
    export_paths_to_gcode( gOutStream, pgn_union );
  }

  cleanup();
  gerber_state_clear( &gs );

  exit(0);

}
