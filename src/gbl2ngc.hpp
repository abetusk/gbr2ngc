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

#ifndef GBL2NGC_HPP
#define GBL2NGC_HPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <getopt.h>

#include <math.h>

#define GBL2NGC_VERSION "0.7.0"

extern "C" {
  #include "gerber_interpreter.h"
}


#include <list>
#include <algorithm>
#include <map>

#include <string>
#include <vector>

#include "clipper.hpp"
using namespace ClipperLib;



//#define g_scalefactor 1000000.0
#define g_scalefactor 1000000000.0
//#define dtoc(x,y) IntPoint( (cInt)((double)(g_scalefactor*((double)(x)) + 0.5)), (cInt)((double)(g_scalefactor*((double)(y)) + 0.5)) )
#define dtoc(x,y) IntPoint( (cInt)(g_scalefactor*(x)), (cInt)(g_scalefactor*(y)) )
#define ctod(x) ((x)/g_scalefactor)


//----- Class definitions



class Aperture_realization {
  public:
    Aperture_realization() { };
    //~Aperture_realization() { } ;

    int m_name;
    int m_type;
    int m_crop_type;
    double m_crop[5];

    Path m_outer_boundary;
    Path m_hole;
    
    std::string m_macro_name;
    std::vector< double > m_macro_param;

    //std::vector<Point_2> m_outer_boundary;
    //std::vector<Point_2> m_hole;

};

typedef std::map<int, Aperture_realization> ApertureNameMap;
typedef std::pair<int, Aperture_realization> ApertureNameMapPair;



//------------- GLOBALS

extern int gVerboseFlag;
extern int gMetricUnits;
extern int gUnitsDefault;
extern char *gInputFilename;
extern char *gOutputFilename;
extern int gFeedRate;
extern int gSeekRate;

extern int gShowComments;
extern int gHumanReadable;

extern int gScanLineVertical;
extern int gScanLineHorizontal;
extern int gScanLineZenGarden;

extern double gZSafe;
extern double gZZero;
extern double gZCut;

extern FILE *gOutStream;
extern FILE *gInpStream;

extern double eps;
extern double gRadius;
extern double gFillRadius;

extern int gInvertFlag;
extern int gSimpleInfill;
extern int gDrawOutline;


extern std::vector<int> gApertureName;
extern ApertureNameMap gAperture;

//----- aperture functions

int realize_apertures(gerber_state_t *gs);


//----- construct functions

typedef std::vector< Paths > PathSet;


void print_polygon_set(gerber_state_t *gs);
void join_polygon_set(Paths &result, gerber_state_t *gs);

//----- export functions

void export_paths_to_gcode( FILE *ofp, Paths &paths);
void export_paths_to_gcode_unit( FILE *ofp, Paths &paths, int src_unit_0mm_1in, int dst_unit_0mm_1in);

#endif
