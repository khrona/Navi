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

#ifndef __TITLECANVAS_H__
#define __TITLECANVAS_H__

#include "Canvas.h"
#include <OGRE/OgreTimer.h>

/**
* The TitleCanvas is used to render 'titles' above various MovableObjects dynamically, in a single batch.
* Titles automatically follow their target, are sized based on camera distance, and have slight text-shadows.
*/

class OcclusionHandler
{
public:
	virtual bool isScreenPointOccluded(int x, int y) = 0;
};

class TitleCanvas
{
	struct Title
	{
		Ogre::MovableObject* target;
		Ogre::DisplayString caption;
		Ogre::ColourValue color;
		Ogre::Vector3 position;
		bool isOccluded;
	};

	Atlas* atlas;
	Canvas* canvas;
	Ogre::Camera* camera;
	std::string font;
	std::vector<Title> titles;
	OcclusionHandler* occlusionHandler;
	Ogre::Timer timer;
	bool isHidden;
public:

	/**
	* Creates a new TitleCanvas.
	*
	* @param	camera	The camera that will be rendering the targets.
	* @param	font	The font to use for all of the titles.
	* @param	sceneMgr	The active scene manager.
	*/
	TitleCanvas(Ogre::Camera* camera, const std::string& font, Ogre::SceneManager* sceneMgr);

	void setOcclusionHandler(OcclusionHandler* handler);

	void hide();
	void show();

	/**
	* Binds a 2D caption to be rendered above a certain MovableObject.
	*
	* @param	target	The target that the caption should follow.
	* @param	caption	The text to display above the target.
	* @param	color	The color of the text.
	*/
	void addTitle(Ogre::MovableObject* target, const Ogre::DisplayString& caption, const Ogre::ColourValue& color = Ogre::ColourValue::White);

	/**
	* Edits the caption of an existing title.
	*
	* @param	target	The target of the existing title.
	* @param	caption	The new text to display above the target.
	* @param	color	The color of the text.
	*/
	void editTitle(Ogre::MovableObject* target, const Ogre::DisplayString& caption, const Ogre::ColourValue& color = Ogre::ColourValue::White);

	/**
	* Removes an existing title.
	*
	* @param	target	The target of the existing title.
	*/
	void removeTitle(Ogre::MovableObject* target);

	/**
	* Updates the position of all the titles in this TitleCanvas.
	*/
	void update();
};

#endif