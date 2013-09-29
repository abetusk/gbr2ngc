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

// print out gcode of the offset of the joined polygons
// as they appear in gPolygonSet.
//
void export_polygon_set_to_gcode( FILE *ofp, Polygon_set_2 &polygon_set)
{
  int pwh_count=0;
  int first;
  vert_it vit;

  std::list< Polygon_with_holes_2 > pwh_list;
  std::list< Polygon_with_holes_2 >::iterator pwh_it;

  polygon_set.polygons_with_holes( std::back_inserter(pwh_list) ) ;

  fprintf(ofp, "f%i\n", gFeedRate);
  fprintf(ofp, "g1 z%f", gZSafe);

  for (pwh_it = pwh_list.begin(); pwh_it != pwh_list.end(); ++pwh_it)
  {
    fprintf(ofp, "\n( offset outer boundary of polygon with hole %i )\n", pwh_count);

    // display outer boundary
    first = 1;
    Polygon_2 ob = (*pwh_it).outer_boundary();
    for (vit = ob.vertices_begin(); vit != ob.vertices_end(); ++vit)
    {
      Point_2 p = (*vit);
      if (first)
      {
        fprintf(ofp, "g0 x%f y%f\n" , CGAL::to_double(p.x()) , CGAL::to_double(p.y()) );
        fprintf(ofp, "g1 z%f\n", gZCut);
        first = 0;
      }
      else
      {
        fprintf(ofp, "g1 x%f y%f\n" , CGAL::to_double(p.x()) , CGAL::to_double(p.y()) );
      }
    }

    // display first point again (to close loop)
    Point_2 p = *(ob.vertices_begin());
    fprintf(ofp, "g1 x%f y%f\n", CGAL::to_double(p.x()) , CGAL::to_double(p.y()) );

    fprintf(ofp, "g1 z%f\n", gZSafe);

    // display holes
    int hole_count=0;
    Polygon_with_holes_2::Hole_const_iterator hit;
    for (hit  = (*pwh_it).holes_begin();
         hit != (*pwh_it).holes_end();
         ++hit)
    {
      fprintf(ofp, "\n( offset hole %i of polygon with hole %i )\n", hole_count , pwh_count );

      Polygon_2 ib = *hit;

      for (vit  = ib.vertices_begin(), first = 1;
           vit != ib.vertices_end();
           ++vit)
      {
        Point_2 p = *vit;
        if (first)
        {
          fprintf(ofp, "g0 x%f y%f\n", CGAL::to_double(p.x()) , CGAL::to_double(p.y()) );
          fprintf(ofp, "g1 z%f\n", gZCut);
          first = 0;
        }
        else
        {
          fprintf(ofp, "g1 x%f y%f\n", CGAL::to_double(p.x()) , CGAL::to_double(p.y()) );
        }
      }

      // display first point again to close loop
      Point_2 p = *(ib.vertices_begin());
      fprintf(ofp, "g1 x%f y%f\n", CGAL::to_double(p.x()) , CGAL::to_double(p.y()) );

      hole_count++;

      // bring back up to safe distance after we're done cutting the hole
      fprintf(ofp, "g1 z%f\n", gZSafe);
    }

    pwh_count++;
  }

}

//-----------------------

// print out gcode of the offset of the joined polygons
// as they appear in gPolygonSet.
//
// vector version
//
void export_polygon_vector_to_gcode( FILE *ofp, std::vector< Polygon_2 > &poly_vector )
{
  int i, j, k;
  int poly_count=0;
  int first;
  vert_it vit;

  fprintf(ofp, "f%i\n", gFeedRate);
  fprintf(ofp, "g1 z%f", gZSafe);

  for (i=0; i<poly_vector.size(); i++)
  {

    fprintf(ofp, "\n( offset polygon boundary of polygon %i )\n", poly_count);

    // display outer boundary
    first = 1;

    vert_it vit;
    for (vit = poly_vector[i].vertices_begin(); vit != poly_vector[i].vertices_end(); ++vit)
    {
      Point_2 p = (*vit);
      if (first)
      {
        fprintf(ofp, "g0 x%f y%f\n" , CGAL::to_double(p.x()) , CGAL::to_double(p.y()) );
        fprintf(ofp, "g1 z%f\n", gZCut);
        first = 0;
      }
      else
      {
        fprintf(ofp, "g1 x%f y%f\n" , CGAL::to_double(p.x()) , CGAL::to_double(p.y()) );
      }
    }

    poly_count++;
  }

}

// print out gcode of the offset of the joined polygons
// as they appear in gPolygonSet.
//
// vector version
//
void export_pwh_vector_to_gcode( FILE *ofp, std::vector< Polygon_with_holes_2 > &pwh_vector )
{
  int i, j, k;
  int pwh_count=0;
  int first;
  vert_it vit;

  std::list< Polygon_with_holes_2 > pwh_list;
  std::list< Polygon_with_holes_2 >::iterator pwh_it;

  fprintf(ofp, "f%i\n", gFeedRate);
  fprintf(ofp, "g1 z%f", gZSafe);

  for (i=0; i<pwh_vector.size(); i++)
  {

    fprintf(ofp, "\n( offset outer boundary of polygon with hole %i )\n", pwh_count);

    // display outer boundary
    first = 1;

    Polygon_2 ob = (pwh_vector[i]).outer_boundary();
    vert_it vit;
    for (vit = ob.vertices_begin(); vit != ob.vertices_end(); ++vit)
    {
      Point_2 p = (*vit);
      if (first)
      {
        fprintf(ofp, "g0 x%f y%f\n" , CGAL::to_double(p.x()) , CGAL::to_double(p.y()) );
        fprintf(ofp, "g1 z%f\n", gZCut);
        first = 0;
      }
      else
      {
        fprintf(ofp, "g1 x%f y%f\n" , CGAL::to_double(p.x()) , CGAL::to_double(p.y()) );
      }
    }

    // display first point again (to close loop)
    Point_2 p = *(ob.vertices_begin());
    fprintf(ofp, "g1 x%f y%f\n", CGAL::to_double(p.x()) , CGAL::to_double(p.y()) );

    fprintf(ofp, "g1 z%f\n", gZSafe);

    // display holes
    int hole_count=0;
    Polygon_with_holes_2::Hole_const_iterator hit;
    for ( hit = (pwh_vector[i]).holes_begin();
          hit != (pwh_vector[i]).holes_end();
          ++hit )
    {
      fprintf(ofp, "\n( offset hole %i of polygon with hole %i )\n", hole_count , pwh_count );

      Polygon_2 ib = *hit;

      for (vit  = ib.vertices_begin(), first = 1;
           vit != ib.vertices_end();
           ++vit)
      {
        Point_2 p = *vit;
        if (first)
        {
          fprintf(ofp, "g0 x%f y%f\n", CGAL::to_double(p.x()) , CGAL::to_double(p.y()) );
          fprintf(ofp, "g1 z%f\n", gZCut);
          first = 0;
        }
        else
        {
          fprintf(ofp, "g1 x%f y%f\n", CGAL::to_double(p.x()) , CGAL::to_double(p.y()) );
        }
      }

      // display first point again to close loop
      Point_2 p = *(ib.vertices_begin());
      fprintf(ofp, "g1 x%f y%f\n", CGAL::to_double(p.x()) , CGAL::to_double(p.y()) );

      hole_count++;

      // bring back up to safe distance after we're done cutting the hole
      fprintf(ofp, "g1 z%f\n", gZSafe);
    }

    pwh_count++;
  }

}


// print out gcode of the offset of the joined polygons
// as they appear in gPolygonSet.
//
// vector version
//
void export_offset_polygon_vector_to_gcode( FILE *ofp, std::vector< Offset_polygon_with_holes_2 > &offset_polygon_vector )
{
  int i, j, k, pwh_count=0, state;
  double x, y;
  vert_it vit;

  Offset_polygon_with_holes_2::Hole_iterator hit;
  Offset_polygon_2::Curve_iterator cit;

  std::list< Polygon_with_holes_2 > pwh_list;
  std::list< Polygon_with_holes_2 >::iterator pwh_it;

  fprintf(ofp, "f%i\n", gFeedRate);
  fprintf(ofp, "g1 z%f", gZSafe);

  for (i=0; i<offset_polygon_vector.size(); i++)
  {

    Polygon_with_holes_2 pwh;
    Polygon_2 outer_boundary;

    fprintf(ofp, "\n( offset outer boundary of polygon with hole %i )\n", pwh_count++);

    fprintf( ofp, "g1 z%f\n", gZSafe );

    state = 2;
    for ( cit  = offset_polygon_vector[i].outer_boundary().curves_begin();
          cit != offset_polygon_vector[i].outer_boundary().curves_end();
          ++cit )
    {

      if ( (*cit).is_linear() ||
           (*cit).is_circular() )
      {
        Offset_point_2 op = (*cit).source();

        x = CGAL::to_double(op.x());
        y = CGAL::to_double(op.y());

        if (state == 2)
        {
          fprintf( ofp, "g1 z%f\n", gZSafe );
          fprintf( ofp, "g0 x%f y%f\n", x, y);
          state = 1;
          continue;
        }
        else if (state == 1)
        {
          fprintf( ofp, "g1 z%f\n", gZCut );
          state = 0;
        }

        fprintf( ofp, "g1 x%f y%f\n", x, y );
      }

    }

    fprintf( ofp, "g1 z%f\n", gZSafe );

    for ( hit  = offset_polygon_vector[i].holes_begin();
          hit != offset_polygon_vector[i].holes_end();
          ++hit )
    {

      Polygon_2 hole;

      state = 2;
      for ( cit  = hit->curves_begin();
            cit != hit->curves_end();
            ++cit )
      {

        if ( cit->is_linear() ||
             cit->is_circular() )
        {
          Offset_point_2 op = cit->source();
          x = CGAL::to_double( op.x() );
          y = CGAL::to_double( op.y() );

          if (state == 2)
          {
            fprintf( ofp, "g1 z%f\n", gZSafe );
            fprintf( ofp, "g0 x%f y%f\n", x, y );
            state = 1;
            continue;
          }
          else if (state == 1)
          {
            fprintf( ofp, "g1 z%f\n", gZCut );
            state = 0;
          }

          fprintf( ofp, "g1 x%f y%f\n", x, y );

        }
      }

      fprintf( ofp, "g1 z%f\n", gZSafe );

    }

  }

}



