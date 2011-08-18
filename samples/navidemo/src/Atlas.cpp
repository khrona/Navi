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

#include "Atlas.h"

/************************
* TextureInfo
************************/

TextureInfo::TextureInfo() : isEmpty(true), width(0), height(0)
{
}

TextureInfo::TextureInfo(int atlasWidth, int atlasHeight, int x, int y, int width, int height) : isEmpty(false), width(width), height(height)
{
	texCoords.left = x / (float)atlasWidth;
	texCoords.top = y / (float)atlasHeight;
	texCoords.right = (x + width) / (float)atlasWidth;
	texCoords.bottom = (y + height) / (float)atlasHeight;
}

/************************
* GlyphInfo
************************/

GlyphInfo::GlyphInfo() : bearingX(0), bearingY(0), advance(0)
{
}

GlyphInfo::GlyphInfo(Ogre::Real bearingX, Ogre::Real bearingY, Ogre::Real advance)
	: bearingX(bearingX), bearingY(bearingY), advance(advance)
{
}

/************************
* FontMetrics
************************/

FontMetrics::FontMetrics() : ascender(0), descender(0), height(0), maxAdvance(0)
{
}

FontMetrics::FontMetrics(Ogre::Real ascender, Ogre::Real descender, Ogre::Real height, Ogre::Real maxAdvance)
	: ascender(ascender), descender(descender), height(height), maxAdvance(maxAdvance)
{

}

/************************
* CharCodeRange
************************/

const CharCodeRange CharCodeRange::BasicLatin = CharCodeRange(32, 166);
const CharCodeRange CharCodeRange::Latin1 = CharCodeRange(32, 255);
const CharCodeRange CharCodeRange::All = CharCodeRange();

CharCodeRange::CharCodeRange()
{
}

CharCodeRange::CharCodeRange(CharCode from, CharCode to)
{
	addRange(from, to);
}

void CharCodeRange::addRange(CharCode from, CharCode to)
{
	ranges.push_back(std::pair<CharCode, CharCode>(from, to));
}

bool CharCodeRange::isWithinRange(CharCode code) const
{
	if(ranges.empty())
		return true;

	for(std::vector<std::pair<CharCode, CharCode> >::const_iterator i = ranges.begin(); i != ranges.end(); i++)
	{
		if(code >= i->first && code <= i->second)
			return true;
	}

	return false;
}

/************************
* ComputationRect
************************/

ComputationRect::ComputationRect(const Ogre::String& texFilename, const Ogre::String& resourceGroup) 
	: x(0), y(0), weight(1), isPlaced(false), isFontGlyph(false), filename(texFilename)
{
	image.load(texFilename, resourceGroup);
	width = (int)image.getWidth();
	height = (int)image.getHeight();
	area = width * height;
}

ComputationRect::ComputationRect(const Ogre::String& texName, unsigned char* buffer, int width, int height)
	: x(0), y(0), weight(1), isPlaced(false), isFontGlyph(false), filename(texName)
{
	image.loadDynamicImage(buffer, width, height, 1, Ogre::PF_BYTE_BGRA, true);
	this->width = (int)image.getWidth();
	this->height = (int)image.getHeight();
	area = this->width * this->height;
}

ComputationRect::ComputationRect(const Ogre::String& fontFilename, Ogre::uint fontSize, CharCode charCode, unsigned char* buffer, int width, int height)
	: x(0), y(0), weight(1), isPlaced(false), isFontGlyph(true), filename(fontFilename), fontSize(fontSize), charCode(charCode)
{
	image.loadDynamicImage(buffer, width, height, 1, Ogre::PF_BYTE_LA, true);
	this->width = (int)image.getWidth();
	this->height = (int)image.getHeight();
	area = this->width * this->height;
}

/************************
* FontFaceDefinition
************************/

FontFaceDefinition::FontFaceDefinition(const Ogre::String& filename, const CharCodeRange& codeRange, short renderType) : filename(filename), codeRange(codeRange), renderType(renderType)
{
}

void FontFaceDefinition::addSize(Ogre::uint fontSize)
{
	sizes.push_back(fontSize);
}

/************************
* FontFace
************************/

FontFace::FontFace()
{
}

FontFace::FontFace(const FontFaceDefinition& definition, const Ogre::String& resourceGroup, ComputationVector& renderContext)
{
	FT_Library library;
	FT_Face face;
	FT_Error error;

	error = FT_Init_FreeType(&library);
	if(error)
		OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR, "Could not load FreeType library.", "FontFace::FontFace");

	Ogre::DataStreamPtr dataStream = Ogre::ResourceGroupManager::getSingleton().openResource(definition.filename, resourceGroup);
	Ogre::MemoryDataStream stream(dataStream);
	
	error = FT_New_Memory_Face(library, stream.getPtr(), (FT_Long)stream.size(), 0, &face);
	if(error)
		OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR, "FreeType could not load a font-face.", "FontFace::FontFace");


	for(std::vector<Ogre::uint>::const_iterator i = definition.sizes.begin(); i != definition.sizes.end(); i++)
	{
		FT_ULong charCode = 0;
		FT_UInt glyphIndex = 1;

		error = FT_Set_Pixel_Sizes(face, 0, (FT_UInt)*i);
		if(error)
			OGRE_EXCEPT(Ogre::Exception::ERR_INTERNAL_ERROR, "FreeType could not set a font-size.", "FontFace::FontFace");

		fontMetrics[*i] = FontMetrics(face->size->metrics.ascender / 64.0f, face->size->metrics.descender / 64.0f, 
			face->size->metrics.height / 64.0f, face->size->metrics.max_advance / 64.0f);

		for(charCode = FT_Get_First_Char(face, &glyphIndex); glyphIndex != 0; charCode = FT_Get_Next_Char(face, charCode, &glyphIndex))
		{
			if(!definition.codeRange.isWithinRange(charCode))
				continue;

			if(definition.renderType == FontFaceDefinition::BetterContrast)
			{
				error = FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT);
				if(error)
					continue;

				error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
				if(error)
					continue;
			}
			else
			{
				error = FT_Load_Glyph(face, glyphIndex, FT_LOAD_TARGET_LIGHT);
				if(error)
					continue;

				error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_LIGHT);
				if(error)
					continue;
			}

			FT_Bitmap& bitmap = face->glyph->bitmap;
			FT_Glyph_Metrics& metrics = face->glyph->metrics;

			if(!bitmap.buffer || (!bitmap.rows && !bitmap.width))
				continue;

			unsigned char* buffer = OGRE_ALLOC_T(unsigned char, bitmap.rows * bitmap.pitch * 2, Ogre::MEMCATEGORY_GENERAL);

			for(int idx = 0; idx < bitmap.rows * bitmap.pitch; idx++)
			{
				buffer[idx * 2] = 255;
				buffer[idx * 2 + 1] = bitmap.buffer[idx];
			}

			fontSizes[*i][charCode] = GlyphInfo(metrics.horiBearingX / 64.0f, metrics.horiBearingY / 64.0f, metrics.horiAdvance / 64.0f);

			renderContext.push_back(new ComputationRect(definition.filename, (Ogre::uint)*i, charCode, buffer, bitmap.pitch, bitmap.rows));
		}
	}
}

/************************
* Atlas
************************/

Atlas::Atlas(const std::vector<Ogre::String>& textureFilenames, const std::vector<FontFaceDefinition>& fonts, const Ogre::String& resourceGroup)
{
#if OGRE_DEBUG_MODE
		Ogre::LogManager::getSingleton().logMessage("Loading an Atlas.");
#endif

	Ogre::Timer timer;

	ComputationVector rectangles;

	for(std::vector<FontFaceDefinition>::const_iterator i = fonts.begin(); i != fonts.end(); i++)
		fontFaces[i->filename] = FontFace(*i, resourceGroup, rectangles);

	for(std::vector<Ogre::String>::const_iterator i = textureFilenames.begin(); i != textureFilenames.end(); i++)
		rectangles.push_back(new ComputationRect(*i, resourceGroup));

	// Temporary image buffer for the "VertexColor" texture
	unsigned char* vcolBuffer = OGRE_ALLOC_T(unsigned char, 16, Ogre::MEMCATEGORY_GENERAL);
	memset(vcolBuffer, 255, 16);
	
	rectangles.push_back(new ComputationRect("VertexColor", vcolBuffer, 2, 2));

	const Ogre::RenderSystemCapabilities* capabilities = Ogre::Root::getSingleton().getRenderSystem()->getCapabilities();
	//supportsNPOT = capabilities->hasCapability(Ogre::RSC_NON_POWER_OF_2_TEXTURES) && !capabilities->getNonPOW2TexturesLimited();
	supportsNPOT = false;

	guessDimensions(rectangles);
	pack(rectangles);
	paint(rectangles);

	int glyphCount = 0;
	int texCount = -1; // Subtract the default VertexColor texture
	ComputationRect* rect;

	for(ComputationVector::iterator i = rectangles.begin(); i != rectangles.end(); i++)
	{
		rect = (*i);

		if(rect->isFontGlyph)
		{
			fontFaces[rect->filename].fontSizes[rect->fontSize][rect->charCode].texInfo = TextureInfo(dimensions.first, dimensions.second, rect->x, rect->y, rect->width, rect->height);
			glyphCount++;
		}
		else
		{
			textures[rect->filename] = TextureInfo(dimensions.first, dimensions.second, rect->x, rect->y, rect->width, rect->height);
			texCount++;
		}

		delete rect;
	}

	Ogre::LogManager::getSingleton().logMessage("Atlas loaded in " + Ogre::StringConverter::toString(timer.getMilliseconds() / 1000.0f) + 
		" secs. Packed " + Ogre::StringConverter::toString(glyphCount) + " font glyphs and " + Ogre::StringConverter::toString(texCount) +
		" textures into " + Ogre::StringConverter::toString(dimensions.first) + "x" + Ogre::StringConverter::toString(dimensions.second) +
		", with an efficiency of " + Ogre::StringConverter::toString(actualArea / (float)(dimensions.first * dimensions.second) * 100) + "%.");
}

const std::pair<int, int>& Atlas::getDimensions() const
{
	return dimensions;
}

const Ogre::String& Atlas::getMaterialName() const
{
	return materialName;
}

const TextureInfo& Atlas::getTextureInfo(const Ogre::String& filename) const
{
	std::map<Ogre::String, TextureInfo>::const_iterator i = textures.find(filename);
	static TextureInfo empty;

	if(i != textures.end())
		return i->second;

	return empty;
}

const FontMetrics& Atlas::getFontMetrics(const Ogre::String& fontFilename, Ogre::uint fontSize) const
{
	std::map<Ogre::String, FontFace>::const_iterator font = fontFaces.find(fontFilename);
	static FontMetrics empty;

	if(font == fontFaces.end())
		return empty;

	FontMetricsMap::const_iterator iter = font->second.fontMetrics.find(fontSize);

	if(iter == font->second.fontMetrics.end())
		return empty;

	return iter->second;
}

const GlyphMap& Atlas::getGlyphMap(const Ogre::String& fontFilename, Ogre::uint fontSize) const
{
	std::map<Ogre::String, FontFace>::const_iterator font = fontFaces.find(fontFilename);
	static GlyphMap empty;

	if(font == fontFaces.end())
		return empty;

	FontSizeMap::const_iterator size = font->second.fontSizes.find(fontSize);

	if(size == font->second.fontSizes.end())
		return empty;

	return size->second;
}

const GlyphInfo& Atlas::getGlyphInfo(const Ogre::String& fontFilename, Ogre::uint fontSize, CharCode charCode) const
{
	std::map<Ogre::String, FontFace>::const_iterator font = fontFaces.find(fontFilename);
	static GlyphInfo empty;

	if(font == fontFaces.end())
		return empty;

	FontSizeMap::const_iterator size = font->second.fontSizes.find(fontSize);

	if(size == font->second.fontSizes.end())
		return empty;

	GlyphMap::const_iterator glyph = size->second.find(charCode);

	if(glyph == size->second.end())
		return empty;

	return glyph->second;
}

void Atlas::guessDimensions(ComputationVector& rectangles)
{
	actualArea = 0;
	int maxWidth = 0;
	int maxHeight = 0;
	Ogre::Real totalOblong = 0;

	for(ComputationVector::const_iterator i = rectangles.begin(); i != rectangles.end(); i++)
	{
		actualArea += (*i)->area;
		maxWidth = std::max(maxWidth, (*i)->width);
		maxHeight = std::max(maxHeight, (*i)->height);
		totalOblong += (*i)->area * (std::max((*i)->width, (*i)->height) + 1) / ((float)std::min((*i)->width, (*i)->height) + 1);
	}

	Ogre::Real oblongFactor = totalOblong / actualArea / 2;
	Ogre::Real oblongitude, percentArea;

	for(ComputationVector::iterator i = rectangles.begin(); i != rectangles.end(); i++)
	{
		oblongitude = (std::max((*i)->width, (*i)->height) + 1) / ((float)std::min((*i)->width, (*i)->height) + 1);

		percentArea = (*i)->area / (Ogre::Real)actualArea;

		(*i)->weight = percentArea * pow(oblongitude, percentArea + oblongFactor);
	}

	int squareRoot = (int)Ogre::Math::Ceil(sqrt((double)actualArea * 1.02));

	if(maxWidth > squareRoot)
	{
		dimensions.first = supportsNPOT ? maxWidth : Ogre::Bitwise::firstPO2From(maxWidth);
		dimensions.second = std::max(actualArea / dimensions.first, maxHeight);
		dimensions.second = supportsNPOT ? dimensions.second : Ogre::Bitwise::firstPO2From(dimensions.second);
	}
	else if(maxHeight > squareRoot)
	{
		dimensions.second = supportsNPOT ? maxHeight : Ogre::Bitwise::firstPO2From(maxHeight);
		dimensions.first = std::max(actualArea / dimensions.second, maxWidth);
		dimensions.first = supportsNPOT ? dimensions.first : Ogre::Bitwise::firstPO2From(dimensions.first);
	}
	else
	{
		if(!supportsNPOT)
		{
			if(maxWidth > maxHeight)
			{
				dimensions.first = Ogre::Bitwise::firstPO2From(squareRoot);
				dimensions.second =  Ogre::Bitwise::firstPO2From(std::max(actualArea / dimensions.first, maxHeight));
			}
			else
			{
				dimensions.second = Ogre::Bitwise::firstPO2From(squareRoot);
				dimensions.first =  Ogre::Bitwise::firstPO2From(std::max(actualArea / dimensions.second, maxWidth));
			}
		}
		else
		{
			dimensions.first = squareRoot;
			dimensions.second = squareRoot;
		}
	}

#if OGRE_DEBUG_MODE
	Ogre::LogManager::getSingleton().logMessage("Atlas: Dimensions estimated as " + Ogre::StringConverter::toString(dimensions.first) + "x" +
		Ogre::StringConverter::toString(dimensions.second));
#endif
}

bool WeightCompare(ComputationRect* a, ComputationRect* b)
{
	return a->weight > b->weight;
}

void Atlas::pack(ComputationVector& rectangles)
{
	std::sort(rectangles.begin(), rectangles.end(), WeightCompare);
	int attemptCount = 0;

	while(true)
	{
		int successCount = 0;
		fill(rectangles, 0, 0, dimensions.first - 1, dimensions.second - 1, successCount);

		if(successCount == (int)rectangles.size())
			break;

#if OGRE_DEBUG_MODE
		Ogre::LogManager::getSingleton().logMessage("Atlas: Failed to pack " + Ogre::StringConverter::toString(rectangles.size() - successCount) + " rectangle(s), trying again");
#endif

		attemptCount++;

		if(!supportsNPOT)
		{
			if(dimensions.first < dimensions.second)
				dimensions.first = Ogre::Bitwise::firstPO2From(dimensions.first + 1);
			else
				dimensions.second = Ogre::Bitwise::firstPO2From(dimensions.second + 1);

			for(ComputationVector::iterator i = rectangles.begin(); i != rectangles.end(); i++)
				(*i)->isPlaced = false;

#if OGRE_DEBUG_MODE
			Ogre::LogManager::getSingleton().logMessage("Atlas: Dimensions resized to " + Ogre::StringConverter::toString(dimensions.first) + "x" +
				Ogre::StringConverter::toString(dimensions.second));
#endif

			continue;
		}

		int maxFailedWidth = 0;
		int maxFailedHeight = 0;
		int totalFailedArea = 0;

		for(ComputationVector::iterator i = rectangles.begin(); i != rectangles.end(); i++)
		{
			if(!(*i)->isPlaced)
			{
				maxFailedWidth = std::max(maxFailedWidth, (*i)->width);
				maxFailedHeight = std::max(maxFailedHeight, (*i)->height);
				totalFailedArea += (*i)->area;
			}
			else
			{
				(*i)->isPlaced = false;
			}
		}

		Ogre::Real growthFactor = pow(1.02f, attemptCount) + totalFailedArea / (Ogre::Real)actualArea;

		if(maxFailedWidth > maxFailedHeight)
			dimensions.first = (dimensions.first + 1) * growthFactor;
		else
			dimensions.second = (dimensions.second + 1) * growthFactor;

#if OGRE_DEBUG_MODE
			Ogre::LogManager::getSingleton().logMessage("Atlas: Dimensions resized to " + Ogre::StringConverter::toString(dimensions.first) + "x" +
				Ogre::StringConverter::toString(dimensions.second));
#endif
	}
}

void Atlas::fill(ComputationVector& rectangles, int x1, int y1, int x2, int y2, int& count)
{                  
	static ComputationVector::iterator iter;

	for(iter = rectangles.begin(); iter != rectangles.end(); iter++)
	{
		if((!(*iter)->isPlaced) && (x2 - x1 + 1 >= (*iter)->width) && (y2 - y1 + 1 >= (*iter)->height))
			break;
	}

	if(iter == rectangles.end())
		return;

	ComputationRect* rect = *iter;

	rect->x = x1;
	rect->y = y1;
	rect->isPlaced = true;
	count++;

	if((x2 - x1 + 1 - rect->width) * rect->height < (y2 - y1 + 1 - rect->height) * rect->width)
	{
		if(y1 + rect->height < y2)
			fill(rectangles, x1, y1 + rect->height, x2, y2, count);

		if((x1 + rect->width < x2) && (y1 < y1 + rect->height - 1))
			fill(rectangles, x1 + rect->width, y1, x2, y1 + rect->height - 1, count);
	}
	else
	{
		if(x1 + rect->width < x2)
			fill(rectangles, x1 + rect->width, y1, x2, y2, count);

		if((y1 + rect->height < y2) && (x1 < x1 + rect->width - 1))
			fill(rectangles, x1, y1 + rect->height, x1 + rect->width - 1, y2, count);
	}
}

void Atlas::paint(const ComputationVector& rectangles)
{
	static unsigned int count = 0;
	Ogre::String texName = "AtlasTexture_" + Ogre::StringConverter::toString(count);
	materialName = "AtlasMaterial_" + Ogre::StringConverter::toString(count);

	Ogre::TexturePtr texture = Ogre::TextureManager::getSingleton().createManual(
		texName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
		Ogre::TEX_TYPE_2D, dimensions.first, dimensions.second, 0, Ogre::PF_BYTE_BGRA,
		Ogre::TU_STATIC_WRITE_ONLY, this);

	Ogre::HardwarePixelBufferSharedPtr pixelBuffer = texture->getBuffer();
	pixelBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD);
	const Ogre::PixelBox& pixelBox = pixelBuffer->getCurrentLock();
	size_t dstBpp = Ogre::PixelUtil::getNumElemBytes(pixelBox.format);
	size_t dstPitch = pixelBox.rowPitch * dstBpp;

	Ogre::uint8* dstData = static_cast<Ogre::uint8*>(pixelBox.data);

	for(ComputationVector::const_iterator i = rectangles.begin(); i != rectangles.end(); i++)
	{
		unsigned char* conversionBuf = 0;
		Ogre::PixelBox srcPixels = (*i)->image.getPixelBox();
		
		if((*i)->image.getFormat() != Ogre::PF_BYTE_BGRA)
		{
			conversionBuf = new unsigned char[(*i)->image.getWidth() * (*i)->image.getHeight() * dstBpp];
			Ogre::PixelBox convPixels(Ogre::Box(0, 0, (*i)->width, (*i)->height), Ogre::PF_BYTE_BGRA, conversionBuf);
			Ogre::PixelUtil::bulkPixelConversion((*i)->image.getPixelBox(), convPixels);
			srcPixels = convPixels;
		}

		size_t srcPitch = srcPixels.rowPitch * dstBpp;
		Ogre::uint8* srcData = static_cast<Ogre::uint8*>(srcPixels.data);

		for(size_t row = 0; row < (*i)->image.getHeight(); row++)
		{
			for(size_t col = 0; col < srcPitch; col++)
			{
				dstData[((row + (*i)->y) * dstPitch) + ((*i)->x * dstBpp) + col] = srcData[(row * srcPitch) + col];
			}
		}

		if(conversionBuf)
			delete[] conversionBuf;
	}

	pixelBuffer->unlock();

	Ogre::MaterialPtr material = Ogre::MaterialManager::getSingleton().create(materialName, 
		Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	Ogre::Pass* pass = material->getTechnique(0)->getPass(0);
	pass->setDepthCheckEnabled(false);
	pass->setDepthWriteEnabled(false);
	pass->setLightingEnabled(false);
	pass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);

	Ogre::TextureUnitState* texUnit = pass->createTextureUnitState(texName);
	texUnit->setTextureFiltering(Ogre::FO_NONE, Ogre::FO_NONE, Ogre::FO_NONE);
	texUnit->setTextureAddressingMode(Ogre::TextureUnitState::TAM_CLAMP);

	count++;
}

void Atlas::loadResource(Ogre::Resource* resource)
{
	Ogre::Texture *texture = static_cast<Ogre::Texture*>(resource); 

	texture->setTextureType(Ogre::TEX_TYPE_2D);
	texture->setWidth(dimensions.first);
	texture->setHeight(dimensions.second);
	texture->setNumMipmaps(0);
	texture->setFormat(Ogre::PF_BYTE_BGRA);
	texture->setUsage(Ogre::TU_STATIC_WRITE_ONLY);
	texture->createInternalResources();

	// Re-painting is not implemented
}