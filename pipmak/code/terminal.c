/*
 
 terminal.c, part of the Pipmak Game Engine
 Copyright (c) 2004-2011 Christian Walther
 
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

/* $Id: terminal.c 228 2011-04-30 19:40:44Z cwalther $ */

#include "terminal.h"

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "SDL.h"
#include "SDL_opengl.h"
#include "SDL_ttf.h"
#include "physfs.h"

#include "misc.h"
#include "platform.h"
#include "config.h"
extern int cpTermToStdout;


typedef struct TerminalLine {
	GLuint textureID;
	int width, height;
	int indent;
	Uint32 time;
	struct TerminalLine *next;
} TerminalLine;

extern GLenum glTextureTarget;

static unsigned char *verafontdata = NULL;
TTF_Font *verafont;
static TerminalLine *firstLine;
static char lineBuffer[300];

int initTerminal() {
	int n;
	PHYSFS_file *f;
	SDL_RWops *rw;
	if (TTF_Init() != 0) {
		printf("TTF_Init: %s\n", TTF_GetError());
		return 0;
	}
	
	/* We must keep the font file in memory, because freetype needs to have it
		open all the time. We can't guarantee that through PhysicsFS, because the
		resources item in its search path is sometimes (e.g. on opening a new
		project) closed and re-opened.
		*/
	
	f = PHYSFS_openRead("resources/Vera.ttf");
	if (f == NULL) {
		printf("Couldn't read font file: %s\n", PHYSFS_getLastError());
		TTF_Quit();
		return 0;
	}
	n = (int)PHYSFS_fileLength(f);
	verafontdata = malloc(n);
	if (verafontdata == NULL) {
		printf("Couldn't allocate memory for font data.\n");
		PHYSFS_close(f);
		TTF_Quit();
		return 0;
	}
	PHYSFS_read(f, verafontdata, 1, n);
	PHYSFS_close(f);
	
	rw = SDL_RWFromConstMem(verafontdata, n);
	if (rw == NULL) {
		printf("Couldn't create RWops: %s\n", SDL_GetError());
		TTF_Quit();
		return 0;
	}
	
	verafont = TTF_OpenFontRW(rw, 1, 11);
	if (verafont == NULL) {
		printf("TTF_OpenFontRW: %s\n", TTF_GetError());
		TTF_Quit();
		return 0;
	}
	firstLine = NULL;
	return 1;
}

void quitTerminal() {
	terminalClear();
	if (TTF_WasInit()) {
		if (verafont != NULL) TTF_CloseFont(verafont);
		TTF_Quit();
	}
	if (verafontdata != NULL) free(verafontdata);
}

void terminalClear() {
	TerminalLine *line, *nextLine;
	line = firstLine;
	while (line != NULL) {
		nextLine = line->next;
		glDeleteTextures(1, &(line->textureID));
		free(line);
		line = nextLine;
	}
	firstLine = NULL;
}

void drawTerminal(Uint32 now) {
	TerminalLine *line;
	GLuint originalTexture;
	GLfloat y = -18;
	if (firstLine != NULL) {
		glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&originalTexture);
		if (now - firstLine->time > 10000 + 500) {
			glDeleteTextures(1, &(firstLine->textureID));
			line = firstLine->next;
			free(firstLine);
			firstLine = line;
		}
	}
	line = firstLine;
	if (glTextureTarget == GL_TEXTURE_RECTANGLE_NV) glDisable(GL_TEXTURE_RECTANGLE_NV);
	while (line != NULL) {
		y += 18*(1 - smoothCubic((float)((int)(now - line->time) - 10000)/500));
		glBindTexture(GL_TEXTURE_2D, line->textureID);
		glBegin(GL_QUADS);
		glTexCoord2f(0, 0);
		glVertex2f((GLfloat)(4 + line->indent), y);
		glTexCoord2f(1, 0);
		glVertex2f((GLfloat)(4 + line->indent + line->width), y);
		glTexCoord2f(1, 1);
		glVertex2f((GLfloat)(4 + line->indent + line->width), y + line->height);
		glTexCoord2f(0, 1);
		glVertex2f((GLfloat)(4 + line->indent), y + line->height);
		glEnd();
		line = line->next;
	}
	if (glTextureTarget == GL_TEXTURE_RECTANGLE_NV) glEnable(GL_TEXTURE_RECTANGLE_NV);
	if (firstLine != NULL) glBindTexture(GL_TEXTURE_2D, originalTexture);
}

static void terminalAddLine(const char *text, int replace) {
#define KERNEL_RADIUS 2
	SDL_Color white;
	SDL_Surface *whiteText, *output;
	SDL_Rect rect;
	TerminalLine *line, *lastLine;
	GLuint originalTexture;
	int x, y, i, j, textwidth, textheight, outwidth, outheight, indent;
	float alpha;
	float kernel[2*KERNEL_RADIUS+1][2*KERNEL_RADIUS+1] = {
		{0.01f, 0.12f, 0.18f, 0.12f, 0.01f},
		{0.06f, 0.44f, 1.20f, 0.44f, 0.06f},
		{0.06f, 0.54f, 2.00f, 0.54f, 0.06f},
		{0.04f, 0.24f, 0.54f, 0.24f, 0.04f},
		{0.00f, 0.02f, 0.04f, 0.02f, 0.00f}
	};
	if (text == NULL || text[0] == 0) {
		return;
	}
	if (cpTermToStdout) {
	   /* Print the text to standard out as well. */
	   printf("Term: %s\n", text);
	}
	indent = 0;
	while (*text == '\t') {
		text++;
		indent += 20;
	}
	white.r = white.g = white.b = 255;
	whiteText = TTF_RenderUTF8_Blended(verafont, text, white);
	if (whiteText == NULL) {
		errorMessage("Error trying to render the text '%s': %s\n", text, TTF_GetError());
		return;
	}
	textwidth = whiteText->w;
	textheight = whiteText->h;
	outwidth = 1;
	while (outwidth < textwidth+2*KERNEL_RADIUS) outwidth *= 2;
	outheight = 1;
	while (outheight < textheight+2*KERNEL_RADIUS) outheight *= 2;
	output = SDL_CreateRGBSurface(SDL_SWSURFACE, outwidth, outheight, 32, BYTEORDER_DEPENDENT_RGBA_MASKS);
	if (output == NULL) {
		printf("SDL_CreateRGBSurface: %s\n", SDL_GetError());
		SDL_FreeSurface(whiteText);
		return;
	}
	SDL_FillRect(output, NULL, 0x00000000);
	/*SDL_FillRect(output, NULL, 0x0000FF80);*/
	rect.x = KERNEL_RADIUS;
	rect.y = KERNEL_RADIUS;
	SDL_BlitSurface(whiteText, NULL, output, &rect);
	SDL_FreeSurface(whiteText);
	SDL_LockSurface(output);
	for (y = 0; y < textheight+2*KERNEL_RADIUS; y++) {
		for (x = 0; x < textwidth+2*KERNEL_RADIUS; x++) {
			alpha = 0;
			for (i = -KERNEL_RADIUS; i <= KERNEL_RADIUS; i++) {
				for (j = -KERNEL_RADIUS; j <= KERNEL_RADIUS; j++) {
					if (x+i >= 0 && y+j >= 0 && x+i < textwidth+2*KERNEL_RADIUS && y+j < textheight+2*KERNEL_RADIUS) {
						alpha += *((unsigned char *)output->pixels + (y+j)*output->pitch + (x+i)*4 + 0) * kernel[j+KERNEL_RADIUS][i+KERNEL_RADIUS];
					}
				}
			}
			if (alpha > 255) alpha = 255;
			*((unsigned char *)output->pixels + y*output->pitch + x*4 + 3) = (unsigned char)alpha;
		}
	}
	line = malloc(sizeof(TerminalLine));
	if (line != NULL) {
		line->width = outwidth;
		line->height = outheight;
		line->indent = indent;
		line->next = NULL;
		line->time = SDL_GetTicks();
		glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&originalTexture);
		glGenTextures(1, &(line->textureID));
		glBindTexture(GL_TEXTURE_2D, line->textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, outwidth, outheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, output->pixels);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, originalTexture);
		if (firstLine == NULL) {
			firstLine = line;
		}
		else if (replace) {
			if (firstLine->next == NULL) {
				glDeleteTextures(1, &(firstLine->textureID));
				free(firstLine);
				firstLine = line;
			}
			else {
				lastLine = firstLine;
				while(lastLine->next->next != NULL) lastLine = lastLine->next;
				glDeleteTextures(1, &(lastLine->next->textureID));
				free(lastLine->next);
				lastLine->next = line;
			}
		}
		else {
			lastLine = firstLine;
			while (lastLine->next != NULL) lastLine = lastLine->next;
			lastLine->next = line;
		}
	}
	else {
		printf("Out of Memory\n");
	}
	SDL_UnlockSurface(output);
	SDL_FreeSurface(output);
}

void terminalPrint(const char *text, int replace) {
	char *bp;
	while (*text != '\0') {
		while (*text == '\r' || *text == '\n') text++;
		bp = lineBuffer;
		while (*text != '\r' && *text != '\n' && *text != '\0' && bp < lineBuffer + sizeof(lineBuffer) - 1) *(bp++) = *(text++);
		*bp = '\0';
		terminalAddLine(lineBuffer, replace);
	}
}

void terminalPrintf(const char *format, ...) {
	char buffer[1024];
	va_list ap;
	va_start(ap, format);
	vsnprintf(buffer, 1024, format, ap);
	terminalPrint(buffer, 0);
	va_end(ap);
}

/*void terminalPrintfR(const char *format, ...) {
	char buffer[1024];
	va_list ap;
	va_start(ap, format);
	vsnprintf(buffer, 1024, format, ap);
	terminalPrint(buffer, 1);
	va_end(ap);
}*/

const char *terminalLastLine() {
	return lineBuffer;
}
