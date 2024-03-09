# Pipmak SDL2

This is a "port" of original [Pipmak Game Engine](http://pipmak.sourceforge.net/) to SDL2.

## Building
Currently only Windows build is supported.
Assuming VS2022 is installed at default location.

1. Run `pipmak/misc/external.bat` to compile all required dependencies.
2. Run `pipmak/build.bat` to compile.

The result will be at `build\pipmak`.

## Known bugs/todos
1. Screen resize not working
   1. ✅ window resize
   2. fullscreen
      1. Reason known: getting list of resolutions is not implemented (changed significantly in SDL2 vs SDL 1.2)
      2. Subsequently fullscreen save/restore games is not supported
         1. see also `// TODO(pabdulin): check fullscreen saved not supported` comments for places to check/fix
      3. Improve: need to support borderless fullscreen instead probably

## Libraries used by project
1. SDL2
2. SDL2_image
3. SDL2_ttf
4. [gl3w](https://github.com/skaslev/gl3w.git)
5. [physfs](https://github.com/icculus/physfs.git)
6. xiph/ogg
7. xiph/vorbis
8. OpenAL
