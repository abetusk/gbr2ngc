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

//#define DEBUG_APERTURE

#define _isnan std::isnan

// local_exposure - { 1 - add, 0 - remove }
// global_exposure - { 1 - additive, 0 - subtractive}
//
/*
static int _expose_bit(int local_exposure, int global_exposure = 1) {
  int gbit=0;
  local_exposure  = ( (local_exposure  > 0) ? 1 : 0);
  gbit = ( (global_exposure > 0) ? 0 : 1);

  return local_exposure ^ gbit;
}
*/

//------------

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

//------------

void realize_circle( gerber_state_t *gs,
										 Aperture_realization &ap,
                     double r,
                     int min_segments = 8,
                     double min_segment_length = 0.01 ) {
  int i, idx, segments;
  double a;
  Path empty_path;

  segments = _get_segment_count(r, min_segment_length, min_segments);

  idx = (int)ap.m_path.size();
  ap.m_path.push_back(empty_path);
  for (i=0; i<segments; i++) {
    a = 2.0 * M_PI * (double)i / (double)segments ;
    ap.m_path[idx].push_back( dtoc( r*cos(a), r*sin(a) ) );
  }
  if (ap.m_path[idx].size() > 0) {
    ap.m_path[idx].push_back( ap.m_path[idx][0] );
  }

  //ap.m_exposure.push_back(1);
  ap.m_exposure.push_back( _expose_bit(1, gs->polarity) );
}

void realize_rectangle( gerber_state_t *gs,
												Aperture_realization &ap,
                        double x,
                        double y ) {
  int idx;
  Path empty_path;

  idx = (int)ap.m_path.size();
  ap.m_path.push_back(empty_path);

  ap.m_path[idx].push_back( dtoc( -x, -y ) );
  ap.m_path[idx].push_back( dtoc(  x, -y ) );
  ap.m_path[idx].push_back( dtoc(  x,  y ) );
  ap.m_path[idx].push_back( dtoc( -x,  y ) );
  ap.m_path[idx].push_back( dtoc( -x, -y ) );

  //ap.m_exposure.push_back(1);
  ap.m_exposure.push_back( _expose_bit(1, gs->polarity) );
}


void realize_obround( gerber_state_t *gs,
											Aperture_realization &ap,
                      double x_len,
                      double y_len,
                      int min_segments = 8,
                      double min_segment_length = 0.01 ) {
  int i, segments, idx;
  double r, a;
  Path empty_path;

  r = ( (fabs(x_len) < fabs(y_len)) ? x_len : y_len );

  segments = _get_segment_count(r, min_segment_length, min_segments);

  idx = (int)ap.m_path.size();
  ap.m_path.push_back(empty_path);
  if (x_len < y_len) {

    r = x_len / 2.0;

    // start at the top right
    //
    for (i=0; i <= (segments/2); i++) {
      a = 2.0 * M_PI * (double)i / (double)segments ;
      ap.m_path[idx].push_back( dtoc( r*cos(a), r*sin(a) + ((y_len/2.0) - r) ) );
    }

    // then the bottom
    //
    for (int i = (segments/2); i <= segments; i++) {
      a = 2.0 * M_PI * (double)i / (double)segments ;
      ap.m_path[idx].push_back( dtoc( r*cos(a), r*sin(a) - ((y_len/2.0) - r) ) );
    }

  }
  else if (y_len < x_len) {

    r = y_len / 2.0;

    // start at bottom right
    //
    for (i=0; i <= (segments/2); i++) {
      a = ( 2.0 * M_PI * (double)i / (double)segments ) - ( M_PI / 2.0 );
      ap.m_path[idx].push_back( dtoc( r*cos(a) + ((x_len/2.0) - r) , r*sin(a) ) );
    }

    // then the left
    //
    for (i=(segments/2); i <= segments; i++) {
      a = ( 2.0 * M_PI * (double)i / (double)segments ) - ( M_PI / 2.0 );
      ap.m_path[idx].push_back( dtoc( r*cos(a) - ((x_len/2.0) - r) , r*sin(a) ) );
    }

  }

  // circle
  //
  else {

    r = x_len / 2.0;
    for (i=0; i<segments; i++) {
      a = 2.0 * M_PI * (double)i / (double)segments ;
      ap.m_path[idx].push_back( dtoc( r*cos( a ), r*sin( a ) ) );
    }
  }

  if (ap.m_path[idx].size()>0) {
    ap.m_path[idx].push_back( ap.m_path[idx][0] );
  }

  //ap.m_exposure.push_back(1);
  ap.m_exposure.push_back( _expose_bit(1, gs->polarity) );
}

void realize_polygon( gerber_state_t *gs,
											Aperture_realization &ap,
                      double r,
                      int n_vert,
                      double rot_deg ) {
  int i, idx;
  double a;
  Path empty_path;

  if ((n_vert < 3) || (n_vert > 12)) {
    printf("ERROR! Number of polygon vertices out of range\n");
  }

  idx = (int)ap.m_path.size();
  ap.m_path.push_back(empty_path);
  for (i=0; i<n_vert; i++) {
    a = (2.0 * M_PI * (double)i / (double)n_vert) + (rot_deg * M_PI / 180.0);
    ap.m_path[idx].push_back( dtoc( r*cos( a ), r*sin( a ) ) );
  }

  if (ap.m_path[idx].size()>0) {
    ap.m_path[idx].push_back( ap.m_path[idx][0] );
  }

  //ap.m_exposure.push_back(1);
  ap.m_exposure.push_back( _expose_bit(1, gs->polarity) );
}

void realize_hole( gerber_state_t *gs,
									 Aperture_realization &ap,
                   double r,
                   int min_segments = 8,
                   double min_segment_length = 0.01 ) {
  int i, idx, segments;
  double a;
  Path empty_path;

  segments = _get_segment_count(r, min_segment_length, min_segments);

  idx = (int)ap.m_path.size();
  ap.m_path.push_back(empty_path);
  for (i=0; i<segments; i++) {

    // counter clockwise
    //
    a = -2.0 * M_PI * (double)i / (double)segments;
    ap.m_path[idx].push_back( dtoc( r*cos( a ), r*sin( a ) ) );

  }

  if (ap.m_path[idx].size()>0) {
    ap.m_path[idx].push_back( ap.m_path[idx][0] );
  }

  //ap.m_exposure.push_back(0);
  ap.m_exposure.push_back( _expose_bit(0, gs->polarity) );
}


/*
void realize_rectangle_hole( Aperture_realization &ap, double x, double y ) {

  // clockwise instead of contouer clockwise
  //
  ap.m_hole.push_back( dtoc( -x, -y ) );
  ap.m_hole.push_back( dtoc( -x,  y ) );
  ap.m_hole.push_back( dtoc(  x,  y ) );
  ap.m_hole.push_back( dtoc(  x, -y ) );

}
*/


am_ll_node_t *aperture_macro_lookup(am_ll_lib_t *am_lib_nod, const char *am_name) {
  while (am_lib_nod) {
    if ((am_lib_nod->am) &&
        (am_lib_nod->am->name) &&
        (strcmp(am_lib_nod->am->name, am_name)==0) ) {
      return am_lib_nod->am;
    }
    am_lib_nod = am_lib_nod->next;
  }
  return NULL;
}

// eval_line holds parameters to be interpreted
// macro_param holds global variables (the hack
// to allow passed in varaibles to be seen by the
// macro 'function' call)
// eval_param are the resuling variables
//
// return 0 on success, non-zero on error
//
int _eval_var( std::vector< double > &eval_param,
               char **eval_line,
               int n_eval_line,
               std::vector< double > &macro_param ) {
  int i, err=0;
  tes_expr *expr=NULL;
  tes_variable *vars=NULL;
  std::vector< std::string > varname;
  std::string s;
  double val;

  if (macro_param.size() > 0) {
    vars = (tes_variable *)malloc(sizeof(tes_variable)*macro_param.size());
    for (i=0; i<macro_param.size(); i++) {
      s.clear();
      s += "$";
      s += std::to_string(i+1);
      varname.push_back(s);
      vars[i].address = &(macro_param[i]);
      vars[i].type = TES_VARIABLE;
    }

    // varname is dynamically allocated, so we need to set the
    // string points here, after varname's size becomes static
    //
    for (i=0; i<macro_param.size(); i++) {
      vars[i].name = varname[i].c_str();
    }
  }

  for (i=0; i<n_eval_line; i++) {
    expr = tes_compile(eval_line[i], vars, macro_param.size(), &err);
    if (!expr) { if(vars) { free(vars); } return -1; }
    val = tes_eval(expr);
    eval_param.push_back(val);
    tes_free(expr);
  }

  if (vars) { free(vars); }
  return 0;
}


int eval_AM_var( am_ll_node_t *am_node,
                 std::vector< double > &macro_param ) {
  int i, j, k, err=0;
  tes_expr *expr=NULL;
  tes_variable *vars=NULL;
  std::vector< std::string > varname;
  std::string s;

  double val;
  Path path;

  int var_idx;

  vars = (tes_variable *)malloc(sizeof(tes_variable)*macro_param.size());
  for (i=0; i<(int)macro_param.size(); i++) {
    s.clear();
    s += "$";
    s += std::to_string(i+1);
    varname.push_back(s);
  }

  for (i=0; i<(int)macro_param.size(); i++) {
    vars[i].name = varname[i].c_str();
    vars[i].address = &(macro_param[i]);
    vars[i].type = TES_VARIABLE;
  }

  var_idx = atoi(am_node->varname + 1);
  var_idx--;
  if (var_idx<0) { free(vars); return -1; }

  expr = tes_compile(am_node->eval_line[0], vars, macro_param.size(), &err);
  if (!expr) { free(vars); return -2; }

  val = tes_eval(expr);

  if ((var_idx+1) > (int)macro_param.size()) {
    for (i=(int)macro_param.size(); i<=var_idx; i++) {
      macro_param.push_back(0.0);
    }
  }

  macro_param[var_idx] = val;

  tes_free(expr);
  free(vars);
  return 0;
}

int add_AM_circle( am_ll_node_t *am_node,
                   std::vector< double > &macro_param,
                   Paths &paths,
                   std::vector< int > &exposure ) {
  int i, j, k, err=0;

  std::vector< double > eval_param;

  Path path;
  int segments = 32;
  double a;
  int expose = 0;
  double px, py, c_a, s_a;
  double r=0.0, cx=0.0, cy=0.0, ang=0.0, ang_deg_ccw=0.0;

  double *dptr=NULL;

  err = _eval_var(eval_param, am_node->eval_line, am_node->n_eval_line, macro_param);
  if (err<0) { return err; }

  if (eval_param.size() < 2) { return -1; }

  if (eval_param[0] > 0.5) { expose = 1; }
  r = eval_param[1]/2;

  if (r <= 0.0) { return -2; }
  if (eval_param.size() >= 3) { cx = eval_param[2]; }
  if (eval_param.size() >= 4) { cy = eval_param[3]; }
  if (eval_param.size() >= 5) { ang_deg_ccw = eval_param[4]; }
  ang = ang_deg_ccw * M_PI / 180.0;

  c_a = cos(ang);
  s_a = sin(ang);

  for (i=0; i<segments; i++) {

    a = 2.0 * M_PI * (double)i / (double)segments;

    px = r*cos(a) + cx;
    py = r*sin(a) + cy;

    path.push_back( dtoc( c_a*px - s_a*py, s_a*px + c_a*py ) );
  }
  if (path.size()>0) { path.push_back( path[0] ); }

  paths.push_back(path);
  exposure.push_back(expose);

  return 0;
}

int add_AM_vector_line( am_ll_node_t *am_node,
                        std::vector< double > &macro_param,
                        Paths &paths,
                        std::vector< int > &exposure ) {
  int i, j, k, err=0;

  Path path;

  std::vector< double > eval_param;

  int segments = 8, expose = 0;
  double px, py, c_a, s_a;
  double width=0.0, ang=0.0, ang_deg_ccw=0.0;
  double sx=0.0, sy=0.0, ex=0.0, ey=0.0;
  double vang = 0.0, c_va=0.0, s_va=0.0;
  double dwl=0.0, dwx0=0.0, dwy0=0.0, dwx1=0.0, dwy1=0.0;

  err = _eval_var(eval_param, am_node->eval_line, am_node->n_eval_line, macro_param);
  if (err!=0) { return err; }

  if (eval_param.size() < 7) { return -1; }

  if (eval_param[0] > 0.5) { expose = 1; }
  width       = eval_param[1];
  sx          = eval_param[2];
  sy          = eval_param[3];
  ex          = eval_param[4];
  ey          = eval_param[5];
  ang_deg_ccw = eval_param[6];

  ang = ang_deg_ccw * M_PI / 180.0;
  c_a = cos(ang);
  s_a = sin(ang);

  vang = atan2( ey-sy, ex-sx );

  c_va = cos(vang + M_PI/2.0);
  s_va = sin(vang + M_PI/2.0);
  dwx0 = c_va * width / (2.0);
  dwy0 = s_va * width / (2.0);

  c_va = cos(vang - M_PI/2.0);
  s_va = sin(vang - M_PI/2.0);
  dwx1 = c_va * width / (2.0);
  dwy1 = s_va * width / (2.0);

  px = sx + dwx0;  py = sy + dwy0;
  path.push_back( dtoc( c_a*px - s_a*py, s_a*px + c_a*py ) );

  px = sx + dwx1;  py = sy + dwy1;
  path.push_back( dtoc( c_a*px - s_a*py, s_a*px + c_a*py ) );

  px = ex + dwx1;  py = ey + dwy1;
  path.push_back( dtoc( c_a*px - s_a*py, s_a*px + c_a*py ) );

  px = ex + dwx0;  py = ey + dwy0;
  path.push_back( dtoc( c_a*px - s_a*py, s_a*px + c_a*py ) );

  if (path.size() > 0) { path.push_back( path[0] ); }

  paths.push_back(path);
  exposure.push_back(expose);

  return 0;
}

int add_AM_center_line( am_ll_node_t *am_node,
                        std::vector< double > &macro_param,
                        Paths &paths,
                        std::vector< int > &exposure ) {
  int i, j, k, err=0;

  Path path;

  std::vector< double > eval_param;

  int segments = 8, expose = 0;
  double a, px, py, c_a, s_a;
  double width=0.0, height=0.0, cx=0.0, cy=0.0, ang=0.0, ang_deg_ccw=0.0;


  err = _eval_var(eval_param, am_node->eval_line, am_node->n_eval_line, macro_param);
  if (err!=0) { return err; }

  if (eval_param.size() < 3) { return -1; }

  if (eval_param[0] > 0.5) { expose = 1; }
  width  = eval_param[1];
  height = eval_param[2];
  if (eval_param.size() >= 4) { cx = eval_param[3]; }
  if (eval_param.size() >= 5) { cy = eval_param[4]; }
  if (eval_param.size() >= 6) { ang_deg_ccw = eval_param[5]; }
  ang = ang_deg_ccw * M_PI / 180.0;

  c_a = cos(ang);
  s_a = sin(ang);

  px = (+width/2) + cx;  py = (+height/2) + cy;
  path.push_back( dtoc( c_a*px - s_a*py, s_a*px + c_a*py ) );

  px = (-width/2) + cx;  py = (+height/2) + cy;
  path.push_back( dtoc( c_a*px - s_a*py, s_a*px + c_a*py ) );

  px = (-width/2) + cx;  py = (-height/2) + cy;
  path.push_back( dtoc( c_a*px - s_a*py, s_a*px + c_a*py ) );

  px = (+width/2) + cx;  py = (-height/2) + cy;
  path.push_back( dtoc( c_a*px - s_a*py, s_a*px + c_a*py ) );

  if (path.size() > 0) { path.push_back( path[0] ); }

  paths.push_back(path);
  exposure.push_back(expose);

  return 0;
}

int add_AM_polygon( am_ll_node_t *am_node,
                    std::vector< double > &macro_param,
                    Paths &paths,
                    std::vector< int > &exposure ) {
  int i, j, k, err=0;

  Path path;

  std::vector< double > eval_param;

  int segments = 0, expose = 0;
  int nvert =3;
  double a, diam, r;
  double cx=0.0, cy=0.0, ang_deg_ccw=0.0, ang=0.0;
  double px=0.0, py=0.0;

  err = _eval_var(eval_param, am_node->eval_line, am_node->n_eval_line, macro_param);
  if (err!=0) { return err; }

  if (eval_param.size() < 6) { return -1; }

  if (eval_param[0] > 0.5) { expose = 1; }
  nvert = (int)eval_param[1];
  cx = eval_param[2];
  cy = eval_param[3];
  diam = eval_param[4];
  ang_deg_ccw = eval_param[5];
  ang = ang_deg_ccw * M_PI / 180.0;

  if (nvert<3) { return -2; }
  if (nvert>12) { return -3; }

  r = diam/2.0;

  for (i=0; i<nvert; i++) {
    a = (double)i * M_PI * 2.0 / (double)nvert;
    a += ang;

    px = r * cos(a);
    py = r * sin(a);

    path.push_back( dtoc( cx + px, cy + py ) );
  }
  path.push_back( path[0] );

  paths.push_back(path);
  exposure.push_back(expose);

  return 0;
}

void _line_path( Path &path,
                 double cx,
                 double cy,
                 double width,
                 double height,
                 double ang ) {
  double c_a, s_a;
  double w2, h2;

  w2 = width/2.0;
  h2 = height/2.0;

  path.clear();

  c_a = cos(ang);
  s_a = sin(ang);

  path.push_back( dtoc( c_a*( w2) - s_a*( h2) + cx,  s_a*( w2) + c_a*( h2) + cy) );
  path.push_back( dtoc( c_a*(-w2) - s_a*( h2) + cx,  s_a*(-w2) + c_a*( h2) + cy) );
  path.push_back( dtoc( c_a*(-w2) - s_a*(-h2) + cx,  s_a*(-w2) + c_a*(-h2) + cy) );
  path.push_back( dtoc( c_a*( w2) - s_a*(-h2) + cx,  s_a*( w2) + c_a*(-h2) + cy) );
  path.push_back( path[0] );
}

int add_AM_moire( am_ll_node_t *am_node,
                  std::vector< double > &macro_param,
                  Paths &paths,
                  std::vector< int > &exposure ) {
  int i, j, k, err=0;
  int ii, jj;

  Path _path;
  Paths _paths, _tmp_paths;
  Clipper clip;

  std::vector< double > eval_param;

  int segments = 32, expose = 1;
  double a, r;
  double cx=0.0, cy=0.0, ang_deg_ccw=0.0, ang=0.0;
  double px=0.0, py=0.0;
  double outer_diam=0.0, ring_thick=0.0, ring_gap = 0.0;
  double hair_thick=0.0, hair_width = 0.0;
  int max_ring = 0;

  err = _eval_var(eval_param, am_node->eval_line, am_node->n_eval_line, macro_param);
  if (err!=0) { return err; }

  if (eval_param.size() < 9) { return -1; }

  cx = eval_param[0];
  cy = eval_param[1];
  outer_diam = eval_param[2];
  ring_thick = eval_param[3];
  ring_gap = eval_param[4];
  max_ring = (int)eval_param[5];
  hair_thick = eval_param[6];
  hair_width = eval_param[7];
  ang_deg_ccw = eval_param[8];

  ang = ang_deg_ccw * M_PI / 180.0;

  if (outer_diam <= 0.0) { return -2; }
  if (ring_thick <= 0.0) { return -3; }
  if (ring_gap <= 0.0) { return -4; }
  if (max_ring < 1) { return  -5; }
  if (hair_thick <= 0.0) { return -6; }
  if (hair_width <= 0.0) { return -7; }

  r = outer_diam/2.0;

  for (ii=0; ii<max_ring; ii++) {

    if (r <= 0.0) { break; }

    clip.Clear();

    _path.clear();
    for (jj=0; jj<segments; jj++) {
      a = (double)jj * M_PI * 2.0 / (double)segments;

      px = r * cos(a);
      py = r * sin(a);

      _path.push_back( dtoc( cx + px, cy + py ) );
    }
    if (_path.size() > 0) { _path.push_back( _path[0] ); }
    _paths.push_back(_path);

    clip.AddPath( _path, ptSubject, true );

    r -= ring_thick;

    if (r <= 0.0) { break; }

    _path.clear();
    for (jj=0; jj<segments; jj++) {

      a = -(double)jj * M_PI * 2.0 / (double)segments;

      px = r * cos(a);
      py = r * sin(a);

      _path.push_back( dtoc( cx + px, cy + py ) );
    }
    if (_path.size() > 0) { _path.push_back( _path[0] ); }
    _paths.push_back(_path);

    clip.AddPath( _path, ptClip, true );

    _tmp_paths.clear();
    clip.Execute( ctDifference, _tmp_paths, pftNonZero, pftNonZero );
    for (jj=0; jj<_tmp_paths.size(); jj++) {
      _paths.push_back(_tmp_paths[jj]);
    }

    r -= ring_gap;
  }

  exposure.push_back(expose);

  clip.AddPaths( _paths, ptSubject, true );
  clip.Execute( ctUnion, _tmp_paths, pftPositive, pftPositive );

  clip.Clear();
  clip.AddPaths( _tmp_paths, ptSubject, true );

  _path.clear();
  _line_path(_path, cx, cy, hair_width, hair_thick, ang);
  clip.AddPath( _path, ptClip, true );

  _path.clear();
  _line_path(_path, cx, cy, hair_width, hair_thick, ang + (M_PI/2.0));
  clip.AddPath( _path, ptClip, true );

  clip.Execute( ctUnion, paths, pftNonZero, pftNonZero);

  return 0;
}

int add_AM_outline( am_ll_node_t *am_node,
                    std::vector< double > &macro_param,
                    Paths &paths,
                    std::vector< int > &exposure ) {
  int i, j, k, err=0;

  Path path;

  std::vector< double > eval_param;

  int segments = 0, expose = 0;
  double a, px, py, c_a, s_a;
  double cx=0.0, cy=0.0, ang_deg_ccw=0.0, ang=0.0;


  err = _eval_var(eval_param, am_node->eval_line, am_node->n_eval_line, macro_param);
  if (err!=0) { return err; }

  if (eval_param.size() < 9) { return -1; }

  segments = eval_param[1];

  if ((2*(segments+1) + 3) != eval_param.size()) { return -3; }

  if (eval_param[0] > 0.5) { expose = 1; }

  ang_deg_ccw = eval_param[ eval_param.size()-1 ];
  ang = ang_deg_ccw * M_PI / 180.0;

  c_a = cos(ang);
  s_a = sin(ang);

  for (i=2; i<(eval_param.size()-1); i+=2) {
    px = eval_param[i];
    py = eval_param[i+1];
    path.push_back( dtoc( c_a*px - s_a*py, s_a*px + c_a*py ) );
  }

  if (path.size() < 2) { return -4; }
  if ( (path[0].X != path[ path.size()-1 ].X) || (path[0].Y != path[ path.size()-1 ].Y)) { return -5; }

  paths.push_back(path);
  exposure.push_back(expose);

  return 0;
}

// realize an arc
// rotation is applied after (cx,cy) translation
//
static void _thermal_arc_path_ccw( Path &path,
                                   double cx,
                                   double cy,
                                   double r,
                                   double rad_beg,
                                   double rad_end,
                                   int segments ) {
  int i=0;
  double a=0.0, c_a=0.0, s_a=0.0;
  IntPoint prv_pnt, pnt;

  if (segments < 1) { segments = 1; }

  if (rad_beg < 0.0) { rad_beg += 2.0*M_PI; }
  if (rad_end < 0.0) { rad_end += 2.0*M_PI; }

  // Thermal pads are at most pi/2, so fix up angles if they
  // cross the 0 line
  //
  if ((rad_end - rad_beg) > (M_PI/2.0)) { rad_end -= 2.0*M_PI; }
  else if ((rad_beg - rad_end) > (M_PI/2.0)) { rad_beg -= 2.0*M_PI; }

  for (i=0; i <= segments; i++) {
    a = (((double)i) * (rad_end - rad_beg) / (double)segments) + rad_beg;
    c_a = cos(a);
    s_a = sin(a);

    pnt = dtoc( c_a*(cx + r), s_a*(cy + r) );
    if ((i>0) && (prv_pnt.X == pnt.X) && (prv_pnt.Y == pnt.Y)) { continue; }

    path.push_back( pnt );
    prv_pnt.X = pnt.X;
    prv_pnt.Y = pnt.Y;
  }

}

// `am_node` holds information about the macro, including the (text) lines to be interpreted
// `macro_param` are the parameters passed into this aperture macro
// `paths` holds the resulting (potentially multiple) paths as the realization of the macro
// `exposure` holds the array of which paths to be exposed
//
// That is, exposure.size() == paths.size(). For this function, exposure should all be 1.
//
// By convention, all paths are ccw.
//
// returns negative on error, 0 on success.
//
int add_AM_thermal( am_ll_node_t *am_node,
                    std::vector< double > &macro_param,
                    Paths &paths,
                    std::vector< int > &exposure ) {
  int i, j, k, err=0;

  std::vector< double > eval_param;

  Path path;
  int segments = 8, expose = 0;
  double cx=0.0, cy=0.0, ang_deg_ccw=0.0, ang=0.0;
  double gapt=0.0, innd=0.0, outd=0.0, innr=0.0, outr=0.0, gapt2=0.0;
  double inndel=0.0, outdel=0.0;
  double a_beg=0.0, a_end=0.0;

  err = _eval_var(eval_param, am_node->eval_line, am_node->n_eval_line, macro_param);
  if (err!=0) { return err; }

  if (eval_param.size() != 6) { return -1; }
  expose = 1;

  cx = eval_param[0];
  cy = eval_param[1];
  outd = eval_param[2];
  innd = eval_param[3];
  gapt = eval_param[4];
  ang_deg_ccw = eval_param[5];

  // check for unrealizable geometry
  //
  if (gapt > (sqrt(2.0)*outd)) { return -2; }

  ang = ang_deg_ccw * M_PI / 180.0;

  outr = outd/2.0;
  innr = innd/2.0;
  gapt2 = gapt/2.0;

  // inner circle 'dissappears', whatever that means.
  // this is my interpretation
  // `See 4.5.4.9 Thermal, Code 7` in rev 2017.11 of `The Gerber Format Specification`
  //
  if (innr < (sqrt(2.0)*gapt2)) { innr = sqrt(2.0)*gapt2; }

  outdel = ( (gapt2 < outr) ? sqrt( outr*outr - (gapt2*gapt2) ) : 0.0 );
  inndel = ( (gapt2 < innr) ? sqrt( innr*innr - (gapt2*gapt2) ) : 0.0 );

  //--
  // upper left
  //--

  path.clear();

  a_beg = atan2( outdel, -gapt2 );
  a_end = atan2( gapt2, -outdel );
  _thermal_arc_path_ccw(path, cx, cy, outr, a_beg + ang, a_end + ang, segments);

  a_beg = atan2( gapt2, -inndel );
  a_end = atan2( inndel, -gapt2);
  _thermal_arc_path_ccw(path, cx, cy, innr, a_beg + ang, a_end + ang, segments);

  if (path.size() > 0) { path.push_back( path[0] ); }

  paths.push_back(path);
  exposure.push_back(expose);

  //--
  // lower left
  //--

  path.clear();

  a_beg = atan2( -gapt2, -outdel );
  a_end = atan2( -outdel, -gapt2 );
  _thermal_arc_path_ccw(path, cx, cy, outr, a_beg + ang, a_end + ang, segments);

  a_beg = atan2( -inndel, -gapt2 );
  a_end = atan2( -gapt2, -inndel );
  _thermal_arc_path_ccw(path, cx, cy, innr, a_beg + ang, a_end + ang, segments);

  if (path.size() > 0) { path.push_back( path[0] ); }

  paths.push_back(path);
  exposure.push_back(expose);

  //--
  // lower right
  //--

  path.clear();

  a_beg = atan2( -gapt2, outdel );
  a_end = atan2( -outdel, gapt2 );
  _thermal_arc_path_ccw(path, cx, cy, outr, a_beg + ang, a_end + ang, segments);

  a_beg = atan2( -inndel, gapt2 );
  a_end = atan2( -gapt2, inndel );
  _thermal_arc_path_ccw(path, cx, cy, innr, a_beg + ang, a_end + ang, segments);

  if (path.size() > 0) { path.push_back( path[0] ); }

  paths.push_back(path);
  exposure.push_back(expose);

  //--
  // upper right
  //--

  path.clear();

  a_beg = atan2( outdel, gapt2 );
  a_end = atan2( gapt2, outdel );
  _thermal_arc_path_ccw(path, cx, cy, outr, a_beg + ang, a_end + ang, segments);

  a_beg = atan2( gapt2, inndel );
  a_end = atan2( inndel, gapt2 );
  _thermal_arc_path_ccw(path, cx, cy, innr, a_beg + ang, a_end + ang, segments);

  if (path.size() > 0) { path.push_back( path[0] ); }

  paths.push_back(path);
  exposure.push_back(expose);

  return 0;
}



// Realize a macro.
// A macro consists of a series of primiitives to be drawn, either with 'exposure' on or off,
// representing additive or subtractive geometry.
// The additions and subtraction only happen within the macro and aren't affected by or
// affect the outside geometry, so they can be realized wholly with the macro defintion
// and the parameters passed in by the aperture definition.
// Each line in the aperture macro needs to be evaluated as they may have expressions.
// For each expression, the 'atomic' geometry is realized with a flag to indicate whether the
// exposure is on or off.
// Once all atomic gemoetry is realized, the final aperture is realized by going through
// and progressively building up the union and difference of the atomic parts sequencially.
//
int realize_macro( gerber_state_t *gs,
                   Aperture_realization &ap,
                   std::string &macro_name,
                   std::vector< double > &macro_param ) {
  int i, j, err=0, idx, segments, m_idx;
  am_ll_node_t *am_nod;
  std::vector< double > param_val;
  std::vector< std::string > var_name;
  std::string buf;
  int var_idx, ret=0;
  tes_expr *expr;

  Paths result;
  Path empty_path;
  IntPoint pnt;


  Clipper clip;

  static int debug_count=0;

  am_nod = aperture_macro_lookup(gs->am_lib_head, macro_name.c_str());
  if (!am_nod) { return -1; }

  var_idx = 0;

  param_val = macro_param;
  for (i=0; i<(int)param_val.size(); i++) {
    buf = "$";
    buf += std::to_string(i+1);
    var_name.push_back(buf);
  }

  while (am_nod) {

    switch (am_nod->type) {
      case AM_ENUM_NAME:

        break;
      case AM_ENUM_COMMENT:

        break;
      case AM_ENUM_VAR:

        ret = eval_AM_var(am_nod, param_val);

        if (ret<0) { printf("# var error? ret %i\n", ret); }
        break;

      case AM_ENUM_CIRCLE:

        ret = add_AM_circle(am_nod, param_val, ap.m_macro_path, ap.m_macro_exposure);
        if (ret<0) { printf("# circle error? ret %i\n", ret); }

        break;

      case AM_ENUM_VECTOR_LINE:

        ret = add_AM_vector_line(am_nod, param_val, ap.m_macro_path, ap.m_macro_exposure);
        if (ret<0) { printf("# am vector line error? ret %i\n", ret); }

        break;

      case AM_ENUM_CENTER_LINE:

        ret = add_AM_center_line(am_nod, param_val, ap.m_macro_path, ap.m_macro_exposure);
        if (ret<0) { printf("# center line  error? ret %i\n", ret); }

        break;

      case AM_ENUM_OUTLINE:

        ret = add_AM_outline(am_nod, param_val, ap.m_macro_path, ap.m_macro_exposure);
        if (ret<0) { printf("# outline error? ret %i\n", ret); }

        break;

      case AM_ENUM_POLYGON:

        ret = add_AM_polygon(am_nod, param_val, ap.m_macro_path, ap.m_macro_exposure);
        if (ret<0) { printf("# outline error? ret %i\n", ret); }

        break;

      case AM_ENUM_MOIRE:

        ret = add_AM_moire(am_nod, param_val, ap.m_macro_path, ap.m_macro_exposure);
        if (ret<0) { printf("# thermal error? ret %i\n", ret); }

        break;

      case AM_ENUM_THERMAL:

        ret = add_AM_thermal(am_nod, param_val, ap.m_macro_path, ap.m_macro_exposure);
        if (ret<0) { printf("# thermal error? ret %i\n", ret); }

        break;

      default:

        break;
    }

    am_nod = am_nod->next;
  }

  for (i=0; i<ap.m_macro_path.size(); i++) {
    ap.m_path.push_back( ap.m_macro_path[i] );
    //ap.m_exposure.push_back( ap.m_macro_exposure[i] );
    ap.m_exposure.push_back( _expose_bit(  ap.m_macro_exposure[i], gs->polarity ) );
  }

  // clip the aperture macro individual geometry realizations for final use.
  //
  if (ap.m_macro_path.size() > 0) {
    result.push_back(ap.m_macro_path[0]);
  }
  for (i=1; i<ap.m_macro_path.size(); i++) {

    clip.Clear();
    clip.AddPaths(result, ptSubject, true);
    clip.AddPath(ap.m_macro_path[i], ptClip, true);

    result.clear();
    if (ap.m_macro_exposure[i] > 0) {
      clip.Execute( ctUnion, result, pftNonZero, pftNonZero );
    }
    else {
      clip.Execute( ctDifference, result, pftNonZero, pftNonZero );
    }

  }

  return ret;
}

//----

int realize_simple_block( gerber_state_t *gs,
                          aperture_data_t *aperture,
                          Aperture_realization &ap ) {
  join_polygon_set(ap.m_geom, aperture->gs);
  ap.m_exposure.push_back( _expose_bit( 1, gs->polarity ) );
  return 0;
}

//----

int realize_step_repeat( gerber_state_t *gs,
                         aperture_data_t *aperture,
                         Aperture_realization &ap ) {
  int _x, _y, ii, jj;
  double dx=0.0, dy=0.0;
  Paths paths, p;
  Path empty_path;
  IntPoint dpnt, pnt;

  //DEBUG
  fprintf(stdout, "##realize_step_repeat...\n");
  for (ii=0; ii<gApertureName.size(); ii++) {
    fprintf(stdout, "### gAperture %i\n", gApertureName[ii]);
  }

  join_polygon_set(paths, aperture->gs);

  ap.m_geom.clear();
  for (_x=0; _x < aperture->x_rep; _x++) {
    for (_y=0; _y < aperture->y_rep; _y++) {

      dx = ((double)_x) * aperture->i_distance;
      dy = ((double)_y) * aperture->j_distance;
      dpnt = dtoc( dx, dy );

      for (ii=0; ii<paths.size(); ii++) {
        ap.m_geom.push_back(empty_path);
        for (jj=0; jj<paths[ii].size(); jj++) {
          pnt.X = paths[ii][jj].X + dpnt.X;
          pnt.Y = paths[ii][jj].Y + dpnt.Y;
          ap.m_geom[ii].push_back( pnt );
        }
      }

    }

  }

  printf("##sr%i.%i\n", gs->name, aperture->name);
  for (ii=0; ii<ap.m_geom.size(); ii++) {
    for (jj=0; jj<ap.m_geom[ii].size(); jj++) {
      printf("##sr%i.%i %lli %lli\n",
          gs->name, aperture->name,
          (long long int)ap.m_geom[ii][jj].X,
          (long long int)ap.m_geom[ii][jj].Y);
    }
    printf("##sr%i.%i\n", gs->name, aperture->name);
  }

  ap.m_exposure.push_back( _expose_bit(1) );
}

//----

void print_aperture_list(aperture_data_t *ap) {
  printf("\n");
  while (ap) {
    printf("# aperture: %i, type %i, crop_type %i (%f,%f,%f,%f,%f)\n",
        ap->name, ap->type, ap->crop_type,
        (float)ap->crop[0], (float)ap->crop[1],
        (float)ap->crop[2], (float)ap->crop[3],
        (float)ap->crop[4]);
    ap = ap->next;
  }
}

static void _pst(int n) {
  int i;
  for (i=0; i<n; i++) { printf(" "); }
}

void print_aperture_tree(gerber_state_t *gs, int level) {
  aperture_data_t *ap;
  gerber_state_t *par;

  ap = gs->aperture_head;

  printf("# gs:%i ", gs->id);
  _pst(level+1);
  printf("...\n");
  dump_information(gs, 0);
  while (ap) {

    par=NULL;
    if (ap->gs) {
      par = ap->gs->absr_lib_parent_gs;
    }

    printf("# gs:%i ", gs->id);
    _pst(level+1);
    printf("aperture(%i): %i, type %i, crop_type %i (%f,%f,%f,%f,%f) (gs %i, parent %i)\n",
        ap->id,
        ap->name, ap->type, ap->crop_type,
        (float)ap->crop[0], (float)ap->crop[1],
        (float)ap->crop[2], (float)ap->crop[3],
        (float)ap->crop[4],
        (ap->gs ? ap->gs->id : -1), (par ? par->id : -1));

    if (ap->gs) {
      print_aperture_tree(ap->gs, level+1);
    }

    ap = ap->next;

  }

}

//----

aperture_data_t *flatten_aperture_list(gerber_state_t *gs, int level) {
  aperture_data_t *ap_node_prev;
  aperture_data_t *ap_node;
  aperture_data_t *ap_flatten_head;
  aperture_data_t *ap_flatten_last;

  aperture_data_t *_ap;

  if (!gs) { return NULL; }

  ap_node_prev = NULL;

  for (ap_node = gs->aperture_head;
       ap_node ;
       ap_node = ap_node->next) {

    // we've found an aperture block, so recur and get
    // the flattened aperture linked list
    //
    if ((ap_node->type == AD_ENUM_BLOCK) ||
        (ap_node->type == AD_ENUM_STEP_REPEAT)) {

      ap_flatten_head = flatten_aperture_list(ap_node->gs, level+1);

      if (ap_flatten_head) {

        // traverse to the end
        //
        for (ap_flatten_last = ap_flatten_head;
             ap_flatten_last && (ap_flatten_last->next);
             ap_flatten_last = ap_flatten_last->next) ;

        // do some linked list surgery to put the flattened list
        // in line with the current list.
        //
        ap_flatten_last->next = ap_node;
        if (ap_node_prev) { ap_node_prev->next = ap_flatten_head; }
        else              { gs->aperture_head  = ap_flatten_head; }
      }

    }

    ap_node_prev = ap_node;
  }

  return gs->aperture_head;
}

int realize_apertures(gerber_state_t *gs) {
  double min_segment_length = 0.01;
  int min_segments = 8;
  int base_mapping[] = {1, 2, 3, 3};
  int i;
  int ii, jj;

  aperture_data_t *aperture;

  //DEBUG
  printf("###>>>BEFORE (%i)\n", gs->absr_lib_depth);
  print_aperture_list(gs->aperture_head);
  printf("###\n");
  print_aperture_tree(gs, 0);
  printf("###\n");
  dump_information(gs, 0);
  printf("###<<<BEFORE\n");
  
  flatten_aperture_list(gs, 0);

  //DEBUG
  printf("###>>>AFTER (%i)\n", gs->absr_lib_depth);
  print_aperture_list(gs->aperture_head);
  printf("###\n");
  dump_information(gs, 0);
  //print_aperture_tree(gs, 0);
  printf("###<<<AFTER\n");
  

  if (gMinSegmentLength > 0.0) {
    min_segment_length = gMinSegmentLength;
  }

  for ( aperture = gs->aperture_head ;
        aperture ;
        aperture = aperture->next ) {
    Aperture_realization ap;

    ap.m_name = aperture->name;
    ap.m_type = aperture->type;

    if ((aperture->type == AD_ENUM_CIRCLE) ||
        (aperture->type == AD_ENUM_RECTANGLE) ||
        (aperture->type == AD_ENUM_OBROUND) ||
        (aperture->type == AD_ENUM_POLYGON)) {
      ap.m_crop_type = aperture->crop_type;
    }
    else if (aperture->type == AD_ENUM_MACRO) {
      ap.m_macro_name = aperture->macro_name;
      for (i=0; i<aperture->macro_param_count; i++) {
        ap.m_macro_param.push_back(aperture->macro_param[i]);
      }
    }
    else if (aperture->type == AD_ENUM_BLOCK) {
    }
    else {
    }

    //DEBUG
    if ((aperture->type == AD_ENUM_BLOCK) || (aperture->type == AD_ENUM_STEP_REPEAT)) { continue; }

    switch (aperture->type) {

      case AD_ENUM_CIRCLE:
        realize_circle( gs, ap, aperture->crop[0]/2.0, min_segments, min_segment_length );
        break;

      case AD_ENUM_RECTANGLE:
        realize_rectangle( gs, ap, aperture->crop[0]/2.0, aperture->crop[1]/2.0 );
        break;

      case AD_ENUM_OBROUND:
        realize_obround( gs, ap, aperture->crop[0], aperture->crop[1], min_segments, min_segment_length  );
        break;

      case AD_ENUM_POLYGON:
        realize_polygon( gs, ap, aperture->crop[0], aperture->crop[1], aperture->crop[2] );
        break;

      case AD_ENUM_MACRO:
        realize_macro( gs, ap, ap.m_macro_name, ap.m_macro_param );
        break;

      case AD_ENUM_BLOCK:
        realize_simple_block( gs, aperture, ap );
        break;

      case AD_ENUM_STEP_REPEAT:

        /*
        printf("##SR realize aperture %i\n", ap.m_name);

        printf("#####################################\n");
        printf("#####################################\n");
        printf("#####################################\n");

        dump_information(gs, 0);

        printf("#####################################\n");
        printf("#####################################\n");
        printf("#####################################\n");

        realize_step_repeat( gs, aperture, ap );

        for (ii=0; ii<ap.m_geom.size(); ii++) {
          for (jj=0; jj<ap.m_geom[ii].size(); jj++) {
            printf("##sr%i %lli %lli\n", ap.m_name, (long long int)ap.m_geom[ii][jj].X, (long long int)ap.m_geom[ii][jj].Y);
          }
          printf("##sr%i\n", ap.m_name);
        }
        */

        break;

      default: break;
    }

    if ((aperture->type == AD_ENUM_CIRCLE) ||
        (aperture->type == AD_ENUM_RECTANGLE) ||
        (aperture->type == AD_ENUM_OBROUND) ||
        (aperture->type == AD_ENUM_POLYGON)) {

      int base = base_mapping[ aperture->type ];

      switch (aperture->crop_type) {

        // solid, do nothing
        //
        case 0:

          break;

        // circle hole
        //
        case 1:

          realize_hole( gs, ap, aperture->crop[base]/2.0, min_segments, min_segment_length );
          break;

        // rect hole
        //
        case 2:

          //realize_rectangle_hole( ap, aperture->crop[base]/2.0, aperture->crop[base+1]/2.0 );
          break;

        default: break;
      }

    }

    //DEBUG
    fprintf(stdout, "### adding aperture %i\n", ap.m_name);
    if (ap.m_name < 0) { printf("## SKIPPING APERTURE %i\n", ap.m_name); continue; }

    gAperture.insert( ApertureNameMapPair(ap.m_name, ap) );
    gApertureName.push_back(ap.m_name);

  }

  return 0;
}
