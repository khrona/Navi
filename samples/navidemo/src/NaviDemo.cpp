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

#include "NaviDemo.h"
#include "NaviUtilities.h"
#include "TitleCanvas.h"

using namespace Ogre;
using namespace NaviLibrary;
using namespace NaviLibrary::NaviUtilities;

void GetMeshInformation(const Ogre::MeshPtr mesh,
                         size_t &vertex_count,
                         Ogre::Vector3* &vertices,
                         size_t &index_count,
                         unsigned long* &indices,
                         const Ogre::Vector3 &position,
                         const Ogre::Quaternion &orient,
                         const Ogre::Vector3 &scale );

#define DEFAULT_URL				"http://www.google.com"
#define DEFAULT_GATE_WIDTH		670
#define DEFAULT_GATE_HEIGHT		512
#define GATE_DISTANCE			0.29f
#define GATE_SELECTION_WIDTH	35
#define GATE_QUERY_MASK			1<<7
#define TERRAIN_TILE_MASK		1<<6
#define SELECTION_PLANE_1_MASK	1<<5
#define SELECTION_PLANE_2_MASK	1<<4
#define RESIZE_PICKING_PLANE_MASK	1<<3
#define INPUT_HEARTBEAT			85
#define MOVE_RATE				165
#define TITLE_COLOR				Ogre::ColourValue(1, 1, 1)
#define SELECTED_TITLE_COLOR	Ogre::ColourValue(1, 228 / 255.0, 109 / 255.0)

bool rayHitPlane(Ogre::MovableObject* plane, Ogre::Camera* camera, int planeWidth, int planeHeight, const Ray& ray, int& outX, int& outY, Ogre::Real& dist);

NaviGate::NaviGate() : navi(0), planeNode(0), overlay(0)
{
}

NaviGate::~NaviGate()
{
	if(overlay)
		delete overlay;
	
	if(navi)
		NaviManager::Get().destroyNavi(navi);

	if(planeNode)
	{
		ManualObject* planeObject = (ManualObject*)planeNode->getAttachedObject(0);

		planeNode->detachAllObjects();
		planeNode->removeAllChildren();
		planeNode->getCreator()->destroyManualObject(planeObject);
		planeNode->getCreator()->getRootSceneNode()->removeAndDestroyChild(planeNode->getName());
	}
}

NaviDemo::NaviDemo()
{
	menu, navibar, status, statistics, examples, help = 0;
	shouldQuit = false;
	renderWin = 0;
	sceneMgr = 0;
	viewport = 0;
	gateIDCounter = 0;
	isFullscreen = 0;
	focusedGate = 0;
	terrainCam = 0;
	raySceneQuery = 0;
	isDraggingNavi = isDraggingGate = isMovingGate = isRotatingGate = isResizingGate = false;
	titleCanvas = 0;
	selectionNode = 0;
	fullscreenShadeNode = 0;
	selectionPlane1 = new SelectionPlane(1);
	selectionPlane2 = new SelectionPlane(2);
	maxGateWidth = maxGateHeight = 1024;
	minGateWidth = minGateHeight = 160;
	resizeWidth = resizeHeight = 0;

	Root *root = new Root();

	shouldQuit = !root->showConfigDialog();
	if(shouldQuit) return;

	renderWin = root->initialise(true, "NaviDemo");
	sceneMgr = root->createSceneManager("TerrainSceneManager");
	WindowEventUtilities::addWindowEventListener(renderWin, this);

	createScene();

	setupNavis();

	loadInputSystem();
}

NaviDemo::~NaviDemo()
{
	delete selectionPlane1;
	delete selectionPlane2;

	if(titleCanvas)
		delete titleCanvas;

	for(std::map<int, NaviGate*>::iterator i = activeGates.begin(); i != activeGates.end(); i++)
		delete i->second;

	delete NaviManager::GetPointer();
	Root::getSingleton().shutdown();
}

void NaviDemo::createScene()
{
	static const ColourValue skyColor(195.0 / 255, 232.0 / 255, 1);

	sceneMgr->setAmbientLight(ColourValue::White);
	sceneMgr->setShadowTechnique(SHADOWTYPE_TEXTURE_MODULATIVE);
	sceneMgr->setShadowFarDistance(460);
	sceneMgr->setShadowColour(ColourValue(0.75, 0.75, 0.75));
	sceneMgr->setShadowTextureSize(512);

	Camera* camera = sceneMgr->createCamera("MainCam");
	viewport = renderWin->addViewport(camera);
	viewport->setBackgroundColour(skyColor);
	camera->setAspectRatio(viewport->getActualWidth() / (Real)viewport->getActualHeight());

	maxGateWidth = (viewport->getActualWidth() < maxGateWidth)? viewport->getActualWidth() : maxGateWidth;
	maxGateHeight = (viewport->getActualHeight() < maxGateHeight)? viewport->getActualHeight() : maxGateHeight;

	parseResources();

	sceneMgr->setWorldGeometry("terrain.cfg");

	SceneNode* camNode = sceneMgr->getRootSceneNode()->createChildSceneNode("camNode");
	terrainCam = new TerrainCamera(camNode, camera, Ogre::Vector3(0, 40, -60), 35);

	camera->setFarClipDistance(2000);
	camera->setNearClipDistance(20);

	titleCanvas = new TitleCanvas(camera, "LucidaSans.ttf", sceneMgr);
	titleCanvas->setOcclusionHandler(this);

	// Set the ambient light and fog
	sceneMgr->setAmbientLight(ColourValue(0.5, 0.5, 0.5));
	sceneMgr->setFog(FOG_LINEAR, skyColor, 0, 400, 1200);

	sceneMgr->setSkyBox(true, "SkyBox", 1100);
	sceneMgr->setSkyDome(true, "Clouds", 4, 5, 1000, true);

	raySceneQuery = sceneMgr->createRayQuery(Ray());

	// Create sun-light
	Light* light = sceneMgr->createLight("Sun");
	light->setType(Light::LT_DIRECTIONAL);
	light->setDirection(Vector3(0, -1, -0.8));

	Plane plane(Vector3::UNIT_X, 0);

	MeshManager::getSingleton().createPlane("naviSelectionPlaneMesh1",
		ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, plane,
		512, 512,
		2,2,true,1,1,1,Vector3::UNIT_Y);

	MeshManager::getSingleton().createPlane("naviSelectionPlaneMesh2",
		ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, plane,
		512, 512,
		2,2,true,1,-1,1,Vector3::UNIT_Y);

	selectionNode = sceneMgr->createSceneNode("selectionNode");
	selectionPlaneNode = selectionNode->createChildSceneNode("selectionPlaneNode");

	Entity* ent;
	SceneNode* node;

	int sPlaneWidth = DEFAULT_GATE_WIDTH + GATE_SELECTION_WIDTH;
	int sPlaneHeight = DEFAULT_GATE_HEIGHT + GATE_SELECTION_WIDTH;
	selectionPlaneNode->scale(Vector3(1, sPlaneHeight / 512.0, sPlaneWidth / 512.0));

	ent = sceneMgr->createEntity("selectionPlane", "naviSelectionPlaneMesh1");
	ent->setMaterialName("selected");
	ent->setCastShadows(false);
	ent->setQueryFlags(SELECTION_PLANE_1_MASK);

	node = selectionPlaneNode->createChildSceneNode(Vector3(-3, 0, 0));
	node->attachObject(ent);

	ent = sceneMgr->createEntity("selectionPlane2", "naviSelectionPlaneMesh2");
	ent->setMaterialName("selected");
	ent->setCastShadows(false);
	ent->setQueryFlags(SELECTION_PLANE_2_MASK);
	
	node = selectionPlaneNode->createChildSceneNode(Vector3(3, 0, 0));
	node->yaw(Degree(-180));
	node->attachObject(ent);

	MeshManager::getSingleton().createPlane("naviResizePickingPlane",
	ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, plane,
	maxGateWidth, maxGateHeight, 2,2,true,1,1,1,Vector3::UNIT_Y);

	ent = sceneMgr->createEntity("resizePickingPlane", "naviResizePickingPlane");
	ent->setMaterialName("resizePickingPlaneMat");
	ent->setCastShadows(false);
	ent->setQueryFlags(RESIZE_PICKING_PLANE_MASK);

	resizePickingNode = selectionNode->createChildSceneNode("resizePickingNode");
	resizePickingNode->translate(Vector3(0, (maxGateHeight - (DEFAULT_GATE_HEIGHT)) / 2,
		(maxGateWidth - (DEFAULT_GATE_WIDTH)) / -2));
	resizePickingNode->attachObject(ent);
	resizePickingNode->setVisible(false);

	Rectangle2D* fullscreenShade = new Ogre::Rectangle2D(true);
	fullscreenShade->setCorners(-1, 1, 1, -1);
	fullscreenShade->setBoundingBox(AxisAlignedBox(AxisAlignedBox::EXTENT_INFINITE)); 
	fullscreenShade->setMaterial("FullscreenShade");

	fullscreenShadeNode = sceneMgr->getRootSceneNode()->createChildSceneNode("fullscreenShadeNode");
	fullscreenShadeNode->attachObject(fullscreenShade);
	fullscreenShadeNode->setVisible(false);

	Node::ChildNodeIterator terrainNodeIter = sceneMgr->getSceneNode("Terrain")->getChild(0)->getChildIterator();

	while(terrainNodeIter.hasMoreElements())
	{
		SceneNode* n = (SceneNode*)terrainNodeIter.getNext();
		MovableObject* m = n->getAttachedObject(0);
		m->setQueryFlags(TERRAIN_TILE_MASK);		
	}
}

void NaviDemo::setupNavis()
{
	// Create the NaviManager make our Navis
	naviMgr = new NaviManager(viewport, "..\\Media");

	menu = naviMgr->createNavi("menu", 120, 455, NaviPosition(Left), true);
	menu->loadFile("menu.html");
	menu->bind("create", NaviDelegate(this, &NaviDemo::onCreate));
	menu->bind("examples", NaviDelegate(this, &NaviDemo::onExamples));
	menu->bind("help", NaviDelegate(this, &NaviDemo::onHelp));
	menu->bind("exit", NaviDelegate(this, &NaviDemo::onExit));
	menu->setTransparent(true);

	navibar = naviMgr->createNavi("navibar", 715, 62, NaviPosition(TopCenter), true);
	navibar->loadFile("navibar.html");
	navibar->bind("goBack", NaviDelegate(this, &NaviDemo::onGoBack));
	navibar->bind("goForward", NaviDelegate(this, &NaviDemo::onGoForward));
	navibar->bind("navigateTo", NaviDelegate(this, &NaviDemo::onNavigateTo));
	navibar->bind("toggleFullscreen", NaviDelegate(this, &NaviDemo::onToggleFullscreen));
	navibar->bind("destroy", NaviDelegate(this, &NaviDemo::onDestroy));
	navibar->bind("_changeKeyboardFocus", NaviDelegate(this, &NaviDemo::onChangeKeyboardFocus));
	navibar->setTransparent(true);
	navibar->hide();

	examples = naviMgr->createNavi("examples", 500, 512, NaviPosition(Center, 0, 0), true);
	examples->loadFile("examples.html");
	examples->bind("create", NaviDelegate(this, &NaviDemo::onCreate));
	examples->setTransparent(true);
	examples->hide();

	help = naviMgr->createNavi("help", 490, 460, NaviPosition(Center, -30, 0), true);
	help->loadFile("help.html");
	help->setTransparent(true);
	help->hide();

	status = naviMgr->createNavi("status", 550, 80, NaviPosition(BottomCenter), true);
	status->loadFile("statusText.html");
	status->setTransparent(true);
	status->hide();
}

void NaviDemo::Update()
{
	naviMgr->Update();
	Root::getSingleton().renderOneFrame();
	Ogre::WindowEventUtilities::messagePump();
	titleCanvas->update();
	terrainCam->update();

	static long lastTime = 0;

	static long sinceLastInputCapture = 0;

	sinceLastInputCapture += timer.getMilliseconds() - lastTime;

	if(sinceLastInputCapture > 1000 / INPUT_HEARTBEAT)
	{
		InputManager::getSingletonPtr()->capture();
		sinceLastInputCapture = 0;
	}

	if(!naviMgr->hasKeyboardFocus())
	{
		Ogre::Real delta = MOVE_RATE * (timer.getMilliseconds() - lastTime) / (Ogre::Real)1000;
		
		OIS::Keyboard* keyboard = inputMgr->getKeyboard();

		bool isTranslating = false;
		Vector3 translation(0, 0, 0);

		if(keyboard->isKeyDown(OIS::KC_W) || keyboard->isKeyDown(OIS::KC_UP))
		{
			translation += Vector3(0, 0, delta);
			isTranslating = true;
		}

		if(keyboard->isKeyDown(OIS::KC_S) || keyboard->isKeyDown(OIS::KC_DOWN))
		{
			translation += Vector3(0, 0, -delta);
			isTranslating = true;
		}

		if(keyboard->isKeyDown(OIS::KC_A) || keyboard->isKeyDown(OIS::KC_LEFT))
		{
			translation += Vector3(delta, 0, 0);
			isTranslating = true;
		}

		if(keyboard->isKeyDown(OIS::KC_D) || keyboard->isKeyDown(OIS::KC_RIGHT))
		{
			translation += Vector3(-delta, 0, 0);
			isTranslating = true;
		}

		if(isTranslating)
		{
			terrainCam->translate(translation);
			terrainCam->clampToTerrain();
		}
	}

	lastTime = timer.getMilliseconds();
}

void NaviDemo::onCreate(Navi* caller, const OSM::JSArguments& args)
{
	std::string beginURL = DEFAULT_URL;
	int distanceOffset = 0;

	if(args.size() >= 1)
	{
		examples->hide(true);
		beginURL = args[0].toString().str();
	}

	if(args.size() == 2)
	{
		distanceOffset = args[1].toInteger();
	}

	std::string gateBaseID = "ng_" + StringConverter::toString(++gateIDCounter);

	Navi* navi = naviMgr->createNaviMaterial(gateBaseID, DEFAULT_GATE_WIDTH, DEFAULT_GATE_HEIGHT, true);
	navi->loadURL(beginURL);
	navi->bind("_beginNavigation", NaviDelegate(this, &NaviDemo::onBeginNavigation));
	navi->bind("_beginLoading", NaviDelegate(this, &NaviDemo::onBeginLoading));
	navi->bind("_finishLoading", NaviDelegate(this, &NaviDemo::onFinishLoading));
	navi->bind("_receiveTitle", NaviDelegate(this, &NaviDemo::onReceiveTitle));
	navi->bind("_changeTargetURL", NaviDelegate(this, &NaviDemo::onChangeTargetURL));
	navi->bind("_changeKeyboardFocus", NaviDelegate(this, &NaviDemo::onChangeKeyboardFocus));
	navi->bind("_openExternalLink", NaviDelegate(this, &NaviDemo::onOpenExternalLink));
	navi->bind("_webViewCrashed", NaviDelegate(this, &NaviDemo::onWebViewCrashed));

	Ogre::Real u1, v1, u2, v2;
	navi->getDerivedUV(u1, v1, u2, v2);

	ManualObject* planeObject = createManualPlane(gateBaseID + "_Mesh", DEFAULT_GATE_WIDTH, DEFAULT_GATE_HEIGHT, navi->getMaterialName(), u2, v2);
	planeObject->setQueryFlags(GATE_QUERY_MASK);
	planeObject->setCastShadows(true);

	titleCanvas->addTitle(planeObject, "");

	MaterialPtr mat = MaterialManager::getSingleton().getByName(navi->getMaterialName());
	mat->setReceiveShadows(false);
	Ogre::Pass* pass = mat->getTechnique(0)->getPass(0);
	pass->getTextureUnitState(0)->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
	pass->getTextureUnitState(0)->setTextureAnisotropy(8);
	pass->setDepthWriteEnabled(true);
	pass->setDepthCheckEnabled(true);
	pass->setLightingEnabled(false);
	pass->setCullingMode(Ogre::CULL_NONE);
	pass->setManualCullingMode(MANUAL_CULL_NONE);

	SceneNode* node = sceneMgr->getRootSceneNode()->createChildSceneNode();
	node->attachObject(planeObject);
	node->scale(0.25, 0.25, 0.25);
	node->setVisible(false);

	NaviGate* gate = new NaviGate();
	gate->navi = navi;
	gate->planeNode = node;
	gate->width = DEFAULT_GATE_WIDTH;
	gate->height = DEFAULT_GATE_HEIGHT;

	activeGates[gateIDCounter] = gate;

	terrainCam->orientPlaneToCamera(node, gate->height * GATE_DISTANCE, distanceOffset, distanceOffset + 270);
	node->setVisible(true);

	if(isFullscreen)
		onToggleFullscreen(0, JSArgs());

	focusGate(gate);
}

void NaviDemo::onExamples(Navi* caller, const OSM::JSArguments& args)
{
	if(examples->getVisibility())
		examples->hide(true);
	else
		examples->show(true);
}

void NaviDemo::onHelp(Navi* caller, const OSM::JSArguments& args)
{
	if(help->getVisibility())
		help->hide(true);
	else
		help->show(true);
}

void NaviDemo::onExit(Navi* caller, const OSM::JSArguments& args)
{
	shouldQuit = true;
}

void NaviDemo::onGoBack(Navi* caller, const OSM::JSArguments& args)
{
	if(focusedGate)
		focusedGate->navi->evaluateJS("history.go(-1)");
}

void NaviDemo::onGoForward(Navi* caller, const OSM::JSArguments& args)
{
	if(focusedGate)
		focusedGate->navi->evaluateJS("history.go(1)");
}

void NaviDemo::onNavigateTo(Navi* caller, const OSM::JSArguments& args)
{
	if(focusedGate && args.size() == 1)
		focusedGate->navi->loadURL(args[0].toString().str());
}

void NaviDemo::onToggleFullscreen(Navi* caller, const OSM::JSArguments& args)
{
	if(!focusedGate)
		return;

	if(!isFullscreen)
	{
		focusedGate->overlay = new NaviOverlay(focusedGate->navi->getName() + "overlay", viewport, focusedGate->width, focusedGate->height, NaviPosition(Center), 
			focusedGate->navi->getMaterialName(), 0, Back);
		Ogre::Real u1, v1, u2, v2;
		focusedGate->navi->getDerivedUV(u1, v1, u2, v2);
		focusedGate->overlay->panel->setUV(u1, v1, u2, v2);	
		focusedGate->overlay->show();
		menu->hide(true);
		fullscreenShadeNode->setVisible(true);
		titleCanvas->hide();
		isFullscreen = true;
	}
	else
	{
		delete focusedGate->overlay;
		MaterialPtr mat = MaterialManager::getSingleton().getByName(focusedGate->navi->getMaterialName());
		Ogre::Pass* pass = mat->getTechnique(0)->getPass(0);
		pass->getTextureUnitState(0)->setTextureFiltering(Ogre::TFO_ANISOTROPIC);
		pass->getTextureUnitState(0)->setTextureAnisotropy(8);
		pass->setDepthWriteEnabled(true);
		pass->setDepthCheckEnabled(true);
		focusedGate->overlay = 0;
		menu->show(true);
		fullscreenShadeNode->setVisible(false);
		titleCanvas->show();
		isFullscreen = false;
	}
}

void NaviDemo::onDestroy(Navi* caller, const OSM::JSArguments& args)
{
	if(focusedGate)
	{
		if(isFullscreen)
			onToggleFullscreen(0, JSArgs());

		int id = StringConverter::parseInt(focusedGate->navi->getName().substr(3));
		activeGates.erase(id);
		titleCanvas->removeTitle(focusedGate->planeNode->getAttachedObject(0));
		delete focusedGate;
		focusedGate = 0;
		focusGate(0);
	}
}

void NaviDemo::onBeginNavigation(Navi* caller, const OSM::JSArguments& args)
{
	NaviGate* gate = activeGates[StringConverter::parseInt(caller->getName().substr(3))];

	std::string url = args.at(0).toString().str();
	std::string frame = args.at(1).toString().str();

	if(frame.empty())
	{
		gate->url = url;

		if(gate == focusedGate)
			navibar->evaluateJS("updateURL(?)", JSArgs(url));
	}
}

void NaviDemo::onBeginLoading(Navi* caller, const OSM::JSArguments& args)
{
	NaviGate* gate = activeGates[StringConverter::parseInt(caller->getName().substr(3))];

	if(gate == focusedGate)
	{
		if(focusedGate->targetURL.empty())
		{
			status->show(true);
			status->evaluateJS("updateStatus(?)", JSArgs("Loading: " + args.at(0).toString().str()));
		}
	}
}

void NaviDemo::onFinishLoading(Navi* caller, const OSM::JSArguments& args)
{
	NaviGate* gate = activeGates[StringConverter::parseInt(caller->getName().substr(3))];

	if(gate == focusedGate)
	{
		if(focusedGate->targetURL.empty())
		{
			status->hide(true, 600);
		}
	}
}

void NaviDemo::onReceiveTitle(Navi* caller, const OSM::JSArguments& args)
{
	NaviGate* gate = activeGates[StringConverter::parseInt(caller->getName().substr(3))];

	std::string title = args.at(0).toString().str();
	std::string frame = args.at(1).toString().str();

	if(frame.empty())
	{
		if(title.length() > 38)
			title = title.substr(0, 38) + "...";

		gate->title = title;

		titleCanvas->editTitle(gate->planeNode->getAttachedObject(0), NaviUtilities::toWide(title), 
			gate == focusedGate? SELECTED_TITLE_COLOR : TITLE_COLOR);
	}
}

void NaviDemo::onChangeTargetURL(Navi* caller, const OSM::JSArguments& args)
{
	NaviGate* gate = activeGates[StringConverter::parseInt(caller->getName().substr(3))];

	if(gate == focusedGate)
	{
		gate->targetURL = args.at(0).toString().str();

		if(gate->targetURL.empty())
		{
			status->hide(true, 600);
		}
		else
		{
			status->show(true);
			status->evaluateJS("updateStatus(?)", JSArgs(gate->targetURL));
		}
	}
}

void NaviDemo::onChangeKeyboardFocus(Navi* caller, const OSM::JSArguments& args)
{
	if(caller == navibar && focusedGate)
	{
		if(args[0].toBoolean())
			focusedGate->navi->setAlwaysReceivesKeyboard(false);
		else
			focusedGate->navi->setAlwaysReceivesKeyboard(true);
	}
	else if(caller->getName().substr(0, 3) == "ng_")
	{
		NaviGate* gate = activeGates[StringConverter::parseInt(caller->getName().substr(3))];
	}
}

void NaviDemo::onOpenExternalLink(Navi* caller, const OSM::JSArguments& args)
{
	onCreate(caller, JSArgs(args[0], -50));
}

void NaviDemo::onWebViewCrashed(Navi* caller, const OSM::JSArguments& args)
{
	NaviGate* gate = activeGates[StringConverter::parseInt(caller->getName().substr(3))];

	std::string title = "This page has CRASHED!";

	gate->title = title;

	titleCanvas->editTitle(gate->planeNode->getAttachedObject(0), NaviUtilities::toWide(title), 
		gate == focusedGate? SELECTED_TITLE_COLOR : TITLE_COLOR);
}

void NaviDemo::parseResources()
{
    ConfigFile cf;
	cf.load("resources.cfg");
    ConfigFile::SectionIterator seci = cf.getSectionIterator();

	Ogre::String secName, typeName, archName;
    while(seci.hasMoreElements())
    {
        secName = seci.peekNextKey();
        ConfigFile::SettingsMultiMap *settings = seci.getNext();
        ConfigFile::SettingsMultiMap::iterator i;
        for(i = settings->begin(); i != settings->end(); ++i)
        {
            typeName = i->first;
            archName = i->second;
            ResourceGroupManager::getSingleton().addResourceLocation(archName, typeName, secName);
        }
    }

	ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
}

void NaviDemo::loadInputSystem()
{
	inputMgr = InputManager::getSingletonPtr();
    inputMgr->initialise(renderWin);
    inputMgr->addMouseListener(this, "NaviDemoMouseListener");
	inputMgr->addKeyListener(this, "NaviDemoKeyListener");
}

Ogre::ManualObject* NaviDemo::createManualPlane(const std::string& name, int width, int height, const std::string& matName, Ogre::Real u2, Ogre::Real v2)
{
	ManualObject* manual = sceneMgr->createManualObject(name);
	manual->begin(matName, RenderOperation::OT_TRIANGLE_LIST);

	Ogre::Real a = height / 2.0;
	Ogre::Real b = width / 2.0;
	manual->position(0, -a, b);
	manual->textureCoord(0, v2);
	manual->position(0, a, b);
	manual->textureCoord(0, 0);
	manual->position(0, a, -b);
	manual->textureCoord(u2, 0);
	manual->position(0, -a, -b);
	manual->textureCoord(u2, v2);
	manual->quad(3,2,1,0);
	manual->end();

	// 3, 2, 1, 1, 0, 3

	return manual;
}

void NaviDemo::focusGate(NaviGate* gate)
{
	if(focusedGate)
	{
		titleCanvas->editTitle(focusedGate->planeNode->getAttachedObject(0), NaviUtilities::toWide(focusedGate->title), TITLE_COLOR);
		focusedGate->planeNode->removeChild(selectionNode);
		focusedGate->navi->setAlwaysReceivesKeyboard(false);
	}

	focusedGate = gate;

	if(!focusedGate)
	{
		navibar->hide(true);
		status->hide(true);
	}
	else
	{
		if(!navibar->getVisibility())
			navibar->show(true);

		//focusedGate->planeNode->showBoundingBox(true);
		focusedGate->planeNode->addChild(selectionNode);

		int sPlaneWidth = focusedGate->width + GATE_SELECTION_WIDTH;
		int sPlaneHeight = focusedGate->height + GATE_SELECTION_WIDTH;
		selectionPlaneNode->setPosition(0, 0, 0);
		selectionPlaneNode->setScale(1, sPlaneHeight / 512.0, sPlaneWidth / 512.0);

		resizePickingNode->setPosition(Vector3(0, (maxGateHeight -  focusedGate->height) / 2, 
			(maxGateWidth - focusedGate->width) / -2));

		navibar->evaluateJS("updateURL(?)", JSArgs(focusedGate->url));
		titleCanvas->editTitle(focusedGate->planeNode->getAttachedObject(0), NaviUtilities::toWide(focusedGate->title), SELECTED_TITLE_COLOR);
		focusedGate->navi->setAlwaysReceivesKeyboard(true);
		// update navibar with new information
	}
}

void NaviDemo::resizeFocusedGate(int width, int height)
{
	focusedGate->navi->resize(width, height);
	naviMgr->Update();
	Ogre::Real u1, v1, u2, v2;
	focusedGate->navi->getDerivedUV(u1, v1, u2, v2);

	ManualObject* planeObject = (ManualObject*)focusedGate->planeNode->getAttachedObject(0);
	titleCanvas->removeTitle(planeObject);
	focusedGate->planeNode->detachAllObjects();
	std::string gateBaseID = focusedGate->navi->getName();

	sceneMgr->destroyManualObject(planeObject);

	planeObject = createManualPlane(gateBaseID + "_Mesh", width, height, focusedGate->navi->getMaterialName(), u2, v2);
	planeObject->setQueryFlags(GATE_QUERY_MASK);
	planeObject->setCastShadows(true);

	titleCanvas->addTitle(planeObject, NaviUtilities::toWide(focusedGate->title), SELECTED_TITLE_COLOR);

	focusedGate->planeNode->attachObject(planeObject);
	
	focusedGate->planeNode->translate(Ogre::Vector3(0, (focusedGate->height - height) / -8, 
		(focusedGate->width - width) / 8), Node::TS_LOCAL);

	int sPlaneWidth = width + GATE_SELECTION_WIDTH;
	int sPlaneHeight = height + GATE_SELECTION_WIDTH;
	selectionPlaneNode->setPosition(0, 0, 0);
	selectionPlaneNode->setScale(1, sPlaneHeight / 512.0, sPlaneWidth / 512.0);

	resizePickingNode->setPosition(Vector3(0, (maxGateHeight - height) / 2, (maxGateWidth - width) / -2));

	focusedGate->width = width;
	focusedGate->height = height;

	terrainCam->clampPlaneToTerrain(focusedGate->planeNode, focusedGate->height * GATE_DISTANCE);
}

bool NaviDemo::isScreenPointOccluded(int x, int y)
{
	bool hit = false;

	Ray mouseRay = terrainCam->getCamera()->getCameraToViewportRay((Ogre::Real)x / viewport->getActualWidth(), 
		(Ogre::Real)y / viewport->getActualHeight());
	raySceneQuery->setRay(mouseRay);
	raySceneQuery->setQueryMask(GATE_QUERY_MASK);
	raySceneQuery->setQueryTypeMask(SceneManager::ENTITY_TYPE_MASK);
	Ogre::Real dist;
	NaviGate* tempGate = 0;

	RaySceneQueryResult &result = raySceneQuery->execute();

	for(RaySceneQueryResult::iterator i = result.begin(); i != result.end(); i++)
	{
		if(i->movable && i->movable->getQueryFlags() == GATE_QUERY_MASK)
		{
			tempGate = activeGates[StringConverter::parseInt(i->movable->getName().substr(3))];

			if(hit = rayHitPlane(i->movable, this->terrainCam->getCamera(), tempGate->width, tempGate->height, mouseRay, x, y, dist))
				break;
		}
	}

	raySceneQuery->clearResults();

	return hit;
}

PlaneObject* NaviDemo::getPlaneObjectAtPoint(int x, int y, int& localX, int& localY)
{
	PlaneObject* result = 0;

	Ray mouseRay = terrainCam->getCamera()->getCameraToViewportRay((Ogre::Real)x / viewport->getActualWidth(),
		(Ogre::Real)y / viewport->getActualHeight());
	raySceneQuery->setRay(mouseRay);
	raySceneQuery->setQueryMask(GATE_QUERY_MASK | SELECTION_PLANE_1_MASK | SELECTION_PLANE_2_MASK);
	raySceneQuery->setQueryTypeMask(SceneManager::ENTITY_TYPE_MASK);

	RaySceneQueryResult &queryResult = raySceneQuery->execute();

	Ogre::Real closestDist = -1;
	Ogre::Real dist;
	NaviGate* tempGate = 0;

	for(RaySceneQueryResult::iterator i = queryResult.begin(); i != queryResult.end(); i++)
	{
		if(!i->movable)
			continue;

		if(i->movable->getQueryFlags() == GATE_QUERY_MASK)
		{
			tempGate = activeGates[StringConverter::parseInt(i->movable->getName().substr(3))];

			if(rayHitPlane(i->movable, this->terrainCam->getCamera(), tempGate->width, tempGate->height, mouseRay, x, y, dist))
			{
				if(closestDist < 0 || dist < closestDist)
				{
					closestDist = dist;
					result = tempGate;
					localX = x;
					localY = y;
				}
			}
		}
		else if(i->movable->getQueryFlags() == SELECTION_PLANE_1_MASK || i->movable->getQueryFlags() == SELECTION_PLANE_2_MASK)
		{

			if(rayHitPlane(i->movable, this->terrainCam->getCamera(), 512, 
				512, mouseRay, x, y, dist))
			{
				if(closestDist < 0 || dist < closestDist)
				{
					 closestDist = dist;
					 result = i->movable->getQueryFlags() == SELECTION_PLANE_1_MASK? selectionPlane1 : selectionPlane2;
					 localX = x;
					 localY = y;

				}
			}
		}
	}

	raySceneQuery->clearResults();


	return result;
}

bool NaviDemo::isPointOverResizePickingPlane(int x, int y, int& localX, int& localY)
{
	Ray mouseRay = terrainCam->getCamera()->getCameraToViewportRay((Ogre::Real)x / viewport->getActualWidth(), 
		(Ogre::Real)y / viewport->getActualHeight());
	raySceneQuery->setRay(mouseRay);
	raySceneQuery->setQueryMask(RESIZE_PICKING_PLANE_MASK);
	raySceneQuery->setQueryTypeMask(SceneManager::ENTITY_TYPE_MASK);

	RaySceneQueryResult &result = raySceneQuery->execute();
	Ogre::Real dist;

	for(RaySceneQueryResult::iterator i = result.begin(); i != result.end(); i++)
		if(i->movable && i->movable->getQueryFlags() == RESIZE_PICKING_PLANE_MASK)
			if(rayHitPlane(i->movable, this->terrainCam->getCamera(), maxGateWidth, maxGateHeight, 
				mouseRay, localX, localY, dist))
				return true;

	return false;
}

bool NaviDemo::isPointOverNaviGate(int screenX, int screenY, int screenWidth, int screenHeight, int& naviX, int& naviY, NaviGate*& resultGate)
{
	resultGate = 0;

	Ray mouseRay = terrainCam->getCamera()->getCameraToViewportRay((Ogre::Real)screenX / screenWidth, (Ogre::Real)screenY / screenHeight);
	raySceneQuery->setRay(mouseRay);
	raySceneQuery->setQueryMask(GATE_QUERY_MASK);
	raySceneQuery->setQueryTypeMask(SceneManager::ENTITY_TYPE_MASK);

	RaySceneQueryResult &result = raySceneQuery->execute();

	Ogre::Real closestDist = -1;
	Ogre::Real dist;
	int x, y;
	NaviGate* tempGate = 0;

	for(RaySceneQueryResult::iterator i = result.begin(); i != result.end(); i++)
	{
		if(i->movable && i->movable->getQueryFlags() == GATE_QUERY_MASK)
		{
			tempGate = activeGates[StringConverter::parseInt(i->movable->getName().substr(3))];
			 if(rayHitPlane(i->movable, this->terrainCam->getCamera(), tempGate->width, tempGate->height, mouseRay, x, y, dist))
			 {
				 if(closestDist < 0 || dist < closestDist)
				 {
					 closestDist = dist;
					 resultGate = tempGate;
					 naviX = x;
					 naviY = y;
				 }
			 }
		}
	}

	return !!resultGate;
}

bool NaviDemo::mouseMoved(const OIS::MouseEvent &arg)
{
	if(isResizingGate && focusedGate)
	{
		int localX, localY;
		if(isPointOverResizePickingPlane(arg.state.X.abs, arg.state.Y.abs, localX, localY))
		{
			localY = maxGateHeight - localY;

			resizeWidth = localX;
			resizeHeight = localY;
		}
		else
		{
			bool faceBack = false;
			if(Ogre::Plane(Ogre::Vector3(focusedGate->planeNode->_getDerivedOrientation().xAxis()), 
				focusedGate->planeNode->_getDerivedPosition()).getSide(terrainCam->getCamera()->getDerivedPosition()) == Ogre::Plane::NEGATIVE_SIDE)
				faceBack = true;

			resizeWidth += faceBack? -arg.state.X.rel : arg.state.X.rel;
			resizeHeight += -arg.state.Y.rel;
		}

		if(resizeWidth > maxGateWidth)
			resizeWidth = maxGateWidth;
		else if(resizeWidth < minGateWidth)
			resizeWidth = minGateWidth;

		if(resizeHeight > maxGateHeight)
			resizeHeight = maxGateHeight;
		else if(resizeHeight < minGateHeight)
			resizeHeight = minGateHeight;

		int sPlaneWidth = resizeWidth + GATE_SELECTION_WIDTH;
		int sPlaneHeight = resizeHeight + GATE_SELECTION_WIDTH;
		selectionPlaneNode->setScale(1, 1, 1);
		selectionPlaneNode->setPosition(0, (sPlaneHeight - focusedGate->height) / 2 - (GATE_SELECTION_WIDTH / 2.0), 
			(sPlaneWidth - focusedGate->width) / -2 - (GATE_SELECTION_WIDTH / -2.0));
		selectionPlaneNode->setScale(1, sPlaneHeight / 512.0, sPlaneWidth / 512.0);
	}

	if(isMovingGate && focusedGate)
	{
		Ogre::Vector3 translation = terrainCam->getCamera()->getDerivedOrientation() * Ogre::Vector3(arg.state.X.rel, 0, arg.state.Y.rel * 1.6);
		translation *= terrainCam->getCamera()->getDerivedPosition().distance(focusedGate->planeNode->_getDerivedPosition()) * 0.001;
		focusedGate->planeNode->translate(translation);
		terrainCam->clampPlaneToTerrain(focusedGate->planeNode, focusedGate->height * GATE_DISTANCE);
	}

	if(isRotatingGate && focusedGate)
	{
		Ogre::Real rotation = arg.state.X.rel * 
			terrainCam->getCamera()->getDerivedPosition().distance(focusedGate->planeNode->_getDerivedPosition()) * 0.001;
		focusedGate->planeNode->rotate(Ogre::Vector3::UNIT_Y, Ogre::Degree(rotation));
	}

	if(isDraggingGate && focusedGate)
	{
		int localX, localY;
		NaviGate* resultGate = 0;
		bool overGate = isPointOverNaviGate(arg.state.X.abs, arg.state.Y.abs, arg.state.width, arg.state.height, localX, localY, resultGate);

		if(overGate && resultGate == focusedGate)
		{
			focusedGate->x = localX;
			focusedGate->y = localY;
			focusedGate->navi->injectMouseMove(localX, localY);
		}
		else
		{
			// lol, I'm totally faking the coordinate projection here for external points outside of our mesh-- 
			// we've saved the coordinates from previous hit-tests in NaviGate::x and y, so now we just translate 
			// them relatively by the mouse move event.

			bool faceBack = false;
			if(Ogre::Plane(Ogre::Vector3(focusedGate->planeNode->_getDerivedOrientation().xAxis()), 
				focusedGate->planeNode->_getDerivedPosition()).getSide(terrainCam->getCamera()->getDerivedPosition()) == Ogre::Plane::NEGATIVE_SIDE)
				faceBack = true;

			focusedGate->x += faceBack? -arg.state.X.rel : arg.state.X.rel;
			focusedGate->y += arg.state.Y.rel;
			focusedGate->navi->injectMouseMove(focusedGate->x, focusedGate->y);
		}
	}
	if(arg.state.buttonDown(OIS::MB_Right) && !isDraggingNavi && !isRotatingGate)
	{
		// If we are in camera-pivot state, spin/pitch the camera based
		// on relative mouse movement.
		terrainCam->spin(Degree(arg.state.X.rel * 0.14));
		terrainCam->pitch(Degree(arg.state.Y.rel * 0.1));
	}

	if(arg.state.Z.rel != 0) naviMgr->injectMouseWheel(arg.state.Z.rel / 3);

	if(!naviMgr->injectMouseMove(arg.state.X.abs, arg.state.Y.abs))
	{
		if(isFullscreen)
		{
			if(arg.state.Z.rel)
				focusedGate->navi->injectMouseWheel(arg.state.Z.rel / 3);
			else
				focusedGate->navi->injectMouseMove(focusedGate->overlay->getRelativeX(arg.state.X.abs), 
					focusedGate->overlay->getRelativeY(arg.state.Y.abs));
		}
		else
		{
			int localX, localY;
			NaviGate* resultGate = 0;
			if(isPointOverNaviGate(arg.state.X.abs, arg.state.Y.abs, arg.state.width, arg.state.height, localX, localY, resultGate))
			{
				resultGate->x = localX;
				resultGate->y = localY;

				if(arg.state.Z.rel)
					resultGate->navi->injectMouseWheel(arg.state.Z.rel / 3);
				else
					resultGate->navi->injectMouseMove(localX, localY);
			}
		}
	}

	return true;
}

bool NaviDemo::mousePressed(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	if(inputMgr->getKeyboard()->isKeyDown(OIS::KC_LSHIFT) || inputMgr->getKeyboard()->isKeyDown(OIS::KC_RSHIFT))
	{
		if(isMovingGate && id == OIS::MB_Right)
		{
			isRotatingGate = true;
			return true;
		}
		else if(isRotatingGate && id == OIS::MB_Left)
		{
			isMovingGate = true;
			return true;
		}
		else if(id == OIS::MB_Right)
		{
			int localX, localY;
			NaviGate* resultGate = 0;
			if(isPointOverNaviGate(arg.state.X.abs, arg.state.Y.abs, arg.state.width, arg.state.height, localX, localY, resultGate))
			{
				if(focusedGate != resultGate)
					focusGate(resultGate);

				isRotatingGate = true;
			}
			return true;
		}
	}

	bool isOverNavi = naviMgr->injectMouseDown(id);

	if(!isOverNavi && id == OIS::MB_Left)
	{
		if(isFullscreen)
		{
			if(!focusedGate->overlay->isWithinBounds(arg.state.X.abs, arg.state.Y.abs) && !isOverNavi)
				onToggleFullscreen(0, JSArgs());
			else
				focusedGate->navi->injectMouseDown(focusedGate->overlay->getRelativeX(arg.state.X.abs), 
					focusedGate->overlay->getRelativeY(arg.state.Y.abs));
		}
		else
		{
			if(inputMgr->getKeyboard()->isKeyDown(OIS::KC_LSHIFT) || inputMgr->getKeyboard()->isKeyDown(OIS::KC_RSHIFT))
			{
				int localX, localY;
				PlaneObject* planeObj = getPlaneObjectAtPoint(arg.state.X.abs, arg.state.Y.abs, localX, localY);
				
				if(planeObj)
				{
					NaviGate* naviGateObj = 0;
					if(naviGateObj = dynamic_cast<NaviGate*>(planeObj))
					{
						if(focusedGate != naviGateObj)
							focusGate(naviGateObj);

						isMovingGate = true;

						return true;
					}

					SelectionPlane* selectionPlaneObj = 0;
					if(selectionPlaneObj = dynamic_cast<SelectionPlane*>(planeObj))
					{
						if(selectionPlaneObj->id == 2)
							localX = 512 - localX;

						Ogre::Real resizeHandleWidth = 64;

						if(localX > 512 - resizeHandleWidth && 
							localY < resizeHandleWidth)
						{
							resizeWidth = focusedGate->width;
							resizeHeight = focusedGate->height;
							isResizingGate = true;
						}
						else
						{
							isMovingGate = true;
						}
						return true;
					}
				}
				else
				{
					focusGate(0);
				}
			}
			else
			{
				int localX, localY;
				NaviGate* resultGate = 0;
				if(isPointOverNaviGate(arg.state.X.abs, arg.state.Y.abs, arg.state.width, arg.state.height, localX, localY, resultGate))
				{
					if(focusedGate != resultGate)
						focusGate(resultGate);

					resultGate->navi->injectMouseDown(localX, localY);
					isDraggingGate = true;
				}
				else
				{
					focusGate(0);
				}
			}
		}
	}
	else if(id == OIS::MB_Right)
	{
		focusGate(0);

		if(isOverNavi)
		{
			isDraggingNavi = true;
		}
	}

	return true;
}

bool NaviDemo::mouseReleased(const OIS::MouseEvent &arg, OIS::MouseButtonID id)
{
	if(focusedGate)
	{
		if(id == OIS::MB_Left)
		{
			if(isDraggingGate)
			{
				focusedGate->navi->injectMouseUp(focusedGate->x, focusedGate->y);
				isDraggingGate = false;
				return true;
			}
			else if(isMovingGate)
			{
				isMovingGate = false;
				return true;
			}
			else if(isResizingGate)
			{
				isResizingGate = false;

				resizeFocusedGate(resizeWidth, resizeHeight);
				// TODO: Handle resizing here!
				return true;
			}
		}
		else if(id == OIS::MB_Right && isRotatingGate)
		{
			isRotatingGate = false;
			return true;
		}
	}

	if(!naviMgr->injectMouseUp(id) && id == OIS::MB_Left)
	{
		if(isFullscreen)
		{
			focusedGate->navi->injectMouseUp(focusedGate->overlay->getRelativeX(arg.state.X.abs), 
				focusedGate->overlay->getRelativeY(arg.state.Y.abs));
		}
		else
		{
			int localX, localY;
			NaviGate* resultGate = 0;
			if(isPointOverNaviGate(arg.state.X.abs, arg.state.Y.abs, arg.state.width, arg.state.height, localX, localY, resultGate))
				resultGate->navi->injectMouseUp(localX, localY);
		}
	}
	else if(isDraggingNavi && id == OIS::MB_Right)
	{
		isDraggingNavi = false;
	}

	return true;
}

bool NaviDemo::keyPressed( const OIS::KeyEvent &arg )
{
	if(inputMgr->getKeyboard()->isKeyDown(OIS::KC_LSHIFT) || inputMgr->getKeyboard()->isKeyDown(OIS::KC_RSHIFT))
	{
		((Entity*)((SceneNode*)selectionPlaneNode->getChild(0))->getAttachedObject(0))->setMaterialName("selectedShift");
		((Entity*)((SceneNode*)selectionPlaneNode->getChild(1))->getAttachedObject(0))->setMaterialName("selectedShift");
	}

	if(naviMgr->isAnyNaviFocused())
		return true;

	return true;
}

bool NaviDemo::keyReleased( const OIS::KeyEvent &arg )
{
	if(!inputMgr->getKeyboard()->isKeyDown(OIS::KC_LSHIFT) && !inputMgr->getKeyboard()->isKeyDown(OIS::KC_RSHIFT))
	{
		((Entity*)((SceneNode*)selectionPlaneNode->getChild(0))->getAttachedObject(0))->setMaterialName("selected");
		((Entity*)((SceneNode*)selectionPlaneNode->getChild(1))->getAttachedObject(0))->setMaterialName("selected");
	}

	switch(arg.key)
	{
	case OIS::KC_ESCAPE:
		shouldQuit = true;
		break;
	case OIS::KC_F1:
	{
		const RenderTarget::FrameStats& stats = renderWin->getStatistics();
		
		printf("_________________________\n");
		printf("%-14s%d\n", "Current FPS:", (int)stats.lastFPS);
		printf("%-14s%d\n", "Triangles:", (int)stats.triangleCount);
		printf("%-14s%d\n", "Batches:", (int)stats.batchCount);
		printf("=========================\n");
		break;
	}
	case OIS::KC_F2:
		sceneMgr->getRootSceneNode()->flipVisibility(true);
		break;
	case OIS::KC_F3:
		naviMgr->resetAllPositions();
		break;
	}

	return true;
}

void NaviDemo::windowMoved(RenderWindow* rw) {}

void NaviDemo::windowResized(RenderWindow* rw) 
{
	inputMgr->setWindowExtents(rw->getWidth(), rw->getHeight());
}

void NaviDemo::windowClosed(RenderWindow* rw) 
{
	shouldQuit = true;
}

void NaviDemo::windowFocusChange(RenderWindow* rw) {}

// Based on code from the 'BrowserPlaneDemo'
bool rayHitPlane(Ogre::MovableObject* plane, Ogre::Camera* camera, int planeWidth, int planeHeight, const Ray& ray, int& outX, int& outY, Ogre::Real& dist)
{
	bool hit = false;
	Ogre::Real closest_distance = -1.0f;

	bool testBackside = false;
	if(Ogre::Plane(Ogre::Vector3(plane->getParentNode()->_getDerivedOrientation().xAxis()), 
		plane->getParentNode()->_getDerivedPosition()).getSide(camera->getDerivedPosition()) == Ogre::Plane::NEGATIVE_SIDE)
		testBackside = true;

	size_t vertexCount, indexCount;
	Ogre::Vector3 *vertices;
	unsigned long *indices;

	/**
	* Note: Since this is just a demo, care hasn't been taken to optimize everything
	*		to the extreme, however, for a production setting, it would be a bit
	*		more efficient to save the mesh information for each Entity we care
	*		about so that we aren't needlessly calling the following function every
	*		time we wish to a little mouse-picking.
	*/
	if(Entity* e = dynamic_cast<Entity*>(plane))
	{
		// get the mesh information
		GetMeshInformation(((Entity*)plane)->getMesh(), vertexCount, vertices, indexCount, indices,             
			plane->getParentNode()->_getDerivedPosition(), plane->getParentNode()->_getDerivedOrientation(),
			plane->getParentNode()->getScale());
	}
	else
	{
		// assume we're a manual object

		vertexCount = 4;
		indexCount = 6;
		vertices = new Ogre::Vector3[vertexCount];
		indices = new unsigned long[indexCount];

		Vector3 position = plane->getParentNode()->_getDerivedPosition();
		Quaternion orient = plane->getParentNode()->_getDerivedOrientation();
		Vector3 scale = plane->getParentNode()->getScale();

		Ogre::Real a = planeHeight / 2.0;
		Ogre::Real b = planeWidth / 2.0;

		vertices[0] = (orient * (Vector3(0, -a, b) * scale)) + position;
		vertices[1] = (orient * (Vector3(0, a, b) * scale)) + position;
		vertices[2] = (orient * (Vector3(0, a, -b) * scale)) + position;
		vertices[3] = (orient * (Vector3(0, -a, -b) * scale)) + position;

		indices[0] = 3;
		indices[1] = 2;
		indices[2] = 1;
		indices[3] = 1;
		indices[4] = 0;
		indices[5] = 3;
	}

	// test for hitting individual triangles on the mesh
	bool new_closest_found = false;
	std::pair<bool, Ogre::Real> intersectTest;
	for(int i = 0; i < (int)indexCount; i += 3)
	{
		/**
		* Note: We could also check for hits on the back-side of the plane by swapping
		*		the 'true' and 'false' tokens in the below statement.
		*/

		// check for a hit against this triangle
		if(testBackside)
			intersectTest = Ogre::Math::intersects(ray, vertices[indices[i]], vertices[indices[i+1]], 
				vertices[indices[i+2]], false, true);
		else
			intersectTest = Ogre::Math::intersects(ray, vertices[indices[i]], vertices[indices[i+1]], 
				vertices[indices[i+2]], true, false);

		// if it was a hit check if its the closest
		if(intersectTest.first)
		{
			if((closest_distance < 0.0f) || (intersectTest.second < closest_distance))
			{
				// this is the closest so far, save it off
				closest_distance = intersectTest.second;
				new_closest_found = true;
			}
		}
	}

	// free the vertices and indices memory
	delete[] vertices;
	delete[] indices;

	// get the parent node
	Ogre::SceneNode* browserNode = plane->getParentSceneNode();

	// if we found a new closest raycast for this object, update the
	// closest_result before moving on to the next object.
	if(browserNode && new_closest_found)
	{
		Ogre::Vector3 pointOnPlane = ray.getPoint(closest_distance);
		Ogre::Quaternion quat = browserNode->_getDerivedOrientation().Inverse();
		Ogre::Vector3 result = quat * pointOnPlane;
		Ogre::Vector3 positionInWorld = quat * browserNode->_getDerivedPosition();
		Ogre::Vector3 scale = browserNode->_getDerivedScale();

		outX = (planeWidth/2) - ((result.z - positionInWorld.z)/scale.z);
		outY = (planeHeight/2) - ((result.y - positionInWorld.y)/scale.y);

		hit = true;
	}

	dist = closest_distance;

	return hit;
}


// Get the mesh information for the given mesh.
// Code found on this forum link: http://www.ogre3d.org/wiki/index.php/RetrieveVertexData
void GetMeshInformation( const Ogre::MeshPtr mesh,
                         size_t &vertex_count,
                         Ogre::Vector3* &vertices,
                         size_t &index_count,
                         unsigned long* &indices,
                         const Ogre::Vector3 &position,
                         const Ogre::Quaternion &orient,
                         const Ogre::Vector3 &scale )
{
    bool added_shared = false;
    size_t current_offset = 0;
    size_t shared_offset = 0;
    size_t next_offset = 0;
    size_t index_offset = 0;

    vertex_count = index_count = 0;

    // Calculate how many vertices and indices we're going to need
    for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
    {
        Ogre::SubMesh* submesh = mesh->getSubMesh( i );

        // We only need to add the shared vertices once
        if(submesh->useSharedVertices)
        {
            if( !added_shared )
            {
                vertex_count += mesh->sharedVertexData->vertexCount;
                added_shared = true;
            }
        }
        else
        {
            vertex_count += submesh->vertexData->vertexCount;
        }

        // Add the indices
        index_count += submesh->indexData->indexCount;
    }


    // Allocate space for the vertices and indices
    vertices = new Ogre::Vector3[vertex_count];
    indices = new unsigned long[index_count];

    added_shared = false;

    // Run through the submeshes again, adding the data into the arrays
    for ( unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
    {
        Ogre::SubMesh* submesh = mesh->getSubMesh(i);

        Ogre::VertexData* vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData : submesh->vertexData;

        if((!submesh->useSharedVertices)||(submesh->useSharedVertices && !added_shared))
        {
            if(submesh->useSharedVertices)
            {
                added_shared = true;
                shared_offset = current_offset;
            }

            const Ogre::VertexElement* posElem =
                vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);

            Ogre::HardwareVertexBufferSharedPtr vbuf =
                vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());

            unsigned char* vertex =
                static_cast<unsigned char*>(vbuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));

            // There is _no_ baseVertexPointerToElement() which takes an Ogre::Real or a double
            //  as second argument. So make it float, to avoid trouble when Ogre::Real will
            //  be comiled/typedefed as double:
            //      Ogre::Real* pReal;
            float* pReal;

            for( size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbuf->getVertexSize())
            {
                posElem->baseVertexPointerToElement(vertex, &pReal);

                Ogre::Vector3 pt(pReal[0], pReal[1], pReal[2]);

                vertices[current_offset + j] = (orient * (pt * scale)) + position;
            }

            vbuf->unlock();
            next_offset += vertex_data->vertexCount;
        }


        Ogre::IndexData* index_data = submesh->indexData;
        size_t numTris = index_data->indexCount / 3;
        Ogre::HardwareIndexBufferSharedPtr ibuf = index_data->indexBuffer;

        bool use32bitindexes = (ibuf->getType() == Ogre::HardwareIndexBuffer::IT_32BIT);

        unsigned long*  pLong = static_cast<unsigned long*>(ibuf->lock(Ogre::HardwareBuffer::HBL_READ_ONLY));
        unsigned short* pShort = reinterpret_cast<unsigned short*>(pLong);


        size_t offset = (submesh->useSharedVertices)? shared_offset : current_offset;

        if ( use32bitindexes )
        {
            for ( size_t k = 0; k < numTris*3; ++k)
            {
                indices[index_offset++] = pLong[k] + static_cast<unsigned long>(offset);
            }
        }
        else
        {
            for ( size_t k = 0; k < numTris*3; ++k)
            {
                indices[index_offset++] = static_cast<unsigned long>(pShort[k]) +
                    static_cast<unsigned long>(offset);
            }
        }

        ibuf->unlock();
        current_offset = next_offset;
    }
}