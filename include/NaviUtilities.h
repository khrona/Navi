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

#ifndef __NaviUtilities_H__
#define __NaviUtilities_H__

#include "NaviPlatform.h"
#include <Awesomium/awesomium_capi.h>
#include "awesomium_capi_helpers.h"
#include <vector>
#include <map>
#include <string>
#include <iomanip>

namespace NaviLibrary
{
	/**
	* Various public utilities that are internally used by
	* by NaviLibrary but may be of some use to you.
	*/
	namespace NaviUtilities
	{
		/**
		* Gets the current working directory for the executable.
		*
		* @return	A string containing the current working directory.
		*/
		std::string _NaviExport getCurrentWorkingDirectory();

		/**
		* Converts a string into its lower-case equivalent.
		*
		* @param	strToLower	The standard string to convert to lower-case.
		*
		* @return	A standard string containing the equivalent lower-case representation.
		*/
		std::string _NaviExport lowerString(std::string strToLower);

		/**
		* Checks whether or not a string is prefixed with a certain prefix.
		*
		* @param	sourceString	The string to check.
		*
		* @param	prefix		The prefix to search for.
		*
		* @param	ignoreCase		Whether or not to ignore differences in case, default is true.
		*
		* @return	Whether or not a match was found.
		*/
		inline bool _NaviExport isPrefixed(const std::string &sourceString, const std::string &prefix, bool ignoreCase = true)
		{
			return ignoreCase ? lowerString(sourceString.substr(0, prefix.length())) == lowerString(prefix) 
				: sourceString.substr(0, prefix.length()) == prefix;
		}

		/**
		* Checks whether or not a string is 'numeric' in nature (begins with actual, parseable digits).
		*
		* @param	numberString	The string to check.
		* @note		Strings beginning with 'true'/'false' (regardless of case) are numeric.
		*
		* @return	Whether or not the string is numeric (can be successfully parsed into a number).
		*/
		bool _NaviExport isNumeric(const std::string &numberString);

		/**
		* Converts a Number (int, float, double, bool, etc.) to a String.
		*
		* @param	number	The number (usually of type int, float, double, bool, etc.) to convert to a String.
		*
		* @return	If the conversion succeeds, returns the string equivalent of the number, otherwise an empty string.
		*/
		template<class NumberType>
		inline std::string _NaviExport numberToString(const NumberType &number)
		{
			std::ostringstream converter;

			return (converter << std::setprecision(17) << number).fail() ? "" : converter.str();
		}

		/**
		* Converts a String to a Number.
		*
		* @param	<NumberType>	The NumberType (int, float, bool, double, etc.) to convert to.
		*
		* @param	numberString	A string containing a valid numeric sequence (can check using isNumeric()).
		* @note		Strings beginning with 'true'/'false' (regardless of case) are numeric and will be converted accordingly.
		*
		* @return	If conversion succeeds, returns a number of type 'NumberType', otherwise returns a '0' equivalent.
		*/
		template<class NumberType>
		inline NumberType _NaviExport toNumber(const std::string &numberString)
		{
			if(isPrefixed(numberString, "true")) return 1;
			else if(isPrefixed(numberString, "false")) return 0;

			std::istringstream converter(numberString);
			
			if(typeid(NumberType)==typeid(bool))
			{
				int result;
				return (converter >> result).fail() ? false : !!result;
			}

			NumberType result;
			return (converter >> result).fail() ? 0 : result;
		}

		/**
		* Converts a multibyte string (standard string) to a wide string.
		*
		* @param	stringToConvert		The multibyte (standard) string to convert.
		*
		* @return	The wide-equivalent of the passed string.
		*/
		std::wstring _NaviExport toWide(const std::string &stringToConvert);

		/**
		* Converts a wide string to a multibyte string (standard string), based on the current locale.
		*
		* @param	wstringToConvert	The wide string to convert.
		*
		* @return	The multibyte-equivalent of the passed string, based on the current locale.
		*/
		std::string _NaviExport toMultibyte(const std::wstring &wstringToConvert);

		/**
		* Sets the current locale, used for 'toMultibyte()'. If you never call this, the default is usually "English". 
		*
		* @param	localeLanguage	The name of the locale language to set. An empty string sets this to the current locale of the OS.
		*/
		void _NaviExport setLocale(const std::string &localeLanguage = "");

		/**
		* Replaces all instances of 'replaceWhat' with 'replaceWith' inside a source string.
		*
		* @param	sourceStr	The string to do this to.
		*
		* @param	replaceWhat		What to be replaced.
		*
		* @param	replaceWith		All occurrences of 'replaceWhat' will be replaced with this.
		*
		* @return	The number of instances replaced within 'sourceStr'.
		*/
		int _NaviExport replaceAll(std::string &sourceStr, const std::string &replaceWhat, const std::string &replaceWith);

		/**
		* Splits a string up into a series of tokens (contained within a string vector), delimited by a certain string.
		*
		* @param	sourceStr	The string to split up.
		*
		* @param	delimiter	What to delimit the source string by.
		*
		* @param	ignoreEmpty		Whether or not to ignore empty tokens. (usually created by 2 or more immediately adjacent delimiters)
		*
		* @return	A string vector containing a series of ordered tokens.
		*/
		std::vector<std::string> _NaviExport split(const std::string &sourceStr, const std::string &delimiter, bool ignoreEmpty = true);

		/**
		* A more advanced form of splitting, parses a string into a string map. Exceptionally useful for use with Query Strings.
		*
		* @param	sourceStr	The string to parse.
		*
		* @param	pairDelimiter	What to delimit pairs by.
		*
		* @param	keyValueDelimiter	What to delimit key-values by.
		*
		* @param	ignoreEmpty		Whether or not to ignore empty values. Empty pairs will always be ignored.
		*
		* @return	A string map containing the parsed equivalent of the passed string.
		*
		* @note
		*	For example:
		*	\code
		*	std::string myQueryString = "name=Bob&sex=none&color=purple";
		*	std::map<std::string,std::string> myMap = splitToMap(myQueryString, "&", "=");
		*	std::string myColor = myMap["color"]; // myColor is now 'purple' 
		*	\endcode
		*/
		std::map<std::string,std::string> _NaviExport splitToMap(const std::string &sourceStr, const std::string &pairDelimiter, const std::string &keyValueDelimiter, bool ignoreEmpty = true);

		/**
		* Joins a string vector into a single string. (Effectively does the inverse of NaviUtilities::split)
		*
		* @param	sourceVector	The string vector to join.
		*
		* @param	delimiter	What to delimit each token by.
		*
		* @param	ignoreEmpty		Whether or not to ignore empty strings within the string vector.
		*/
		std::string _NaviExport join(const std::vector<std::string> &sourceVector, const std::string &delimiter, bool ignoreEmpty = true);

		/**
		* Joins a string map into a single string. (Effectively does the inverse of NaviUtilities::splitToMap)
		*
		* @param	sourceMap	The string map to join.
		*
		* @param	pairDelimiter	What to delimit each pair by.
		*
		* @param	keyValueDelimiter	What to delimit each key-value by.
		*
		* @param	ignoreEmpty		Whether or not to ignore empty string values within the string map.
		*/
		std::string _NaviExport joinFromMap(const std::map<std::string,std::string> &sourceMap, const std::string &pairDelimiter, const std::string &keyValueDelimiter, bool ignoreEmpty = true);

		/**
		* This is a simple way to quickly make inline JSValue vectors (which is really
		* useful when declaring arguments to pass to Navi::evaluateJS).
		*
		* Syntax is: \code JSArgs(x, x, x, x,...) \endcode
		*
		* @note
		*	For example:
		*	\code
		*	// Before:
		*	OSM::JSArguments myArgs;
		*	myArgs.push_back("hello there");
		*	myArgs.push_back(3.1416);
		*	myArgs.push_back(1337);
		*	myNavi->evaluateJS("displayInfo(?, ?, ?)", myArgs);
		*
		*	// After:
		*	myNavi->evaluateJS("displayInfo(?, ?, ?)", JSArgs("hello there", 3.1416, 1337));
		*	\endcode
		*/
		class _NaviExport JSArgs
		{
			OSM::JSArguments args;
		public:
			JSArgs() { }

			explicit JSArgs(const OSM::JSValue &firstArg) : args(1, firstArg) { }

			JSArgs(const OSM::JSValue& a1, const OSM::JSValue& a2)
			{
				args.push_back(a1);
				args.push_back(a2);
			}

			JSArgs(const OSM::JSValue& a1, const OSM::JSValue& a2, const OSM::JSValue& a3)
			{
				args.push_back(a1);
				args.push_back(a2);
				args.push_back(a3);
			}

			JSArgs(const OSM::JSValue& a1, const OSM::JSValue& a2, const OSM::JSValue& a3,
				const OSM::JSValue& a4)
			{
				args.push_back(a1);
				args.push_back(a2);
				args.push_back(a3);
				args.push_back(a4);
			}

			JSArgs(const OSM::JSValue& a1, const OSM::JSValue& a2, const OSM::JSValue& a3,
				const OSM::JSValue& a4, const OSM::JSValue& a5)
			{
				args.push_back(a1);
				args.push_back(a2);
				args.push_back(a3);
				args.push_back(a4);
				args.push_back(a5);
			}

			JSArgs(const OSM::JSValue& a1, const OSM::JSValue& a2, const OSM::JSValue& a3,
				const OSM::JSValue& a4, const OSM::JSValue& a5, const OSM::JSValue& a6)
			{
				args.push_back(a1);
				args.push_back(a2);
				args.push_back(a3);
				args.push_back(a4);
				args.push_back(a5);
				args.push_back(a6);
			}

			JSArgs(const OSM::JSValue& a1, const OSM::JSValue& a2, const OSM::JSValue& a3,
				const OSM::JSValue& a4, const OSM::JSValue& a5, const OSM::JSValue& a6, 
				const OSM::JSValue& a7)
			{
				args.push_back(a1);
				args.push_back(a2);
				args.push_back(a3);
				args.push_back(a4);
				args.push_back(a5);
				args.push_back(a6);
				args.push_back(a7);
			}

			JSArgs(const OSM::JSValue& a1, const OSM::JSValue& a2, const OSM::JSValue& a3,
				const OSM::JSValue& a4, const OSM::JSValue& a5, const OSM::JSValue& a6, 
				const OSM::JSValue& a7, const OSM::JSValue& a8)
			{
				args.push_back(a1);
				args.push_back(a2);
				args.push_back(a3);
				args.push_back(a4);
				args.push_back(a5);
				args.push_back(a6);
				args.push_back(a7);
				args.push_back(a8);
			}

			JSArgs(const OSM::JSValue& a1, const OSM::JSValue& a2, const OSM::JSValue& a3,
				const OSM::JSValue& a4, const OSM::JSValue& a5, const OSM::JSValue& a6, 
				const OSM::JSValue& a7, const OSM::JSValue& a8, const OSM::JSValue& a9)
			{
				args.push_back(a1);
				args.push_back(a2);
				args.push_back(a3);
				args.push_back(a4);
				args.push_back(a5);
				args.push_back(a6);
				args.push_back(a7);
				args.push_back(a8);
				args.push_back(a9);
			}

			operator OSM::JSArguments() const
			{
				return args;
			}
		};

		/**
		* Converts a Hex Color String to R, G, B values.
		*
		* @param	hexString	A hex color string in format: "#XXXXXX"
		*
		* @param[out]	R	The unsigned char to store the Red value in.
		* @param[out]	G	The unsigned char to store the Green value in.
		* @param[out]	B	The unsigned char to store the Blue value in.
		*
		* @return	Returns whether or not the conversion was successful.
		*/
		bool _NaviExport hexStringToRGB(const std::string& hexString, unsigned char &R, unsigned char &G, unsigned char &B);

		/** 
		* Encodes a string into Base64.
		*
		* @param	strToEncode		The string to encode.
		*
		* @return	The Base64-encoded representation of the passed string.
		*/
		std::string _NaviExport encodeBase64(const std::string &strToEncode);

		/**
		* Ensures that a number (input) is within certain limits.
		*
		* @param	input	The number that will be limited.
		* @param	min		The minimum limit.
		* @param	max		The maximum limit.
		*/
		template<class NumberType>
		inline void _NaviExport limit(NumberType &input, NumberType min, NumberType max)
		{
			if(input < min)
				input = min;
			else if(input > max)
				input = max;
		}

		bool wildcardCompare(const std::string& wildcardTemplate, const std::string& source);
	}
}

#endif