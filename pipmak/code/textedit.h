/*
 
 textedit.h, part of the Pipmak Game Engine
 Copyright (c) 2007 Christian Walther
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 
 */

/* $Id: textedit.h 153 2007-05-01 15:52:10Z cwalther $ */

#ifndef TEXTEDIT_H_SEEN
#define TEXTEDIT_H_SEEN

#include "SDL.h"
#include "lua.h"
#include "lauxlib.h"

typedef struct TextEditor TextEditor;

extern const luaL_reg pipmakInternalTexteditorFuncs[];
extern const luaL_reg texteditorFuncs[];

int textEditHandleKey(SDL_KeyboardEvent *event);
void textEditorDraw(TextEditor *te, Uint32 now);
void defocusTextEditors();
void textEditorFeedToGL(TextEditor *te);
void textEditorRemoveFromGL(TextEditor *te);
void textEditorDestroy(TextEditor *te);

#endif /*TEXTEDIT_H_SEEN*/
