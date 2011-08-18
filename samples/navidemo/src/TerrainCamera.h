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

#ifndef __TerrainCamera_H__
#define __TerrainCamera_H__

#include <OGRE/Ogre.h>

class TerrainCamera
{
	Ogre::SceneNode *baseNode, *pivotNode, *camNode;
	Ogre::Camera *camera;
	Ogre::RaySceneQuery *rayQuery;
	Ogre::Real height, targetHeight;
	Ogre::Timer timer;
public:
	TerrainCamera(Ogre::SceneNode *baseNode, Ogre::Camera *camera, Ogre::Vector3 offset = Ogre::Vector3(0, 40, -60), Ogre::Real height = 20);

	~TerrainCamera();

	Ogre::Camera* getCamera();

	void spin(const Ogre::Radian &angle);

	void pitch(const Ogre::Radian &angle);

	void translate(const Ogre::Vector3& displacement);

	void clampToTerrain();

	void orientPlaneToCamera(Ogre::SceneNode* planeNode, int planeHeight, int xOffset, int zOffset);

	void clampPlaneToTerrain(Ogre::SceneNode* planeNode, int planeHeight);

	void update();
};

#endif