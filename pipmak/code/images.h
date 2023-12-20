/*
 
 images.h, part of the Pipmak Game Engine
 Copyright (c) 2004-2006 Christian Walther
 
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

/* $Id: images.h 145 2007-01-03 21:10:49Z cwalther $ */

#ifndef IMAGES_H_SEEN
#define IMAGES_H_SEEN

#include "SDL.h"
#include "SDL_opengl.h"


typedef struct Image {
	int w, h;
	Uint8 bpp;
	Uint8 isFile;
	GLsizei textureWidth, textureHeight;
	GLuint textureID;
	int texrefcount;
	int cursorHotX, cursorHotY;
	char *path;
	struct Image *prev, *next;
	int datarefcount;
	int datasize;
	SDL_Surface *data;
	SDL_Color drawingColor;
} Image;

Uint8 *getImageData(Image *image, int inlua);
Image *feedImageToGL(Image *image);
void releaseImageFromGL(Image *img);

#endif /*IMAGES_H_SEEN*/
