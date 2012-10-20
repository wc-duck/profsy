--[[
   DOC ME HERE ASWELL!.
   version 0.1, march, 2012

   Copyright (C) 2012- Fredrik Kihlander

   This software is provided 'as-is', without any express or implied
   warranty.  In no event will the authors be held liable for any damages
   arising from the use of this software.

   Permission is granted to anyone to use this software for any purpose,
   including commercial applications, and to alter it and redistribute it
   freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
      claim that you wrote the original software. If you use this software
      in a product, an acknowledgment in the product documentation would be
      appreciated but is not required.
   2. Altered source versions must be plainly marked as such, and must not be
      misrepresented as being the original software.
   3. This notice may not be removed or altered from any source distribution.

   Fredrik Kihlander
--]]

BUILD_PATH = "local"

platform = "linux_x86_64"
config   = "debug"

local settings = NewSettings() -- {}

settings.cc.includes:Add("include")
if family ~= "windows" then
	-- TODO: HAXXOR!!!
	settings.link.libpath:Add( '../shmup/local/debug/linux_x86_64' )

	settings.cc.flags:Add( "-Wconversion", "-Wextra", "-Wall", "-Werror", "-Wstrict-aliasing=2" )
	settings.link.libs:Add( 'gtest', 'pthread', 'rt' )
else
	platform = "winx64"
	
	-- TODO: HAXXOR!!!
	settings.link.flags:Add( "/NODEFAULTLIB:LIBCMT.LIB" );
	
	settings.link.libpath:Add("../wc_games/shmup/local/debug/winx64/")
	settings.link.libs:Add( 'gtest' )
	settings.cc.includes:Add("../wc_games/wc_engine/externals/gtest/include/")
	settings.cc.defines:Add("_ITERATOR_DEBUG_LEVEL=0")
end

local output_path = PathJoin( BUILD_PATH, PathJoin( config, platform ) )
local output_func = function(settings, path) return PathJoin(output_path, PathFilename(PathBase(path)) .. settings.config_ext) end
settings.cc.Output = output_func
settings.lib.Output = output_func
settings.link.Output = output_func

local objs  = Compile( settings, 'src/profsy.cpp' )
local lib   = StaticLibrary( settings, 'profsy', objs )

local test_objs  = Compile( settings, 'test/profsy_tests.cpp' )
local tests      = Link( settings, 'profsy_tests', test_objs, lib )