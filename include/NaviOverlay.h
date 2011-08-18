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

#ifndef __ViewportOverlay_H__
#define __ViewportOverlay_H__

#include <OGRE/Ogre.h>
#include <OGRE/OgrePanelOverlayElement.h>
#include "NaviPlatform.h"

namespace NaviLibrary {

/**
* Enumerates relative positions. Used by NaviPosition.
*/
enum RelativePosition
{
	Left,
	TopLeft,
	TopCenter,
	TopRight,
	Right,
	BottomRight,
	BottomCenter,
	BottomLeft,
	Center
};

/**
* An object that holds position-data for a Navi. Used by NaviManager::createNavi and NaviManager::setNaviPosition.
*/
class _NaviExport NaviPosition
{
	bool usingRelative;
	union {
		struct { RelativePosition position; short x; short y; } rel;
		struct { short left; short top; } abs;
	} data;

	friend class NaviOverlay;
	NaviPosition();
public:
	/**
	* Creates a relatively-positioned NaviPosition object.
	*
	* @param	relPosition		The position of the Navi in relation to the viewport.
	*
	* @param	offsetLeft	How many pixels from the left to offset the Navi from the relative position.
	*
	* @param	offsetTop	How many pixels from the top to offset the Navi from the relative position.
	*/
	NaviPosition(const RelativePosition &relPosition, short offsetLeft = 0, short offsetTop = 0);

	/**
	* Creates an absolutely-positioned NaviPosition object.
	*
	* @param	absoluteLeft	The number of pixels from the left of the viewport.
	*
	* @param	absoluteTop		The number of pixels from the top of the viewport.
	*/
	NaviPosition(short absoluteLeft, short absoluteTop);
};

/**
* An enumeration of the three tiers NaviOverlay can reside in.
*/
enum Tier
{
	Back = 0,
	Middle = 1,
	Front = 2
};

/**
* A simple implementation of a viewport-overlay used by the 'Navi' class.
*/
class _NaviExport NaviOverlay : public Ogre::RenderTargetListener
{
public:
	Ogre::Viewport* viewport;
	Ogre::Overlay* overlay;
	Ogre::PanelOverlayElement* panel;
	NaviPosition position;
	bool isVisible;
	int width, height;
	Tier tier;
	Ogre::uchar zOrder;

	NaviOverlay(const Ogre::String& name, Ogre::Viewport* viewport, int width, int height, const NaviPosition& pos, 
		const Ogre::String& matName, Ogre::uchar zOrder, Tier tier);
	~NaviOverlay();

	void setViewport(Ogre::Viewport* newViewport);

	void move(int deltaX, int deltaY);
	void setPosition(const NaviPosition& position);
	void resetPosition();
	
	void resize(int width, int height);

	void hide();
	void show();

	void setTier(Tier tier);
	void setZOrder(Ogre::uchar zOrder);

	Tier getTier();
	Ogre::uchar getZOrder();

	int getX();
	int getY();
	
	int getRelativeX(int absX);
	int getRelativeY(int absY);

	bool getVisibility() const;

	bool isWithinBounds(int absX, int absY);

	bool operator>(const NaviOverlay& rhs) const;
	bool operator<(const NaviOverlay& rhs) const;

	void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
	void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
	void preViewportUpdate(const Ogre::RenderTargetViewportEvent& evt);
	void postViewportUpdate(const Ogre::RenderTargetViewportEvent& evt);
	void viewportAdded(const Ogre::RenderTargetViewportEvent& evt);
	void viewportRemoved(const Ogre::RenderTargetViewportEvent& evt);
};

}

#endif