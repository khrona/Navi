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

#include "Navi.h"
#include "NaviUtilities.h"
#include <OGRE/OgreBitwise.h>

using namespace Ogre;
using namespace NaviLibrary;
using namespace NaviLibrary::NaviUtilities;

Navi::Navi(const std::string& name, unsigned short width, unsigned short height, const NaviPosition &naviPosition,
			bool asyncRender, int maxAsyncRenderRate, Ogre::uchar zOrder, Tier tier, Ogre::Viewport* viewport)
{
	webView = 0;
	naviName = name;
	naviWidth = width;
	naviHeight = height;
	overlay = 0;
	movable = true;
	maxUpdatePS = 0;
	lastUpdateTime = 0;
	opacity = 1;
	usingMask = false;
	ignoringTrans = true;
	transparent = 0.05f;
	isWebViewTransparent = false;
	ignoringBounds = false;
	okayToDelete = false;
	compensateNPOT = false;
	texWidth = width;
	texHeight = height;
	alphaCache = 0;
	alphaCachePitch = 0;
	matPass = 0;
	baseTexUnit = 0;
	maskTexUnit = 0;
	fadeValue = 1;
	isFading = false;
	deltaFadePerMS = 0;
	lastFadeTimeMS = 0;
	texFiltering = Ogre::FO_NONE;
	tooltipsEnabled = true;
	needsForceRender = false;
	alwaysReceivesKeyboard = false;
	hasInternalKeyboardFocus = false;
	resizeParameters = std::pair<int, int>(0, 0);

	createMaterial();
	
	overlay = new NaviOverlay(name + "_overlay", viewport, width, height, naviPosition, getMaterialName(), zOrder, tier);

	if(compensateNPOT)
		overlay->panel->setUV(0, 0, (Real)naviWidth/(Real)texWidth, (Real)naviHeight/(Real)texHeight);	

	createWebView(asyncRender, maxAsyncRenderRate);
}

Navi::Navi(const std::string& name, unsigned short width, unsigned short height, 
			bool asyncRender, int maxAsyncRenderRate, Ogre::FilterOptions texFiltering)
{
	webView = 0;
	naviName = name;
	naviWidth = width;
	naviHeight = height;
	overlay = 0;
	movable = false;
	maxUpdatePS = 0;
	lastUpdateTime = 0;
	opacity = 1;
	usingMask = false;
	ignoringTrans = true;
	transparent = 0.05f;
	isWebViewTransparent = false;
	ignoringBounds = false;
	okayToDelete = false;
	compensateNPOT = false;
	texWidth = width;
	texHeight = height;
	alphaCache = 0;
	alphaCachePitch = 0;
	matPass = 0;
	baseTexUnit = 0;
	maskTexUnit = 0;
	fadeValue = 1;
	isFading = false;
	deltaFadePerMS = 0;
	lastFadeTimeMS = 0;
	this->texFiltering = texFiltering;
	tooltipsEnabled = true;
	needsForceRender = false;
	alwaysReceivesKeyboard = false;
	hasInternalKeyboardFocus = false;
	resizeParameters = std::pair<int, int>(0, 0);

	createMaterial();
	createWebView(asyncRender, maxAsyncRenderRate);	
}

Navi::~Navi()
{
	if(alphaCache)
		delete[] alphaCache;

	if(webView)
		awe_webview_destroy(webView);

	if(overlay)
		delete overlay;

	MaterialManager::getSingletonPtr()->remove(naviName + "Material");
	TextureManager::getSingletonPtr()->remove(naviName + "Texture");
	if(usingMask) TextureManager::getSingletonPtr()->remove(naviName + "MaskTexture");
}

void Navi::createWebView(bool asyncRender, int maxAsyncRenderRate)
{
	webView = awe_webcore_create_webview(naviWidth, naviHeight, false);
	OSM::WebViewEventHelper::instance().addListener(webView, this);
	
	awe_webview_create_object(webView, OSM_STR("Client"));

	bind("drag", NaviDelegate(this, &Navi::onRequestDrag));
}

void Navi::createMaterial()
{
	limit<float>(opacity, 0, 1);

	if(!Bitwise::isPO2(naviWidth) || !Bitwise::isPO2(naviHeight))
	{
		if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_NON_POWER_OF_2_TEXTURES))
		{
			if(Root::getSingleton().getRenderSystem()->getCapabilities()->getNonPOW2TexturesLimited())
				compensateNPOT = true;
		}
		else compensateNPOT = true;

		if(compensateNPOT)
		{
			texWidth = Bitwise::firstPO2From(naviWidth);
			texHeight = Bitwise::firstPO2From(naviHeight);
		}
	}

	// Create the texture
	TexturePtr texture = TextureManager::getSingleton().createManual(
		naviName + "Texture", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		TEX_TYPE_2D, texWidth, texHeight, 0, PF_BYTE_BGRA,
		TU_DYNAMIC_WRITE_ONLY_DISCARDABLE, this);

	HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
	pixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
	const PixelBox& pixelBox = pixelBuffer->getCurrentLock();
	texDepth = Ogre::PixelUtil::getNumElemBytes(pixelBox.format);
	texPitch = (pixelBox.rowPitch*texDepth);

	uint8* pDest = static_cast<uint8*>(pixelBox.data);

	memset(pDest, 128, texHeight*texPitch);

	pixelBuffer->unlock();

	MaterialPtr material = MaterialManager::getSingleton().create(naviName + "Material", 
		ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	matPass = material->getTechnique(0)->getPass(0);
	matPass->setSceneBlending(SBT_TRANSPARENT_ALPHA);
	matPass->setDepthWriteEnabled(false);

	baseTexUnit = matPass->createTextureUnitState(naviName + "Texture");
	
	baseTexUnit->setTextureFiltering(texFiltering, texFiltering, FO_NONE);
	if(texFiltering == FO_ANISOTROPIC)
		baseTexUnit->setTextureAnisotropy(4);
}

// This is for when the rendering device has a hiccup and loses the dynamic texture
void Navi::loadResource(Resource* resource)
{
	Texture *tex = static_cast<Texture*>(resource); 

	tex->setTextureType(TEX_TYPE_2D);
	tex->setWidth(texWidth);
	tex->setHeight(texHeight);
	tex->setNumMipmaps(0);
	tex->setFormat(PF_BYTE_BGRA);
	tex->setUsage(TU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
	tex->createInternalResources();

	needsForceRender = true;

	if(overlay)
		resetPosition();
}

void Navi::update()
{
	if(!webView)
		return;

	resizeIfNeeded();

	if(maxUpdatePS)
		if(timer.getMilliseconds() - lastUpdateTime < 1000 / maxUpdatePS)
			return;

	updateFade();

	if(usingMask)
		baseTexUnit->setAlphaOperation(LBX_SOURCE1, LBS_MANUAL, LBS_CURRENT, static_cast<Ogre::Real>(fadeValue * opacity));
	else if(isWebViewTransparent)
		baseTexUnit->setAlphaOperation(LBX_BLEND_TEXTURE_ALPHA, LBS_MANUAL, LBS_TEXTURE, static_cast<Ogre::Real>(fadeValue * opacity));
	else
		baseTexUnit->setAlphaOperation(LBX_SOURCE1, LBS_MANUAL, LBS_CURRENT, static_cast<Ogre::Real>(fadeValue * opacity));

	if(!needsForceRender)
		if(!awe_webview_is_dirty(webView))
			return;

	TexturePtr texture = TextureManager::getSingleton().getByName(naviName + "Texture");
	
	HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
	pixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
	const PixelBox& pixelBox = pixelBuffer->getCurrentLock();

	uint8* destBuffer = static_cast<uint8*>(pixelBox.data);

	const awe_renderbuffer* renderBuffer = awe_webview_render(webView);

	awe_renderbuffer_copy_to(renderBuffer, destBuffer, texPitch, texDepth, false, false);

	if(isWebViewTransparent && !usingMask && ignoringTrans)
	{
		for(int row = 0; row < texHeight; row++)
			for(int col = 0; col < texWidth; col++)
				alphaCache[row * alphaCachePitch + col] = destBuffer[row * texPitch + col * 4 + 3];
	}

	pixelBuffer->unlock();

	lastUpdateTime = timer.getMilliseconds();
	needsForceRender = false;
}

void Navi::updateFade()
{
	if(isFading)
	{
		fadeValue += deltaFadePerMS * (timer.getMilliseconds() - lastFadeTimeMS);

		if(fadeValue > 1)
		{
			fadeValue = 1;
			isFading = false;
		}
		else if(fadeValue < 0)
		{
			fadeValue = 0;
			isFading = false;
			overlay->hide();
		}

		lastFadeTimeMS = timer.getMilliseconds();
	}
}

void Navi::resizeIfNeeded()
{
	if(!webView)
		return;

	if(!resizeParameters.first)
		return;

	int width = resizeParameters.first;
	int height = resizeParameters.second;

	resizeParameters.first = 0;
	resizeParameters.second = 0;

	if(width == naviWidth && height == naviHeight)
		return;

	naviWidth = width;
	naviHeight = height;

	int newTexWidth = naviWidth;
	int newTexHeight = naviHeight;

	if(!Bitwise::isPO2(naviWidth) || !Bitwise::isPO2(naviHeight))
	{
		if(Root::getSingleton().getRenderSystem()->getCapabilities()->hasCapability(RSC_NON_POWER_OF_2_TEXTURES))
		{
			if(Root::getSingleton().getRenderSystem()->getCapabilities()->getNonPOW2TexturesLimited())
				compensateNPOT = true;
		}
		else compensateNPOT = true;
		
		if(compensateNPOT)
		{
			newTexWidth = Bitwise::firstPO2From(naviWidth);
			newTexHeight = Bitwise::firstPO2From(naviHeight);
		}
	}

	if(overlay)
	{
		overlay->resize(naviWidth, naviHeight);
		overlay->panel->setUV(0, 0, (Real)naviWidth/newTexWidth, (Real)naviHeight/newTexHeight);	
	}

	awe_webview_resize(webView, naviWidth, naviHeight, false, 0);

	if(newTexWidth == texWidth && newTexHeight == texHeight)
		return;

	texWidth = newTexWidth;
	texHeight = newTexHeight;

	matPass->removeAllTextureUnitStates();
	maskTexUnit = 0;

	Ogre::TextureManager::getSingleton().remove(naviName + "Texture");

	TexturePtr texture = TextureManager::getSingleton().createManual(
		naviName + "Texture", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		TEX_TYPE_2D, texWidth, texHeight, 0, PF_BYTE_BGRA,
		TU_DYNAMIC_WRITE_ONLY_DISCARDABLE, this);

	HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
	pixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
	const PixelBox& pixelBox = pixelBuffer->getCurrentLock();
	texDepth = Ogre::PixelUtil::getNumElemBytes(pixelBox.format);
	texPitch = (pixelBox.rowPitch*texDepth);

	uint8* pDest = static_cast<uint8*>(pixelBox.data);

	memset(pDest, 128, texHeight*texPitch);

	pixelBuffer->unlock();

	baseTexUnit = matPass->createTextureUnitState(naviName + "Texture");
	
	baseTexUnit->setTextureFiltering(texFiltering, texFiltering, FO_NONE);
	if(texFiltering == FO_ANISOTROPIC)
		baseTexUnit->setTextureAnisotropy(4);

	if(usingMask)
	{
		setMask(maskImageParameters.first, maskImageParameters.second);
	}
	else if(alphaCache)
	{
		delete[] alphaCache;
		alphaCache = new unsigned char[texWidth * texHeight];
		alphaCachePitch = texWidth;
	}
}

bool Navi::isPointOverMe(int x, int y)
{
	if(isMaterialOnly())
		return false;
	if(!overlay->getVisibility())
		return false;

	if(overlay->isWithinBounds(x, y))
	{
		int localX = overlay->getRelativeX(x);
		int localY = overlay->getRelativeY(y);

		return !ignoringTrans || !alphaCache ? true : 
			alphaCache[localY * alphaCachePitch + localX] > 255 * transparent;
	}		

	return false;
}

void Navi::loadURL(const std::string& url)
{
	if(webView)
		awe_webview_load_url(webView, OSM_STR(url), OSM_EMPTY(),
		OSM_EMPTY(), OSM_EMPTY());
}

void Navi::loadFile(const std::string& file)
{
	if(webView)
		awe_webview_load_file(webView, OSM_STR(file), OSM_EMPTY());
}

void Navi::loadHTML(const std::string& html)
{
	if(webView)
		awe_webview_load_html(webView, OSM_STR(html), OSM_EMPTY());
}

void Navi::evaluateJS(const std::string& javascript, const OSM::JSArguments& args)
{
	if(!webView)
		return;

	if(!args.size())
	{
		awe_webview_execute_javascript(webView, OSM_STR(javascript), OSM_EMPTY());
		return;
	}

	std::string resultScript;
	char paramName[15];
	unsigned int i, count;

	for(i = 0, count = 0; i < javascript.length(); i++)
	{
		if(javascript[i] == '?')
		{
			count++;
			if(count <= args.size())
			{
				sprintf(paramName, "__p00%d", count - 1);
				setProperty(paramName, args[count-1]);
				resultScript += "Client.";
				resultScript += paramName;
			}
			else
			{
				resultScript += "undefined";
			}
		}
		else
		{
			resultScript.push_back(javascript[i]);
		}
	}

	awe_webview_execute_javascript(webView, OSM_STR(resultScript), OSM_EMPTY());
}

OSM::JSValue Navi::evaluateJSWithResult(const std::string& javascript, const OSM::JSArguments& args)
{
	if(!webView)
		return OSM::JSValue();

	if(!args.size())
	{
		awe_jsvalue* result = awe_webview_execute_javascript_with_result(webView, OSM::String(javascript).getInstance(), 
			awe_string_empty(), 900);
		return OSM::JSValue(result, true);
	}
	
	std::string resultScript;
	char paramName[15];
	unsigned int i, count;

	for(i = 0, count = 0; i < javascript.length(); i++)
	{
		if(javascript[i] == '?')
		{
			count++;
			if(count <= args.size())
			{
				sprintf(paramName, "__p00%d", count - 1);
				setProperty(paramName, args[count-1]);
				resultScript += "Client.";
				resultScript += paramName;
			}
			else
			{
				resultScript += "undefined";
			}
		}
		else
		{
			resultScript.push_back(javascript[i]);
		}
	}

	awe_jsvalue* result = awe_webview_execute_javascript_with_result(webView, 
		OSM::String(resultScript).getInstance(), 
		awe_string_empty(), 900);

	return OSM::JSValue(result, true);
}

void Navi::bind(const std::string& name, const NaviDelegate& callback)
{
	if(!webView)
		return;

	delegateMap[name] = callback;

	awe_webview_set_object_callback(webView, OSM_STR("Client"), OSM_STR(name));
}

void Navi::setProperty(const std::string& name, const OSM::JSValue& value)
{
	if(!webView)
		return;

	awe_webview_set_object_property(webView, OSM_STR("Client"), OSM_STR(name), 
		(const awe_jsvalue*)(value.getInstance()));
}

void Navi::setTransparent(bool isTransparent)
{
	if(!webView)
		return;

	if(!isTransparent)
	{
		if(alphaCache && !usingMask)
		{
			delete[] alphaCache;
			alphaCache = 0;
		}
	}
	else
	{
		if(!alphaCache && !usingMask)
		{
			alphaCache = new unsigned char[texWidth * texHeight];
			alphaCachePitch = texWidth;
		}
	}

	awe_webview_set_transparent(webView, isTransparent);
	isWebViewTransparent = isTransparent;
}

void Navi::setIgnoreBounds(bool ignoreBounds)
{
	ignoringBounds = ignoreBounds;
}

void Navi::setIgnoreTransparent(bool ignoreTrans, float threshold)
{
	ignoringTrans = ignoreTrans;

	limit<float>(threshold, 0, 1);

	transparent = threshold;
}

void Navi::setMask(std::string maskFileName, std::string groupName)
{
	if(usingMask)
	{
		if(maskTexUnit)
		{
			matPass->removeTextureUnitState(1);
			maskTexUnit = 0;
		}

		if(!TextureManager::getSingleton().getByName(naviName + "MaskTexture").isNull())
			TextureManager::getSingleton().remove(naviName + "MaskTexture");
	}

	if(alphaCache)
	{
		delete[] alphaCache;
		alphaCache = 0;
	}

	if(maskFileName == "")
	{
		usingMask = false;
		maskImageParameters.first = "";
		maskImageParameters.second = "";

		if(isWebViewTransparent)
		{
			setTransparent(true);
			update();
		}

		return;
	}

	maskImageParameters.first = maskFileName;
	maskImageParameters.second = groupName;

	if(!maskTexUnit)
	{
		maskTexUnit = matPass->createTextureUnitState();
		maskTexUnit->setIsAlpha(true);
		maskTexUnit->setTextureFiltering(FO_NONE, FO_NONE, FO_NONE);
		maskTexUnit->setColourOperationEx(LBX_SOURCE1, LBS_CURRENT, LBS_CURRENT);
		maskTexUnit->setAlphaOperation(LBX_MODULATE);
	}
	
	Image srcImage;
	srcImage.load(maskFileName, groupName);

	Ogre::PixelBox srcPixels = srcImage.getPixelBox();
	unsigned char* conversionBuf = 0;
	
	if(srcImage.getFormat() != Ogre::PF_BYTE_A)
	{
		size_t dstBpp = Ogre::PixelUtil::getNumElemBytes(Ogre::PF_BYTE_A);
		conversionBuf = new unsigned char[srcImage.getWidth() * srcImage.getHeight() * dstBpp];
		Ogre::PixelBox convPixels(Ogre::Box(0, 0, srcImage.getWidth(), srcImage.getHeight()), Ogre::PF_BYTE_A, conversionBuf);
		Ogre::PixelUtil::bulkPixelConversion(srcImage.getPixelBox(), convPixels);
		srcPixels = convPixels;
	}

	TexturePtr maskTexture = TextureManager::getSingleton().createManual(
		naviName + "MaskTexture", ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		TEX_TYPE_2D, texWidth, texHeight, 0, PF_BYTE_A, TU_STATIC_WRITE_ONLY);

	HardwarePixelBufferSharedPtr pixelBuffer = maskTexture->getBuffer();
	pixelBuffer->lock(HardwareBuffer::HBL_DISCARD);
	const PixelBox& pixelBox = pixelBuffer->getCurrentLock();
	size_t maskTexDepth = Ogre::PixelUtil::getNumElemBytes(pixelBox.format);
	alphaCachePitch = pixelBox.rowPitch;

	alphaCache = new unsigned char[alphaCachePitch*texHeight];

	uint8* buffer = static_cast<uint8*>(pixelBox.data);

	memset(buffer, 0, alphaCachePitch * texHeight);

	size_t minRowSpan = std::min(alphaCachePitch, srcPixels.rowPitch);
	size_t minHeight = std::min(texHeight, (unsigned short)srcPixels.getHeight());

	if(maskTexDepth == 1)
	{
		for(unsigned int row = 0; row < minHeight; row++)
			memcpy(buffer + row * alphaCachePitch, (unsigned char*)srcPixels.data + row * srcPixels.rowPitch, minRowSpan);

		memcpy(alphaCache, buffer, alphaCachePitch*texHeight);
	}
	else if(maskTexDepth == 4)
	{
		size_t destRowOffset, srcRowOffset, cacheRowOffset;

		for(unsigned int row = 0; row < minHeight; row++)
		{
			destRowOffset = row * alphaCachePitch * maskTexDepth;
			srcRowOffset = row * srcPixels.rowPitch;
			cacheRowOffset = row * alphaCachePitch;

			for(unsigned int col = 0; col < minRowSpan; col++)
				alphaCache[cacheRowOffset + col] = buffer[destRowOffset + col * maskTexDepth + 3] = ((unsigned char*)srcPixels.data)[srcRowOffset + col];
		}
	}
	else
	{
		OGRE_EXCEPT(Ogre::Exception::ERR_RT_ASSERTION_FAILED, 
			"Unexpected depth and format were encountered while creating a PF_BYTE_A HardwarePixelBuffer. Pixel format: " + 
			StringConverter::toString(pixelBox.format) + ", Depth:" + StringConverter::toString(maskTexDepth), "Navi::setMask");
	}

	pixelBuffer->unlock();

	if(conversionBuf)
		delete[] conversionBuf;

	maskTexUnit->setTextureName(naviName + "MaskTexture");
	usingMask = true;
}

void Navi::setMaxUPS(unsigned int maxUPS)
{
	maxUpdatePS = maxUPS;
}

void Navi::setMovable(bool isMovable)
{
	if(!isMaterialOnly())
		movable = isMovable;
}

void Navi::setEnableTooltips(bool isEnabled)
{
	tooltipsEnabled = isEnabled;

	if(!isEnabled)
		NaviManager::Get().handleTooltip(this, L"");
}

void Navi::setAlwaysReceivesKeyboard(bool isEnabled)
{
	alwaysReceivesKeyboard = isEnabled;
}

void Navi::setModal(bool isModal)
{
	if(overlay)
		NaviManager::Get().setNaviModality(this, isModal);
}

void Navi::setViewport(Ogre::Viewport* viewport)
{
	if(overlay)
		overlay->setViewport(viewport);
}

void Navi::setOpacity(float opacity)
{
	limit<float>(opacity, 0, 1);
	
	this->opacity = opacity;
}

void Navi::setPosition(const NaviPosition &naviPosition)
{
	if(overlay)
		overlay->setPosition(naviPosition);
}

void Navi::resetPosition()
{
	if(overlay)
		overlay->resetPosition();
}

void Navi::hide(bool fade, unsigned short fadeDurationMS)
{
	updateFade();

	NaviManager::Get().handleNaviHide(this);

	if(fade)
	{
		isFading = true;
		deltaFadePerMS = -1 / (double)fadeDurationMS;
		lastFadeTimeMS = timer.getMilliseconds();
	}
	else
	{
		isFading = false;
		fadeValue = 0;
		overlay->hide();
	}
}

void Navi::show(bool fade, unsigned short fadeDurationMS)
{
	updateFade();

	if(fade)
	{
		isFading = true;
		deltaFadePerMS = 1 / (double)fadeDurationMS;
		lastFadeTimeMS = timer.getMilliseconds();
	}
	else
	{
		isFading = false;
		fadeValue = 1;
	}

	overlay->show();
}

void Navi::focus()
{
	if(!webView)
		return;

	if(overlay)
		NaviManager::GetPointer()->focusNavi(0, 0, this);
	else
		awe_webview_focus(webView);
}

void Navi::moveNavi(int deltaX, int deltaY)
{
	if(overlay)
		overlay->move(deltaX, deltaY);
}

void Navi::getExtents(unsigned short &width, unsigned short &height)
{
	width = naviWidth;
	height = naviHeight;
}

int Navi::getRelativeX(int absX)
{
	if(isMaterialOnly())
		return 0;
	else
		return overlay->getRelativeX(absX);
}

int Navi::getRelativeY(int absY)
{
	if(isMaterialOnly())
		return 0;
	else
		return overlay->getRelativeY(absY);
}

bool Navi::isMaterialOnly()
{
	return !overlay;
}

NaviOverlay* Navi::getOverlay()
{
	return overlay;
}

std::string Navi::getName()
{
	return naviName;
}

std::string Navi::getMaterialName()
{
	return naviName + "Material";
}

bool Navi::getVisibility()
{
	if(isMaterialOnly())
		return fadeValue != 0;
	else
		return overlay->getVisibility();
}

void Navi::getDerivedUV(Ogre::Real& u1, Ogre::Real& v1, Ogre::Real& u2, Ogre::Real& v2)
{
	u1 = v1 = 0;
	u2 = v2 = 1;

	if(compensateNPOT)
	{
		u2 = (Ogre::Real)naviWidth/texWidth;
		v2 = (Ogre::Real)naviHeight/(Ogre::Real)texHeight;
	}
}

void Navi::injectMouseMove(int xPos, int yPos)
{
	if(webView)
		awe_webview_inject_mouse_move(webView, xPos, yPos);
}

void Navi::injectMouseWheel(int relScroll)
{
	if(webView)
		awe_webview_inject_mouse_wheel(webView, relScroll, 0);
}

void Navi::injectMouseDown(int xPos, int yPos)
{
	if(hasInternalKeyboardFocus)
		NaviManager::Get().handleKeyboardFocusChange(this, true);

	if(webView)
		awe_webview_inject_mouse_down(webView, AWE_MB_LEFT);
}

void Navi::injectMouseUp(int xPos, int yPos)
{
	if(webView)
		awe_webview_inject_mouse_up(webView, AWE_MB_LEFT);
}

void Navi::captureImage(const std::string& filename)
{
	if(webView)
	{
		const awe_renderbuffer* buffer = awe_webview_render(webView);
		if(buffer)
			awe_renderbuffer_save_to_jpeg(buffer, OSM_STR(filename), 90);
	}
}

void Navi::resize(int width, int height)
{
	resizeParameters.first = width;
	resizeParameters.second = height;
}

void Navi::setZoom(int percent)
{
	if(webView)
		awe_webview_set_zoom(webView, percent);
}

void Navi::resetZoom()
{
	if(webView)
		awe_webview_reset_zoom(webView);
}

void Navi::onBeginNavigation(awe_webview* caller, 
								   const OSM::String& url, 
								   const OSM::String& frameName)
{
	onJSCallback(caller, L"Client", L"_beginNavigation", JSArgs(url, frameName));
}

void Navi::onBeginLoading(awe_webview* caller, 
									const OSM::String& url, 
									const OSM::String& frameName, 
									int statusCode, 
									const OSM::String& mimeType)
{
	onJSCallback(caller, L"Client", L"_beginLoading", JSArgs(url, frameName, statusCode, mimeType));
}

void Navi::onFinishLoading(awe_webview* caller)
{
	onJSCallback(caller, L"Client", L"_finishLoading", JSArgs());
}

void Navi::onJSCallback(awe_webview* caller, 
								const OSM::String& objectName, 
								const OSM::String& callbackName, 
								const OSM::JSArguments& args)
{
	if(objectName.str() == "Client")
	{
		std::string name = callbackName.str();

		std::map<std::string, NaviDelegate>::iterator i = delegateMap.find(name);

		if(i != delegateMap.end())
			NaviManager::Get().queueCallback(this, args, i->second);
	}
}

void Navi::onReceiveTitle(awe_webview* caller, 
									const OSM::String& title, 
									const OSM::String& frameName)
{
	onJSCallback(caller, L"Client", L"_receiveTitle", JSArgs(title, frameName));
}

void Navi::onChangeTooltip(awe_webview* caller, 
									 const OSM::String& tooltip)
{
	if(tooltipsEnabled)
		NaviManager::Get().handleTooltip(this, tooltip.wstr());
}

void Navi::onChangeCursor(awe_webview* caller, 
									awe_cursor_type cursor)
{
}

void Navi::onChangeKeyboardFocus(awe_webview* caller, 
										   bool isFocused)
{
	NaviManager::Get().handleKeyboardFocusChange(this, isFocused);
	hasInternalKeyboardFocus = isFocused;
	onJSCallback(caller, L"Client", L"_changeKeyboardFocus", JSArgs(isFocused));
}

void Navi::onChangeTargetURL(awe_webview* caller, 
									   const OSM::String& url)
{
	onJSCallback(caller, L"Client", L"_changeTargetURL", JSArgs(url));
}

void Navi::onOpenExternalLink(awe_webview* caller, 
										const OSM::String& url, 
										const OSM::String& source)
{
	onJSCallback(caller, L"Client", L"_openExternalLink", JSArgs(url, source));
}

void Navi::onRequestDownload(awe_webview* caller,
										const OSM::String& url)
{
	onJSCallback(caller, L"Client", L"_requestDownload", JSArgs(url));
}

void Navi::onWebViewCrashed(awe_webview* caller)
{
	onJSCallback(caller, L"Client", L"_webViewCrashed", JSArgs());
}

void Navi::onPluginCrashed(awe_webview* caller, 
									 const OSM::String& pluginName)
{
}

void Navi::onRequestMove(awe_webview* caller, 
								   int x, int y)
{
}

void Navi::onGetPageContents(awe_webview* caller, 
									   const OSM::String& url, 
									   const OSM::String& contents)
{
}

void Navi::onDOMReady(awe_webview* caller)
{
	onJSCallback(caller, L"Client", L"_DOMReady", JSArgs());
}

void Navi::onRequestFileChooser(awe_webview* caller,
										  bool selectMultipleFiles,
										  const OSM::String& title,
										  const OSM::String& defaultPath)
{
}

void Navi::onGetScrollData(awe_webview* caller,
									 int contentWidth,
									 int contentHeight,
									 int preferredWidth,
									 int scrollX,
									 int scrollY)
{
}

void Navi::onJSConsoleMessage(awe_webview* caller,
										const OSM::String& message,
										int lineNumber,
										const OSM::String& source)
{
}

void Navi::onGetFindResults(awe_webview* caller,
									  int requestID,
									  int numMatches,
									  awe_rect selection,
									  int curMatch,
									  bool finalUpdate)
{
}


void Navi::onUpdateIME(awe_webview* caller,
								 awe_ime_state imeState,
								 awe_rect caretRect)
{
}

void Navi::onRequestDrag(Navi *caller, const OSM::JSArguments &args)
{
	if(overlay)
		NaviManager::Get().handleRequestDrag(this);
}