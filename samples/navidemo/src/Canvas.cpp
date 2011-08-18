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

#include "Canvas.h"

using namespace Ogre;

/************************
* Fill
************************/

Fill::Fill() : isEmpty(true)
{
}

Fill::Fill(const Ogre::ColourValue& color) : isEmpty(false), atlasKey("VertexColor")
{
	coloring.colors.first = color;
	coloring.hasGradient = false;
}

Fill::Fill(const Ogre::ColourValue& gradientColor1, const Ogre::ColourValue& gradientColor2, short orientation) : isEmpty(false), atlasKey("VertexColor")
{
	coloring.colors.first = gradientColor1;
	coloring.colors.second = gradientColor2;
	coloring.hasGradient = true;
	coloring.orientation = orientation;
}

Fill::Fill(const Ogre::String& texture, const Ogre::ColourValue& color) : isEmpty(false), atlasKey(texture)
{
	coloring.colors.first = color;
	coloring.hasGradient = false;
}

Fill::Fill(const Ogre::String& texture, const Ogre::ColourValue& gradientColor1, const Ogre::ColourValue& gradientColor2, short orientation) : isEmpty(false), atlasKey(texture)
{
	coloring.colors.first = gradientColor1;
	coloring.colors.second = gradientColor2;
	coloring.hasGradient = true;
	coloring.orientation = orientation;
}

/************************
* Border
************************/

Border::Border() : isEmpty(true)
{
}

Border::Border(int width, const Ogre::ColourValue& color) : widths(width, width, width, width), colors(color, color, color, color), isEmpty(false)
{
}

Border::Border(const WidthRect& widths, const ColorRect& colors) : widths(widths), colors(colors), isEmpty(false)
{
}

/************************
* Canvas
************************/

Canvas::Canvas(Atlas* atlas, Ogre::Viewport* viewport) : atlas(atlas), viewport(viewport), vertexData(0), indexData(0), 
	bufferSize(100), renderQueueID(Ogre::RENDER_QUEUE_OVERLAY), isDirty(false), visibility(true)
{
	viewport->getTarget()->addListener(this);
	material = Ogre::MaterialManager::getSingleton().getByName(atlas->getMaterialName());
	setUseIdentityProjection(true);
	setUseIdentityView(true);

	resizeBuffers();
	clearClip();
}


Canvas::~Canvas()
{
	destroyBuffers();
	viewport->getTarget()->removeListener(this);
}

void Canvas::drawRectangle(int x, int y, int width, int height, const Fill& fill, const Border& border)
{
	PixelRect rect(x, y, x + width, y + height);

	if(border.isEmpty)
	{
		if(isOutsideClip(rect))
			return;
	}
	else
	{
		if(isOutsideClip(PixelRect(rect.left - border.widths.left, rect.top - border.widths.top, rect.right + border.widths.right, rect.bottom + border.widths.bottom)))
			return;
	}

	if(!fill.isEmpty)
	{
		TextureInfo texInfo = atlas->getTextureInfo(fill.atlasKey);
		if(texInfo.isEmpty)
			return;

		// Draw a simple rectangle with the normal texture-coordinates at each corner
		if(fill.atlasKey == "VertexColor" || (width == texInfo.width && height == texInfo.height))
		{
			drawQuad(rect, texInfo.texCoords, fill.coloring);
		}
		else // Draw a tiled rectangle, may contain multiple quads to give the illusion that the texture is "tiling"
		{
			Ogre::Real xMax = width / (double)texInfo.width;
			Ogre::Real yMax = height / (double)texInfo.height;

			Ogre::Real right = 0;
			Ogre::Real bottom = 0;

			for(Ogre::Real left = 0; left < Ogre::Math::Ceil(xMax); left++)
			{
				for(Ogre::Real top = 0; top < Ogre::Math::Ceil(yMax); top++)
				{
					if(left + 1 > xMax)
						right = xMax;
					else
						right = left + 1;

					if(top + 1 > yMax)
						bottom = yMax;
					else
						bottom = top + 1;

					PixelRect tile(x + (left * texInfo.width), y + (top * texInfo.height), x + (right * texInfo.width), y + (bottom * texInfo.height));

					if(isOutsideClip(tile))
						continue;

					Ogre::FloatRect texCoords = texInfo.texCoords;
					texCoords.right = texCoords.left + (right - left) * (texCoords.width());
					texCoords.bottom = texCoords.top + (bottom - top) * (texCoords.height());

					Coloring coloring = fill.coloring;
					if(coloring.hasGradient)
					{
						Ogre::Real amount1, amount2;

						if(coloring.orientation == Coloring::Vertical)
						{
							amount1 = top / yMax;
							amount2 = bottom / yMax;
						}
						else
						{
							amount1 = left / xMax;
							amount2 = right / xMax;
						}

						coloring.colors.first = (fill.coloring.colors.first * (1 - amount1)) + (fill.coloring.colors.second * amount1);
						coloring.colors.second = (fill.coloring.colors.first * (1 - amount2)) + (fill.coloring.colors.second * amount2);
					}

					drawQuad(tile, texCoords, coloring);
				}
			}
		}
	}

	// Draw the four sides of the border, if present
	if(!border.isEmpty)
	{
		Ogre::FloatRect vColCoords = atlas->getTextureInfo("VertexColor").texCoords;
		Corners<Ogre::Vector2> corners;

		PixelRect bRect(rect.left - border.widths.left, rect.top - border.widths.top, rect.right + border.widths.right, rect.bottom + border.widths.bottom);

		// Left Border
		if(!isOutsideClip(PixelRect(bRect.left, bRect.top, rect.left, bRect.bottom)))
		{
			corners.topLeft = Ogre::Vector2(bRect.left, bRect.top);
			corners.bottomLeft = Ogre::Vector2(bRect.left, bRect.bottom);
			corners.bottomRight = Ogre::Vector2(rect.left, rect.bottom);
			corners.topRight = Ogre::Vector2(rect.left, rect.top);
			
			drawQuad(corners, vColCoords, border.colors.left);
		}

		// Bottom Border
		if(!isOutsideClip(PixelRect(bRect.left, rect.bottom, bRect.right, bRect.bottom)))
		{
			corners.topLeft = Ogre::Vector2(rect.left, rect.bottom);
			corners.bottomLeft = Ogre::Vector2(bRect.left, bRect.bottom);
			corners.bottomRight = Ogre::Vector2(bRect.right, bRect.bottom);
			corners.topRight = Ogre::Vector2(rect.right, rect.bottom);

			drawQuad(corners, vColCoords, border.colors.bottom);
		}

		// Right Border
		if(!isOutsideClip(PixelRect(rect.right, bRect.top, bRect.right, bRect.bottom)))
		{
			corners.topLeft = Ogre::Vector2(rect.right, rect.top);
			corners.bottomLeft = Ogre::Vector2(rect.right, rect.bottom);
			corners.bottomRight = Ogre::Vector2(bRect.right, bRect.bottom);
			corners.topRight = Ogre::Vector2(bRect.right, bRect.top);

			drawQuad(corners, vColCoords, border.colors.right);
		}

		// Top Border
		if(!isOutsideClip(PixelRect(bRect.left, bRect.top, bRect.right, rect.top)))
		{
			corners.topLeft = Ogre::Vector2(bRect.left, bRect.top);
			corners.bottomLeft = Ogre::Vector2(rect.left, rect.top);
			corners.bottomRight = Ogre::Vector2(rect.right, rect.top);
			corners.topRight = Ogre::Vector2(bRect.right, bRect.top);

			drawQuad(corners, vColCoords, border.colors.top);
		}
	}
}

void Canvas::drawGlyph(const GlyphInfo& glyph, int x, int y, int width, int height, const Ogre::ColourValue& color)
{
	if(glyph.texInfo.isEmpty)
		return;

	PixelRect rect(x, y, x + width, y + height);

	if(isOutsideClip(rect))
		return;

	Coloring coloring;
	coloring.colors.first = color;
	coloring.hasGradient = false;

	drawQuad(rect, glyph.texInfo.texCoords, coloring);
}

void Canvas::clear()
{
	quadList.clear();
	isDirty = true;
}

void Canvas::setClip(int left, int top, int right, int bottom)
{
	clip.left = left;
	clip.top = top;
	clip.right = right;
	clip.bottom = bottom;
}

void Canvas::clearClip()
{
	clip.left = 0;
	clip.top = 0;
	clip.right = viewport->getActualWidth();
	clip.bottom = viewport->getActualHeight();
}

const Ogre::MaterialPtr& Canvas::getMaterial() const
{
	return material;
}

void Canvas::getRenderOperation(Ogre::RenderOperation& op)
{
	op.operationType = Ogre::RenderOperation::OT_TRIANGLE_LIST;

	op.vertexData = vertexData;
	op.vertexData->vertexStart = 0;
	op.vertexData->vertexCount = quadList.size() * 4;

	op.useIndexes = true;
	op.indexData = indexData;
	op.indexData->indexStart = 0;
	op.indexData->indexCount = quadList.size() * 6;
}

void Canvas::getWorldTransforms(Ogre::Matrix4* xform) const
{
	xform[0] = this->_getParentNodeFullTransform();
}

const Ogre::Quaternion& Canvas::getWorldOrientation() const
{
	return this->getParentNode()->_getDerivedOrientation();
}

const Ogre::Vector3& Canvas::getWorldPosition() const
{
	return this->getParentNode()->_getDerivedPosition();
}

Ogre::Real Canvas::getSquaredViewDepth(const Ogre::Camera* cam) const
{
	Ogre::Node* node = this->getParentNode();
	assert(node);
	return node->getSquaredViewDepth(cam);
}

const Ogre::LightList& Canvas::getLights() const
{
	return this->queryLights();
}

const Ogre::String& Canvas::getMovableType() const
{
	static Ogre::String typeName("Canvas");

	return typeName;
}

const Ogre::AxisAlignedBox& Canvas::getBoundingBox() const
{
	static Ogre::AxisAlignedBox box;
	box.setInfinite();

	return box;
}

Ogre::Real Canvas::getBoundingRadius() const
{
	return 2.0;
}

void Canvas::_updateRenderQueue(Ogre::RenderQueue* queue)
{
	resizeBuffers();
	updateGeometry();

	queue->addRenderable(this, renderQueueID);
}

void Canvas::setVisible(bool visible)
{
    mVisible = visibility = visible;
}

bool Canvas::isVisible() const
{
    if (!visibility || mBeyondFarDistance || mRenderingDisabled)
        return false;

    SceneManager* sm = Root::getSingleton()._getCurrentSceneManager();
    if (sm && !(mVisibilityFlags & sm->_getCombinedVisibilityMask()))
        return false;

    return true;
}

void Canvas::visitRenderables(Ogre::Renderable::Visitor* visitor, bool debugRenderables)
{
}

void Canvas::preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
}

void Canvas::postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt)
{
}

void Canvas::preViewportUpdate(const Ogre::RenderTargetViewportEvent& evt)
{
	if(evt.source == viewport && mVisible)
		visibility = true;
}

void Canvas::postViewportUpdate(const Ogre::RenderTargetViewportEvent& evt)
{
	if(evt.source == viewport && mVisible)
		visibility = false;
}

void Canvas::viewportAdded(const Ogre::RenderTargetViewportEvent& evt)
{
}

void Canvas::viewportRemoved(const Ogre::RenderTargetViewportEvent& evt)
{
}

void Canvas::destroyBuffers()
{
	if(vertexData)
	{
		delete vertexData;
		vertexData = 0;

		buffer.setNull();
	}

	if(indexData)
	{
		delete indexData;
		indexData = 0;
	}
}

void Canvas::resizeBuffers()
{
	if(bufferSize < quadList.size())
	{
		bufferSize = quadList.size() * 2;
		destroyBuffers();
	}

	if(!vertexData)
	{
		vertexData = new Ogre::VertexData();
		vertexData->vertexStart = 0;
		vertexData->vertexCount = bufferSize * 4;

		Ogre::VertexDeclaration* decl = vertexData->vertexDeclaration;
		Ogre::VertexBufferBinding* binding = vertexData->vertexBufferBinding;

		size_t offset = 0;
		decl->addElement(0, offset, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
		offset += Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
		decl->addElement(0, offset, Ogre::VET_COLOUR, Ogre::VES_DIFFUSE);
		offset += Ogre::VertexElement::getTypeSize(Ogre::VET_COLOUR);
		decl->addElement(0, offset, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES, 0);

		buffer = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(
			decl->getVertexSize(0), vertexData->vertexCount, Ogre::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);
		binding->setBinding(0, buffer);
	}

	if(!indexData)
	{
		indexData = new Ogre::IndexData();
		indexData->indexStart = 0;
		indexData->indexCount = bufferSize * 6;

		indexData->indexBuffer = Ogre::HardwareBufferManager::getSingleton().createIndexBuffer(
			Ogre::HardwareIndexBuffer::IT_16BIT, indexData->indexCount, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);

		unsigned short* indexBuffer = (unsigned short*)indexData->indexBuffer->lock(0, indexData->indexBuffer->getSizeInBytes(), Ogre::HardwareBuffer::HBL_DISCARD);

		// Indexes are generated here because we know that we will only be rendering quads
		// This means that we only have to handle updating the vertex buffer in Canvas::updateGeometry
		for(size_t indexIdx, vertexIdx, quadIdx = 0; quadIdx < bufferSize; quadIdx++)
		{
			indexIdx = quadIdx * 6;
			vertexIdx = quadIdx * 4;

			indexBuffer[indexIdx++] = (unsigned short)(vertexIdx + 0);
			indexBuffer[indexIdx++] = (unsigned short)(vertexIdx + 2);
			indexBuffer[indexIdx++] = (unsigned short)(vertexIdx + 1);
			indexBuffer[indexIdx++] = (unsigned short)(vertexIdx + 1);
			indexBuffer[indexIdx++] = (unsigned short)(vertexIdx + 2);
			indexBuffer[indexIdx++] = (unsigned short)(vertexIdx + 3);
		}

		indexData->indexBuffer->unlock();
	}
}

bool Canvas::isOutsideClip(const PixelRect& rect)
{
	if(rect.left > clip.right)
		return true;
	else if(rect.right < clip.left)
		return true;
	else if(rect.top > clip.bottom)
		return true;
	else if(rect.bottom < clip.top)
		return true;

	return false;
}

void Canvas::localize(Ogre::Vector2& vertex)
{
	Ogre::Real xTexel = Ogre::Root::getSingleton().getRenderSystem()->getHorizontalTexelOffset();
	Ogre::Real yTexel = Ogre::Root::getSingleton().getRenderSystem()->getVerticalTexelOffset();

	vertex.x = ((vertex.x + xTexel) / (Ogre::Real)viewport->getActualWidth()) * 2 - 1;
	vertex.y = ((vertex.y + yTexel) / (Ogre::Real)viewport->getActualHeight()) * -2 + 1;
}

void Canvas::drawQuad(const PixelRect& rect, const Ogre::FloatRect& texCoords, const Coloring& coloring)
{
	PixelRect clipped;
	Ogre::FloatRect clippedTexCoords(texCoords);
	Coloring clippedColoring = coloring;

	if(rect.left > clip.left)
	{
		clipped.left = rect.left;
	}
	else
	{
		clipped.left = clip.left;
		Ogre::Real delta = (clipped.left - rect.left) / (float)rect.width();
		clippedTexCoords.left += delta * texCoords.width();

		if(coloring.hasGradient && coloring.orientation == Coloring::Horizontal)
			clippedColoring.colors.first = (coloring.colors.first * (1 - delta)) + (coloring.colors.second * delta);
	}

	if(rect.top > clip.top)
	{
		clipped.top = rect.top;
	}
	else
	{
		clipped.top = clip.top;
		Ogre::Real delta = (clipped.top - rect.top) / (float)rect.height();
		clippedTexCoords.top += delta * texCoords.height();

		if(coloring.hasGradient && coloring.orientation == Coloring::Vertical)
			clippedColoring.colors.first = (coloring.colors.first * (1 - delta)) + (coloring.colors.second * delta);
	}

	if(rect.right < clip.right)
	{
		clipped.right = rect.right;
	}
	else
	{
		clipped.right = clip.right;
		Ogre::Real delta = (clipped.right - rect.right) / (float)rect.width();
		clippedTexCoords.right += delta * texCoords.width();

		if(coloring.hasGradient && coloring.orientation == Coloring::Horizontal)
			clippedColoring.colors.second = (coloring.colors.first * (-delta)) + (coloring.colors.second * (1 + delta));
	}

	if(rect.bottom < clip.bottom)
	{
		clipped.bottom = rect.bottom;
	}
	else
	{
		clipped.bottom = clip.bottom;
		Ogre::Real delta = (clipped.bottom - rect.bottom) / (float)rect.height();
		clippedTexCoords.bottom += delta * texCoords.height();

		if(coloring.hasGradient && coloring.orientation == Coloring::Vertical)
			clippedColoring.colors.second = (coloring.colors.first * (-delta)) + (coloring.colors.second * (1 + delta));
	}
	
	Canvas::Quad quad;
	quad.vertices = Corners<Ogre::Vector2>(Ogre::Vector2(clipped.left, clipped.top), Ogre::Vector2(clipped.left, clipped.bottom), 
		Ogre::Vector2(clipped.right, clipped.bottom), Ogre::Vector2(clipped.right, clipped.top));

	localize(quad.vertices.topLeft);
	localize(quad.vertices.bottomLeft);
	localize(quad.vertices.bottomRight);
	localize(quad.vertices.topRight);

	quad.texCoords.topLeft = Ogre::Vector2(clippedTexCoords.left, clippedTexCoords.top);
	quad.texCoords.bottomLeft = Ogre::Vector2(clippedTexCoords.left, clippedTexCoords.bottom);
	quad.texCoords.bottomRight = Ogre::Vector2(clippedTexCoords.right, clippedTexCoords.bottom);
	quad.texCoords.topRight = Ogre::Vector2(clippedTexCoords.right, clippedTexCoords.top);
	
	if(clippedColoring.hasGradient)
	{
		if(clippedColoring.orientation == Coloring::Vertical)
			quad.colors = Corners<Ogre::ColourValue>(clippedColoring.colors.first, clippedColoring.colors.second, clippedColoring.colors.second, clippedColoring.colors.first);
		else
			quad.colors = Corners<Ogre::ColourValue>(clippedColoring.colors.first, clippedColoring.colors.first, clippedColoring.colors.second, clippedColoring.colors.second);
	}
	else
	{
		quad.colors = Corners<Ogre::ColourValue>(clippedColoring.colors.first);
	}

	quadList.push_back(quad);
	isDirty = true;
}

void Canvas::drawQuad(const Corners<Ogre::Vector2>& corners, const Ogre::FloatRect& texCoords, const Ogre::ColourValue& color)
{
	Corners<Ogre::Vector2> clippedCorners;
	clippedCorners.topLeft.x = corners.topLeft.x > clip.left ? corners.topLeft.x : clip.left;
	clippedCorners.topLeft.y = corners.topLeft.y > clip.top ? corners.topLeft.y : clip.top;
	clippedCorners.bottomLeft.x = corners.bottomLeft.x > clip.left ? corners.bottomLeft.x : clip.left;
	clippedCorners.bottomLeft.y = corners.bottomLeft.y < clip.bottom ? corners.bottomLeft.y : clip.bottom;
	clippedCorners.bottomRight.x = corners.bottomRight.x < clip.right ? corners.bottomRight.x : clip.right;
	clippedCorners.bottomRight.y = corners.bottomRight.y < clip.bottom ? corners.bottomRight.y : clip.bottom;
	clippedCorners.topRight.x = corners.topRight.x < clip.right ? corners.topRight.x : clip.right;
	clippedCorners.topRight.y = corners.topRight.y > clip.top ? corners.topRight.y : clip.top;

	Canvas::Quad quad;
	quad.vertices = clippedCorners;

	localize(quad.vertices.topLeft);
	localize(quad.vertices.bottomLeft);
	localize(quad.vertices.bottomRight);
	localize(quad.vertices.topRight);

	quad.texCoords.topLeft = Ogre::Vector2(texCoords.left, texCoords.top);
	quad.texCoords.bottomLeft = Ogre::Vector2(texCoords.left, texCoords.bottom);
	quad.texCoords.bottomRight = Ogre::Vector2(texCoords.right, texCoords.bottom);
	quad.texCoords.topRight = Ogre::Vector2(texCoords.right, texCoords.top);
	
	quad.colors = Corners<Ogre::ColourValue>(color);

	quadList.push_back(quad);
	isDirty = true;
}

void Canvas::updateGeometry()
{
	if(!isDirty)
		return;

	float* vBuffer = (float*)buffer->lock(0, quadList.size() * buffer->getVertexSize() * 4, Ogre::HardwareBuffer::HBL_DISCARD);
	
	Ogre::RGBA color;
	Ogre::RGBA* vBufferCol = 0;

	for(std::vector<Canvas::Quad>::iterator i = quadList.begin(); i != quadList.end(); i++)
	{
		// Top-Left Vertex
		*vBuffer++ = i->vertices.topLeft.x;
		*vBuffer++ = i->vertices.topLeft.y;
		*vBuffer++ = 0;

		vBufferCol = (Ogre::RGBA*)vBuffer;
		Ogre::Root::getSingleton().convertColourValue(i->colors.topLeft, &color);
		*vBufferCol++ = color;

		vBuffer = (float*)vBufferCol;

		*vBuffer++ = i->texCoords.topLeft.x;
		*vBuffer++ = i->texCoords.topLeft.y;

		// Top-Right Vertex
		*vBuffer++ = i->vertices.topRight.x;
		*vBuffer++ = i->vertices.topRight.y;
		*vBuffer++ = 0;

		vBufferCol = (Ogre::RGBA*)vBuffer;
		Ogre::Root::getSingleton().convertColourValue(i->colors.topRight, &color);
		*vBufferCol++ = color;

		vBuffer = (float*)vBufferCol;

		*vBuffer++ = i->texCoords.topRight.x;
		*vBuffer++ = i->texCoords.topRight.y;

		// Bottom-Left Vertex
		*vBuffer++ = i->vertices.bottomLeft.x;
		*vBuffer++ = i->vertices.bottomLeft.y;
		*vBuffer++ = 0;

		vBufferCol = (Ogre::RGBA*)vBuffer;
		Ogre::Root::getSingleton().convertColourValue(i->colors.bottomLeft, &color);
		*vBufferCol++ = color;

		vBuffer = (float*)vBufferCol;

		*vBuffer++ = i->texCoords.bottomLeft.x;
		*vBuffer++ = i->texCoords.bottomLeft.y;

		// Bottom-Right Vertex
		*vBuffer++ = i->vertices.bottomRight.x;
		*vBuffer++ = i->vertices.bottomRight.y;
		*vBuffer++ = 0;

		vBufferCol = (Ogre::RGBA*)vBuffer;
		Ogre::Root::getSingleton().convertColourValue(i->colors.bottomRight, &color);
		*vBufferCol++ = color;

		vBuffer = (float*)vBufferCol;

		*vBuffer++ = i->texCoords.bottomRight.x;
		*vBuffer++ = i->texCoords.bottomRight.y;
	}

	buffer->unlock();
	isDirty = false;
}