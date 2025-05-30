# lua-s7iso

[![License](http://img.shields.io/badge/License-MIT-brightgreen.svg)](LICENSE)

A lua module for communication to Siemens S7-PLCs

## Basic usage
Check [Demo.lua]

## Remarks

- lua-s7iso was compiled and tested with macOS 10.14, Windows 10 x64 and Ubuntu 20.04 x64.
- lua-s7iso is also available via LuaRocks.
- This module was inspired by [lua-snap7](https://github.com/srdgame/lua-snap7), but was written from scratch.
- lua-s7iso uses Snap7 (see below) for communication to PLCs, but does not use/provide each method from it. Rather, lua-s7iso aims to provide a easy-to-use interface.

## External dependencies (included)
- [Sol3 (sol2 v3.0) - a C++ <-> Lua API wrapper with advanced features and top notch performance]( https://github.com/ThePhD/sol2 )
- [Snap7 - Step7 Open Source Ethernet Communication Suite](http://snap7.sourceforge.net)
__