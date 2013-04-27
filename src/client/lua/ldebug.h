
/*
** $Id: ldebug.h,v 1.2 2003/03/17 22:45:30 cipher Exp $
** Auxiliary functions from Debug Interface module
** See Copyright Notice in lua.h
*/

#ifndef ldebug_h
#define ldebug_h

#include "lstate.h"
#include "luadebug.h"

void            luaG_typeerror(lua_State * L,
                               StkId o,
                               const char *op);
void            luaG_binerror(lua_State * L,
                              StkId p1,
                              int t,
                              const char *op);
int             luaG_getline(int *lineinfo,
                             int pc,
                             int refline,
                             int *refi);
void            luaG_ordererror(lua_State * L,
                                StkId top);

#endif
