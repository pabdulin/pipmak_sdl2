/* printLuaStack() - a Lua debugging tool - Christian Walther 2005 - public domain */

#include "lua.h"
#include "lauxlib.h"

void printLuaStack(lua_State *L) {
	//const char *s;
	int i, t;
	int n = lua_gettop(L);
	if (n == 0) {
		printf("Lua stack is empty.\n");
	}
	else if (n < 0) {
		printf("Lua stack top is %d!\n", n);
	}
	else {
		printf("Lua stack:\n");
		for (i = 1; i <= n; i++) {
			switch (t = lua_type(L, i)) {
				case LUA_TNUMBER:
					printf("%2d %3d  number %f\n", i, i-n-1, lua_tonumber(L, i));
					break;
				case LUA_TSTRING:
					printf("%2d %3d  string \"%s\"\n", i, i-n-1, lua_tostring(L, i));
					break;
				case LUA_TUSERDATA:
				case LUA_TTABLE:
					if (lua_getmetatable(L, i)) {
						lua_rawget(L, LUA_REGISTRYINDEX);
						if (lua_isnil(L, -1)) {
							lua_pop(L, 1);
							lua_pushliteral(L, "unknown metatable");
						}
					}
					else {
						lua_pushliteral(L, "no metatable");
					}
					printf("%2d %3d  %s (%s)\n", i, i-n-1, lua_typename(L, t), lua_tostring(L, -1));
					lua_pop(L, 1);
					if (t == LUA_TTABLE) {
						lua_pushnil(L);
						while (lua_next(L, i) != 0) {
							switch (lua_type(L, -2)) {
								case LUA_TSTRING:
									printf("          [\"%s\"] = %s\n", lua_tostring(L, -2), lua_typename(L, lua_type(L, -1)));
									break;
								case LUA_TNUMBER:
									printf("          [%f] = %s\n", lua_tonumber(L, -2), lua_typename(L, lua_type(L, -1)));
									break;
								default:
									printf("          [%s] = %s\n", lua_typename(L, lua_type(L, -2)), lua_typename(L, lua_type(L, -1)));
									break;
							}
							lua_pop(L, 1);
						} 
					}
					break;
				default:
					printf("%2d %3d  %s\n", i, i-n-1, lua_typename(L, t));
					break;
			}
		}
	}
}
