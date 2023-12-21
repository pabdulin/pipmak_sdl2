/*
 
 textedit.c, part of the Pipmak Game Engine
 Copyright (c) 2007-2011 Christian Walther
 
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

/* $Id: textedit.c 228 2011-04-30 19:40:44Z cwalther $ */

#include "textedit.h"

#include "SDL_opengl.h"
#include "SDL_ttf.h"

#include "terminal.h"
#include "nodes.h"
#include "misc.h"


struct TextEditor {
	int x, y, w, h;
	int scroll;
	int maxlength, length;
	int selectionAnchor, selectionHead;
	int texw, texh;
	int keydownFuncRef;
	CNode *node;
	Uint8 textModified:1, selectionModified:1;
	GLuint textureID;
	Uint16 *text;
	Uint8 *advance;
	struct TextEditor *next;
};


extern GLenum glTextureTarget;
extern TTF_Font *verafont;
extern int mouseX, mouseY;
extern lua_State *L;
extern CNode *thisCNode;

static TextEditor *firstTextEditor = NULL;
static TextEditor *focusedTextEditor = NULL;


void textEditorDraw(TextEditor *te, Uint32 now) {
	int i;
	static Uint32 lastSelectionModTime = 0;

	if (glTextureTarget == GL_TEXTURE_RECTANGLE_NV) glDisable(GL_TEXTURE_RECTANGLE_NV);
	
	/*update scroll amount & insertion point blinking phase*/
	if (te->selectionModified) {
		int x = -te->scroll;
		for (i = 0; i < te->selectionHead; i++) x += te->advance[i];
		if (x < 0) te->scroll += x;
		else if (x >= te->w) te->scroll += x - te->w;
		lastSelectionModTime = now;
		te->selectionModified = 0;
	}
	
	/*re-render text if necessary*/
	glBindTexture(GL_TEXTURE_2D, te->textureID);
	if (te->textModified) {
		SDL_Surface *textsurf;
		if (te->text[0] != 0) {
			SDL_Color black = {0, 0, 0, 255};
			textsurf = TTF_RenderUNICODE_Blended(verafont, te->text, black);
		}
		else {
			/*create a dummy surface to avoid special cases below*/
			textsurf = SDL_CreateRGBSurface(SDL_SWSURFACE, 1, TTF_FontHeight(verafont), 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
		}
		if (textsurf == NULL) {
			terminalPrintf("Error rendering text editor text: %s", TTF_GetError());
		}
		else {
			SDL_Surface *outsurf;
			i = textsurf->w - te->w;
			if (i < 0) i = 0;
			if (te->scroll > i) te->scroll = i;
			te->texw = 8;
			while (te->texw < textsurf->w+2) te->texw *= 2;
			te->texh = 8;
			while (te->texh < textsurf->h+2) te->texh *= 2;
			outsurf = SDL_CreateRGBSurface(SDL_SWSURFACE, te->texw, te->texh, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
			if (outsurf == NULL) {
				terminalPrintf("SDL_CreateRGBSurface: %s", SDL_GetError());
			}
			else {
				SDL_Rect rect = {1, 1, 0, 0}; /*keep a 1px transparent border around the text*/
				SDL_SetAlpha(textsurf, 0, 255); /*ignore alpha when blitting, just copy it over*/
				SDL_BlitSurface(textsurf, NULL, outsurf, &rect);
				SDL_LockSurface(outsurf);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, te->texw, te->texh, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, outsurf->pixels);
				SDL_UnlockSurface(outsurf);
				SDL_FreeSurface(outsurf);
			}
			SDL_FreeSurface(textsurf);
		}
		te->textModified = 0;
	}
	
	glDisable(GL_TEXTURE_2D);
	if (te->selectionAnchor == te->selectionHead) {
		/*draw blinking insertion point*/
		if (te == focusedTextEditor && ((now - lastSelectionModTime) & (1<<9)) == 0) {
			int x = -te->scroll;
			for (i = 0; i < te->selectionAnchor; i++) x += te->advance[i];
			glColor4f(0.0, 0.0, 0.0, 1.0);
			glBegin(GL_QUADS);
			glVertex2f((GLfloat)(te->x + x), (GLfloat)te->y);
			glVertex2f((GLfloat)(te->x + x + 1), (GLfloat)te->y);
			glVertex2f((GLfloat)(te->x + x + 1), (GLfloat)(te->y + te->h));
			glVertex2f((GLfloat)(te->x + x), (GLfloat)(te->y + te->h));
			glEnd();
		}
	}
	else {
		/*draw selection*/
		int x1 = -te->scroll, x2 = -te->scroll;
		for (i = 0; i < te->selectionAnchor; i++) x1 += te->advance[i];
		for (i = 0; i < te->selectionHead; i++) x2 += te->advance[i];
		if (x1 < 0) x1 = 0;
		else if (x1 > te->w) x1 = te->w;
		if (x2 < 0) x2 = 0;
		else if (x2 > te->w) x2 = te->w;
		if (te == focusedTextEditor) glColor4f(0.6f, 0.75f, 1.0f, 0.8f);
		else glColor4f(0.75f, 0.75f, 0.75f, 0.8f);
		glBegin(GL_QUADS);
		glVertex2f((GLfloat)(te->x + x1), (GLfloat)te->y);
		glVertex2f((GLfloat)(te->x + x2), (GLfloat)te->y);
		glVertex2f((GLfloat)(te->x + x2), (GLfloat)(te->y + te->h));
		glVertex2f((GLfloat)(te->x + x1), (GLfloat)(te->y + te->h));
		glEnd();
	}
	glColor4f(1.0, 1.0, 1.0, 1.0);
	glEnable(GL_TEXTURE_2D);
	
	/*draw text*/
	glBegin(GL_QUADS);
	glTexCoord2f((float)(te->scroll + 1)/te->texw, 1.0f/te->texh);
	glVertex2f((GLfloat)te->x, (GLfloat)te->y);
	glTexCoord2f((float)(te->w + te->scroll + 1)/te->texw, 1.0f/te->texh);
	glVertex2f((GLfloat)(te->x + te->w), (GLfloat)te->y);
	glTexCoord2f((float)(te->w + te->scroll + 1)/te->texw, (float)(te->h + 1)/te->texh);
	glVertex2f((GLfloat)(te->x + te->w), (GLfloat)(te->y + te->h));
	glTexCoord2f((float)(te->scroll + 1)/te->texw, (float)(te->h + 1)/te->texh);
	glVertex2f((GLfloat)te->x, (GLfloat)(te->y + te->h));
	glEnd();
	
	if (glTextureTarget == GL_TEXTURE_RECTANGLE_NV) glEnable(GL_TEXTURE_RECTANGLE_NV);
}

static void focusTextEditor(TextEditor* te) {
	focusedTextEditor = te;
	if (te == NULL) {
		SDL_EnableKeyRepeat(0, 0);
	}
	else {
		SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);
	}
}

void defocusTextEditors() {
	focusTextEditor(NULL);
}

void textEditorFeedToGL(TextEditor *te) {
	glGenTextures(1, &(te->textureID));
	te->textModified = 1;
	te->selectionModified = 1;
}

void textEditorRemoveFromGL(TextEditor *te) {
	glDeleteTextures(1, &(te->textureID));
	te->textureID = 0;
}

int textEditHandleKey(SDL_KeyboardEvent *event) {
	if (focusedTextEditor == NULL) return 0;
	if (
		focusedTextEditor->keydownFuncRef != 0
		&& (
			event->keysym.sym == 0
			|| event->keysym.sym == SDLK_RETURN
			|| event->keysym.sym == SDLK_ESCAPE
			|| event->keysym.sym == SDLK_TAB
			|| event->keysym.sym == SDLK_LEFT
			|| event->keysym.sym == SDLK_RIGHT
			|| event->keysym.sym == SDLK_UP
			|| event->keysym.sym == SDLK_DOWN
		)
	) {
		lua_rawgeti(L, LUA_REGISTRYINDEX, focusedTextEditor->keydownFuncRef);
		lua_pushnumber(L, event->keysym.sym);
		lua_pushboolean(L, event->keysym.mod & KMOD_SHIFT);
		if (pcallWithBacktrace(L, 2, 1, focusedTextEditor->node) != 0) {
			terminalPrintf("Error running text editor keydown handler:\n%s", lua_tostring(L, -1));
			lua_pop(L, 1);
		}
		else if (lua_toboolean(L, -1)) {
			lua_pop(L, 1);
			return 1;
		}
		else lua_pop(L, 1);
	}
	switch (event->keysym.sym) {
		case SDLK_RETURN:
		case SDLK_ESCAPE:
			return 0;
			break;
		case SDLK_TAB:
			focusTextEditor((focusedTextEditor->next == NULL) ? firstTextEditor : focusedTextEditor->next);
			break;
		case SDLK_LEFT:
			if (event->keysym.mod & KMOD_SHIFT) {
				/*extend selection*/
				if (focusedTextEditor->selectionHead != 0) {
					focusedTextEditor->selectionHead = focusedTextEditor->selectionHead - 1;
				}
			}
			else {
				/*move insertion point left, or to beginning of selection*/
				if (focusedTextEditor->selectionAnchor == focusedTextEditor->selectionHead) {
					if (focusedTextEditor->selectionHead != 0) {
						focusedTextEditor->selectionAnchor = focusedTextEditor->selectionHead = focusedTextEditor->selectionHead - 1;
					}
				}
				else if (focusedTextEditor->selectionAnchor < focusedTextEditor->selectionHead) {
					focusedTextEditor->selectionHead = focusedTextEditor->selectionAnchor;
				}
				else {
					focusedTextEditor->selectionAnchor = focusedTextEditor->selectionHead;
				}
			}
			focusedTextEditor->selectionModified = 1;
			break;
		case SDLK_RIGHT:
			if (event->keysym.mod & KMOD_SHIFT) {
				/*extend selection*/
				if (focusedTextEditor->selectionHead != focusedTextEditor->length) {
					focusedTextEditor->selectionHead = focusedTextEditor->selectionHead + 1;
				}
			}
			else {
				/*move insertion point left, or to beginning of selection*/
				if (focusedTextEditor->selectionAnchor == focusedTextEditor->selectionHead) {
					if (focusedTextEditor->selectionHead != focusedTextEditor->length) {
						focusedTextEditor->selectionAnchor = focusedTextEditor->selectionHead = focusedTextEditor->selectionHead + 1;
					}
				}
				else if (focusedTextEditor->selectionAnchor < focusedTextEditor->selectionHead) {
					focusedTextEditor->selectionAnchor = focusedTextEditor->selectionHead;
				}
				else {
					focusedTextEditor->selectionHead = focusedTextEditor->selectionAnchor;
				}
			}
			focusedTextEditor->selectionModified = 1;
			break;
		case SDLK_UP:
			focusedTextEditor->selectionHead = 0;
			if (!(event->keysym.mod & KMOD_SHIFT)) {
				focusedTextEditor->selectionAnchor = 0;
			}
			focusedTextEditor->selectionModified = 1;
			break;
		case SDLK_DOWN:
			focusedTextEditor->selectionHead = focusedTextEditor->length;
			if (!(event->keysym.mod & KMOD_SHIFT)) {
				focusedTextEditor->selectionAnchor = focusedTextEditor->length;
			}
			focusedTextEditor->selectionModified = 1;
			break;
		default:
			if (event->keysym.sym != 0 || event->keysym.sym == SDLK_BACKSPACE) {
				Uint16 *start, *end;
				Uint8 *advstart, *advend;
				if (focusedTextEditor->selectionAnchor > focusedTextEditor->selectionHead) {
					start = focusedTextEditor->text + focusedTextEditor->selectionHead;
					end = focusedTextEditor->text + focusedTextEditor->selectionAnchor;
					advstart = focusedTextEditor->advance + focusedTextEditor->selectionHead;
					advend = focusedTextEditor->advance + focusedTextEditor->selectionAnchor;
				}
				else {
					start = focusedTextEditor->text + focusedTextEditor->selectionAnchor;
					end = focusedTextEditor->text + focusedTextEditor->selectionHead;
					advstart = focusedTextEditor->advance + focusedTextEditor->selectionAnchor;
					advend = focusedTextEditor->advance + focusedTextEditor->selectionHead;
				}
				if (start == end && event->keysym.sym == SDLK_BACKSPACE && start != focusedTextEditor->text) {
					/*select one character to the left of the insertion point*/
					start--;
					advstart--;
				}
				if (start != end) {
					/*delete selection*/
					Uint16 *c = start;
					Uint8 *a = advstart;
					focusedTextEditor->length -= (int)(end-start);
					while (1) {
						*c = *end;
						*a = *advend;
						if (*end == 0) break;
						c++; end++;
						a++; advend++;
					}
				}
				if (event->keysym.sym != SDLK_BACKSPACE && focusedTextEditor->length != focusedTextEditor->maxlength) {
					/*insert a character*/
					Uint16 savedChar;
					int i;
					int w[3];
					
					/*shift what comes after the insertion point by one character*/
					for (end = focusedTextEditor->text + focusedTextEditor->length, advend = focusedTextEditor->advance + focusedTextEditor->length; end >= start; end--, advend--) {
						*(end+1) = *end;
						*(advend+1) = *advend;
					}
					*start = event->keysym.sym;
					
					/*recalculate the advance width of the inserted char and (due to kerning) the following one*/
					for (i = 0; i < 3; i++) {
						savedChar = start[i];
						start[i] = 0;
						TTF_SizeUNICODE(verafont, (start == focusedTextEditor->text) ? start : start-1, &w[i], NULL);
						start[i] = savedChar;
					}
					advstart[0] = w[1] - w[0];
					advstart[1] = w[2] - w[1];
					
					start++;
					focusedTextEditor->length++;
				}
				focusedTextEditor->selectionAnchor = focusedTextEditor->selectionHead = (int)(start - focusedTextEditor->text);
				focusedTextEditor->selectionModified = 1;
				focusedTextEditor->textModified = 1;
			}
			break;
	}
	return 1;
}

static int newtexteditorLua(lua_State *L) {
	TextEditor *te;
	
	luaL_checktype(L, 1, LUA_TNUMBER);
	luaL_checktype(L, 2, LUA_TNUMBER);
	luaL_checktype(L, 3, LUA_TNUMBER);
	luaL_checktype(L, 4, LUA_TNUMBER);
	
	te = lua_newuserdata(L, sizeof(TextEditor));
	luaL_getmetatable(L, "pipmak-texteditor");
	lua_setmetatable(L, -2);
	
	te->x = (int)lua_tonumber(L, 1);
	te->y = (int)lua_tonumber(L, 2);
	te->w = (int)lua_tonumber(L, 3);
	te->maxlength = (int)lua_tonumber(L, 4);
	
	te->text = (Uint16*)malloc(2*(te->maxlength+1));
	if (te->text == NULL) luaL_error(L, "out of memory");
	te->text[0] = 0;
	te->length = 0;
	
	te->advance = (Uint8*)malloc(te->maxlength);
	if (te->advance == NULL) luaL_error(L, "out of memory");
	
	te->selectionAnchor = te->selectionHead = 0;
	te->scroll = 0;
	te->h = TTF_FontHeight(verafont);
	
	te->next = firstTextEditor;
	firstTextEditor = te;
	focusTextEditor(te);
	
	te->keydownFuncRef = 0;
	te->node = thisCNode;
	
	return 1;
}

void textEditorDestroy(TextEditor *te) {
	if (te->text != NULL) {
		free(te->text);
		te->text = NULL;
	}
	if (te->advance != NULL) {
		free(te->advance);
		te->advance = NULL;
	}
	if (te->textureID != 0) {
		textEditorRemoveFromGL(te);
	}
	if (te->next != te) {
		if (te == firstTextEditor) {
			firstTextEditor = te->next;
		}
		else {
			TextEditor *prev = firstTextEditor;
			while (prev->next != te) prev = prev->next;
			prev->next = te->next;
		}
		te->next = te;
	}
	if (te->keydownFuncRef != 0) {
		luaL_unref(L, LUA_REGISTRYINDEX, te->keydownFuncRef);
		te->keydownFuncRef = 0;
	}
	if (te == focusedTextEditor) focusTextEditor(NULL);
}

static int texteditorDestroyLua(lua_State *L) {
	/*this method serves as both te:destroy and te:__gc and may therefore be called multiple times*/
	TextEditor *te = (TextEditor*)luaL_checkudata(L, 1, "pipmak-texteditor");
	if (te == NULL) luaL_typerror(L, 1, "texteditor");
	textEditorDestroy(te);
	return 0;
}

static int texteditorTextLua(lua_State *L) {
	char *s;
	TextEditor *te = (TextEditor*)luaL_checkudata(L, 1, "pipmak-texteditor");
	if (te == NULL || te->text == NULL) luaL_typerror(L, 1, "texteditor");
	
	s = SDL_iconv_string("UTF-8", "UCS-2", (char*)(te->text), (te->length+1)*2);
	if (s == NULL) luaL_error(L, "UCS-2 to UTF-8 conversion failed");
	lua_pushstring(L, s);
	SDL_free(s);
	
	if (lua_gettop(L) > 2) {
		const char *utf8text;
		char *ucs2buf;
		SDL_iconv_t cd;
		size_t inbytesleft, outbytesleft;
		int i;
		int width, prevWidth;
		Uint16 savedChar;
		
		/*convert text to UCS-2*/
		utf8text = luaL_checkstring(L, 2);
		inbytesleft = lua_strlen(L, 2);
		outbytesleft = 2*te->maxlength;
		ucs2buf = (char*)(te->text);
		cd = SDL_iconv_open("UCS-2", "UTF-8");
		if (cd == (SDL_iconv_t)-1) luaL_error(L, "UTF-8 to UCS-2 conversion failed");
		SDL_iconv(cd, &utf8text, &inbytesleft, &ucs2buf, &outbytesleft);
		SDL_iconv_close(cd);
		te->length = te->maxlength - (int)outbytesleft/2;
		te->text[te->length] = 0;
		
		/*measure the advance width of each character (we can't use TTF_GlyphMetrics() due to kerning)*/
		prevWidth = 0;
		for (i = 0; i < te->length; i++) {
			savedChar = te->text[i+1];
			te->text[i+1] = 0;
			TTF_SizeUNICODE(verafont, te->text, &width, NULL);
			te->advance[i] = width - prevWidth;
			prevWidth = width;
			te->text[i+1] = savedChar;
		}
		
		te->selectionAnchor = te->selectionHead = te->length;
		te->textModified = 1;
		te->selectionModified = 1;
	}
	return 1;
}

static int texteditorPasteLua(lua_State *L) {
	TextEditor *te;
	Uint16 *start, *end;
	Uint8 *advstart, *advend;
	int selectionLength, tailLength, freeLength, pastedLength;
	SDL_iconv_t cd;
	const char *utf8text;
	char *ucs2out;
	size_t inbytesleft, outbytesleft;
	int width, prevWidth;
	int i;
	Uint16 savedChar;
	Uint16 *mstart;
	
	te = (TextEditor*)luaL_checkudata(L, 1, "pipmak-texteditor");
	if (te == NULL || te->text == NULL) luaL_typerror(L, 1, "texteditor");
	
	cd = SDL_iconv_open("UCS-2", "UTF-8");
	if (cd == (SDL_iconv_t)-1) luaL_error(L, "UTF-8 to UCS-2 conversion failed");
	
	/*divide the buffer into these parts: head, selection, tail (including terminating zero), free*/
	selectionLength = te->selectionAnchor - te->selectionHead;
	if (selectionLength >= 0) {
		start = te->text + te->selectionHead;
		end = te->text + te->selectionAnchor;
		advstart = te->advance + te->selectionHead;
		advend = te->advance + te->selectionAnchor;
	}
	else {
		selectionLength = -selectionLength;
		start = te->text + te->selectionAnchor;
		end = te->text + te->selectionHead;
		advstart = te->advance + te->selectionAnchor;
		advend = te->advance + te->selectionHead;
	}
	tailLength = (int)(te->text + te->length + 1 - end);
	freeLength = te->maxlength - te->length;
	
	/*move the tail out of the way to the end of the buffer*/
	if (freeLength != 0) {
		SDL_memmove(end + freeLength, end, 2*tailLength);
		SDL_memmove(advend + freeLength, advend, tailLength);
	}

	/*convert the text into where the selection was*/
	utf8text = luaL_checkstring(L, 2);
	ucs2out = (char*)start;
	inbytesleft = lua_strlen(L, 2);
	outbytesleft = 2*(selectionLength + freeLength);
	SDL_iconv(cd, &utf8text, &inbytesleft, &ucs2out, &outbytesleft);
	pastedLength = (int)((Uint16*)ucs2out - start);
	SDL_iconv_close(cd);
	
	/*move the tail back*/
	if (pastedLength != selectionLength + freeLength) {
		SDL_memmove(ucs2out, end + freeLength, 2*tailLength);
		SDL_memmove(advstart + pastedLength, advend + freeLength, tailLength);
	}
	
	/*measure the advance widths of the pasted text (plus one character, due to kerning)*/
	mstart = (start == te->text) ? start : start-1;
	savedChar = start[0];
	start[0] = 0;
	TTF_SizeUNICODE(verafont, mstart, &prevWidth, NULL);
	start[0] = savedChar;
	for (i = 0; i <= pastedLength; i++) {
		savedChar = start[i+1];
		start[i+1] = 0;
		TTF_SizeUNICODE(verafont, mstart, &width, NULL);
		advstart[i] = width - prevWidth;
		prevWidth = width;
		start[i+1] = savedChar;
	}
	
	te->length = te->length - selectionLength + pastedLength;
	te->selectionAnchor = te->selectionHead = (int)((Uint16*)ucs2out - te->text);
	te->textModified = 1;
	te->selectionModified = 1;
	
	return 0;
}

static int texteditorSelectionLua(lua_State *L) {
	TextEditor *te = (TextEditor*)luaL_checkudata(L, 1, "pipmak-texteditor");
	if (te == NULL || te->text == NULL) luaL_typerror(L, 1, "texteditor");
	
	if (te->selectionAnchor < te->selectionHead) {
		lua_pushnumber(L, te->selectionAnchor);
		lua_pushnumber(L, te->selectionHead);
	}
	else {
		lua_pushnumber(L, te->selectionHead);
		lua_pushnumber(L, te->selectionAnchor);
	}
	
	if (lua_gettop(L) > 3) {
		te->selectionAnchor = luaL_checkint(L, 2);
		te->selectionHead = (lua_gettop(L) > 4) ? luaL_checkint(L, 3) : te->selectionAnchor;
		if (te->selectionAnchor < 0) te->selectionAnchor += te->length + 1;
		if (te->selectionHead < 0) te->selectionHead += te->length + 1;
	}
	return 2;
}

static int texteditorMouseLua(lua_State *L) {
	TextEditor *te;
	float h, v;
	int x, i;
	
	te = (TextEditor*)luaL_checkudata(L, 1, "pipmak-texteditor");
	if (te == NULL || te->text == NULL) luaL_typerror(L, 1, "texteditor");
	focusTextEditor(te);
	thisCNode->mouseXYtoHV(thisCNode, mouseX, mouseY, &h, &v);
	x = (int)h - te->x + te->scroll;
	if (v < te->y) i = 0;
	else if (v >= te->y + te->h) i = te->length;
	else {
		for (i = 0; i < te->length && x > te->advance[i]/2; i++) x -= te->advance[i];
	}
	te->selectionHead = i;
	if (!(SDL_GetModState() & KMOD_SHIFT) && lua_toboolean(L, 2)) {
		te->selectionAnchor = i;
	}
	te->selectionModified = 1;
	return 0;
}

static int texteditorSetkeydownfuncLua(lua_State *L) {
	TextEditor *te = (TextEditor*)luaL_checkudata(L, 1, "pipmak-texteditor");
	if (te == NULL) luaL_typerror(L, 1, "texteditor");
	luaL_checktype(L, -1, LUA_TFUNCTION);
	if (te->keydownFuncRef != 0) luaL_unref(L, LUA_REGISTRYINDEX, te->keydownFuncRef);
	te->keydownFuncRef = luaL_ref(L, LUA_REGISTRYINDEX);
	return 0;
}

static int texteditorFocusLua(lua_State *L) {
	TextEditor *te = (TextEditor*)luaL_checkudata(L, 1, "pipmak-texteditor");
	if (te == NULL) luaL_typerror(L, 1, "texteditor");
	focusTextEditor(te);
	return 0;
}

const luaL_reg pipmakInternalTexteditorFuncs[] = {
	{"newtexteditor", newtexteditorLua},
	{NULL, NULL}
};

const luaL_reg texteditorFuncs[] = {
	{"__gc", texteditorDestroyLua},
	{"destroy", texteditorDestroyLua},
	{"text", texteditorTextLua},
	{"paste", texteditorPasteLua},
	{"selection", texteditorSelectionLua},
	{"mouse", texteditorMouseLua},
	{"setkeydownfunc", texteditorSetkeydownfuncLua},
	{"focus", texteditorFocusLua},
	{NULL, NULL}
};
