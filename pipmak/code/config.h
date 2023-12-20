/*
 
 config.h, part of the Pipmak Game Engine
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

/* $Id: config.h 214 2008-11-21 20:58:23Z wgsilkie $ */

/*#define DEBUG*/
#define CLICK_DURATION 300
#define MOUSE_WARP_DURATION 200
#define IMAGE_CACHE_SIZE (20*1024*1024)

#ifdef DEBUG
#define IFDEBUG(x) x
#else
#define IFDEBUG(x)
#endif


#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define BYTEORDER_DEPENDENT_R4_MASK 0xFF000000
#define BYTEORDER_DEPENDENT_G4_MASK 0x00FF0000
#define BYTEORDER_DEPENDENT_B4_MASK 0x0000FF00
#define BYTEORDER_DEPENDENT_A4_MASK 0x000000FF
#define BYTEORDER_DEPENDENT_R3_MASK 0xFF0000
#define BYTEORDER_DEPENDENT_G3_MASK 0x00FF00
#define BYTEORDER_DEPENDENT_B3_MASK 0x0000FF
#define BYTEORDER_DEPENDENT_A3_MASK 0x000000
#else
#define BYTEORDER_DEPENDENT_R4_MASK 0x000000FF
#define BYTEORDER_DEPENDENT_G4_MASK 0x0000FF00
#define BYTEORDER_DEPENDENT_B4_MASK 0x00FF0000
#define BYTEORDER_DEPENDENT_A4_MASK 0xFF000000
#define BYTEORDER_DEPENDENT_R3_MASK 0x0000FF
#define BYTEORDER_DEPENDENT_G3_MASK 0x00FF00
#define BYTEORDER_DEPENDENT_B3_MASK 0xFF0000
#define BYTEORDER_DEPENDENT_A3_MASK 0x000000
#endif
#define BYTEORDER_DEPENDENT_RGBA_MASKS BYTEORDER_DEPENDENT_R4_MASK, BYTEORDER_DEPENDENT_G4_MASK, BYTEORDER_DEPENDENT_B4_MASK, BYTEORDER_DEPENDENT_A4_MASK
#define BYTEORDER_DEPENDENT_RGB_MASKS BYTEORDER_DEPENDENT_R3_MASK, BYTEORDER_DEPENDENT_G3_MASK, BYTEORDER_DEPENDENT_B3_MASK, BYTEORDER_DEPENDENT_A3_MASK
