/*
	This file is part of NaviLibrary, a library that allows developers to create and 
	interact with web-content as an overlay or material in Ogre3D applications.

	Copyright (C) 2011 Khrona LLC
	https://github.com/khrona/navi

	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Lesser General Public
	License as published by the Free Software Foundation; either
	version 2.1 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __NaviPlatform_H__
#define __NaviPlatform_H__
#if _MSC_VER > 1000
#pragma once
#endif

#if defined(__WIN32__) || defined(_WIN32)
#	if defined(NAVI_NONCLIENT_BUILD)
#		define _NaviExport __declspec( dllexport )
#	else
#		define _NaviExport __declspec( dllimport )
#	endif
#pragma warning (disable : 4251)
#elif defined(__APPLE__)
#	define _NaviExport __attribute__((visibility("default")))
#else
#	define _NaviExport
#endif

#endif