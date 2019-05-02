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

#include <math.h>

//#include "tesexpr.h"
#include "string_ll.h"

//#define GERBER_STATE_LINEBUF 4099
#define GERBER_STATE_LINEBUF 65537

struct gerber_item_ll_type;

// consider renaming this to GERBER_ITEM or GERBER_AST or something
//
enum GERBER_ITEM {
  GERBER_NONE = 0,

  GERBER_MO,
  GERBER_D1,
  GERBER_D2,
  GERBER_D3,
  GERBER_D10P,
  GERBER_AD,
  GERBER_ADE,
  GERBER_G36,
  GERBER_AM,

  // 10
  //
  GERBER_G37,

  GERBER_AB,
  GERBER_SR,
  GERBER_LP,
  GERBER_LM,
  GERBER_LR,
  GERBER_LS,

  GERBER_G01,
  GERBER_G02,
  GERBER_G03,

  // 20
  //
  GERBER_G74,
  GERBER_G75,


  GERBER_FLASH,
  GERBER_SEGMENT,
  GERBER_SEGMENT_ARC,
  GERBER_REGION,

  GERBER_REGION_D01,
  GERBER_REGION_D02,
  GERBER_REGION_G01,
  GERBER_REGION_G02,
  
  // 30
  //
  GERBER_REGION_G03,
  GERBER_REGION_G74,
  GERBER_REGION_G75,

  GERBER_REGION_MOVE,
  GERBER_REGION_SEGMENT,
  GERBER_REGION_SEGMENT_ARC,

  GERBER_M02,
};

extern const char *g_gerber_enum_item_str[];

typedef enum {
  QUADRENT_MODE_NONE = 0,
  QUADRENT_MODE_SINGLE,
  QUADRENT_MODE_MULTI,
} quadrent_mode_enum;

enum INTERPOLATION_MODE_ENUM {
  INTERPOLATION_MODE_NONE = 0,
  INTERPOLATION_MODE_LINEAR,
  INTERPOLATION_MODE_CW,
  INTERPOLATION_MODE_CCW,
};

enum MIRROR_AXIS {
  MIRROR_AXIS_NONE = 0,
  MIRROR_AXIS_X,
  MIRROR_AXIS_Y,
  MIRROR_AXIS_XY,
};

/*
typedef struct gerber_region_type {
  double x, y;
  struct gerber_region_type *next;
} gerber_region_t;
*/


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
//
// TODO: consider renaming to gerber_aperture_data_type ...
//
typedef struct aperture_data_type {
  int id;

  int name;
  int type;       // 0 - C, 1 - R, 2 - O, 3 - P, 4 - macro, 5 - block, 6 - step repeat
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

  int x_rep, y_rep;
  double i_distance, j_distance;

  struct aperture_data_type *next;

  // Our 'local' gerber_state_type data structure
  //
  struct gerber_state_type *gs;

} aperture_data_t;

// TODO: consider renaming GERBER_READ_STATE_...
//
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

// TODO: consider renaming to GGERBER_AD_...
//
enum AD_ENUM_TYPE {
  AD_ENUM_CIRCLE = 0,
  AD_ENUM_RECTANGLE,
  AD_ENUM_OBROUND,
  AD_ENUM_POLYGON,
  AD_ENUM_MACRO,
};

// Aperture Macro
//

// TODO: consider renaming to GGERBER_AM_...
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

// TODO: consider renaming to gerber_am_ll_node_type...
//
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
  int id;

  int name;

  int g_state;
  int d_state;
  int region;

  int unit_init, pos_init;
  int is_init;

  int quadrent_mode;
  int interpolation_mode;

  char *units_str[2];
  int units_metric;  // 1 - metric, 0 - inches

  int polarity, polarity_bit;
  int mirror_axis;
  double rotation_degree;
  double scale;

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

  aperture_data_t *aperture_head;
  aperture_data_t *aperture_cur;

  struct gerber_item_ll_type *item_head;
  struct gerber_item_ll_type *item_tail;

  struct gerber_item_ll_type *_item_cur;

  am_ll_lib_t *am_lib_head;
  am_ll_lib_t *am_lib_tail;

  string_ll_t string_ll_buf;
  int gerber_read_state;

  // Root should only need to use absr_active to indicate
  // we're in an AB block that still needs processing.
  // 0 - out of AB or SR block
  // 1 - processing AB or SR block
  //
  int absr_active;

  // flag for whether there's a polarity, mirror
  // rotation or scale parameter set.
  //
  int pmrs_active;

  // AB child depth. 0 for root.
  //
  int depth;

  // Global parent/root gerber_state_t
  //
  struct gerber_state_type *_root_gerber_state;

  // Current active AB lib child (only valid for root gerber_state_t)
  //
  struct gerber_state_type *_active_gerber_state;

  // Parent gerber_state_t data.
  // NULL iff root
  //
  struct gerber_state_type *_parent_gerber_state;

} gerber_state_t;

// a node in the linked list which contains
// x, y information, d_name information, etc.
//
typedef struct gerber_item_ll_type {
  int type;
  int d_name;
  int n;
  double x, y, i, j;
  int region;

  // LP
  //
  int polarity;

  int units_metric;

  // SR (step repeat)
  //
  int sr_x, sr_y;
  double sr_i, sr_j;

  // LS
  //
  double scale;

  // LR
  //
  double rotation_degree;

  // LM
  //
  int mirror_axis;

  // G74, G75
  //
  int quadrent_mode;

  // G01, G02, G03
  //
  int interpolation_mode;

  // D01 I, J
  //
  double arc_center_x, arc_center_y;
  double arc_r, arc_r_deviation;
  double arc_ang_rad_beg, arc_ang_rad_del;

  aperture_data_t *aperture;
  am_ll_lib_t *aperture_macro;

  //gerber_region_t *region_head;
  //gerber_region_t *region_tail;

  struct gerber_item_ll_type *region_head;
  struct gerber_item_ll_type *region_tail;

  gerber_state_t *step_repeat;
  gerber_state_t *aperture_block;

  struct gerber_item_ll_type *next;
} gerber_item_ll_t;



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
void parse_lm(gerber_state_t *gs, char *linebuf);
void parse_lr(gerber_state_t *gs, char *linebuf);
void parse_ls(gerber_state_t *gs, char *linebuf);

void parse_sr(gerber_state_t *gs, char *linebuf);
void parse_d01(gerber_state_t *gs, char *linebuf) ;
void parse_d02(gerber_state_t *gs, char *linebuf) ;
void parse_d03(gerber_state_t *gs, char *linebuf) ;
void parse_d10p(gerber_state_t *gs, char *linebuf) ;
char *parse_single_coord(gerber_state_t *gs, double *val, int fs_int, int fs_real, char *s);
char *parse_single_int(gerber_state_t *gs, int *val, char *s);
void add_segment(gerber_state_t *gs, double prev_x, double prev_y, double cur_x, double cur_y, double cur_i, double cur_j, int aperture_name);
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
void dump_information(gerber_state_t *gs, int level);
int gerber_state_post_process(gerber_state_t *gs);
int gerber_state_interpret_line(gerber_state_t *gs, char *linebuf);

int gerber_state_load_file(gerber_state_t *gs, char *fn);

void gerber_report_state(gerber_state_t *gs);

//int eval_ad_func(gerber_state_t *gs, aperture_data_t *ap_d);

aperture_data_t *aperture_data_create_absr_node(int absr_name, gerber_state_t *gs_parent, int is_ab);

#endif
