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

#ifndef __Navi_H__
#define __Navi_H__

#include "NaviManager.h"
#include "NaviDelegate.h"

namespace NaviLibrary
{
	/**
	* The core component of NaviLibrary, an offscreen browser window rendered to a dynamic texture (encapsulated 
	* as an Ogre Material) that can optionally be attached to an overlay and manipulated within a scene.
	*/
	class _NaviExport Navi : public Ogre::ManualResourceLoader, public Awesomium::WebViewListener
	{
	public:

		/**
		* Loads a URL into the main frame.
		*/
		void loadURL(const std::string& url);

		/**
		* Loads a local file into the main frame.
		*
		* @note	The file should reside in the base directory.
		*/
		void loadFile(const std::string& file);

		/**
		* Loads a string of HTML directly into the main frame.
		*
		* @note	Relative URLs will be resolved using the base directory.
		*/
		void loadHTML(const std::string& html);

		/**
		* Evaluates Javascript in the context of the current page.
		*
		* @param	script	The Javascript to evaluate.
		*
		* @param	args	An optional vector of JSValues that will be used in the translation
		*					of a templated string of Javascript. (see below note)
		*
		* @note
		*	If you need to pass C++ variables directly through Javascript, it is highly
		*	recommended to use the templating feature of this function with NaviUtilities::JSArgs.
		*
		*	Examples of templated JS evaluation:
		*	\code
		*	using namespace NaviLibrary::NaviUtilities;
		*	
		*	myNavi->evaluateJS("newCharacter(?, ?, ?)", JSArgs(nickname)(characterType)(level));
		*
		*	myNavi->evaluateJS("document.getElementById(?).innerHTML = ?", JSArgs("chatElement")(chatText));
		*	\endcode
		*/
		void evaluateJS(const std::string& javascript, const Awesomium::JSArguments& args = NaviUtilities::JSArgs());

		/**
		* Evaluates Javascript in the context of the current page and returns a handle to a future result.
		*
		* @param	script	The Javascript to evaluate.
		*
		* @param	args	An optional vector of JSValues that will be used in the translation
		*					of a templated string of Javascript. (see first overload of this function)
		*
		* @returns	Returns a handle to the future result, see the following note for details.
		*
		* @note	FutureJSValue is a special wrapper around a JSValue that allows asynchronous retrieval of the 
		*		actual value at a later time (using FutureJSValue::get). If you are unfamiliar with the concept 
		*		of a 'Future', please see: http://en.wikipedia.org/wiki/Futures_and_promises>.
		*/
		Awesomium::FutureJSValue evaluateJSWithResult(const std::string& javascript, const Awesomium::JSArguments& args = NaviUtilities::JSArgs());

		/**
		* Sets a global 'Client' callback that can be invoked via Javascript from
		* within all pages loaded into this Navi.
		*
		* @param	name	The name of the callback.
		* @param	callback	The C++ callback to invoke when called via Javascript.
		*
		* @note	All C++ callbacks should follow the general form of:
		*		void myCallback(Navi* caller, const Awesomium::JSArgs& args)
		*		{
		*		}
		*
		*		An example of specifying a static function named 'onItemSelected' as a callback:
		*			myNavi->setCallback("itemSelected", &onItemSelected);
		*
		*		An example of specifying a member function named 'onItemSelected' of class 'MyClass' as a callback:
		*			myNavi->setCallback("itemSelected", NaviDelegate(this, &MyClass::onItemSelected));
		*				OR
		*			myNavi->setCallback("itemSelected", NaviDelegate(myClassInstance, &MyClass::onItemSelected));
		*
		*		An example of calling a bound C++ callback from Javascript:
		*			Client.itemSelected(itemName);
		*/
		void bind(const std::string& name, const NaviDelegate& callback);

		/**
		* Sets a global 'Client' property that can be accessed via Javascript from
		* within all pages loaded into this Navi.
		*
		* @param	name	The name of the property.
		* @param	value	The javascript-value of the property.
		*
		* @note	You can access all properties you set via the 'Client' object using Javascript. For example,
		*		if you set the property with a name of 'color' and a value of 'blue', you could access
		*		this from the page using Javascript: document.write("The color is " + Client.color);
		*/
		void setProperty(const std::string& name, const Awesomium::JSValue& value);

		/**
		* Attempts to render the background of all pages loaded into this Navi as transparent.
		*
		* @param	isTransparent	Whether or not to enable transparent-background rendering.
		*
		* @note	Setting a mask will override any transparency gleaned from this render-mode.
		*/
		void setTransparent(bool isTransparent);

		/**
		* Normally, mouse movement is only injected into a specific Navi from NaviManager if the mouse is within the boundaries of
		* a Navi and over an opaque area (not transparent). This behavior may be detrimental to certain Navis, for
		* example an animated 'dock' with floating icons on a transparent background: the mouse-out event would never
		* be invoked on each icon because the Navi only received mouse movement input over opaque areas. Use this function
		* to makes this Navi always inject mouse movement, regardless of boundaries or transparency.
		*
		* @param	ignoreBounds	Whether or not this Navi should ignore bounds/transparency when injecting mouse movement.
		*
		* @note
		*	The occlusivity of each Navi will still be respected, mouse movement will not be injected via NaviManager
		*	if another Navi is occluding this Navi.
		*/
		void setIgnoreBounds(bool ignoreBounds = true);

		/**
		* Using alpha-masking/color-keying doesn't just affect the visuals of a Navi; by default, Navis 'ignore'
		* mouse movement/clicks over 'transparent' areas of a Navi (Areas with opacity less than 5%). You may
		* disable this behavior or redefine the 'transparent' threshold of opacity to something else other 
		* than 5%.
		*
		* @param	ignoreTrans		Whether or not this Navi should ignore 'transparent' areas when mouse-picking.
		*
		* @param	threshold		Areas with opacity less than this percent will be ignored
		*								(if ignoreTrans is true, of course). Default is 5% (0.05).
		*/
		void setIgnoreTransparent(bool ignoreTrans, float threshold = 0.05);

		/**
		* Masks the alpha channel of this Navi with that of a provided image.
		*
		* @param	maskFileName	The filename of the Alpha Mask Image. The Alpha Mask Image MUST have a
		*							width greater than or equal to the Navi width and it MUST have a height
		*							greater than or equal to the Navi height. Alpha Mask Images larger than
		*							the Navi will not be stretched, instead Navi will take Alpha values starting
		*							from the Top-Left corner of the Alpha Mask Image. To reset Navi to use no
		*							Alpha Mask Image, simply provide an empty String ("").
		*
		* @param	groupName		The Resource Group to find the Alpha Mask Image filename.
		*
		* @throws	Ogre::Exception::ERR_INVALIDPARAMS	Throws this if the width or height of the Alpha Mask Image is
		*												less than the width or height of the Navi it is applied to.
		*/
		void setMask(std::string maskFileName, std::string groupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

		/**
		* Adjusts the number of times per second this Navi may update.
		*
		* @param	maxUPS		The maximum number of times per second this Navi can update. Set this to '0' to 
		*						use no update limiting (default).
		*/
		void setMaxUPS(unsigned int maxUPS = 0);

		/**
		* Toggles whether or not this Navi is movable. (not applicable to NaviMaterials)
		*
		* @param	isMovable	Whether or not this Navi should be movable.
		*/
		void setMovable(bool isMovable = true);

		/**
		* Sets whether or not tooltips are enabled for this Navi (enabled by default).
		*
		* @param	isEnabled	Whether or not to enable tooltips.
		*/
		void setEnableTooltips(bool isEnabled);

		/**
		* NaviManager usually only injects keyboard events into the Navi which has an active
		* textbox or similar focused element. You can override this behavior so that this
		* Navi always receives keyboard events.
		*
		* @param	isEnabled	Whether or not this Navi always receives keyboard events.
		*/
		void setAlwaysReceivesKeyboard(bool isEnabled);

		/**
		* Gives this Navi modal focus. A Navi with modal focus will temporarily be popped to
		* the front of its tier and will consume all input (making it the sole receiver of
		* mouse and keyboard events). (not applicable to NaviMaterials)
		*
		* @param	isModal		Whether or not this Navi should have modal focus.
		*/
		void setModal(bool isModal);

		/**
		* Resets the viewport for this Navi. If the new viewport is non-null, the Navi's position
		* will be reset (see Navi::resetPosition) and will then be displayed in the new viewport.
		* (not applicable to NaviMaterials)
		*
		* @param	viewport	The new viewport that this Navi should display in. Passing a null
		*						viewport is also valid, the Navi will just be hidden.
		*/
		void setViewport(Ogre::Viewport* viewport);

		/**
		* Changes the overall opacity of this Navi to a certain percentage.
		*
		* @param	opacity		The opacity percentage as a float. 
		*						Fully Opaque = 1.0, Fully Transparent = 0.0.
		*/
		void setOpacity(float opacity);

		/** 
		* Sets the default position of this Navi to a new position and then moves
		* the Navi to that position. (not applicable to NaviMaterials)
		*
		* @param	naviPosition	The new NaviPosition to set the Navi to.
		*/
		void setPosition(const NaviPosition &naviPosition);

		/**
		* Resets the position of this Navi to its default position. (not applicable to NaviMaterials)
		*/
		void resetPosition();

		/**
		* Hides this Navi.
		*
		* @param	fade	Whether or not to fade this Navi down. (Optional, default is false)
		*
		* @param	fadeDurationMS	If fading, the number of milliseconds to fade for.
		*/
		void hide(bool fade = false, unsigned short fadeDurationMS = 300);

		/**
		* Shows this Navi.
		*
		* @param	fade	Whether or not to fade the Navi up. (Optional, default is false)
		*
		* @param	fadeDurationMS	If fading, the number of milliseconds to fade for.
		*/
		void show(bool fade = false, unsigned short fadeDurationMS = 300);

		/**
		* 'Focuses' this Navi by popping it to the front of all other Navis. (not applicable to NaviMaterials)
		*/
		void focus();

		/**
		* Moves this Navi by relative amounts. (not applicable to NaviMaterials or non-movable Navis)
		*
		* @param	deltaX	The relative X amount to move this Navi by. Positive amounts move it right.
		*
		* @param	deltaY	The relative Y amount to move this Navi by. Positive amounts move it down.
		*/
		void moveNavi(int deltaX, int deltaY);

		/**
		* Retrieves the width and height that this Navi was created with.
		*
		* @param[out]	width	The unsigned short that will be used to store the retrieved width.
		*
		* @param[out]	height	The unsigned short that will be used to store the retrieved height.
		*/
		void getExtents(unsigned short &width, unsigned short &height);

		/**
		* Transforms an X-coordinate in screen-space to that of this Navi's relative space.
		*
		* @param	absX	The X-coordinate in screen-space to transform.
		*
		* @return	The X-coordinate in this Navi's relative space.
		*/
		int getRelativeX(int absX);

		/**
		* Transforms a Y-coordinate in screen-space to that of this Navi's relative space.
		*
		* @param	absX	The Y-coordinate in screen-space to transform.
		*
		* @return	The Y-coordinate in this Navi's relative space.
		*/
		int getRelativeY(int absY);

		/**
		* Returns whether or not this Navi was created as a NaviMaterial.
		*/
		bool isMaterialOnly();

		/**
		* Returns this Navi's internal NaviOverlay (if it has one).
		*/
		NaviOverlay* getOverlay();

		/**
		* Returns the name of this Navi.
		*/
		std::string getName();

		/**
		* Returns the name of the Ogre::Material used internally by this Navi.
		*/
		std::string getMaterialName();

		/**
		* Returns whether or not this Navi is currently visible. (See Navi::hide and Navi::show)
		*/
		bool getVisibility();

		/**
		* Gets the derived UV's of this Navi's internal texture. On certain systems we must compensate for lack of
		* NPOT-support on the videocard by using the next-highest POT texture. Normal Navi's compensate their UV's accordingly
		* however NaviMaterials will need to adjust their own by use of this function.
		*
		* @param[out]	u1	The Ogre::Real that will be used to store the retrieved u1-coordinate.
		* @param[out]	v1	The Ogre::Real that will be used to store the retrieved v1-coordinate.
		* @param[out]	u2	The Ogre::Real that will be used to store the retrieved u2-coordinate.
		* @param[out]	v2	The Ogre::Real that will be used to store the retrieved v2-coordinate.
		*/
		void getDerivedUV(Ogre::Real& u1, Ogre::Real& v1, Ogre::Real& u2, Ogre::Real& v2);

		/**
		* Injects the mouse's current coordinates (in this Navi's own local coordinate space, see Navi::getRelativeX and 
		* Navi::getRelativeY) into this Navi.
		*
		* @param	xPos	The X-coordinate of the mouse, relative to this Navi's origin.
		* @param	yPos	The Y-coordinate of the mouse, relative to this Navi's origin.
		*/
		void injectMouseMove(int xPos, int yPos);

		/**
		* Injects mouse wheel events into this Navi.
		*
		* @param	relScroll	The relative Scroll-Value of the mouse.
		*
		* @note
		*	To inject this using OIS: on a OIS::MouseListener::MouseMoved event, simply 
		*	inject "arg.state.Z.rel" of the "MouseEvent".
		*/
		void injectMouseWheel(int relScroll);

		/**
		* Injects mouse down events into this Navi. You must supply the current coordinates of the mouse in this
		* Navi's own local coordinate space. (see Navi::getRelativeX and Navi::getRelativeY)
		*
		* @param	xPos	The absolute X-Value of the mouse, relative to this Navi's origin.
		* @param	yPos	The absolute Y-Value of the mouse, relative to this Navi's origin.
		*/
		void injectMouseDown(int xPos, int yPos);

		/**
		* Injects mouse up events into this Navi. You must supply the current coordinates of the mouse in this
		* Navi's own local coordinate space. (see Navi::getRelativeX and Navi::getRelativeY)
		*
		* @param	xPos	The absolute X-Value of the mouse, relative to this Navi's origin.
		* @param	yPos	The absolute Y-Value of the mouse, relative to this Navi's origin.
		*/
		void injectMouseUp(int xPos, int yPos);

		/**
		* Saves a capture of this Navi to an image.
		*/
		void captureImage(const std::string& filename);

		/**
		* Resizes this Navi to new dimensions.
		*
		* @param	width	The new width.
		* @param	height	The new height.
		*
		* @note	There is currently an issue with calling Navi::resize from a JS callback and so, as a workaround, 
		*		the actual resizing is deferred until the next call to NaviManager::Update
		*/
		void resize(int width, int height);

		/**
		 * Zooms the page a specified percent.
		 *
		 * @param	zoomPercent	The percent of the page to zoom to. Valid range
		 *                      is from 10% to 500%.
		 */
		void setZoom(int percent);

		/**
		* Resets the zoom.
		*/
		void resetZoom();

	protected:
		Awesomium::WebView* webView;
		std::string naviName;
		unsigned short naviWidth;
		unsigned short naviHeight;
		NaviOverlay* overlay;
		bool movable;
		unsigned int maxUpdatePS;
		Ogre::Timer timer;
		unsigned long lastUpdateTime;
		float opacity;
		bool usingMask;
		unsigned char* alphaCache;
		size_t alphaCachePitch;
		Ogre::Pass* matPass;
		Ogre::TextureUnitState* baseTexUnit;
		Ogre::TextureUnitState* maskTexUnit;
		bool ignoringTrans;
		float transparent;
		bool isWebViewTransparent;
		bool ignoringBounds;
		bool okayToDelete;
		double fadeValue;
		bool isFading;
		double deltaFadePerMS;
		double lastFadeTimeMS;
		bool compensateNPOT;
		unsigned short texWidth;
		unsigned short texHeight;
		size_t texDepth;
		size_t texPitch;
		std::map<std::string, NaviDelegate> delegateMap;
		Ogre::FilterOptions texFiltering;
		std::pair<std::string, std::string> maskImageParameters;
		bool tooltipsEnabled, needsForceRender, alwaysReceivesKeyboard;
		bool hasInternalKeyboardFocus;
		std::pair<int, int> resizeParameters;

		friend class NaviManager;

		Navi(const std::string& name, unsigned short width, unsigned short height, const NaviPosition &naviPosition,
			bool asyncRender, int maxAsyncRenderRate, Ogre::uchar zOrder, Tier tier, Ogre::Viewport* viewport);

		Navi(const std::string& name, unsigned short width, unsigned short height, 
			bool asyncRender, int maxAsyncRenderRate, Ogre::FilterOptions texFiltering);

		~Navi();

		void createWebView(bool asyncRender, int maxAsyncRenderRate);

		void createMaterial();

		void loadResource(Ogre::Resource* resource);

		void update();

		void updateFade();

		void resizeIfNeeded();

		bool isPointOverMe(int x, int y);


		virtual void onBeginNavigation(Awesomium::WebView* caller, 
									   const std::string& url, 
									   const std::wstring& frameName);
		
		virtual void onBeginLoading(Awesomium::WebView* caller, 
									const std::string& url, 
									const std::wstring& frameName, 
									int statusCode, const std::wstring& mimeType);
		
		virtual void onFinishLoading(Awesomium::WebView* caller);
		
		virtual void onCallback(Awesomium::WebView* caller, 
								const std::wstring& objectName, 
								const std::wstring& callbackName, 
								const Awesomium::JSArguments& args);
		
		virtual void onReceiveTitle(Awesomium::WebView* caller, 
									const std::wstring& title, 
									const std::wstring& frameName);
		
		virtual void onChangeTooltip(Awesomium::WebView* caller, 
									 const std::wstring& tooltip);
		
		virtual void onChangeCursor(Awesomium::WebView* caller, 
									Awesomium::CursorType cursor);

		virtual void onChangeKeyboardFocus(Awesomium::WebView* caller,
										   bool isFocused);
		
		virtual void onChangeTargetURL(Awesomium::WebView* caller, 
									   const std::string& url);
		
		virtual void onOpenExternalLink(Awesomium::WebView* caller, 
										const std::string& url, 
										const std::wstring& source);

		virtual void onRequestDownload(Awesomium::WebView* caller,
									   const std::string& url);
		
		virtual void onWebViewCrashed(Awesomium::WebView* caller);
		
		virtual void onPluginCrashed(Awesomium::WebView* caller, 
									 const std::wstring& pluginName);
		
		virtual void onCreateWindow(Awesomium::WebView* caller, 
									Awesomium::WebView* createdWindow, 
									int width, int height);
		
		virtual void onRequestMove(Awesomium::WebView* caller, int x, int y);
		
		virtual void onGetPageContents(Awesomium::WebView* caller, 
									   const std::string& url, 
									   const std::wstring& contents);
		
		virtual void onDOMReady(Awesomium::WebView* caller);

		virtual void onRequestFileChooser(Awesomium::WebView* caller,
										  bool selectMultipleFiles,
										  const std::wstring& title,
										  const std::wstring& defaultPath);

		virtual void onGetScrollData(Awesomium::WebView* caller,
									 int contentWidth,
									 int contentHeight,
									 int preferredWidth,
									 int scrollX,
									 int scrollY);
		
		virtual void onJavascriptConsoleMessage(Awesomium::WebView* caller,
												const std::wstring& message,
												int lineNumber,
												const std::wstring& source);

		virtual void onGetFindResults(Awesomium::WebView* caller,
                                      int requestID,
                                      int numMatches,
                                      const Awesomium::Rect& selection,
                                      int curMatch,
                                      bool finalUpdate);


		virtual void onUpdateIME(Awesomium::WebView* caller,
                                 Awesomium::IMEState imeState,
                                 const Awesomium::Rect& caretRect);

		void onRequestDrag(Navi* caller, const Awesomium::JSArguments& args);
	};
}

#endif