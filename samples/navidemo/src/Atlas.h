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

#ifndef __Atlas__
#define __Atlas__

#include <OGRE/Ogre.h>
#include <OGRE/OgreBitwise.h>
#include <map>
#include <vector>
#include <algorithm>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

/**
* TextureInfo represents a texture within an Atlas instance. 
* Contains the actual dimensions of a texture and its location within the atlas.
*/
struct TextureInfo
{
	bool isEmpty;
	Ogre::FloatRect texCoords;
	int width, height;

	TextureInfo();
	TextureInfo(int atlasWidth, int atlasHeight, int x, int y, int width, int height);
};

/**
* GlyphInfo represents a glyph within an Atlas instance.
* Contains glyph metrics which can be used for text layout purposes.
* See: http://freetype.sourceforge.net/freetype2/docs/glyphs/glyphs-3.html
*/
struct GlyphInfo
{
	Ogre::Real bearingX;
	Ogre::Real bearingY;
	Ogre::Real advance;

	TextureInfo texInfo;

	GlyphInfo();
	GlyphInfo(Ogre::Real bearingX, Ogre::Real bearingY, Ogre::Real advance);
};

/**
* FontMetrics represents the scaled global metrics for a certain font size.
* Contains font metrics which can be used for text layout purposes.
*
* <ul>
* <li>ascender - The typographic ascender of the face, expressed in Real pixels.
* <li>descender - The typographic descender of the face, expressed in Real pixels.
* <li>height - The height is the vertical distance between two consecutive baselines, expressed in Real pixels.
* <li>maxAdvance - The maximal advance width, in Real pixels, for all glyphs in this face. This can be used to make word wrapping computations faster.
* </ul>
*/
struct FontMetrics
{
	Ogre::Real ascender;
	Ogre::Real descender;
	Ogre::Real height;
	Ogre::Real maxAdvance;

	FontMetrics();
	FontMetrics(Ogre::Real ascender, Ogre::Real descender, Ogre::Real height, Ogre::Real maxAdvance);
};

typedef Ogre::uint32 CharCode;

/**
* CharCodeRange is used to specify the ranges of characters that a font should load.
* For convenience, two common unicode ranges (BasicLatin & Latin1) have been already defined.
*/
struct CharCodeRange
{
	std::vector<std::pair<CharCode, CharCode> > ranges;

	CharCodeRange();
	CharCodeRange(CharCode from, CharCode to);

	/**
	* Add a range of characters to the definition.
	*
	* @param	from	The beginning of the range, inclusive.
	* @param	to		The end of the range, inclusive.
	*/
	void addRange(CharCode from, CharCode to);

	/**
	* Test if a character code is within this CharCodeRange.
	*
	* @return	True if a character code is within any of the ranges, false otherwise.
	*/
	bool isWithinRange(CharCode code) const;

	static const CharCodeRange BasicLatin;
	static const CharCodeRange Latin1;
	static const CharCodeRange All;
};

/**
* Defines a font-face for Atlas initialization purposes.
*/
struct FontFaceDefinition
{
	Ogre::String filename;
	std::vector<Ogre::uint> sizes;
	CharCodeRange codeRange;
	short renderType;

	/**
	* Specifies the type of rendering that this font should use.
	* <ul>
	* <li>BetterContrast - Sharper text, more like Windows' font rendering.
	* <li>BetterShape - Smoother text, more like MacOSX's font rendering.
	* </ul>
	*/
	enum RenderType
	{
		BetterContrast,
		BetterShape
	};

	/**
	* Constructs a FontFaceDefinition
	*
	* @param	filename	The filename of the font-face.
	* @param	codeRange	The range of CharCode's to render for this font-face.
	* @param	renderType	Optional; the type of rendering to use for this font-face.
	*/
	FontFaceDefinition(const Ogre::String& filename, const CharCodeRange& codeRange = CharCodeRange::BasicLatin, short renderType = FontFaceDefinition::BetterContrast);

	/**
	* Adds a font-size (in px) to this font-face definition.
	*/
	void addSize(Ogre::uint fontSize);
};

struct ComputationRect
{
	int width, height;
	int x, y;
	int area;
	Ogre::Real weight;
	bool isPlaced;
	
	Ogre::String filename;
	bool isFontGlyph;
	Ogre::uint fontSize;
	CharCode charCode;

	Ogre::Image image;

	ComputationRect(const Ogre::String& texFilename, const Ogre::String& resourceGroup);
	ComputationRect(const Ogre::String& texName, unsigned char* buffer, int width, int height);
	ComputationRect(const Ogre::String& fontFilename, Ogre::uint fontSize, CharCode charCode, unsigned char* buffer, int width, int height);
};

typedef std::vector<ComputationRect*> ComputationVector;

typedef std::map<CharCode, GlyphInfo> GlyphMap;
typedef std::map<Ogre::uint, GlyphMap> FontSizeMap;
typedef std::map<Ogre::uint, FontMetrics> FontMetricsMap;

struct FontFace
{
	FontSizeMap fontSizes;
	FontMetricsMap fontMetrics;

	FontFace();

	FontFace(const FontFaceDefinition& definition, const Ogre::String& resourceGroup, ComputationVector& renderContext);
};

/**
* The work-horse of Canvas; Atlas is a programmatic texture-atlas that can hold textures and font-glyphs.
*/
class Atlas : public Ogre::ManualResourceLoader
{
	std::map<Ogre::String, FontFace> fontFaces;
	std::map<Ogre::String, TextureInfo> textures;
	std::pair<int, int> dimensions;
	int actualArea;
	Ogre::String materialName;
	bool supportsNPOT;

	void guessDimensions(ComputationVector& rectangles);
	void pack(ComputationVector& rectangles);
	void fill(ComputationVector& rectangles, int x1, int y1, int x2, int y2, int& count);
	void paint(const ComputationVector& rectangles);

public:
	/**
	* Constructs an Atlas.
	*
	* @param	textureFilenames	The filenames of the textures to load into this atlas.
	* @param	fonts	The fonts to load into this atlas.
	* @param	resourceGroup	The name of the resource group where the textures and fonts can be found.
	*/
	Atlas(const std::vector<Ogre::String>& textureFilenames, const std::vector<FontFaceDefinition>& fonts, const Ogre::String& resourceGroup);

	/**
	* Retrieve the dimensions of this atlas, in pixels.
	*/
	const std::pair<int, int>& getDimensions() const;

	/**
	* Retrieve the name of the internal material
	*/
	const Ogre::String& getMaterialName() const;

	/**
	* Retrieve info about a certain texture within this atlas.
	*
	* @note	If the filename is not found, the returned TextureInfo's member "isEmpty" will be true.
	*
	* @param	filename	The filename of the texture to look up.
	*/
	const TextureInfo& getTextureInfo(const Ogre::String& filename) const;

	/**
	* Retrieve the metrics for a certain font size.
	*
	* @param	fontFilename	The filename of the font to look up.
	* @param	fontSize	The font size to look up.
	*/
	const FontMetrics& getFontMetrics(const Ogre::String& fontFilename, Ogre::uint fontSize) const;

	/**
	* Retrieve the GlyphMap for a certain font size.
	*
	* @param	fontFilename	The filename of the font to look up.
	* @param	fontSize	The font size to look up.
	*/
	const GlyphMap& getGlyphMap(const Ogre::String& fontFilename, Ogre::uint fontSize) const;

	/**
	* Retrieve info about a certain font glyph within this atlas.
	*
	* @param	fontFilename	The filename of the font-face.
	* @param	fontSize	The size of the font.
	* @param	charCode	The CharCode to look up.
	*
	* @note	If the filename, size, or CharCode is not found, the returned GlyphInfo will contain a TextureInfo with
	*		the member "isEmpty" set to true.
	*/
	const GlyphInfo& getGlyphInfo(const Ogre::String& fontFilename, Ogre::uint fontSize, CharCode charCode) const;

	// Inherited from Ogre::ManualResourceLoader
	void loadResource(Ogre::Resource* resource);
};

#endif