#include "gerber_interpreter.h"

static int _tes_variable_idx(tes_variable *vars, int nvar, char *name) {
  int i;
  for (i=0; i<nvar; i++) {
    if ((vars[i].name) &&
        (strcmp(vars[i].name, name)==0)) {
      return i;
    }
  }
  return -1;
}

int eval_ad_func(gerber_state_t *gs, aperture_data_t *ap_d) {
  int i, err=0;
  am_ll_lib_t *am_lib_nod;
  am_ll_node_t *am_nod, *am_nod_head;
  tes_variable *vars=NULL;
  double *param_val=NULL;

  char **var_names=NULL;

  int var_idx=-1, nvar=0;
  int ret=0;
  tes_expr *expr;

  int nod_idx=0;

  // DEBUG
  //
  printf("# am func: %s\n", ap_d->macro_name);
  printf("#   f[%i]:", ap_d->macro_param_count);
  for (i=0; i<ap_d->macro_param_count; i++) {
    printf("#  %f", ap_d->macro_param[i]);
  }
  printf("\n");


  // Find the macro definition
  //
  am_lib_nod = gs->am_lib_head;
  while (am_lib_nod) {
    if (strcmp(am_lib_nod->am->name, ap_d->macro_name)==0) { break; }
    am_lib_nod = am_lib_nod->next;
  }

  // error if we haven't found it
  //
  if (!am_lib_nod) { return -1; }
  am_nod_head = am_lib_nod->am;

  // Allocate variables and set up tes expr state
  //
  nvar = ap_d->macro_param_count;
  if (ap_d->macro_param_count>0) {
    vars = (tes_variable *)malloc(sizeof(tes_variable)*(ap_d->macro_param_count));
    param_val = (double *)malloc(sizeof(double)*(ap_d->macro_param_count));
    var_names = (char **)malloc(sizeof(char *)*nvar);

    // transfer initial values to local array
    //
    for (i=0; i<nvar; i++) { param_val[i] = ap_d->macro_param[i]; }

    // Set up variable information
    //
    for (i=0; i<ap_d->macro_param_count; i++) {
      param_val[i] = 0;
      //asprintf(&(vars[i].name), "$%i", i+1);
      asprintf(&(var_names[i]), "$%i", i+1);
      vars[i].address = &(param_val[i]);
    }
  }

  while (am_nod) {

    //DEBUG
    printf("# eval nod idx %i\n", nod_idx);
    printf("# param_val[%i]:", nvar);
    for (i=0; i<nvar; i++) { printf(" %f", param_val[i]); }
    printf("\n");
    printf("# ad type: %i\n", am_nod->type);
    nod_idx++;

    switch (am_nod->type) {
      case AM_ENUM_NAME: printf("# ad name: %s\n", am_nod->name); break;
      case AM_ENUM_COMMENT: printf("# ad comment: %s\n", am_nod->comment); break;
      case AM_ENUM_VAR:

        printf("# ad var: %s (%i)\n", am_nod->varname, am_nod->n_eval_line);
        for (i=0; i<am_nod->n_eval_line; i++) {
          printf("#  [%i]: %s\n", i, am_nod->eval_line[i]);
        }
        printf("\n\n");

        var_idx = _tes_variable_idx(vars, nvar, am_nod->varname);
        if (var_idx < 0) { ret = -1; break; }

        printf("# evaluating var %s, idx %i\n", am_nod->varname, var_idx);

        expr = tes_compile(am_nod->eval_line[0], vars, nvar, &err);
        if (!expr) { ret=-2; break; }

        param_val[var_idx] = tes_eval(expr);

        tes_free(expr);

        break;

      case AM_ENUM_CIRCLE: break;
      case AM_ENUM_VECTOR_LINE: break;
      case AM_ENUM_CENTER_LINE: break;
      case AM_ENUM_OUTLINE: break;
      case AM_ENUM_POLYGON: break;
      case AM_ENUM_MOIRE: break;
      case AM_ENUM_THERMAL: break;
      default: ret=-3; break;
    }

    if (ret<0) { break; }
  }


cleanup:

  if (param_val) { free(param_val); }
  if (vars) {
    for (i=0; i<nvar; i++) { free(var_names[i]); }
    free(vars);
  }

  // DEBUG
  if (ret<0) {
    printf("ERROR got %i\n", ret);
  }

  return ret;
}
