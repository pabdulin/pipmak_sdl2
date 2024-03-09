/* makepatch
 *
 * Finds differences in a set of images and generates patches.
 *
 * Compile with "cc -o makepatch `sdl-config --cflags --libs` -lSDL_image makepatch.c"
 * (needs SDL and SDL_image)
 *
 * To do:
 * - a tolerance option to allow small nonzero differences
 * - customizable output filenames and numbering
 *
 * Christian Walther, 2004-06-27
 * Public Domain
 */

#include "SDL.h"
#include "SDL_image.h"

void quit(int status) {
	SDL_Quit();
	exit(status);
}

int main(int argc, char *argv[]) {
	if (argc >= 3) {
		
		SDL_Surface *images[argc-1];
		SDL_Surface *temp;
		SDL_Rect rect;
		int i, j, x, y;
		int minx, miny, maxx, maxy;
		char outfilename[20];

		if (SDL_Init(SDL_INIT_VIDEO) != 0) { 
			fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
			quit(1);
		}
		
		for (i = 0; i < argc-1; i++) {
			temp = IMG_Load(argv[i+1]);
			if (temp == NULL) {
				fprintf(stderr, "Could not load image %s: %s\n", argv[i+1], IMG_GetError());
				quit(1);
			}
			if (i > 0 && (temp->w != images[0]->w || temp->h != images[0]->h)) {
				fprintf(stderr, "%s has a different size than %s\n", argv[i+1], argv[1]);
				SDL_FreeSurface(temp);
				for (j = 0; j < i; j++) SDL_FreeSurface(images[j]);
				quit(1);
			}
			images[i] = SDL_CreateRGBSurface(SDL_SWSURFACE, temp->w, temp->h, 24, 0xFF0000, 0x00FF00, 0x0000FF, 0x000000);
			if (images[i] == NULL) {
				fprintf(stderr, "Could not create RGB surface: %s\n", SDL_GetError());
				SDL_FreeSurface(temp);
				for (j = 0; j < i; j++) SDL_FreeSurface(images[j]);
				quit(1);
			}
			SDL_BlitSurface(temp, NULL, images[i], NULL);
			SDL_FreeSurface(temp);
		}
		
		minx = images[0]->w;
		miny = images[0]->h;
		maxx = 0;
		maxy = 0;
		
		SDL_LockSurface(images[0]);
		for (i = 1; i < argc-1; i++) {
			SDL_LockSurface(images[i]);
			for (y = 0; y < images[0]->h; y++) {
				for (x = 0; x < images[0]->w; x++) {
					if (
						((Uint8*)(images[i]->pixels))[y*images[i]->pitch + 3*x + 0] != ((Uint8*)(images[0]->pixels))[y*images[0]->pitch + 3*x + 0] ||
						((Uint8*)(images[i]->pixels))[y*images[i]->pitch + 3*x + 1] != ((Uint8*)(images[0]->pixels))[y*images[0]->pitch + 3*x + 1] ||
						((Uint8*)(images[i]->pixels))[y*images[i]->pitch + 3*x + 2] != ((Uint8*)(images[0]->pixels))[y*images[0]->pitch + 3*x + 2]
					) {
						if (x < minx) minx = x;
						if (y < miny) miny = y;
						if (x > maxx) maxx = x;
						if (y > maxy) maxy = y;
					}
				}
			}
			SDL_UnlockSurface(images[i]);
		}
		SDL_UnlockSurface(images[0]);
		
		if (minx > maxx || miny > maxy) {
			printf("The images are identical.\n");
		}
		else {
			if (minx > 0) minx--;
			if (miny > 0) miny--;
			if (maxx < images[0]->w - 1) maxx++;
			if (maxy < images[0]->h - 1) maxy++;
			rect.x = minx;
			rect.y = miny;
			rect.w = maxx-minx+1;
			rect.h = maxy-miny+1;
			temp = SDL_CreateRGBSurface(SDL_SWSURFACE, maxx-minx+1, maxy-miny+1, 24, 0xFF0000, 0x00FF00, 0x0000FF, 0x000000);
			if (temp == NULL) {
				fprintf(stderr, "Could not create RGB surface: %s\n", SDL_GetError());
				for (j = 0; j < argc-1; j++) SDL_FreeSurface(images[j]);
				quit(1);
			}
			for (i = 0; i < argc-1; i++) {
				SDL_BlitSurface(images[i], &rect, temp, NULL);
				sprintf(outfilename, "patch%d.bmp", i);
				if (SDL_SaveBMP(temp, outfilename) != 0) {
					fprintf(stderr, "Could not save BMP file: %s", SDL_GetError());
				}
				else {
					printf("saved %s\n", outfilename);
				}
			}
			SDL_FreeSurface(temp);
			printf("x = %d, y = %d, w = %d, h = %d\n", minx, miny, maxx-minx+1, maxy-miny+1);
		}
		
		quit(0);
	}
	else {
		printf("Finds differences in a set of images and generates patches.\nUsage: %s image1 image2 [image3...]\n", argv[0]);
	}
	return 0;
}