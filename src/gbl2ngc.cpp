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
  {"zengarden", no_argument     , 0, 'G'},
  {"verbose", no_argument       , 0, 'v'},
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

//  "route out blank areas with a horizontal scan line technique",
//  "route out blank areas with a vertical scan line technique",
  "route out blank areas with a 'zen garden' technique",

  "verbose",
  "help (this screen)"
};



void show_help(void)
{
  int i, j, k;
  int len;

  printf("a gerber to gcode converter\n");
  printf("version %s\n", GBL2NGC_VERSION);

  for (i=0; gLongOption[i].name; i++)
  {
    len = strlen(gLongOption[i].name);

    printf("  --%s", gLongOption[i].name);
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

  while ((ch = getopt_long(argc, argv, "hi:r:v", gLongOption, &option_index)) > 0) switch(ch)
  {
    case 0:
      printf("error, bad option '%s'", gLongOption[option_index].name);
      if (optarg)
        printf(" with arg '%s'", optarg);
      printf("\n");
      break;
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
}



int main(int argc, char **argv)
{
  int i, j, k;
  gerber_state_t                                gs;
  std::vector< Offset_polygon_with_holes_2 >    offset_polygon_vector;
  Polygon_set_2                                 polygon_set;
  std::vector< Polygon_with_holes_2 >           pwh_vector;
  std::vector< Polygon_with_holes_2 >           pwh_routed;

  std::vector< Polygon_2 >                      poly_vector;

  process_command_line_options(argc, argv);

  // Initalize and load gerber file
  //
  gerber_state_init(&gs);
  k = gerber_state_load_file(&gs, gInputFilename);
  if (k < 0)
  {
    perror(argv[1]);
    exit(0);
  }

  // Construct library of atomic shapes and create CGAL polygons
  //
  realize_apertures(&gs);
  join_polygon_set(polygon_set, &gs);

  fprintf( gOutStream, "%s\ng90\n", ( gMetricUnits ? "g21" : "g20" ) );

  // Offsetting is enabled if the tool radius is specified
  //
  if (gRadius > eps)
  {

    construct_polygon_offset( offset_polygon_vector, polygon_set );

    /*
    linearize_polygon_offset_vector( pwh_vector, offset_polygon_vector );

    // Regardless of whether we're filling or not, print out contour first
    //
    export_pwh_vector_to_gcode( gOutStream, pwh_vector );
    */

    // trying for a safer export
    export_offset_polygon_vector_to_gcode( gOutStream, offset_polygon_vector );

    /*
    if ( gScanLineHorizontal )
    {
      //scanline_horizontal_export_to_gcode( gOutStream, pwh_vector, gRouteRadius );
      scanline_horizontal_export_offset_polygon_vector_to_gcode( gOutStream, offset_polygon_vector, gRouteRadius );
      //generate_scanline_horizontal_test( pwh_routed, pwh_vector, gRouteRadius );
      //export_pwh_vector_to_gcode( gOutStream, pwh_routed );
    }
    else if ( gScanLineVertical )
    {
      scanline_vertical_export_to_gcode( gOutStream, pwh_vector, gRouteRadius );
      //scanline_vertical_export_to_gcode( gOutStream, pwh_vector, gRouteRadius );
    }
    else 
    */

    if ( gScanLineZenGarden )
    {
      //zen_export_to_gcode( gOutStream, pwh_vector, gRouteRadius, 0.00001 );
      zen_export_polygon_set_to_gcode( gOutStream, polygon_set, gRouteRadius, 0.00001 );
    }

  }
  else 
  {
    export_polygon_set_to_gcode( gOutStream, polygon_set );
  }

  cleanup();

  exit(0);

}
