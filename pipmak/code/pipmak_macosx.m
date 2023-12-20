/*
 
 pipmak_macosx.m, part of the Pipmak Game Engine
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

/* $Id: pipmak_macosx.m 130 2006-11-19 15:37:38Z cwalther $ */

#include "platform.h"
#import <Cocoa/Cocoa.h>
#include <stdarg.h>
#include <string.h>
#import "SDLMain.h"
#include "physfs.h"
#include "SDL.h"
#include "terminal.h"


const char* lineEnding = "\n";

void initGUI() {
	// nothing to do - Cocoa is already up and running because SDL runs on it
}

int prependResourcesToPhysfsSearchpath() {
	return PHYSFS_addToSearchPath([[[NSBundle mainBundle] resourcePath] UTF8String], 0);
}

void errorMessage(const char *message, ...) {
	char buffer[1024];
	va_list ap;
	va_start(ap, message);
	vsnprintf(buffer, 1024, message, ap);
	NSRunAlertPanel(nil, [NSString stringWithCString: buffer], nil, nil, nil);
	va_end(ap);
}

char* saveGamePath() {
	char *path;
	const char *t;
	NSSavePanel *savepanel = [NSSavePanel savePanel];
	[savepanel setCanSelectHiddenExtension: YES];
	[savepanel setRequiredFileType: @"pipsave"];
	[savepanel setTitle: @"Save Game"];
	if ([savepanel runModalForDirectory:nil file:@"Saved Game"] == NSFileHandlingPanelOKButton) {
		t = [[savepanel filename] UTF8String];
		path = malloc(strlen(t)+1);
		if (path != NULL) strcpy(path, t);
	}
	else {
		path = NULL;
	}
	return path;
}

char* newProjectPath() {
	char *path;
	const char *t;
	NSSavePanel *savepanel = [NSSavePanel savePanel];
	[savepanel setCanSelectHiddenExtension: YES];
	[savepanel setRequiredFileType: @"pipmak"];
	[savepanel setTitle: @"New Project"];
	if ([savepanel runModalForDirectory:nil file:@"untitled"] == NSFileHandlingPanelOKButton) {
		t = [[savepanel filename] UTF8String];
		path = malloc(strlen(t)+1);
		if (path != NULL) strcpy(path, t);
	}
	else {
		path = NULL;
	}
	return path;
}

char* openSavedGamePath() {
	char *path;
	const char *t;
	NSOpenPanel *openpanel = [NSOpenPanel openPanel];
	[openpanel setTitle: @"Open Saved Game"];
	if ([openpanel runModalForTypes: [NSArray arrayWithObjects: @"pipsave", NSFileTypeForHFSTypeCode(0xB8504D53), nil]] == NSOKButton) {
		t = [[[openpanel filenames] objectAtIndex: 0] UTF8String];
		path = malloc(strlen(t)+1);
		if (path != NULL) strcpy(path, t);
	}
	else {
		path = NULL;
	}
	return path;
}

char* openProjectPath() {
	char *path;
	const char *t;
	NSOpenPanel *openpanel = [NSOpenPanel openPanel];
	[openpanel setTitle: @"Open Project"];
	if ([openpanel runModalForTypes: [NSArray arrayWithObjects: @"pipmak", nil]] == NSOKButton) {
		t = [[[openpanel filenames] objectAtIndex: 0] UTF8String];
		path = malloc(strlen(t)+1);
		if (path != NULL) strcpy(path, t);
	}
	else {
		path = NULL;
	}
	return path;
}

void openFile(const char *project, const char *path) {
	if (![[NSWorkspace sharedWorkspace] openFile: [NSString stringWithFormat: @"%s/%s", project, path]]) {
		terminalPrintf("Could not open %s/%s", project, path);
	}
}
