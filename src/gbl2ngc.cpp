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

// gbl2ngc will look here by default for a config file
#define DEFAULT_CONFIG_FILENAME "./gbl2ngc.ini"

// Users should specify boolean options as "yes" or "no"
// (without quotes).
//
// e.g.
//  - verbose = yes|no
//
#define CONFIG_FILE_YES "yes"
#define CONFIG_FILE_NO "no"

// There are only 26 letters in the alphabet; 52 including uppercase. As the
// program grows in complexity and cabability, the letter choices available
// for command-line arguments will get slim. One solution is to assign numbers
// instead of letters for less-commonly used or advanced options.
#define ARG_GCODE_HEADER '2'
#define ARG_GCODE_FOOTER '3'

struct option gLongOption[] =
{
  {"radius" , required_argument , 0, 'r'},
  {"fillradius" , required_argument , 0, 'F'},

  {"input"  , required_argument , 0, 'i'},
  {"output" , required_argument , 0, 'o'},
  {"config-file", required_argument , 0, 'c'},

  {"feed"   , required_argument , 0, 'f'},
  {"seek"   , required_argument , 0, 's'},

  {"zsafe"  , required_argument , 0, 'z'},
  {"zcut"   , required_argument , 0, 'Z'},

  {"gcode-header", required_argument, 0, ARG_GCODE_HEADER},
  {"gcode-footer", required_argument, 0, ARG_GCODE_FOOTER},

  {"metric" , no_argument       , 0, 'M'},
  {"inches" , no_argument       , 0, 'I'},
//  {"scan"   , no_argument       , 0, 'H'},
//  {"scanvert",no_argument       , 0, 'V'},

  {"no-comment", no_argument      , 0, 'C'},
  {"machine-readable", no_argument      , 0, 'R'},

  {"horizontal", no_argument     , 0, 'H'},
  {"vertical", no_argument       , 0, 'V'},
  {"zengarden", no_argument     , 0, 'G'},

  {"print-polygon", no_argument     , 0, 'P'},

  {"invertfill", no_argument       , &gInvertFlag, 1},
  {"simple-infill", no_argument       , &gSimpleInfill, 1},
  {"no-outline", no_argument       , &gDrawOutline, 0},

  {"verbose", no_argument       , 0, 'v'},
  {"version", no_argument       , 0, 'N'},
  {"help"   , no_argument       , 0, 'h'},

  {0, 0, 0, 0}
};

char gOptionDescription[][1024] =
{
  "radius (default 0)",
  "radius to be used for fill pattern (default to radius above)",

  "input file",
  "output file (default stdout)",
  "configuration file (default ./gbl2ngc.ini)",

  "feed rate (default 10)",
  "seek rate (default 100)",

  "z safe height (default 0.1 inches)",
  "z cut height (default -0.05 inches)",

  "prepend custom G-code to the beginning of the program",
  "append custom G-code to the end of the program",

  "units in metric",
  "units in inches",

  "do not show comments",
  "machine readable (uppercase, no spaces in gcode)",

  "route out blank areas with a horizontal scan line technique",
  "route out blank areas with a vertical scan line technique",
  "route out blank areas with a 'zen garden' technique",

  "print polygon regions only (for debugging)",

  "invert the fill pattern (experimental)",
  "infill copper polygons with pattern (currently only -H and -V supported)",
  "do not route out outline when doing infill",

  "verbose",
  "display version information",
  "help (this screen)",

  "n/a"

};

int gPrintPolygon = 0;

option lookup_option_by_name(const char* name)
{
  int i;

  for (i = 0; gLongOption[i].name; i++) {
    const option opt = gLongOption[i];
    if (strcmp(opt.name, name) == 0) {
      return opt;
    }
  }

  const option no_option = {0, 0, 0, 0};

  return no_option;
}

void show_help(void)
{
  int i, j;
  int len;

  printf("\ngbl2ngc: A gerber to gcode converter\n");
  printf("Version %s\n", GBL2NGC_VERSION);

  for (i=0; gLongOption[i].name; i++)
  {
    len = strlen(gLongOption[i].name);

    if (gLongOption[i].flag != 0) {
      printf("  --%s", gLongOption[i].name);
      len -= 4;
    } else {
      printf("  -%c, --%s", gLongOption[i].val, gLongOption[i].name);
    }

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
  printf("\n");

}

// Use as a go-between for the internal globals and optarg,
// which could either come from a config file or be NULL if
// the option was given as a command line switch.
// `default_` is the value that is returned if the option is
// set to CONFIG_FILE_YES or given as a CLI switch.
bool bool_option(const char* optarg, bool default_ = true)
{
  if (optarg == NULL) {
    return default_;
  }

  else if (strcmp(optarg, CONFIG_FILE_YES) == 0) {
    return default_;
  }

  else if (strcmp(optarg, CONFIG_FILE_NO) == 0) {
    return !default_;
  }

  // Treat all other values for optarg as NO
  return !default_;
}

bool set_option(const char option_char, const char* optarg) {

  switch(option_char)
  {
    case 'C':
      gShowComments = bool_option(optarg, 0);
      break;
    case 'R':
      gHumanReadable = bool_option(optarg, 0);
      break;
    case 'r':
      gRadius = atof(optarg);
      break;
    case 'F':
      gFillRadius = atof(optarg);
      break;
    case '$':
      gFillRadius = atof(optarg);
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

    case ARG_GCODE_HEADER:
      gGCodeHeader = strdup(optarg);
      break;
    case ARG_GCODE_FOOTER:
      gGCodeFooter = strdup(optarg);

    case 'I':
      gMetricUnits = bool_option(optarg, 0);
      gUnitsDefault = 0;
      break;
    case 'M':
      gMetricUnits = bool_option(optarg);
      gUnitsDefault = 0;
      break;

    case 'H':
      gScanLineHorizontal = bool_option(optarg);
      break;
    case 'V':
      gScanLineVertical = bool_option(optarg);
      break;
    case 'G':
      gScanLineZenGarden = bool_option(optarg);
      break;

    case 'P':
      gPrintPolygon = bool_option(optarg);
      break;

    case 'v':
      gVerboseFlag = bool_option(optarg);
      break;
    default:
      return false;
      break;
  }

  return true;
}

void process_config_file_options() {

  if (!gConfigFilename) {
    gConfigFilename = strdup(DEFAULT_CONFIG_FILENAME);
  }

  if (! (gCfgStream = fopen(gConfigFilename, "r"))) {
    if (strcmp(gConfigFilename, DEFAULT_CONFIG_FILENAME) == 0) {
      return;
    }

    fprintf(stderr, "Can't load configuration: ");
    perror(gOutputFilename);
    exit(1);
  }

  char option_name[64];
  char option_value[64];
  char line[256] = {'\0'};

  while (fgets(line, sizeof(line), gCfgStream)) {

    if (strcmp(line, "\n") == 0) {
      // printf("skipping blank line");
    }

    else if (line[0] == ';') {
      // printf("skipping comment");
    }

    else {
      memset(option_name, 0, sizeof(option_name));
      memset(option_value, 0, sizeof(option_value));

      const char* name_end = strpbrk(line, " \t=");
      const char* value_start = name_end + strspn(name_end, " \t=");
      strncpy(option_name, line, name_end - line);
      strncpy(option_value, value_start, strlen(value_start)-1);

      const char option_char = lookup_option_by_name(option_name).val;
      set_option(option_char, option_value);
    }
  }

  fclose(gCfgStream);
}

void process_command_line_options(int argc, char **argv)
{

  extern char *optarg;
  extern int optind, opterr;
  int option_index;

  char ch;

  gFillRadius = -1.0;

  // Check if a custom path for the configuration file was given. If so,
  // those options need to be loaded before applying the command-line
  // options.

  opterr = 0;  // disable error messages for extra arguments

  while ((ch = getopt_long(argc, argv, "-c:", gLongOption, &option_index)) >= 0) {
    if (ch == 'c') {
      gConfigFilename = strdup(optarg);
      break;
    }
  }

  optind = 0;

  process_config_file_options();

  // Now the cli args and be applied. 'c:' is still included in the
  // argstring so that it won't be defaulted as a bad option.

  opterr = 1;  // this time extra arguments SHOULD raise errors

  while ((ch = getopt_long(argc, argv, "i:o:c:r:s:z:Z:f:IMHVGvNhCRF:P", gLongOption, &option_index)) >= 0) {
    switch(ch) {
      case 0:
        // long option
        //
        break;
      case 'N':
      case 'h':
        show_help();
        exit(0);
        break;

      case 'i':
        gInputFilename = strdup(optarg);
        break;
      case 'o':
        gOutputFilename = strdup(optarg);
        break;
      case 'c':
        // Do nothing, but don't go to default!
        break;

      default:
        if (!set_option(ch, optarg)) {
          printf("bad option: -%c %s\n", ch, optarg);
          show_help();
          exit(1);
        }
        break;
    }

  }

  if (gFillRadius <= 0.0)
    gFillRadius = gRadius;

  if (gOutputFilename)
  {
    if (!(gOutStream = fopen(gOutputFilename, "w")))
    {
      perror(gOutputFilename);
      exit(1);
    }
  }

  if (!gInputFilename)
  {
    fprintf(stderr, "ERROR: Must provide input file\n");
    show_help();
    exit(1);
  }

  if ( ((gScanLineHorizontal + gScanLineVertical + gScanLineZenGarden)>0) &&
       ((gRadius < eps) && (gFillRadius < eps)) ) {
    fprintf(stderr, "ERROR: Radius (-r) or fill radius (-F) must be specified for fill options (-H, -V or -G)\n");
    show_help();
    exit(1);
  }

  if ((gSimpleInfill>0) && (gRadius >= eps)) {
    fprintf(stderr, "ERROR: Cannot specify offset radius (-r) for simple infills (-H or -V)\n");
    show_help();
    exit(1);
  }

  if ((gSimpleInfill>0) && (gScanLineZenGarden>0)) {
    fprintf(stderr, "ERROR: Currently simple infills do not support the zen garden fill pattern, please use -H or -V\n");
    show_help();
    exit(1);
  }

  if (gVerboseFlag)
  {
    fprintf(gOutStream, "( radius %f )\n", gRadius);
  }

  if (gHumanReadable==0) {
    gShowComments = 0;
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
  if (gConfigFilename)
    free(gConfigFilename);

  if (gGCodeHeader)
    free(gGCodeHeader);
  if (gGCodeFooter)
    free(gGCodeFooter);
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
  unsigned int i, j;

  for (i=0; i<paths.size(); i++)
  {
    for (j=0; j<paths[i].size(); j++)
    {
      printf("%lli %lli\n", paths[i][j].X, paths[i][j].Y );
    }
    printf("\n\n");
  }

}

void find_min_max_path(Path &src, IntPoint &minp, IntPoint &maxp)
{
  int j, m;
  m = src.size();

  minp.X = 0;
  minp.Y = 0;
  maxp.X = 0;
  maxp.Y = 0;

  for (j=0; j<m; j++)
  {
    if (j==0)
    {
      minp.X = src[0].X;
      minp.Y = src[0].Y;
      maxp.X = src[0].X;
      maxp.Y = src[0].Y;
    }

    if (minp.X > src[j].X) minp.X = src[j].X;
    if (minp.Y > src[j].Y) minp.Y = src[j].Y;
    if (maxp.X < src[j].X) maxp.X = src[j].X;
    if (maxp.Y < src[j].Y) maxp.Y = src[j].Y;
  }

  minp.X--;
  minp.Y--;
  maxp.X++;
  maxp.Y++;
}

void find_min_max(Paths &src, IntPoint &minp, IntPoint &maxp)
{
  int i, j, n, m;

  minp.X = 0;
  minp.Y = 0;
  maxp.X = 0;
  maxp.Y = 0;

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
  int i, n, m;
  unsigned int k;
  ClipperOffset co;
  Paths soln;
  Paths tpath;

  IntPoint minpathp, maxpathp;

  // We assume at least an outer boundary.  If only
  // the outer boundary is left, we've finished
  //
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
  co.Execute( soln, 2.0 * g_scalefactor * gFillRadius );

  do_zen_r(soln, minp, maxp);

  n = soln.size();
  for (i=0; i<n; i++)
  {
    m = soln[i].size();
    if (m <= 2 ) continue;


    for (k=0; k<soln[i].size(); k++) {
      if ( (soln[i][k].X < minp.X) ||
           (soln[i][k].Y < minp.Y) ||
           (soln[i][k].X > maxp.X) ||
           (soln[i][k].Y > maxp.Y) )
        break;
    }
    if (k<soln[i].size()) { continue; }

    /*
    if ( (soln[i][0].X < minp.X) ||
         (soln[i][0].Y < minp.Y) ||
         (soln[i][0].X > maxp.X) ||
         (soln[i][0].Y > maxp.Y) )
      continue;
      */

    paths.push_back(soln[i]);
  }

}



// extends outwards.  Need to do a final intersect with final (rectangle) polygon
//
void do_zen( Paths &src, Paths &dst )
{
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
  co.Execute( soln, 2.0 * g_scalefactor * gFillRadius );

  do_zen_r(soln, minp, maxp);

  dst.insert( dst.end(), soln.begin(), soln.end() );
}

void do_horizontal( Paths &src, Paths &dst )
{
  Paths line_collection;
  cInt h;
  cInt cury;
  IntPoint minp, maxp;

  h = 2.0 * g_scalefactor * gFillRadius;
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

    line_collection.insert( line_collection.end(), soln.begin(), soln.end() );
  }

  dst.insert( dst.end(), line_collection.begin(), line_collection.end() );
}

void do_horizontal_infill( Paths &src, Paths &dst )
{
  Paths line_collection;
  cInt h;
  cInt cury;
  IntPoint minp, maxp;


  h = 2.0 * g_scalefactor * gFillRadius;
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

    clip.AddPaths( src, ptSubject, true );
    clip.AddPath( line, ptClip, true );
    clip.Execute( ctIntersection, soln, pftNonZero, pftNonZero );

    line_collection.insert( line_collection.end(), soln.begin(), soln.end() );
  }

  dst.insert( dst.end(), line_collection.begin(), line_collection.end() );

  if (gDrawOutline) {
    dst.insert( dst.end(), src.begin(), src.end());
  }

}


void do_vertical( Paths &src, Paths &dst  )
{
  Paths line_collection;
  cInt w;
  cInt curx;
  IntPoint minp, maxp;

  w = 2.0 * g_scalefactor * gFillRadius ;
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

    line_collection.insert( line_collection.end(), soln.begin(), soln.end() );
  }

  dst.insert( dst.end(), line_collection.begin(), line_collection.end() );
}

void do_vertical_infill( Paths &src, Paths &dst  )
{
  Paths line_collection;
  cInt w;
  cInt curx;
  IntPoint minp, maxp;

  w = 2.0 * g_scalefactor * gFillRadius ;
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

    clip.AddPaths( src, ptSubject, true );
    clip.AddPath( line, ptClip, true );
    clip.Execute( ctIntersection, soln, pftNonZero, pftNonZero );

    line_collection.insert( line_collection.end(), soln.begin(), soln.end() );
  }

  dst.insert( dst.end(), line_collection.begin(), line_collection.end() );

  if (gDrawOutline) {
    dst.insert( dst.end(), src.begin(), src.end());
  }
}

void invert(Paths &src, Paths &dst) {
  unsigned int i;
  ClipperOffset co;
  Clipper clip_stencil;

  Paths stencil;
  Paths oot;
  Path p;

  // Construct outline stencil
  //
  for (i=0; i<src.size(); i++) {
    p = src[i];
    if (Area(p) < 0.0) { std::reverse(p.begin(), p.end()); }
    clip_stencil.AddPath(p, ptSubject, true);
  }
  clip_stencil.Execute(ctUnion, stencil, pftNonZero, pftNonZero);

  for (i=0; i<stencil.size(); i++) {
    dst.push_back(stencil[i]);
  }

  // Reverse all paths for the inversion
  //
  for (i=0; i<src.size(); i++) {
    p = src[i];
    std::reverse(p.begin(), p.end());
    dst.push_back(p);
  }

}

void dump_options() {
  printf("input = %s\n", gInputFilename);
  printf("output = %s\n", gOutputFilename);
  printf("config-file = %s", gConfigFilename);

  printf("radius = %f\n", gRadius);
  printf("fillradius = %f\n", gFillRadius);

  printf("feed = %d\n", gFeedRate);
  printf("seek = %d\n", gSeekRate);

  printf("zsafe = %f\n", gZSafe);
  printf("zcut = %f\n", gZCut);

  printf("metric = %d\n", gMetricUnits);

  printf("no-comment = %d\n", !gShowComments);
  printf("machine-readable = %d\n", !gHumanReadable);

  printf("horizinal = %d\n", gScanLineHorizontal);
  printf("vertical = %d\n", gScanLineVertical);
  printf("zengarden = %d\n", gScanLineZenGarden);

  printf("verbose = %d\n", gVerboseFlag);
}


int main(int argc, char **argv)
{
  int k;
  gerber_state_t gs;

  Paths offset_polygons;

  Paths pgn_union;
  Paths offset;

  process_command_line_options(argc, argv);
  // dump_options();

  // Initalize and load gerber file
  //
  gerber_state_init(&gs);
  k = gerber_state_load_file(&gs, gInputFilename);
  if (k < 0)
  {
    perror(argv[1]);
    exit(errno);
  }

  // Construct library of atomic shapes and create polygons
  //
  realize_apertures(&gs);

  if (gPrintPolygon) {
    print_polygon_set(&gs);
    exit(0);
  }

  join_polygon_set( pgn_union, &gs );



  if (gShowComments) {
    fprintf( gOutStream, "( union path size %lu )\n", pgn_union.size());
  }

  // If units haven't been specified on the command line,
  // inherit units from the Gerber file.
  //
  if (gUnitsDefault) {
    gMetricUnits = gs.units_metric;
  }

  // G20 - inch
  // G21 - mm
  if (gHumanReadable) {
    fprintf( gOutStream, "%s\ng90\n", ( gMetricUnits ? "g21" : "g20" ) );
  } else {
    fprintf( gOutStream, "%s\nG90\n", ( gMetricUnits ? "G21" : "G20" ) );
  }

  if ((gSimpleInfill>0) && (gFillRadius > eps))
  {
    Paths fin_polygons;

    //if      ( gScanLineZenGarden )  { do_zen( inverted_polygons, offset_polygons); }
    if      ( gScanLineVertical )   { do_vertical_infill( pgn_union, fin_polygons); }
    else if ( gScanLineHorizontal ) { do_horizontal_infill( pgn_union, fin_polygons); }
    else { //error
      fprintf(stderr, "unsupported command (for simple-infill)\n");
      exit(1);
    }

    export_paths_to_gcode_unit(gOutStream, fin_polygons, gs.units_metric, gMetricUnits);
  }

  // Offsetting is enabled if the tool radius is specified
  //
  else if ((gRadius > eps) || (gFillRadius > eps))
  {
    construct_polygon_offset( pgn_union, offset_polygons );

    if (gInvertFlag) {
      Paths inverted_polygons;

      if (gShowComments) { fprintf( gOutStream, "( inverted selection radius %f, fill radius %f )\n", gRadius, gFillRadius ); }

      invert(offset_polygons, inverted_polygons);
      if      ( gScanLineZenGarden )  { do_zen( inverted_polygons, offset_polygons); }
      else if ( gScanLineVertical )   { do_vertical( inverted_polygons, offset_polygons); }
      else if ( gScanLineHorizontal ) { do_horizontal( inverted_polygons, offset_polygons); }

    } else {

      if      ( gScanLineZenGarden )  { do_zen( offset_polygons, offset_polygons ); }
      else if ( gScanLineVertical )   { do_vertical( offset_polygons, offset_polygons ); }
      else if ( gScanLineHorizontal ) { do_horizontal( offset_polygons, offset_polygons ); }

    }

    export_paths_to_gcode_unit(gOutStream, offset_polygons, gs.units_metric, gMetricUnits);
  }
  else
  {
    export_paths_to_gcode_unit(gOutStream, pgn_union, gs.units_metric, gMetricUnits);
  }

  cleanup();
  gerber_state_clear( &gs );

  exit(0);
}
