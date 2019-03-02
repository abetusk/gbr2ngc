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


#ifndef GERBER_INTERPRETER
#define GERBER_INTERPRETER

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "tesexpr.h"
#include "string_ll.h"

//#define GERBER_STATE_LINEBUF 4099
#define GERBER_STATE_LINEBUF 65537

typedef struct point_link_double_2_type {
  double x, y;
  struct point_link_double_2_type *next;
} point_link_double_2_t;


// a node in the linked list which contains
// x, y information, d_name information, etc.
//
typedef struct contour_ll_type {
  int d_name;
  int n;
  double x, y;
  int region;
  int polarity;

  struct contour_ll_type *next;
} contour_ll_t;

// each node contains a pointer to a contour_ll_types,
// so basically a list of linked lists.
//
typedef struct contour_list_ll_type {
  int n;
  contour_ll_t *c;
  struct contour_list_ll_type *next;
} contour_list_ll_t;


// CROP  TYPE
//  C    solid            crop[0] - diameter
//  C    circle hole      crop[0] - diameter,
//                        crop[1] - hole diameter
//  C    rect hole        copr[0] - diameter,
//                        crop[1] - hole x length,
//                        crop[2] - hole y length
//
//  R    solid            crop[0] - x length,
//                        crop[1] - y length
//  R    circle hole      crop[0] - x length,
//                        crop[1] - y length,
//                        crop[2] - hole diameter
//  R    rect hole        crop[0] - x length,
//                        crop[1] - y length,
//                        crop[2] - hole x length,
//                        crop[3] - hole y length
//
//  O    solid            crop[0] - x length,
//                        crop[1] - y length
//  O    circle hole      crop[0] - x length,
//                        crop[1] - y length,
//                        crop[2] - hole diameter
//  O    rect hole        crop[0] - x length,
//                        crop[1] - y length,
//                        crop[3] - hole x length,
//                        crop[4] - hole y length
//
//  P    solid            crop[0] - outer diameter (bounding circle),
//                        crop[1] - number of vertices (3 to 12),
//                        crop[2] - degree of rotation
//  P    circle hole      crop[0] - outer diameter (bounding circle),
//                        crop[1] - number of vertices (3 to 12),
//                        crop[2] - degree of rotation,
//                        crop[3] - hole diameter
//  P    rect hole        crop[0] - outer diameter (bounding circle),
//                        crop[1] - number of vertices (3 to 12),
//                        crop[2] - degree of rotation,
//                        crop[3] - hole x length,
//                        crop[4] - hole y length
//

struct gerber_state_type;

// linked list of aperture data.
//
typedef struct aperture_data_type {
  int name;
  int type;       // 0 - C, 1 - R, 2 - O, 3 - P, 4 - macro, 5 - block
  int crop_type;  // 0 - solid, 1 - circle cutout, 2 - rectangle cutout
  double crop[5];
                  // c[0] - diameter of hole, c[1] either inner diam of cutout circle or x rect,, c[2] y rect
                  // r[0] - x rect, r[1] - y rect, r[2:3] same as above
                  // o[0] - x oblong, o[1] - y oblong, o[2:3] same as above
                  // p[0] - outer diatemer (bounding circle)
                  // p[1] - number of vertices (3 to 12)
                  // p[2] - degrees of rotation
                  // p[3:4] - inner cutout (same as [2:3] above)

  char *macro_name;
  int macro_param_count;
  double *macro_param;

  struct aperture_data_type *next;

  //--
  //--
  //--

  // Our 'local' gerber_state_type data structure
  //
  struct gerber_state_type *gs;

  // AB pointer to parent (parent's 'child' structure points to this).
  // NULL for root.
  //
  //struct aperture_data_type *ab_lib_parent;

  // All children AB linked lists have a pointer back to
  // root for ease of access.
  // By convention, the root node has thispoint to itself.
  //
  //struct gerber_state_type *ab_lib_root_gs;

  //--
  //--
  //--

} aperture_data_t;

typedef struct aperture_data_block_type {
} aperture_data_block_t;

enum GERBER_READ_STATE {
  GRS_NONE = 0,

  GRS_AD,
  GRS_AD_INIT,
  GRS_AD_MODIFIER,

  GRS_AM,
  GRS_AM_INIT,
  GRS_AM_COMMENT,
  GRS_AM_MACRO,
  GRS_AM_MODIFIER,
  GRS_AM_VAR,

  GRS_AB,
  GRS_AB_INIT,
  GRS_AB_COMMENT,
  GRS_AB_MODIFIER,

  GRS_SR,
} ;

//----

// Aperture Definition
//

enum AD_ENUM_TYPE {
  AD_ENUM_CIRCLE = 0,
  AD_ENUM_RECTANGLE,
  AD_ENUM_OBROUND,
  AD_ENUM_POLYGON,
  AD_ENUM_MACRO,
  AD_ENUM_BLOCK,
  AD_ENUM_STEP_REPEAT,
};

// Aperture Macro
//

enum AM_ENUM_TYPE {
  AM_ENUM_NAME,
  AM_ENUM_COMMENT,
  AM_ENUM_VAR,
  AM_ENUM_CIRCLE,
  AM_ENUM_VECTOR_LINE,
  AM_ENUM_CENTER_LINE,
  AM_ENUM_OUTLINE,
  AM_ENUM_POLYGON,
  AM_ENUM_MOIRE,
  AM_ENUM_THERMAL,
};

typedef struct am_ll_node_type {
  int type;
  char *name;
  char *comment;
  char *varname;
  char **eval_line;
  int n_eval_line;
  struct am_ll_node_type *next;
} am_ll_node_t;

typedef struct am_ll_lib_type {
  struct am_ll_lib_type *next;
  struct am_ll_node_type *am;
} am_ll_lib_t;

//----

// current state of the gerber interpreter
//
typedef struct gerber_state_type {

  int g_state;
  int d_state;
  int region;

  int unit_init, pos_init;
  int is_init;

  int quadrent_mode;

  char *units_str[2];
  int units_metric;  // 1 - metric, 0 - inches

  int polarity, polarity_bit;
  int eof;
  int line_no;

  int fs_init;

  char *fs_omit_zero[2];
  int fs_omit_leading_zero;
  char *fs_coord_value[2];
  int fs_coord_absolute;

  int fs_x_int, fs_x_real;
  int fs_y_int, fs_y_real;
  int fs_i_int, fs_i_real;
  int fs_j_int, fs_j_real;

  double cur_x, cur_y;
  double cur_i, cur_j;
  int current_aperture;


  aperture_data_t *aperture_head, *aperture_cur;

  contour_ll_t *contour_head, *contour_cur;
  contour_list_ll_t *contour_list_head, *contour_list_cur;

  am_ll_lib_t *am_lib_head;
  am_ll_lib_t *am_lib_tail;

  string_ll_t string_ll_buf;
  int gerber_read_state;

  // Root should only need to use ab_active to indicate
  // we're in an AB block that still needs processing.
  // 0 - out of AB block
  // 1 - processing AB block
  //
  int ab_active;

  // AB child depth. 0 for root.
  //
  int ab_lib_depth;

  // Global parent/root gerber_state_t
  //
  struct gerber_state_type *ab_lib_root_gs;

  // Current active AB lib child (only valid for root gerber_state_t)
  //
  struct gerber_state_type *ab_lib_active_gs;

  // Parent gerber_state_t data.
  // NULL iff root
  //
  struct gerber_state_type *ab_lib_parent_gs;

  // Name of AB block.
  // Note: children can and will have different names than
  //   their parent
  //
  //int ab_name;

  // AB child depth. 0 for root.
  //
  //int ab_lib_depth;

  // AB linked list in line with this structure.
  // Should only be used for children.
  // The root node should always have this as NULL
  //
  //struct gerber_state_type *ab_lib_next;

  // AB pointer to parent (parent's 'child' structure points to this).
  // NULL for root.
  //
  //struct gerber_state_type *ab_lib_parent;

  // AB linked list of children
  //
  //struct gerber_state_type *ab_lib_child_head;
  //struct gerber_state_type *ab_lib_child_last;

  // root node will have a pointer to the current AB data structure
  //
  //struct gerber_state_type *ab_lib_child_cur;

  // All children AB linked lists have a pointer back to
  // root for ease of access.
  // By convention, the root node has thispoint to itself.
  //
  //struct gerber_state_type *ab_lib_root;

} gerber_state_t;

void gerber_state_init(gerber_state_t *);
void gerber_state_clear(gerber_state_t *);
void gerber_report_state(gerber_state_t *);
void gerber_report_ab_state(gerber_state_t *);

void gerber_state_set_units(gerber_state_t *gs, int units);
extern int (*function_code_handler[13])(gerber_state_t *, char *);
int default_function_code_handler(gerber_state_t *gs, char *buf);
void parse_fs(gerber_state_t *gs, char *linebuf);
void parse_in(gerber_state_t *gs, char *linebuf);
void parse_ip(gerber_state_t *gs, char *linebuf);
void parse_mo(gerber_state_t *gs, char *linebuf);
void parse_ad(gerber_state_t *gs, char *linebuf_orig);
void parse_am(gerber_state_t *gs, char *linebuf);
void parse_ab(gerber_state_t *gs, char *linebuf);
void parse_ln(gerber_state_t *gs, char *linebuf);
void parse_lp(gerber_state_t *gs, char *linebuf);
void parse_sr(gerber_state_t *gs, char *linebuf);
void parse_d01(gerber_state_t *gs, char *linebuf) ;
void parse_d02(gerber_state_t *gs, char *linebuf) ;
void parse_d03(gerber_state_t *gs, char *linebuf) ;
void parse_d10p(gerber_state_t *gs, char *linebuf) ;
char *parse_single_coord(gerber_state_t *gs, double *val, int fs_int, int fs_real, char *s);
char *parse_single_int(gerber_state_t *gs, int *val, char *s);
void add_segment(gerber_state_t *gs, double prev_x, double prev_y, double cur_x, double cur_y, int aperture_name);
void add_flash(gerber_state_t *gs, double cur_x, double cur_y, int aperture_name);
void parse_data_block(gerber_state_t *gs, char *linebuf);
char *parse_d_state(gerber_state_t *gs, char *s);
void parse_g01(gerber_state_t *gs, char *linebuf_orig) ;
void parse_g02(gerber_state_t *gs, char *linebuf) ;
void parse_g03(gerber_state_t *gs, char *linebuf) ;
void parse_g04(gerber_state_t *gs, char *linebuf) ;
void parse_g36(gerber_state_t *gs, char *linebuf) ;
void parse_g37(gerber_state_t *gs, char *linebuf) ;
void parse_g74(gerber_state_t *gs, char *linebuf) ;
void parse_g75(gerber_state_t *gs, char *linebuf) ;
void parse_m02(gerber_state_t *gs, char *linebuf) ;
void parse_g54(gerber_state_t *gs, char *linebuf) ;
void parse_g55(gerber_state_t *gs, char *linebuf) ;
void parse_g70(gerber_state_t *gs, char *linebuf) ;
void parse_g71(gerber_state_t *gs, char *linebuf) ;
void parse_g90(gerber_state_t *gs, char *linebuf) ;
void parse_g91(gerber_state_t *gs, char *linebuf) ;
void parse_m00(gerber_state_t *gs, char *linebuf) ;
void parse_m01(gerber_state_t *gs, char *linebuf) ;
void dump_information(gerber_state_t *gs);
int gerber_state_post_process(gerber_state_t *gs);
int gerber_state_interpret_line(gerber_state_t *gs, char *linebuf);

int gerber_state_load_file(gerber_state_t *gs, char *fn);

void gerber_report_state(gerber_state_t *gs);

int eval_ad_func(gerber_state_t *gs, aperture_data_t *ap_d);

//gerber_state_t *gerber_state_add_AB_child(gerber_state_t *gs_parent, int ab_name);
aperture_data_t *aperture_data_create_ab_node(int ab_name, gerber_state_t *gs_parent);

#endif
