/*
** $Id: lparser.h,v 1.32 2001/06/28 14:56:25 roberto Exp $
** LL(1) Parser and code generator for Lua
** See Copyright Notice in lua.h
*/

#ifndef lparser_h
#define lparser_h

#include "lobject.h"
#include "lzio.h"


/*
** Expression descriptor
*/

typedef enum {
  VVOID,	/* no value */
  VNIL,
  VNUMBER,	/* n = value */
  VK,		/* info = index of constant in `k' */
  VGLOBAL,	/* info = index of global name in `k' */
  VLOCAL,	/* info = local register */
  VINDEXED,	/* info = table register; aux = index register (or `k') */
  VRELOCABLE,	/* info = instruction pc */
  VNONRELOC,	/* info = result register */
  VJMP,		/* info = result register */
  VCALL		/* info = result register */
} expkind;

typedef struct expdesc {
  expkind k;
  union {
    struct {
      int info, aux;
    } i;
    lua_Number n;
  } u;
  int t;  /* patch list of `exit when true' */
  int f;  /* patch list of `exit when false' */
} expdesc;


/* state needed to generate code for a given function */
typedef struct FuncState {
  Proto *f;  /* current function header */
  struct FuncState *prev;  /* enclosing function */
  struct LexState *ls;  /* lexical state */
  struct lua_State *L;  /* copy of the Lua state */
  int pc;  /* next position to code (equivalent to `ncode') */
  int lasttarget;   /* `pc' of last `jump target' */
  int jlt;  /* list of jumps to `lasttarget' */
  int freereg;  /* first free register */
  int nk;  /* number of elements in `k' */
  int np;  /* number of elements in `p' */
  int nlineinfo;  /* number of elements in `lineinfo' */
  int nlocvars;  /* number of elements in `locvars' */
  int nactloc;  /* number of active local variables */
  int lastline;  /* line where last `lineinfo' was generated */
  struct Breaklabel *bl;  /* chain of breakable blocks */
  expdesc upvalues[MAXUPVALUES];  /* upvalues */
  int actloc[MAXLOCALS];  /* local-variable stack (indices to locvars) */
} FuncState;


Proto *luaY_parser (lua_State *L, ZIO *z);


#endif
