This folder contains various odds and ends related to Pipmak.

hotspot-palette.gif:
This image contains a color palette with easily distinguishable colors suited for drawing hotspot maps.

luaplugin:
An example Lua plugin that defines a function and a class with a method, and uses the Pipmak terminal, to be loaded using loadlib(pathtotheplugin, "init")().

makepatch.c:
A small command-line utility to compare images and make patches. See the comments at the beginning of the file.

serialize.lua:
Lua serialization functions, used for saving games in Pipmak. They were specifically written for Pipmak, but may also be useful for other purposes. This file is not directly used for Pipmak, there is a copy of the serialization functions in 'defaults.lua'.