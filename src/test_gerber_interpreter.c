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
#include <math.h>

#include "gerber_interpreter.h"

#define N_LINEBUF 4099

void _print_gerber_state_r(FILE *fp, gerber_state_t *gs, int lvl);

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
    case GERBER_FLASH: strncpy(buf, "flash", 1+strlen("flash")); break;
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
  fprintf(fp, "D%i*\n", item->d_name);
}

void _print_am(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item) {
  int i, initial_comma=1, n=0;
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

    n=0;

    am_nod = am->am;
    while (am_nod) {


      if (am_nod->type == AM_ENUM_NAME) {
        fprintf(fp, "%%AM%s*", am_nod->name);
      }
      else if (am_nod->type == AM_ENUM_COMMENT) {
      }
      else {

        if (n>0) { fprintf(fp, "\n"); }

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
        //fprintf(fp, "*\n");
        fprintf(fp, "*");
      }

      am_nod = am_nod->next;

      n++;
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

    fprintf(fp, "%%ADD%i%c,",
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

void _print_lm(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item) {
  char mirror_axis[3];

  mirror_axis[1] = '\0';
  mirror_axis[2] = '\0';

  switch (item->mirror_axis) {
    case MIRROR_AXIS_NONE: mirror_axis[0] = 'N'; break;
    case MIRROR_AXIS_X: mirror_axis[0] = 'X'; break;
    case MIRROR_AXIS_Y: mirror_axis[0] = 'Y'; break;
    case MIRROR_AXIS_XY: mirror_axis[0] = 'X'; mirror_axis[1] = 'Y'; break;
    default: mirror_axis[0] = 'N'; break;
  }

  fprintf(fp, "%%LM%s*%%\n", mirror_axis);
}

void _print_lr(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item) {
  fprintf(fp, "%%LR%f*%%\n", (float)item->rotation_degree);
}

void _print_ls(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item) {
  fprintf(fp, "%%LS%f*%%\n", (float)item->scale);
}

void _print_g01(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item) {
  fprintf(fp, "G01*\n");
}

void _print_g02(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item) {
  fprintf(fp, "G02*\n");
}

void _print_g03(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item) {
  fprintf(fp, "G03*\n");
}

void _print_g74(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item) {
  fprintf(fp, "G74*\n");
}

void _print_g75(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item) {
  fprintf(fp, "G75*\n");
}

void _print_flash(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item_nod) {
  int _ix, _iy;
  double C;

  C = pow(10.0, (double)(gs->fs_x_real));

  _ix = (int)(item_nod->x * C);
  _iy = (int)(item_nod->y * C);

  fprintf(fp, "X%iY%iD03*\n", _ix, _iy);
}

void _print_segment(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item_nod) {
  int n=0, m=0;
  double C = 1.0;

  int _ix, _iy, _ii, _ij;
  int _ixprv, _iyprv;

  C = pow(10.0, (double)(gs->fs_x_real));

  //gerber_region_t *region_nod;
  gerber_item_ll_t *region_nod;

  region_nod = item_nod->region_head;
  m=0;

  while (region_nod) {

    _ix = (int)(region_nod->x * C);
    _iy = (int)(region_nod->y * C);

    if (m==0) {
      fprintf(fp, "X%iY%iD02*\n", _ix, _iy);
    } else if (m==1) {
      fprintf(fp, "X%iY%iD01*\n", _ix, _iy);
    }
    else {
      fprintf(fp, "##???\n");
    }

    _ixprv = _ix;
    _iyprv = _iy;

    region_nod = region_nod->next;
    m++;
  }

}

void _print_segment_arc(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item_nod) {
  int n=0, m=0;
  double C = 1.0;

  double _cx, _cy, _r, _a0, _a1;
  double _X0, _Y0, _X1, _Y1, _I, _J;

  int _ix, _iy, _ii, _ij;
  int _ixprv, _iyprv;

  C = pow(10.0, (double)(gs->fs_x_real));


  _cx = item_nod->arc_center_x;
  _cy = item_nod->arc_center_y;

  _r = item_nod->arc_r;

  _a0 = item_nod->arc_ang_rad_beg;
  _a1 = _a0 + item_nod->arc_ang_rad_del;

  _X0 = _cx + (_r * cos(_a0));
  _Y0 = _cy + (_r * sin(_a0));

  _X1 = _cx + (_r * cos(_a1));
  _Y1 = _cy + (_r * sin(_a1));

  _I = _cx - _X0;
  _J = _cy - _Y0;

  if (item_nod->quadrent_mode == QUADRENT_MODE_SINGLE) {
    _I = fabs(_I);
    _J = fabs(_J);
  }

  fprintf(fp, "X%iY%iD02*\n",
      (int)(_X0*C), (int)(_Y0*C));

  fprintf(fp, "X%iY%iI%iJ%iD01*\n",
      (int)(_X1*C), (int)(_Y1*C),
      (int)(_I*C), (int)(_J*C));

}

void _print_region(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item_nod) {
  int n=0, m=0;
  double C = 1.0;

  int _ix, _iy, _ii, _ij;
  int _ixprv, _iyprv, _iiprv, _ijprv;

  C = pow(10.0, (double)(gs->fs_x_real));

  //gerber_region_t *region_nod;
  gerber_item_ll_t *region_nod;

  region_nod = item_nod->region_head;
  m=0;

  //DEBUG
  fprintf(fp, "\n");
  //DEBUG

  fprintf(fp, "G36*\n");

  for ( ; region_nod ; region_nod = region_nod->next) {
  //while (region_nod) {

    if (region_nod->type == GERBER_REGION_G74) {
      gs->quadrent_mode = QUADRENT_MODE_SINGLE;
      fprintf(fp, "G74*\n");
      continue;
    }
    else if (region_nod->type == GERBER_REGION_G75) {
      gs->quadrent_mode = QUADRENT_MODE_MULTI;
      fprintf(fp, "G75*\n");
      continue;
    }
    else if (region_nod->type == GERBER_REGION_G01) {
      gs->interpolation_mode = INTERPOLATION_MODE_LINEAR;
      fprintf(fp, "G01*\n");
      continue;
    }
    else if (region_nod->type == GERBER_REGION_G02) {
      gs->interpolation_mode = INTERPOLATION_MODE_CW;
      fprintf(fp, "G02*\n");
      continue;
    }
    else if (region_nod->type == GERBER_REGION_G03) {
      gs->interpolation_mode = INTERPOLATION_MODE_CCW;
      fprintf(fp, "G03*\n");
      continue;
    }

    _ix = (int)(region_nod->x * C);
    _iy = (int)(region_nod->y * C);

    _ii = (int)(region_nod->i * C);
    _ij = (int)(region_nod->j * C);

    if (m==0) {
      fprintf(fp, "X%iY%iD02*\n", _ix, _iy);
    } else if (m==1) {

      if ((gs->interpolation_mode == INTERPOLATION_MODE_NONE) ||
          (gs->interpolation_mode == INTERPOLATION_MODE_LINEAR)) {
        if ((_ix != _ixprv) && (_iy != _iyprv)) { fprintf(fp, "X%iY%iD01*\n", _ix, _iy); }
        else if (_ix != _ixprv) { fprintf(fp, "X%iD01*\n", _ix); }
        else if (_iy != _iyprv) { fprintf(fp, "Y%iD01*\n", _iy); }
      }
      else {
        fprintf(fp, "X%iY%iI%iJ%iD01*\n", _ix, _iy, _ii, _ij);
      }

    } else {

      if ((gs->interpolation_mode == INTERPOLATION_MODE_NONE) ||
          (gs->interpolation_mode == INTERPOLATION_MODE_LINEAR)) {
        if ((_ix != _ixprv) && (_iy != _iyprv)) { fprintf(fp, "X%iY%iD01*\n", _ix, _iy); }
        else if (_ix != _ixprv) { fprintf(fp, "X%iD01*\n", _ix); }
        else if (_iy != _iyprv) { fprintf(fp, "Y%iD01*\n", _iy); }
      }
      else {
        fprintf(fp, "X%iY%iI%iJ%iD01*\n", _ix, _iy, _ii, _ij);
      }
    }

    _ixprv = _ix;
    _iyprv = _iy;

    //region_nod = region_nod->next;
    m++;
  }

  fprintf(fp, "G37*\n");

  //DEBUG
  fprintf(fp, "\n");
}

void _print_ab(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item_nod, int lvl) {
  fprintf(fp, "%%ABD%i*%%\n", item_nod->d_name);

  _print_gerber_state_r(fp, item_nod->aperture_block, lvl+1);

  fprintf(fp, "%%AB*%%\n");
  fprintf(fp, "\n");
}

void _print_sr(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item_nod, int lvl) {
  fprintf(fp, "%%SRX%iY%iI%0.5fJ%0.5f*%%\n",
      item_nod->sr_x, item_nod->sr_y,
      (float)item_nod->sr_i, (float)item_nod->sr_j);

  _print_gerber_state_r(fp, item_nod->step_repeat, lvl+1);

  fprintf(fp, "%%SR*%%\n");
  fprintf(fp, "\n");
}

void _print_m02(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item_nod) {
  fprintf(fp, "M02*\n");
}

//void _print_d10p(FILE *fp, gerber_state_t *gs, gerber_item_ll_t *item_nod) { fprintf(fp, "D%i*\n", item_nod->d_name); }

void _print_gerber_state_r(FILE *fp, gerber_state_t *gs, int lvl) {
  int n=0, m=0;

  gerber_item_ll_t *item_nod;
  //gerber_region_t *region_nod;
  gerber_item_ll_t *region_nod;

  item_nod = gs->item_head;

  while (item_nod) {
    //fprintf(fp, "\n");
    //_pws(fp, lvl); fprintf(fp, "[%i] %s (%i)\n", n, lookup_type_name(item_nod->type), item_nod->type);

    switch (item_nod->type) {
      case GERBER_MO: _print_mo(fp, gs, item_nod); break;
      case GERBER_LP: _print_lp(fp, gs, item_nod); break;
      case GERBER_LM: _print_lm(fp, gs, item_nod); break;
      case GERBER_LR: _print_lr(fp, gs, item_nod); break;
      case GERBER_LS: _print_ls(fp, gs, item_nod); break;
      case GERBER_AM: _print_am(fp, gs, item_nod); break;
      case GERBER_AD: _print_ad(fp, gs, item_nod); break;
      case GERBER_ADE: _print_ade(fp, gs, item_nod); break;

      case GERBER_G01:
                       gs->interpolation_mode = INTERPOLATION_MODE_LINEAR;
                       _print_g01(fp, gs, item_nod); break;
      case GERBER_G02:
                       gs->interpolation_mode = INTERPOLATION_MODE_CW;
                       _print_g02(fp, gs, item_nod); break;
      case GERBER_G03:
                       gs->interpolation_mode = INTERPOLATION_MODE_CCW;
                       _print_g03(fp, gs, item_nod); break;

      case GERBER_G74:
                       gs->quadrent_mode = QUADRENT_MODE_SINGLE;
                       _print_g74(fp, gs, item_nod); break;
      case GERBER_G75:
                       gs->quadrent_mode = QUADRENT_MODE_MULTI;
                       _print_g75(fp, gs, item_nod); break;

      case GERBER_D10P: _print_d10p(fp, gs, item_nod); break;

      case GERBER_AB: _print_ab(fp, gs, item_nod, lvl); break;
      case GERBER_SR: _print_sr(fp, gs, item_nod, lvl); break;

      case GERBER_D3:
      case GERBER_FLASH:        _print_flash(fp, gs, item_nod); break;
      case GERBER_SEGMENT:      _print_segment(fp, gs, item_nod); break;
      case GERBER_SEGMENT_ARC:  _print_segment_arc(fp, gs, item_nod); break;
      case GERBER_REGION:       _print_region(fp, gs, item_nod); break;

      case GERBER_M02: _print_m02(fp, gs, item_nod); break;
      default: fprintf(stderr, "WARNING: unknown item type (%i), skipping\n", item_nod->type); break;
    }

    item_nod = item_nod->next;
    n++;
  }



}

void print_gerber_state(gerber_state_t *gs) {

  fprintf(stdout, "%%FSLAX%i%iY%i%i*%%\n",
      gs->fs_x_int, gs->fs_x_real,
      gs->fs_y_int, gs->fs_y_real);
  _print_gerber_state_r(stdout, gs, 0);
}

//------------------------

void _print_summary(gerber_state_t *gs, int level) {
  int i;
  gerber_item_ll_t *item, *region;

  for (item = gs->item_head; item; item = item->next) {

    for (i=0; i<level; i++) { printf(" "); }

    printf("%i", item->type);

    if (item->type == GERBER_REGION) {
      printf(":");
      for (region = item->region_head; region; region = region->next) {
        printf(" %i", region->type);
      }
    }

    printf("\n");

    if (item->type == GERBER_SR) {
      _print_summary(item->step_repeat, level+1);
    }
    else if (item->type == GERBER_AB) {
      _print_summary(item->aperture_block, level+1);
    }

  }

}

//---

int main(int argc, char **argv) {
  int i, j, k, n_line;
  char *chp, *ts;
  FILE *fp;

  int image_parameter_code;
  int function_code;

  int print_summary=0;

  gerber_state_t gs;

  gerber_state_init(&gs);

  if (argc!=2) { printf("provide filename\n"); exit(0); }

  k = gerber_state_load_file(&gs, argv[1]);
  if (k<0) {
    perror(argv[1]);
  }

  if (print_summary) {
    _print_summary(&gs, 0);
  }
  else {
    print_gerber_state(&gs);
  }

  //gerber_report_state(&gs);

  gerber_state_clear(&gs);
}

#endif
