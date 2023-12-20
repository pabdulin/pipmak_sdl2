/*
 
 terminal.h, part of the Pipmak Game Engine
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

/* $Id: terminal.h 121 2006-10-15 15:14:21Z cwalther $ */

#ifndef TERMINAL_H_SEEN
#define TERMINAL_H_SEEN

int initTerminal();
void quitTerminal();
void drawTerminal();
void terminalClear();
void terminalPrint(const char *text, int replace);
void terminalPrintf(const char *format, ...);
/*void terminalPrintfR(const char *format, ...);*/
const char *terminalLastLine();

#endif /*TERMINAL_H_SEEN*/
