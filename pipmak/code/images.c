/*
 
 images.c, part of the Pipmak Game Engine
 Copyright (c) 2004-2007 Christian Walther
 
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

/* $Id: images.c 166 2007-12-31 21:17:41Z cwalther $ */

#include "images.h"

#include <stdlib.h>
#include <assert.h>

#include "SDL.h"
#include "SDL_image.h"
#include "lua.h"
#include "lauxlib.h"
#include "physfs.h"
#include "physfsrwops.h"

#include "terminal.h"
#include "config.h"


extern lua_State *L;
extern Image *imageListStart;
extern GLenum glTextureTarget;
extern Uint8 controlColorPalette[256][3];


/*
 * Return a pointer to the pixel data of image, which must have at least its
 * path filled in. If the data is not in the cache, load it from disk and
 * cache it (potentially deleting others from the cache), and fill in the rest
 * of the Image fields. If loading the image fails, load "missing.png" instead
 * and complain on the Pipmak terminal. If inlua is true, use Lua error
 * functions in case of fatal errors, otherwise report them on the Pipmak
 * terminal and return NULL.
 */
Uint8 *getImageData(Image *image, int inlua) {
	Image *listimage;
	SDL_RWops *rw;
	SDL_Surface *surf, *datasurf;
	int i;
	
	if (image->data == NULL) {
		assert(image->isFile);
		
		IFDEBUG(printf("debug: loading image %s\n", image->path);)
		rw = PHYSFSRWOPS_openRead(image->path);
		if (rw == NULL) {
			terminalPrintf("Error loading image \"%s\": %s", image->path, SDL_GetError());
			rw = PHYSFSRWOPS_openRead("resources/missing.png");
		}
		surf = IMG_Load_RW(rw, 1);
		if (surf == NULL) {
			terminalPrintf("Error loading image \"%s\": %s", image->path, IMG_GetError());
			rw = PHYSFSRWOPS_openRead("resources/missing.png");
			surf = IMG_Load_RW(rw, 1);
			if (surf == NULL) {
				terminalPrintf("Pipmak internal error: IMG_Load: %s", IMG_GetError());
				return NULL;
			}
		}

		image->w = surf->w;
		image->h = surf->h;
		image->bpp = surf->format->BitsPerPixel;
		
		datasurf = surf;
		switch (image->bpp) {
			case 8:
				if (surf->pitch != ((image->w + 3) & ~3)) {
					datasurf = SDL_CreateRGBSurface(SDL_SWSURFACE, image->w, image->h, 8, 0, 0, 0, 0);
				}
				break;
			case 32:
				if (
					surf->pitch != ((4*image->w + 3) & ~3)
					|| surf->format->Rmask != BYTEORDER_DEPENDENT_R4_MASK
					|| surf->format->Gmask != BYTEORDER_DEPENDENT_G4_MASK
					|| surf->format->Bmask != BYTEORDER_DEPENDENT_B4_MASK
					|| surf->format->Amask != BYTEORDER_DEPENDENT_A4_MASK
				) {
					datasurf = SDL_CreateRGBSurface(SDL_SWSURFACE, image->w, image->h, 32, BYTEORDER_DEPENDENT_RGBA_MASKS);
				}
				break;
			case 24:
			default:
				if (
					surf->pitch != ((3*image->w + 3) & ~3)
					|| surf->format->Rmask != BYTEORDER_DEPENDENT_R3_MASK
					|| surf->format->Gmask != BYTEORDER_DEPENDENT_G3_MASK
					|| surf->format->Bmask != BYTEORDER_DEPENDENT_B3_MASK
				) {
					datasurf = SDL_CreateRGBSurface(SDL_SWSURFACE, image->w, image->h, 24, BYTEORDER_DEPENDENT_RGB_MASKS);
				}
				break;
		}
		if (datasurf != surf) {
			IFDEBUG(terminalPrint("debug: unsuitable surface format, blitting", 0);)
			if (datasurf == NULL) {
				if (inlua) luaL_error(L, "Pipmak internal error: Could not create RGB surface: %s", SDL_GetError());
				else terminalPrintf("Pipmak internal error: Could not create RGB surface: %s", SDL_GetError());
				return NULL;
			}

			// TODO(pabdulin): fix#10 SDL_SetColors, see: https://stackoverflow.com/questions/29609544/how-to-use-palettes-in-sdl-2
			if (image->bpp == 8) //SDL_SetColors(datasurf, surf->format->palette->colors, 0, surf->format->palette->ncolors);
				SDL_SetPaletteColors(datasurf->format->palette, surf->format->palette->colors, 0, surf->format->palette->ncolors);
			
			// TODO(pabdulin): fix#11 SDL_SetAlpha
			SDL_SetSurfaceAlphaMod(surf, 255);
			// SDL_SetAlpha(surf, 0, 255); /*ignore alpha when blitting, just copy it over*/
			SDL_BlitSurface(surf, NULL, datasurf, NULL);
			SDL_FreeSurface(surf);
		}
		
		image->data = datasurf;
		image->datasize = datasurf->h*datasurf->pitch;
		
		/*clean out cache*/
		listimage = imageListStart;
		i = 0;
		image->datarefcount = 1;
		while (listimage != NULL) {
			if (listimage->datarefcount == 0 && listimage->isFile) {
				i += listimage->datasize;
				if (i > IMAGE_CACHE_SIZE && listimage->data != NULL) {
					IFDEBUG(printf("debug: cleaning data of image %s from cache\n", listimage->path);)
					SDL_FreeSurface(listimage->data);
					listimage->data = NULL;
				}
			}
			listimage = listimage->next;
		}
		image->datarefcount = 0;
	}
	else {
		IFDEBUG(printf("debug: got data for image %s from cache\n", image->path);)
	}
	return image->data->pixels;
}

/*
 * Upload an image to OpenGL if it isn't already there and return a pointer to
 * it. Either pass the image directly in <image>, or set <image> to NULL to
 * pass it on the Lua stack (it will be left there). The image will be the
 * currently bound texture afterwards, so that glTexParameteri(glTextureTarget,
 * ...) can be used on it.
 */
Image *feedImageToGL(Image *image) {
	Uint8 *data;
	int i;
	
	if (image == NULL) {
		image = (Image*)luaL_checkudata(L, -1, "pipmak-image");
		if (image == NULL) {
			terminalPrintf("Pipmak internal error: feedImageToGL: image expected, got %s", lua_typename(L, lua_type(L, -1)));
			return NULL;
		}
	}
	image->texrefcount++;
	if (glIsTexture(image->textureID)) {
		glBindTexture(glTextureTarget, image->textureID);
	}
	else {
		glGenTextures(1, &(image->textureID));
		glBindTexture(glTextureTarget, image->textureID);
		
		IFDEBUG(printf("debug: feeding image %s to GL, %d bpp, texture ID %d\n", image->path, image->bpp, (int)image->textureID);)
		data = getImageData(image, 0);
		if (data != NULL) {
			if (glTextureTarget == GL_TEXTURE_RECTANGLE_NV) {
				image->textureWidth = 1;
				image->textureHeight = 1;
				switch (image->bpp) {
					case 8: {
						Uint8 *ndata;
						Uint8 px;
						int j, k;
						/*must convert the data ourselves because rectangle textures do not support paletted formats*/
						ndata = malloc(4*image->w*image->h);
						if (ndata != NULL) {
							for (j = 0; j < image->h; j++) {
								for (i = 0; i < image->w; i++) {
									px = data[j*((image->w + 3) & ~3) + i];
									k = 4*(j*image->w + i);
									ndata[k + 0] = controlColorPalette[px][0];
									ndata[k + 1] = controlColorPalette[px][1];
									ndata[k + 2] = controlColorPalette[px][2];
									ndata[k + 3] = (px == 0) ? 0 : 128;
								}
							}
							glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA, image->w, image->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, ndata);
							free(ndata);
						}
					} break;
					case 32:
						glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA, image->w, image->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
						break;
					case 24:
					default:
						glTexImage2D(GL_TEXTURE_RECTANGLE_NV, 0, GL_RGBA, image->w, image->h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
						break;
				}
			}
			else {
				i = 1;
				while (i < image->w) i *= 2;
				image->textureWidth = i;
				i = 1;
				while (i < image->h) i*= 2;
				image->textureHeight = i;
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); /* this works around an OpenGL driver bug on at least Mac OS X 10.4.8, ATI X1600 (MacBookPro2,2) - see http://lists.apple.com/archives/mac-opengl/2006/Dec/msg00012.html */
				switch (image->bpp) {
					case 8:
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->textureWidth, image->textureHeight, 0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, NULL);
						glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image->w, image->h, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, data);
						break;
					case 32:
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->textureWidth, image->textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
						glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image->w, image->h, GL_RGBA, GL_UNSIGNED_BYTE, data);
						break;
					case 24:
					default:
						glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->textureWidth, image->textureHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
						glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image->w, image->h, GL_RGB, GL_UNSIGNED_BYTE, data);
						break;
				}
			}
			glTexParameteri(glTextureTarget, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(glTextureTarget, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
	}
	
	return image;
}

void releaseImageFromGL(Image *img) {
	img->texrefcount--;
	if (img->texrefcount <= 0) {
		glDeleteTextures(1, &(img->textureID));
		img->textureID = 0;
	}
}
