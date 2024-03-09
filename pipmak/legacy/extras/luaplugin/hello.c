#include <stdlib.h>
#include <string.h>
#include "lua.h"
#include "lauxlib.h"
#include "../../source/terminal.h"

static int greeting(lua_State *L) {
	lua_pushliteral(L, "Hello World!");
	return 1;
}

typedef struct {
	char *greeting;
} Greeter;

static int newgreeter(lua_State *L) {
	const char *s = luaL_checkstring(L, 1);
	
	Greeter *greeter = lua_newuserdata(L, sizeof(Greeter));
	luaL_getmetatable(L, "hello-greeter");
	lua_setmetatable(L, -2);
	
	greeter->greeting = malloc(strlen(s) + 1);
	strcpy(greeter->greeting, s);
	
	terminalPrintf("Greeter object created!");
	
	return 1;
}

static int greeterCollect(lua_State *L) {
	Greeter *greeter = (Greeter*)luaL_checkudata(L, 1, "hello-greeter");
	if (greeter == NULL) luaL_typerror(L, 1, "greeter");
	
	free(greeter->greeting);
	
	terminalPrintf("Greeter object collected!");
	
	return 0;
}

static int greeterGreeting(lua_State *L) {
	Greeter *greeter = (Greeter*)luaL_checkudata(L, 1, "hello-greeter");
	if (greeter == NULL) luaL_typerror(L, 1, "greeter");
	
	lua_pushstring(L, greeter->greeting);
	return 1;
}

static const luaL_reg greeterMethods[] = {
	{"__gc", greeterCollect},
	{"greeting", greeterGreeting},
	{NULL, NULL}
};

int init(lua_State *L) {
	lua_register(L, "greeting", greeting);
	
	luaL_newmetatable(L, "hello-greeter");
	lua_pushliteral(L, "__index");
	lua_pushvalue(L, -2);
	lua_rawset(L, -3);
	luaL_openlib(L, NULL, greeterMethods, 0);
	lua_pop(L, 1); /*greeter metatable*/
	
	lua_register(L, "newgreeter", newgreeter);
	
	return 0;
}
