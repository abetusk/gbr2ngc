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

#include "gerber_interpreter.h"

#define DEBUG_INTERPRETER

int (*function_code_handler[13])(gerber_state_t *, char *);



//------------------------

enum {
  QUADRENT_MODE_NONE = 0,
  QUADRENT_MODE_SINGLE,
  QUADRENT_MODE_MULTI,
} quadrent_mode_enum;

void gerber_state_init(gerber_state_t *gs) {
  gs->g_state = -1;
  gs->d_state = -1;
  gs->region = 0;

  gs->unit_init = 0;
  gs->pos_init = 0;
  gs->is_init = 0;

  gs->units_metric = -1;
  gs->polarity = -1;
  gs->polarity_bit = 0;
  gs->eof = 0;
  gs->line_no = 0;

  gs->quadrent_mode = -1;

  gs->current_aperture = -1;

  gs->units_str[0] = strdup("inches");
  gs->units_str[1] = strdup("mm");

  gs->fs_omit_zero[0] = strdup("omit trailing");
  gs->fs_omit_zero[1] = strdup("omit leading");

  gs->fs_coord_value[0] = strdup("relative");
  gs->fs_coord_value[1] = strdup("absolute");

  gs->fs_omit_leading_zero = 0;
  gs->fs_coord_absolute = 1;

  gs->fs_x_int = 0; gs->fs_x_real = 0;
  gs->fs_y_int = 0; gs->fs_y_real = 0;
  gs->fs_i_int = 0; gs->fs_i_real = 0;
  gs->fs_j_int = 0; gs->fs_j_real = 0;

  gs->fs_init = 0;

  gs->aperture_head = gs->aperture_cur = NULL;

  gs->cur_x = 0.0; 
  gs->cur_y = 0.0;

  gs->contour_head = gs->contour_cur = NULL;
  gs->contour_list_head = gs->contour_list_cur = NULL;

  gs->am_lib_head = NULL;
  gs->am_lib_tail = NULL;

  // AB init
  //

  gs->ab_active = 0;
  gs->ab_lib_depth = 0;
  gs->ab_name = -1;

  gs->ab_lib_next = NULL;
  gs->ab_lib_parent = NULL;
  gs->ab_lib_root = gs;

  gs->ab_lib_child_head = NULL;
  gs->ab_lib_child_cur = NULL;
  gs->ab_lib_child_last = NULL;

  //--

  string_ll_init(&(gs->string_ll_buf));
  gs->gerber_read_state = GRS_NONE;
}

// 
gerber_state_t *gerber_state_add_ab_child(gerber_state_t *gs_parent, int ab_name) {
  gerber_state_t *gs_child = NULL;

  gs_child = (gerber_state_t *)malloc(sizeof(gerber_state_t));

  gs_child->g_state   = gs_parent->g_state;
  gs_child->d_state   = gs_parent->d_state;
  gs_child->region    = gs_parent->region;

  gs_child->unit_init = gs_parent->unit_init;
  gs_child->pos_init  = gs_parent->pos_init;
  gs_child->is_init   = gs_parent->is_init;

  gs_child->units_metric  = gs_parent->units_metric;
  gs_child->polarity      = gs_parent->polarity;
  gs_child->polarity_bit  = gs_parent->polarity_bit;
  gs_child->eof           = gs_parent->eof;
  gs_child->line_no       = gs_parent->line_no;

  gs_child->quadrent_mode = gs_parent->quadrent_mode;

  gs_child->current_aperture = gs_parent->current_aperture;

  gs_child->units_str[0] = strdup(gs_parent->units_str[0]);
  gs_child->units_str[1] = strdup(gs_parent->units_str[1]);

  gs_child->fs_omit_zero[0] = strdup(gs_parent->fs_omit_zero[0]);
  gs_child->fs_omit_zero[1] = strdup(gs_parent->fs_omit_zero[1]);

  gs_child->fs_coord_value[0] = strdup(gs_parent->fs_coord_value[0]);
  gs_child->fs_coord_value[1] = strdup(gs_parent->fs_coord_value[1]);

  gs_child->fs_omit_leading_zero = gs_parent->fs_omit_leading_zero;
  gs_child->fs_coord_absolute = gs_parent->fs_coord_absolute;

  gs_child->fs_x_int  = gs_parent->fs_x_int;
  gs_child->fs_x_real = gs_parent->fs_x_real;

  gs_child->fs_y_int  = gs_parent->fs_y_int;
  gs_child->fs_y_real = gs_parent->fs_y_real;

  gs_child->fs_i_int  = gs_parent->fs_i_int;
  gs_child->fs_i_real = gs_parent->fs_i_real;

  gs_child->fs_j_int  = gs_parent->fs_j_int;
  gs_child->fs_j_real = gs_parent->fs_j_real;

  gs_child->fs_init = gs_parent->fs_init;

  gs_child->aperture_head = NULL;
  gs_child->aperture_cur = NULL;

  gs_child->cur_x = gs_parent->cur_x;
  gs_child->cur_y = gs_parent->cur_y;

  gs_child->contour_head = NULL;
  gs_child->contour_cur = NULL;
  gs_child->contour_list_head = NULL;
  gs_child->contour_list_cur = NULL;

  gs_child->am_lib_head = NULL;
  gs_child->am_lib_tail = NULL;

  // Aperture Block
  //

  gs_child->ab_active = 1;
  gs_child->ab_name = ab_name;
  gs_child->ab_lib_depth = gs_parent->ab_lib_depth + 1;

  gs_child->ab_lib_next = NULL;
  gs_child->ab_lib_parent = gs_parent;

  gs_child->ab_lib_child_head = NULL;
  gs_child->ab_lib_child_cur  = NULL;
  gs_child->ab_lib_child_last = NULL;

  gs_child->ab_lib_root = gs_parent->ab_lib_root;

  if (gs_parent->ab_lib_child_head == NULL) {
    gs_parent->ab_lib_child_head = gs_child;
  }
  else {
    gs_parent->ab_lib_child_last->ab_lib_next = gs_child;
  }
  gs_parent->ab_lib_child_cur = gs_child;
  gs_parent->ab_lib_child_last = gs_child;

  //--

  string_ll_init(&(gs_child->string_ll_buf));
  gs_child->gerber_read_state = gs_parent->gerber_read_state;

  return gs_child;
}


//------------------------

void gerber_state_clear(gerber_state_t *gs) {
  contour_list_ll_t *contour_list_nod, *prev_contour_list_nod;
  contour_ll_t *contour_nod, *prev_contour_nod;
  aperture_data_t *aperture_nod, *prev_aperture_nod;

  if (gs->units_str[0])       { free(gs->units_str[0]); }
  if (gs->units_str[1])       { free(gs->units_str[1]); }
  if (gs->fs_omit_zero[0])    { free(gs->fs_omit_zero[0]); }
  if (gs->fs_omit_zero[1])    { free(gs->fs_omit_zero[1]); }
  if (gs->fs_coord_value[0])  { free(gs->fs_coord_value[0]); }
  if (gs->fs_coord_value[1])  { free(gs->fs_coord_value[1]); }

  contour_list_nod = gs->contour_list_head;
  while (contour_list_nod) {
    contour_nod = contour_list_nod->c;
    while (contour_nod) {
      prev_contour_nod = contour_nod;
      contour_nod = contour_nod->next;
      free(prev_contour_nod);
    }

    prev_contour_list_nod = contour_list_nod;
    contour_list_nod = contour_list_nod->next;
    free(prev_contour_list_nod);
  }

  aperture_nod = gs->aperture_head;
  while (aperture_nod) {
    prev_aperture_nod = aperture_nod;
    aperture_nod = aperture_nod->next;
    free(prev_aperture_nod);
  }

}

//------------------------

void _pps(FILE *fp, int n) {
  int i;
  for (i=0;i<n;i++) { fprintf(fp, "."); }
}

void gerber_report_ab_state(gerber_state_t *gs) {
  gerber_state_t *_nod;

  if (!gs) { return; }

  printf("# "); _pps(stdout, gs->ab_lib_depth);
  printf("gerber_report_ab_state: %p\n", gs);

  printf("# "); _pps(stdout, gs->ab_lib_depth);
  printf("ab_active: %i\n", gs->ab_active);

  printf("# "); _pps(stdout, gs->ab_lib_depth);
  printf("ab_name: %i\n", gs->ab_name);

  printf("# "); _pps(stdout, gs->ab_lib_depth);
  printf("ab_lib_depth: %i\n", gs->ab_lib_depth);

  printf("# "); _pps(stdout, gs->ab_lib_depth);
  printf("ab_lib_depth: %i\n", gs->ab_lib_depth);

  printf("# "); _pps(stdout, gs->ab_lib_depth);
  printf("ab_lib_next: %p\n", gs->ab_lib_next);

  printf("# "); _pps(stdout, gs->ab_lib_depth);
  printf("ab_lib_parent: %p\n", gs->ab_lib_parent);

  printf("# "); _pps(stdout, gs->ab_lib_depth);
  printf("ab_lib_child_head: %p\n", gs->ab_lib_child_head);

  printf("# "); _pps(stdout, gs->ab_lib_depth);
  printf("ab_lib_child_last: %p\n", gs->ab_lib_child_last);

  printf("# "); _pps(stdout, gs->ab_lib_depth);
  printf("ab_lib_child_cur: %p\n", gs->ab_lib_child_cur);

  printf("# "); _pps(stdout, gs->ab_lib_depth);
  printf("ab_lib_root: %p\n", gs->ab_lib_root);

  printf("\n");

  dump_information(gs);
  
  _nod = gs->ab_lib_child_head;
  while (_nod) {
    gerber_report_ab_state(_nod);
    _nod = _nod->ab_lib_next;
  }

  _nod = gs->ab_lib_next;
  while (_nod) {
    gerber_report_ab_state(_nod);
    _nod = _nod->ab_lib_next;
  }


}

void gerber_report_state(gerber_state_t *gs) {
  int i, j, k;

  aperture_data_t *ap;

  printf("gerber state:\n");
  printf("  g_state: %i\n", gs->g_state);
  printf("  d_state: %i\n", gs->d_state);
  printf("  unit_init: %i\n", gs->unit_init);
  printf("  pos_init: %i\n", gs->pos_init);
  printf("  fs_init: %i\n", gs->fs_init);
  printf("  is_init: %i\n", gs->is_init);

  printf("  units: %i (%s)\n", gs->units_metric, gs->units_str[gs->units_metric]);
  printf("  polarity: %i\n", gs->polarity);
  printf("  polarity_bit: %i\n", gs->polarity_bit);
  printf("  eof: %i\n", gs->eof);

  printf("  fs_omit_leading_zero: %i (%s)\n", gs->fs_omit_leading_zero, gs->fs_omit_zero[ gs->fs_omit_leading_zero]);
  printf("  fs_coord_absolute: %i (%s)\n", gs->fs_coord_absolute, gs->fs_coord_value[ gs->fs_coord_absolute ]);
  printf("  fs data block format: X%i.%i, Y%i.%i\n", gs->fs_x_int, gs->fs_x_real, gs->fs_y_int, gs->fs_y_real);

  printf("  aperture:\n");
  ap = gs->aperture_head;
  while (ap) {
    printf("    name: %i\n", ap->name);
    printf("    type: %i\n", ap->type);
    printf("    crop_type: %i\n", ap->crop_type);
    printf("    data:");
    k = 0;
    if (ap->type == 0)      { k = ap->crop_type + 1; }
    else if (ap->type == 1) { k = ap->crop_type + 2; }
    else if (ap->type == 2) { k = ap->crop_type + 2; }
    else if (ap->type == 3) { k = ap->crop_type + 2; }

    for (i=0; i<k; i++) { printf(" %g", ap->crop[i]); }
    printf("\n");
    printf("    next: %p\n", ap->next);
    printf("\n");
    ap = ap->next;
  }

  printf("  current_aperture: %i\n", gs->current_aperture);
  printf("  x %g, y %g\n", gs->cur_x, gs->cur_y);
}

//------------------------

void gerber_state_set_units(gerber_state_t *gs, int units_metric) {
  gs->units_metric = units_metric;
  gs->unit_init = 1;
}

//------------------------

int parse_nums(double *rop, char *s, char sep, char term, int max_rop) {
  int k;
  char *chp, ch;
  chp = s;

  for (k=0; k<max_rop; k++) {

    while ( (*chp) && (*chp != sep) && (*chp != term) ) {
      chp++;
    }
    if (!(*chp)) { return -1; }

    ch = *chp;
    *chp = '\0';

    rop[k] = atof(s);

    *chp = ch;

    if (ch == term) { return k+1; }
    chp++;
    s = chp;
  }

  if (ch != term) { return -1; }
  return max_rop;

}

//------------------------

void parse_error(char *s, int line_no, char *l) {
  if (l) {
    printf("PARSE ERROR: %s at line %i '%s'\n", s, line_no, l);
  }
  else {
    printf("PARSE ERROR: %s at line %i\n", s, line_no );
  }
  exit(2);
}

//------------------------

char *skip_whitespace(char *s) {
  while ((*s) && 
         ( (*s == '\n') || (*s == ' ') || (*s == '\t') ) ) {
    s++;
  }
  return s;
}

//------------------------

void string_cleanup(char *rop, char *op) {
  if (!rop) return;
  if (!op) return;
  while (*op) {
    if (*op != '\n') {
      *rop = *op;
      rop++;
    }
    op++;
  }
  *rop = '\0';
}

//------------------------

void strip_whitespace(char *rop, char *op) {
  if (!rop) return;
  if (!op) return;
  while (*op) {
    if (!isspace(*op)) {
      *rop = *op;
      rop++;
    }
    op++;
  }
  *rop = '\0';
}

#define N_FUNCTION_CODE 13

int default_function_code_handler(gerber_state_t *gs, char *buf) {
  printf("default_function_code_handler: '%s'\n", buf);
}

int munch_line(char *linebuf, int n, FILE *fp) {
  char *chp;

  if (!fgets(linebuf, n, fp)) return 0;

  chp = linebuf;
  while ( *linebuf ) {
    while ( *linebuf && isspace( *linebuf ) ) { linebuf++; }
    if (!*linebuf) { *chp = *linebuf; }
    else           { *chp++ = *linebuf++; }
  }

  return 1;
}


// function codes
//

enum {
  FC_D01 = 1, FC_D02, FC_D03, FC_D10P,
  FC_G01, FC_G02, FC_G03, FC_G04,
  FC_G36, FC_G37,
  FC_G74, FC_G75,

  FC_X, FC_Y, FC_I, FC_J,

  FC_M02,

  FC_G54, FC_G55, FC_G70, FC_G71, FC_G90, FC_G91, FC_M00, FC_M01
} FUNCTION_CODE;

//------------------------

int is_function_code_deprecated(int function_code) {
  if (function_code > FC_M02) return 1;
  return 0;
}

//------------------------

int get_function_code(char *linebuf) {
  char *pos = linebuf;

  if (pos[0] == 'X') return FC_X;
  if (pos[0] == 'Y') return FC_Y;
  if (pos[0] == 'I') return FC_I;
  if (pos[0] == 'J') return FC_J;

  if ( (pos[0] == 'D') && (pos[1] == '0') && (pos[2] == '1') ) return FC_D01;
  if ( (pos[0] == 'D') && (pos[1] == '0') && (pos[2] == '2') ) return FC_D02;
  if ( (pos[0] == 'D') && (pos[1] == '0') && (pos[2] == '3') ) return FC_D03;
  if ( (pos[0] == 'G') && (pos[1] == '0') && (pos[2] == '1') ) return FC_G01;
  if ( (pos[0] == 'G') && (pos[1] == '0') && (pos[2] == '2') ) return FC_G02;
  if ( (pos[0] == 'G') && (pos[1] == '0') && (pos[2] == '3') ) return FC_G03;
  if ( (pos[0] == 'G') && (pos[1] == '0') && (pos[2] == '4') ) return FC_G04;
  if ( (pos[0] == 'G') && (pos[1] == '3') && (pos[2] == '6') ) return FC_G36;
  if ( (pos[0] == 'G') && (pos[1] == '3') && (pos[2] == '7') ) return FC_G37;
  if ( (pos[0] == 'G') && (pos[1] == '7') && (pos[2] == '4') ) return FC_G74;
  if ( (pos[0] == 'G') && (pos[1] == '7') && (pos[2] == '5') ) return FC_G75;
  if ( (pos[0] == 'M') && (pos[1] == '0') && (pos[2] == '2') ) return FC_M02;

  if ( (pos[0] == 'G') && (pos[1] == '5') && (pos[2] == '4') ) return FC_G54;
  if ( (pos[0] == 'G') && (pos[1] == '5') && (pos[2] == '5') ) return FC_G55;
  if ( (pos[0] == 'G') && (pos[1] == '7') && (pos[2] == '0') ) return FC_G70;
  if ( (pos[0] == 'G') && (pos[1] == '7') && (pos[2] == '1') ) return FC_G71;
  if ( (pos[0] == 'G') && (pos[1] == '9') && (pos[2] == '0') ) return FC_G90;
  if ( (pos[0] == 'G') && (pos[1] == '9') && (pos[2] == '1') ) return FC_G91;
  if ( (pos[0] == 'M') && (pos[1] == '0') && (pos[2] == '0') ) return FC_M00;
  if ( (pos[0] == 'M') && (pos[1] == '0') && (pos[2] == '1') ) return FC_M01;

  if ( (pos[0] == 'D') && (pos[1] != '0') )
    return FC_D10P;

  return -1;
}

// image parameter codes

enum {
  IMG_PARAM_FS = 1,   // Format Specification
  IMG_PARAM_IN,       // Image Name
  IMG_PARAM_IP,       // Image Polarity
  IMG_PARAM_MO,       // MOde
  IMG_PARAM_AD,       // Aperture Definition
  IMG_PARAM_AM,       // Aperture Macro
  IMG_PARAM_AB,       // Aperture Block
  IMG_PARAM_LN,       // Level Name
  IMG_PARAM_LP,       // Level Polarity
  IMG_PARAM_SR,       // Step and Repeat
 
  // Attributes
  //
  IMG_PARAM_TF,       // File attributes
  IMG_PARAM_TO,       // Object attribute
  IMG_PARAM_TA,       // Aperture attribute
  IMG_PARAM_TD,       // Delete aperture attribute
  IMG_PARAM_DR,       // Set region D-code

  // Depricated Parameters
  //
  IMG_PARAM_AS,       // Axis Select
  IMG_PARAM_MI,       // Mirror Image
  IMG_PARAM_OF,       // OFfset 
  IMG_PARAM_IR,       // Image Rotation
  IMG_PARAM_SF,       // Scale Factor

} IMG_PARAM;

int get_image_parameter_code(char *linebuf) {
  char *pos = linebuf + 1;
  if ( pos[0] && (pos[0] == 'F') && (pos[1] == 'S') ) return IMG_PARAM_FS;
  if ( pos[0] && (pos[0] == 'I') && (pos[1] == 'N') ) return IMG_PARAM_IN;
  if ( pos[0] && (pos[0] == 'I') && (pos[1] == 'P') ) return IMG_PARAM_IP;
  if ( pos[0] && (pos[0] == 'M') && (pos[1] == 'O') ) return IMG_PARAM_MO;
  if ( pos[0] && (pos[0] == 'A') && (pos[1] == 'D') ) return IMG_PARAM_AD;
  if ( pos[0] && (pos[0] == 'A') && (pos[1] == 'M') ) return IMG_PARAM_AM;
  if ( pos[0] && (pos[0] == 'A') && (pos[1] == 'B') ) return IMG_PARAM_AB;
  if ( pos[0] && (pos[0] == 'L') && (pos[1] == 'N') ) return IMG_PARAM_LN;
  if ( pos[0] && (pos[0] == 'L') && (pos[1] == 'P') ) return IMG_PARAM_LP;
  if ( pos[0] && (pos[0] == 'S') && (pos[1] == 'R') ) return IMG_PARAM_SR;

  if ( pos[0] && (pos[0] == 'A') && (pos[1] == 'S') ) return IMG_PARAM_SR;
  if ( pos[0] && (pos[0] == 'M') && (pos[1] == 'I') ) return IMG_PARAM_SR;
  if ( pos[0] && (pos[0] == 'O') && (pos[1] == 'F') ) return IMG_PARAM_SR;
  if ( pos[0] && (pos[0] == 'I') && (pos[1] == 'R') ) return IMG_PARAM_SR;
  if ( pos[0] && (pos[0] == 'S') && (pos[1] == 'F') ) return IMG_PARAM_SR;

  if ( pos[0] && (pos[0] == 'T') && (pos[1] == 'F') ) return IMG_PARAM_TF;
  if ( pos[0] && (pos[0] == 'T') && (pos[1] == 'O') ) return IMG_PARAM_TO;
  if ( pos[0] && (pos[0] == 'T') && (pos[1] == 'A') ) return IMG_PARAM_TA;
  if ( pos[0] && (pos[0] == 'T') && (pos[1] == 'D') ) return IMG_PARAM_TD;
  if ( pos[0] && (pos[0] == 'D') && (pos[1] == 'R') ) return IMG_PARAM_DR;
  return -1;
}

//------------------------

void parse_fs(gerber_state_t *gs, char *linebuf) {
  char *chp;
  int a, b;

  chp = linebuf + 3;
  if ((*chp != 'L') && (*chp != 'T')) parse_error("bad FS zero omission mode", gs->line_no, linebuf);
  if (*chp == 'L') gs->fs_omit_leading_zero = 1;
  else gs->fs_omit_leading_zero = 0;
  chp++;

  if ((*chp != 'A') && (*chp != 'I')) parse_error("bad FS coordinate value mode", gs->line_no, linebuf);
  if (*chp == 'A') gs->fs_coord_absolute = 1;
  else gs->fs_coord_absolute = 0;
  chp++;

  if (*chp != 'X') parse_error("bad FS format (expected 'X')", gs->line_no, linebuf);
  chp++;

  if (!isdigit(*chp)) parse_error("bad FS format (expected digit after 'X')", gs->line_no, linebuf);
  gs->fs_x_int = *chp++ - '0';
  if ((gs->fs_x_int<0) || (gs->fs_x_int>7)) parse_error("bad FS format (expected digit after 'X')", gs->line_no, linebuf);

  if (!isdigit(*chp)) parse_error("bad FS format (expected digit after 'X')", gs->line_no, linebuf);
  gs->fs_x_real = *chp++ - '0';
  if ((gs->fs_x_real<0) || (gs->fs_x_real>7)) parse_error("bad FS format (expected digit after 'X')", gs->line_no, linebuf);

  if (*chp != 'Y') parse_error("bad FS format (expected 'Y')", gs->line_no, linebuf);
  chp++;

  if (!isdigit(*chp)) parse_error("bad FS format (expected digit after 'Y')", gs->line_no, linebuf);
  gs->fs_y_int = *chp++ - '0';
  if ((gs->fs_y_int<0) || (gs->fs_y_int>7)) parse_error("bad FS format (expected digit after 'Y')", gs->line_no, linebuf);

  if (!isdigit(*chp)) parse_error("bad FS format (expected digit after 'Y')", gs->line_no, linebuf);
  gs->fs_y_real = *chp++ - '0';
  if ((gs->fs_y_real<0) || (gs->fs_y_real>7)) parse_error("bad FS format (expected digit after 'Y')", gs->line_no, linebuf);

  gs->fs_i_int = gs->fs_x_int;
  gs->fs_j_int = gs->fs_y_int;

  gs->fs_i_real = gs->fs_x_real;
  gs->fs_j_real = gs->fs_y_real;

  gs->fs_init = 1;
}

//------------------------

void parse_in(gerber_state_t *gs, char *linebuf) {
}

//------------------------

void parse_ip(gerber_state_t *gs, char *linebuf) {
}

//------------------------

void parse_mo(gerber_state_t *gs, char *linebuf) {
  char *chp;

  chp = linebuf+3;
  if      ( (chp[0] == 'I') && (chp[1] = 'N') ) gs->units_metric = 0;
  else if ( (chp[0] == 'M') && (chp[1] = 'N') ) gs->units_metric = 1;
  else parse_error("bad MO format", gs->line_no, linebuf);

  gs->unit_init = 1;
}

//------------------------

void print_aperture_data(gerber_state_t *gs) {
  int i;
  aperture_data_t *a_nod;

  printf("\n");
  printf("## AD\n");
  printf("##\n");

  a_nod = gs->aperture_head;
  while (a_nod) {
    printf("# name: %i\n", a_nod->name);
    printf("# type: %i\n", a_nod->type);
    printf("# crop_type: %i\n", a_nod->crop_type);
    printf("# crop[5]:");
    for (i=0; i<5; i++) { printf(" %f", a_nod->crop[i]); }
    printf("\n");
    printf("# macro_name: %s\n", a_nod->macro_name ? a_nod->macro_name : "" );
    printf("# macro_param[%i]:", a_nod->macro_param_count);
    for (i=0; i<a_nod->macro_param_count; i++) {
      printf(" %f", a_nod->macro_param[i]);
    }
    printf("\n");
    printf("#---\n\n");
    a_nod = a_nod->next;
  }
}

// This is an "extended" aperture definition, treat it separately from
// normal aperture definition.
// Here, the extened aperutre definition means there is a 'modifier set'
// that includes the variables to be passed into the previously defined
// aperture.
// The modifier set is a list of numbers.
// These numbers are variable parameters to the function.
// The array of numbers are separated by an 'X'.
//
void parse_extended_ad(gerber_state_t *gs, char *linebuf) {
  char *chp, *s, ch, *chp_end=NULL;
  long int d_code;
  int complete=0, n=0, param_count=0;

  int i;

  aperture_data_t *ap_db;

  chp = linebuf + 3;

#ifdef DEBUG_INTERPRETER
  printf("## parse extended ad : %s\n", linebuf);
  printf("## parse extended chp: %s\n", chp);
#endif

  if (*chp != 'D') { parse_error("bad AD format", gs->line_no, linebuf); }
  chp++;

  d_code = strtol(chp, NULL, 10);
  if (d_code < 10) { parse_error("bad AD format, D code must be >= 10", gs->line_no, linebuf); }

  while (*chp) {
    if ( (*chp < '0') || (*chp > '9') ) break;
    chp++;
  }
  if (!(*chp)) { parse_error("bad AD format, expected aperture type", gs->line_no, linebuf); }

  for (n=0; chp[n] && (chp[n]!=',') && (chp[n]!='*'); n++);
  if (!chp[n]) { parse_error("bad AD format, premature eol", gs->line_no, linebuf); }
  if (n<2) { parse_error("bad AD format, expected aperture macro name", gs->line_no, linebuf); }

#ifdef DEBUG_INTERPRETER
  printf("## dcode %i\n", (int)d_code);
#endif

  ap_db = (aperture_data_t *)malloc(sizeof(aperture_data_t));
  memset(ap_db, 0, sizeof(aperture_data_t));
  ap_db->next = NULL;
  ap_db->name = d_code;
  ap_db->macro_name = strndup(chp, n);
  ap_db->type = 4;

#ifdef DEBUG_INTERPRETER
  printf("## ap_db->macro_name: %s\n", ap_db->macro_name); fflush(stdout);
#endif

  chp += n;

#ifdef DEBUG_INTERPRETER
  printf("## chp: %s\n", chp); fflush(stdout);
#endif

  param_count=0;
  for (n=0; (chp[n]) && (chp[n]!='*'); n++) {
    if ((chp[n] == 'X') || (chp[n]==',')) {
      param_count++;
    }
  }

#ifdef DEBUG_INTERPRETER
  printf("## param_count: %i\n", param_count); fflush(stdout);
#endif

  ap_db->macro_param_count = param_count;
  if (param_count>0) {
    ap_db->macro_param = (double *)malloc(sizeof(double)*param_count);

    chp++;
    for (i=0; i<param_count; i++) {

      chp_end=chp;
      while ((*chp_end) && ((*chp_end) != '*') && ((*chp_end) != 'X')) { chp_end++; }
      if (!(*chp_end)) { parse_error("bad AD format, unexpcted eol while parsing macro parameters", gs->line_no, linebuf); }

      ch = *chp_end;
      *chp_end = '\0';

#ifdef DEBUG_INTERPRETER
      printf("# converting %s\n", chp); fflush(stdout);
#endif

      ap_db->macro_param[i] = strtod(chp, NULL);

      *chp_end = ch;
      chp = chp_end+1;

    }
  }

  if (!gs->aperture_head) { gs->aperture_head = ap_db; }
  else                    { gs->aperture_cur->next = ap_db; }
  gs->aperture_cur = ap_db;

#ifdef DEBUG_INTERPRETER
  printf("## mactro_param[%i]:", ap_db->macro_param_count);
  for (i=0; i<ap_db->macro_param_count; i++) {
    printf(" %f", ap_db->macro_param[i]);
  }
  printf("\n");
  printf("## ad end\n");
  print_aperture_data(gs);
  fflush(stdout);
#endif

  gs->gerber_read_state = GRS_NONE;
}

void parse_ad(gerber_state_t *gs, char *linebuf_orig) {
  char *linebuf;
  char *chp_beg, *chp, *s, ch;
  char aperture_code;
  int d_code, complete=0, n=0;

  aperture_data_t *ap_db;

  // handle multi line AD
  //
  chp = linebuf_orig;
  n = ( (gs->gerber_read_state == GRS_AD) ? 0 : 1 );

  string_ll_add(&(gs->string_ll_buf), linebuf_orig);
  gs->gerber_read_state = GRS_AD;

  while (chp[n]) {
    if (chp[n] == '%') { complete=1; break; }
    n++;
  }

  if (!complete) { return; }

  linebuf = string_ll_dup_str(&(gs->string_ll_buf));
  string_ll_free(&(gs->string_ll_buf));

  chp = linebuf + 3;

  if (*chp != 'D') { parse_error("bad AD format", gs->line_no, linebuf); }
  chp++;

  chp_beg = chp;

  while (*chp) {
    if ( (*chp < '0') || (*chp > '9') ) break;
    chp++;
  }
  if (!(*chp)) { parse_error("bad AD format, expected aperture type", gs->line_no, linebuf); }

  aperture_code = *chp;
  *chp = '\0';
  d_code = atoi(chp_beg);
  if (d_code < 10) { parse_error("bad AD format, D code must be >= 10", gs->line_no, linebuf); }

  *chp = aperture_code;
  if (!(*chp)) { parse_error("bad AD format, premature eol", gs->line_no, linebuf); }
  chp++;
  if ((*chp) != ',') {
    parse_extended_ad(gs, linebuf);
    free(linebuf);
    return;
  }

  chp++;

  ap_db = (aperture_data_t *)malloc(sizeof(aperture_data_t));
  memset(ap_db, 0, sizeof(aperture_data_t));
  ap_db->next = NULL;
  ap_db->name = d_code;

  switch (aperture_code) {
    case 'C':
      ap_db->type = 0;
      ap_db->crop_type = parse_nums(ap_db->crop, chp, 'X', '*', 3) - 1;
      break;
    case 'R':
      ap_db->type = 1;
      ap_db->crop_type = parse_nums(ap_db->crop, chp, 'X', '*', 4) - 2;
      break;
    case 'O':
      ap_db->type = 2;
      ap_db->crop_type = parse_nums(ap_db->crop, chp, 'X', '*', 4) - 2;
      break;
    case 'P':
      ap_db->type = 3;
      ap_db->crop_type = parse_nums(ap_db->crop, chp, 'X', '*', 5) - 2;
      break;
    default:
      parse_error("bad AD, bad aperture code (must be one of 'C', 'R', 'O', 'P')", gs->line_no, linebuf);
      break;
  }

  if (ap_db->crop_type < 0) {
    parse_error("bad AD, bad aperture data", gs->line_no, linebuf);
  }

  if (!gs->aperture_head) { gs->aperture_head = ap_db; }
  else                    { gs->aperture_cur->next = ap_db; }
  gs->aperture_cur = ap_db;

  gs->gerber_read_state = GRS_NONE;

  free(linebuf);
}

// -----------------------------------------------
// -----------------------------------------------
// -----------------------------------------------
// -----------------------------------------------
// -----------------------------------------------

// only parse the AB line and not the whole block.
// Adding the block to the aperture data library is done
// at a higher level.
//
void parse_ab(gerber_state_t *gs, char *linebuf_orig) {
  char *linebuf;
  char *chp_beg, *chp, *s, ch;
  char save_char;
  int d_code, complete=0, n=0;

  int begin_ab_block = -1;
  gerber_state_t *new_gs=NULL;

  // handle multi line AD
  //
  chp = linebuf_orig;
  n = ( (gs->gerber_read_state == GRS_AB) ? 0 : 1 );

  string_ll_add(&(gs->string_ll_buf), linebuf_orig);
  gs->gerber_read_state = GRS_AB;

  while (chp[n]) {
    if (chp[n] == '%') { complete=1; break; }
    n++;
  }

  //DEBUG
  printf("## parse_ab linebuf_orig: '%s'\n", linebuf_orig);
  printf("## parse_ab complete: %i\n", complete);

  if (!complete) { return; }

  linebuf = string_ll_dup_str(&(gs->string_ll_buf));
  string_ll_free(&(gs->string_ll_buf));

  chp = linebuf + 3;

  if (*chp == 'D') { begin_ab_block = 1; }
  else if (*chp == '*') { begin_ab_block = 0; }
  else { parse_error("bad AB format", gs->line_no, linebuf); }

  chp++;

  // begin Aperture Block
  //
  if (begin_ab_block == 1) {
    chp_beg = chp;

    while (*chp) {
      if ( (*chp < '0') || (*chp > '9') ) break;
      chp++;
    }
    if (!(*chp)) { parse_error("bad AB format, expected character '*'", gs->line_no, linebuf); }

    save_char = *chp;
    *chp = '\0';
    d_code = atoi(chp_beg);
    if (d_code < 10) { parse_error("bad AB format, D code must be >= 10", gs->line_no, linebuf); }

    if (save_char != '*') {
      parse_error("bad AB format, expected character '*'", gs->line_no, linebuf);
    }

    chp++;
    if (!(*chp)) { parse_error("bad AB format, expected character '%'", gs->line_no, linebuf); }
    if (*chp != '%') { parse_error("bad AB format, expected character '%'", gs->line_no, linebuf); }

    //new_gs = (gerber_state_t *)malloc(sizeof(gerber_state_t));
    //gerber_state_init_AB(new_gs, gs);

    new_gs = gerber_state_add_ab_child(gs, d_code);
    if (!new_gs) {
      fprintf(stderr, "could not allocate new AB block, exiting\n");
      exit(-1);
    }

    // Regardless of the 'ab_active' state of the root, we can update it to
    // 1 as we're processing an AB
    //
    new_gs->ab_lib_root->ab_active = 1;

    // Save our current AB data structure so the root can refer to it when
    // processing the rest of teh AB
    //
    new_gs->ab_lib_root->ab_lib_child_cur = new_gs;
    new_gs->gerber_read_state = GRS_NONE;

    //DEBUG
    printf("## begin AB done\n");
    //gerber_report_ab_state(gs);

  }

  // end Aperture Block
  //
  else {

    if (gs->ab_lib_parent == NULL) {
      parse_error("found end of AB without beginning", gs->line_no, linebuf);
    }

    // Since we've tied off the AB, we go up the stack of ABs and make
    // sure the root is pointing to the parent for further processing.
    //
    gs->ab_lib_root->ab_lib_child_cur = gs->ab_lib_parent;

    gs->ab_active = 0;

    // If we've tied the AB and we're one below the root, we've stopped processing
    // the AB and need to updat the root accordingly.
    //
    if (gs->ab_lib_depth==1) {
      gs->ab_lib_parent->ab_active=0;
    }
    //new_gs->gerber_read_state = GRS_NONE;

    //DEBUG
    printf("## end AB\n");
    //gerber_report_ab_state(gs->ab_lib_parent);

  }

  gs->gerber_read_state = GRS_NONE;

  free(linebuf);
}

// -----------------------------------------------
// -----------------------------------------------
// -----------------------------------------------
// -----------------------------------------------
// -----------------------------------------------

void am_node_print(am_ll_node_t *nod) {
  int i;
  char *typs[10] = {
    "AM_ENUM_NAME",
    "AM_ENUM_COMMENT",
    "AM_ENUM_VAR",
    "AM_ENUM_CIRCLE",
    "AM_ENUM_VECTOR_LINE",
    "AM_ENUM_CENTER_LINE",
    "AM_ENUM_OUTLINE",
    "AM_ENUM_POLYGON",
    "AM_ENUM_MOIRE",
    "AM_ENUM_THERMAL"
  };

  while (nod) {
    printf("# type: %i (%s)\n", nod->type, typs[nod->type]);
    printf("#   name: %s\n", nod->name ? nod->name : "" );
    printf("#   comment: %s\n", nod->comment ? nod->comment : "" );
    printf("#   varname: %s\n", nod->varname ? nod->varname : "" );
    printf("#   eval_line[%i]:", nod->n_eval_line);
    for (i=0; i<nod->n_eval_line; i++) {
      printf(" {$%i:\"%s\"}", i+1, nod->eval_line[i]);
    }
    printf("\n");
    nod = nod->next;
  }
}

void am_fill_eval_line(am_ll_node_t *nod, int n_eval_line, char *chp, int *end_pos, int skip_field) {
  int i, apos=0;

  nod->n_eval_line = n_eval_line;
  nod->eval_line = (char **)malloc(sizeof(char **)*(nod->n_eval_line));
  for (i=0; i<nod->n_eval_line; i++) {
    if ((i+skip_field-1)<0) { apos = 0; }
    else { apos = end_pos[i+(skip_field-1)]+1; }

    nod->eval_line[i] = strndup(chp + apos, end_pos[i+skip_field] - apos);
  }

}

void am_ll_node_free(am_ll_node_t *nod) {
  int i;
  am_ll_node_t *tnod;

  while (nod) {
    tnod = nod;
    nod = nod->next;

    if (tnod->name) { free(tnod->name); }
    if (tnod->comment) { free(tnod->comment); }
    if (tnod->varname) { free(tnod->varname); }
    for (i=0; i<tnod->n_eval_line; i++) {
      free(tnod->eval_line[i]);
    }
    if (tnod->eval_line) { free(tnod->eval_line); }
    free(tnod);
  }

}


// duplicate name
//
am_ll_node_t *am_ll_node_create_name_n(char *name, int n) {
  am_ll_node_t *nod;
  char *s;

  nod = (am_ll_node_t *)malloc(sizeof(am_ll_node_t));
  memset(nod, 0, sizeof(am_ll_node_t));
  nod->type = AM_ENUM_NAME;
  nod->name = strndup(name, n);

  return nod;
}

// duplicate whole comment, including code
//
am_ll_node_t * am_ll_node_create_comment_n(char *comment, int n) {
  am_ll_node_t *nod;
  char *s;

  nod = (am_ll_node_t *)malloc(sizeof(am_ll_node_t));
  memset(nod, 0, sizeof(am_ll_node_t));
  nod->type = AM_ENUM_COMMENT;
  nod->comment = strndup(comment, n);

  return nod;
}

// Split the variable name with the evauluation string.
//
// eval_line holds the variable evaluation.
//
am_ll_node_t * am_ll_node_create_var_n(char *s, int n) {
  am_ll_node_t *nod;
  char *chp;
  int idx=0;

  chp = s;

  if (chp[0] != '$') { return NULL; }
  for (idx=0; chp[idx] && (chp[idx] != '=') ; idx++) ;
  if (!chp[idx]) { return NULL; }

  nod = (am_ll_node_t *)malloc(sizeof(am_ll_node_t));
  memset(nod, 0, sizeof(am_ll_node_t));

  nod->varname = strndup(chp, idx);
  nod->type = AM_ENUM_VAR;
  chp += idx+1;

  for (idx=0; chp[idx] && (chp[idx] != '*'); idx++) ;
  if (!chp[idx]) {
    free(nod->varname);
    free(nod);
    return NULL;
  }

  nod->eval_line = (char **)malloc(sizeof(char *));
  nod->n_eval_line = 1;
  nod->eval_line[0] = strndup(chp, idx);

  return nod;
}

// quick n dirty tokenizer
//
int am_parse_end_pos(char *s, int *end_pos, int n_end_pos, int n_opt_param) {
  char *chp;
  int i, idx, n_actual;

  for (chp=s,idx=0,i=0; chp[i] && (chp[i]!='*'); i++) {
    if (chp[i] == ',') {
      end_pos[idx] = i;
      idx++;
    }
    if (idx >= n_end_pos) { return -1; }
  }
  if (idx<n_end_pos) {
    end_pos[idx] = i;
    idx++;
  }
  n_actual = idx;

  if (n_actual < (n_end_pos - n_opt_param - 1)) { return -2; }
  if (n_actual > n_end_pos) { return -3; }

  for (i=1; i<n_actual; i++) {
    if ((end_pos[i] - end_pos[i-1]) <= 0) { return -4; }
  }

  return n_actual;
}


// Circle has 5 fields, 6 with the circle code.
//
// circle_code=1,exposure,diameter,center_x,center_y,rotation
// 
// format string for input into tesexpr.
// eval_line holds diameter, center_x, center_y and rotation
// (4 in total).
//
am_ll_node_t * am_ll_node_create_circle_n(char *s, int n) {
   am_ll_node_t *nod;
  int n_end_pos=6, end_pos[6], i, r;

  r = am_parse_end_pos(s, end_pos, n_end_pos, 1);
  if (r<0) { return NULL; }

  n_end_pos = r;

  nod = (am_ll_node_t *)malloc(sizeof(am_ll_node_t));
  memset(nod, 0, sizeof(am_ll_node_t));
  nod->type = AM_ENUM_CIRCLE;
  am_fill_eval_line(nod, n_end_pos-1, s, end_pos, 1);

  return nod;
}


// vector_line_code=20,exposure,width,start_x,start_y,end_x,end_y,rotation
//
// eval_line holds width, start_x, start_y, end_x, end_y, rotation
// (6 in total).
//
am_ll_node_t * am_ll_node_create_vector_line_n(char *s, int n) {
  am_ll_node_t *nod;
  int n_end_pos=8, end_pos[8], i, r;

  r = am_parse_end_pos(s, end_pos, n_end_pos, 0);
  if (r<0) { return NULL; }

  nod = (am_ll_node_t *)malloc(sizeof(am_ll_node_t));
  memset(nod, 0, sizeof(am_ll_node_t));
  nod->type = AM_ENUM_VECTOR_LINE;
  am_fill_eval_line(nod, 7, s, end_pos, 1);

  return nod;
}

// center_line_ocde=21,exposure,width,height,center_x,center_y,rotation
//
// eval_line holds width,height,center_x,center_y and rotation
// (5 in total).
//
am_ll_node_t * am_ll_node_create_center_line_n(char *s, int n) {
  am_ll_node_t *nod;
  int n_end_pos=7, end_pos[7], i, r;

  r = am_parse_end_pos(s, end_pos, n_end_pos, 0);
  if (r<0) { return NULL; }

  nod = (am_ll_node_t *)malloc(sizeof(am_ll_node_t));
  memset(nod, 0, sizeof(am_ll_node_t));
  nod->type = AM_ENUM_CENTER_LINE;
  am_fill_eval_line(nod, 6, s, end_pos, 1);

  return nod;
}

am_ll_node_t * am_ll_node_create_outline_n(char *s, int n) {
  am_ll_node_t *nod;
  char *chp;
  long int n_pair;
  int n_end_pos=0, *end_pos=NULL, i, r;
  int pfx_end_pos[3], idx=0;

  chp = s;
  for (i=0; chp[i] && (chp[i]!='*'); i++) {
    if (chp[i] == ',') {
      pfx_end_pos[idx] = i;
      idx++;
    }
    if (idx>=3) { break; }
  }
  if (idx!=3) { return NULL; }
  if (!chp[i]) { return NULL; }
  if (chp[i] == '*') { return NULL; }

  n_pair = strtol(chp + pfx_end_pos[1] + 1, NULL, 10);
  if ((errno == EINVAL) || (errno==ERANGE)) { return NULL; }
  if (n_pair < 0) { return NULL; }

  n_end_pos = (int)(2*n_pair) + 3 + 3;
  end_pos = (int *)malloc(sizeof(int)*n_end_pos);

  r = am_parse_end_pos(s, end_pos, n_end_pos, 0);
  if (r<0) { return NULL; }

  nod = (am_ll_node_t *)malloc(sizeof(am_ll_node_t));
  memset(nod, 0, sizeof(am_ll_node_t));
  nod->type = AM_ENUM_OUTLINE;
  am_fill_eval_line(nod, n_end_pos-1, chp, end_pos, 1);

  return nod;
}

am_ll_node_t * am_ll_node_create_polygon_n(char *s, int n) {
  am_ll_node_t *nod;
  int n_end_pos=7, end_pos[7], i, r;

  r = am_parse_end_pos(s, end_pos, n_end_pos, 0);
  if (r<0) { return NULL; }

  nod = (am_ll_node_t *)malloc(sizeof(am_ll_node_t));
  memset(nod, 0, sizeof(am_ll_node_t));
  nod->type = AM_ENUM_POLYGON;
  am_fill_eval_line(nod, 6, s, end_pos, 1);

  return nod;
}

am_ll_node_t * am_ll_node_create_moire_n(char *s, int n) {
  am_ll_node_t *nod;
  int n_end_pos=10, end_pos[10], i, r;

  r = am_parse_end_pos(s, end_pos, n_end_pos, 0);
  if (r<0) { return NULL; }

  nod = (am_ll_node_t *)malloc(sizeof(am_ll_node_t));
  memset(nod, 0, sizeof(am_ll_node_t));
  nod->type = AM_ENUM_MOIRE;
  am_fill_eval_line(nod, 9, s, end_pos, 1);

  return nod;
}

// thermal_code=7,center_x,center_y,outer_diam,inner_diam,gap,rotation
//
// eval_line holds center_x,center_y,outer_diam,inner_diam,gap,rotation
// (6 total).
//
am_ll_node_t * am_ll_node_create_thermal_n(char *s, int n) {
  am_ll_node_t *nod;
  int n_end_pos=7, end_pos[7], i, r;

  r = am_parse_end_pos(s, end_pos, n_end_pos, 0);
  if (r<0) { return NULL; }

  nod = (am_ll_node_t *)malloc(sizeof(am_ll_node_t));
  memset(nod, 0, sizeof(am_ll_node_t));
  nod->type = AM_ENUM_THERMAL;
  am_fill_eval_line(nod, 6, s, end_pos, 1);

  return nod;
}

void am_lib_print(gerber_state_t *gs) {
  am_ll_lib_t *am_lib_nod;

  am_lib_nod = gs->am_lib_head;

  printf("\n\n");
  printf("## AM_LIB\n");
  printf("##\n");

  while (am_lib_nod) {
    am_node_print(am_lib_nod->am);
    am_lib_nod = am_lib_nod->next;
    printf("#---\n\n");
  }

  printf("\n");
}

// aperture macro
//
void parse_am(gerber_state_t *gs, char *linebuf) {
  char *chp=NULL, *dup_str=NULL;
  int i, n=0, complete=0;
  am_ll_node_t *am_nod, *am_nod_head;
  am_ll_lib_t *am_lib_nod;

  chp = linebuf;
  n = ( (gs->gerber_read_state == GRS_AM) ? 0 : 1 );

  string_ll_add(&(gs->string_ll_buf), linebuf);
  gs->gerber_read_state = GRS_AM;

  while (chp[n]) {
    if (chp[n] == '%') { complete=1; break; }
    n++;
  }

  if (!complete) { return; }

  dup_str = string_ll_dup_str(&(gs->string_ll_buf));
  string_ll_free(&(gs->string_ll_buf));

#ifdef DEBUG_INTERPRETER
  printf("### AM %s\n", dup_str);
#endif

  // skip "%AM"
  //
  chp = dup_str+3;

#ifdef DEBUG_INTERPRETER
  printf("### chp: %s\n", chp);
#endif

  // scan for end of line ('*' token denotes eol) and
  // save the macro name.
  //
  for (n=0; (chp[n]) && (chp[n]!='*'); n++) ;
  am_nod_head = am_ll_node_create_name_n(chp, n);
  am_nod = am_nod_head;

  chp += n+1;

#ifdef DEBUG_INTERPRETER
  printf("## am_nod_head(%p) %s\n", am_nod_head, am_nod_head->name);
#endif

  while ((*chp) && ((*chp) != '%')) {

    for (n=0; chp[n] && (chp[n]!='*'); n++);

    if (!chp[n]) { goto am_parse_error; }

    if      ((*chp) == '0') { am_nod->next = am_ll_node_create_comment_n(chp, n); }
    else if ((*chp) == '$') { am_nod->next = am_ll_node_create_var_n(chp, n); }
    else if ((*chp) == '1') { am_nod->next = am_ll_node_create_circle_n(chp, n); }
    else if ((*chp) == '4') { am_nod->next = am_ll_node_create_outline_n(chp, n); }
    else if ((*chp) == '5') { am_nod->next = am_ll_node_create_polygon_n(chp, n); }
    else if ((*chp) == '6') { am_nod->next = am_ll_node_create_moire_n(chp, n); }
    else if ((*chp) == '7') { am_nod->next = am_ll_node_create_thermal_n(chp, n); }

    else if ((*chp) == '2') {
      if (n<2) { goto am_parse_error; }
      if      (chp[1] == '0') { am_nod->next = am_ll_node_create_vector_line_n(chp, n); }
      else if (chp[1] == '1') { am_nod->next = am_ll_node_create_center_line_n(chp, n); }
      else { goto am_parse_error; }
    }

    else { goto am_parse_error; }

    if (!am_nod->next) { goto am_parse_error; }

    chp += n+1;
    am_nod = am_nod->next;
  }

  //--

  gs->gerber_read_state = GRS_NONE;
  if (dup_str) { free(dup_str); }

  //--

  // add it to our aperture macro library
  //
  am_lib_nod = (am_ll_lib_t *)malloc(sizeof(am_ll_lib_t));
  am_lib_nod->next = NULL;
  am_lib_nod->am = am_nod_head;
  if (gs->am_lib_head)  { gs->am_lib_tail->next = am_lib_nod; }
  else                  { gs->am_lib_head       = am_lib_nod; }
  gs->am_lib_tail = am_lib_nod;

#ifdef DEBUG_INTERPRETER
  am_lib_print(gs);
#endif

  return;

am_parse_error:

  am_ll_node_free(am_nod_head);
  printf("PARSE ERROR: bad AM format, line_no %i\n", gs->line_no);
  if (dup_str) { free(dup_str); }

  parse_error("bad AM format", gs->line_no, linebuf);
}

//------------------------

void parse_ln(gerber_state_t *gs, char *linebuf) {
  //printf("# ln '%s'\n", linebuf);
}

//------------------------

void parse_lp(gerber_state_t *gs, char *linebuf) {
  int polarity = -1;
  int ch;

  ch = *linebuf;
  if ((ch != 'C') && (ch != 'D')) {
    parse_error("invalid LP option. Expected 'C' or 'D' for LP comamnd", gs->line_no, NULL);
  }

  if (ch=='C') {
    gs->polarity = 0;
    gs->polarity_bit = 1;
  }
  else if (ch=='D') {
    gs->polarity = 1;
    gs->polarity_bit = 0;
  }

}

//------------------------

// TODO
// step and repeat
//
void parse_sr(gerber_state_t *gs, char *linebuf) {

}

// File attributes
//
// not strictly an image parameter, I guess,
// but easier to include in the grouping than
// not.
//
void parse_tf(gerber_state_t *gs, char *linebuf) {
}

// Object attribute
//
void parse_to(gerber_state_t *gs, char *linebuf) {
}

// Aperture attributes
//
void parse_ta(gerber_state_t *gs, char *linebuf) {
}

// Delete aperture attribute
//
void parse_td(gerber_state_t *gs, char *linebuf) {
}

// Set region D-code
//
void parse_dr(gerber_state_t *gs, char *linebuf) {
}


//-----------------
// function codes
// ----------------


void parse_d01(gerber_state_t *gs, char *linebuf) {
}

//------------------------

void parse_d02(gerber_state_t *gs, char *linebuf) { 
}

//------------------------

void parse_d03(gerber_state_t *gs, char *linebuf) { 
}

//------------------------

void parse_d10p(gerber_state_t *gs, char *linebuf) {
  int d_code = -1;

  if (*linebuf != 'D') parse_error("expected D op code", gs->line_no, NULL);
  linebuf++;

  d_code = atoi(linebuf);

  if (d_code < 10) parse_error("D code < 10", gs->line_no, NULL);
  gs->current_aperture = d_code;

}

//------------------------

char *parse_single_coord(gerber_state_t *gs, double *val, int fs_int, int fs_real, char *s) {
  int i, j, k;
  char *chp, ch, *tbuf;
  int max_buf, pos=0;
  int real_processed_count=0;

  max_buf = fs_int + fs_real + 3;
  tbuf = (char *)malloc(sizeof(char)*(max_buf));

  s++;
  
  // advance to non-digit portion
  //
  for (chp = s; (*chp && (isdigit(*chp) || (*chp == '-') || (*chp == '+'))) ; chp++);
  if (!*chp) parse_error("unexpected eol", gs->line_no, NULL);
  k = (int)(chp - s);
  if (k >= max_buf) {
    parse_error("invalid coordinate", gs->line_no, NULL);
  }

  // that is, trailing zeros manditory
  //
  if (gs->fs_omit_leading_zero) {

    tbuf[max_buf-1] = '\0';
    pos = max_buf-2;
    for (i=0; i<fs_real; i++) {
      if ((!isdigit(chp[-i-1])) || (chp[-i-1] == '-') || (chp[-i-1] == '+')) { break; }
      tbuf[pos--] = chp[-i-1];
      real_processed_count++;
    }

    // fill in leading zeros, if any
    //
    for (; i<fs_real; i++) { tbuf[pos--] = '0'; }

    tbuf[pos--] = '.';
    chp -= real_processed_count+1;

    while (chp >= s) {
      if (pos < 0) {
        parse_error("coordinate format exceeded (olz)", gs->line_no, NULL);
      }
      tbuf[pos--] = *chp--;
    }

    while (pos>=0) { tbuf[pos--] = ' '; }

  } else {

    chp = s;
    if (*chp == '+') { chp++; }
    if (*chp == '-') {
      chp++;
      tbuf[pos++] = '-';
    }

    for (i=0; i<fs_int; i++) {
      if (!isdigit(*chp)) { parse_error("expected number", gs->line_no, NULL); }
      tbuf[pos++] = *chp++;
    }
    tbuf[pos++] = '.';

    while (*chp && (isdigit(*chp))) {
      tbuf[pos++] = *chp++;

      if (pos >= (max_buf-1)) {
        parse_error("coordinate format exceeded (wlz)", gs->line_no, NULL);
      }

    }
    tbuf[pos] = '\0';
  }

  errno = 0;
  *val = strtod(tbuf, NULL);
  if ((*val == 0.0) && errno) {
    parse_error("bad conversion", gs->line_no, NULL);
  }

  free(tbuf);
  return s + k;
}

//------------------------

char *parse_single_int(gerber_state_t *gs, int *val, char *s) {
  int i, j, k;
  char *chp, ch, *tbuf;;
  double d;

  //if (*s != tok) parse_error("expected token", line_no, NULL);
  s++;
  
  for (chp = s;  (*chp && (isdigit(*chp) || (*chp == '-') || (*chp == '+'))) ; chp++);
  if (!*chp) { parse_error("unexpected eol", gs->line_no, NULL); }

  ch = *chp;
  *chp = '\0';

  tbuf = strdup(s);
  *chp = ch;

  errno = 0;
  *val = atoi(tbuf);
  if (*val <= 0) {
    parse_error("bad conversion", gs->line_no, NULL);
  }
  free(tbuf);

  return chp;
}

//------------------------

void add_segment(gerber_state_t *gs, double prev_x, double prev_y, double cur_x, double cur_y, int aperture_name) {
  contour_ll_t *contour_nod;
  contour_list_ll_t *contour_list_nod;

  contour_list_nod = (contour_list_ll_t *)malloc(sizeof(contour_list_ll_t));
  contour_list_nod->next = NULL;

  contour_nod = (contour_ll_t *)malloc(sizeof(contour_ll_t));
  contour_nod->d_name = aperture_name;
  contour_nod->x = prev_x;
  contour_nod->y = prev_y;
  contour_nod->region = 0;

  contour_list_nod->c = contour_nod;

  contour_nod->next = (contour_ll_t *)malloc(sizeof(contour_ll_t));
  contour_nod = contour_nod->next;
  contour_nod->d_name = aperture_name;
  contour_nod->x = cur_x;
  contour_nod->y = cur_y;
  contour_nod->next = NULL;
  contour_nod->region = 0;

  if (gs->contour_list_head == NULL) {
    gs->contour_list_head = contour_list_nod;
  }
  else {
    gs->contour_list_cur->next = contour_list_nod;
  }
  gs->contour_list_cur = contour_list_nod;

}

//------------------------

void add_flash(gerber_state_t *gs, double cur_x, double cur_y, int aperture_name) {
  contour_ll_t *contour_nod;
  contour_list_ll_t *contour_list_nod;

  contour_list_nod = (contour_list_ll_t *)malloc(sizeof(contour_list_ll_t));
  contour_list_nod->next = NULL;

  contour_nod = (contour_ll_t *)malloc(sizeof(contour_ll_t));
  contour_nod->d_name = aperture_name;
  contour_nod->x = cur_x;
  contour_nod->y = cur_y;
  contour_nod->region = 0;

  contour_nod->next = NULL;

  contour_list_nod->c = contour_nod;

  if (gs->contour_list_head == NULL) {
    gs->contour_list_head = contour_list_nod;
  }
  else {
    gs->contour_list_cur->next = contour_list_nod;
  }
  gs->contour_list_cur = contour_list_nod;
}

//------------------------

void parse_data_block(gerber_state_t *gs, char *linebuf) {
  char *chp;
  unsigned char state=0;
  double prev_x, prev_y;
  contour_ll_t *contour_nod;
  contour_list_ll_t *contour_list_nod;
  int prev_d_state;

  prev_x = gs->cur_x;
  prev_y = gs->cur_y;
  prev_d_state = gs->d_state;

  chp = linebuf;

  while (*chp) {

    if (*chp == 'X') {
      if (state & 1) parse_error("multiple 'X' tokens found", gs->line_no, NULL);
      state |= 1;
      chp = parse_single_coord(gs, &(gs->cur_x), gs->fs_x_int, gs->fs_x_real, chp);
    }

    else if (*chp == 'Y') {
      if (state & 2) parse_error("multiple 'Y' tokens found", gs->line_no, NULL);
      state |= 2;
      chp = parse_single_coord(gs, &(gs->cur_y), gs->fs_y_int, gs->fs_y_real, chp);
    }

    else if (*chp == 'D') {
      if (state & 4) parse_error("multiple 'D' tokens found", gs->line_no, NULL);
      state |= 4;
      chp = parse_single_int(gs, &(gs->d_state), chp);
    }

    else if (*chp == 'I') {
      if (state & 8) parse_error("multiple 'I' tokens found", gs->line_no, NULL);
      state |= 2;
      chp = parse_single_coord(gs, &(gs->cur_i), gs->fs_i_int, gs->fs_i_real, chp);
    }

    else if (*chp == 'J') {
      if (state & 16) parse_error("multiple 'I' tokens found", gs->line_no, NULL);
      state |= 2;
      chp = parse_single_coord(gs, &(gs->cur_j), gs->fs_j_int, gs->fs_j_real, chp);
    }

    else if (*chp == '*') {
      break;
    }
    else {
      parse_error("unknown token when parsing data block", gs->line_no, NULL);
    }

  }

  // handle region
  //
  if (gs->region) {

    if (gs->d_state == 1) {

      if (gs->contour_head == NULL) {

        contour_nod = (contour_ll_t *)malloc(sizeof(contour_ll_t));
        contour_nod->d_name = gs->current_aperture;
        contour_nod->x = prev_x;
        contour_nod->y = prev_y;
        contour_nod->region = 1;
        contour_nod->next  = NULL;

        gs->contour_head = gs->contour_cur = contour_nod;
      }

      contour_nod = (contour_ll_t *)malloc(sizeof(contour_ll_t));
      contour_nod->d_name = gs->current_aperture;
      contour_nod->x = gs->cur_x;
      contour_nod->y = gs->cur_y;
      contour_nod->region = 1;
      contour_nod->next = NULL;

      gs->contour_cur->next = contour_nod;
      gs->contour_cur = contour_nod;
    }

    // need to tie off region
    //
    else {

      if (gs->contour_head) {

        contour_list_nod = (contour_list_ll_t *)malloc(sizeof(contour_list_ll_t));
        contour_list_nod->n = 1;
        contour_list_nod->next = NULL;
        contour_list_nod->c = gs->contour_head;

        if (!gs->contour_list_head) {
          gs->contour_list_head = contour_list_nod;
        }
        else {
          gs->contour_list_cur->next = contour_list_nod;
        }
        gs->contour_list_cur = contour_list_nod;

        gs->contour_head = NULL;
        gs->contour_cur= NULL;
      }

    }
      
  }
  else {

    if (gs->d_state == 1) {
      add_segment(gs, prev_x, prev_y, gs->cur_x, gs->cur_y, gs->current_aperture);
    }

  }

  // Move without any realization
  //
  if (gs->d_state == 2) {

  }

  if (gs->d_state == 3) {
    add_flash(gs, gs->cur_x, gs->cur_y, gs->current_aperture);
  }

}

//------------------------

char *parse_d_state(gerber_state_t *gs, char *s) {
  char *chp, ch;
  if (*s != 'D') parse_error("bad D state, expected 'D'", gs->line_no, NULL);
  s++;

  for (chp = s; (*chp) && (isdigit(*chp)); chp++ );
  ch = *chp;
  *chp = '\0';

  gs->d_state = atoi(s);
  if (gs->d_state < 0) parse_error("bad D state, expected >= 0", gs->line_no, NULL);

  *chp = ch;

  if (!ch) return chp;
  return chp+1;

}

//------------------------

void parse_g01(gerber_state_t *gs, char *linebuf_orig) { 
  char *linebuf;
  char *chp;
  unsigned int state = 0;

  double prev_x, prev_y;
  contour_ll_t *contour_nod;

  prev_x = gs->cur_x;
  prev_y = gs->cur_y;

  linebuf = strdup(linebuf_orig);

  chp = linebuf + 1;
  gs->g_state = 1;

  if (chp[0] == '1') chp++;
  else if ((chp[0] == '0') && (chp[1] == '1')) chp+=2;
  else parse_error("invalid g01 code", gs->line_no, linebuf);

  if (*chp == '*') {
    free(linebuf);
    return;
  }

  parse_data_block(gs, chp);

  free(linebuf);


}


//------------------------

// clockwise circular interpolation
//
void parse_g02(gerber_state_t *gs, char *linebuf) { 
  parse_error("unsuported g02", gs->line_no, NULL);
}

//------------------------

// counter-clockwise circular interpolation
//
void parse_g03(gerber_state_t *gs, char *linebuf) { 
  parse_error("unsuported g03", gs->line_no, NULL);
}

//------------------------

// comment
//
void parse_g04(gerber_state_t *gs, char *linebuf) { 
}

//------------------------

// start region
//
void parse_g36(gerber_state_t *gs, char *linebuf) { 
  gs->g_state = 36;
  gs->region = 1;
  gs->contour_head = gs->contour_cur = NULL;

}

//------------------------

void parse_g37(gerber_state_t *gs, char *linebuf) { 

  contour_list_ll_t *nod;

  gs->g_state = 37;
  gs->region = 0;

  // tie off region
  //
  if (gs->contour_head) {
    nod = (contour_list_ll_t *)malloc(sizeof(contour_list_ll_t));
    nod->next = NULL;
    nod->c = gs->contour_head;
    nod->n = 1;

    if (!gs->contour_list_head) {
      gs->contour_list_head = nod;
    }
    else {
      gs->contour_list_cur->next = nod;
    }
    gs->contour_list_cur = nod;

    gs->contour_head = NULL;
    gs->contour_cur = NULL;
  }

}

//------------------------

void parse_g74(gerber_state_t *gs, char *linebuf) { 
  gs->quadrent_mode = QUADRENT_MODE_SINGLE;
}

//------------------------

void parse_g75(gerber_state_t *gs, char *linebuf) { 
  gs->quadrent_mode = QUADRENT_MODE_MULTI;
}

//------------------------

void parse_m02(gerber_state_t *gs, char *linebuf) { 
  gs->eof = 1;
}

//------------------------

void parse_g54(gerber_state_t *gs, char *linebuf) { 
  linebuf += 3;
  parse_d10p(gs, linebuf);
}

//------------------------

void parse_g55(gerber_state_t *gs, char *linebuf) { 
  parse_error("unsuported g55", gs->line_no, NULL);
}

//------------------------

// deprecated untis to inches
//
void parse_g70(gerber_state_t *gs, char *linebuf) { 
  gs->units_metric = 0;
}

//------------------------

// deprecated untis to mm
//
void parse_g71(gerber_state_t *gs, char *linebuf) { 
  gs->units_metric = 1;
}

//------------------------

// deprecated absolute coordinate
//
void parse_g90(gerber_state_t *gs, char *linebuf) { 
  gs->fs_coord_absolute = 1;
}

//------------------------

// deprecated incremental coordinate
//
void parse_g91(gerber_state_t *gs, char *linebuf) { 
  gs->fs_coord_absolute = 0;
}

//------------------------

// deprecated program stop
//
void parse_m00(gerber_state_t *gs, char *linebuf) { 
  gs->eof = 1;
}

//------------------------

// deprecated optional stop
//
void parse_m01(gerber_state_t *gs, char *linebuf) { 
  gs->eof = 1;
}

//-------------------------------

enum {
  APERTURE_CIRCLE = 0,
  APERTURE_RECTANGLE,
  APERTURE_OBROUND,
  APERTURE_POLYGON
} aperture_enum;

void dump_information(gerber_state_t *gs) {
  int i, j, k, n=0, verbose_print=1;
  int cur_contour_list = 0;

  aperture_data_t *adb;
  contour_list_ll_t *cl;
  contour_ll_t *c;

  if (verbose_print) {
    printf("# aperture list:\n");
  }

  adb = gs->aperture_head;
  k=0;
  while (adb) {

    if (verbose_print) {
      printf("#  [%i] name: %i, type: %i, crop_type %i, ", k, adb->name, adb->type, adb->crop_type);
    }

    if (adb->type == APERTURE_CIRCLE) {
      if (verbose_print)
        printf("# circle:");

      n = adb->crop_type + 1;
    }

    else if (adb->type == APERTURE_RECTANGLE) {
      if (verbose_print)
        printf("# rectangle:");

      n = adb->crop_type + 2;
    }

    else if (adb->type == APERTURE_OBROUND) {
      if (verbose_print)
        printf("# obround:");

      n = adb->crop_type + 2;
    }

    else if (adb->type == APERTURE_POLYGON) {
      if (verbose_print)
        printf("# polygon:");

      n = adb->crop_type + 3;
    }

    if (verbose_print) {
      for (i=0; i<n; i++) printf(" %g", adb->crop[i]);
      printf("\n");
    }

    adb = adb->next;
    k++;
  }

  cur_contour_list = 0;
  cl = gs->contour_list_head;
  while (cl) {

    if (verbose_print) {
      printf("# [%i] contour_list\n", cur_contour_list);
    }

    k=0;
    c = cl->c;

    while (c) {
      if (verbose_print) {
        printf("#  [%i] contour d_name: %i, region: %i, x,y: (%g,%g)\n", k, c->d_name, c->region, c->x, c->y);
        printf("# %g %g\n", c->x, c->y);
      }
      c = c->next;
      k++;
    }

    cl = cl->next;
    cur_contour_list++;

    if (verbose_print) { printf("\n"); }
  }

}

//-------------------------------

// do any post processing that needs to be done
//
int gerber_state_post_process(gerber_state_t *gs) {
}

//-------------------------------

int gerber_state_interpret_line(gerber_state_t *root_gs, char *linebuf) {
  int image_parameter_code;
  int function_code;

  gerber_state_t *ab_gs, *gs;

  gs = root_gs;
  if (root_gs->ab_active) {

    //DEBUG
    printf("## cp.gsil root_gs->ab_active %i, '%s'\n", root_gs->ab_active, linebuf);
    //gerber_report_ab_state(root_gs);


    gs = root_gs->ab_lib_child_cur;
  }

  else {

    //DEBUG
    printf("## cp.gsil ... '%s'\n", linebuf);

  }

  // multiline parsing
  //
  if      (gs->gerber_read_state == GRS_NONE) { }
  else if (gs->gerber_read_state == GRS_AD) { parse_ad(gs, linebuf); return 0; }
  else if (gs->gerber_read_state == GRS_AM) { parse_am(gs, linebuf); return 0; }
  else if (gs->gerber_read_state == GRS_AB) { parse_ab(gs, linebuf); return 0; }

  // it's an image parameter
  //
  if ( linebuf[0] == '%' ) {

    image_parameter_code = get_image_parameter_code(linebuf);

    switch (image_parameter_code) {
      case IMG_PARAM_FS: parse_fs(gs, linebuf); break;
      case IMG_PARAM_IN: parse_in(gs, linebuf); break;
      case IMG_PARAM_IP: parse_ip(gs, linebuf); break;
      case IMG_PARAM_MO: parse_mo(gs, linebuf); break;
      case IMG_PARAM_AD: parse_ad(gs, linebuf); break;
      case IMG_PARAM_AM: parse_am(gs, linebuf); break;
      case IMG_PARAM_AB:
                         parse_ab(gs, linebuf);


                         //DEBUG
                         printf("## root_gs->ab_active %i\n", root_gs->ab_active);
                         if (root_gs->ab_active==0) {
                           printf("### SHOULD BE OUT OF AB, reporting AB state\n");
                           gerber_report_ab_state(root_gs);
                         }

                         break;
      case IMG_PARAM_LN: parse_ln(gs, linebuf); break;
      case IMG_PARAM_LP: parse_lp(gs, linebuf); break;
      case IMG_PARAM_SR: parse_sr(gs, linebuf); break;
      case IMG_PARAM_TF: parse_tf(gs, linebuf); break;
      case IMG_PARAM_TO: parse_to(gs, linebuf); break;
      case IMG_PARAM_TA: parse_ta(gs, linebuf); break;
      case IMG_PARAM_TD: parse_td(gs, linebuf); break;
      case IMG_PARAM_DR: parse_dr(gs, linebuf); break;
      default:
        parse_error("unknown image parameter code", gs->line_no, linebuf);
        break;
    }

  }
  else {

    function_code = get_function_code(linebuf);

    switch (function_code) {
      case FC_X:
      case FC_Y:
      case FC_I:
      case FC_J:
        parse_data_block(gs, linebuf);

        break;
      case FC_D01: parse_d01(gs, linebuf); break;
      case FC_D02: parse_d02(gs, linebuf); break;
      case FC_D03: parse_d03(gs, linebuf); break;
      case FC_D10P: parse_d10p(gs, linebuf); break;
      case FC_G01: parse_g01(gs, linebuf); break;
      case FC_G02: parse_g02(gs, linebuf); break;
      case FC_G03: parse_g03(gs, linebuf); break;
      case FC_G04: parse_g04(gs, linebuf); break;
      case FC_G36: parse_g36(gs, linebuf); break;
      case FC_G37: parse_g37(gs, linebuf); break;
      case FC_G74: parse_g74(gs, linebuf); break;
      case FC_G75: parse_g75(gs, linebuf); break;
      case FC_M02: parse_m02(gs, linebuf); break;
      case FC_G54: parse_g54(gs, linebuf); break;
      case FC_G55: parse_g55(gs, linebuf); break;
      case FC_G70: parse_g70(gs, linebuf); break;
      case FC_G71: parse_g71(gs, linebuf); break;
      case FC_G90: parse_g90(gs, linebuf); break;
      case FC_G91: parse_g91(gs, linebuf); break;
      case FC_M00: parse_m00(gs, linebuf); break;
      case FC_M01: parse_m01(gs, linebuf); break;
      default:
        parse_error("unknown function code", gs->line_no, linebuf);
        break;
    }
  }

  return 0;
}

//------------------------


int gerber_state_load_file(gerber_state_t *gs, char *fn) {
  FILE *fp;
  //char linebuf[GERBER_STATE_LINEBUF];
  char *linebuf=NULL;

  linebuf = (char *)malloc(sizeof(char)*GERBER_STATE_LINEBUF);
  if (linebuf==NULL) { return -2; }

  if (strncmp(fn, "-", 2) == 0) {
    fp = stdin;
  } 
  else if ( !(fp = fopen(fn, "r")) )  {
    free(linebuf);
    return -1;
  }

  gs->line_no = 0;
  while (!feof(fp)) {
    if (!munch_line(linebuf, GERBER_STATE_LINEBUF, fp)) {
      continue;
    }
    gs->line_no++;

     // it's an empty line
    if (linebuf[0] == '\0') {
      continue;
    }

    gerber_state_interpret_line(gs, linebuf);
  }

  if (fp != stdin) {
    fclose(fp);
  }

  gerber_state_post_process(gs);

  free(linebuf);
  return 0;
}
