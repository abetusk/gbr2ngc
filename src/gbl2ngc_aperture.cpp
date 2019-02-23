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

#define DEBUG_APERTURE

#define _isnan std::isnan

static int _get_segment_count(double r, double min_segment_length, int min_segments) {
  int segments;
  double c, theta, z;

  segments=min_segments;
  c = 2.0 * M_PI * r;

  theta = 2.0 * asin( min_segment_length / (2.0*r) );
  if (_isnan(theta)) { return min_segments; }
  if (_isnan(c/theta)) { return min_segments; }
  segments = (int)(c / theta);
  if (segments < min_segments) {
    segments = min_segments;
  }

  return segments;
}


void realize_circle(Aperture_realization &ap, double r, int min_segments = 8, double min_segment_length = 0.01 ) {
  int i, idx, segments;
  double a;
  //double c, theta, a;
  //c = 2.0 * M_PI * r;
  Path empty_path;

  segments = _get_segment_count(r, min_segment_length, min_segments);

#ifdef DEBUG_APERTURE
  printf("# realize_circle: r %f seg %i\n", (float)r, segments);
#endif

  //theta = 2.0 * asin( min_segment_length / (2.0*r) );
  //segments = (int)(c / theta);
  //if (segments < min_segments) {
  //  segments = min_segments;
  //}

  idx = (int)ap.m_path.size();
  ap.m_path.push_back(empty_path);
  for (i=0; i<segments; i++) {
    a = 2.0 * M_PI * (double)i / (double)segments ;
    //ap.m_outer_boundary.push_back( dtoc( r*cos(a), r*sin(a) ) );
    ap.m_path[idx].push_back( dtoc( r*cos(a), r*sin(a) ) );
  }
  if (ap.m_path[idx].size() > 0) {
    ap.m_path[idx].push_back( ap.m_path[idx][0] );
  }
  ap.m_exposure.push_back(1);

#ifdef DEBUG_APERTURE
  for (i=0; i<ap.m_path[idx].size(); i++) {
    printf("# %lli %lli\n", (long long int)ap.m_path[idx][i].X, (long long int)ap.m_path[idx][i].Y);
  }
#endif
}

void realize_rectangle( Aperture_realization &ap, double x, double y ) {
  int idx;
  Path empty_path;

  idx = (int)ap.m_path.size();
  ap.m_path.push_back(empty_path);

  // ap.m_outer_boundary.push_back( dtoc( -x, -y ) );
  // ap.m_outer_boundary.push_back( dtoc(  x, -y ) );
  // ap.m_outer_boundary.push_back( dtoc(  x,  y ) );
  // ap.m_outer_boundary.push_back( dtoc( -x,  y ) );

  ap.m_path[idx].push_back( dtoc( -x, -y ) );
  ap.m_path[idx].push_back( dtoc(  x, -y ) );
  ap.m_path[idx].push_back( dtoc(  x,  y ) );
  ap.m_path[idx].push_back( dtoc( -x,  y ) );
  ap.m_exposure.push_back(1);
}


void realize_obround( Aperture_realization &ap, double x_len, double y_len, int min_segments = 8, double min_segment_length = 0.01 ) {
  int i, segments, idx;
  double r, a;
  //double theta, a, r;
  //double c;
  Path empty_path;

  r = ( (fabs(x_len) < fabs(y_len)) ? x_len : y_len );

//  c = 2.0 * M_PI * r;
//  theta = 2.0 * asin( min_segment_length / (2.0*r) );
//  segments = (int)(c / theta);
//  if (segments < min_segments)
//    segments = min_segments;

  segments = _get_segment_count(r, min_segment_length, min_segments);

  idx = (int)ap.m_path.size();
  ap.m_path.push_back(empty_path);
  if (x_len < y_len) {

    r = x_len / 2.0;

    // start at the top right
    //
    for (i=0; i <= (segments/2); i++) {
      a = 2.0 * M_PI * (double)i / (double)segments ;
      //ap.m_outer_boundary.push_back( dtoc( r*cos(a), r*sin(a) + ((y_len/2.0) - r) ) );
      ap.m_path[idx].push_back( dtoc( r*cos(a), r*sin(a) + ((y_len/2.0) - r) ) );
    }

    // then the bottom
    //
    for (int i = (segments/2); i <= segments; i++) {
      a = 2.0 * M_PI * (double)i / (double)segments ;
      //ap.m_outer_boundary.push_back( dtoc( r*cos(a), r*sin(a) - ((y_len/2.0) - r) ) );
      ap.m_path[idx].push_back( dtoc( r*cos(a), r*sin(a) - ((y_len/2.0) - r) ) );
    }

  }
  else if (y_len < x_len) {

    r = y_len / 2.0;

    // start at bottom right
    //
    for (i=0; i <= (segments/2); i++) {
      a = ( 2.0 * M_PI * (double)i / (double)segments ) - ( M_PI / 2.0 );
      //ap.m_outer_boundary.push_back( dtoc( r*cos(a) + ((x_len/2.0) - r) , r*sin(a) ) );
      ap.m_path[idx].push_back( dtoc( r*cos(a) + ((x_len/2.0) - r) , r*sin(a) ) );
    }

    // then the left
    //
    for (i=(segments/2); i <= segments; i++) {
      a = ( 2.0 * M_PI * (double)i / (double)segments ) - ( M_PI / 2.0 );
      //ap.m_outer_boundary.push_back( dtoc( r*cos(a) - ((x_len/2.0) - r) , r*sin(a) ) );
      ap.m_path[idx].push_back( dtoc( r*cos(a) - ((x_len/2.0) - r) , r*sin(a) ) );
    }

  }

  // circle
  //
  else {

    r = x_len / 2.0;
    for (i=0; i<segments; i++) {
      a = 2.0 * M_PI * (double)i / (double)segments ;
      //ap.m_outer_boundary.push_back( dtoc( r*cos( a ), r*sin( a ) ) );
      ap.m_path[idx].push_back( dtoc( r*cos( a ), r*sin( a ) ) );
    }
  }

  ap.m_exposure.push_back(1);
}


void realize_polygon( Aperture_realization &ap, double r, int n_vert, double rot_deg) {
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
    //ap.m_outer_boundary.push_back( dtoc( r*cos( a ), r*sin( a ) ) );
    ap.m_path[idx].push_back( dtoc( r*cos( a ), r*sin( a ) ) );
  }
  ap.m_exposure.push_back(1);

}


void realize_hole( Aperture_realization &ap, double r, int min_segments = 8, double min_segment_length = 0.01 ) {
  int i, idx, segments;
  double a;
  //double a, c, theta;
  Path empty_path;

//  c = 2.0 * M_PI * r;
//  theta = 2.0 * asin( min_segment_length / (2.0*r) );
//  segments = (int)(c / theta);
//  if (segments < min_segments)
//    segments = min_segments;

  segments = _get_segment_count(r, min_segment_length, min_segments);

  idx = (int)ap.m_path.size();
  ap.m_path.push_back(empty_path);
  for (i=0; i<segments; i++) {

    // counter clockwise
    //
    a = -2.0 * M_PI * (double)i / (double)segments;
    //ap.m_outer_boundary.push_back( dtoc( r*cos( a ), r*sin( a ) ) );
    ap.m_path[idx].push_back( dtoc( r*cos( a ), r*sin( a ) ) );

  }
  ap.m_exposure.push_back(0);

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
// macro_param holds global variables (the hack to allow passed in varaibles to be seen by the macro 'function' call)
// eval_param are the resuling variables
//
// return 0 on success, non-zero on error
//
int _eval_var(std::vector< double > &eval_param, char **eval_line, int n_eval_line, std::vector< double > &macro_param) {
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


int eval_AM_var(am_ll_node_t *am_node, std::vector< double > &macro_param) {
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

  //DEBUG
#ifdef DEBUG_APERTURE
  printf("## got var_idx %i\n", var_idx);
  printf("## var eval: %s\n", am_node->eval_line[0]);
  fflush(stdout);
#endif

  expr = tes_compile(am_node->eval_line[0], vars, macro_param.size(), &err);
  if (!expr) { free(vars); return -2; }

  val = tes_eval(expr);

  //DEBUG
#ifdef DEBUG_APERTURE
  printf("## got val %f\n", val); fflush(stdout);
#endif

  if ((var_idx+1) > (int)macro_param.size()) {
    for (i=(int)macro_param.size(); i<=var_idx; i++) {
      macro_param.push_back(0.0);
    }
  }

  macro_param[var_idx] = val;

  //DEBUG
#ifdef DEBUG_APERTURE
  printf("# macro_param[%i]:", (int)macro_param.size()); fflush(stdout);
  for (i=0; i<macro_param.size(); i++) {
    printf("## %f", macro_param[i]);
  }
  printf("\n");
#endif

  tes_free(expr);
  free(vars);
  return 0;
}

int add_AM_circle(am_ll_node_t *am_node, std::vector< double > &macro_param, Paths &paths, std::vector< int > &exposure) {
  int i, j, k, err=0;

  std::vector< double > eval_param;

  Path path;
  int segments = 32;
  double a;
  int expose = 0;
  double px, py, c_a, s_a;
  double r=0.0, cx=0.0, cy=0.0, ang=0.0, ang_deg_ccw=0.0;

  double *dptr=NULL;

#ifdef DEBUG_APERTURE
  printf("# AM_circle\n");
  printf("# macro_param[%i]:", (int)macro_param.size());
  for (i=0; i<macro_param.size(); i++) {
    printf("## %f", macro_param[i]);
  }
  printf("##\n");
#endif

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


#ifdef DEBUG_APERTURE
  printf("## am_circle: ");
  for (i=0; i<eval_param.size(); i++) { printf(" %f", eval_param[i]); }
  printf("\n");
  printf("## %i\n", expose);
  for (i=0; i<path.size(); i++) {
    printf("## %f %f\n", ctod(path[i].X), ctod(path[i].Y));
  }
  printf("\n");

  printf("# eval_param[%i]:", (int)(eval_param.size()));
  for (i=0; i<eval_param.size(); i++) {
    printf("## %f", eval_param[i]);
  }
  printf("\n");
#endif

  return 0;
}

int add_AM_vector_line(am_ll_node_t *am_node, std::vector< double > &macro_param, Paths &paths, std::vector< int > &exposure) {
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
  //dwl = sqrt( (ex-sx)*(ex-sx) + (ey-sy)*(ey-sy) );
  //if (dwl < eps) { dwl = 1.0; }

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

  //DEBUG
#ifdef DEBUG_APERTURE
  printf("## am_vector_line:\n");
  for (i=0; i<eval_param.size(); i++) {
    printf("## [%i] %f\n", i, eval_param[i]);
  }
  printf("##\n");
  printf("## %i\n", expose);
  for (i=0; i<path.size(); i++) {
    printf("## %f %f\n", ctod(path[i].X), ctod(path[i].Y));
  }
  printf("##\n");
#endif

  return 0;
}

int add_AM_center_line(am_ll_node_t *am_node, std::vector< double > &macro_param, Paths &paths, std::vector< int > &exposure) {
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

  //DEBUG
#ifdef DEBUG_APERTURE
  printf("## am_center_line:\n");
  for (i=0; i<eval_param.size(); i++) {
    printf("## [%i] %f\n", i, eval_param[i]);
  }
  printf("##\n");
  printf("## %i\n", expose);
  for (i=0; i<path.size(); i++) {
    printf("## %f %f\n", ctod(path[i].X), ctod(path[i].Y));
  }
  printf("##\n");
#endif

  return 0;
}

int add_AM_polygon(am_ll_node_t *am_node, std::vector< double > &macro_param, Paths &paths, std::vector< int > &exposure) {
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

  //DEBUG
#ifdef DEBUG_APERTURE
  printf("## am_polygon:\n");
  for (i=0; i<eval_param.size(); i++) {
    printf("## [%i] %f\n", i, eval_param[i]);
  }
  printf("##\n");
  printf("## %i\n", expose);
  for (i=0; i<path.size(); i++) {
    printf("## %f %f\n", ctod(path[i].X), ctod(path[i].Y));
  }
  printf("##\n");
#endif

  return 0;
}


void _line_path( Path &path, double cx, double cy, double width, double height, double ang) {
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

int add_AM_moire(am_ll_node_t *am_node, std::vector< double > &macro_param, Paths &paths, std::vector< int > &exposure) {
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

  //DEBUG
#ifdef DEBUG_APERTURE
  printf("## am_moire:\n");
  for (i=0; i<eval_param.size(); i++) {
    printf("## [%i] %f\n", i, eval_param[i]);
  }
  printf("##\n");
  printf("## expose %i\n", expose);
  for (ii=0; ii<paths.size(); ii++) {
    for (jj=0; jj<paths.size(); jj++) {
      printf("##moire %f %f\n", ctod(paths[ii][jj].X), ctod(paths[ii][jj].Y));
    }
    printf("##moire\n");
  }
  printf("##\n");
#endif

  return 0;
}

int add_AM_outline(am_ll_node_t *am_node, std::vector< double > &macro_param, Paths &paths, std::vector< int > &exposure) {
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

  //DEBUG
#ifdef DEBUG_APERTURE
  printf("## am_outline:\n");
  for (i=0; i<eval_param.size(); i++) {
    printf("## [%i] %f\n", i, eval_param[i]);
  }
  printf("##\n");
  printf("## %i\n", expose);
  for (i=0; i<path.size(); i++) {
    printf("## %f %f\n", ctod(path[i].X), ctod(path[i].Y));
  }
  printf("##\n");
#endif

  return 0;
}

// realize an arc
// rotation is applied after (cx,cy) translation
//
static void _thermal_arc_path_ccw(Path &path, double cx, double cy, double r, double rad_beg, double rad_end, int segments) {
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
int add_AM_thermal(am_ll_node_t *am_node, std::vector< double > &macro_param, Paths &paths, std::vector< int > &exposure) {
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
int realize_macro(gerber_state_t *gs, Aperture_realization &ap, std::string &macro_name, std::vector< double > &macro_param) {

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

  //DEBUG
#ifdef DEBUG_APERTURE
  printf("# realizing macro\n"); fflush(stdout);
#endif

  am_nod = aperture_macro_lookup(gs->am_lib_head, macro_name.c_str());
  if (!am_nod) { return -1; }

  var_idx = 0;

  //DEBUG
#ifdef DEBUG_APERTURE
  printf("# found %s\n", am_nod->name); fflush(stdout);
#endif

  param_val = macro_param;
  for (i=0; i<(int)param_val.size(); i++) {
    buf = "$";
    buf += std::to_string(i+1);
    var_name.push_back(buf);
  }

#ifdef DEBUG_APERTURE
  for (i=0; i<(int)var_name.size(); i++) {
    printf("# %s %f\n", var_name[i].c_str(), param_val[i]); fflush(stdout);
  }
#endif

  while (am_nod) {

    switch (am_nod->type) {
      case AM_ENUM_NAME:

#ifdef DEBUG_APERTURE
        printf("#xx name: %s\n", am_nod->name);
#endif

        break;
      case AM_ENUM_COMMENT:

#ifdef DEBUG_APERTURE
        printf("#xx comment %s\n", am_nod->comment);
#endif

        break;
      case AM_ENUM_VAR:

#ifdef DEBUG_APERTURE
        printf("#xx var %s = %s\n", am_nod->varname, am_nod->eval_line[0]); fflush(stdout);
#endif
        ret = eval_AM_var(am_nod, param_val);

        if (ret<0) { printf("# var error? ret %i\n", ret); }
        break;

      case AM_ENUM_CIRCLE:

#ifdef DEBUG_APERTURE
        printf("#xx circle\n");
#endif

        ret = add_AM_circle(am_nod, param_val, ap.m_macro_path, ap.m_macro_exposure);
        if (ret<0) { printf("# circle error? ret %i\n", ret); }

        // trying to add to add from am
        //
        //for (i=0; i<ap.m_macro_path.size(); i++) {
        //  ap.m_path.push_back(ap.m_macro_path[i]);
        //  ap.m_exposure.push_back( ap.m_macro_exposure[i] );
        //}

        break;

      case AM_ENUM_VECTOR_LINE:

#ifdef DEBUG_APERTURE
        printf("# vector line\n");
#endif

        ret = add_AM_vector_line(am_nod, param_val, ap.m_macro_path, ap.m_macro_exposure);
        if (ret<0) { printf("# am vector line error? ret %i\n", ret); }

        break;

      case AM_ENUM_CENTER_LINE:

#ifdef DEBUG_APERTURE
        printf("#xx center line\n");
#endif

        ret = add_AM_center_line(am_nod, param_val, ap.m_macro_path, ap.m_macro_exposure);
        if (ret<0) { printf("# center line  error? ret %i\n", ret); }

        // trying to add to add from am
        //
        //for (i=0; i<ap.m_macro_path.size(); i++) {
        //  ap.m_path.push_back(ap.m_macro_path[i]);
        //  ap.m_exposure.push_back( ap.m_macro_exposure[i] );
        //}


        break;

      case AM_ENUM_OUTLINE:

#ifdef DEBUG_APERTURE
        printf("#xx outline\n");
#endif

        ret = add_AM_outline(am_nod, param_val, ap.m_macro_path, ap.m_macro_exposure);
        if (ret<0) { printf("# outline error? ret %i\n", ret); }

        // trying to add to add from am
        //
        //for (i=0; i<ap.m_macro_path.size(); i++) {
        //  ap.m_path.push_back(ap.m_macro_path[i]);
        //  ap.m_exposure.push_back( ap.m_macro_exposure[i] );
        //}


        break;

      case AM_ENUM_POLYGON:

#ifdef DEBUG_APERTURE
        printf("#xx polygon\n");
#endif

        //printf("#xx polygon\n");

        ret = add_AM_polygon(am_nod, param_val, ap.m_macro_path, ap.m_macro_exposure);
        if (ret<0) { printf("# outline error? ret %i\n", ret); }

        break;

      case AM_ENUM_MOIRE:

#ifdef DEBUG_APERTURE
        printf("#xx moire\n");
#endif

        ret = add_AM_moire(am_nod, param_val, ap.m_macro_path, ap.m_macro_exposure);
        if (ret<0) { printf("# thermal error? ret %i\n", ret); }

        break;

      case AM_ENUM_THERMAL:

#ifdef DEBUG_APERTURE
        printf("#xx thermal\n");
#endif

        ret = add_AM_thermal(am_nod, param_val, ap.m_macro_path, ap.m_macro_exposure);
        if (ret<0) { printf("# thermal error? ret %i\n", ret); }

        break;

      default:

#ifdef DEBUG_APERTURE
        printf("#xx unknown\n");
#endif


        break;
    }

    am_nod = am_nod->next;
  }

  for (i=0; i<ap.m_macro_path.size(); i++) {

#ifdef DEBUG_APERTURE
    printf("## adding to m_path (%i)\n", i);
#endif

    ap.m_path.push_back( ap.m_macro_path[i]);
    ap.m_exposure.push_back( ap.m_macro_exposure[i] );
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

  //DEBUG
#ifdef DEBUG_APERTURE
  printf("\n"); fflush(stdout);

  for (i=0; i<ap.m_macro_path.size(); i++) {
    printf("##d%03i ##e %i\n", debug_count, ap.m_macro_exposure[i]);
    for (j=0; j<ap.m_macro_path[i].size(); j++) {
      printf("##d%03i %lli %lli\n", debug_count, (long long int)ap.m_macro_path[i][j].X, (long long int)ap.m_macro_path[i][j].Y);
    }
    printf("##d%03i\n##d%03i\n", debug_count, debug_count);
  }

  for (i=0; i<result.size(); i++) {
    printf("###d%03i\n", debug_count);
    for (j=0; j<result[i].size(); j++) {
      printf("###d%03i %lli %lli\n", debug_count, (long long int)result[i][j].X, (long long int)result[i][j].Y);
    }
    printf("###d%03i\n", debug_count);
  }
  debug_count++;
#endif

  // not ready for this yet...we're adding aperture macro stuff above,
  // need ot figure out how to actually get it into an aperture data definition/realization
  //
  //gAperture.insert( ApertureNameMapPair(ap.m_name, ap) );
  //gApertureName.push_back(ap.m_name);

  return ret;
}

//----

int realize_apertures(gerber_state_t *gs) {
  double min_segment_length = 0.01;
  int min_segments = 8;
  int base_mapping[] = {1, 2, 3, 3};
  int i;

  aperture_data_t *aperture;

  for ( aperture = gs->aperture_head ;
        aperture ;
        aperture = aperture->next )
  {
    Aperture_realization ap;

    //DEBUG
#ifdef DEBUG_APERTURE
    printf("##>> aperture->type %i, aperture->name %i\n", aperture->type, aperture->name);
#endif

    if (aperture->type != 4) {
      ap.m_name = aperture->name;
      ap.m_type = aperture->type;
      ap.m_crop_type = aperture->crop_type;
    } else {
      ap.m_name = aperture->name;
      ap.m_type = aperture->type;
      ap.m_macro_name = aperture->macro_name;
      for (i=0; i<aperture->macro_param_count; i++) {
        ap.m_macro_param.push_back(aperture->macro_param[i]);
      }
    }

    switch (aperture->type) {

      // circle
      //
      case 0:
        realize_circle( ap, aperture->crop[0]/2.0, min_segments, min_segment_length );
        break;

      // rectangle
      //
      case 1:
        realize_rectangle( ap, aperture->crop[0]/2.0, aperture->crop[1]/2.0 );
        break;

      // obround
      //
      case 2:
        realize_obround( ap, aperture->crop[0], aperture->crop[1], min_segments, min_segment_length  );
        break;

      // polygon
      //
      case 3:
        realize_polygon( ap, aperture->crop[0], aperture->crop[1], aperture->crop[2] );
        break;

      // macro
      //
      case 4:
        realize_macro( gs, ap, ap.m_macro_name, ap.m_macro_param );
        break;

      default: break;
    }

    int base = ( (aperture->type < 4) ? base_mapping[ aperture->type ] : 0 );

    if (aperture->type != 4) {

      switch (aperture->crop_type)
      {
        // solid, do nothing
        //
        case 0: break;

        // circle hole
        //
        case 1:

#ifdef DEBUG_APERTURE
          printf("## circle hole?\n");
#endif

          realize_hole( ap, aperture->crop[base]/2.0, min_segments, min_segment_length );

          break;

        // rect hole
        //
        case 2:

#ifdef DEBUG_APERTURE
          printf("## rect?\n");
#endif

          //realize_rectangle_hole( ap, aperture->crop[base]/2.0, aperture->crop[base+1]/2.0 );
          break;

        default: break;
      }

    }

    //DEBUG
#ifdef DEBUG_APERTURE
    printf("##?? adding aperture %i\n", ap.m_name); fflush(stdout);
#endif

    gAperture.insert( ApertureNameMapPair(ap.m_name, ap) );
    gApertureName.push_back(ap.m_name);

  }

  return 0;
}
