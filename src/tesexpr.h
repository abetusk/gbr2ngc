/*
 * TINYEXPR - Tiny recursive descent parser and evaluation engine in C
 *
 * Copyright (c) 2015, 2016 Lewis Van Winkle
 *
 * http://CodePlea.com
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgement in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#ifndef __TINYEXPR_H__
#define __TINYEXPR_H__


#ifdef __cplusplus
extern "C" {
#endif



typedef struct tes_expr {
    int type;
    union {double value; const double *bound; const void *function;};
    void *parameters[1];
} tes_expr;


enum {
    TES_VARIABLE = 0,

    TES_FUNCTION0 = 8, TES_FUNCTION1, TES_FUNCTION2, TES_FUNCTION3,
    TES_FUNCTION4, TES_FUNCTION5, TES_FUNCTION6, TES_FUNCTION7,

    TES_CLOSURE0 = 16, TES_CLOSURE1, TES_CLOSURE2, TES_CLOSURE3,
    TES_CLOSURE4, TES_CLOSURE5, TES_CLOSURE6, TES_CLOSURE7,

    TES_FLAG_PURE = 32
};

typedef struct tes_variable {
    const char *name;
    const void *address;
    int type;
    void *context;
} tes_variable;



/* Parses the input expression, evaluates it, and frees it. */
/* Returns NaN on error. */
double tes_interp(const char *expression, int *error);

/* Parses the input expression and binds variables. */
/* Returns NULL on error. */
tes_expr *tes_compile(const char *expression, const tes_variable *variables, int var_count, int *error);

/* Evaluates the expression. */
double tes_eval(const tes_expr *n);

/* Prints debugging information on the syntax tree. */
void tes_print(const tes_expr *n);

/* Frees the expression. */
/* This is safe to call on NULL pointers. */
void tes_free(tes_expr *n);


#ifdef __cplusplus
}
#endif

#endif /*__TINYEXPR_H__*/
