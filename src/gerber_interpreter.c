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

int (*function_code_handler[13])(gerber_state_t *, char *);



//------------------------

void gerber_state_init(gerber_state_t *gs)
{
  gs->g_state = -1;
  gs->d_state = -1;
  gs->region = 0;

  gs->unit_init = 0;
  gs->pos_init = 0;
  gs->is_init = 0;

  gs->units = -1;
  gs->polarity = -1;
  gs->eof = 0;
  gs->line_no = 0;

  gs->current_aperture = -1;

  gs->units_str[0] = strdup("mm");
  gs->units_str[1] = strdup("inches");

  gs->fs_omit_zero[0] = strdup("omit trailing");
  gs->fs_omit_zero[1] = strdup("omit leading");

  gs->fs_coord_value[0] = strdup("relative");
  gs->fs_coord_value[1] = strdup("absolute");

  gs->fs_init = 0;

  gs->aperture_head = gs->aperture_cur = NULL;

  gs->cur_x = 0.0; 
  gs->cur_y = 0.0;

  gs->contour_head = gs->contour_cur = NULL;
  gs->contour_list_head = gs->contour_list_cur = NULL;
}

//------------------------

void gerber_state_clear(gerber_state_t *gs)
{
  contour_list_ll_t *contour_list_nod, *prev_contour_list_nod;
  contour_ll_t *contour_nod, *prev_contour_nod;
  aperture_data_block_t *aperture_nod, *prev_aperture_nod;

  if (gs->units_str[0])
    free(gs->units_str[0]);

  if (gs->units_str[1])
    free(gs->units_str[1]);

  if (gs->fs_omit_zero[0])
    free(gs->fs_omit_zero[0]);

  if (gs->fs_omit_zero[1])
    free(gs->fs_omit_zero[1]);

  if (gs->fs_coord_value[0])
    free(gs->fs_coord_value[0]);

  if (gs->fs_coord_value[1])
    free(gs->fs_coord_value[1]);


  contour_list_nod = gs->contour_list_head;
  while (contour_list_nod)
  {

    contour_nod = contour_list_nod->c;
    while (contour_nod)
    {
      prev_contour_nod = contour_nod;
      contour_nod = contour_nod->next;
      free(prev_contour_nod);
    }

    prev_contour_list_nod = contour_list_nod;
    contour_list_nod = contour_list_nod->next;
    free(prev_contour_list_nod);
  }

  aperture_nod = gs->aperture_head;
  while (aperture_nod)
  {

    prev_aperture_nod = aperture_nod;
    aperture_nod = aperture_nod->next;
    free(prev_aperture_nod);
  }
}


//------------------------

void gerber_report_state(gerber_state_t *gs)
{
  int i, j, k;

  aperture_data_block_t *ap;

  printf("gerber state:\n");
  printf("  g_state: %i\n", gs->g_state);
  printf("  d_state: %i\n", gs->d_state);
  printf("  unit_init: %i\n", gs->unit_init);
  printf("  pos_init: %i\n", gs->pos_init);
  printf("  fs_init: %i\n", gs->fs_init);
  printf("  is_init: %i\n", gs->is_init);

  printf("  units: %i (%s)\n", gs->units, gs->units_str[gs->units]);
  printf("  polarity: %i\n", gs->polarity);
  printf("  eof: %i\n", gs->eof);

  printf("  fs_omit_leading_zero: %i (%s)\n", gs->fs_omit_leading_zero, gs->fs_omit_zero[ gs->fs_omit_leading_zero]);
  printf("  fs_coord_absolute: %i (%s)\n", gs->fs_coord_absolute, gs->fs_coord_value[ gs->fs_coord_absolute ]);
  printf("  fs data block format: X%i.%i, Y%i.%i\n", gs->fs_x_int, gs->fs_x_real, gs->fs_y_int, gs->fs_y_real);

  printf("  aperture:\n");
  ap = gs->aperture_head;
  while (ap)
  {
    printf("    name: %i\n", ap->name);
    printf("    type: %i\n", ap->type);
    printf("    crop_type: %i\n", ap->crop_type);
    printf("    data:");
    k = 0;
    if (ap->type == 0)
      k = ap->crop_type + 1;
    else if (ap->type == 1)
      k = ap->crop_type + 2;
    else if (ap->type == 2)
      k = ap->crop_type + 2;
    else if (ap->type == 3)
      k = ap->crop_type + 2;

    for (i=0; i<k; i++)
      printf(" %g", ap->crop[i]);
    printf("\n");
    printf("    next: %p\n", ap->next);
    printf("\n");
    ap = ap->next;
  }


  printf("  current_aperture: %i\n", gs->current_aperture);
  printf("  x %g, y %g\n", gs->cur_x, gs->cur_y);
}

//------------------------

#define GERBER_STATE_INCHES 1
#define GERBER_STATE_MM 2

void gerber_state_set_units(gerber_state_t *gs, int units)
{
  gs->units = units;
  gs->unit_init = 1;
}

//------------------------

int parse_nums(double *rop, char *s, char sep, char term, int max_rop)
{
  int k;
  char *chp, ch;
  chp = s;

  for (k=0; k<max_rop; k++)
  {

    //printf(" k%i\n", k);

    while ( (*chp) && (*chp != sep) && (*chp != term) )
      chp++;
    if (!(*chp)) return -1;

    //printf("    chp %p (%c) pos %i\n", chp, *chp, (int)(chp-s) );

    ch = *chp;
    *chp = '\0';

    rop[k] = atof(s);

    *chp = ch;

    if (ch == term) return k+1;
    chp++;
    s = chp;
  }

  if (ch != term) return -1;
  return max_rop;

}

//------------------------

void parse_error(char *s, int line_no, char *l)
{
  if (l)
    printf("PARSE ERROR: %s at line %i '%s'\n", s, line_no, l);
  else
    printf("PARSE ERROR: %s at line %i\n", s, line_no, l);
  exit(2);
}

//------------------------

char *skip_whitespace(char *s)
{
  while ((*s) && 
         ( (*s == '\n') || (*s == ' ') || (*s == '\t') ) )
    s++;
  return s;
}

//------------------------

void string_cleanup(char *rop, char *op)
{
  if (!rop) return;
  if (!op) return;
  while (*op)
  {
    if (*op != '\n')
    {
      *rop = *op;
      rop++;
    }
    op++;
  }
  *rop = '\0';
}

//------------------------

void strip_whitespace(char *rop, char *op)
{
  if (!rop) return;
  if (!op) return;
  while (*op)
  {
    if (!isspace(*op))
    {
      *rop = *op;
      rop++;
    }
    op++;
  }
  *rop = '\0';
}

//---

/*
char function_code[][4] = { "D01", "D02", "D03", 
                            "G01", "G02", "G03", "G04", 
                            "G36", "G37",
                            "G74", "G75",
                            "M02", '\0' };

int (*function_code_handler[13])(gerber_state_t *, char *);
*/

#define N_FUNCTION_CODE 13

//int fch_comment(gerbser_state_t *gs, char *buf) { printf("# comment: '%s'\n", buf); }

int default_function_code_handler(gerber_state_t *gs, char *buf)
{
  printf("default_function_code_handler: '%s'\n", buf);
}

int munch_line(char *linebuf, int n, FILE *fp)
{
  char *chp;

  if (!fgets(linebuf, n, fp)) return 0;

  chp = linebuf;
  while ( *linebuf )
  {
    while ( *linebuf && isspace( *linebuf ) )
      linebuf++;
    if (!*linebuf)
      *chp = *linebuf;
    else
      *chp++ = *linebuf++;
  }

  return 1;
}


// function codes

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

int is_function_code_deprecated(int function_code)
{
  if (function_code > FC_M02) return 1;
  return 0;
}

//------------------------

int get_function_code(char *linebuf)
{
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
  IMG_PARAM_LN,       // Level Name
  IMG_PARAM_LP,       // Level Polarity
  IMG_PARAM_SR,       // Step and Repeat
 

  // Depricated Parameters
  IMG_PARAM_AS,       // Axis Select
  IMG_PARAM_MI,       // Mirror Image
  IMG_PARAM_OF,       // OFfset 
  IMG_PARAM_IR,       // Image Rotation
  IMG_PARAM_SF        // Scale Factor
} IMG_PARAM;

int get_image_parameter_code(char *linebuf)
{
  char *pos = linebuf + 1;
  if ( pos[0] && (pos[0] == 'F') && (pos[1] == 'S') ) return IMG_PARAM_FS;
  if ( pos[0] && (pos[0] == 'I') && (pos[1] == 'N') ) return IMG_PARAM_IN;
  if ( pos[0] && (pos[0] == 'I') && (pos[1] == 'P') ) return IMG_PARAM_IP;
  if ( pos[0] && (pos[0] == 'M') && (pos[1] == 'O') ) return IMG_PARAM_MO;
  if ( pos[0] && (pos[0] == 'A') && (pos[1] == 'D') ) return IMG_PARAM_AD;
  if ( pos[0] && (pos[0] == 'A') && (pos[1] == 'M') ) return IMG_PARAM_AM;
  if ( pos[0] && (pos[0] == 'L') && (pos[1] == 'N') ) return IMG_PARAM_LN;
  if ( pos[0] && (pos[0] == 'L') && (pos[1] == 'P') ) return IMG_PARAM_LP;
  if ( pos[0] && (pos[0] == 'S') && (pos[1] == 'R') ) return IMG_PARAM_SR;

  if ( pos[0] && (pos[0] == 'A') && (pos[1] == 'S') ) return IMG_PARAM_SR;
  if ( pos[0] && (pos[0] == 'M') && (pos[1] == 'I') ) return IMG_PARAM_SR;
  if ( pos[0] && (pos[0] == 'O') && (pos[1] == 'F') ) return IMG_PARAM_SR;
  if ( pos[0] && (pos[0] == 'I') && (pos[1] == 'R') ) return IMG_PARAM_SR;
  if ( pos[0] && (pos[0] == 'S') && (pos[1] == 'F') ) return IMG_PARAM_SR;
  return -1;
}

//------------------------

void parse_fs(gerber_state_t *gs, char *linebuf)
{
  char *chp;
  int a, b;

  //printf("# fs '%s'\n", linebuf);

  chp = linebuf + 3;
  if ((*chp != 'L') && (*chp != 'T')) parse_error("bad FS zero omission mode", gs->line_no, linebuf);
  if (*chp == 'L') gs->fs_omit_leading_zero = 1;
  else gs->fs_omit_leading_zero = 0;
  //gs->fs_omit_zero = *chp++;
  chp++;

  if ((*chp != 'A') && (*chp != 'I')) parse_error("bad FS coordinate value mode", gs->line_no, linebuf);
  if (*chp == 'A') gs->fs_coord_absolute = 1;
  else gs->fs_coord_absolute = 0;
  //gs->fs_coord_value = *chp++;
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

  gs->fs_init = 1;

  /*
  printf("# fs omit_zero '%c', coord value '%c', x%i.%i, y%i.%i\n",
      gs->fs_omit_zero, gs->fs_coord_value, 
      gs->fs_x_int, gs->fs_x_real,
      gs->fs_y_int, gs->fs_y_real);
      */

}

//------------------------

void parse_in(gerber_state_t *gs, char *linebuf)
{
  //printf("# in '%s'\n", linebuf);
}

//------------------------

void parse_ip(gerber_state_t *gs, char *linebuf)
{
  //printf("# ip '%s'\n", linebuf);
}

//------------------------

void parse_mo(gerber_state_t *gs, char *linebuf)
{
  char *chp;

  //printf("# mo '%s'\n", linebuf);

  chp = linebuf+3;
  if      ( (chp[0] == 'I') && (chp[1] = 'N') ) gs->units = 1;
  else if ( (chp[0] == 'M') && (chp[1] = 'N') ) gs->units = 0;
  else parse_error("bad MO format", gs->line_no, linebuf);

  gs->unit_init = 1;
}

//------------------------

void parse_ad(gerber_state_t *gs, char *linebuf_orig)
{
  char *linebuf;
  char *chp_beg, *chp, *s, ch;
  char aperture_code;
  int d_code;

  aperture_data_block_t *ap_db;

  linebuf = strdup(linebuf_orig);

  //printf("# ad '%s'\n", linebuf);

  chp = linebuf + 3;

  if (*chp != 'D') parse_error("bad AD format", gs->line_no, linebuf);
  chp++;

  chp_beg = chp;

  while (*chp)
  {
    if ( (*chp < '0') || (*chp > '9') ) break;
    chp++;
  }
  if (!(*chp)) parse_error("bad AD format, expected aperture type", gs->line_no, linebuf);

  aperture_code = *chp;
  *chp = '\0';
  d_code = atoi(chp_beg);
  if (d_code < 10) parse_error("bad AD format, D code must be >= 10", gs->line_no, linebuf);

  *chp = aperture_code;
  if (!(*chp)) parse_error("bad AD format, premature eol", gs->line_no, linebuf);
  chp++;
  if (*chp != ',') parse_error("bad AD format, expected ','", gs->line_no, linebuf);
  chp++;

  ap_db = (aperture_data_block_t *)malloc(sizeof(aperture_data_block_t));
  ap_db->next = NULL;
  ap_db->name = d_code;

  switch (aperture_code)
  {
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

  if (ap_db->crop_type < 0)
  {
    //printf(" got %i\n", ap_db->crop_type);
    parse_error("bad AD, bad aperture data", gs->line_no, linebuf);
  }

  if (!gs->aperture_head)
    gs->aperture_head = ap_db;
  else
    gs->aperture_cur->next = ap_db;
  gs->aperture_cur = ap_db;

  free(linebuf);

}


// aperture macro
void parse_am(gerber_state_t *gs, char *linebuf)
{
  //printf("# am '%s'\n", linebuf);
}

//------------------------

void parse_ln(gerber_state_t *gs, char *linebuf)
{
  //printf("# ln '%s'\n", linebuf);
}

//------------------------

void parse_lp(gerber_state_t *gs, char *linebuf)
{
  //printf("# lp '%s'\n", linebuf);
}

//------------------------

void parse_sr(gerber_state_t *gs, char *linebuf)
{
  //printf("# sr '%s'\n", linebuf);
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

char *parse_single_coord(gerber_state_t *gs, double *val, int fs_int, int fs_real, char *s)
{
  int i, j, k;
  char *chp, ch, *tbuf;
  int max_buf, pos=0;
  max_buf = fs_int + fs_real + 2;

  tbuf = (char *)malloc(sizeof(char)*(max_buf));

  s++;
  
  // advance to non-digit portion
  for (chp = s; (*chp && (isdigit(*chp) || (*chp == '-') || (*chp == '+'))) ; chp++);
  if (!*chp) parse_error("unexpected eol", gs->line_no, NULL);
  k = (int)(chp - s);
  if (k >= max_buf) parse_error("invalid coordinate", gs->line_no, NULL);

  if (gs->fs_omit_leading_zero)  // that is, trailing zeros manditory
  {
    tbuf[max_buf-1] = '\0';
    pos = max_buf-2;
    for (i=0; i<fs_real; i++)
      tbuf[pos--] = chp[-i-1];

    tbuf[pos--] = '.';
    chp -= fs_real+1;

    while (chp >= s)
    {
      tbuf[pos--] = *chp--;
      if (pos < 0) parse_error("coordinate format exceeded", gs->line_no, NULL);
    }

    while (pos>=0) tbuf[pos--] = ' ';
  }
  else
  {
    chp = s;
    if (*chp == '+') chp++;
    if (*chp == '-') tbuf[pos++] = '-';

    for (i=0; i<fs_int; i++)
    {
      if (!isdigit(*chp)) parse_error("expected number", gs->line_no, NULL);
      tbuf[pos++] = *chp++;
    }
    tbuf[pos++] = '.';

    while (*chp && (isdigit(*chp)))
    {
      tbuf[pos++] = *chp++;
      if (pos >= (max_buf-1)) parse_error("coordinate format exceeded", gs->line_no, NULL);
    }
    tbuf[pos] = '\0';
  }

  errno = 0;
  *val = strtod(tbuf, NULL);
  if ((*val == 0.0) && errno)
    parse_error("bad conversion", gs->line_no, NULL);

  free(tbuf);
  return s + k;
}

//------------------------

char *parse_single_int(gerber_state_t *gs, int *val, char *s)
{
  int i, j, k;
  char *chp, ch, *tbuf;;
  double d;

  //if (*s != tok) parse_error("expected token", line_no, NULL);
  s++;
  
  for (chp = s;  (*chp && (isdigit(*chp) || (*chp == '-') || (*chp == '+'))) ; chp++);
  if (!*chp) parse_error("unexpected eol", gs->line_no, NULL);

  ch = *chp;
  *chp = '\0';

  tbuf = strdup(s);
  *chp = ch;

  errno = 0;
  *val = atoi(tbuf);
  if (*val <= 0)
    parse_error("bad conversion", gs->line_no, NULL);
  free(tbuf);

  return chp;
}

//------------------------

void add_segment(gerber_state_t *gs, double prev_x, double prev_y, double cur_x, double cur_y, int aperture_name)
{
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

  if (gs->contour_list_head == NULL)
    gs->contour_list_head = contour_list_nod;
  else
    gs->contour_list_cur->next = contour_list_nod;
  gs->contour_list_cur = contour_list_nod;

}

//------------------------

void add_flash(gerber_state_t *gs, double cur_x, double cur_y, int aperture_name)
{
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

  if (gs->contour_list_head == NULL)
    gs->contour_list_head = contour_list_nod;
  else
    gs->contour_list_cur->next = contour_list_nod;
  gs->contour_list_cur = contour_list_nod;

}

//------------------------

void parse_data_block(gerber_state_t *gs, char *linebuf)
{
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

  while (*chp)
  {
    if (*chp == 'X')
    {
      if (state & 1) parse_error("multiple 'X' tokens found", gs->line_no, NULL);
      state |= 1;
      chp = parse_single_coord(gs, &(gs->cur_x), gs->fs_x_int, gs->fs_x_real, chp);
    }

    else if (*chp == 'Y')
    {
      if (state & 2) parse_error("multiple 'Y' tokens found", gs->line_no, NULL);
      state |= 2;
      chp = parse_single_coord(gs, &(gs->cur_y), gs->fs_y_int, gs->fs_y_real, chp);
    }

    else if (*chp == 'D')
    {
      if (state & 4) parse_error("multiple 'D' tokens found", gs->line_no, NULL);
      state |= 4;
      chp = parse_single_int(gs, &(gs->d_state), chp);
    }

    else if (*chp == '*')
      break;
    else
      parse_error("unknown token when parsing data block", gs->line_no, NULL);

  }

  // handle region
  if (gs->region)
  {

    if (gs->d_state == 1)
    {
      if (gs->contour_head == NULL)
      {
        contour_nod = (contour_ll_t *)malloc(sizeof(contour_ll_t));
        contour_nod->d_name = gs->current_aperture;
        contour_nod->x = prev_x;
        contour_nod->y = prev_y;
        contour_nod->region = 1;
        contour_nod->next  = NULL;

        gs->contour_head = gs->contour_cur = contour_nod;

        //printf("%g %g # %i\n", prev_x, prev_y, gs->d_state);

      }

      contour_nod = (contour_ll_t *)malloc(sizeof(contour_ll_t));
      contour_nod->d_name = gs->current_aperture;
      contour_nod->x = gs->cur_x;
      contour_nod->y = gs->cur_y;
      contour_nod->region = 1;
      contour_nod->next = NULL;

      gs->contour_cur->next = contour_nod;
      gs->contour_cur = contour_nod;

      //printf("%g %g # %i \n", gs->cur_x, gs->cur_y, gs->d_state);
    }
    else  // need to tie off region
    {

      if (gs->contour_head)
      {
        contour_list_nod = (contour_list_ll_t *)malloc(sizeof(contour_list_ll_t));
        contour_list_nod->n = 1;
        contour_list_nod->next = NULL;
        contour_list_nod->c = gs->contour_head;

        if (!gs->contour_list_head)
          gs->contour_list_head = contour_list_nod;
        else
          gs->contour_list_cur->next = contour_list_nod;
        gs->contour_list_cur = contour_list_nod;

        gs->contour_head = NULL;
        gs->contour_cur= NULL;
      }

    }
      
  }
  else
  {

    if (gs->d_state == 1)
    {
      add_segment(gs, prev_x, prev_y, gs->cur_x, gs->cur_y, gs->current_aperture);
    }

  }

  if (gs->d_state == 2)
  {
    //printf("# %g %g  (%i)\n", gs->cur_x, gs->cur_y, gs->d_state);
  }

  if (gs->d_state == 3)
  {
    add_flash(gs, gs->cur_x, gs->cur_y, gs->current_aperture);
  }


}


//------------------------

char *parse_d_state(gerber_state_t *gs, char *s)
{
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

void parse_g02(gerber_state_t *gs, char *linebuf) { 
  parse_error("unsuported g02", gs->line_no, NULL);
}

//------------------------

void parse_g03(gerber_state_t *gs, char *linebuf) { 
  parse_error("unsuported g03", gs->line_no, NULL);
}

//------------------------

// coment
void parse_g04(gerber_state_t *gs, char *linebuf) { 
  //parse_error("unsuported g04", line_no, NULL);
}

//------------------------

// start region
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
  if (gs->contour_head)
  {
    nod = (contour_list_ll_t *)malloc(sizeof(contour_list_ll_t));
    nod->next = NULL;
    nod->c = gs->contour_head;
    nod->n = 1;

    if (!gs->contour_list_head)
      gs->contour_list_head = nod;
    else
      gs->contour_list_cur->next = nod;
    gs->contour_list_cur = nod;

    gs->contour_head = NULL;
    gs->contour_cur = NULL;
  }

}

//------------------------

void parse_g74(gerber_state_t *gs, char *linebuf) { 
  parse_error("unsuported g74", gs->line_no, NULL);
}

//------------------------

void parse_g75(gerber_state_t *gs, char *linebuf) { 
  parse_error("unsuported g75", gs->line_no, NULL);
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
void parse_g70(gerber_state_t *gs, char *linebuf) { 
  gs->units = 1;
}

//------------------------

// deprecated untis to mm
void parse_g71(gerber_state_t *gs, char *linebuf) { 
  gs->units = 0;
}

//------------------------

// deprecated absolute coordinate
void parse_g90(gerber_state_t *gs, char *linebuf) { 
  gs->fs_coord_absolute = 1;
}

//------------------------

// deprecated incremental coordinate
void parse_g91(gerber_state_t *gs, char *linebuf) { 
  gs->fs_coord_absolute = 0;
}

//------------------------

// deprecated program stop
void parse_m00(gerber_state_t *gs, char *linebuf) { 
  gs->eof = 1;
}

//------------------------

// deprecated optional stop
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

void dump_information(gerber_state_t *gs)
{
  int i, j, k;
  int n=0;

  int verbose_print = 0;

  int cur_contour_list = 0;

  aperture_data_block_t *adb;
  contour_list_ll_t *cl;
  contour_ll_t *c;

  if (verbose_print)
    printf("# aperture list:\n");

  adb = gs->aperture_head;
  k=0;
  while (adb)
  {

    if (verbose_print)
      printf("#  [%i] name: %i, type: %i, crop_type %i, ", k, adb->name, adb->type, adb->crop_type);

    if (adb->type == APERTURE_CIRCLE)
    {
      if (verbose_print)
        printf("circle:");

      n = adb->crop_type + 1;
    }
    else if (adb->type == APERTURE_RECTANGLE)
    {
      if (verbose_print)
        printf("rectangle:");

      n = adb->crop_type + 2;
    }
    else if (adb->type == APERTURE_OBROUND)
    {
      if (verbose_print)
        printf("obround:");

      n = adb->crop_type + 2;
    }
    else if (adb->type == APERTURE_POLYGON)
    {
      if (verbose_print)
        printf("polygon:");

      n = adb->crop_type + 3;
    }

    if (verbose_print)
    {
      for (i=0; i<n; i++) printf(" %g", adb->crop[i]);
      printf("\n");
    }

    adb = adb->next;
    k++;
  }

  cur_contour_list = 0;
  cl = gs->contour_list_head;
  while (cl)
  {

    if (verbose_print)
      printf("# [%i] contour_list\n", cur_contour_list);

    k=0;
    c = cl->c;

    while (c)
    {
      if (verbose_print)
      {
        printf("#  [%i] contour d_name: %i, region: %i, x,y: (%g,%g)\n", k, c->d_name, c->region, c->x, c->y);
        printf("%g %g\n", c->x, c->y);
      }
      c = c->next;
      k++;
    }

    cl = cl->next;
    cur_contour_list++;

    if (verbose_print)
      printf("\n");
  }


}

//-------------------------------

// do any post processing that needs to be done
int gerber_state_post_process(gerber_state_t *gs)
{
}

//-------------------------------

int gerber_state_interpret_line(gerber_state_t *gs, char *linebuf)
{
  int image_parameter_code;
  int function_code;

  if ( linebuf[0] == '%' )  // it's an image parameter
  {

    image_parameter_code = get_image_parameter_code(linebuf);

    switch (image_parameter_code)
    {
      case IMG_PARAM_FS: parse_fs(gs, linebuf); break;
      case IMG_PARAM_IN: parse_in(gs, linebuf); break;
      case IMG_PARAM_IP: parse_ip(gs, linebuf); break;
      case IMG_PARAM_MO: parse_mo(gs, linebuf); break;
      case IMG_PARAM_AD: parse_ad(gs, linebuf); break;
      case IMG_PARAM_AM: parse_am(gs, linebuf); break;
      case IMG_PARAM_LN: parse_ln(gs, linebuf); break;
      case IMG_PARAM_LP: parse_lp(gs, linebuf); break;
      case IMG_PARAM_SR: parse_sr(gs, linebuf); break;
      default:
        parse_error("unknown image parameter code", gs->line_no, linebuf);
        break;
    }

  }
  else
  {

    function_code = get_function_code(linebuf);

    //if (is_function_code_depricated(function_code)) continue;

    switch (function_code)
    {
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

}

//------------------------

#define GERBER_STATE_LINEBUF 4099


int gerber_state_load_file(gerber_state_t *gs, char *fn)
{
  FILE *fp;
  char linebuf[GERBER_STATE_LINEBUF];

  if (strncmp(fn, "-", 2) == 0) 
  {
    fp = stdin;
  } 
  else if ( !(fp = fopen(fn, "r")) )  
  {
    return -1;
  }

  gs->line_no = 0;
  while (!feof(fp))
  {
    if (!munch_line(linebuf, GERBER_STATE_LINEBUF, fp)) 
      continue;
    gs->line_no++;

    gerber_state_interpret_line(gs, linebuf);
  }

  if (fp != stdin)
  {
    fclose(fp);
  }

  gerber_state_post_process(gs);

  return 0;

}

