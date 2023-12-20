/*
 
 opensave.h, part of the Pipmak Game Engine
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

/* $Id: opensave.h 65 2006-03-27 14:52:31Z cwalther $ */

#ifndef OPENSAVE_H_SEEN
#define OPENSAVE_H_SEEN

int openProject(const char *filename);
int openAndEnterProject(const char *filename);
int openSavedGame(const char *filename);

#endif /*OPENSAVE_H_SEEN*/
