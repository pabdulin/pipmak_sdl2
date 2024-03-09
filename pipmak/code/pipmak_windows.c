/*
 
 pipmak_windows.c, part of the Pipmak Game Engine
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

/* $Id: pipmak_windows.c 158 2007-07-23 13:50:33Z cwalther $ */

#include "platform.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "physfs.h"
#include "SDL.h"
#include "terminal.h"
#include <windows.h>
#include <shlobj.h>

#ifdef _MSC_VER
#define snprintf _snprintf  /* because snprintf is deprecated */
#endif


const char* lineEnding = "\r\n";


static int registryFileAssociation(void);


void initGUI() {
	registryFileAssociation();
}

int prependResourcesToPhysfsSearchpath() {
	if (PHYSFS_addToSearchPath("pipmak_data", 0)) return 1;
	if (PHYSFS_addToSearchPath("Pipmak Resources", 0)) return 1;
	return 0;
}

void FlushMessageQueue() {
/* I have no idea what this function does, but it seems to solve the problem
 * that MessageBox()es sometimes aren't shown after SDL has been initialized.
 * -Christian
 * Source: http://article.gmane.org/gmane.comp.lib.sdl/821
 * (John Popplewell, 22 Jul 2002)
 */
	MSG  msg;
	
	while ( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
	{
		if ( msg.message == WM_QUIT ) break;
		TranslateMessage( &msg );
		DispatchMessage( &msg );
	}
}

void errorMessage(const char *message, ...) {
	char buffer[1024];
	va_list ap;
	va_start(ap, message);
	vsnprintf(buffer, 1024, message, ap);
	FlushMessageQueue();
	MessageBox(NULL, buffer, "Alert", MB_OK);
	va_end(ap);
}

char* saveGamePath() {
	OPENFILENAME ofn;
	
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = malloc(1024);
	if (ofn.lpstrFile == NULL) return NULL;
	strcpy(ofn.lpstrFile, "Saved Game.pipsave");
	ofn.nMaxFile = 1024;
	ofn.lpstrFilter = "Pipmak Saved Games\0*.pipsave\0All\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrTitle = "Save Game As";
	ofn.lpstrDefExt = "pipsave";
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
	
	FlushMessageQueue();
	if (GetSaveFileName(&ofn)) {
		return ofn.lpstrFile;
	}
	else {
		free(ofn.lpstrFile);
		return NULL;
	}
}

char* newProjectPath() {
	OPENFILENAME ofn;
	
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = malloc(1024);
	if (ofn.lpstrFile == NULL) return NULL;
	strcpy(ofn.lpstrFile, "untitled.pipmak");
	ofn.nMaxFile = 1024;
	ofn.lpstrFilter = "Pipmak Projects\0*.pipmak\0All\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrTitle = "New Project";
	ofn.lpstrDefExt = "pipmak";
	ofn.Flags = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
	
	FlushMessageQueue();
	if (GetSaveFileName(&ofn)) {
		return ofn.lpstrFile;
	}
	else {
		free(ofn.lpstrFile);
		return NULL;
	}
}

char* openSavedGamePath() {
	OPENFILENAME ofn;
	
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = malloc(1024);
	if (ofn.lpstrFile == NULL) return NULL;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = 1024;
	ofn.lpstrFilter = "Pipmak Saved Games\0*.pipsave\0All\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrTitle = "Open Saved Game";
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
	
	FlushMessageQueue();
	if (GetOpenFileName(&ofn)) {
		return ofn.lpstrFile;
	}
	else {
		free(ofn.lpstrFile);
		return NULL;
	}
}

char* openProjectPath() {
	OPENFILENAME ofn;
	
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = malloc(1024);
	if (ofn.lpstrFile == NULL) return NULL;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = 1024;
	ofn.lpstrFilter = "Pipmak Projects\0*.pipmak;main.lua\0All\0*.*\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrTitle = "Open a project or its main.lua";
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_NOCHANGEDIR;
	
	FlushMessageQueue();
	if (GetOpenFileName(&ofn)) {
		return ofn.lpstrFile;
	}
	else {
		free(ofn.lpstrFile);
		return NULL;
	}
}

void openFile(const char *project, const char *path) {
	char fullpath[1024];
	int res;
	fullpath[1023] = '\0'; /*Windows' snprintf doesn't null-terminate*/
	snprintf(fullpath, 1023, "%s\\%s", project, path);
	/*try "edit" verb*/
	res = (int)ShellExecute(NULL, "edit", fullpath, NULL, NULL, SW_SHOWNORMAL);
	if (res == SE_ERR_NOASSOC) {
		/*no "edit" verb defined, try default verb*/
		res = (int)ShellExecute(NULL, NULL, fullpath, NULL, NULL, SW_SHOWNORMAL);
		if (res == SE_ERR_NOASSOC) {
			/*no file association, try notepad*/
			res = (int)ShellExecute(NULL, NULL, "notepad.exe", fullpath, NULL, SW_SHOWNORMAL);
		}
	}
	if (res <= 32) {
		DWORD attr = GetFileAttributes(project);
		if (attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
			terminalPrintf("Could not open %s because %s is an archive, not a folder. Unarchive it to be able to edit files.", path, project);
		}
		else {
			terminalPrintf("Could not open %s (%d)", fullpath, res);
		}
	}
}

static void registryKeyWrite(HKEY hkey_class, unsigned char *key_path, unsigned char *key_name, unsigned char *key_value) {
	HKEY hKey;
	unsigned long dwDisp; /* could be REG_CREATED_NEW_KEY or REG_OPENED_EXISTING_KEY */

	RegCreateKeyEx(hkey_class, key_path, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &dwDisp);
	RegSetValueEx(hKey, key_name, 0, REG_SZ, key_value, (DWORD)strlen(key_value));
	RegCloseKey(hKey);
}

static int registryKeyRead(HKEY hkey_class, unsigned char *key_path, unsigned char *key_name, unsigned char *key_value) {
	HKEY hKey;
	unsigned char buffer[_MAX_PATH];
	unsigned long datatype;
	unsigned long bufferlength = sizeof(buffer);

	if (RegOpenKeyEx(hkey_class, key_path, 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
		RegQueryValueEx(hKey, key_name, NULL, &datatype, buffer, &bufferlength);
		strcpy(key_value, buffer);
		RegCloseKey(hKey);
		return 1;
	}
	else return 0;
}

static int registryFileAssociation(void) {
	unsigned char buffer[MAX_PATH];
	unsigned char path[MAX_PATH + 1];
	unsigned char proj_icon[MAX_PATH+2];
	unsigned char save_icon[MAX_PATH+2];
	unsigned char pipmak_cmd[MAX_PATH+5];
	int res1, res2, res3;

	GetModuleFileName(NULL, path, MAX_PATH + 1);
	sprintf(pipmak_cmd, "%s \"%%1\"", path);

	res1 = registryKeyRead(HKEY_CLASSES_ROOT, ".pipmak", "", buffer);
	res2 = registryKeyRead(HKEY_CLASSES_ROOT, ".pipsave", "", buffer);
	res3 = registryKeyRead(HKEY_CLASSES_ROOT, "PipmakProj\\Shell\\Open\\Command", "", buffer);

	if (res1*res2*res3 == 0 || strcmp(buffer, pipmak_cmd) != 0) {
		sprintf(proj_icon, "%s,1", path);
		sprintf(save_icon, "%s,2", path);

		registryKeyWrite(HKEY_CLASSES_ROOT, "PipmakProj", "", path);
		registryKeyWrite(HKEY_CLASSES_ROOT, "PipmakProj\\DefaultIcon", "", proj_icon);
		registryKeyWrite(HKEY_CLASSES_ROOT, "PipmakProj\\Shell", "", "");
		registryKeyWrite(HKEY_CLASSES_ROOT, "PipmakProj\\Shell\\Open", "", "");
		registryKeyWrite(HKEY_CLASSES_ROOT, "PipmakProj\\Shell\\Open\\Command", "", pipmak_cmd);
		registryKeyWrite(HKEY_CLASSES_ROOT, ".pipmak", "", "PipmakProj");

		registryKeyWrite(HKEY_CLASSES_ROOT, "PipmakSavedGame", "", path);
		registryKeyWrite(HKEY_CLASSES_ROOT, "PipmakSavedGame\\DefaultIcon", "", save_icon);
		registryKeyWrite(HKEY_CLASSES_ROOT, "PipmakSavedGame\\Shell", "", "");
		registryKeyWrite(HKEY_CLASSES_ROOT, "PipmakSavedGame\\Shell\\Open", "", "");
		registryKeyWrite(HKEY_CLASSES_ROOT, "PipmakSavedGame\\Shell\\Open\\Command", "", pipmak_cmd);
		registryKeyWrite(HKEY_CLASSES_ROOT, ".pipsave", "", "PipmakSavedGame");
		
		/* Reload Icon association */
		SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, 0, 0);
		return 1;
	}
	return 0;
}
