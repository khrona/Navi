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

/*
	Portions Copyright (C) Scott Bilas, 2000 and Steve Streeting, 2007
*/

#ifndef __NaviSingleton_H__
#define __NaviSingleton_H__
#if _MSC_VER > 1000
#pragma once
#endif

#include <OGRE/OgreException.h>

namespace NaviLibrary {

	template <typename Class> class Singleton
    {
    protected:
        static Class* instance;

    public:
        Singleton()
        {
			if(instance)
				OGRE_EXCEPT(Ogre::Exception::ERR_RT_ASSERTION_FAILED, 
					"An attempt was made to re-instantiate a NaviLibrary::Singleton that has already been instantiated!", 
					typeid(*this).name());

#if _MSC_VER < 1200	 
            instance = (class*)((int)this + ((int)(Class*)1 - (int)(Singleton <Class>*)(Class*)1));
#else
			instance = static_cast<Class*>(this);
#endif
        }

        ~Singleton()
		{
			if(!instance)
				OGRE_EXCEPT(Ogre::Exception::ERR_RT_ASSERTION_FAILED, 
					"An attempt was made to destroy a NaviLibrary::Singleton that has not been instantiated!", 
					typeid(*this).name());

			instance = 0;
		}

		static Class& Get()
		{
			if(!instance)
				OGRE_EXCEPT(Ogre::Exception::ERR_RT_ASSERTION_FAILED, 
					"An attempt was made to retrieve a NaviLibrary::Singleton that has not been instantiated!", 
					"Singleton::Get");

			return *instance;
		}

		static Class* GetPointer()
		{
			return instance;
		}
	};
}

#endif