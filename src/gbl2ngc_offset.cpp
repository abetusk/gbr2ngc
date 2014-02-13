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


//---------------

void linearize_polygon_offset_vector( std::vector< Polygon_with_holes_2 > &pwh_vector,
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
        Offset_point_2 op = (*cit).source();
        Point_2 p ( CGAL::to_double(op.x()), CGAL::to_double(op.y()) );

        outer_boundary.push_back(p);
      }
    }

    for (hit = offset_vec[v].holes_begin(); hit != offset_vec[v].holes_end(); ++hit)
    {
      Polygon_2 hole;

      for (cit = (*hit).curves_begin();
           cit != (*hit).curves_end();
           ++cit)
      {
        if ( (*cit).is_linear() ||
             (*cit).is_circular() )
        {
          Offset_point_2 op = (*cit).source();
          Point_2 p ( CGAL::to_double(op.x()), CGAL::to_double(op.y()) );

          hole.push_back(p);

        }
      }

      hole_list.push_back(hole);
    }

    pwh_vector.push_back( Polygon_with_holes_2( outer_boundary, hole_list.begin(), hole_list.end() ) );

  }

}

//------------------------------------------

int check_polygon_set_ok(Polygon_set_2 &polygon_set)
{

  std::list< Polygon_with_holes_2 > pwh_list;
  std::list< Polygon_with_holes_2 >::iterator pwh_it;

  polygon_set.polygons_with_holes( std::back_inserter(pwh_list) ) ;

  for (pwh_it = pwh_list.begin(); pwh_it != pwh_list.end(); ++pwh_it)
  {
    Polygon_with_holes_2 pwh = *pwh_it;

    Polygon_2 ob = pwh.outer_boundary();
    if (!ob.is_simple()) return 0;

    Polygon_with_holes_2::Hole_const_iterator hit;
    for (hit = pwh.holes_begin(); hit != pwh.holes_end(); ++hit)
    {
      Polygon_2 ib = *hit;

      if (!ib.is_simple()) return 0;
    }

  }


  return 1;
}


// Offset the joined polygons
// as they appear in gPolygonSet.
//
//void construct_polygon_offset( std::vector< Offset_polygon_with_holes_2 > &pwh_vector,
//                               Polygon_set_2 &polygon_set)
void construct_polygon_offset( Paths &src, Paths &soln )
{

  ClipperOffset co;

  cp.AddPaths( src, jtRound, etClosedPolygon);
  co.Execute( soln, g_scalefactor * gRadius );
}


//typedef CGAL::Straight_skeleton_2< Kernel >  Ss ;
typedef boost::shared_ptr<Polygon_2> PolygonPtr ;
//typedef boost::shared_ptr<Ss> SsPtr ;


typedef std::vector<PolygonPtr> PolygonPtrVector ;


typedef boost::shared_ptr<Polygon_with_holes_2 > PolygonWithHolesPtr ;
typedef std::vector< PolygonWithHolesPtr > PolygonWithHolesPtrVector;



// Offset the joined polygons
// as they appear in gPolygonSet.
//
void construct_polygon_contours( std::vector< Polygon_2 > &poly_vector, 
                                 Polygon_set_2 &polygon_set )
{
  int i, j, k;
  double radius = gRadius;

  std::list< Polygon_with_holes_2 > pwh_list;
  std::list< Polygon_with_holes_2 >::iterator pwh_it;

  polygon_set.polygons_with_holes( std::back_inserter(pwh_list) ) ;

  for ( pwh_it  = pwh_list.begin(); 
        pwh_it != pwh_list.end(); 
        ++pwh_it)
  {

    PolygonPtrVector inner_offset_polygons = CGAL::create_interior_skeleton_and_offset_polygons_2( radius, *pwh_it );
    //PolygonWithHolesPtrVector outer_offset_polygons = CGAL::create_exterior_skeleton_and_offset_polygons_2( radius, *pwh_it );

    for ( i=0; i<inner_offset_polygons.size(); i++) 
      poly_vector.push_back( *(inner_offset_polygons[i]) );

    //for ( i=0; i<outer_offset_polygons.size(); i++) poly_vector.push_back( outer_offset_polygons[i]->outer_boundary() );

  }

}

// Offset the joined polygons
// as they appear in gPolygonSet.
//
void construct_pwh_contours( std::vector< Polygon_with_holes_2 > &pwh_vector, 
                                 Polygon_set_2 &polygon_set )
{
  int i, j, k;
  double radius = gRadius;

  std::list< Polygon_with_holes_2 > pwh_list;
  std::list< Polygon_with_holes_2 >::iterator pwh_it;

  polygon_set.polygons_with_holes( std::back_inserter(pwh_list) ) ;

  for ( pwh_it  = pwh_list.begin(); 
        pwh_it != pwh_list.end(); 
        ++pwh_it)
  {

    PolygonWithHolesPtrVector offset_pwh = CGAL::create_interior_skeleton_and_offset_polygons_with_holes_2( radius, *pwh_it );

    for ( i=0; i<offset_pwh.size(); i++) 
    {
      pwh_vector.push_back( *(offset_pwh[i]) );
    }

  }

}


//------------------------------------------

void find_bounding_box_pwh( double &xmin, double &ymin,
                            double &xmax, double &ymax,
                            const Polygon_with_holes_2 &pwh)
{
  double tx, ty;
  int first = 1;
  vert_it vit;

  for ( vit  = pwh.outer_boundary().vertices_begin();
        vit != pwh.outer_boundary().vertices_end();
        ++vit )
  {
    tx = CGAL::to_double( vit->x() );
    ty = CGAL::to_double( vit->y() );

    if (first)
    {
      xmin = tx;
      ymin = ty;
      xmax = tx;
      ymax = ty;
      continue;
    }

    if ( xmin > tx )
      xmin = tx;
    if ( xmax < tx )
      xmax = tx;
    if ( ymin > ty )
      ymin = ty;
    if ( ymax < ty )
      ymax = ty;

  }

}

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

void find_bounding_box_opwh_vector(
    double &xmin, double &ymin,
    double &xmax, double &ymax,
    std::vector<Offset_polygon_with_holes_2> &opwh_vector)
{
  int i, first=1;
  double tx, ty;
  vert_it vit;
  Point_2 point_min, point_max;

  for (i=0; i<opwh_vector.size(); i++)
  {
    Offset_polygon_with_holes_2 opwh = opwh_vector[i];

    /*
    Polygon_2 ob = opwh.outer_boundary();
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
    */

  }
}

void find_bounding_box_pwh_vector(
    double &xmin, double &ymin,
    double &xmax, double &ymax,
    std::vector<Polygon_with_holes_2> &pwh_vector)
{
  int i, first=1;
  double tx, ty;
  vert_it vit;
  Point_2 point_min, point_max;

  for (i=0; i<pwh_vector.size(); i++)
  {
    Polygon_with_holes_2 pwh = pwh_vector[i];

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

  }
}

//-----------------------

void linearize_polygon_offset( Polygon_with_holes_2 &pwh, Offset_polygon_with_holes_2 offset )
{
  Offset_polygon_with_holes_2::Hole_iterator hit;
  Offset_polygon_2::Curve_iterator cit;
  Polygon_2 outer_boundary;
  std::list< Polygon_2 > hole_list;

  // construct outer boundary
  int ob_point_count = 0;
  for ( cit  = offset.outer_boundary().curves_begin();
        cit != offset.outer_boundary().curves_end();
        ++cit)
  {

    if ( (*cit).is_linear() ||
         (*cit).is_circular() )
    {

      Offset_point_2 op = (*cit).source();
      Point_2 p ( CGAL::to_double(op.x()), CGAL::to_double(op.y()) );
      outer_boundary.push_back(p);

      ob_point_count++;

    }
  }

  if (ob_point_count < 2)
    return;

  printf("linearize_polygon_offset, ob end\n"); fflush(stdout); 

  // construct holes
  for ( hit  = offset.holes_begin();
        hit != offset.holes_end();
        ++hit)
  {
    Polygon_2 hole;
    int hole_point_count = 0;


    printf("linearize_polygon_offset, hole..\n"); fflush(stdout); 

    // already in reverse order
    for (cit  = (*hit).curves_begin();
         cit != (*hit).curves_end();
         ++cit)
    {

      if ( (*cit).is_linear() ||
           (*cit).is_circular() )
      {

        Offset_point_2 op = (*cit).source();
        Point_2 p( CGAL::to_double(op.x()), CGAL::to_double(op.y()) );
        hole.push_back(p);

        hole_point_count++;
      }

    }

    if (hole_point_count > 1)
      hole_list.push_back( hole );

  }

  printf("linearize_polygon_offset, hole end\n"); fflush(stdout); 

  //crappy way to copy...
  Polygon_with_holes_2 t_pwh( outer_boundary, hole_list.begin(), hole_list.end() );
  pwh = t_pwh;

}

//-------------------  helper classes and functions

class double_line_2 {
  public:
    double x0, y0, x1, y1;
    double_line_2( double a, double b, double c, double d ) { x0 = a; y0 = b; x1 = c; y1 = d; }
};

double mind(double a, double b) { return (a < b) ? a : b ; }
double maxd(double a, double b) { return (a > b) ? a : b ; }


//------------------ filling functions proper

void generate_zen_pwh( Polygon_with_holes_2 &zen_pwh, Polygon_with_holes_2 &pwh_orig, double radius, double err_bound )
{
  Offset_polygon_with_holes_2 offset;
  Polygon_with_holes_2 pwh;

  offset = approximated_offset_2( pwh_orig, radius, err_bound );
  linearize_polygon_offset( zen_pwh, offset );

}


//-----------------------

typedef CGAL::Straight_skeleton_2< Kernel >  Ss ;
typedef boost::shared_ptr<Ss> SsPtr ;

typedef boost::shared_ptr<Polygon_2> PolygonPtr ;
typedef std::vector<PolygonPtr> PolygonPtrVector ;


//typedef CGAL::Polygon_with_holes_2<Kernel_epick> Polygon_with_holes_2_epick;
//CGAL::Cartesian_converter< Kernel, Kernel_epick > converter;

#include <CGAL/create_straight_skeleton_from_polygon_with_holes_2.h>

//-----------------------

void export_polygon_ptr_vector( FILE *ofp,
                                PolygonPtrVector &pvec)
{
  vert_it vit;
  int i, j, k;
  int state;
  double x, y;
  double start_x, start_y;

  for (i=0; i<pvec.size(); i++)
  {

    Polygon_2 poly = *(pvec[i]);

    state = 2;

//    for ( vit  = poly.vertices_begin();
//          vit != poly.vertices_end();
//          ++vit )
    for ( vit  = pvec[i]->vertices_begin();
          vit != pvec[i]->vertices_end();
          ++vit )
    {

      x = CGAL::to_double( vit->x() );
      y = CGAL::to_double( vit->y() );

      if (state == 2)
      {
        fprintf( ofp, "g1 z%f\n", gZSafe);
        fprintf( ofp, "g0 x%f y%f\n", x, y);
        state = 1;

        start_x = x;
        start_y = y;
        continue;
      }
      else if (state == 1)
      {
        fprintf( ofp, "g1 z%f\n", gZCut);
        state = 0;
      }

      fprintf( ofp, "g1 x%f y%f\n", x, y);

    }

    if (state < 2)
    {
      fprintf( ofp, "g1 x%f y%f\n", start_x, start_y );
      fprintf( ofp, "g1 z%f\n", gZSafe );
    }

  }

}

//-----------------------

void zen_export_polygon_set_to_gcode( FILE *ofp,
                                      Polygon_set_2 &polygon_set,
                                      double radius,
                                      double err_bound )
{
  int i, j, k;
  double xmin, ymin, xmax, ymax;
  double tx_min, ty_min, tx_max, ty_max;
  double cur_radius;
  std::vector< Polygon_with_holes_2 > pwh_vector;

  Polygon_set_2 complement_set;
  complement_set.complement(polygon_set);

  complement_set.polygons_with_holes( std::back_inserter( pwh_vector ) );


  for ( i=0; i<pwh_vector.size(); i++)
  {

    printf(" zen pwh %i / %lu\n", i, pwh_vector.size() ); fflush(stdout);

    if (pwh_vector[i].is_unbounded())
    {

      printf(" skipping unbounded\n"); fflush(stdout);

      continue;
    }

    printf("    skel"); fflush(stdout);

    SsPtr ss_p = CGAL::create_interior_straight_skeleton_2( pwh_vector[i] );

    printf("    done"); fflush(stdout);

    int zen_iteration=0, zen_max_iteration = 10000;
    for ( zen_iteration = 0;  zen_iteration < zen_max_iteration; zen_iteration ++)
    {

      cur_radius = radius + 2.0*zen_iteration*radius;

      printf("j%i %f\n", zen_iteration, cur_radius); fflush(stdout);

      PolygonPtrVector offset_polygons = CGAL::create_offset_polygons_2<Polygon_2>( cur_radius, *ss_p );

      if (offset_polygons.size() == 0)
        break;

      export_polygon_ptr_vector( ofp, offset_polygons );

    }

    printf("  zen for current pwh done\n");

  }

}

void zen_export_to_gcode_OLD( FILE *ofp, 
                          std::vector< Polygon_with_holes_2 > &pwh_vector,
                          double radius,
                          double err_bound )
{

  int i, j;
  int zen_iteration = 0, max_zen_iteration = 10000;
  Polygon_with_holes_2::Hole_const_iterator hit;
  vert_it vit;
  Polygon_with_holes_2 pwh;

  double x, y, prev_x, prev_y, cur_offset;
  int state;

  for ( i=0; i<pwh_vector.size(); i++)
  {

    //printf("zen pwh %i / %lu\n", i, pwh_vector.size()); fflush(stdout);
    printf("zen pwh %i / %lu\n", i, pwh_vector.size()); fflush(stdout);

    pwh = pwh_vector[i];
    zen_iteration = 0;

    //printf("zen after copy, pwh %i / %lu\n", i, pwh_vector.size()); fflush(stdout);
    printf("zen after copy, pwh %i / %lu\n", i, pwh_vector.size()); fflush(stdout);

    while ( ( zen_iteration < max_zen_iteration ) &&
            ( pwh.holes_begin() != pwh.holes_end() ) )
    {
      Polygon_with_holes_2 zen_pwh;

      printf(" gen... pwh %i, zen_it %i\n", i, zen_iteration); fflush(stdout);

      //cur_offset = (double)(zen_iteration + 1) * radius;
      cur_offset = radius + (double)zen_iteration * 2.0 * radius;

      generate_zen_pwh( zen_pwh, pwh, cur_offset, err_bound);

      printf(" pwh %i, zen_it %i\n", i, zen_iteration); fflush(stdout);

      for ( hit  = zen_pwh.holes_begin();
            hit != zen_pwh.holes_end();
            ++hit)
      {

        fprintf( ofp, "( zen hole %i of pwh %i)\n", i, zen_iteration);

        state = 2;
        for ( vit  = hit->vertices_begin();
              vit != hit->vertices_end();
              ++vit )
        {
          x = CGAL::to_double( vit->x() );
          y = CGAL::to_double( vit->y() );

          if (state == 2)
          {
            prev_x = x;
            prev_y = y;
            state = 1;

            fprintf( ofp, "g1 z%f\n", gZSafe );
            fprintf( ofp, "g0 x%f y%f\n", x, y);

            continue;
          }
          else if (state == 1)
          {
            fprintf( ofp, "g1 z%f\n", gZCut );
            state = 0;
          }

          fprintf( ofp, "g1 x%f y%f\n", x, y);

        }
      }

      fprintf( ofp, "g1 z%f\n", gZSafe );

      pwh = zen_pwh;
      zen_iteration++;
    }

  }

}

bool cmp_double_line_vertical_asc( double_line_2 a, double_line_2 b )
{
  return a.y0 < b.y0;
}

bool cmp_double_line_vertical_desc( double_line_2 a, double_line_2 b )
{
  return a.y0 > b.y0;
}

void print_vertical_lines( FILE *ofp, std::vector< Polygon_with_holes_2 > &pwh_vector, double x0, double x1 )
{
  int i, j;
  double prev_x, prev_y, cur_x, cur_y;
  double first_x, first_y;

  for ( i=0; i<pwh_vector.size(); i++)
  {
    std::vector< double_line_2 > left_lines, right_lines;

    vert_it vit;
    int first = 1;

    for ( vit  = pwh_vector[i].outer_boundary().vertices_begin();
          vit != pwh_vector[i].outer_boundary().vertices_end();
          ++vit )
    {
      if (first)
      {
        prev_x = CGAL::to_double( (*vit).x() );
        prev_y = CGAL::to_double( (*vit).y() );

        first_x = prev_x;
        first_y = prev_y;

        first = 0;
        continue;
      }

      cur_x = CGAL::to_double( (*vit).x() );
      cur_y = CGAL::to_double( (*vit).y() );

      if ( fabs( cur_x - prev_x ) < eps )
      {
        if ( fabs( cur_x - x0 ) < eps )
        {

          printf("l"); fflush(stdout);

          left_lines.push_back( double_line_2( cur_x, mind(cur_y, prev_y), cur_x, maxd(cur_y, prev_y) ) );
        }
        else if ( fabs( cur_x - x1) < eps )
        {

          printf("r"); fflush(stdout);

          right_lines.push_back( double_line_2( cur_x, mind(cur_y, prev_y), cur_x, maxd(cur_y, prev_y) ) );
        }
      }

      prev_x = cur_x;
      prev_y = cur_y;

    }

    if ( fabs( cur_x - first_x) < eps )
    {
      if ( fabs( cur_x - x0 ) < eps )
      {

        printf("l+"); fflush(stdout);

        left_lines.push_back( double_line_2( cur_x, mind(cur_y, first_y), cur_x, maxd(cur_y, first_y) ) );
      }
      else if ( fabs( cur_x - x1) < eps )
      {

        printf("r+"); fflush(stdout);

        right_lines.push_back( double_line_2( cur_x, mind(cur_y, first_y), cur_x, maxd(cur_y, first_y) ) );
      }
    }

    std::sort( left_lines.begin(),  left_lines.end(),  cmp_double_line_vertical_asc  );
    std::sort( right_lines.begin(), right_lines.end(), cmp_double_line_vertical_desc );

    //fprintf( ofp, "( left %lu, right %lu)\n", left_lines.size(), right_lines.size() ); fflush(ofp);
    fprintf( ofp, "( left %lu, right %lu)\n", left_lines.size(), right_lines.size() ); fflush(ofp);

    fprintf( ofp, "g1 z%f\n", gZSafe);
    for ( j=0; j<left_lines.size(); j++ )
    {
      fprintf( ofp, "g0 x%f y%f\n", left_lines[j].x0, left_lines[j].y0 );
      fprintf( ofp, "g1 z%f\n", gZCut);
      fprintf( ofp, "g1 x%f y%f\n", left_lines[j].x1, left_lines[j].y1 );
      fprintf( ofp, "g1 z%f\n", gZSafe);
    }

    for ( j=0; j<right_lines.size(); j++ )
    {
      fprintf( ofp, "g1 x%f y%f\n", right_lines[j].x1, right_lines[j].y1 );
      fprintf( ofp, "g1 z%f\n", gZCut);
      fprintf( ofp, "g0 x%f y%f\n", right_lines[j].x0, right_lines[j].y0 );
      fprintf( ofp, "g1 z%f\n", gZSafe );
    }

  }

}



void scanline_vertical_export_to_gcode( FILE *ofp, 
                                        std::vector< Polygon_with_holes_2 > &pwh_vector,
                                        double radius)
{
  int i, j, k, n;

  double xmin, xmax;
  double ymin, ymax;
  double xdel;
  double rect_width;

  double border_offset = 0.25;

  std::vector< Polygon_with_holes_2 > intersected_scanline_pwh_vector;

  // Optimization:  Split big polygon vector into 32 regions so intersection test is sped up
  int n_region = 32;
  std::vector< Polygon_set_2 > negative_region;

  Polygon_set_2 scan_lines;

  double diameter = 2.0 * radius;
  double x, xp;
  int pos;
  double d, x0, x1;

  xdel = 2.0 * diameter;
  rect_width = diameter;

  // find bounding box
  find_bounding_box_pwh_vector(xmin, ymin, xmax, ymax, pwh_vector);

  xmin -= border_offset;
  xmax += border_offset;
  ymin -= border_offset;
  ymax += border_offset;

  for (i=0; i<n_region; i++)
  {

    Polygon_with_holes_2 pwh;
    Polygon_set_2 ps;

    d = xmax - xmin;

    x0 = xmin + ( d * (double)i/(double)n_region ) - 2.0*diameter;
    x1 = xmin + ( d * (double)(i+1)/(double)n_region ) + 2.0*diameter;

    pwh.outer_boundary().push_back( Point_2( x0, ymin ) );
    pwh.outer_boundary().push_back( Point_2( x1, ymin ) );
    pwh.outer_boundary().push_back( Point_2( x1, ymax ) );
    pwh.outer_boundary().push_back( Point_2( x0, ymax ) );

    ps.insert( pwh );

    for ( j=0; j<pwh_vector.size(); j++)
    {
      ps.difference( pwh_vector[j] );
    }

    negative_region.push_back( ps );

    printf(" negative region %i / %i\n", i, n_region); fflush(stdout);

  }


  n = (int)( (xmax - xmin) / xdel );

  // construct scan lines
  //
  for (i=0; i<n; i++)
  {
    Polygon_2 scan_line;
    std::list< Polygon_with_holes_2> pwh_list;
    std::list< Polygon_with_holes_2>::const_iterator it;

    pos = i * n_region / n;

    x = xmin + ((xmax - xmin) * (double)i / (double)n);
    xp = x + rect_width;

    scan_line.push_back( Point_2( x , ymin ) );
    scan_line.push_back( Point_2( xp, ymin ) );
    scan_line.push_back( Point_2( xp, ymax ) );
    scan_line.push_back( Point_2( x , ymax ) );


    Polygon_set_2 ps = negative_region[pos];

    ps.intersection( scan_line );
    ps.polygons_with_holes( std::back_inserter(pwh_list) );
    for ( it = pwh_list.begin() ; 
          it != pwh_list.end() ;
          ++it )
    {
      intersected_scanline_pwh_vector.push_back( *it );
    }

    printf(" vertical line %i / %i\n", i, n); fflush(stdout);

    print_vertical_lines( ofp, intersected_scanline_pwh_vector, x, xp );

  }

}


bool cmp_double_line_horizontal_asc( double_line_2 a, double_line_2 b )
{
  return a.x0 < b.x0;
}

bool cmp_double_line_horizontal_desc( double_line_2 a, double_line_2 b )
{
  return a.x0 > b.x0;
}

void print_horizontal_offset_lines( FILE *ofp, std::vector< Offset_polygon_with_holes_2 > &opwh_vector, double y0, double y1 )
{
  int i, j;
  double prev_x, prev_y, cur_x, cur_y;
  double first_x, first_y;

  Offset_polygon_with_holes_2::Hole_iterator hit;
  Offset_polygon_2::Curve_iterator cit;

  for ( i=0; i<opwh_vector.size(); i++)
  {
    std::vector< double_line_2 > top_lines, bot_lines;

    int first = 1;

    for ( cit  = opwh_vector[i].outer_boundary().curves_begin();
          cit != opwh_vector[i].outer_boundary().curves_end();
          ++cit )
    {

      if ( cit->is_linear() || 
           cit->is_circular() )
      {
        cur_x = CGAL::to_double( cit->source().x() );
        cur_y = CGAL::to_double( cit->source().y() );
      }
      else 
        continue;

      if (first)
      {
        prev_x = cur_x;
        prev_y = cur_y;

        first_x = cur_x;
        first_y = cur_y;

        first = 0;
        continue;
      }

      if ( fabs( cur_y - prev_y ) < eps )
      {
        if ( fabs( cur_y - y0 ) < eps )
        {
          top_lines.push_back( double_line_2( mind(cur_x, prev_x), cur_y, maxd(cur_x, prev_x), cur_y ) );
        }
        else if ( fabs( cur_y - y1) < eps )
        {
          bot_lines.push_back( double_line_2( mind(cur_x, prev_x), cur_y, maxd(cur_x, prev_x), cur_y ) );
        }
      }

      prev_x = cur_x;
      prev_y = cur_y;

    }

    if ( fabs( cur_y - first_y) < eps )
    {
      if ( fabs( cur_y - y0 ) < eps )
      {
        top_lines.push_back( double_line_2( mind(cur_x, first_x), cur_y, maxd(cur_x, first_x), cur_y ) );
      }
      else if ( fabs( cur_y - y1) < eps )
      {
        bot_lines.push_back( double_line_2( mind(cur_x, first_x), cur_y, maxd(cur_x, first_x), cur_y ) );
      }
    }

    std::sort( top_lines.begin(), top_lines.end(), cmp_double_line_horizontal_asc  );
    std::sort( bot_lines.begin(), bot_lines.end(), cmp_double_line_horizontal_desc );

    fprintf( ofp, "g1 z%f\n", gZSafe);
    for ( j=0; j<top_lines.size(); j++ )
    {
      fprintf( ofp, "g0 x%f y%f\n", top_lines[j].x0, top_lines[j].y0 );
      fprintf( ofp, "g1 z%f\n", gZCut);
      fprintf( ofp, "g1 x%f y%f\n", top_lines[j].x1, top_lines[j].y1 );
      fprintf( ofp, "g1 z%f\n", gZSafe);
    }

    for ( j=0; j<bot_lines.size(); j++ )
    {
      fprintf( ofp, "g1 x%f y%f\n", bot_lines[j].x1, bot_lines[j].y1 );
      fprintf( ofp, "g1 z%f\n", gZCut);
      fprintf( ofp, "g0 x%f y%f\n", bot_lines[j].x0, bot_lines[j].y0 );
      fprintf( ofp, "g1 z%f\n", gZSafe );
    }

  }

}


void print_horizontal_lines( FILE *ofp, std::vector< Polygon_with_holes_2 > &pwh_vector, double y0, double y1 )
{
  int i, j;
  double prev_x, prev_y, cur_x, cur_y;
  double first_x, first_y;

  for ( i=0; i<pwh_vector.size(); i++)
  {
    std::vector< double_line_2 > top_lines, bot_lines;

    vert_it vit;
    int first = 1;

    for ( vit  = pwh_vector[i].outer_boundary().vertices_begin();
          vit != pwh_vector[i].outer_boundary().vertices_end();
          ++vit )
    {
      if (first)
      {
        prev_x = CGAL::to_double( (*vit).x() );
        prev_y = CGAL::to_double( (*vit).y() );

        first_x = prev_x;
        first_y = prev_y;

        first = 0;
        continue;
      }

      cur_x = CGAL::to_double( (*vit).x() );
      cur_y = CGAL::to_double( (*vit).y() );

      if ( fabs( cur_y - prev_y ) < eps )
      {
        if ( fabs( cur_y - y0 ) < eps )
        {
          top_lines.push_back( double_line_2( mind(cur_x, prev_x), cur_y, maxd(cur_x, prev_x), cur_y ) );
        }
        else if ( fabs( cur_y - y1) < eps )
        {
          bot_lines.push_back( double_line_2( mind(cur_x, prev_x), cur_y, maxd(cur_x, prev_x), cur_y ) );
        }
      }

      prev_x = cur_x;
      prev_y = cur_y;

    }

    if ( fabs( cur_y - first_y) < eps )
    {
      if ( fabs( cur_y - y0 ) < eps )
      {
        top_lines.push_back( double_line_2( mind(cur_x, first_x), cur_y, maxd(cur_x, first_x), cur_y ) );
      }
      else if ( fabs( cur_y - y1) < eps )
      {
        bot_lines.push_back( double_line_2( mind(cur_x, first_x), cur_y, maxd(cur_x, first_x), cur_y ) );
      }
    }

    std::sort( top_lines.begin(), top_lines.end(), cmp_double_line_horizontal_asc  );
    std::sort( bot_lines.begin(), bot_lines.end(), cmp_double_line_horizontal_desc );

    fprintf( ofp, "g1 z%f\n", gZSafe);
    for ( j=0; j<top_lines.size(); j++ )
    {
      fprintf( ofp, "g0 x%f y%f\n", top_lines[j].x0, top_lines[j].y0 );
      fprintf( ofp, "g1 z%f\n", gZCut);
      fprintf( ofp, "g1 x%f y%f\n", top_lines[j].x1, top_lines[j].y1 );
      fprintf( ofp, "g1 z%f\n", gZSafe);
    }

    for ( j=0; j<bot_lines.size(); j++ )
    {
      fprintf( ofp, "g1 x%f y%f\n", bot_lines[j].x1, bot_lines[j].y1 );
      fprintf( ofp, "g1 z%f\n", gZCut);
      fprintf( ofp, "g0 x%f y%f\n", bot_lines[j].x0, bot_lines[j].y0 );
      fprintf( ofp, "g1 z%f\n", gZSafe );
    }

  }

}

void scanline_horizontal_export_offset_polygon_vector_to_gcode( 
    FILE *ofp, 
    std::vector< Offset_polygon_with_holes_2 > &opwh_vector,
    double radius)
{
  int i, j, k, n;

  double xmin, xmax;
  double ymin, ymax;
  double ydel;
  double rect_height;

  double border_offset = 0.25;

  std::vector< Offset_polygon_with_holes_2 > intersected_scanline_opwh_vector;

  // Optimization:  Split big polygon vector into 32 regions so intersection test is sped up
  int n_region = 32;
  std::vector< Offset_polygon_set_2 > negative_region;

  Polygon_set_2 scan_lines;

  double diameter = 2.0 * radius;
  double y, yp;
  int pos;
  double d, y0, y1;

  ydel = 2.0 * diameter;
  rect_height = diameter;

  // find bounding box
  find_bounding_box_opwh_vector(xmin, ymin, xmax, ymax, opwh_vector);

  xmin -= border_offset;
  xmax += border_offset;
  ymin -= border_offset;
  ymax += border_offset;

  for (i=0; i<n_region; i++)
  {

    Offset_polygon_with_holes_2 opwh;
    Offset_polygon_set_2 ops;

    d = ymax - ymin;

    y0 = ymin + ( d * (double)i/(double)n_region ) - 2.0*diameter;
    y1 = ymin + ( d * (double)(i+1)/(double)n_region ) + 2.0*diameter;

    Point_2 p0( xmin, y0 );
    Point_2 p1( xmax, y0 );
    Point_2 p2( xmax, y1 );
    Point_2 p3( xmin, y1 );
    
    opwh.outer_boundary().push_back( X_monotone_curve_2( p0, p1 ) );
    opwh.outer_boundary().push_back( X_monotone_curve_2( p1, p2 ) );
    opwh.outer_boundary().push_back( X_monotone_curve_2( p2, p3 ) );
    opwh.outer_boundary().push_back( X_monotone_curve_2( p3, p0 ) );

    Gps_traits_2 traits;

    ops.insert( opwh );

    for ( j=0; j<opwh_vector.size(); j++)
    {
      ops.difference( opwh_vector[j] );
    }

    negative_region.push_back( ops );
  }

  n = (int)( (ymax - ymin) / ydel );

  // construct scan lines
  //
  for (i=0; i<n; i++)
  {

    Offset_polygon_2 scan_line;
    std::list< Offset_polygon_with_holes_2> opwh_list;
    std::list< Offset_polygon_with_holes_2>::const_iterator it;

    pos = i * n_region / n;

    y = ymin + ((ymax - ymin) * (double)i / (double)n);
    yp = y + rect_height;

    scan_line.push_back( X_monotone_curve_2( Point_2( xmin, y  ), Point_2( xmax, y  ) ) );
    scan_line.push_back( X_monotone_curve_2( Point_2( xmax, y  ), Point_2( xmax, yp ) ) );
    scan_line.push_back( X_monotone_curve_2( Point_2( xmax, yp ), Point_2( xmin, yp ) ) );
    scan_line.push_back( X_monotone_curve_2( Point_2( xmin, yp ), Point_2( xmin, y  ) ) );


    Offset_polygon_set_2 ops = negative_region[pos];

    ops.intersection( scan_line );
    ops.polygons_with_holes( std::back_inserter( opwh_list ) );
    for ( it  = opwh_list.begin() ; 
          it != opwh_list.end() ;
          ++it )
    {
      intersected_scanline_opwh_vector.push_back( *it );
    }

    print_horizontal_offset_lines( ofp, intersected_scanline_opwh_vector, y, yp );

  }

}

/*
void scanline_horizontal_export_offset_polygon_vector_to_gcode_dm(
    FILE *ofp,
    std::vector< Offset_polygon_with_holes_2 > *opwh_vector,
    double radius )
{
  int i, j, k, n ;

  double xmin, xmax;
  double ymin, ymax;
  double ydel;
  double rect_height;

  double border_offset = 0.25;

  std::vector< Offset_polygon_with_holes_2 > intersected_scanline_pwh_vector;

  // Optimization:  Split big polygon vector into 32 regions so intersection test is sped up
  int n_region = 32;
  std::vector< Offset_polygon_set_2 > negative_region;

  double diameter = 2.0 * radius;
  double y, yp;
  int pos;
  double d, y0, y1;

  ydel = 2.0 * diameter;
  rect_height = diameter;

  // find bounding box
  find_bounding_box_opwh_vector(xmin, ymin, xmax, ymax, opwh_vector);

  xmin -= border_offset;
  xmax += border_offset;
  ymin -= border_offset;
  ymax += border_offset;

  for (i=0; i<n_region; i++)
  {

    Polygon_with_holes_2 pwh;
    Polygon_set_2 ps;

    d = ymax - ymin;

    y0 = ymin + ( d * (double)i/(double)n_region ) - 2.0*diameter;
    y1 = ymin + ( d * (double)(i+1)/(double)n_region ) + 2.0*diameter;

    pwh.outer_boundary().push_back( Point_2( xmin, y0 ) );
    pwh.outer_boundary().push_back( Point_2( xmax, y0 ) );
    pwh.outer_boundary().push_back( Point_2( xmax, y1 ) );
    pwh.outer_boundary().push_back( Point_2( xmin, y1 ) );

    ps.insert( pwh );

    for ( j=0; j<pwh_vector.size(); j++)
    {
      ps.difference( pwh_vector[j] );
    }

    negative_region.push_back( ps );
  }


  n = (int)( (ymax - ymin) / ydel );

  // construct scan lines
  //
  for (i=0; i<n; i++)
  {
    Polygon_2 scan_line;
    std::list< Polygon_with_holes_2> pwh_list;
    std::list< Polygon_with_holes_2>::const_iterator it;

    pos = i * n_region / n;

    y = ymin + ((ymax - ymin) * (double)i / (double)n);
    yp = y + rect_height;

    scan_line.push_back( Point_2( xmin, y  ) );
    scan_line.push_back( Point_2( xmax, y  ) );
    scan_line.push_back( Point_2( xmax, yp ) );
    scan_line.push_back( Point_2( xmin, yp ) );


    Polygon_set_2 ps = negative_region[pos];

    ps.intersection( scan_line );
    ps.polygons_with_holes( std::back_inserter(pwh_list) );
    for ( it = pwh_list.begin() ; 
          it != pwh_list.end() ;
          ++it )
    {
      intersected_scanline_pwh_vector.push_back( *it );
    }

    print_horizontal_lines( ofp, intersected_scanline_pwh_vector, y, yp );

  }


}

*/


void scanline_horizontal_export_to_gcode( FILE *ofp, 
                                          std::vector< Polygon_with_holes_2 > &pwh_vector,
                                          double radius)
{
  int i, j, k, n;

  double xmin, xmax;
  double ymin, ymax;
  double ydel;
  double rect_height;

  double border_offset = 0.25;

  std::vector< Polygon_with_holes_2 > intersected_scanline_pwh_vector;

  // Optimization:  Split big polygon vector into 32 regions so intersection test is sped up
  int n_region = 32;
  std::vector< Polygon_set_2 > negative_region;

  Polygon_set_2 scan_lines;

  double diameter = 2.0 * radius;
  double y, yp;
  int pos;
  double d, y0, y1;

  ydel = 2.0 * diameter;
  rect_height = diameter;

  // find bounding box
  find_bounding_box_pwh_vector(xmin, ymin, xmax, ymax, pwh_vector);

  xmin -= border_offset;
  xmax += border_offset;
  ymin -= border_offset;
  ymax += border_offset;

  for (i=0; i<n_region; i++)
  {

    Polygon_with_holes_2 pwh;
    Polygon_set_2 ps;

    d = ymax - ymin;

    y0 = ymin + ( d * (double)i/(double)n_region ) - 2.0*diameter;
    y1 = ymin + ( d * (double)(i+1)/(double)n_region ) + 2.0*diameter;

    pwh.outer_boundary().push_back( Point_2( xmin, y0 ) );
    pwh.outer_boundary().push_back( Point_2( xmax, y0 ) );
    pwh.outer_boundary().push_back( Point_2( xmax, y1 ) );
    pwh.outer_boundary().push_back( Point_2( xmin, y1 ) );

    ps.insert( pwh );

    for ( j=0; j<pwh_vector.size(); j++)
    {
      ps.difference( pwh_vector[j] );
    }

    negative_region.push_back( ps );
  }


  n = (int)( (ymax - ymin) / ydel );

  // construct scan lines
  //
  for (i=0; i<n; i++)
  {
    Polygon_2 scan_line;
    std::list< Polygon_with_holes_2> pwh_list;
    std::list< Polygon_with_holes_2>::const_iterator it;

    pos = i * n_region / n;

    y = ymin + ((ymax - ymin) * (double)i / (double)n);
    yp = y + rect_height;

    scan_line.push_back( Point_2( xmin, y  ) );
    scan_line.push_back( Point_2( xmax, y  ) );
    scan_line.push_back( Point_2( xmax, yp ) );
    scan_line.push_back( Point_2( xmin, yp ) );


    Polygon_set_2 ps = negative_region[pos];

    ps.intersection( scan_line );
    ps.polygons_with_holes( std::back_inserter(pwh_list) );
    for ( it = pwh_list.begin() ; 
          it != pwh_list.end() ;
          ++it )
    {
      intersected_scanline_pwh_vector.push_back( *it );
    }

    print_horizontal_lines( ofp, intersected_scanline_pwh_vector, y, yp );

  }

}


void generate_scanline_horizontal_test( std::vector<Polygon_with_holes_2> &pwh_vector_rop, std::vector<Polygon_with_holes_2> &pwh_vector_op, double radius)
{
  int i, j, k, n;

  double xmin, xmax;
  double ymin, ymax;
  double ydel;
  double rect_height;

  double border_offset = 0.25;

  struct timeval tv0, tv1;

  int n_region = 32;
  //std::vector< Polygon_with_holes_2 > containing_rectangle;
  //Polygon_set_2 negative_region;
  std::vector< Polygon_set_2 > negative_region;

  Polygon_set_2 scan_lines;

  double diameter = 2.0 * radius;

  ydel = radius / 2.0;
  rect_height = ydel / 100.0;

  // find bounding box
  find_bounding_box_pwh_vector(xmin, ymin, xmax, ymax, pwh_vector_op);

  xmin -= border_offset;
  xmax += border_offset;
  ymin -= border_offset;
  ymax += border_offset;

  for (i=0; i<n_region; i++)
  {

    Polygon_with_holes_2 pwh;
    Polygon_set_2 ps;

    double d = ymax - ymin;

    double y0 = ymin + ( d * (double)i/(double)n_region ) - 2.0*diameter;
    double y1 = ymin + ( d * (double)(i+1)/(double)n_region ) + 2.0*diameter;

    pwh.outer_boundary().push_back( Point_2( xmin, y0 ) );
    pwh.outer_boundary().push_back( Point_2( xmax, y0 ) );
    pwh.outer_boundary().push_back( Point_2( xmax, y1 ) );
    pwh.outer_boundary().push_back( Point_2( xmin, y1 ) );

    ps.insert( pwh );

    for ( j=0; j<pwh_vector_op.size(); j++)
    {
      ps.difference( pwh_vector_op[j] );
    }

    negative_region.push_back( ps );
  }

  //printf("# negative_region %lu\n", negative_region.size() );
  printf("# negative_region %lu\n", negative_region.size() );

  n = (int)( (ymax - ymin) / ydel );

  // construct scan lines
  //for (i=0; i<=n; i++)
  for (i=0; i<n; i++)
  {
    double y, yp;


    int pos = i * n_region / n;

    printf("#  cp0 scanline %i / %i, pos: %i\n", i, n, pos); fflush(stdout);

    y = ymin + ((ymax - ymin) * (double)i / (double)n);
    yp = y + rect_height;

    printf("#  cp1 y %f, yp %f\n", y, yp); fflush(stdout);

    Polygon_2 scan_line;
    scan_line.push_back( Point_2( xmin, y  ) );
    scan_line.push_back( Point_2( xmax, y  ) );
    scan_line.push_back( Point_2( xmax, yp ) );
    scan_line.push_back( Point_2( xmin, yp ) );

    Polygon_set_2 ps = negative_region[pos];
    ps.intersection( scan_line );

    std::list< Polygon_with_holes_2> pwh_list;
    std::list< Polygon_with_holes_2>::const_iterator it;
    ps.polygons_with_holes( std::back_inserter(pwh_list) );
    for (it = pwh_list.begin(); it != pwh_list.end(); ++it)
    {


      pwh_vector_rop.push_back( *it );
    }

    printf("#  scanline %i / %i, pos: %i\n", i, n, pos); fflush(stdout);

  }

  printf("# done\n"); fflush(stdout);



}


void generate_scanline_horizontal( std::vector<Polygon_with_holes_2> &pwh_vector_rop, std::vector<Polygon_with_holes_2> &pwh_vector_op, double radius)
{
  int i, j, k, n;

  double xmin, xmax;
  double ymin, ymax;
  double ydel;
  double rect_height;

  double border_offset = 0.25;

  struct timeval tv0, tv1;

  //Polygon_2 containing_rectangle;
  //Polygon_set_2 negative_region;
  Polygon_with_holes_2 containing_rectangle;
  Polygon_set_2 negative_region;

  Polygon_set_2 scan_lines;

  //diameter = 0.008;
  //radius = diameter / 2.0;
  double diameter = 2.0 * radius;

  ydel = radius / 2.0;
  rect_height = ydel / 100.0;

  // find bounding box
  find_bounding_box_pwh_vector(xmin, ymin, xmax, ymax, pwh_vector_op);

  xmin -= border_offset;
  xmax += border_offset;
  ymin -= border_offset;
  ymax += border_offset;

  /*
  containing_rectangle.push_back( Point_2( xmin, ymin ) );
  containing_rectangle.push_back( Point_2( xmax, ymin ) );
  containing_rectangle.push_back( Point_2( xmax, ymax ) );
  containing_rectangle.push_back( Point_2( xmin, ymax ) );
  */

  containing_rectangle.outer_boundary().push_back( Point_2( xmin, ymin ) );
  containing_rectangle.outer_boundary().push_back( Point_2( xmax, ymin ) );
  containing_rectangle.outer_boundary().push_back( Point_2( xmax, ymax ) );
  containing_rectangle.outer_boundary().push_back( Point_2( xmin, ymax ) );

  negative_region.insert( containing_rectangle );

  for ( i=0; i<pwh_vector_op.size(); i++)
  {
    negative_region.difference( pwh_vector_op[i] );
  }

  n = (int)( (ymax - ymin) / ydel );

  // construct scan lines
  for (i=0; i<=n; i++)
  {
    double y, yp;

    y = ymin + ((ymax - ymin) * (double)i / (double)n);
    //yp = y + (ydel/2.0);
    //yp = y + radius;
    yp = y + rect_height;

    Polygon_2 scan_line;
    scan_line.push_back( Point_2( xmin, y  ) );
    scan_line.push_back( Point_2( xmax, y  ) );
    scan_line.push_back( Point_2( xmax, yp ) );
    scan_line.push_back( Point_2( xmin, yp ) );

    //Polygon_with_holes_2 scan_line;
    //scan_line.outer_boundary().push_back( Point_2( xmin, y  ) );
    //scan_line.outer_boundary().push_back( Point_2( xmax, y  ) );
    //scan_line.outer_boundary().push_back( Point_2( xmax, yp ) );
    //scan_line.outer_boundary().push_back( Point_2( xmin, yp ) );

    scan_lines.insert( scan_line );

    /*
    Polygon_set_2 scan_lines;

    gettimeofday(&tv0, NULL);
    scan_lines = negative_region;
    gettimeofday(&tv1, NULL);

    printf(" copy: ");
    print_profile(&tv0, &tv1);
    printf("\n"); fflush(stdout);

    gettimeofday(&tv0, NULL);
    scan_lines.intersection( scan_line );
    gettimeofday(&tv1, NULL);

    printf(" intersect: ");
    print_profile(&tv0, &tv1);
    printf("\n"); fflush(stdout);

    std::list< Polygon_with_holes_2> pwh_list;
    std::list< Polygon_with_holes_2>::const_iterator it;

    gettimeofday(&tv0, NULL);
    scan_lines.polygons_with_holes( std::back_inserter(pwh_list) );
    for (it = pwh_list.begin(); it != pwh_list.end(); ++it)
    {
      pwh_vector_rop.push_back( *it );
    }
    gettimeofday(&tv1, NULL);

    printf(" pwh_vector_rop.push_back: ");
    print_profile(&tv0, &tv1);
    printf("\n"); fflush(stdout);
    */

  }

  std::list< Polygon_with_holes_2> pwh_list;
  std::list< Polygon_with_holes_2>::const_iterator it;


  printf(" intersection: "); fflush(stdout);
  gettimeofday(&tv0, NULL);
  scan_lines.intersection( negative_region );
  gettimeofday(&tv1, NULL);
  print_profile(&tv0, &tv1); fflush(stdout);
  printf("\n"); fflush(stdout);

  scan_lines.polygons_with_holes( std::back_inserter(pwh_list) );

  /*
  printf(" intersection: "); fflush(stdout);
  gettimeofday(&tv0, NULL);
  negative_region.intersection( scan_lines );
  gettimeofday(&tv1, NULL);
  print_profile(&tv0, &tv1); fflush(stdout);
  printf("\n"); fflush(stdout);

  negative_region.polygons_with_holes( std::back_inserter(pwh_list) );
  */


  for (it = pwh_list.begin(); it != pwh_list.end(); ++it)
  {
    pwh_vector_rop.push_back( *it );
  }



  //export_gcode2( pwh_vector_rop );
  exit(0);


}

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


