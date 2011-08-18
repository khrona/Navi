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

#include "TerrainCamera.h"

using namespace Ogre;

TerrainCamera::TerrainCamera(Ogre::SceneNode *baseNode, Ogre::Camera *camera, Vector3 offset, Real height) : baseNode(baseNode), camera(camera), height(height)
{
	pivotNode = baseNode->createChildSceneNode(baseNode->getName()+"_PivotNode", Ogre::Vector3(700, 300, 700));
	camNode = pivotNode->createChildSceneNode(baseNode->getName()+"_CameraNode", offset);
	camNode->yaw(Degree(180));
	camNode->attachObject(camera);
	rayQuery = camera->getSceneManager()->createRayQuery(Ray(pivotNode->getPosition(), Vector3::NEGATIVE_UNIT_Y));
	targetHeight = pivotNode->getPosition().y;
	clampToTerrain();
	pivotNode->setPosition(pivotNode->getPosition().x, targetHeight, pivotNode->getPosition().z);
}

TerrainCamera::~TerrainCamera()
{
	camNode->detachAllObjects();
	baseNode->removeAndDestroyChild(pivotNode->getName());
}

Camera* TerrainCamera::getCamera()
{
	return camera;
}

void TerrainCamera::spin(const Ogre::Radian &angle)
{
	pivotNode->yaw(angle);
}

void TerrainCamera::pitch(const Ogre::Radian &angle)
{
	camNode->pitch(angle);
}

void TerrainCamera::translate(const Ogre::Vector3& displacement)
{
	pivotNode->translate(displacement, Ogre::Node::TS_LOCAL);
}

void TerrainCamera::clampToTerrain()
{
	rayQuery->setRay(Ray(pivotNode->getPosition() + Vector3(0, 200, 0), Vector3::NEGATIVE_UNIT_Y));
	RaySceneQueryResult &qryResult = rayQuery->execute();
	RaySceneQueryResult::iterator i = qryResult.begin();
	if(i != qryResult.end() && i->worldFragment)
	{
		//pivotNode->setPosition(pivotNode->getPosition().x, i->worldFragment->singleIntersection.y + 1 + height, pivotNode->getPosition().z);
		targetHeight = i->worldFragment->singleIntersection.y + 1 + height;
	}
}

void TerrainCamera::orientPlaneToCamera(Ogre::SceneNode* planeNode, int planeHeight, int xOffset, int zOffset)
{
	pivotNode->translate(Ogre::Vector3(xOffset, 0, zOffset), Ogre::Node::TS_LOCAL);

	planeNode->setPosition(pivotNode->getPosition());
	
	pivotNode->translate(Ogre::Vector3(-xOffset, 0, -zOffset), Ogre::Node::TS_LOCAL);

	planeNode->setOrientation(pivotNode->getOrientation());
	planeNode->yaw(Degree(90));

	clampPlaneToTerrain(planeNode, planeHeight);
}

void TerrainCamera::clampPlaneToTerrain(Ogre::SceneNode* planeNode, int planeHeight)
{
	rayQuery->setRay(Ray(planeNode->getPosition() + Vector3(0, 170, 0), Vector3::NEGATIVE_UNIT_Y));
	RaySceneQueryResult &qryResult = rayQuery->execute();
	RaySceneQueryResult::iterator i = qryResult.begin();
	if(i != qryResult.end() && i->worldFragment)
		planeNode->setPosition(planeNode->getPosition().x, i->worldFragment->singleIntersection.y + planeHeight / 2, planeNode->getPosition().z);
}

#define CHANGE_VELOCITY	0.015

void TerrainCamera::update()
{
	pivotNode->translate(0, (targetHeight - pivotNode->getPosition().y) / 2 * CHANGE_VELOCITY * timer.getMilliseconds(), 0);

	timer.reset();
}