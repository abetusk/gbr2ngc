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
    int64_t ix, iy;
    double x, y;
    int64_t jump_pos;
};

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
void populate_gerber_point_vector_from_contour( gerber_state_t *gs,
                                                std::vector< Gerber_point_2 > &p,
                                                gerber_item_ll_t *contour) {
  int first=1, n_seg=16, n;
  double C = 1000000000000.0;
  Gerber_point_2 prev_pnt;
  Gerber_point_2 dpnt;

  double ang_rad, tr, tx, ty, _p;
  int _segment;

  // Get rid of points that are duplicated next to each other.
  // It does no good to have them and it just makes things more
  // complicated downstream to work around them.
  //
  for ( ; contour ; contour = contour->next) {

    switch (contour->type) {
      case GERBER_REGION_G74: gs->quadrent_mode = QUADRENT_MODE_SINGLE; continue; break;
      case GERBER_REGION_G75: gs->quadrent_mode = QUADRENT_MODE_MULTI; continue; break;
      case GERBER_REGION_G01: gs->interpolation_mode = INTERPOLATION_MODE_LINEAR; continue; break;
      case GERBER_REGION_G02: gs->interpolation_mode = INTERPOLATION_MODE_CW; continue; break;
      case GERBER_REGION_G03: gs->interpolation_mode = INTERPOLATION_MODE_CCW; continue; break;
      default: break;
    }

    if (contour->type == GERBER_REGION_SEGMENT) {

      dpnt.ix = (int64_t)(contour->x * C);
      dpnt.iy = (int64_t)(contour->y * C);

      dpnt.x = contour->x;
      dpnt.y = contour->y;

      if (first) { p.push_back( dpnt ); }
      else if ( (prev_pnt.ix == dpnt.ix) &&
                (prev_pnt.iy == dpnt.iy) ) {
        // duplicate
      }
      else { p.push_back( dpnt ); }

      prev_pnt = dpnt;
      first = 0;

    }
    else if (contour->type == GERBER_REGION_SEGMENT_ARC) {

      n_seg = _get_segment_count(contour->arc_r, gMinSegmentLength, gMinSegment);

      for (_segment=1; _segment<n_seg; _segment++) {

        _p = ((double)_segment) / ((double)(n_seg-1));

        ang_rad = contour->arc_ang_rad_beg;
        ang_rad += contour->arc_ang_rad_del * _p;

        tr = contour->arc_r + (_p * contour->arc_r_deviation);
        tx = tr*cos(ang_rad) + contour->arc_center_x;
        ty = tr*sin(ang_rad) + contour->arc_center_y;

        dpnt.ix = (int64_t)(tx * C);
        dpnt.iy = (int64_t)(ty * C);

        dpnt.x = tx;
        dpnt.y = ty;

        if (first) { p.push_back( dpnt ); }
        else if ( (prev_pnt.ix == dpnt.ix) &&
                  (prev_pnt.iy == dpnt.iy) ) {
          // duplicate
        }
        else { p.push_back( dpnt ); }

        prev_pnt = dpnt;
        first = 0;

      }

    }
    else if (contour->type == GERBER_REGION_MOVE) {

      dpnt.ix = (int64_t)(contour->x * C);
      dpnt.iy = (int64_t)(contour->y * C);

      dpnt.x = contour->x;
      dpnt.y = contour->y;

      n = p.size();
      if ((n>0) && (p[n-1].ix == dpnt.ix) && (p[n-1].iy == dpnt.iy)) { }
      else { p.push_back( dpnt ); }

      prev_pnt = dpnt;
      first = 0;
    }
    else {
      fprintf(stderr, "WARNING: populate_gerber_point_vector_from_contour found unknown type %i\n", contour->type);
    }

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
//int construct_contour_region( PathSet &pwh_vec, gerber_region_t *contour ) {
int construct_contour_region( gerber_state_t *gs, PathSet &pwh_vec, gerber_item_ll_t *contour ) {
  int i, ds;

  std::vector< Gerber_point_2 > p;

  Path path;
  Paths pwh, soln;

  Clipper clip;

  gerber_item_ll_t *xnod;

  // Initially populate p vector
  //
  populate_gerber_point_vector_from_contour( gs, p, contour );

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

//--

static void _construct_transform_matrix(double *M, int mirror_axis, double rot_deg, double scale, double tx, double ty) {
  double c, s, t;

  c = cos(rot_deg * M_PI / 180.0 );
  s = sin(rot_deg * M_PI / 180.0 );

  memset(M, 0, sizeof(double)*3*3);
  M[0] = 1.0;
  M[3*1 + 1] = 1.0;
  M[3*2 + 2] = 1.0;

  M[0*3 + 0] =  c;
  M[0*3 + 1] = -s;

  M[1*3 + 0] =  s;
  M[1*3 + 1] =  c;

  M[0*3 + 0] *= scale;
  M[0*3 + 1] *= scale;
  M[1*3 + 0] *= scale;
  M[1*3 + 1] *= scale;

  M[0*3 + 2] = tx;
  M[1*3 + 2] = ty;

  if (mirror_axis == MIRROR_AXIS_X) {
    M[0*3 + 0] *= -1.0;
    M[1*3 + 0] *= -1.0;
  }
  else if (mirror_axis == MIRROR_AXIS_Y) {
    M[0*3 + 1] *= -1.0;
    M[1*3 + 1] *= -1.0;
  }
  else if (mirror_axis == MIRROR_AXIS_XY) {
    M[0*3 + 0] *= -1.0;
    M[0*3 + 1] *= -1.0;
    M[1*3 + 0] *= -1.0;
    M[1*3 + 1] *= -1.0;
  }

  return;
}

static void _mulmat3x3(double *result, double *A, double *B) {
  int r,c,k;
  double tm[3*3];
  memset(tm, 0, sizeof(double)*3*3);
  for (r=0; r<3; r++) for (c=0; c<3; c++) for (k=0;  k<3; k++) {
    tm[3*r + c] += A[3*r + k] * B[3*k + c];
  }
  memcpy(result, tm, sizeof(double)*3*3);
  return;
}

void _mulvec3(double *result, double *M, double *v) {
  int r,c,k;
  double tv[3];
  memset(tv, 0, sizeof(double)*3);
  for (r=0; r<3; r++) for (k=0; k<3; k++) {
    tv[r] += M[3*r + k] * v[k];
  }
  memcpy(v, tv, sizeof(double)*3);
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
int join_polygon_set_r(Paths &result, Clipper &clip, gerber_state_t *gs, double *transformMatrixParent, int level) {
  unsigned int i, ii, jj, _i, _j;

  gerber_item_ll_t *item_nod;
  gerber_item_ll_t *region;

  PathSet temp_pwh_vec;
  IntPoint prev_pnt, cur_pnt, _origin;

  int n=0, name=0, _path_polarity=1;
  int polarity = 1, d_name = -1;
  int pmrs_active = 0;

  Path clip_path, point_list, res_point, tmp_path;
  Paths clip_paths, clip_result,
        aperture_geom, it_paths, tmp_paths;
  Clipper aperture_clip;

  double dx, dy;

  cInt tX, tY, uX, uY, vX, vY;
  IntPoint _pnt0, _pnt1, _pnt;

  IntPoint _prv_arc_pnt;
  double ang_rad, tx, ty, tr, _p;
  int n_seg = 16;

  double _odx, _ody,  _x, _y;

  int stack_polarity, stack_d_name, stack_pmrs_active;
  int stack_mirror;
  double stack_rot, stack_scale;
  gerber_state_t *stack_gs;

  int _clip_update = 0;

  double transformMatrix[3*3];
  double dpnt[3];

  struct timeval tv;

  memcpy(transformMatrix, transformMatrixParent, sizeof(double)*3*3);
  memset( dpnt, 0, sizeof(double)*3);

  //--

  for (item_nod = gs->item_head;
       item_nod;
       item_nod = item_nod->next) {

    _clip_update = 0;
    polarity = gs->polarity;
    d_name = gs->d_state;

    //--

    if (item_nod->type == GERBER_LP) {
      polarity = item_nod->polarity;

      gs->polarity = polarity;
      continue;
    }

    else if (item_nod->type == GERBER_LM) {
      gs->mirror_axis = item_nod->mirror_axis;
      continue;
    }
    else if (item_nod->type == GERBER_LR) {
      gs->rotation_degree = item_nod->rotation_degree;
      continue;
    }
    else if (item_nod->type == GERBER_LS) {
      gs->scale = item_nod->scale;
      continue;
    }

    //--

    else if (item_nod->type == GERBER_D10P) {

      gs->d_state = item_nod->d_name;
      d_name = item_nod->d_name;
      continue;
    }

    //--

    else if (item_nod->type == GERBER_REGION) {

      temp_pwh_vec.clear();
      construct_contour_region(gs, temp_pwh_vec, item_nod->region_head);

      if ( ! _expose_bit(polarity) ) {

        for (i=0; i<temp_pwh_vec.size(); i++) {
          clip.AddPaths( temp_pwh_vec[i], ptClip, true );
        }

        it_paths.clear();
        clip.Execute(ctDifference, it_paths, pftNonZero, pftNonZero);

        clip.Clear();
        clip.AddPaths(it_paths, ptSubject, true);

        _clip_update = 1;
      }
      else {

        for (i=0; i<temp_pwh_vec.size(); i++) {
          clip.AddPaths( temp_pwh_vec[i], ptSubject, true );
        }

      }



      if (_clip_update) {
        clip.Execute( ctUnion, result, pftNonZero, pftNonZero );
      }

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

      _construct_transform_matrix(transformMatrix, gs->mirror_axis, gs->rotation_degree, gs->scale, ctod(prev_pnt.X), ctod(prev_pnt.Y));
      _mulmat3x3(transformMatrix, transformMatrixParent, transformMatrix);

      for (ii=0; ii<gAperture[ name ].m_path.size(); ii++) {
        for (jj=0; jj<gAperture[ name ].m_path[ii].size(); jj++) {

          dpnt[0] = ctod( gAperture[ name ].m_path[ii][jj].X );
          dpnt[1] = ctod( gAperture[ name ].m_path[ii][jj].Y );
          dpnt[2] = 1.0;

          _mulvec3(dpnt, transformMatrix, dpnt);

          point_list.push_back( dtoc( dpnt[0], dpnt[1] ) );

        }
      }

      _construct_transform_matrix(transformMatrix, gs->mirror_axis, gs->rotation_degree, gs->scale, ctod(cur_pnt.X), ctod(cur_pnt.Y));
      _mulmat3x3(transformMatrix, transformMatrixParent, transformMatrix);

      for (ii=0; ii<gAperture[ name ].m_path.size(); ii++) {
        for (jj=0; jj<gAperture[ name ].m_path[ii].size(); jj++) {

          dpnt[0] = ctod( gAperture[ name ].m_path[ii][jj].X );
          dpnt[1] = ctod( gAperture[ name ].m_path[ii][jj].Y );
          dpnt[2] = 1.0;

          _mulvec3(dpnt, transformMatrix, dpnt);

          point_list.push_back( dtoc( dpnt[0], dpnt[1] ) );

        }
      }

      res_point.clear();
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

          _clip_update = 1;
        }

        if (_clip_update) {
          clip.Execute( ctUnion, result, pftNonZero, pftNonZero );
        }

      }

    }

    //--

    else if (item_nod->type == GERBER_SEGMENT_ARC) {


      name = d_name;

      n_seg = _get_segment_count(item_nod->arc_r, gMinSegmentLength, gMinSegment);

      ang_rad = item_nod->arc_ang_rad_beg;
      tr = item_nod->arc_r;
      tx = tr*cos(ang_rad) + item_nod->arc_center_x;
      ty = tr*sin(ang_rad) + item_nod->arc_center_y;

      for (int _segment=1; _segment<n_seg; _segment++) {

        _p = ((double)(_segment-1)) / ((double)(n_seg-1));

        ang_rad = item_nod->arc_ang_rad_beg;
        ang_rad += item_nod->arc_ang_rad_del * _p;

        tr = item_nod->arc_r + (_p * item_nod->arc_r_deviation);
        tx = tr*cos(ang_rad) + item_nod->arc_center_x;
        ty = tr*sin(ang_rad) + item_nod->arc_center_y;

        point_list.clear();

        _construct_transform_matrix(transformMatrix, gs->mirror_axis, gs->rotation_degree, gs->scale, tx, ty);
        _mulmat3x3(transformMatrix, transformMatrixParent, transformMatrix);

        for (ii=0; ii<gAperture[ name ].m_path.size(); ii++) {
          for (jj=0; jj<gAperture[ name ].m_path[ii].size(); jj++) {

            dpnt[0] = ctod( gAperture[ name ].m_path[ii][jj].X );
            dpnt[1] = ctod( gAperture[ name ].m_path[ii][jj].Y );
            dpnt[2] = 1.0;

            _mulvec3(dpnt, transformMatrix, dpnt);

            point_list.push_back( dtoc( dpnt[0], dpnt[1] ) );
          }
        }

        _p = ((double)(_segment)) / ((double)(n_seg-1));

        ang_rad = item_nod->arc_ang_rad_beg;
        ang_rad += item_nod->arc_ang_rad_del * _p;

        tr = item_nod->arc_r + (_p * item_nod->arc_r_deviation);
        tx = tr*cos(ang_rad) + item_nod->arc_center_x;
        ty = tr*sin(ang_rad) + item_nod->arc_center_y;

        _construct_transform_matrix(transformMatrix, gs->mirror_axis, gs->rotation_degree, gs->scale, tx, ty);
        _mulmat3x3(transformMatrix, transformMatrixParent, transformMatrix);

        for (ii=0; ii<gAperture[ name ].m_path.size(); ii++) {
          for (jj=0; jj<gAperture[ name ].m_path[ii].size(); jj++) {

            dpnt[0] = ctod( gAperture[ name ].m_path[ii][jj].X );
            dpnt[1] = ctod( gAperture[ name ].m_path[ii][jj].Y );
            dpnt[2] = 1.0;

            _mulvec3(dpnt, transformMatrix, dpnt);

            point_list.push_back( dtoc( dpnt[0], dpnt[1] ) );
          }
        }

        res_point.clear();
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

            _clip_update = 1;
          }

        }

        if (_clip_update) {
          clip.Execute( ctUnion, result, pftNonZero, pftNonZero );
        }

        _prv_arc_pnt = cur_pnt;
      }

    }

    //--

    else if (item_nod->type == GERBER_FLASH) {

      name = d_name;
      cur_pnt = dtoc( item_nod->x, item_nod->y );

      // Simple aperture
      //
      if ( gAperture.find(name) != gAperture.end() ) {

        _construct_transform_matrix(transformMatrix, gs->mirror_axis, gs->rotation_degree, gs->scale, item_nod->x, item_nod->y );
        _mulmat3x3(transformMatrix, transformMatrixParent, transformMatrix);

        aperture_clip.Clear();
        aperture_geom.clear();
        for (ii=0; ii<gAperture[ name ].m_path.size(); ii++) {

          tmp_path.clear();
          for (jj=0; jj<gAperture[ name ].m_path[ii].size(); jj++) {

            dpnt[0] = ctod( gAperture[ name ].m_path[ii][jj].X );
            dpnt[1] = ctod( gAperture[ name ].m_path[ii][jj].Y );
            dpnt[2] = 1.0;

            _mulvec3(dpnt, transformMatrix, dpnt);

            tmp_path.push_back( dtoc( dpnt[0], dpnt[1] ) );

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
          _clip_update = 1;
        }

        clip.AddPaths( it_paths, ptSubject, true );

        if (_clip_update) {
          clip.Execute( ctUnion, result, pftNonZero, pftNonZero );
        }

      }

      // Aperture block realization
      //
      else if ( gApertureBlock.find(name) != gApertureBlock.end() ) {

        // AB realization does not change global state so "reset" it after every
        // round (currently only polarity, will need to reset other
        // state variables as well, later).
        //
        stack_gs        = gApertureBlock[name];
        stack_polarity    = stack_gs->polarity;
        stack_d_name      = stack_gs->d_state;
        stack_pmrs_active = stack_gs->pmrs_active;
        stack_mirror      = stack_gs->mirror_axis;
        stack_rot         = stack_gs->rotation_degree;
        stack_scale       = stack_gs->scale;

        stack_gs->pmrs_active = pmrs_active;

        _construct_transform_matrix(transformMatrix, gs->mirror_axis, gs->rotation_degree, gs->scale, item_nod->x, item_nod->y);
        _mulmat3x3(transformMatrix, transformMatrixParent, transformMatrix);

        join_polygon_set_r( result, clip, stack_gs, transformMatrix, level+1 );

        // AB calls do not alter state but by convention, we alter state in the `gerber_state_t` structure,
        // so restore state after we're done with the aperture block realization
        //
        stack_gs->polarity    = stack_polarity;
        stack_gs->d_state     = stack_d_name;
        stack_gs->pmrs_active = stack_pmrs_active;
        stack_gs->mirror_axis = stack_mirror;
        stack_gs->scale       = stack_scale;
        stack_gs->rotation_degree = stack_rot;
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

          _construct_transform_matrix(transformMatrix, gs->mirror_axis, gs->rotation_degree, gs->scale, dx, dy);
          _mulmat3x3(transformMatrix, transformMatrixParent, transformMatrix);

          // The SR inherits the polarity and current D name
          //
          item_nod->step_repeat->polarity = polarity;
          item_nod->step_repeat->d_state = d_name;
          item_nod->step_repeat->pmrs_active = pmrs_active;

          //experimental
          item_nod->step_repeat->mirror_axis      = gs->mirror_axis;
          item_nod->step_repeat->rotation_degree  = gs->rotation_degree;
          item_nod->step_repeat->scale            = gs->scale;

          join_polygon_set_r(result, clip, item_nod->step_repeat, transformMatrix, level+1);

          // SR affects state, so percolate state changes up
          //
          gs->polarity = polarity;
          gs->d_state = item_nod->step_repeat->d_state;

          //TODO restore item_nod->step_repeat state

          //experimental
          gs->mirror_axis     = item_nod->step_repeat->mirror_axis;
          gs->rotation_degree = item_nod->step_repeat->rotation_degree;
          gs->scale           = item_nod->step_repeat->scale;

        }
      }

    }
  }

  if (_clip_update == 0) {
    clip.Execute( ctUnion, result, pftNonZero, pftNonZero );
  }

  return 0;
}

int join_polygon_set(Paths &result, gerber_state_t *gs) {
  Clipper clip;

  double ident[3*3];

  memset(ident, 0, sizeof(double)*3*3);
  ident[0] = 1.0;
  ident[4] = 1.0;
  ident[8] = 1.0;

  return join_polygon_set_r(result, clip, gs, ident, 0);
}

