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

#ifdef GERBER_TEST

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gerber_interpreter.h"

#define N_LINEBUF 4099

static void _pws(FILE *fp, int n) {
  int i;
  for (i=0; i<n; i++) { fprintf(fp, " "); }
}

char *lookup_type_name(int type) {
  static int init=0;
  static char buf[32];

  if (!init) {
    init=1;
    strncpy(buf, "error", 5);
  }

  switch (type) {
    case GERBER_NONE: strncpy(buf, "none", 1+strlen("none")); break;
    case GERBER_MO: strncpy(buf, "mo", 1+strlen("mo")); break;
    case GERBER_D1: strncpy(buf, "d1", 1+strlen("d1")); break;
    case GERBER_D2: strncpy(buf, "d2", 1+strlen("s2")); break;
    case GERBER_D3: strncpy(buf, "d3", 1+strlen("d3")); break;
    case GERBER_D10P: strncpy(buf, "d10p", 1+strlen("d10p")); break;
    case GERBER_AD: strncpy(buf, "ad", 1+strlen("ad")); break;
    case GERBER_ADE: strncpy(buf, "ade", 1+strlen("ade")); break;
    case GERBER_G36: strncpy(buf, "g36", 1+strlen("g36")); break;
    case GERBER_AM: strncpy(buf, "am", 1+strlen("am")); break;
    case GERBER_G37: strncpy(buf, "g37", 1+strlen("g37")); break;
    case GERBER_AB: strncpy(buf, "ab", 1+strlen("ab")); break;
    case GERBER_SR: strncpy(buf, "sr", 1+strlen("sr")); break;
    case GERBER_LP: strncpy(buf, "lp", 1+strlen("lp")); break;
    case GERBER_LM: strncpy(buf, "lm", 1+strlen("lm")); break;
    case GERBER_LR: strncpy(buf, "lr", 1+strlen("lr")); break;
    case GERBER_LS: strncpy(buf, "ls", 1+strlen("ls")); break;
    case GERBER_G74: strncpy(buf, "g74", 1+strlen("g74")); break;
    case GERBER_G75: strncpy(buf, "g75", 1+strlen("g75")); break;
    case GERBER_SEGMENT: strncpy(buf, "segment", 1+strlen("segment")); break;
    case GERBER_REGION: strncpy(buf, "region", 1+strlen("region")); break;
    default:
      strncpy(buf, "xxx", 1+strlen("xxx"));
      break;
  }

  return buf;
}

void _print_mo(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item) {
  fprintf(fp, "%%MO%s*%%\n", (item->units_metric ? "MM" : "IN"));
}

void _print_d10p(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item) {
  fprintf(fp, "%%ADD%i ... *%%\n", item->d_name);
}

void _print_am(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item) {
  int i, initial_comma=1;
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

  am_ll_lib_t *am;
  am_ll_node_t *am_nod;

  am = item->aperture_macro;
  while (am) {

    am_nod = am->am;
    while (am_nod) {

      if (am_nod->type == AM_ENUM_NAME) {
        fprintf(fp, "%%AM%s*\n", am_nod->name);
      }
      else if (am_nod->type == AM_ENUM_COMMENT) {
      }
      else {

        initial_comma=1;

        if (am_nod->type == AM_ENUM_VAR) { fprintf(fp, "%s=", am_nod->varname); initial_comma=0; }
        else if (am_nod->type == AM_ENUM_CIRCLE) { fprintf(fp, "1"); }
        else if (am_nod->type == AM_ENUM_VECTOR_LINE) { fprintf(fp, "20"); }
        else if (am_nod->type == AM_ENUM_CENTER_LINE) { fprintf(fp, "21"); }
        else if (am_nod->type == AM_ENUM_OUTLINE) { fprintf(fp, "4"); }
        else if (am_nod->type == AM_ENUM_POLYGON) { fprintf(fp, "5"); }
        else if (am_nod->type == AM_ENUM_MOIRE) { fprintf(fp, "6"); }
        else if (am_nod->type == AM_ENUM_THERMAL) { fprintf(fp, "7"); }

        for (i=0; i<am_nod->n_eval_line; i++) {
          if (initial_comma || (i>0)) { fprintf(fp, ","); }
          fprintf(fp, "%s", am_nod->eval_line[i]);
        }
        fprintf(fp, "*\n");
      }

      am_nod = am_nod->next;

    }

    fprintf(fp, "%%\n");
    fprintf(fp, "\n");

    //--
    break;
    am = am->next;
  }
}

void _print_ade(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item) {
  int i;
  char crop[5] = "CROP";
  //int crop_n[4] = { 3, 4, 4, 5 };
  int crop_n[3][4] = { {1, 2, 2, 3}, {2,3,3,4},{ 3, 4, 4, 5 } };
  aperture_data_t *ade;

  ade = item->aperture;
  while (ade) {

    //fprintf(fp, "# ade name:%i, type:%i, crop_type:%i, #param:%i\n",
    //    ade->name, ade->type, ade->crop_type, ade->macro_param_count);

    if ( (ade->type == AD_ENUM_CIRCLE) ||
         (ade->type == AD_ENUM_RECTANGLE) ||
         (ade->type == AD_ENUM_OBROUND) ||
         (ade->type == AD_ENUM_POLYGON) ) {

      fprintf(fp, "%%ADD%i%c,",
          ade->name, crop[ ade->type - AD_ENUM_CIRCLE ]);
      for (i=0; i<crop_n[ade->crop_type][ ade->type - AD_ENUM_CIRCLE ]; i++) {
        if (i>0) { fprintf(fp, "X"); }
        fprintf(fp, "%0.4f", (float)ade->crop[i]);
      }
      fprintf(fp, "*%%\n");

    }
    else if (ade->type == AD_ENUM_MACRO) {

      fprintf(fp, "%%ADD%i%s,", ade->name, ade->macro_name);
      for (i=0; i<ade->macro_param_count; i++) {
        if (i>0) { fprintf(fp, "X"); }
        fprintf(fp, "%0.4f", (float)ade->macro_param[i]);
      }

      fprintf(fp, "*%%\n");
    }

    //--
    break;
    ade = ade->next;
  }
}

void _print_ad(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item) {
  int i;
  char crop[5] = "CROP";
  int crop_n[3][4] = { {1, 2, 2, 3}, {2,3,3,4},{ 3, 4, 4, 5 } };
  aperture_data_t *ad;

  ad = item->aperture;
  while (ad) {

    fprintf(fp, "%%add%i%c,",
        ad->name, crop[ ad->type - AD_ENUM_CIRCLE ]);
    for (i=0; i<crop_n[ad->crop_type][ ad->type - AD_ENUM_CIRCLE ]; i++) {
      if (i>0) { fprintf(fp, "X"); }
      fprintf(fp, "%0.4f", (float)ad->crop[i]);
    }
    fprintf(fp, "*%%\n");

    //--
    break;
    ad = ad->next;
  }
}

void _print_lp(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item) {
  fprintf(fp, "%%LP%c*%%\n",
      (item->polarity ? 'D' : 'C'));
}

void _print_gerber_state_r(FILE *fp, gerber_state_t *gs, int lvl) {
  int n=0, m=0;
  double C = 1000000.0;

  gerber_item_ll_t *item_nod;
  gerber_region_t *region_nod;

  item_nod = gs->item_head;

  while (item_nod) {
    fprintf(fp, "\n");

    _pws(fp, lvl);
    fprintf(fp, "[%i] %s (%i)\n",
        n,
        lookup_type_name(item_nod->type),
        item_nod->type);

    switch (item_nod->type) {
      case GERBER_MO: _print_mo(fp, gs, item_nod); break;
      case GERBER_LP: _print_lp(fp, gs, item_nod); break;
      case GERBER_AM: _print_am(fp, gs, item_nod); break;
      case GERBER_AD: _print_ad(fp, gs, item_nod); break;
      case GERBER_ADE: _print_ade(fp, gs, item_nod); break;
      default: break;
    }

    if (item_nod->region) {
      region_nod = item_nod->region_head;
      m=0;

      fprintf(fp, "G36*\n");

      //fprintf(fp, "X%iY%iD02*\n", (int)(item_nod->x*C), (int)(item_nod->y*C));

      while (region_nod) {

        //_pws(fp,lvl); fprintf(fp, ".[%i] %f %f\n", m, (float)region_nod->x, (float)region_nod->y);

        if (m==0) {
          fprintf(fp, "X%iY%iD02*\n", (int)(region_nod->x * C), (int)(region_nod->y * C));
        } else if (m==1) {
          fprintf(fp, "X%iY%iD01*\n", (int)(region_nod->x * C), (int)(region_nod->y * C));
        } else {
          fprintf(fp, "X%iY%i*\n", (int)(region_nod->x * C), (int)(region_nod->y * C));
        }


        region_nod = region_nod->next;
        m++;
      }

      fprintf(fp, "G37*\n");

    }

    item_nod = item_nod->next;
    n++;
  }

}

void print_gerber_state(gerber_state_t *gs) {
  _print_gerber_state_r(stdout, gs, 0);
}

//------------------------

int main(int argc, char **argv) {
  int i, j, k, n_line;
  char *chp, *ts;
  FILE *fp;

  int image_parameter_code;
  int function_code;

  gerber_state_t gs;

  gerber_state_init(&gs);

  if (argc!=2) { printf("provide filename\n"); exit(0); }

  k = gerber_state_load_file(&gs, argv[1]);
  if (k<0) {
    perror(argv[1]);
  }

  print_gerber_state(&gs);

  //gerber_report_state(&gs);
}

#endif
