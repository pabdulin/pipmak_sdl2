/*
 
 platform.h, part of the Pipmak Game Engine
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

/* $Id: platform.h 130 2006-11-19 15:37:38Z cwalther $ */

#ifndef PLATFORM_H_SEEN
#define PLATFORM_H_SEEN

extern const char* lineEnding;

void initGUI();
int prependResourcesToPhysfsSearchpath();
void errorMessage(const char *message, ...);
char* saveGamePath();
char* newProjectPath();
char* openSavedGamePath();
char* openProjectPath();
void openFile(const char *project, const char *path);

#endif /*PLATFORM_H_SEEN*/
