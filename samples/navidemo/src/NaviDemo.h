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

#ifndef __NaviDemo_H__
#define __NaviDemo_H__

#include <OGRE/Ogre.h>
#include "InputManager.h"
#include "Navi.h"
#include "TerrainCamera.h"
#include "TitleCanvas.h"

using namespace NaviLibrary;

class PlaneObject 
{
public:
	virtual ~PlaneObject() {}
};

class NaviGate : public PlaneObject
{
public:
	NaviGate();
	~NaviGate();

	Ogre::SceneNode* planeNode;
	Navi* navi;
	std::string url;
	std::string title;
	std::string targetURL;
	NaviOverlay* overlay;
	int x, y;
	int width, height;
};

class SelectionPlane : public PlaneObject 
{
public:
	int id;
	SelectionPlane(int id) : id(id) {}
};

class NaviDemo : public OIS::MouseListener, public OIS::KeyListener, public Ogre::WindowEventListener, public OcclusionHandler
{
	Ogre::Viewport* viewport;
	Ogre::RenderWindow* renderWin;
	Ogre::SceneManager* sceneMgr;
	NaviManager* naviMgr;
	Navi *menu, *navibar, *status, *statistics, *examples, *help;
	std::map<int, NaviGate*> activeGates;
	NaviGate* focusedGate;
	bool isFullscreen, isDraggingNavi, isDraggingGate, isMovingGate, isRotatingGate, isResizingGate;
	int gateIDCounter;
	TerrainCamera* terrainCam;
	Ogre::RaySceneQuery* raySceneQuery;
	Ogre::MeshPtr naviPlane;
	InputManager* inputMgr;
	Ogre::Timer timer;
	Ogre::SceneNode* selectionNode, *selectionPlaneNode, *resizePickingNode, *fullscreenShadeNode;
	TitleCanvas* titleCanvas;
	SelectionPlane *selectionPlane1, *selectionPlane2;
	int maxGateWidth, maxGateHeight;
	int minGateWidth, minGateHeight;
	int resizeWidth, resizeHeight;

	void parseResources();
	void loadInputSystem();
	bool isScreenPointOccluded(int x, int y);
public:
	bool shouldQuit;
	NaviDemo();
	~NaviDemo();

	void createScene();
	void setupNavis();

	void Update();

	void onCreate(Navi* caller, const OSM::JSArguments& args);
	void onExamples(Navi* caller, const OSM::JSArguments& args);
	void onHelp(Navi* caller, const OSM::JSArguments& args);
	void onExit(Navi* caller, const OSM::JSArguments& args);

	void onGoBack(Navi* caller, const OSM::JSArguments& args);
	void onGoForward(Navi* caller, const OSM::JSArguments& args);
	void onNavigateTo(Navi* caller, const OSM::JSArguments& args);
	void onToggleFullscreen(Navi* caller, const OSM::JSArguments& args);
	void onDestroy(Navi* caller, const OSM::JSArguments& args);

	void onBeginNavigation(Navi* caller, const OSM::JSArguments& args);
	void onBeginLoading(Navi* caller, const OSM::JSArguments& args);
	void onFinishLoading(Navi* caller, const OSM::JSArguments& args);
	void onReceiveTitle(Navi* caller, const OSM::JSArguments& args);
	void onChangeTargetURL(Navi* caller, const OSM::JSArguments& args);
	void onChangeKeyboardFocus(Navi* caller, const OSM::JSArguments& args);
	void onOpenExternalLink(Navi* caller, const OSM::JSArguments& args);
	void onWebViewCrashed(Navi* caller, const OSM::JSArguments& args);

	Ogre::ManualObject* createManualPlane(const std::string& name, int width, int height, const std::string& matName, Ogre::Real u2 = 1, Ogre::Real v2 = 1);
	void focusGate(NaviGate* gate);
	void resizeFocusedGate(int width, int height);
	PlaneObject* getPlaneObjectAtPoint(int x, int y, int& localX, int& localY);
	bool isPointOverResizePickingPlane(int x, int y, int& localX, int& localY);
	bool isPointOverNaviGate(int screenX, int screenY, int screenWidth, int screenHeight, int& naviX, int& naviY, NaviGate*& resultGate);
	
	bool mouseMoved(const OIS::MouseEvent &arg);
	bool mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id);
	bool mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id);

	bool keyPressed( const OIS::KeyEvent &arg );
	bool keyReleased( const OIS::KeyEvent &arg );	

	void windowMoved(Ogre::RenderWindow* rw);
	void windowResized(Ogre::RenderWindow* rw);
	void windowClosed(Ogre::RenderWindow* rw);
	void windowFocusChange(Ogre::RenderWindow* rw);
};

#endif