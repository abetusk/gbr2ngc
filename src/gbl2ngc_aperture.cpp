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

void realize_circle(Aperture_realization &ap, double r, int min_segments = 8, double min_segment_length = 0.01 )
{
  int i, segments;
  double c, theta, a;
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

}

void realize_rectangle( Aperture_realization &ap, double x, double y )
{
  ap.m_outer_boundary.push_back( Point_2( -x, -y ) );
  ap.m_outer_boundary.push_back( Point_2(  x, -y ) );
  ap.m_outer_boundary.push_back( Point_2(  x,  y ) );
  ap.m_outer_boundary.push_back( Point_2( -x,  y ) );
}


void realize_obround( Aperture_realization &ap, double x_len, double y_len, int min_segments = 8, double min_segment_length = 0.01 )
{
  int i, segments;
  double theta, a, r, z;
  double c = 2.0 * M_PI * r;

  theta = 2.0 * asin( min_segment_length / (2.0*r) );
  segments = (int)(c / theta);
  if (segments < min_segments)
    segments = min_segments;


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

}


void realize_polygon( Aperture_realization &ap, double r, int n_vert, double rot_deg)
{
  int i;
  double a;

  if ((n_vert < 3) || (n_vert > 12))
  {
    printf("ERROR! Number of polygon vertices out of range\n");
  }

  for (int i=0; i<n_vert; i++)
  {
    a = (2.0 * M_PI * (double)i / (double)n_vert) + (rot_deg * M_PI / 180.0);
    ap.m_outer_boundary.push_back( Point_2( r*cos( a ), r*sin( a ) ) );
  }

}


void realize_circle_hole( Aperture_realization &ap, double r, int min_segments = 8, double min_segment_length = 0.01 )
{
  int i, segments;
  double a, c, theta;

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

}


void realize_rectangle_hole( Aperture_realization &ap, double x, double y )
{

  ap.m_hole.push_back( Point_2( -x, -y ) );  //clockwise instead of contouer clockwise
  ap.m_hole.push_back( Point_2( -x,  y ) );
  ap.m_hole.push_back( Point_2(  x,  y ) );
  ap.m_hole.push_back( Point_2(  x, -y ) );

}


// Construct polygons that will be used for the apertures.
// Curves are linearized.  Circles have a minium of 8
// vertices.
//
int realize_apertures(gerber_state_t *gs)
{
  double min_segment_length = 0.01;
  int min_segments = 8;
  int base_mapping[] = {1, 2, 3, 3};

  aperture_data_block_t *aperture;

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
        realize_circle( ap, aperture->crop[0]/2.0, min_segments, min_segment_length );
        break;

      case 1:  // rectangle
        realize_rectangle( ap, aperture->crop[0]/2.0, aperture->crop[1]/2.0 );
        break;

      case 2:  // obround
        realize_obround( ap, aperture->crop[0], aperture->crop[1], min_segments, min_segment_length  );
        break;

      case 3:  // polygon
        realize_polygon( ap, aperture->crop[0], aperture->crop[1], aperture->crop[2] );
        break;

      default: break;
    }

    int base = ( (aperture->type < 4) ? base_mapping[ aperture->type ] : 0 );

    switch (aperture->crop_type)
    {
      case 0: // solid, do nothing
        break;

      case 1: // circle hole
        realize_circle_hole( ap, aperture->crop[base]/2.0, min_segments, min_segment_length );
        break;

      case 2: // rect hole
        realize_rectangle_hole( ap, aperture->crop[base]/2.0, aperture->crop[base+1]/2.0 );
        break;

      default: break;
    }

    gAperture.insert( ApertureNameMapPair(ap.m_name, ap) );
    gApertureName.push_back(ap.m_name);

  }

}


