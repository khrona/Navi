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

#ifndef __NaviManager_H__
#define __NaviManager_H__

#include "NaviUtilities.h"
#include "NaviSingleton.h"
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif
#include <OGRE/Ogre.h>
#include <OGRE/OgrePanelOverlayElement.h>
#include "KeyboardHook.h"
#include <Awesomium/WebCore.h>
#include "NaviOverlay.h"
#include "NaviDelegate.h"

/**
* Global namespace 'NaviLibrary' encapsulates all NaviLibrary-specific stuff.
*/
namespace NaviLibrary
{
	class Navi;

	/**
	* Enumerates internal mouse button IDs. Used by NaviManager::injectMouseDown, NaviManager::injectMouseUp
	*/
	enum MouseButtonID
	{
		LeftMouseButton = 0,
		RightMouseButton, 
		MiddleMouseButton
	};
 
	/**
	* Supreme dictator and Singleton: NaviManager
	*
	* The class you will need to go to for all your Navi-related needs.
	*/
	class _NaviExport NaviManager : public Singleton<NaviManager>, public Impl::HookListener
	{
	public:
		/**
		* Creates the NaviManager singleton.
		*
		* @param	defaultViewport		The default Ogre::Viewport to place Navis in. This can be overriden
		*								per-Navi via the last parameter of NaviManager::createNavi. If you
		*								would like to defer initialization of the viewport, you may pass a
		*								null (0) value for the viewport and update the value later for all
		*								Navis using NaviManager::setDefaultViewport.
		*
		* @param	baseDirectory		The relative path to your base directory. This directory is used
		*								by Navi::loadFile and Navi::loadHTML (to resolve relative URLs).
		*/
		NaviManager(Ogre::Viewport* defaultViewport, const std::string &baseDirectory = "");

		/**
		* Destroys the NaviManager singleton (also destroys any lingering Navis).
		*/
		~NaviManager();

		/**
		* Gets the NaviManager Singleton.
		*
		* @return	A reference to the NaviManager Singleton.
		*
		* @throws	Ogre::Exception::ERR_RT_ASSERTION_FAILED	Throws this if NaviManager has not been instantiated yet.
		*/
		static NaviManager& Get();

		/**
		* Gets the NaviManager Singleton as a pointer.
		*
		* @return	If the NaviManager has been instantiated, returns a pointer to the NaviManager Singleton,
		*			otherwise this returns 0.
		*/
		static NaviManager* GetPointer();

		/**
		* Gives each active Navi a chance to update.
		*/
		void Update();

		/**
		* Creates a Navi.
		*
		* @param	naviName	The name of the Navi, used to refer to a specific Navi in subsequent calls.
		*
		* @param	width	The width of the Navi.
		*
		* @param	height	The height of the Navi.
		*
		* @param	naviPosition	The unified position (either relative or absolute) of a Navi.
		*							See NaviManager::NaviPosition for more information.
		*
		* @param	asyncRender		Whether or not this Navi should render asynchronously (disabled by default). Enabling
		*							this mode allows most internal rendering of web content to occur fully on another
		*							thread which potentially frees up the main graphics thread a bit (allowing higher
		*							overall FPS). It is best to only enable this mode for Navis with high-animation content.
		*
		* @param	maxAsyncRenderRate	If asyncRender is enabled, you can specify the maximum times per second the internal
		*								thread should render the web content. Default is 70 times per second.
		*
		* @param	tier	The tier that this Navi belongs to (either Front, Middle, or Back). You can group Navis into
		*					different tiers to keep certain Navis always in the foreground or background.
		*
		* @param	viewport	The viewport that this Navi should be contained in. Specify 0 here to use the default
		*						viewport that was given to NaviManager at initialization.
		*
		* @throws	Ogre::Exception::ERR_RT_ASSERTION_FAILED	Throws this if a Navi by the same name already exists.
		*/
		Navi* createNavi(const std::string &naviName, unsigned short width, unsigned short height, const NaviPosition &naviPosition, 
			bool asyncRender = false, int maxAsyncRenderRate = 70, Tier tier = Middle, Ogre::Viewport* viewport = 0);

		/**
		* Creates a NaviMaterial. NaviMaterials are just like Navis except that they lack a movable overlay element. 
		* Instead, you handle the material and apply it to anything you like. Mouse input for NaviMaterials should be 
		* injected via the Navi::injectMouse_____ API calls instead of the global NaviManager::injectMouse_____ calls.
		*
		* @param	naviName	The name of the NaviMaterial, used to refer to this specific Navi in subsequent calls.
		*
		* @param	width	The width of the NaviMaterial.
		*
		* @param	height	The height of the NaviMaterial.
		*
		* @param	asyncRender		Whether or not this Navi should render asynchronously (disabled by default). Enabling
		*							this mode allows most internal rendering of web content to occur fully on another
		*							thread which potentially frees up the main graphics thread a bit (allowing higher
		*							overall FPS). It is best to only enable this mode for Navis with high-animation content.
		*
		* @param	maxAsyncRenderRate	If asyncRender is enabled, you can specify the maximum times per second the internal
		*								thread should render the web content. Default is 70 times per second.
		*
		* @param	texFiltering	The texture filtering to use for this material. (see Ogre::FilterOptions) If the NaviMaterial is
		*							applied to a 3D object, FO_ANISOTROPIC is the best (and default) choice, otherwise set this to
		*							FO_NONE for use in other overlays/GUI elements.
		*
		* @throws	Ogre::Exception::ERR_RT_ASSERTION_FAILED	Throws this if a Navi by the same name already exists.
		*/
		Navi* createNaviMaterial(const std::string &naviName, unsigned short width, unsigned short height,
			bool asyncRender = false, int maxAsyncRenderRate = 70, Ogre::FilterOptions texFiltering = Ogre::FO_ANISOTROPIC);

		/**
		* Retrieve a pointer to a Navi by name.
		*
		* @param	naviName	The name of the Navi to retrieve.
		*
		* @return	If the Navi is found, returns a pointer to the Navi, otherwise returns 0.
		*/
		Navi* getNavi(const std::string &naviName);

		/**
		* Retrieve all Navis whose name match a certain pattern.
		*
		* @param	pattern		A string that describes the names of the Navis that you wish to retrive.
		*						There are two special pattern-matching characters you can use:
		*							* : Match zero or more characters.
		*							? : Match exactly one occurrence of any character.
		*/
		std::vector<Navi*> getNavis(const std::string& pattern);

		/**
		* Immediately destroys a Navi by name.
		*
		* @param	naviName	The name of the Navi to destroy.
		*/
		void destroyNavi(const std::string &naviName);

		/**
		* Immediately destroys a Navi.
		*
		* @param	naviName	A pointer to the Navi to destroy.
		*/
		void destroyNavi(Navi* naviToDestroy);

		/**
		* Resets the positions of all Navis to their default positions. (not applicable to NaviMaterials)
		*/
		void resetAllPositions();

		/**
		* Returns whether or not a Navi is focused/selected. (not applicable to NaviMaterials)
		*
		* @return	True if a Navi is focused, False otherwise.
		*/
		bool isAnyNaviFocused();

		/**
		* Gets the currently focused/selected Navi. (not applicable to NaviMaterials)
		*
		* @return	A pointer to the Navi that is currently focused, returns 0 if none are focused.
		*/
		Navi* getFocusedNavi();
		
		/**
		* Returns whether or not any Navi (including NaviMaterials) currently has keyboard focus.
		*
		* @return	Whether or not any Navi has keyboard focus.
		*/
		bool hasKeyboardFocus();

		/**
		* Returns whether or not the currently focused Navi is modal (and is receiving all input).
		*
		* @return	True if there is a currently focused Navi and it is modal, False otherwise.
		*/
		bool hasModalFocus();

		/**
		* Injects the mouse's current position into NaviManager. Used to generally keep track of where the mouse 
		* is for things like moving Navis around, telling the internal pages of each Navi where the mouse is and
		* where the user has clicked, etc. (not applicable to NaviMaterials)
		*
		* @param	xPos	The current X-coordinate of the mouse.
		* @param	yPos	The current Y-coordinate of the mouse.
		*
		* @return	Returns True if the injected coordinate is over a Navi, False otherwise.
		*/
		bool injectMouseMove(int xPos, int yPos);

		/**
		* Injects mouse wheel events into NaviManager. Used to scroll the focused Navi. (not applicable to NaviMaterials)
		*
		* @param	relScroll	The relative Scroll-Value of the mouse.
		*
		* @note
		*	To inject this using OIS: on a OIS::MouseListener::MouseMoved event, simply 
		*	inject "arg.state.Z.rel" of the "MouseEvent".
		*
		* @return	Returns True if the mouse wheel was scrolled while a Navi was focused, False otherwise.
		*/
		bool injectMouseWheel(int relScroll);

		/**
		* Injects mouse down events into NaviManager. Used to know when the user has pressed a mouse button
		* and which button they used. (not applicable to NaviMaterials)
		*
		* @param	buttonID	The ID of the button that was pressed. Left = 0, Right = 1, Middle = 2.
		*
		* @return	Returns True if the mouse went down over a Navi, False otherwise.
		*/
		bool injectMouseDown(int buttonID);

		/**
		* Injects mouse up events into NaviManager. Used to know when the user has released a mouse button 
		* and which button they used. (not applicable to NaviMaterials)
		*
		* @param	buttonID	The ID of the button that was released. Left = 0, Right = 1, Middle = 2.
		*
		* @return	Returns True if the mouse went up while a Navi was focused, False otherwise.
		*/
		bool injectMouseUp(int buttonID);

		/**
		* De-Focuses any currently-focused Navis.
		*/
		void deFocusAllNavis();

		/**
		* Retrieves the top-most visible Navi at a certain point on the screen.
		*
		* @param	x	The x-coordinate in screen-space.
		* @param	y	The y-coordinate in screen-space.
		*
		* @return	Returns the top-most Navi if there is one at that point, else returns 0.
		*/
		Navi* getTopNavi(int x, int y);

		/**
		* Sets the new default viewport to use for all future Navis. Also updates all existing 
		* that are currently using the default viewport value (see Navi::setViewport).
		*
		* @param	viewport	The new default viewport (a null value is also valid).
		*/
		void setDefaultViewport(Ogre::Viewport* viewport);

	protected:
		friend class Navi; // Our very close friend <3

		Awesomium::WebCore* webCore;
		std::map<std::string,Navi*> activeNavis;
		Navi* focusedNavi, *tooltipNavi, *tooltipParent, *keyboardFocusedNavi;
		std::map<std::string,Navi*>::iterator iter;
		Ogre::Viewport* defaultViewport;
		int mouseXPos, mouseYPos;
		bool mouseButtonRDown, mouseButtonLDown;
		unsigned short zOrderCounter;
		Impl::KeyboardHook* keyboardHook;
		Ogre::Timer tooltipTimer;
		double lastTooltip, tooltipShowTime;
		bool isDraggingFocusedNavi;
		bool isFocusedNaviModal;
		struct CallbackInvocation { Navi* caller; Awesomium::JSArguments args; NaviDelegate callback; };
		std::deque<CallbackInvocation> queuedCallbacks;

		bool focusNavi(int x, int y, Navi* selection = 0);
		void handleKeyMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
		void onResizeTooltip(Navi* Navi, const Awesomium::JSArguments& args);
		void handleTooltip(Navi* tooltipParent, const std::wstring& tipText);
		void handleRequestDrag(Navi* caller);
		void handleKeyboardFocusChange(Navi* caller, bool isFocused);
		void setNaviModality(Navi* caller, bool isModal);
		void handleNaviHide(Navi* caller);
		void queueCallback(Navi* caller, const Awesomium::JSArguments& args, const NaviDelegate& callback);
		void moveTooltip(int x, int y);
	};

}

#endif
