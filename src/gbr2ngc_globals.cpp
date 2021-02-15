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

int gDebug = 0;

int gVerboseFlag = 0;
int gMetricUnits = 0;
int gUnitsDefault = 1;

char *gInputFilename = NULL;
char *gOutputFilename = NULL;
char *gConfigFilename = NULL;
char *gGCodeHeader = NULL;
char *gGCodeFooter = NULL;

int gFeedRate = 10;
int gFeedRateSet = 0;
int gSeekRate = 100;
int gSeekRateSet = 0;

int gShowComments = 1;
int gHumanReadable = 1;

int gScanLineVertical = 0;
int gScanLineHorizontal = 0;
int gScanLineZenGarden = 0;

double gZSafe = 0.1;
double gZZero = 0.0;
double gZCut = -0.05;

FILE *gOutStream = stdout;
FILE *gInpStream = stdin;
FILE *gCfgStream;

double eps = 0.000001;
double gRadius = 0.0;
double gFillRadius = -1.0;

int gInvertFlag = 0;
int gSimpleInfill = 0;
int gDrawOutline = 1;

int gMinSegment = 8;
//double gMinSegmentLengthInch = 0.001;
//double gMinSegmentLengthMM = 0.01;
double gMinSegmentLengthInch = 0.004;
double gMinSegmentLengthMM = 0.1;
double gMinSegmentLength = -1.0;

int gHeightOffset = 0;
std::string gHeightFileName;
std::string gHeightAlgorithm;

HeightMap gHeightMap;

/*
Polygon_set_2 gPolygonSet;
Offset_polygon_set_2 gOffsetPolygonSet;
std::vector< Offset_polygon_with_holes_2 > gOffsetPolygonVector;

Pwh_vector_2 gerber_list;
*/

std::vector<int> gApertureName;
ApertureNameMap gAperture;

ApertureBlockMap gApertureBlock;

struct timeval gProfileStart;
struct timeval gProfileEnd;

// local_exposure - { 1 - add, 0 - remove }
// global_exposure - { 1 - additive, 0 - subtractive}
//
//int _expose_bit(int local_exposure, int global_exposure = 1) {
int _expose_bit(int local_exposure, int global_exposure) {
  int gbit=0;
  local_exposure  = ( (local_exposure  > 0) ? 1 : 0);
  gbit = ( (global_exposure > 0) ? 0 : 1);

  return local_exposure ^ gbit;
}

