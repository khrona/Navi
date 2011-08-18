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

#include "NaviUtilities.h"
#include "NaviManager.h"
#include <ctype.h>
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#include <direct.h>
#include <stdlib.h>
#endif

using namespace NaviLibrary;

std::string NaviUtilities::getCurrentWorkingDirectory()
{
	std::string workingDirectory = "";
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
	char currentPath[_MAX_PATH];
	getcwd(currentPath, _MAX_PATH);
	workingDirectory = currentPath;
#endif

	return workingDirectory + "\\";
}

std::string NaviUtilities::lowerString(std::string strToLower)
{	
	for(unsigned int i = 0; i < strToLower.length(); i++)
		strToLower[i] = tolower(strToLower[i]);

	return strToLower;
}

bool NaviUtilities::isNumeric(const std::string &numberString)
{
	std::istringstream converter(numberString);
	int test;

	return isPrefixed(numberString, "true") || isPrefixed(numberString, "false") ? true : !(converter >> test).fail();
}

std::wstring NaviUtilities::toWide(const std::string &stringToConvert)
{
	size_t size = mbstowcs(0, stringToConvert.c_str(), 0) + 1;
	wchar_t *temp = new wchar_t[size];
	mbstowcs(temp, stringToConvert.c_str(), size);
	std::wstring result(temp);
	delete[] temp;
	return result;
}

std::string NaviUtilities::toMultibyte(const std::wstring &wstringToConvert)
{
	size_t size = wcstombs(0, wstringToConvert.c_str(), 0) + 1;
	char *temp = new char[size];
	wcstombs(temp, wstringToConvert.c_str(), size);
	std::string result(temp);
	delete[] temp;
	return result;
}

void NaviUtilities::setLocale(const std::string &localeLanguage)
{
	setlocale(LC_CTYPE, localeLanguage.c_str());
}

int NaviUtilities::replaceAll(std::string &sourceStr, const std::string &replaceWhat, const std::string &replaceWith)
{
	int count = 0;

	for(size_t i = sourceStr.find(replaceWhat); i != std::string::npos; i = sourceStr.find(replaceWhat, i + replaceWith.length()))
	{
		sourceStr.erase(i, replaceWhat.length());
		sourceStr.insert(i, replaceWith);
		++count;
	}

	return count;
}

std::vector<std::string> NaviUtilities::split(const std::string &sourceStr, const std::string &delimiter, bool ignoreEmpty)
{
	std::vector<std::string> resultVector;

	size_t idxA = 0;
	size_t idxB = sourceStr.find(delimiter);
	std::string tempStr;
	bool done = false;

	while(!done)
	{
		if(idxB != std::string::npos)
		{
			tempStr = sourceStr.substr(idxA, idxB-idxA);
			idxA = idxB + delimiter.length();
			idxB = sourceStr.find(delimiter, idxA);
		}
		else
		{
			tempStr = sourceStr.substr(idxA);
			done = true;
		}

		if(!(ignoreEmpty && tempStr.empty()))
			resultVector.push_back(tempStr);
	}

	return resultVector;
}

std::map<std::string,std::string> NaviUtilities::splitToMap(const std::string &sourceStr, const std::string &pairDelimiter, const std::string &keyValueDelimiter, bool ignoreEmpty)
{
	std::map<std::string,std::string> resultMap;

	size_t idx = 0;
	std::string strA, strB;

	std::vector<std::string> tempVector = split(sourceStr, pairDelimiter);

	for(std::vector<std::string>::const_iterator i = tempVector.begin(); i != tempVector.end(); i++)
	{
		idx = i->find(keyValueDelimiter);
		if(idx != std::string::npos)
		{
			strA = i->substr(0, idx);
			strB = i->substr(idx + keyValueDelimiter.length());

			if(!(ignoreEmpty && (strA.empty() || strB.empty())) && !resultMap.count(strA))
				resultMap[strA] = strB;
		}
	}

	return resultMap;
}

std::string NaviUtilities::join(const std::vector<std::string> &sourceVector, const std::string &delimiter, bool ignoreEmpty)
{
	std::string result = "";

	for(std::vector<std::string>::const_iterator i = sourceVector.begin(); i != sourceVector.end(); i++)
		if(!(ignoreEmpty && i->empty()))
			result += result.length() ? delimiter + (*i) : (*i);

	return result;
}

std::string NaviUtilities::joinFromMap(const std::map<std::string,std::string> &sourceMap, const std::string &pairDelimiter, const std::string &keyValueDelimiter, bool ignoreEmpty)
{
	std::string result = "";

	for(std::map<std::string,std::string>::const_iterator i = sourceMap.begin(); i != sourceMap.end(); i++)
		if(!(ignoreEmpty && (i->first.empty() || i->second.empty())))
			result += result.length() ? pairDelimiter + i->first + keyValueDelimiter + i->second : i->first + keyValueDelimiter + i->second;

	return result;
}

bool NaviUtilities::hexStringToRGB(const std::string& hexString, unsigned char &R, unsigned char &G, unsigned char &B)
{
	if(hexString.length() == 7)
	{
		if(hexString.find("#") == 0 && hexString.substr(1).find_first_not_of("0123456789ABCDEFabcdef") == std::string::npos)
		{
			R = static_cast<unsigned char>(strtoul(hexString.substr(1, 2).c_str(), 0, 16));
			G = static_cast<unsigned char>(strtoul(hexString.substr(3, 2).c_str(), 0, 16));
			B = static_cast<unsigned char>(strtoul(hexString.substr(5, 2).c_str(), 0, 16));

			return true;
		}
	}

	return false;
}

/**
* The following is freely-available code by René Nyffenegger
* Obtained from: http://www.adp-gmbh.ch/cpp/common/base64.html
*/
std::string NaviUtilities::encodeBase64(const std::string &strToEncode)
{
	unsigned char const* bytes_to_encode = reinterpret_cast<const unsigned char*>(strToEncode.c_str());
	unsigned int in_len = (unsigned int)strToEncode.length();
	const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while(in_len--)
	{
		char_array_3[i++] = *(bytes_to_encode++);

		if(i == 3)
		{
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for(i = 0; i < 4; i++)
				ret += base64_chars[char_array_4[i]];

			i = 0;
		}
	}

	if(i)
	{
		for(j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while((i++ < 3))
			ret += '=';
	}

	return ret;
}

/**
* Code obtained from public domain: <http://www.codeproject.com/KB/string/wildcmp.aspx>
* Original author: Jack Handy <jakkhandy@hotmail.com>
*/
bool NaviUtilities::wildcardCompare(const std::string& wildcardTemplate, const std::string& source)
{
	const char* wild = wildcardTemplate.c_str();
	const char* string = source.c_str();
	const char *cp = NULL, *mp = NULL;

	while((*string) && (*wild != '*'))
	{
		if((*wild != *string) && (*wild != '?'))
			return 0;
		
		wild++;
		string++;
	}

	while(*string)
	{
		if(*wild == '*')
		{
			if(!*++wild)
				return 1;

			mp = wild;
			cp = string+1;
		}
		else if((*wild == *string) || (*wild == '?'))
		{
			wild++;
			string++;
		}
		else
		{
			wild = mp;
			string = cp++;
		}
	}

	while(*wild == '*')
		wild++;

	return !*wild;
}