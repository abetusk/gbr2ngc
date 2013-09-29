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

//------------------------------------------------------

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

//------------------------------------------------------

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

//------------------------------------------------------

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

//------------------------------------------------------

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



void print_profile(struct timeval *tv0, struct timeval *tv1)
{
  unsigned long long int t;

  t = tv1->tv_usec - tv0->tv_usec;
  t += (tv1->tv_sec - tv0->tv_sec)*1000000;

  printf("%llu", t);
}


