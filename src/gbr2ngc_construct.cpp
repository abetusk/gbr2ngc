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

#define _isnan std::isnan

#define DEBUG_CONSTRUCT

class Gerber_point_2 {
  public:
    int ix, iy;
    double x, y;
    int jump_pos;
};

class irange_2 {
  public:
    int start, end;
};

typedef struct dpoint_2_type {
  int ix, iy;
  double x, y;
} dpoint_2_t;

struct Gerber_point_2_cmp {

  bool operator()(const Gerber_point_2 &lhs, const Gerber_point_2 &rhs) const {
    if (lhs.ix < rhs.ix) return true;
    if (lhs.ix > rhs.ix) return false;
    if (lhs.iy < rhs.iy) return true;
    return false;
  }

};

typedef std::map< Gerber_point_2, int, Gerber_point_2_cmp  > PointPosMap;
typedef std::pair< Gerber_point_2, int > PointPosMapPair;

typedef std::map< Gerber_point_2, int , Gerber_point_2_cmp  > HolePosMap;

//--

static int _get_segment_count(double r, double min_segment_length, int min_segments) {
  int segments;
  double c, theta, z;

  segments=min_segments;
  c = 2.0 * M_PI * r;

  z = c / min_segment_length;
  if (_isnan(z)) { return min_segments; }
  if ((int)z < min_segments) {
    return min_segments;
  }
  return (int)z;
}


// add a hole to the vector hole_vec from the Gerber_point_2
// vector p.  The points are decorated with a 'jump_pos' which
// holds the position of the first time the path comes back
// to the same point.  The path can come back to the same
// point more than twice, so we need to skip over the duplicate
// points.
//
// Here is some ASCII art to illustrate:
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
void add_hole( Paths &hole_vec,
               std::vector< Gerber_point_2 > &p,
               int start,
               int n ) {
  int i, ds, jp, end;
  Path hole_polygon;

  end = start+n;

  jp = p[start].jump_pos;

  if (jp<0) {
    fprintf(stderr, "ERROR: jump pos MUST be > 0, is %i, exiting (start %i, n %i)\n", jp, start, n );
    printf("ERROR: jump pos MUST be > 0, is %i, exiting (start %i, n %i)\n", jp, start, n );
    exit(2);
  }

  while ( p[jp].jump_pos >= 0 ) {
    ds = p[start].jump_pos - (start+1);
    add_hole( hole_vec, p, start+1, ds );
    start = jp;
    jp = p[start].jump_pos;
  }

  n = end-start;
  hole_polygon.push_back( dtoc(p[start].x, p[start].y) );

  for (i=start+1; i<(start+n); i++) {
    hole_polygon.push_back( dtoc(p[i].x, p[i].y) );
    if (p[i].jump_pos<0) continue;

    ds = p[i].jump_pos - (i+1);
    add_hole( hole_vec, p, i+1, ds);
    i = p[i].jump_pos;

  }

  if (hole_polygon.size() > 2)
    hole_vec.push_back( hole_polygon );

}

// Copy linked list points in contour to vector p, removing contiguougs
// duplicates.
//
void populate_gerber_point_vector_from_contour( std::vector< Gerber_point_2 > &p,
                                                gerber_region_t *contour) {
  int first=1;
  Gerber_point_2 prev_pnt;
  Gerber_point_2 dpnt;

  // Get rid of points that are duplicated next to each other.
  // It does no good to have them and it just makes things more
  // complicated downstream to work around them.
  //
  while (contour) {
    dpnt.ix = int(contour->x * 1000000.0);
    dpnt.iy = int(contour->y * 1000000.0);
    dpnt.x = contour->x;
    dpnt.y = contour->y;

    if (first) {
      p.push_back( dpnt );
    }
    else if ( (prev_pnt.ix == dpnt.ix) &&
              (prev_pnt.iy == dpnt.iy) ) {
      // duplicate
    }
    else {
      p.push_back( dpnt );
    }

    prev_pnt = dpnt;
    first = 0;

    contour = contour->next;
  }

}


// Create the start and end positions of the holes in the vector p.
// Store in hole_map.
//
int gerber_point_2_decorate_with_jump_pos( std::vector< Gerber_point_2 > &p ) {
  unsigned int i;
  int k;
  HolePosMap hole_map;

  for (i=0; i<p.size(); i++) {
    p[i].jump_pos = -1;

    // if found in p_map, add it to our map
    //
    if ( hole_map.find( p[i] ) != hole_map.end() ) {
      k = hole_map[ p[i] ];
      p[ k ].jump_pos = i;

      hole_map[ p[i] ] = i;
    }

    // otherwise add it
    //
    else {
      hole_map[ p[i] ] = i;
    }

  }

  return 0;
}


//----------------------------------
//
//

// Very similar to add_hole but special care needs to be taken for the outer
// contour.
// Construct the point vector and hole map.  Create the polygon with holes
// vector.
//
int construct_contour_region( PathSet &pwh_vec, gerber_region_t *contour ) {
  int i, ds;

  std::vector< Gerber_point_2 > p;

  Path path;
  Paths pwh, soln;

  Clipper clip;

  // Initially populate p vector
  //
  populate_gerber_point_vector_from_contour( p, contour );

  // Find the start and end regions for each of the
  // holes.
  //
  gerber_point_2_decorate_with_jump_pos( p );

  if (p.size()==0) { return -1; }

  int n = p.size();
  for (i=0; i<n; i++) {

    path.push_back( dtoc( p[i].x, p[i].y ) );
    if (p[i].jump_pos < 0) { continue; }

    // Special case when the boundary end ties back to the beginning
    // without any jumps to holes.
    //
    if (p[i].jump_pos == (n-1)) { continue; }

    ds = p[i].jump_pos - (i+1);
    add_hole( pwh, p, i+1, ds );
    i += ds;

  }
  pwh.push_back(path);

  // The contour doesn't necessarily start on the outer boundary,
  // so we need to do an even/odd union to get the proper
  // region.
  //
  clip.AddPaths( pwh, ptSubject, true );
  clip.Execute( ctUnion, soln, pftEvenOdd, pftEvenOdd );

  pwh_vec.push_back( soln );

  return 0;
}


//-----------------------------------------

bool isccw( Path &p ) {
  unsigned int i;
  cInt dx, dy, s = 0;

  for (i=1; i<p.size(); i++) {
    dx = p[i].X - p[0].X;
    dy = p[i].Y - p[0].Y;
    s += dx*dy;
  }

  if (s < 0) return true;
  return false;
}

static void debug_gAperture() {
  int key;
  Aperture_realization *rlz;
  ApertureNameMap::iterator it;

  for (it = gAperture.begin(); it != gAperture.end(); ++it ) {
    key = it->first;
    rlz = &(it->second);

    printf("## debug_gAperture: key:%i\n", key);
  }

}

// construct the final polygon set (before offsetting)
//   of the joined polygons from the parsed gerber file.
//
// This got pretty complex because of all the extra features
// of Gerber (and still more to be implemented).
// A brief overview:
//
// * For simple geometry, it's union and difference operations of aperture
//   definitions or other primitives
// * Apertures have an array of polarity to construc tthem piece wise
// * step repeat and aperture block recur to resolve the geometry
// * Since polarity and other state information is global, the root
//   `gerber_state_t` structure is updated and referenced to determine
//   global parameter values
//
//
int join_polygon_set_r(Paths &result, Clipper &clip, gerber_state_t *gs, IntPoint &virtual_origin, int level) {
  unsigned int i, ii, jj, _i, _j;

  gerber_item_ll_t *item_nod;
  gerber_region_t *region;

  PathSet temp_pwh_vec;
  IntPoint prev_pnt, cur_pnt, _origin;

  int n=0, name=0, _path_polarity=1;
  int polarity = 1, d_name = -1;

  Path clip_path, point_list, res_point, tmp_path;
  Paths clip_paths, clip_result,
        aperture_geom, it_paths, tmp_paths;
  Clipper aperture_clip;

  double dx, dy;

  IntPoint _prv_arc_pnt;
  double ang_rad, tx, ty, tr, _p;
  int n_seg = 16;

  int stack_polarity, stack_d_name;
  gerber_state_t *stack_gs;



  //--

  for (item_nod = gs->item_head;
       item_nod;
       item_nod = item_nod->next) {

    //polarity = gs->_root_gerber_state->polarity;
    //d_name = gs->_root_gerber_state->d_state;

    polarity = gs->polarity;
    d_name = gs->d_state;

    //--

    if (item_nod->type == GERBER_LP) {
      polarity = item_nod->polarity;

      //gs->_root_gerber_state->polarity = polarity;
      gs->polarity = polarity;

      continue;
    }

    //--

    else if (item_nod->type == GERBER_D10P) {

      //gs->_root_gerber_state->d_state = item_nod->d_name;
      //d_name = gs->_root_gerber_state->d_state;

      gs->d_state = item_nod->d_name;
      d_name = item_nod->d_name;

      continue;
    }

    //--

    else if (item_nod->type == GERBER_REGION) {

      temp_pwh_vec.clear();
      construct_contour_region(temp_pwh_vec, item_nod->region_head);

      for (i=0; i<temp_pwh_vec.size(); i++) {
        clip.AddPaths( temp_pwh_vec[i], ptSubject, true );
      }

      //EXPERIMENT
      clip.Execute( ctUnion, result, pftNonZero, pftNonZero );

      continue;
    }

    //--

    // segments are a 'streak' of a primitive.
    // If the primitive has holes in it, they're ignored.
    // The source and destination point are used to createa  convecx
    // hull of the basic geometry type.
    //
    else if (item_nod->type == GERBER_SEGMENT) {

      name = d_name;

      region = item_nod->region_head;
      if (!region) { fprintf(stderr, "error (0)"); return -1; }

      dx = region->x;
      dy = region->y;

      prev_pnt = dtoc( region->x, region->y );

      region = region->next;
      if (!region) { fprintf(stderr, "error (1)"); return -1; }

      cur_pnt = dtoc( region->x, region->y );

      point_list.clear();
      res_point.clear();

      _origin = prev_pnt;
      _origin.X += virtual_origin.X;
      _origin.Y += virtual_origin.Y;

      for (ii=0; ii<gAperture[ name ].m_path.size(); ii++) {
        for (jj=0; jj<gAperture[ name ].m_path[ii].size(); jj++) {
          point_list.push_back( IntPoint( gAperture[ name ].m_path[ii][jj].X + _origin.X,
                                          gAperture[ name ].m_path[ii][jj].Y + _origin.Y  ) );
        }
      }

      _origin = cur_pnt;
      _origin.X += virtual_origin.X;
      _origin.Y += virtual_origin.Y;

      for (ii=0; ii<gAperture[ name ].m_path.size(); ii++) {
        for (jj=0; jj<gAperture[ name ].m_path[ii].size(); jj++) {
          point_list.push_back( IntPoint( gAperture[ name ].m_path[ii][jj].X + _origin.X ,
                                          gAperture[ name ].m_path[ii][jj].Y + _origin.Y ) );
        }
      }

      ConvexHull( point_list, res_point );

      if (res_point.size() == 0) {
        fprintf(stdout, "# WARNING: empty polygon found for name %i, skipping\n", name);
        fprintf(stderr, "# WARNING: empty polygon found for name %i, skipping\n", name);
      }
      else {

        if (!Orientation(res_point)) { ReversePath(res_point); }

        if (_expose_bit(polarity, gAperture[name].m_exposure[0])) {
          clip.AddPath( res_point, ptSubject, true );
        }
        else {
          clip.AddPath(res_point, ptClip, true);

          it_paths.clear();
          clip.Execute(ctDifference, it_paths, pftNonZero, pftNonZero);

          clip.Clear();
          clip.AddPaths(it_paths, ptSubject, true);
        }

        //EXPERIMENT
        clip.Execute( ctUnion, result, pftNonZero, pftNonZero );

      }

    }

    //--

    else if (item_nod->type == GERBER_SEGMENT_ARC) {
      name = d_name;

      _get_segment_count(item_nod->arc_r, gMinSegmentLength, gMinSegment);

      ang_rad = item_nod->arc_ang_rad_beg;
      tr = item_nod->arc_r;
      tx = tr*cos(ang_rad) + item_nod->arc_center_x;
      ty = tr*sin(ang_rad) + item_nod->arc_center_y;

      _prv_arc_pnt = dtoc( tx, ty );

      for (int _segment=1; _segment<n_seg; _segment++) {

        _p = ((double)_segment) / ((double)(n_seg-1));

        ang_rad = item_nod->arc_ang_rad_beg;
        ang_rad += item_nod->arc_ang_rad_del * _p;

        tr = item_nod->arc_r + (_p * item_nod->arc_r_deviation);
        tx = tr*cos(ang_rad) + item_nod->arc_center_x;
        ty = tr*sin(ang_rad) + item_nod->arc_center_y;

        cur_pnt = dtoc( tx, ty );

        _origin = _prv_arc_pnt;
        _origin.X += virtual_origin.X;
        _origin.Y += virtual_origin.Y;

        point_list.clear();
        res_point.clear();

        for (ii=0; ii<gAperture[ name ].m_path.size(); ii++) {
          for (jj=0; jj<gAperture[ name ].m_path[ii].size(); jj++) {
            point_list.push_back( IntPoint( gAperture[ name ].m_path[ii][jj].X + _origin.X,
                                            gAperture[ name ].m_path[ii][jj].Y + _origin.Y  ) );
          }
        }

        _origin = cur_pnt;
        _origin.X += virtual_origin.X;
        _origin.Y += virtual_origin.Y;

        for (ii=0; ii<gAperture[ name ].m_path.size(); ii++) {
          for (jj=0; jj<gAperture[ name ].m_path[ii].size(); jj++) {
            point_list.push_back( IntPoint( gAperture[ name ].m_path[ii][jj].X + _origin.X ,
                                            gAperture[ name ].m_path[ii][jj].Y + _origin.Y ) );
          }
        }

        ConvexHull( point_list, res_point );

        if (res_point.size() == 0) {
          fprintf(stdout, "# WARNING: empty polygon found for name %i, skipping\n", name);
          fprintf(stderr, "# WARNING: empty polygon found for name %i, skipping\n", name);
        }
        else {

          if (!Orientation(res_point)) { ReversePath(res_point); }

          if (_expose_bit(polarity, gAperture[name].m_exposure[0])) {
            clip.AddPath( res_point, ptSubject, true );
          }
          else {
            clip.AddPath(res_point, ptClip, true);

            it_paths.clear();
            clip.Execute(ctDifference, it_paths, pftNonZero, pftNonZero);

            clip.Clear();
            clip.AddPaths(it_paths, ptSubject, true);
          }

          clip.Execute( ctUnion, result, pftNonZero, pftNonZero );
        }

        _prv_arc_pnt = cur_pnt;
      }

    }

    //--

    else if (item_nod->type == GERBER_FLASH) {

      //name = item_nod->d_name;
      name = d_name;
      cur_pnt = dtoc( item_nod->x, item_nod->y );

      // Simple aperture
      //
      if ( gAperture.find(name) != gAperture.end() ) {

        _origin = cur_pnt;
        _origin.X += virtual_origin.X;
        _origin.Y += virtual_origin.Y;

        aperture_clip.Clear();
        aperture_geom.clear();
        for (ii=0; ii<gAperture[ name ].m_path.size(); ii++) {

          tmp_path.clear();
          for (jj=0; jj<gAperture[ name ].m_path[ii].size(); jj++) {
            tmp_path.push_back( IntPoint( gAperture[ name ].m_path[ii][jj].X + _origin.X,
                                          gAperture[ name ].m_path[ii][jj].Y + _origin.Y ) );
          }

          if (tmp_path.size() < 2) { fprintf(stdout, "## WARNING, tmp_path.size() %i\n", (int)tmp_path.size()); fflush(stdout); continue; }

          int last_idx = (int)(tmp_path.size()-1);
          if ((tmp_path[0].X != tmp_path[last_idx].X) &&
              (tmp_path[0].Y != tmp_path[last_idx].Y)) {
            fprintf(stdout, "## WARNING, tmp_path for %i is not closed!\n", name); fflush(stdout);
            tmp_path.push_back(tmp_path[0]);
          }

          if (gAperture[name].m_exposure[ii]) {
            aperture_clip.AddPath(tmp_path, ptSubject, true);
          }
          else {
            aperture_clip.AddPath(tmp_path, ptClip, true);
            aperture_clip.Execute(ctDifference , aperture_geom, pftNonZero, pftNonZero);
            aperture_clip.Clear();
            aperture_clip.AddPaths(aperture_geom, ptSubject, true);
            aperture_geom.clear();
          }

        }

        it_paths.clear();
        aperture_clip.Execute( ctUnion, it_paths, pftNonZero, pftNonZero );

        if ( ! _expose_bit(polarity) ) {
          clip.AddPaths(it_paths, ptClip, true);
          it_paths.clear();
          clip.Execute(ctDifference, it_paths, pftNonZero, pftNonZero);
          clip.Clear();
        }

        clip.AddPaths( it_paths, ptSubject, true );

        //EXPERIMENT
        clip.Execute( ctUnion, result, pftNonZero, pftNonZero );

      }

      // Aperture blocks
      //
      else if ( gApertureBlock.find(name) != gApertureBlock.end() ) {
        _origin = cur_pnt;
        _origin.X += virtual_origin.X;
        _origin.Y += virtual_origin.Y;

        // AB realization does not change global state so "reset" it after every
        // round (currently only polarity, will need to reset other
        // state variables as well, later).
        //
        //polarity = gs->_root_gerber_state->polarity;
        //d_name = gs->_root_gerber_state->d_state;
        //polarity = gs->polarity;
        //d_name = gs->d_state;

        stack_gs        = gApertureBlock[name];
        stack_polarity  = stack_gs->polarity;
        stack_d_name    = stack_gs->d_state;

        //join_polygon_set_r( result, clip, gApertureBlock[name], _origin, level+1 );
        join_polygon_set_r( result, clip, stack_gs, _origin, level+1 );

        stack_gs->polarity  = stack_polarity;
        stack_gs->d_state   = stack_d_name;

        // Restore state.
        //
        //gs->_root_gerber_state->polarity = polarity;
        //gs->_root_gerber_state->d_state = d_name;

      }

      // error
      //
      else {
        fprintf(stderr, "ERROR: could not find %i in aperture list\n", name);
        fprintf(stdout, "ERROR: could not find %i in aperture list\n", name);
        return -1;
      }

    }

    //--

    else if (item_nod->type == GERBER_SR) {

      for (jj=0; jj<item_nod->sr_y; jj++) {
        for (ii=0; ii<item_nod->sr_x; ii++) {

          dx = (double) (((double)ii) * (item_nod->sr_i));
          dy = (double) (((double)jj) * (item_nod->sr_j));

          _origin = dtoc( item_nod->x + dx, item_nod->y + dy );
          _origin.X += virtual_origin.X;
          _origin.Y += virtual_origin.Y;

          // The SR inherits the polarity and current D name
          //
          item_nod->step_repeat->polarity = polarity;
          item_nod->step_repeat->d_state = d_name;

          join_polygon_set_r(result, clip, item_nod->step_repeat, _origin, level+1);

          // SR affects state, so percolate state changes up
          //
          gs->polarity = polarity;
          gs->d_state = item_nod->step_repeat->d_state;

        }
      }

    }
  }

  //clip.Execute( ctUnion, result, pftNonZero, pftNonZero );

  return 0;
}

int join_polygon_set(Paths &result, gerber_state_t *gs) {
  Clipper clip;
  IntPoint origin;

  origin.X = 0;
  origin.Y = 0;
  return join_polygon_set_r(result, clip, gs, origin, 0);
}

