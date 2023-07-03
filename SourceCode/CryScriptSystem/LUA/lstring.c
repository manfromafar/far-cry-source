/*
** $Id: lstring.c,v 1.65 2001/06/15 20:36:57 roberto Exp $
** String table (keeps all strings handled by Lua)
** See Copyright Notice in lua.h
*/


#include <string.h>

#define LUA_PRIVATE
#include "lua.h"

#include "lmem.h"
#include "lobject.h"
#include "lstate.h"
#include "lstring.h"



void luaS_freeall (lua_State *L) {
  lua_assert(G(L)->strt.nuse==0);
  luaM_freearray(L, G(L)->strt.hash, G(L)->strt.size, TString *);
}


void luaS_resize (lua_State *L, int newsize) {
  TString **newhash = luaM_newvector(L, newsize, TString *);
  stringtable *tb = &G(L)->strt;
  int i;
  for (i=0; i<newsize; i++) newhash[i] = NULL;
  /* rehash */
  for (i=0; i<tb->size; i++) {
    TString *p = tb->hash[i];
    while (p) {  /* for each node in the list */
      TString *next = p->tsv.nexthash;  /* save next */
      lu_hash h = p->tsv.hash;
      int h1 = lmod(h, newsize);  /* new position */
      lua_assert((int)(h%newsize) == lmod(h, newsize));
      p->tsv.nexthash = newhash[h1];  /* chain it in new position */
      newhash[h1] = p;
      p = next;
    }
  }
  luaM_freearray(L, tb->hash, tb->size, TString *);
  tb->size = newsize;
  tb->hash = newhash;
}


static TString *newlstr (lua_State *L, const l_char *str, size_t l, lu_hash h) {
  TString *ts = (TString *)luaM_malloc(L, sizestring(l));
  stringtable *tb;
  ts->tsv.nexthash = NULL;
  ts->tsv.len = l;
  ts->tsv.hash = h;
  ts->tsv.marked = 0;
  ts->tsv.constindex = 0;
  memcpy(getstr(ts), str, l*sizeof(l_char));
  getstr(ts)[l] = l_c('\0');  /* ending 0 */
  tb = &G(L)->strt;
  h = lmod(h, tb->size);
  ts->tsv.nexthash = tb->hash[h];  /* chain new entry */
  tb->hash[h] = ts;
  tb->nuse++;
  if (tb->nuse > (ls_nstr)tb->size && tb->size <= MAX_INT/2)
    luaS_resize(L, tb->size*2);  /* too crowded */
  return ts;
}

/*just a test!
__forceinline int _memcmp(void *a,void *b,size_t size)
{
	//static unsigned int *an,*bn,nIdx;
	
	while(size>=4)
	{
		if(*((unsigned int *)a)!=*((unsigned int *)b))
			return -1;
		*((unsigned int *)a)++;
		*((unsigned int *)b)++;
		size-=4;
	}
	while(size>0)
	{
		if(*((unsigned char *)a)!=*((unsigned char *)b))
			return -1;
		*((unsigned char *)a)++;
		*((unsigned char *)b)++;
		size--;
	}
	return 0;
}*/

TString *luaS_newlstr (lua_State *L, const l_char *str, size_t l) {
  TString *ts;
  lu_hash h = l;  /* seed */
  size_t step = (l>>5)+1;  /* if string is too long, don't hash all its chars */
  size_t l1;
  for (l1=l; l1>=step; l1-=step)  /* compute hash */
    h = h ^ ((h<<5)+(h>>2)+uchar(str[l1-1]));
  for (ts = G(L)->strt.hash[lmod(h, G(L)->strt.size)];
       ts != NULL;
       ts = ts->tsv.nexthash) {
    if (ts->tsv.len == l && (memcmp(str, getstr(ts), l) == 0))
      return ts;
  }
  return newlstr(L, str, l, h);  /* not found */
}


Udata *luaS_newudata (lua_State *L, size_t s) {
  Udata *u = (Udata *)luaM_malloc(L, sizeudata(s));
  u->uv.len = s;
  u->uv.tag = 0;
  u->uv.value = u + 1;
  /* chain it on udata list */
  u->uv.next = G(L)->rootudata;
  G(L)->rootudata = u;
  return u;
}

