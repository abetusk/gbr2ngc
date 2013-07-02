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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

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


#include <list>
#include <algorithm>

//------------- GLOBALS 

int gVerboseFlag = 0;
int gMetricUnits = 0;
char *gInputFilename = NULL;
char *gOutputFilename = NULL;
int gFeedRate = 100;
//int gSeekRate = 100;

double gZSafe = 0.1;
double gZZero = 0.0;
double gZCut = -0.05;

FILE *gOutStream = stdout;
FILE *gInpStream = stdin;

//------------- GLOBALS 

typedef CGAL::Exact_predicates_exact_constructions_kernel Kernel;
//typedef CGAL::Exact_predicates_inexact_constructions_kernel Kernel;
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

Polygon_set_2 gPolygonSet;
Offset_polygon_set_2 gOffsetPolygonSet;
std::vector< Offset_polygon_with_holes_2 > gOffsetPolygonVector;

Pwh_vector_2 gerber_list;

void dump_polygon_set(Polygon_set_2 &polygon_set);
void dump_offset_polygon_set(Offset_polygon_set_2 &offset_polygon_set);
void dump_offset_polygon_set2(int count, Offset_polygon_set_2 &offset_polygon_set);
void dump_offset_polygon_vector(std::vector< Offset_polygon_with_holes_2 > &offset_polygon_vector);

double eps = 0.000001;
double gRadius = 0.0;

void dump_poly_set(void)
{
  std::list<Polygon_with_holes_2> pwh_list;
  std::list<Polygon_with_holes_2>::const_iterator it;

  gPolygonSet.polygons_with_holes (std::back_inserter(pwh_list));

  for (it = pwh_list.begin(); it != pwh_list.end(); ++it)
  {
    Polygon_with_holes_2 pwh = *it;

    std::cout << "\n#outer boundary:\n";

    Polygon_2 ob = pwh.outer_boundary();
    vert_it vit;
    for (vit = ob.vertices_begin(); vit != ob.vertices_end(); ++vit)
    {
      std::cout << *vit << "\n";
    }

    std::cout << "\n#inner holes:\n";

    Polygon_with_holes_2::Hole_const_iterator hit;
    for (hit = pwh.holes_begin(); hit != pwh.holes_end(); ++hit)
    {

      Polygon_2 ib = *hit;

      std::cout << "\n";

      for (vit = ib.vertices_begin(); vit != ib.vertices_end(); ++vit)
      {
        std::cout << *vit << "\n";
      }

    }

  }
}

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

std::vector<int> gApertureName;
ApertureNameMap gAperture;


// Construct polygons that will be used for the apertures.
// Curves are linearized.  Circles have a minium of 8
// vertices.
//
int realize_apertures(gerber_state_t *gs)
{
  double min_segment_length = 0.01;
  int min_segments = 8;
  aperture_data_block_t *aperture;

  int i, j, k;
  double r, c, theta, a;
  double x, y;
  double x_len, y_len;
  int segments;

  int n_vert;
  double rot_deg;

  for ( aperture = gs->aperture_head ;
        aperture ;
        aperture = aperture->next )
  {
    Aperture_realization ap;

    ap.m_name = aperture->name;
    ap.m_type = aperture->type;
    ap.m_crop_type = aperture->crop_type;

    switch (aperture->type)
    {
      case 0:  // circle
        r = aperture->crop[0]/2.0;
        c = 2.0 * M_PI * r;

        theta = 2.0 * asin( min_segment_length / (2.0*r) );
        segments = (int)(c / theta);
        if (segments < min_segments)
          segments = min_segments;

        for (int i=0; i<segments; i++)
        {
          a = 2.0 * M_PI * (double)i / (double)segments ;
          ap.m_outer_boundary.push_back( Point_2( r*cos( a ), r*sin( a ) ) );
        }

        break;
      case 1:  // rectangle
        x = aperture->crop[0]/2.0;
        y = aperture->crop[1]/2.0;
        ap.m_outer_boundary.push_back( Point_2( -x, -y ) );
        ap.m_outer_boundary.push_back( Point_2(  x, -y ) );
        ap.m_outer_boundary.push_back( Point_2(  x,  y ) );
        ap.m_outer_boundary.push_back( Point_2( -x,  y ) );
        break;
      case 2:  // obround

        x_len = aperture->crop[0];
        y_len = aperture->crop[1];

        if (x_len < y_len)
        {
          r = x_len / 2.0;

          // start at the top right
          for (int i=0; i <= (segments/2); i++)
          {
            a = 2.0 * M_PI * (double)i / (double)segments ;
            ap.m_outer_boundary.push_back( Point_2( r*cos(a), r*sin(a) + ((y_len/2.0) - r) ) );
          }

          // then the bottom
          for (int i = (segments/2); i <= segments; i++)
          {
            a = 2.0 * M_PI * (double)i / (double)segments ;
            ap.m_outer_boundary.push_back( Point_2( r*cos(a), r*sin(a) - ((y_len/2.0) - r) ) );
          }

        }
        else if (y_len < x_len)
        {
          r = y_len / 2.0;

          // start at bottom right
          for (int i=0; i <= (segments/2); i++)
          {
            a = ( 2.0 * M_PI * (double)i / (double)segments ) - ( M_PI / 2.0 );
            ap.m_outer_boundary.push_back( Point_2( r*cos(a) + ((x_len/2.0) - r) , r*sin(a) ) );
          }

          // then the left
          for (int i=(segments/2); i <= segments; i++)
          {
            a = ( 2.0 * M_PI * (double)i / (double)segments ) - ( M_PI / 2.0 );
            ap.m_outer_boundary.push_back( Point_2( r*cos(a) - ((x_len/2.0) - r) , r*sin(a) ) );
          }

        }
        else  // circle
        {
          r = x_len / 2.0;
          for (int i=0; i<segments; i++)
          {
            a = 2.0 * M_PI * (double)i / (double)segments ;
            ap.m_outer_boundary.push_back( Point_2( r*cos( a ), r*sin( a ) ) );
          }
        }

        break;
      case 3:  // polygon

        r = aperture->crop[0];
        n_vert = (int)aperture->crop[1];
        rot_deg = aperture->crop[2];

        if ((n_vert < 3) || (n_vert > 12))
        {
          printf("ERROR! Number of polygon vertices out of range\n");
        }

        for (int i=0; i<n_vert; i++)
        {
          a = (2.0 * M_PI * (double)i / (double)n_vert) + (rot_deg * M_PI / 180.0);
          ap.m_outer_boundary.push_back( Point_2( r*cos( a ), r*sin( a ) ) );
        }

        break;
      default: break;
    }

    int base = 0;
    switch (aperture->type)
    {
      case 0: base = 1; break;
      case 1: base = 2; break;
      case 2: base = 3; break;
      case 3: base = 3; break;
      default: break;
    }

    switch (aperture->crop_type)
    {
      case 0: // solid, do nothing
        break;
      case 1: // circle hole
        r = aperture->crop[base]/2.0;
        c = 2.0 * M_PI * r;

        theta = 2.0 * asin( min_segment_length / (2.0*r) );
        segments = (int)(c / theta);
        if (segments < min_segments)
          segments = min_segments;

        for (i=0; i<segments; i++)
        {
          a = -2.0 * M_PI * (double)i / (double)segments;  // counter clockwise
          ap.m_outer_boundary.push_back( Point_2( r*cos( a ), r*sin( a ) ) );
        }

        break;
      case 2: // rect hole
        x = aperture->crop[base]/2.0;
        y = aperture->crop[base+1]/2.0;
        ap.m_hole.push_back( Point_2( -x, -y ) );  //clock wise instead of contouer clockwise
        ap.m_hole.push_back( Point_2( -x,  y ) );
        ap.m_hole.push_back( Point_2(  x,  y ) );
        ap.m_hole.push_back( Point_2(  x, -y ) );
        break;
      default: break;
    }

    gAperture.insert( ApertureNameMapPair(ap.m_name, ap) );
    gApertureName.push_back(ap.m_name);

  }

}

//-----------------------------------------


class Gerber_point_2 {
  public:
    int ix, iy;
    double x, y;
};

class irange_2 {
  public:
    int start, end;
};

typedef struct dpoint_2_type {
  int ix, iy;
  double x, y;
} dpoint_2_t;

struct Gerber_point_2_cmp
{
  bool operator()(const Gerber_point_2 &lhs, const Gerber_point_2 &rhs)
  {
    if (lhs.x < rhs.x) return true;
    if (lhs.x > rhs.x) return false;
    if (lhs.y < rhs.y) return true;
    return false;
  }
};

typedef std::map< Gerber_point_2, int, Gerber_point_2_cmp  > PointPosMap;
typedef std::pair< Gerber_point_2, int > PointPosMapPair;

typedef std::map< Gerber_point_2, irange_2 , Gerber_point_2_cmp  > HolePosMap;
typedef std::pair< Gerber_point_2, irange_2 > HolePosMapPair;


// add a hole to the vector hole_vec from the Gerber_point_2
// vector p.  hole_map holds the start and end of the hole points.
// KiCAD (and others?) make the region so that there are two
// pairs of overlapping points.  Here is some ASCII art to illustrate:
//
//            >...     ...>
//                \   /
//                 a a
//                 | |
//                 | |
//            <----b b-----<
//
// where a is a distinct point from b and the 'a' points overlap
// and the 'b' points overlap.  The hole really starts (and ends)
// at point 'a'.  The outer region connects to the inner hole
// through point 'b'.
// start and end hold the positions the starting 'a' and ending 'a'
// point in the p vector.
//
void add_hole( std::vector< Polygon_2 > &hole_vec, 
               std::vector< Gerber_point_2 > &p, 
               HolePosMap &hole_map, 
               int start, 
               int end )
{
  int i, j, k;
  Polygon_2 hole_polygon;
  HolePosMap::iterator hole_map_it;
  irange_2 hole_range;

  // First point and end point are duplicated,
  // Put first point on and skip over last point.
  //
  hole_polygon.push_back( Point_2(p[start].x, p[start].y) );

  for (i=start+1; i<end; i++)
  {

    hole_polygon.push_back( Point_2(p[i].x, p[i].y) );

    hole_map_it = hole_map.find( p[i] );
    if ( hole_map_it != hole_map.end() )
    {
      hole_range = (*hole_map_it).second;

      if (hole_range.end < 0) 
        continue;

      // recursively add hole
      //
      add_hole( hole_vec, p, hole_map, hole_range.start+1, hole_range.end-1 );

      // shoot past hole
      //
      i = hole_range.end;

    }
  }

  hole_vec.push_back( hole_polygon );

}


// Copy linked list points in contour to vector p.
//
void populate_gerber_point_vector_from_contour( std::vector< Gerber_point_2 > &p, 
                                                contour_ll_t *contour) 
{
  Gerber_point_2 dpnt;

  while (contour)
  {
    dpnt.ix = int(contour->x * 10000.0);
    dpnt.iy = int(contour->y * 10000.0);
    dpnt.x = contour->x;
    dpnt.y = contour->y;

    p.push_back( dpnt );
    contour = contour->next;
  }

}

// Create the start and end positions of the holes in the vector p.
// Store in hole_map.
//
int create_hole_map( HolePosMap &hole_map,
                     std::vector< Gerber_point_2 > &p )
{
  int i, j, k;
  HolePosMap::iterator hole_map_it;
  irange_2 range;

  for (i=0; i<p.size(); i++)
  {
    // if found in p_map, add it to our map
    //
    hole_map_it = hole_map.find( p[i] );
    if ( hole_map_it != hole_map.end() )
    {
      range = hole_map[ p[i] ];

      if (range.end >= 0) 
      {
        // special case:
        // last point is duplicated, so ignore if that's the duplicate point
        // If it's out of range, we have a duplicate point?
        //
        if ( (range.start > 0) || (range.end < (p.size()-2)) )  
        {
          fprintf(stderr, "ERROR: possible duplicate point? (%f,%f)\n", p[i].x, p[i].y);
          return -1;
        }

        continue;
      }

      hole_map[ p[i] ].end = i;
    }
    else   // otherwise add it
    {
      range.start = i;
      range.end = -1;

      hole_map.insert( HolePosMapPair( p[i], range ) );
    }
  }

  return 0;
}

//----------------------------------

void debug_print_p_vec( std::vector< Gerber_point_2 > &p,
                        HolePosMap &hole_map)
{
  int i, j, k;
  HolePosMap::iterator hole_map_it;
  irange_2 range;

  for (i=0; i<p.size(); i++)
  {
    printf("#[%i] %f %f", i, p[i].x, p[i].y);
    hole_map_it = hole_map.find(p[i]);
    if ( hole_map_it != hole_map.end() )
    {
      range = (*hole_map_it).second;
      if (range.end >= 0)
      {
        if (range.start == i)
          printf("(*,%i)", range.end);
        else printf("(%i,*)", range.start);
      }
    }

    printf("\n");
    fflush(stdout);
  }

}

//----------------------------------

// Very similar to add_hole but special care needs to be taken for the outer
// contour.
// Construct the point vector and hole map.  Create the polygon with holes
// vector.
//
int construct_contour_region( std::vector< Polygon_with_holes_2 > &pwh_vec, 
                               contour_ll_t *contour )
{
  int i, j, k;

  std::vector< Gerber_point_2 > p;
  std::vector< Polygon_2 > hole_vec;

  HolePosMap hole_map;
  HolePosMap::iterator hole_map_it;
  irange_2 range;
  irange_2 outer_boundary_range;

  Polygon_2 outer_boundary_polygon;
  Polygon_with_holes_2 pwh;

  // Initially populate p vector
  //
  populate_gerber_point_vector_from_contour( p, contour );

  // Find the start and end regions for each of the
  // holes.
  create_hole_map( hole_map, p );

  outer_boundary_polygon.push_back( Point_2( p[0].x, p[0].y ) );

  hole_map_it = hole_map.find( p[0] );
  if (hole_map_it == hole_map.end())
  {
    printf("ERROR, first point must connect back\n");
    return -1;
  }
  outer_boundary_range = (*hole_map_it).second;

  // Treat first point as a special case
  // the last point is duplicated so we need to make
  // sure not to add it to our outer boundary again.
  // Might want to make sure that no duplicate holes get added
  // anyway...
  // 
  for ( i = outer_boundary_range.start+1 ;
        i < outer_boundary_range.end ;
        i++ )
  {

    //pwh.outer_boundary().push_back( Point_2( p[i].x, p[i].y ) );
    outer_boundary_polygon.push_back( Point_2( p[i].x, p[i].y ) );

    hole_map_it = hole_map.find( p[i] );
    if ( hole_map_it != hole_map.end() )
    {
      range = (*hole_map_it).second;

      // check for a valid hole, if not, go on
      //
      if ( range.end < 0 ) 
        continue;

      // hole start is one past this duplicate point
      // hole end is one before the other position of the duplicate point
      //
      add_hole( hole_vec, p, hole_map, range.start + 1, range.end - 1);

      // shoot past hole
      //
      i = range.end;
    }
  }

  if (outer_boundary_polygon.is_clockwise_oriented())
    outer_boundary_polygon.reverse_orientation();

  pwh.outer_boundary() = outer_boundary_polygon;

  for (i=0; i<hole_vec.size(); i++)
  {
    if (hole_vec[i].is_counterclockwise_oriented())
      hole_vec[i].reverse_orientation();

    pwh.add_hole( hole_vec[i] );
  }

  pwh_vec.push_back( pwh );

}

//-----------------------------------------

// construct the final polygon set (before offsetting) 
//   of the joined polygons from the parsed gerber file.
//
void join_polygon_set(gerber_state_t *gs)
{

  int i, j, k;
  contour_list_ll_t *contour_list;
  contour_ll_t      *contour, *prev_contour;

  std::vector< Polygon_2 > temp_poly_vec;
  std::vector< Polygon_with_holes_2 > temp_pwh_vec;

  for (contour_list = gs->contour_list_head; 
       contour_list; 
       contour_list = contour_list->next)
  {
    prev_contour = NULL;
    contour = contour_list->c;

    if (contour->region)
    {
      construct_contour_region(temp_pwh_vec, contour);
      continue;
    }

    while (contour)
    {
      if (!prev_contour)
      {
        prev_contour = contour;
        continue;
      }

      std::vector<Point_2> point_list;
      int name = contour->d_name;

      for (i=0; i<gAperture[ name ].m_outer_boundary.size(); i++)
        point_list.push_back( Point_2( gAperture[ name ].m_outer_boundary[i].x() + prev_contour->x,
                                       gAperture[ name ].m_outer_boundary[i].y() + prev_contour->y ) );

      for (i=0; i<gAperture[ name ].m_outer_boundary.size(); i++)
        point_list.push_back( Point_2( gAperture[ name ].m_outer_boundary[i].x() + contour->x,
                                       gAperture[ name ].m_outer_boundary[i].y() + contour->y ) );


      std::vector<Point_2> res_point;
      Polygon_2 poly;

      CGAL::ch_graham_andrew( point_list.begin(), point_list.end(), std::back_inserter(res_point) );

      for (i=0; i < res_point.size(); i++)
        poly.push_back( res_point[i] );

      if (poly.is_clockwise_oriented())
        poly.reverse_orientation();

      temp_poly_vec.push_back(poly);

      contour = contour->next;

    }

  }

  gPolygonSet.join(temp_poly_vec.begin(), temp_poly_vec.end() ,
                   temp_pwh_vec.begin(), temp_pwh_vec.end() );


  //DEBUG
  //dump_polygon_set( gPolygonSet );

}

//------------------------------------------

void linearize_polygon_offset_vector( Polygon_set_2 &polygon_set, 
                                      std::vector< Offset_polygon_with_holes_2 > &offset_vec)
{
  int v;
  Offset_polygon_with_holes_2::Hole_iterator hit;
  Offset_polygon_2::Curve_iterator cit;

  for (v=0; v<offset_vec.size(); v++)
  {
    Polygon_with_holes_2 pwh;
    Polygon_2 outer_boundary;
    std::list< Polygon_2 > hole_list;


    for (cit  = offset_vec[v].outer_boundary().curves_begin();
         cit != offset_vec[v].outer_boundary().curves_end();
         ++cit)
    {

      if ( (*cit).is_linear() ||
           (*cit).is_circular() )
      {
        //Point_2 p = (*cit).source();
        //Point_2 p ( (*cit).source().x(), (*cit).source().y() );
        Offset_point_2 op = (*cit).source();
        //Point_2 p ( op.x(), op.y() );
        Point_2 p ( CGAL::to_double(op.x()), CGAL::to_double(op.y()) );


        outer_boundary.push_back(p);

      }
    }

    for (hit = offset_vec[v].holes_begin(); hit != offset_vec[v].holes_end(); ++hit)
    {
      Polygon_2 hole;

      //std::cout << "\n#offset hole\n";
      for (cit = (*hit).curves_begin();
           cit != (*hit).curves_end();
           ++cit)
      {
        if ( (*cit).is_linear() ||
             (*cit).is_circular() )
        {
          //Point_2 p = (*cit).source();
          Offset_point_2 op = (*cit).source();
          //Point_2 p ( op.x(), op.y() );
          Point_2 p ( CGAL::to_double(op.x()), CGAL::to_double(op.y()) );


          hole.push_back(p);

        }
      }

      hole_list.push_back(hole);
    }


    polygon_set.insert( Polygon_with_holes_2( outer_boundary, hole_list.begin(), hole_list.end() ) );

  }

}

//------------------------------------------

void print_linearized_offset_polygon_with_holes( Offset_polygon_with_holes_2 &offset )
{

  Offset_polygon_with_holes_2::Hole_iterator hit;
  Offset_polygon_2::Curve_iterator cit;

  std::cout << "\n#offset outer boundary\n";

  for (cit  = offset.outer_boundary().curves_begin();
       cit != offset.outer_boundary().curves_end();
       ++cit)
  {
    if ( (*cit).is_linear() ||
         (*cit).is_circular() )
    {
      Offset_point_2 p = (*cit).source();
      std::cout << "#ob " << CGAL::to_double(p.x()) << " " << CGAL::to_double(p.y()) << "\n";
      //std::cout << CGAL::to_double(p.x()) << " " << CGAL::to_double(p.y()) << "\n";
    }
  }

  // display first point again (to close loop)
  for (cit  = offset.outer_boundary().curves_begin();
       cit != offset.outer_boundary().curves_end();
       ++cit)
  {
    if ( (*cit).is_linear() ||
         (*cit).is_circular() )
    {
      Offset_point_2 p = (*cit).source();
      std::cout << "#ob " << CGAL::to_double(p.x()) << " " << CGAL::to_double(p.y()) << "\n";
      //std::cout << CGAL::to_double(p.x()) << " " << CGAL::to_double(p.y()) << "\n";
      break;
    }
  }

  for (hit = offset.holes_begin(); hit != offset.holes_end(); ++hit)
  {
    std::cout << "\n#offset hole\n";
    for (cit = (*hit).curves_begin();
         cit != (*hit).curves_end();
         ++cit)
    {
      if ( (*cit).is_linear() ||
           (*cit).is_circular() )
      {
        Offset_point_2 p = (*cit).source();
        std::cout << "#ho " << CGAL::to_double(p.x()) << " " << CGAL::to_double(p.y()) << "\n";
        //std::cout << CGAL::to_double(p.x()) << " " << CGAL::to_double(p.y()) << "\n";
      }

    }

    for (cit = (*hit).curves_begin();
         cit != (*hit).curves_end();
         ++cit)
    {
      if ( (*cit).is_linear() ||
           (*cit).is_circular() )
      {
        Offset_point_2 p = (*cit).source();
        std::cout << "#ho " << CGAL::to_double(p.x()) << " " << CGAL::to_double(p.y()) << "\n";
        //std::cout << CGAL::to_double(p.x()) << " " << CGAL::to_double(p.y()) << "\n";
        break;
      }

    }
  }

}

void print_linearized_offset_polygon_with_holes2( int count, Offset_polygon_with_holes_2 &offset )
{

  Offset_polygon_with_holes_2::Hole_iterator hit;
  Offset_polygon_2::Curve_iterator cit;

  std::cout << "\n#count" << count << " offset outer boundary\n";

  for (cit  = offset.outer_boundary().curves_begin();
       cit != offset.outer_boundary().curves_end();
       ++cit)
  {
    if ( (*cit).is_linear() ||
         (*cit).is_circular() )
    {
      Offset_point_2 p = (*cit).source();
      std::cout << "#count" << count << " ob" << CGAL::to_double(p.x()) << " " << CGAL::to_double(p.y()) << "\n";
      //std::cout << CGAL::to_double(p.x()) << " " << CGAL::to_double(p.y()) << "\n";
    }
  }

  // display first point again (to close loop)
  for (cit  = offset.outer_boundary().curves_begin();
       cit != offset.outer_boundary().curves_end();
       ++cit)
  {
    if ( (*cit).is_linear() ||
         (*cit).is_circular() )
    {
      Offset_point_2 p = (*cit).source();
      std::cout << "#count " << count << " ob " << CGAL::to_double(p.x()) << " " << CGAL::to_double(p.y()) << "\n";
      //std::cout << CGAL::to_double(p.x()) << " " << CGAL::to_double(p.y()) << "\n";
      break;
    }
  }

  for (hit = offset.holes_begin(); hit != offset.holes_end(); ++hit)
  {
    std::cout << "\n#offset hole\n";
    for (cit = (*hit).curves_begin();
         cit != (*hit).curves_end();
         ++cit)
    {
      if ( (*cit).is_linear() ||
           (*cit).is_circular() )
      {
        Offset_point_2 p = (*cit).source();
        std::cout << "#count " << count << " ho " << CGAL::to_double(p.x()) << " " << CGAL::to_double(p.y()) << "\n";
        //std::cout << CGAL::to_double(p.x()) << " " << CGAL::to_double(p.y()) << "\n";
      }

    }

    for (cit = (*hit).curves_begin();
         cit != (*hit).curves_end();
         ++cit)
    {
      if ( (*cit).is_linear() ||
           (*cit).is_circular() )
      {
        Offset_point_2 p = (*cit).source();
        std::cout << "#count" << count << " ho " << CGAL::to_double(p.x()) << " " << CGAL::to_double(p.y()) << "\n";
        //std::cout << CGAL::to_double(p.x()) << " " << CGAL::to_double(p.y()) << "\n";
        break;
      }

    }
  }

}


//------------------------------------------

// Offset the joined polygons
// as they appear in gPolygonSet.
//
void construct_polygon_offset( std::vector< Offset_polygon_with_holes_2 > &pwh_vector,
                               Polygon_set_2 &polygon_set)
{
  int pwh_count=0;
  vert_it vit;
  double radius = gRadius;
  double err_bound = 0.000001;
  Offset_polygon_with_holes_2 offset;

  std::list< Polygon_with_holes_2 > pwh_list;
  std::list< Polygon_with_holes_2 >::iterator pwh_it;

  //gPolygonSet.polygons_with_holes( std::back_inserter(pwh_list) ) ;
  polygon_set.polygons_with_holes( std::back_inserter(pwh_list) ) ;

  for (pwh_it = pwh_list.begin(); pwh_it != pwh_list.end(); ++pwh_it)
  {
    offset = approximated_offset_2( *pwh_it, radius, err_bound );
    //gOffsetPolygonVector.push_back( offset );
    pwh_vector.push_back( offset );
  }

  //dump_offset_polygon_vector( gOffsetPolygonVector );
  //exit(0);

}

//------------------------------------------

void find_bounding_box(double &xmin, double &ymin,
                       double &xmax, double &ymax,
                       Polygon_set_2 &polygon_set)
{
  int first=1;
  double tx, ty;
  std::list< Polygon_with_holes_2 > pwh_list;
  std::list< Polygon_with_holes_2 >::iterator pwh_it;
  vert_it vit;
  Hole_it hit;
  Point_2 point_min, point_max;

  //gPolygonSet.polygons_with_holes( std::back_inserter(pwh_list) ) ;
  polygon_set.polygons_with_holes( std::back_inserter(pwh_list) ) ;

  for (pwh_it = pwh_list.begin(); pwh_it != pwh_list.end(); ++pwh_it)
  {
    Polygon_with_holes_2 pwh = *pwh_it;

    Polygon_2 ob = pwh.outer_boundary();
    for (vit = ob.vertices_begin(); vit != ob.vertices_end(); ++vit)
    {
      tx = CGAL::to_double( (*vit).x() );
      ty = CGAL::to_double( (*vit).y() );

      if (first)
      {
        first = 0;
        xmin = xmax = tx;
        ymin = ymax = ty;
      }

      if ( tx < xmin ) xmin = tx;
      if ( ty < ymin ) ymin = ty;
      if ( tx > xmax ) xmax = tx;
      if ( ty > ymax ) ymax = ty;
    }

    for (hit = pwh.holes_begin(); hit != pwh.holes_end(); ++hit)
    {
      for (vit = (*hit).vertices_begin(); vit != (*hit).vertices_end(); ++vit)
      {
        tx = CGAL::to_double( (*vit).x() );
        ty = CGAL::to_double( (*vit).y() );

        if ( tx < xmin ) xmin = tx;
        if ( ty < ymin ) ymin = ty;
        if ( tx > xmax ) xmax = tx;
        if ( ty > ymax ) ymax = ty;
      }
    }
  }
}

//-----------------------

void generate_zen_unfilled_regions(void)
{
}

//-----------------------

void generate_scanline_unfilled_regions(void)
{
  int i, j, k, n=1000;

  double diameter, radius;

  double xmin, xmax;
  double ymin, ymax;
  double ydel;

  double border_offset = 0.25;

  Polygon_2 containing_rectangle;
  Polygon_set_2 negative_region;

  diameter = 0.008;
  radius = diameter / 2.0;

  ydel = radius / 2.0;


  // find bounding box
  find_bounding_box(xmin, ymin, xmax, ymax, gPolygonSet);

  xmin -= border_offset;
  xmax += border_offset;
  ymin -= border_offset;
  ymax += border_offset;


  containing_rectangle.push_back( Point_2( xmin, ymin ) );
  containing_rectangle.push_back( Point_2( xmax, ymin ) );
  containing_rectangle.push_back( Point_2( xmax, ymax ) );
  containing_rectangle.push_back( Point_2( xmin, ymax ) );

  negative_region.insert( containing_rectangle );

  negative_region.difference( gPolygonSet );

  n = (int)( (ymax - ymin) / ydel );

  // construct scan lines
  for (i=0; i<=n; i++)
  {
    double y, yp;

    y = ymin + ((ymax - ymin) * (double)i / (double)n);
    //yp = y + (ydel/2.0);
    yp = y + radius;

    Polygon_2 scan_line;
    scan_line.push_back( Point_2( xmin, y  ) );
    scan_line.push_back( Point_2( xmax, y  ) );
    scan_line.push_back( Point_2( xmax, yp ) );
    scan_line.push_back( Point_2( xmin, yp ) );

    Polygon_set_2 scan_lines = negative_region;

    scan_lines.intersection( scan_line );

    dump_polygon_set( scan_lines );

  }

  //dump_polygon_set(negative_region);


}

// print out gcode of the offset of the joined polygons
// as they appear in gPolygonSet.
//
void export_gcode(Polygon_set_2 &polygon_set)
{
  int pwh_count=0;
  int first;
  vert_it vit;

  std::list< Polygon_with_holes_2 > pwh_list;
  std::list< Polygon_with_holes_2 >::iterator pwh_it;

  polygon_set.polygons_with_holes( std::back_inserter(pwh_list) ) ;

  fprintf(gOutStream, "f%i\n", gFeedRate);
  fprintf(gOutStream, "g1 z%f", gZSafe);

  for (pwh_it = pwh_list.begin(); pwh_it != pwh_list.end(); ++pwh_it)
  {
    fprintf(gOutStream, "\n( offset outer boundary of polygon with hole %i )\n", pwh_count);

    // display outer boundary
    first = 1;
    Polygon_2 ob = (*pwh_it).outer_boundary();
    vert_it vit;
    for (vit = ob.vertices_begin(); vit != ob.vertices_end(); ++vit)
    {
      Point_2 p = (*vit);
      if (first)
      {
        fprintf(gOutStream, "g0 x%f y%f\n" , CGAL::to_double(p.x()) , CGAL::to_double(p.y()) );
        fprintf(gOutStream, "g1 z%f\n", gZCut);
        first = 0;
      }
      else
      {
        fprintf(gOutStream, "g1 x%f y%f\n" , CGAL::to_double(p.x()) , CGAL::to_double(p.y()) );
      }
    }

    // display first point again (to close loop)
    Point_2 p = *(ob.vertices_begin());
    fprintf(gOutStream, "g1 x%f y%f\n", CGAL::to_double(p.x()) , CGAL::to_double(p.y()) );

    fprintf(gOutStream, "g1 z%f\n", gZSafe);

    // display holes
    int hole_count=0;
    Polygon_with_holes_2::Hole_const_iterator hit;
    for (hit  = (*pwh_it).holes_begin();
         hit != (*pwh_it).holes_end();
         ++hit)
    {
      fprintf(gOutStream, "\n( offset hole %i of polygon with hole %i )\n", hole_count , pwh_count );

      Polygon_2 ib = *hit;

      for (vit  = ib.vertices_begin(), first = 1;
           vit != ib.vertices_end();
           ++vit)
      {
        Point_2 p = *vit;
        if (first)
        {
          fprintf(gOutStream, "g0 x%f y%f\n", CGAL::to_double(p.x()) , CGAL::to_double(p.y()) );
          fprintf(gOutStream, "g1 z%f\n", gZCut);
          first = 0;
        }
        else
        {
          fprintf(gOutStream, "g1 x%f y%f\n", CGAL::to_double(p.x()) , CGAL::to_double(p.y()) );
        }
      }

      // display first point again to close loop
      Point_2 p = *(ib.vertices_begin());
      fprintf(gOutStream, "g1 x%f y%f\n", CGAL::to_double(p.x()) , CGAL::to_double(p.y()) );

      hole_count++;

      // bring back up to safe distance after we're done cutting the hole
      fprintf(gOutStream, "g1 z%f\n", gZSafe);
    }

    pwh_count++;
  }

}

//------------------------------------------

void dump_polygon_set(Polygon_set_2 &polygon_set)
{
  int pwh_count=0;
  vert_it vit;

  std::list< Polygon_with_holes_2 > pwh_list;
  std::list< Polygon_with_holes_2 >::iterator pwh_it;

  //gPolygonSet.polygons_with_holes( std::back_inserter(pwh_list) ) ;
  polygon_set.polygons_with_holes( std::back_inserter(pwh_list) ) ;

  for (pwh_it = pwh_list.begin(); pwh_it != pwh_list.end(); ++pwh_it)
  {
    printf("#pwh[%i]\n", pwh_count);
    printf("\n#outer_boundary:\n");
    pwh_count++;

    Polygon_with_holes_2 pwh = *pwh_it;

    Polygon_2 ob = pwh.outer_boundary();
    for (vit = ob.vertices_begin(); vit != ob.vertices_end(); ++vit)
    {
      std::cout << *vit << "\n";
    }
    vit = ob.vertices_begin();
    std::cout << *vit << "\n";

    printf("\n#holes:\n");
    Hole_it hit;
    int hole_count=0;
    for (hit = pwh.holes_begin(); hit != pwh.holes_end(); ++hit)
    {
      printf("\n#hole[%i]:\n", hole_count);
      Polygon_2 h = *hit;

      for (vit = h.vertices_begin(); vit != h.vertices_end(); ++vit)
      {
        std::cout << *vit << "\n";
      }
      vit = h.vertices_begin();
      std::cout << *vit << "\n";

    }
  }
}

void dump_offset_polygon_set(Offset_polygon_set_2 &offset_polygon_set)
{
  int pwh_count=0;
  std::list< Offset_polygon_with_holes_2 > pwh_list;
  std::list< Offset_polygon_with_holes_2 >::iterator pwh_it;

  offset_polygon_set.polygons_with_holes( std::back_inserter(pwh_list) ) ;

  std::cout << "# dump_offset_polygon_set set size: " << offset_polygon_set.number_of_polygons_with_holes() << "\n";
  std::cout << "# dump_offset_polygon_set pwh_list size: " << pwh_list.size() << "\n";

  for (pwh_it = pwh_list.begin(); pwh_it != pwh_list.end(); ++pwh_it) 
  {
    printf("#offset pwh[%i]\n", pwh_count);
    pwh_count++;

    Offset_polygon_with_holes_2 offset = *pwh_it;
    print_linearized_offset_polygon_with_holes( offset );
  }
}

void dump_offset_polygon_set2(int count, Offset_polygon_set_2 &offset_polygon_set)
{
  int pwh_count=0;
  std::list< Offset_polygon_with_holes_2 > pwh_list;
  std::list< Offset_polygon_with_holes_2 >::iterator pwh_it;

  offset_polygon_set.polygons_with_holes( std::back_inserter(pwh_list) ) ;

  std::cout << "#dump" <<  count << " dump_offset_polygon_set2 set size: " << offset_polygon_set.number_of_polygons_with_holes() << "\n";
  std::cout << "#dump" << count << " dump_offset_polygon_set2 pwh_list size: " << pwh_list.size() << "\n";

  fflush(stdout);

  for (pwh_it = pwh_list.begin(); pwh_it != pwh_list.end(); ++pwh_it) 
  {
    printf("#dump%i offset pwh[%i]\n", count, pwh_count);
    pwh_count++;

    fflush(stdout);

    Offset_polygon_with_holes_2 offset = *pwh_it;
    print_linearized_offset_polygon_with_holes2( count, offset );
  }
}

void dump_offset_polygon_vector( std::vector< Offset_polygon_with_holes_2 > &offset_polygon_vector )
{
  int pwh_count=0;
  std::cout << "# dump_offset_polygon_vector size: " << offset_polygon_vector.size() << "\n";

  for (pwh_count=0; pwh_count < offset_polygon_vector.size(); pwh_count++)
  {
    printf("#offset pwh[%i]\n", pwh_count);
    print_linearized_offset_polygon_with_holes( offset_polygon_vector[pwh_count] );
  }
}

//------------------------------------------

struct option gLongOption[] =
{
  {"radius" , required_argument , 0, 'r'},
  {"input"  , required_argument , 0, 'i'},
  {"output" , required_argument , 0, 'o'},
  {"feed"   , required_argument , 0, 'f'},
  {"zsafe"  , required_argument , 0, 'z'},
  {"zcut"   , required_argument , 0, 'Z'},
  {"metric" , no_argument       , 0, 'I'},
  {"inches" , no_argument       , 0, 'M'},
//  {"seek"   , required_argument , 0, 's'},
  {"verbose", no_argument       , 0, 'v'},
  {"help"   , no_argument       , 0, 'h'},
  {0, 0, 0, 0}
};


char gOptionDescription[][1024] =
{
  "radius (default 0)",
  "input file (default stdin)",
  "output file (default stdout)",
  "feed rate (default 100)",
  "z safe height (default 0.1 inches)",
  "z cut height (default -0.05 inches)",
  "units in metric",
  "units in inches (default)",
//  "seek rate (default 100)",
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

int main(int argc, char **argv)
{
  int i, j, k;
  gerber_state_t gs;
  Offset_polygon_with_holes_2 offset;
  Polygon_set_2 polygon_set;

  extern char *optarg;
  extern int optind;
  int option_index;

  char ch;

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
      /*
    case 's':
      gSeekRate = atoi(optarg);
      break;
      */
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
    case 'v':
      gVerboseFlag = 1;
      break;
    default:
      printf("bad option\n");
      show_help();
      exit(0);
      break;
  }

  if (gOutputFilename)
  {
    if (!(gOutStream = fopen(gOutputFilename, "w")))
    {
      perror(gOutputFilename);
      exit(0);
    }
  }


  if (gVerboseFlag)
  {
    fprintf(gOutStream, "( radius %f )\n", gRadius);
  }

  if (gInputFilename)
  {
    /*
    if (gInpStream = fopen(gInputFilenam, "r"))
    {
      perror(gInpFilenam);
      exit(0);
    }
    */

  }
  else
  {
    printf("must provide input file\n");
    show_help();
    exit(0);
  }

  gerber_state_init(&gs);
  k = gerber_state_load_file(&gs, gInputFilename);
  if (k < 0)
  {
    perror(argv[1]);
    exit(0);
  }

  //dump_information(&gs);

  realize_apertures(&gs);

  join_polygon_set(&gs);

  //printf("# original:\n");
  //dump_polygon_set(gPolygonSet);

  //generate_scanline_unfilled_regions();
  //generate_zen_unfilled_regions();
  //construct_polygon_offset();

  if (gMetricUnits)
    fprintf(gOutStream, "g21\n");
  else
    fprintf(gOutStream, "g20\n");

  fprintf(gOutStream, "g90\n");

  if (gRadius > eps)
  {

    construct_polygon_offset( gOffsetPolygonVector, gPolygonSet );

    linearize_polygon_offset_vector( polygon_set, gOffsetPolygonVector );

    export_gcode( polygon_set );

  }
  else 
  {
    export_gcode( gPolygonSet );
  }

  exit(0);



}
