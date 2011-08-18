/*
	This file is part of Canvas, a fast, lightweight 2D graphics engine for Ogre3D.

	Copyright (C) 2008 Adam J. Simmons
	ajs15822@gmail.com

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

#ifndef __Canvas_H__
#define __Canvas_H__

#include <vector>
#include "Atlas.h"

class Canvas;

/**
* An internal templated helper class.
*/
template<typename T> struct Corners
{
	T topLeft, bottomLeft, bottomRight, topRight;

	Corners()
	{
	}

	Corners(const T& all) : topLeft(all), bottomLeft(all), bottomRight(all), topRight(all)
	{
	}

	Corners(const T& topLeft, const T& bottomLeft, const T& bottomRight, const T& topRight) :
		topLeft(topLeft), bottomLeft(bottomLeft), bottomRight(bottomRight), topRight(topRight)
	{
	}
};

/**
* Internal structure, used to define solid fill colors/gradients.
*/
struct Coloring
{
	std::pair<Ogre::ColourValue, Ogre::ColourValue> colors;
	bool hasGradient;
	short orientation;

	enum GradientOrientation
	{
		Vertical,
		Horizontal
	};
};

/**
* Defines the "fill" (color, texture, gradient) for a shape drawn with Canvas.
*/
struct Fill
{
	bool isEmpty;
	Ogre::String atlasKey;
	Coloring coloring;

	/**
	* Create an empty fill.
	*/
	Fill();

	/**
	* Create a pure-color fill.
	*/
	Fill(const Ogre::ColourValue& color);

	/**
	* Create a gradient fill.
	*
	* @param	gradientColor1	In a vertical gradient, the top color, and in a horizontal gradient, the left color.
	* @param	gradientColor2	In a vertical gradient, the bottom color, and in a horizontal gradient, the right color.
	* @param	orientation	The orientation of the gradient, can be either Coloring::Vertical or Coloring::Horizontal.
	*/
	Fill(const Ogre::ColourValue& gradientColor1, const Ogre::ColourValue& gradientColor2, short orientation = Coloring::Vertical);

	/**
	* Create a texture fill (must be loaded in current atlas) with an optional multiplied color.
	*
	* @param	texture	The filename of the texture, should be loaded in the current atlas.
	* @param	color	The optional color to multiply the texture with.
	*/
	Fill(const Ogre::String& texture, const Ogre::ColourValue& color = Ogre::ColourValue::White);

	/**
	* Create a texture fill (must be loaded in current atlas) with a multiplied gradient.
	*
	* @param	texture	The filename of the texture, should be loaded in the current atlas.
	* @param	gradientColor1	In a vertical gradient, the top color, and in a horizontal gradient, the left color.
	* @param	gradientColor2	In a vertical gradient, the bottom color, and in a horizontal gradient, the right color.
	* @param	orientation	The orientation of the gradient, can be either Coloring::Vertical or Coloring::Horizontal.
	*/
	Fill(const Ogre::String& texture, const Ogre::ColourValue& gradientColor1, const Ogre::ColourValue& gradientColor2, short orientation = Coloring::Vertical);
};

typedef Ogre::TRect<int> WidthRect;
typedef Ogre::TRect<int> ClipRect;
typedef Ogre::TRect<int> PixelRect;
typedef Ogre::TRect<Ogre::ColourValue> ColorRect;

/**
* Defines the "border" for a rectangle drawn with Canvas.
*/
struct Border
{
	bool isEmpty;
	WidthRect widths;
	ColorRect colors;

	/**
	* Create an empty border.
	*/
	Border();

	/**
	* Create a border with a uniform pixel width and uniform color
	*
	* @param	width	The width, in pixels, for every side of the border.
	* @param	color	The color for every side of the border.
	*/
	Border(int width, const Ogre::ColourValue& color);

	/**
	* Create a border with custom pixel widths and colors for each side.
	*
	* @param	widths	The widths, in pixels, to use for each side.
	* @param	colors	The colors to use for each side.
	*/
	Border(const WidthRect& widths, const ColorRect& colors);
};

/**
* The Canvas.
*/
class Canvas : public Ogre::Renderable, public Ogre::MovableObject, public Ogre::RenderTargetListener
{
	struct Quad
	{
		Corners<Ogre::Vector2> vertices;
		Corners<Ogre::Vector2> texCoords;
		Corners<Ogre::ColourValue> colors;
	};

	Atlas* atlas;
	std::vector<Canvas::Quad> quadList;
	Ogre::HardwareVertexBufferSharedPtr buffer;
	Ogre::VertexData* vertexData;
	Ogre::IndexData* indexData;
	size_t bufferSize;
	Ogre::MaterialPtr material;
	Ogre::Viewport* viewport;
	Ogre::uint8 renderQueueID;
	ClipRect clip;
	bool isDirty;
	bool visibility;

public:
	/**
	* Constructs the canvas.
	*
	* @param	atlas	The texture atlas to use for this canvas.
	* @param	viewport	The viewport that this canvas will display in.
	*/
	Canvas(Atlas* atlas, Ogre::Viewport* viewport);

	/**
	* Destroys the canvas.
	*/
	~Canvas();

	/**
	* Draws a rectangle on the canvas.
	*
	* @param	x	The x-coordinate of the origin, in pixels.
	* @param	y	The y-coordinate of the origin, in pixels.
	* @param	width	The width of the rectangle, in pixels.
	* @param	height	The height of the rectangle, in pixels.
	* @param	fill	The fill to use.
	* @param	border	The optional border.
	*/
	void drawRectangle(int x, int y, int width, int height, const Fill& fill, const Border& border = Border());

	/**
	* Draws a glyph on the canvas.
	*
	* @param	glyph	The GlyphInfo of the glyph to draw (obtained from Atlas).
	* @param	x	The x-coordinate of the origin, in pixels.
	* @param	y	The y-coordinate of the origin, in pixels.
	* @param	width	The width of the rectangle, in pixels.
	* @param	height	The height of the rectangle, in pixels.
	* @param	color	The color of the glyph.
	*/
	void drawGlyph(const GlyphInfo& glyph, int x, int y, int width, int height, const Ogre::ColourValue& color);

	/**
	* Clears the canvas.
	*/
	void clear();

	/**
	* Sets the current clipping boundaries to use for subsequent draw calls.
	*
	* @param	left	The left-boundary, in pixels.
	* @param	top	The top-boundary, in pixels.
	* @param	right	The right-boundary, in pixels.
	* @param	bottom	The bottom-boundary, in pixels.
	*/
	void setClip(int left, int top, int right, int bottom);

	/**
	* Resets the current clipping boundaries to the dimensions of the viewport.
	*/
	void clearClip();

	// Inherited from Ogre::Renderable
	const Ogre::MaterialPtr& getMaterial() const;
	void getRenderOperation(Ogre::RenderOperation& op);
	void getWorldTransforms(Ogre::Matrix4* xform) const;
	const Ogre::Quaternion& getWorldOrientation() const;
	const Ogre::Vector3& getWorldPosition() const;
	Ogre::Real getSquaredViewDepth(const Ogre::Camera* cam) const;
	const Ogre::LightList& getLights() const;

	// Inherited from Ogre::MovableObject
	const Ogre::String& getMovableType() const;
	const Ogre::AxisAlignedBox& getBoundingBox() const;
	Ogre::Real getBoundingRadius() const;
	void _updateRenderQueue(Ogre::RenderQueue* queue);
	void setVisible(bool visible);
	bool isVisible() const;
	void visitRenderables(Ogre::Renderable::Visitor* visitor, bool debugRenderables);

	// Inherited from Ogre::RenderTargetListener
	void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
	void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
	void preViewportUpdate(const Ogre::RenderTargetViewportEvent& evt);
	void postViewportUpdate(const Ogre::RenderTargetViewportEvent& evt);
	void viewportAdded(const Ogre::RenderTargetViewportEvent& evt);
	void viewportRemoved(const Ogre::RenderTargetViewportEvent& evt);

protected:

	void destroyBuffers();

	void resizeBuffers();

	void localize(Ogre::Vector2& vertex);

	bool isOutsideClip(const PixelRect& rect);

	void drawQuad(const PixelRect& rect, const Ogre::FloatRect& texCoords, const Coloring& coloring);

	void drawQuad(const Corners<Ogre::Vector2>& corners, const Ogre::FloatRect& texCoords, const Ogre::ColourValue& color);

	void updateGeometry();
};

#endif