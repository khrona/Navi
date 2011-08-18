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

#include "NaviOverlay.h"

using namespace Ogre;
using namespace NaviLibrary;

NaviPosition::NaviPosition()
{
	usingRelative = false;
	data.abs.left = 0;
	data.abs.top = 0;
}

NaviPosition::NaviPosition(const RelativePosition &relPosition, short offsetLeft, short offsetTop)
{
	usingRelative = true;
	data.rel.position = relPosition;
	data.rel.x = offsetLeft;
	data.rel.y = offsetTop;
}

NaviPosition::NaviPosition(short absoluteLeft, short absoluteTop)
{
	usingRelative = false;
	data.abs.left = absoluteLeft;
	data.abs.top = absoluteTop;
}

NaviOverlay::NaviOverlay(const Ogre::String& name, Ogre::Viewport* viewport, int width, int height, 
	const NaviPosition& pos, const Ogre::String& matName, Ogre::uchar zOrder, Tier tier)
: viewport(viewport), width(width), height(height), position(pos), isVisible(true), zOrder(zOrder), tier(tier)
{
	if(zOrder > 199)
		OGRE_EXCEPT(Ogre::Exception::ERR_RT_ASSERTION_FAILED, 
			"Z-order is out of bounds, must be within [0, 199].", "NaviOverlay::NaviOverlay");

	if(!viewport)
		Ogre::LogManager::getSingleton().logMessage("NaviOverlay created with null viewport, won't be displayed until one is given.");

	OverlayManager& overlayManager = OverlayManager::getSingleton();

	panel = static_cast<PanelOverlayElement*>(overlayManager.createOverlayElement("Panel", name + "Panel"));
	panel->setMetricsMode(Ogre::GMM_PIXELS);
	panel->setMaterialName(matName);
	panel->setDimensions(width, height);
	
	overlay = overlayManager.create(name + "Overlay");
	overlay->add2D(panel);
	overlay->hide();
	setZOrder(zOrder);
	resetPosition();

	if(viewport)
		viewport->getTarget()->addListener(this);
}

NaviOverlay::~NaviOverlay()
{
	if(viewport)
		viewport->getTarget()->removeListener(this);

	if(overlay)
	{
		overlay->remove2D(panel);
		OverlayManager::getSingletonPtr()->destroyOverlayElement(panel);
		OverlayManager::getSingletonPtr()->destroy(overlay);
	}
}

void NaviOverlay::setViewport(Ogre::Viewport* newViewport)
{
	overlay->hide();

	if(viewport)
		viewport->getTarget()->removeListener(this);

	viewport = newViewport;

	if(viewport)
	{
		viewport->getTarget()->addListener(this);
		resetPosition();
	}
}

void NaviOverlay::move(int deltaX, int deltaY)
{
	panel->setPosition(panel->getLeft()+deltaX, panel->getTop()+deltaY);
}

void NaviOverlay::setPosition(const NaviPosition& position)
{
	this->position = position;
	resetPosition();
}

void NaviOverlay::resetPosition()
{
	if(!viewport)
	{
		panel->setPosition(0, 0);
		return;
	}

	int viewWidth = viewport->getActualWidth();
	int viewHeight = viewport->getActualHeight();

	if(position.usingRelative)
	{
		int left = 0 + position.data.rel.x;
		int center = (viewWidth/2)-(width/2) + position.data.rel.x;
		int right = viewWidth - width + position.data.rel.x;

		int top = 0 + position.data.rel.y;
		int middle = (viewHeight/2)-(height/2) + position.data.rel.y;
		int bottom = viewHeight-height + position.data.rel.y;

		switch(position.data.rel.position)
		{
		case Left:
			panel->setPosition(left, middle);
			break;
		case TopLeft:
			panel->setPosition(left, top);
			break;
		case TopCenter:
			panel->setPosition(center, top);
			break;
		case TopRight:
			panel->setPosition(right, top);
			break;
		case Right:
			panel->setPosition(right, middle);
			break;
		case BottomRight:
			panel->setPosition(right, bottom);
			break;
		case BottomCenter:
			panel->setPosition(center, bottom);
			break;
		case BottomLeft:
			panel->setPosition(left, bottom);
			break;
		case Center:
			panel->setPosition(center, middle);
			break;
		default:
			panel->setPosition(position.data.rel.x, position.data.rel.y);
			break;
		}
	}
	else
		panel->setPosition(position.data.abs.left, position.data.abs.top);
}

void NaviOverlay::resize(int width, int height)
{
	this->width = width;
	this->height = height;
	panel->setDimensions(width, height);
}

void NaviOverlay::hide()
{
	isVisible = false;
}

void NaviOverlay::show()
{
	isVisible = true;
}

void NaviOverlay::setTier(Tier tier)
{
	this->tier = tier;
	overlay->setZOrder(200 * tier + zOrder);
}

void NaviOverlay::setZOrder(Ogre::uchar zOrder)
{
	this->zOrder = zOrder;
	overlay->setZOrder(200 * tier + zOrder);
}

Tier NaviOverlay::getTier()
{
	return tier;
}

Ogre::uchar NaviOverlay::getZOrder()
{
	return zOrder;
}

int NaviOverlay::getX()
{
	return viewport? viewport->getActualLeft() + panel->getLeft() : 0;
}

int NaviOverlay::getY()
{
	return viewport? viewport->getActualTop() + panel->getTop() : 0;
}

int NaviOverlay::getRelativeX(int absX)
{
	return viewport? absX - viewport->getActualLeft() - panel->getLeft() : 0;
}

int NaviOverlay::getRelativeY(int absY)
{
	return viewport? absY - viewport->getActualTop() - panel->getTop() : 0;
}

bool NaviOverlay::getVisibility() const
{
	return isVisible && viewport;
}

bool NaviOverlay::isWithinBounds(int absX, int absY)
{
	if(!viewport)
		return false;

	if(absX < viewport->getActualLeft() || absX > viewport->getActualWidth() + viewport->getActualLeft() ||
		absY < viewport->getActualTop() || absY > viewport->getActualHeight() + viewport->getActualTop())
		return false;

	int localX = getRelativeX(absX);
	int localY = getRelativeY(absY);

	if(localX > 0 && localX < width)
		if(localY > 0 && localY < height)
			return true;

	return false;
}

bool NaviOverlay::operator>(const NaviOverlay& rhs) const
{
	return tier * 200 + zOrder > rhs.tier * 200 + rhs.zOrder;
}

bool NaviOverlay::operator<(const NaviOverlay& rhs) const
{
	return tier * 200 + zOrder < rhs.tier * 200 + rhs.zOrder;
}

void NaviOverlay::preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
}

void NaviOverlay::postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
}

void NaviOverlay::preViewportUpdate(const Ogre::RenderTargetViewportEvent& evt)
{
	if(evt.source == viewport && isVisible)
		overlay->show();
}

void NaviOverlay::postViewportUpdate(const Ogre::RenderTargetViewportEvent& evt)
{
	overlay->hide();
}

void NaviOverlay::viewportAdded(const Ogre::RenderTargetViewportEvent& evt)
{
}

void NaviOverlay::viewportRemoved(const Ogre::RenderTargetViewportEvent& evt)
{
}