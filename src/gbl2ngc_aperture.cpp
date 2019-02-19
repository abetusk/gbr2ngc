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


void realize_circle(Aperture_realization &ap, double r, int min_segments = 8, double min_segment_length = 0.01 )
{
  int i, idx, segments;
  double a;
  //double c, theta, a;
  //c = 2.0 * M_PI * r;
  Path empty_path;

  segments = _get_segment_count(r, min_segment_length, min_segments);

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
  ap.m_exposure.push_back(1);

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


// WIP
//
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

int eval_AM_var(am_ll_node_t *am_node, std::vector< double > &macro_param) {
  int i, j, k, err=0;
  tes_expr *expr=NULL;
  tes_variable *vars=NULL;
  std::vector< std::string > varname;
  std::string s;

  double val;
  std::vector< double > eval_param;
  Path path;

  int segments = 8;
  double a;
  int expose = 0;
  int var_idx;
  double px, py, c_a, s_a;
  double r=0.0, cx=0.0, cy=0.0, ang=0.0;

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
  printf("## got var_idx %i\n", var_idx);
  printf("## var eval: %s\n", am_node->eval_line[0]);
  fflush(stdout);

  expr = tes_compile(am_node->eval_line[0], vars, macro_param.size(), &err);
  if (!expr) { free(vars); return -2; }

  val = tes_eval(expr);

  //DEBUG
  printf("## got val %f\n", val); fflush(stdout);

  if ((var_idx+1) > (int)macro_param.size()) {
    for (i=(int)macro_param.size(); i<=var_idx; i++) {
      macro_param.push_back(0.0);
    }
  }

  macro_param[var_idx] = val;

  //DEBUG
  printf("# macro_param[%i]:", (int)macro_param.size()); fflush(stdout);
  for (i=0; i<macro_param.size(); i++) {
    printf(" %f", macro_param[i]);
  }
  printf("\n");

  tes_free(expr);
  free(vars);
  return 0;
}

int add_AM_circle(am_ll_node_t *am_node, std::vector< double > &macro_param, Paths &paths, std::vector< int > &exposure) {
  int i, j, k, err=0;
  tes_expr *expr=NULL;
  tes_variable *vars=NULL;
  std::vector< std::string > varname;
  std::string s;

  double val;
  std::vector< double > eval_param;
  Path path;

  int segments = 8;
  double a;
  int expose = 0;
  double px, py, c_a, s_a;
  double r=0.0, cx=0.0, cy=0.0, ang=0.0;

  double *dptr=NULL;

  //DEBUG
  printf("# AM_circle\n");
  printf("# macro_param[%i]:", (int)macro_param.size());
  for (i=0; i<macro_param.size(); i++) {
    printf(" %f", macro_param[i]);
  }
  printf("\n");

  vars = (tes_variable *)malloc(sizeof(tes_variable)*macro_param.size());
  memset(vars, 0, sizeof(tes_variable)*macro_param.size());
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

  //DEBUG
  for (i=0; i<(int)macro_param.size(); i++) {
    dptr = (double *)vars[i].address;
    printf("# var %s %f\n", vars[i].name, *dptr);
  }


  for (i=0; i<am_node->n_eval_line; i++) {

    //DEBUG
    printf("#>> %s\n", am_node->eval_line[i]);

    expr = tes_compile(am_node->eval_line[i], vars, macro_param.size(), &err);
    if (!expr) { free(vars); return -1; }

    val = tes_eval(expr);

    eval_param.push_back(val);
    tes_free(expr);
  }

  if (eval_param.size() < 2) { free(vars); return -1; }

  if (eval_param[0] > 0.5) { expose = 1; }
  r = eval_param[1]/2;
  if (r <= 0.0) { free(vars); return -2; }
  if (eval_param.size() >= 3) { cx = eval_param[2]; }
  if (eval_param.size() >= 4) { cy = eval_param[3]; }
  if (eval_param.size() >= 5) { ang = eval_param[3]; }

  c_a = cos(ang);
  s_a = sin(ang);

  for (i=0; i<segments; i++) {
    //path.push
    a = -2.0 * M_PI * (double)i / (double)segments;

    px = r*cos(a) + cx;
    py = r*sin(a) + cy;

    path.push_back( dtoc( c_a*px + s_a*py, s_a*px - c_a*py ) );
  }

  paths.push_back(path);
  exposure.push_back(expose);

  //DBEUG
  printf("## am_circle: ");
  for (i=0; i<eval_param.size(); i++) { printf(" %f", eval_param[i]); }
  printf("\n");
  printf("## %i\n", expose);
  for (i=0; i<path.size(); i++) {
    printf("%f %f\n", ctod(path[i].X), ctod(path[i].Y));
  }
  printf("\n");


  //DBEUG
  printf("# eval_param[%i]:", (int)(eval_param.size()));
  for (i=0; i<eval_param.size(); i++) {
    printf(" %f", eval_param[i]);
  }
  printf("\n");


  free(vars);
  return 0;
}

int add_AM_center_line(am_ll_node_t *am_node, std::vector< double > &macro_param, Paths &paths, std::vector< int > &exposure) {
  int i, j, k, err=0;
  tes_expr *expr=NULL;
  tes_variable *vars=NULL;
  std::vector< std::string > varname;
  std::string s;

  Path path;

  double val;
  std::vector< double > eval_param;

  int segments = 8, expose = 0;
  double a, px, py, c_a, s_a;
  double width=0.0, height=0.0, cx=0.0, cy=0.0, ang=0.0;


  if (macro_param.size() > 0) {
    vars = (tes_variable *)malloc(sizeof(tes_variable)*macro_param.size());
    for (i=0; i<macro_param.size(); i++) {
      s.clear();
      s += "$";
      s += std::to_string(i+1);
      varname.push_back(s);
      //vars[i].name = varname[i].c_str();
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


  for (i=0; i<am_node->n_eval_line; i++) {
    expr = tes_compile(am_node->eval_line[i], vars, macro_param.size(), &err);
    if (!expr) { free(vars); return -1; }
    val = tes_eval(expr);
    eval_param.push_back(val);
    tes_free(expr);
  }

  if (eval_param.size() < 3) { free(vars); return -1; }

  if (eval_param[0] > 0.5) { expose = 1; }
  width  = eval_param[1];
  height = eval_param[2];
  if (eval_param.size() >= 4) { cx = eval_param[3]; }
  if (eval_param.size() >= 5) { cy = eval_param[4]; }
  if (eval_param.size() >= 6) { ang = eval_param[5]; }

  c_a = cos(ang);
  s_a = sin(ang);

  px = (+width/2) + cx;  py = (+height/2) + cy;
  path.push_back( dtoc( c_a*px + s_a*py, s_a*px - c_a*py ) );

  px = (-width/2) + cx;  py = (+height/2) + cy;
  path.push_back( dtoc( c_a*px + s_a*py, s_a*px - c_a*py ) );

  px = (-width/2) + cx;  py = (-height/2) + cy;
  path.push_back( dtoc( c_a*px + s_a*py, s_a*px - c_a*py ) );

  px = (+width/2) + cx;  py = (-height/2) + cy;
  path.push_back( dtoc( c_a*px + s_a*py, s_a*px - c_a*py ) );

  paths.push_back(path);
  exposure.push_back(expose);

  //DBEUG
  printf("## am_center_line:\n");
  for (i=0; i<eval_param.size(); i++) { printf("[%i] %f\n", i, eval_param[i]); }
  printf("\n");
  printf("## %i\n", expose);
  for (i=0; i<path.size(); i++) {
    printf("%f %f\n", ctod(path[i].X), ctod(path[i].Y));
  }
  printf("\n");

  free(vars);
  return 0;
}

// Realize a macro.
// A macro consists of a series of primiitives to be drawn, either with 'exposure' on or off,
// representing additive or subtractive geometry.
// The additions and subtraction only happen within the macro and aren't affected by or
// affect the outside geometry, so they can be realized wholly with the macro defintion
// and the parameters passed in by the aperture definition.
// Each line in the aperture macro needs to be evaluated as they may have expressions.
// Each line has to be evaluated in order as some variables.
// For each expression, the 'atomic' geometry is realized with a flag to indicate whether the
// exposure is on or off.
// Once all atomic gemoetry is realized, the final aperture is realized by going through
// and progressively building up the union and difference of the atomic parts sequencially.
// Since this is done progressively, the final geometry has to potential to grow exponentially.
//
int realize_macro(gerber_state_t *gs, Aperture_realization &ap, std::string &macro_name, std::vector< double > &macro_param) {

  int i, err=0, idx, segments, m_idx;
  am_ll_node_t *am_nod;
  std::vector< double > param_val;
  std::vector< std::string > var_name;
  std::string buf;
  int var_idx, ret=0;
  tes_expr *expr;

  Path empty_path;
  IntPoint pnt;

  //std::vector< int > exposure;
  //Paths atomic_geom;


  //DEBUG
  printf("# realizing macro\n"); fflush(stdout);

  am_nod = aperture_macro_lookup(gs->am_lib_head, macro_name.c_str());
  if (!am_nod) { return -1; }

  var_idx = 0;

  //DEBUG
  printf("# found %s\n", am_nod->name); fflush(stdout);

  param_val = macro_param;
  for (i=0; i<(int)param_val.size(); i++) {
    buf = "$";
    buf += std::to_string(i+1);
    var_name.push_back(buf);
  }

  for (i=0; i<(int)var_name.size(); i++) {
    printf("# %s %f\n", var_name[i].c_str(), param_val[i]); fflush(stdout);
  }

  while (am_nod) {

    switch (am_nod->type) {
      case AM_ENUM_NAME: printf("#xx name: %s\n", am_nod->name); break;
      case AM_ENUM_COMMENT: printf("#xx comment %s\n", am_nod->comment); break;
      case AM_ENUM_VAR:

        printf("#xx var %s = %s\n", am_nod->varname, am_nod->eval_line[0]); fflush(stdout);

        ret = eval_AM_var(am_nod, param_val);

        if (ret<0) { printf("# var error? ret %i\n", ret); }
        break;

      case AM_ENUM_CIRCLE:
        printf("#xx circle\n");
        ret = add_AM_circle(am_nod, param_val, ap.m_macro_path, ap.m_macro_exposure);
        if (ret<0) { printf("# circle error? ret %i\n", ret); }


        // trying to add to add from am
        //
        m_idx = (int)(ap.m_macro_path.size()-1);
        idx = (int)ap.m_path.size();
        ap.m_path.push_back(empty_path);
        for (i=0; i<ap.m_macro_path.size(); i++) {
          ap.m_path[idx].push_back( ap.m_macro_path[m_idx][i] );
        }
        ap.m_exposure.push_back( ap.m_macro_exposure[m_idx] );

        break;

      case AM_ENUM_VECTOR_LINE: printf("# vector line\n"); break;

      case AM_ENUM_CENTER_LINE:

        printf("#xx center line\n");

        ret = add_AM_center_line(am_nod, param_val, ap.m_macro_path, ap.m_macro_exposure);

        if (ret<0) { printf("# center line  error? ret %i\n", ret); }

        // trying to add to add from am
        //
        m_idx = (int)(ap.m_macro_path.size()-1);
        idx = (int)ap.m_path.size();
        ap.m_path.push_back(empty_path);
        for (i=0; i<ap.m_macro_path.size(); i++) {
          ap.m_path[idx].push_back( ap.m_macro_path[m_idx][i] );
        }
        ap.m_exposure.push_back( ap.m_macro_exposure[m_idx] );

        break;

      case AM_ENUM_OUTLINE: printf("#xx outline\n"); break;
      case AM_ENUM_POLYGON: printf("#xx polygon\n"); break;
      case AM_ENUM_MOIRE: printf("#xx moire\n"); break;
      case AM_ENUM_THERMAL: printf("#xx thermal\n"); break;

      default: printf("#xx unknown\n"); break;
    }

    am_nod = am_nod->next;
  }

  //DEBUG
  printf("\n"); fflush(stdout);

  // not ready for this yet...we're adding aperture macro stuff above,
  // need ot figure out how to actually get it into an aperture data definition/realization
  //
  //gAperture.insert( ApertureNameMapPair(ap.m_name, ap) );
  //gApertureName.push_back(ap.m_name);




  return ret;
}

// Construct polygons that will be used for the apertures.
// Curves are linearized.  Circles have a minium of 8
// vertices.
//
int realize_apertures_old(gerber_state_t *gs) {
  double min_segment_length = 0.01;
  int min_segments = 8;
  int base_mapping[] = {1, 2, 3, 3};
  int i;

  aperture_data_t *aperture;

  for ( aperture = gs->aperture_head ;
        aperture ;
        aperture = aperture->next ) {

    Aperture_realization ap;

    if (aperture->type != 4) {
      ap.m_name = aperture->name;
      ap.m_type = aperture->type;
      ap.m_crop_type = aperture->crop_type;
    } else {
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

    switch (aperture->crop_type)
    {
      // solid, do nothing
      //
      case 0: break;

      // circle hole
      //
      case 1:
        realize_hole( ap, aperture->crop[base]/2.0, min_segments, min_segment_length );
        break;

      // rect hole
      //
      case 2:
        //realize_rectangle_hole( ap, aperture->crop[base]/2.0, aperture->crop[base+1]/2.0 );
        break;

      default: break;
    }

    gAperture.insert( ApertureNameMapPair(ap.m_name, ap) );
    gApertureName.push_back(ap.m_name);

  }

  return 0;
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
    printf("##>> aperture->type %i, aperture->name %i\n", aperture->type, aperture->name);

    if (aperture->type != 4) {
      ap.m_name = aperture->name;
      ap.m_type = aperture->type;
      ap.m_crop_type = aperture->crop_type;
    } else {
      ap.m_name = aperture->name;
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
          printf("## circle hole?\n");
          //realize_circle_hole( ap, aperture->crop[base]/2.0, min_segments, min_segment_length );
          break;

        // rect hole
        //
        case 2:
          printf("## rect?\n");
          //realize_rectangle_hole( ap, aperture->crop[base]/2.0, aperture->crop[base+1]/2.0 );
          break;

        default: break;
      }

    }

    //DEBUG
    printf("##?? adding aperture %i\n", ap.m_name); fflush(stdout);

    gAperture.insert( ApertureNameMapPair(ap.m_name, ap) );
    gApertureName.push_back(ap.m_name);

  }

  return 0;
}
