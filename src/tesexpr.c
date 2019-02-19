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

/* COMPILE TIME OPTIONS */

/* Exponentiation associativity:
For a^b^c = (a^b)^c and -a^b = (-a)^b do nothing.
For a^b^c = a^(b^c) and -a^b = -(a^b) uncomment the next line.*/
/* #define TES_POW_FROM_RIGHT */

/* Logarithms
For log = base 10 log do nothing
For log = natural log uncomment the next line. */
/* #define TES_NAT_LOG */

/* (tiny expressing stripped)
 * disabled builtin functions to only allow simple
 * calculators
 */

#include "tesexpr.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#ifndef NAN
#define NAN (0.0/0.0)
#endif

#ifndef INFINITY
#define INFINITY (1.0/0.0)
#endif


typedef double (*tes_fun2)(double, double);

enum {
    TOK_NULL = TES_CLOSURE7+1, TOK_ERROR, TOK_END, TOK_SEP,
    TOK_OPEN, TOK_CLOSE, TOK_NUMBER, TOK_VARIABLE, TOK_INFIX
};


enum {TES_CONSTANT = 1};


typedef struct state {
    const char *start;
    const char *next;
    int type;
    union {double value; const double *bound; const void *function;};
    void *context;

    const tes_variable *lookup;
    int lookup_len;
} state;


#define TYPE_MASK(TYPE) ((TYPE)&0x0000001F)

#define IS_PURE(TYPE) (((TYPE) & TES_FLAG_PURE) != 0)
#define IS_FUNCTION(TYPE) (((TYPE) & TES_FUNCTION0) != 0)
#define IS_CLOSURE(TYPE) (((TYPE) & TES_CLOSURE0) != 0)
#define ARITY(TYPE) ( ((TYPE) & (TES_FUNCTION0 | TES_CLOSURE0)) ? ((TYPE) & 0x00000007) : 0 )
#define NEW_EXPR(type, ...) new_expr((type), (const tes_expr*[]){__VA_ARGS__})

static tes_expr *new_expr(const int type, const tes_expr *parameters[]) {
    const int arity = ARITY(type);
    const int psize = sizeof(void*) * arity;
    const int size = (sizeof(tes_expr) - sizeof(void*)) + psize + (IS_CLOSURE(type) ? sizeof(void*) : 0);

    //DEBUG
    //if (type==0) {
    //  fprintf(stderr, "# type: %i, param: %p, size: %i, psize: %i\n",
    //      type, parameters, size, psize);
    //  fflush(stderr);
    //}
    //DEBUG

    tes_expr *ret = malloc(size);
    memset(ret, 0, size);
    if (arity && parameters) {
        memcpy(ret->parameters, parameters, psize);
    }
    ret->type = type;
    ret->bound = 0;
    return ret;
}


void tes_free_parameters(tes_expr *n) {
    if (!n) return;
    switch (TYPE_MASK(n->type)) {
        case TES_FUNCTION7: case TES_CLOSURE7: tes_free(n->parameters[6]);
        case TES_FUNCTION6: case TES_CLOSURE6: tes_free(n->parameters[5]);
        case TES_FUNCTION5: case TES_CLOSURE5: tes_free(n->parameters[4]);
        case TES_FUNCTION4: case TES_CLOSURE4: tes_free(n->parameters[3]);
        case TES_FUNCTION3: case TES_CLOSURE3: tes_free(n->parameters[2]);
        case TES_FUNCTION2: case TES_CLOSURE2: tes_free(n->parameters[1]);
        case TES_FUNCTION1: case TES_CLOSURE1: tes_free(n->parameters[0]);
    }
}


void tes_free(tes_expr *n) {
    if (!n) return;
    tes_free_parameters(n);
    free(n);
}


static double pi() {return 3.14159265358979323846;}
static double e() {return 2.71828182845904523536;}
static double fac(double a) {/* simplest version of fac */
    if (a < 0.0)
        return NAN;
    if (a > UINT_MAX)
        return INFINITY;
    unsigned int ua = (unsigned int)(a);
    unsigned long int result = 1, i;
    for (i = 1; i <= ua; i++) {
        if (i > ULONG_MAX / result)
            return INFINITY;
        result *= i;
    }
    return (double)result;
}
static double ncr(double n, double r) {
    if (n < 0.0 || r < 0.0 || n < r) return NAN;
    if (n > UINT_MAX || r > UINT_MAX) return INFINITY;
    unsigned long int un = (unsigned int)(n), ur = (unsigned int)(r), i;
    unsigned long int result = 1;
    if (ur > un / 2) ur = un - ur;
    for (i = 1; i <= ur; i++) {
        if (result > ULONG_MAX / (un - ur + i))
            return INFINITY;
        result *= un - ur + i;
        result /= i;
    }
    return result;
}
static double npr(double n, double r) {return ncr(n, r) * fac(r);}

static const tes_variable functions[] = {
    {0, 0, 0, 0}
};

static const tes_variable functions_disabled[] = {
    /* must be in alphabetical order */
    {"abs", fabs,     TES_FUNCTION1 | TES_FLAG_PURE, 0},
    {"acos", acos,    TES_FUNCTION1 | TES_FLAG_PURE, 0},
    {"asin", asin,    TES_FUNCTION1 | TES_FLAG_PURE, 0},
    {"atan", atan,    TES_FUNCTION1 | TES_FLAG_PURE, 0},
    {"atan2", atan2,  TES_FUNCTION2 | TES_FLAG_PURE, 0},
    {"ceil", ceil,    TES_FUNCTION1 | TES_FLAG_PURE, 0},
    {"cos", cos,      TES_FUNCTION1 | TES_FLAG_PURE, 0},
    {"cosh", cosh,    TES_FUNCTION1 | TES_FLAG_PURE, 0},
    {"e", e,          TES_FUNCTION0 | TES_FLAG_PURE, 0},
    {"exp", exp,      TES_FUNCTION1 | TES_FLAG_PURE, 0},
    {"fac", fac,      TES_FUNCTION1 | TES_FLAG_PURE, 0},
    {"floor", floor,  TES_FUNCTION1 | TES_FLAG_PURE, 0},
    {"ln", log,       TES_FUNCTION1 | TES_FLAG_PURE, 0},
#ifdef TES_NAT_LOG
    {"log", log,      TES_FUNCTION1 | TES_FLAG_PURE, 0},
#else
    {"log", log10,    TES_FUNCTION1 | TES_FLAG_PURE, 0},
#endif
    {"log10", log10,  TES_FUNCTION1 | TES_FLAG_PURE, 0},
    {"ncr", ncr,      TES_FUNCTION2 | TES_FLAG_PURE, 0},
    {"npr", npr,      TES_FUNCTION2 | TES_FLAG_PURE, 0},
    {"pi", pi,        TES_FUNCTION0 | TES_FLAG_PURE, 0},
    {"pow", pow,      TES_FUNCTION2 | TES_FLAG_PURE, 0},
    {"sin", sin,      TES_FUNCTION1 | TES_FLAG_PURE, 0},
    {"sinh", sinh,    TES_FUNCTION1 | TES_FLAG_PURE, 0},
    {"sqrt", sqrt,    TES_FUNCTION1 | TES_FLAG_PURE, 0},
    {"tan", tan,      TES_FUNCTION1 | TES_FLAG_PURE, 0},
    {"tanh", tanh,    TES_FUNCTION1 | TES_FLAG_PURE, 0},
    {0, 0, 0, 0}
};

static const tes_variable *find_builtin(const char *name, int len) {
    int imin = 0;
    int imax = (int)((sizeof(functions) / sizeof(tes_variable)) - 2);

    /*Binary search.*/
    while (imax >= imin) {
        const int i = (imin + ((imax-imin)/2));
        int c = strncmp(name, functions[i].name, len);
        if (!c) c = '\0' - functions[i].name[len];
        if (c == 0) {
            return functions + i;
        } else if (c > 0) {
            imin = i + 1;
        } else {
            imax = i - 1;
        }
    }

    return 0;
}

static const tes_variable *find_lookup(const state *s, const char *name, int len) {
    int iters;
    const tes_variable *var;
    if (!s->lookup) return 0;

    for (var = s->lookup, iters = s->lookup_len; iters; ++var, --iters) {

      //DEBUG
      //fprintf(stderr, "## lookup: name:%s == var->name %s (%i)\n", name, var->name, len); fflush(stderr);
      //fprintf(stderr, "## var(%i,%s,%f)\n", var->type, var->name, (float)(*((double *)(var->address)))); fflush(stderr);
      //DEBUG

        if (strncmp(name, var->name, len) == 0 && var->name[len] == '\0') {

          //DEBUG
          //fprintf(stderr, "### returning...\n"); fflush(stderr);
          //DEBUG

            return var;
        }
    }
    return 0;
}



static double add(double a, double b) {return a + b;}
static double sub(double a, double b) {return a - b;}
static double mul(double a, double b) {return a * b;}
static double divide(double a, double b) {return a / b;}
static double negate(double a) {return -a;}
static double comma(double a, double b) {(void)a; return b;}


void next_token(state *s) {
    s->type = TOK_NULL;

    do {

        if (!*s->next){
            s->type = TOK_END;
            return;
        }

        /* Try reading a number. */
        if ((s->next[0] >= '0' && s->next[0] <= '9') || s->next[0] == '.') {
            s->value = strtod(s->next, (char**)&s->next);
            s->type = TOK_NUMBER;
        } else {
            /* Look for a variable or builtin function call. */

            //if (s->next[0] >= 'a' && s->next[0] <= 'z') {
            if (s->next[0] == '$') {
                const char *start;
                start = s->next;

                //while ((s->next[0] >= 'a' && s->next[0] <= 'z') || (s->next[0] >= '0' && s->next[0] <= '9') || (s->next[0] == '_')) s->next++;
                s->next++;
                while ( (s->next[0] >= '0') && (s->next[0] <= '9') ) s->next++;

                const tes_variable *var = find_lookup(s, start, s->next - start);


                  //DEBUG
                  //fprintf(stderr, "#cp0 (tesexpr.c::next_token)\n"); fflush(stderr);
                  //fprintf(stderr, "#  var: %p\n", var); fflush(stderr);
                  //DEBUG


                // builtin functions can't start with '$', only variables, so
                // skip this check
                //
                //if (!var) var = find_builtin(start, s->next - start);

                if (!var) {
                    s->type = TOK_ERROR;
                } else {

                  //DEBUG
                  //fprintf(stderr, "#cp (tesexpr.c::next_token)\n"); fflush(stderr);
                  //fprintf(stderr, "#  name: %s\n", var->name); fflush(stderr);
                  //fprintf(stderr, "#  type: %i,  type_maske: %i, (TES_VARIABLE %i)\n", (int)var->type, (int)TYPE_MASK(var->type), TES_VARIABLE); fflush(stderr);
                  //DEBUG

                    switch(TYPE_MASK(var->type))
                    {
                        case TES_VARIABLE:

                          //DEBUG
                          //fprintf(stderr, "# cp2... val: %f\n", (float)(*((double *)var->address)));
                          //DEBUG

                            s->type = TOK_VARIABLE;
                            s->bound = var->address;
                            break;

                        case TES_CLOSURE0: case TES_CLOSURE1: case TES_CLOSURE2: case TES_CLOSURE3:
                        case TES_CLOSURE4: case TES_CLOSURE5: case TES_CLOSURE6: case TES_CLOSURE7:
                            s->context = var->context;

                        case TES_FUNCTION0: case TES_FUNCTION1: case TES_FUNCTION2: case TES_FUNCTION3:
                        case TES_FUNCTION4: case TES_FUNCTION5: case TES_FUNCTION6: case TES_FUNCTION7:
                            s->type = var->type;
                            s->function = var->address;
                            break;
                    }
                }

            } else {
                /* Look for an operator or special character. */
                switch (s->next++[0]) {
                    case '+': s->type = TOK_INFIX; s->function = add; break;
                    case '-': s->type = TOK_INFIX; s->function = sub; break;

                    //case '*': s->type = TOK_INFIX; s->function = mul; break;
                    case 'X':
                    case 'x': s->type = TOK_INFIX; s->function = mul; break;

                    case '/': s->type = TOK_INFIX; s->function = divide; break;
                    //case '^': s->type = TOK_INFIX; s->function = pow; break;
                    //case '%': s->type = TOK_INFIX; s->function = fmod; break;
                    case '(': s->type = TOK_OPEN; break;
                    case ')': s->type = TOK_CLOSE; break;

                    //case ',': s->type = TOK_SEP; break;

                    case ' ': case '\t': case '\n': case '\r': break;
                    default: s->type = TOK_ERROR; break;
                }
            }
        }
    } while (s->type == TOK_NULL);
}


static tes_expr *list(state *s);
static tes_expr *expr(state *s);
static tes_expr *power(state *s);

static tes_expr *base(state *s) {
    /* <base>      =    <constant> | <variable> | <function-0> {"(" ")"} | <function-1> <power> | <function-X> "(" <expr> {"," <expr>} ")" | "(" <list> ")" */
    tes_expr *ret;
    int arity;

    switch (TYPE_MASK(s->type)) {
        case TOK_NUMBER:
            ret = new_expr(TES_CONSTANT, 0);
            ret->value = s->value;
            next_token(s);
            break;

        case TOK_VARIABLE:
            ret = new_expr(TES_VARIABLE, 0);
            ret->bound = s->bound;
            next_token(s);
            break;

        case TES_FUNCTION0:
        case TES_CLOSURE0:
            ret = new_expr(s->type, 0);
            ret->function = s->function;
            if (IS_CLOSURE(s->type)) ret->parameters[0] = s->context;
            next_token(s);
            if (s->type == TOK_OPEN) {
                next_token(s);
                if (s->type != TOK_CLOSE) {
                    s->type = TOK_ERROR;
                } else {
                    next_token(s);
                }
            }
            break;

        case TES_FUNCTION1:
        case TES_CLOSURE1:
            ret = new_expr(s->type, 0);
            ret->function = s->function;
            if (IS_CLOSURE(s->type)) ret->parameters[1] = s->context;
            next_token(s);
            ret->parameters[0] = power(s);
            break;

        case TES_FUNCTION2: case TES_FUNCTION3: case TES_FUNCTION4:
        case TES_FUNCTION5: case TES_FUNCTION6: case TES_FUNCTION7:
        case TES_CLOSURE2: case TES_CLOSURE3: case TES_CLOSURE4:
        case TES_CLOSURE5: case TES_CLOSURE6: case TES_CLOSURE7:
            arity = ARITY(s->type);

            ret = new_expr(s->type, 0);
            ret->function = s->function;
            if (IS_CLOSURE(s->type)) ret->parameters[arity] = s->context;
            next_token(s);

            if (s->type != TOK_OPEN) {
                s->type = TOK_ERROR;
            } else {
                int i;
                for(i = 0; i < arity; i++) {
                    next_token(s);
                    ret->parameters[i] = expr(s);
                    if(s->type != TOK_SEP) {
                        break;
                    }
                }
                if(s->type != TOK_CLOSE || i != arity - 1) {
                    s->type = TOK_ERROR;
                } else {
                    next_token(s);
                }
            }

            break;

        case TOK_OPEN:
            next_token(s);
            ret = list(s);
            if (s->type != TOK_CLOSE) {
                s->type = TOK_ERROR;
            } else {
                next_token(s);
            }
            break;

        default:
            ret = new_expr(0, 0);
            s->type = TOK_ERROR;
            ret->value = NAN;
            break;
    }

    return ret;
}


static tes_expr *power(state *s) {
    /* <power>     =    {("-" | "+")} <base> */
    int sign = 1;
    while (s->type == TOK_INFIX && (s->function == add || s->function == sub)) {
        if (s->function == sub) sign = -sign;
        next_token(s);
    }

    tes_expr *ret;

    if (sign == 1) {
        ret = base(s);
    } else {
        ret = NEW_EXPR(TES_FUNCTION1 | TES_FLAG_PURE, base(s));
        ret->function = negate;
    }

    return ret;
}

#ifdef TES_POW_FROM_RIGHT
static tes_expr *factor(state *s) {
    /* <factor>    =    <power> {"^" <power>} */
    tes_expr *ret = power(s);

    int neg = 0;
    tes_expr *insertion = 0;

    if (ret->type == (TES_FUNCTION1 | TES_FLAG_PURE) && ret->function == negate) {
        tes_expr *se = ret->parameters[0];
        free(ret);
        ret = se;
        neg = 1;
    }

    while (s->type == TOK_INFIX && (s->function == pow)) {
        tes_fun2 t = s->function;
        next_token(s);

        if (insertion) {
            /* Make exponentiation go right-to-left. */
            tes_expr *insert = NEW_EXPR(TES_FUNCTION2 | TES_FLAG_PURE, insertion->parameters[1], power(s));
            insert->function = t;
            insertion->parameters[1] = insert;
            insertion = insert;
        } else {
            ret = NEW_EXPR(TES_FUNCTION2 | TES_FLAG_PURE, ret, power(s));
            ret->function = t;
            insertion = ret;
        }
    }

    if (neg) {
        ret = NEW_EXPR(TES_FUNCTION1 | TES_FLAG_PURE, ret);
        ret->function = negate;
    }

    return ret;
}
#else
static tes_expr *factor(state *s) {
    /* <factor>    =    <power> {"^" <power>} */
    tes_expr *ret = power(s);

    while (s->type == TOK_INFIX && (s->function == pow)) {
        tes_fun2 t = s->function;
        next_token(s);
        ret = NEW_EXPR(TES_FUNCTION2 | TES_FLAG_PURE, ret, power(s));
        ret->function = t;
    }

    return ret;
}
#endif



static tes_expr *term(state *s) {
    /* <term>      =    <factor> {("*" | "/" | "%") <factor>} */
    tes_expr *ret = factor(s);

    while (s->type == TOK_INFIX && (s->function == mul || s->function == divide || s->function == fmod)) {
        tes_fun2 t = s->function;
        next_token(s);
        ret = NEW_EXPR(TES_FUNCTION2 | TES_FLAG_PURE, ret, factor(s));
        ret->function = t;
    }

    return ret;
}


static tes_expr *expr(state *s) {
    /* <expr>      =    <term> {("+" | "-") <term>} */
    tes_expr *ret = term(s);

    while (s->type == TOK_INFIX && (s->function == add || s->function == sub)) {
        tes_fun2 t = s->function;
        next_token(s);
        ret = NEW_EXPR(TES_FUNCTION2 | TES_FLAG_PURE, ret, term(s));
        ret->function = t;
    }

    return ret;
}


static tes_expr *list(state *s) {
    /* <list>      =    <expr> {"," <expr>} */
    tes_expr *ret = expr(s);

    while (s->type == TOK_SEP) {
        next_token(s);
        ret = NEW_EXPR(TES_FUNCTION2 | TES_FLAG_PURE, ret, expr(s));
        ret->function = comma;
    }

    return ret;
}


#define TES_FUN(...) ((double(*)(__VA_ARGS__))n->function)
#define M(e) tes_eval(n->parameters[e])


double tes_eval(const tes_expr *n) {
    if (!n) return NAN;

    switch(TYPE_MASK(n->type)) {
        case TES_CONSTANT: return n->value;
        case TES_VARIABLE: return *n->bound;

        case TES_FUNCTION0: case TES_FUNCTION1: case TES_FUNCTION2: case TES_FUNCTION3:
        case TES_FUNCTION4: case TES_FUNCTION5: case TES_FUNCTION6: case TES_FUNCTION7:
            switch(ARITY(n->type)) {
                case 0: return TES_FUN(void)();
                case 1: return TES_FUN(double)(M(0));
                case 2: return TES_FUN(double, double)(M(0), M(1));
                case 3: return TES_FUN(double, double, double)(M(0), M(1), M(2));
                case 4: return TES_FUN(double, double, double, double)(M(0), M(1), M(2), M(3));
                case 5: return TES_FUN(double, double, double, double, double)(M(0), M(1), M(2), M(3), M(4));
                case 6: return TES_FUN(double, double, double, double, double, double)(M(0), M(1), M(2), M(3), M(4), M(5));
                case 7: return TES_FUN(double, double, double, double, double, double, double)(M(0), M(1), M(2), M(3), M(4), M(5), M(6));
                default: return NAN;
            }

        case TES_CLOSURE0: case TES_CLOSURE1: case TES_CLOSURE2: case TES_CLOSURE3:
        case TES_CLOSURE4: case TES_CLOSURE5: case TES_CLOSURE6: case TES_CLOSURE7:
            switch(ARITY(n->type)) {
                case 0: return TES_FUN(void*)(n->parameters[0]);
                case 1: return TES_FUN(void*, double)(n->parameters[1], M(0));
                case 2: return TES_FUN(void*, double, double)(n->parameters[2], M(0), M(1));
                case 3: return TES_FUN(void*, double, double, double)(n->parameters[3], M(0), M(1), M(2));
                case 4: return TES_FUN(void*, double, double, double, double)(n->parameters[4], M(0), M(1), M(2), M(3));
                case 5: return TES_FUN(void*, double, double, double, double, double)(n->parameters[5], M(0), M(1), M(2), M(3), M(4));
                case 6: return TES_FUN(void*, double, double, double, double, double, double)(n->parameters[6], M(0), M(1), M(2), M(3), M(4), M(5));
                case 7: return TES_FUN(void*, double, double, double, double, double, double, double)(n->parameters[7], M(0), M(1), M(2), M(3), M(4), M(5), M(6));
                default: return NAN;
            }

        default: return NAN;
    }

}

#undef TES_FUN
#undef M

static void optimize(tes_expr *n) {
    /* Evaluates as much as possible. */
    if (n->type == TES_CONSTANT) return;
    if (n->type == TES_VARIABLE) return;

    /* Only optimize out functions flagged as pure. */
    if (IS_PURE(n->type)) {
        const int arity = ARITY(n->type);
        int known = 1;
        int i;
        for (i = 0; i < arity; ++i) {
            optimize(n->parameters[i]);
            if (((tes_expr*)(n->parameters[i]))->type != TES_CONSTANT) {
                known = 0;
            }
        }
        if (known) {
            const double value = tes_eval(n);
            tes_free_parameters(n);
            n->type = TES_CONSTANT;
            n->value = value;
        }
    }
}


tes_expr *tes_compile(const char *expression, const tes_variable *variables, int var_count, int *error) {
    state s;
    s.start = s.next = expression;
    s.lookup = variables;
    s.lookup_len = var_count;

    next_token(&s);
    tes_expr *root = list(&s);

    if (s.type != TOK_END) {
        tes_free(root);
        if (error) {
            *error = (s.next - s.start);
            if (*error == 0) *error = 1;
        }
        return 0;
    } else {
        optimize(root);
        if (error) *error = 0;
        return root;
    }
}


double tes_interp(const char *expression, int *error) {
    tes_expr *n = tes_compile(expression, 0, 0, error);
    double ret;
    if (n) {
        ret = tes_eval(n);
        tes_free(n);
    } else {
        ret = NAN;
    }
    return ret;
}

static void pn (const tes_expr *n, int depth) {
    int i, arity;
    printf("%*s", depth, "");

    switch(TYPE_MASK(n->type)) {
    case TES_CONSTANT: printf("%f\n", n->value); break;
    case TES_VARIABLE: printf("bound %p\n", n->bound); break;

    case TES_FUNCTION0: case TES_FUNCTION1: case TES_FUNCTION2: case TES_FUNCTION3:
    case TES_FUNCTION4: case TES_FUNCTION5: case TES_FUNCTION6: case TES_FUNCTION7:
    case TES_CLOSURE0: case TES_CLOSURE1: case TES_CLOSURE2: case TES_CLOSURE3:
    case TES_CLOSURE4: case TES_CLOSURE5: case TES_CLOSURE6: case TES_CLOSURE7:
         arity = ARITY(n->type);
         printf("f%d", arity);
         for(i = 0; i < arity; i++) {
             printf(" %p", n->parameters[i]);
         }
         printf("\n");
         for(i = 0; i < arity; i++) {
             pn(n->parameters[i], depth + 1);
         }
         break;
    }
}


void tes_print(const tes_expr *n) {
    pn(n, 0);
}
