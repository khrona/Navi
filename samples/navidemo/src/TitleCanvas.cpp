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

#include "TitleCanvas.h"

#define SIZE_OFFSET 5
#define HEIGHT_OFFSET 8
#define BEGIN_RANGE 500
#define RANGE_LENGTH 400
#define OCCLUSION_CHECK_RATE 500

TitleCanvas::TitleCanvas(Ogre::Camera* camera, const std::string& font, Ogre::SceneManager* sceneMgr) : camera(camera), font(font), 
	occlusionHandler(0), isHidden(false)
{
	FontFaceDefinition titleFont(font);
	for(unsigned int size = 10 + SIZE_OFFSET; size <= (20 + SIZE_OFFSET); size += 1)
		titleFont.addSize(size);

	std::vector<Ogre::String> textures;
	std::vector<FontFaceDefinition> fonts;
	fonts.push_back(titleFont);

	atlas = new Atlas(textures, fonts, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	canvas = new Canvas(atlas, camera->getViewport());
	sceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(canvas);
}

void TitleCanvas::setOcclusionHandler(OcclusionHandler* handler)
{
	occlusionHandler = handler;
}

void TitleCanvas::hide()
{
	isHidden = true;
	canvas->setVisible(false);
}

void TitleCanvas::show()
{
	isHidden = false;
	canvas->setVisible(true);
}

void TitleCanvas::addTitle(Ogre::MovableObject* target, const Ogre::DisplayString& caption, const Ogre::ColourValue& color)
{
	for(std::vector<Title>::iterator i = titles.begin(); i != titles.end(); i++)
	{
		if(i->target == target)
		{
			editTitle(target, caption, color);
			return;
		}
	}

	Title title;
	title.target = target;
	title.caption = caption;
	title.color = color;
	title.isOccluded = false;
	titles.push_back(title);
}

void TitleCanvas::editTitle(Ogre::MovableObject* target, const Ogre::DisplayString& caption, const Ogre::ColourValue& color)
{
	for(std::vector<Title>::iterator i = titles.begin(); i != titles.end(); i++)
	{
		if(i->target == target)
		{
			i->caption = caption;
			i->color = color;
			break;
		}
	}
}

void TitleCanvas::removeTitle(Ogre::MovableObject* target)
{
	for(std::vector<Title>::iterator i = titles.begin(); i != titles.end(); i++)
	{
		if(i->target == target)
		{
			titles.erase(i);
			break;
		}
	}
}

void TitleCanvas::update()
{
	if(isHidden)
		return;

	static std::vector<Title> visibleTitles;
	visibleTitles.clear();

	bool isCheckingOcclusion = false;
	if(timer.getMilliseconds() > OCCLUSION_CHECK_RATE && occlusionHandler)
		isCheckingOcclusion = true;

	for(std::vector<Title>::iterator i = titles.begin(); i != titles.end(); i++)
	{
		if(!i->target->isInScene())
			continue;

		// Derive the average point between the top-most corners of the object's bounding box
		const Ogre::AxisAlignedBox &AABB = i->target->getWorldBoundingBox(true);
		Ogre::Vector3 point = (AABB.getCorner(Ogre::AxisAlignedBox::FAR_LEFT_TOP)
			+ AABB.getCorner(Ogre::AxisAlignedBox::FAR_RIGHT_TOP)
			+ AABB.getCorner(Ogre::AxisAlignedBox::NEAR_LEFT_TOP)
			+ AABB.getCorner(Ogre::AxisAlignedBox::NEAR_RIGHT_TOP)) / 4;

		point += Ogre::Vector3(0, HEIGHT_OFFSET, 0);

		i->position.z = camera->getDerivedPosition().distance(point);

		if(i->position.z >= BEGIN_RANGE + RANGE_LENGTH)
			continue;

		// Is the camera facing that point? If not, hide the overlay and return.
		Ogre::Plane cameraPlane = Ogre::Plane(Ogre::Vector3(camera->getDerivedOrientation().zAxis()), camera->getDerivedPosition());
		if(cameraPlane.getSide(point) != Ogre::Plane::NEGATIVE_SIDE)
			continue;
		
		// Derive the 2D (x,y) screen-space coordinates for that point
		point = camera->getProjectionMatrix() * (camera->getViewMatrix() * point);
		Ogre::Real x = (point.x / 2) + 0.5f;
		Ogre::Real y = 1 - ((point.y / 2) + 0.5f);

		if(x < 0 || x > 1)
			continue;
		else if(y < 0 || y > 1)
			continue;

		if(isCheckingOcclusion)
			i->isOccluded = occlusionHandler->isScreenPointOccluded(x * camera->getViewport()->getActualWidth(), 
				y * camera->getViewport()->getActualHeight());
		
		if(i->isOccluded)
			continue;

		i->position.x = x;
		i->position.y = y;
		
		visibleTitles.push_back(*i); 
	}

	canvas->clear();

	for(std::vector<Title>::iterator i = visibleTitles.begin(); i != visibleTitles.end(); i++)
	{
		int x = i->position.x * camera->getViewport()->getActualWidth();
		int y = i->position.y * camera->getViewport()->getActualHeight();

		// scale [40, 400] to [5, 0]: (-5 / 360)x + (50 / 9) = y
		Ogre::Real distance = Ogre::Math::Floor((-10 / 360.0)*i->position.z + (100 / 9.0));

		if(distance < 0) distance = 0; 
		else if(distance > 10) distance = 10;

		Ogre::uint fontSize = distance + 10 + SIZE_OFFSET;
		Ogre::Real avgAdvance = atlas->getGlyphInfo(font, fontSize, 'x').advance;
		Ogre::Real pen = x - (i->caption.length() * avgAdvance) / 2;
		
		Ogre::Real opacity;
		if(i->position.z <= BEGIN_RANGE)
			opacity = 1;
		else if(i->position.z > (BEGIN_RANGE + RANGE_LENGTH))
			opacity = 0;
		else
			opacity = (RANGE_LENGTH - i->position.z + BEGIN_RANGE) / RANGE_LENGTH;

		Ogre::ColourValue col;

		for(Ogre::DisplayString::const_iterator j = i->caption.begin(); j != i->caption.end(); j++)
		{
			if(*j == ' ')
			{
				pen += avgAdvance;
				continue;
			}

			col = i->color;
			col.a = opacity;
			GlyphInfo glyph = atlas->getGlyphInfo(font, fontSize, (*j));
			canvas->drawGlyph(glyph, glyph.bearingX + pen + 1, y - glyph.bearingY + 1, glyph.texInfo.width, glyph.texInfo.height, Ogre::ColourValue(0, 0.1, 0.35, 0.5 * opacity));
			canvas->drawGlyph(glyph, glyph.bearingX + pen, y - glyph.bearingY, glyph.texInfo.width, glyph.texInfo.height, col);
			pen += glyph.advance;
		}
	}

	if(isCheckingOcclusion)
		timer.reset();
}