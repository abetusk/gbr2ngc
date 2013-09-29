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

#define GBL2NGC_VERSION "0.0.1"

extern "C" {
  #include "gerber_interpreter.h"
}

#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Boolean_set_operations_2.h>
#include <CGAL/ch_graham_andrew.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/Polygon_set_2.h>

#include <CGAL/Cartesian.h>
#include <CGAL/approximated_offset_2.h>
#include <CGAL/offset_polygon_2.h>
#include <CGAL/bounding_box.h>
#include <CGAL/minkowski_sum_2.h>

#include<CGAL/create_offset_polygons_from_polygon_with_holes_2.h>




#include <list>
#include <algorithm>



//------------- CGAL definitions

typedef CGAL::Exact_predicates_exact_constructions_kernel Kernel;
//typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel_epick;

typedef Kernel::Point_2                                   Point_2;
typedef CGAL::Polygon_2<Kernel>                           Polygon_2;
typedef CGAL::Polygon_with_holes_2<Kernel>                Polygon_with_holes_2;
typedef CGAL::Polygon_set_2<Kernel>                       Polygon_set_2;
//typedef CGAL::General_polygon_set_2<Kernel>                       General_polygon_set_2;

//typedef CGAL::Arrangement_2<Kernel>                       Arrangement_2;

typedef std::list<Polygon_with_holes_2>                   Pwh_list_2;
typedef std::vector<Polygon_with_holes_2>                 Pwh_vector_2;

typedef Polygon_2::Vertex_iterator                        vert_it;
typedef Polygon_with_holes_2::Hole_iterator               Hole_it;

typedef CGAL::Gps_circle_segment_traits_2<Kernel>  Gps_traits_2;
/*
typedef Gps_traits_2::Polygon_2                    Offset_polygon_2;
typedef Gps_traits_2::Polygon_with_holes_2         Offset_polygon_with_holes_2;
typedef Gps_traits_2::Point_2                      Offset_point_2;
typedef CGAL::Polygon_set_2< Gps_traits_2 >            Offset_polygon_set_2 ;
*/
typedef Gps_traits_2::General_polygon_2                    Offset_polygon_2;
typedef Gps_traits_2::General_polygon_with_holes_2         Offset_polygon_with_holes_2;
typedef Gps_traits_2::Point_2                              Offset_point_2;
typedef CGAL::General_polygon_set_2< Gps_traits_2 >            Offset_polygon_set_2 ;
//typedef CGAL::Polygon_set_2            Offset_polygon_set_2 ;

typedef CGAL::Arr_segment_traits_2<Kernel>        Arr_traits_2;
typedef CGAL::Arrangement_2<Arr_traits_2>         Arrangement_2;

typedef Gps_traits_2::X_monotone_curve_2                      X_monotone_curve_2;


//----- Class definitions




class Aperture_realization {
  public:
    Aperture_realization() { };
    //~Aperture_realization() { } ;

    int m_name;
    int m_type;
    int m_crop_type;
    double m_crop[5];
    std::vector<Point_2> m_outer_boundary;
    std::vector<Point_2> m_hole;

};

typedef std::map<int, Aperture_realization> ApertureNameMap;
typedef std::pair<int, Aperture_realization> ApertureNameMapPair;



//------------- GLOBALS

extern int gVerboseFlag;
extern int gMetricUnits;
extern char *gInputFilename;
extern char *gOutputFilename;
extern int gFeedRate;
extern int gSeekRate;

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
extern double gRouteRadius;

extern Polygon_set_2 gPolygonSet;
extern Offset_polygon_set_2 gOffsetPolygonSet;
extern std::vector< Offset_polygon_with_holes_2 > gOffsetPolygonVector;

extern Pwh_vector_2 gerber_list;

extern std::vector<int> gApertureName;
extern ApertureNameMap gAperture;

//extern int (*function_code_handler[13])(gerber_state_t *, char *);



//----- debug functions

void dump_polygon_set(Polygon_set_2 &polygon_set);
void dump_offset_polygon_set(Offset_polygon_set_2 &offset_polygon_set);
void dump_offset_polygon_set2(int count, Offset_polygon_set_2 &offset_polygon_set);
void dump_offset_polygon_vector(std::vector< Offset_polygon_with_holes_2 > &offset_polygon_vector);

//void debug_print_p_vec( std::vector< Gerber_point_2 > &p, HolePosMap &hole_map);
void print_linearized_offset_polygon_with_holes( Offset_polygon_with_holes_2 &offset );
void print_linearized_offset_polygon_with_holes2( int count, Offset_polygon_with_holes_2 &offset );
void print_profile(struct timeval *tv0, struct timeval *tv1);

//----- aperture functions

int realize_apertures(gerber_state_t *gs);


//----- construct functions

int construct_contour_region( std::vector< Polygon_with_holes_2 > &pwh_vec, contour_ll_t *contour);
void join_polygon_set(Polygon_set_2 &polygon_set, gerber_state_t *gs);


//----- export functions

void export_polygon_set_to_gcode( FILE *ofp, Polygon_set_2 &polygon_set);
void export_pwh_vector_to_gcode( FILE *ofp, std::vector< Polygon_with_holes_2 > &pwh_vector );
void export_polygon_vector_to_gcode( FILE *ofp, std::vector< Polygon_with_holes_2 > &pwh_vector );
void export_offset_polygon_vector_to_gcode( FILE *ofp, std::vector< Offset_polygon_with_holes_2 > &offset_polygon_vector );

//----- offset functions

void linearize_polygon_offset( Polygon_with_holes_2 &pwh, Offset_polygon_with_holes_2 offset );
void linearize_polygon_offset_vector( std::vector< Polygon_with_holes_2 > &pwh_vector, std::vector< Offset_polygon_with_holes_2 > &offset_vec);
//void linearize_polygon_offset_vector2( Polygon_set_2 &polygon_set,  std::vector< Offset_polygon_with_holes_2 > &offset_vec);

int check_polygon_set_ok(Polygon_set_2 &polygon_set);

void construct_polygon_offset( std::vector< Offset_polygon_with_holes_2 > &pwh_vector, Polygon_set_2 &polygon_set);
void construct_polygon_contours( std::vector< Polygon_2 > &poly_vector, Polygon_set_2 &polygon_set );
void construct_pwh_contours( std::vector< Polygon_with_holes_2 > &pwh_vector, Polygon_set_2 &polygon_set );

void find_bounding_box(double &xmin, double &ymin, double &xmax, double &ymax, Polygon_set_2 &polygon_set);
void find_bounding_box_pwh_vector(  double &xmin, double &ymin, double &xmax, double &ymax, std::vector<Polygon_with_holes_2> &pwh_vector);

void generate_zen_pwh( Polygon_with_holes_2 &zen_pwh, Polygon_with_holes_2 &pwh_orig, double radius, double err_bound = 0.00001 );
void generate_zen_unfilled_regions( std::vector<Polygon_with_holes_2> &pwh_vector_rop, std::vector<Polygon_with_holes_2> &pwh_vector_op, double radius, double err_bound = 0.00001);

//void generate_zen_pwh( Polygon_with_holes_2 &zen_pwh, Polygon_with_holes_2 &pwh_orig, double radius, double err_bound );
//void generate_zen_unfilled_regions( std::vector<Polygon_with_holes_2> &pwh_vector_rop, std::vector<Polygon_with_holes_2> &pwh_vector_op, double radius, double err_bound );

void generate_scanline_horizontal_test( std::vector<Polygon_with_holes_2> &pwh_vector_rop, std::vector<Polygon_with_holes_2> &pwh_vector_op, double radius);
void generate_scanline_horizontal( std::vector<Polygon_with_holes_2> &pwh_vector_rop, std::vector<Polygon_with_holes_2> &pwh_vector_op, double radius);

void generate_scanline_unfilled_regions(void);

void scanline_horizontal_export_to_gcode( FILE *ofp, std::vector< Polygon_with_holes_2 > &pwh_vector, double radius);

void scanline_horizontal_export_offset_polygon_vector_to_gcode(
  FILE *ofp,
  std::vector< Offset_polygon_with_holes_2 > &opwh_vector,
  double radius);

void scanline_vertical_export_to_gcode( FILE *ofp, std::vector< Polygon_with_holes_2 > &pwh_vector, double radius);
void zen_export_to_gcode( FILE *ofp, std::vector< Polygon_with_holes_2 > &pwh_vector, double radius, double err_bound = 0.00001 );
void zen_export_polygon_set_to_gcode( FILE *ofp, Polygon_set_2 &polygon_set, double radius, double err_bound = 0.00001 );


#endif
