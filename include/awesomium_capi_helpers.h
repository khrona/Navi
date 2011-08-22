#ifndef __AWESOMIUM_CAPI_HELPERS_H__
#define __AWESOMIUM_CAPI_HELPERS_H__

#include <Awesomium/awesomium_capi.h>
#include "NaviPlatform.h"
#include <vector>
#include <string>
#include <map>

namespace OSM {

// This class wraps awe_string with a friendly STL interface
class _NaviExport String
{
	awe_string* instance;
	bool ownsInstance;
public:
	/// Creates an empty string.
	String();

	/// Creates an ASCII string.
	String(const char* value);
	
	/// Creates a UTF-8 string.
	String(const std::string& value);
	
	/// Creates a wide string.
	String(const wchar_t* value);
	
	/// Creates a wide string.
	String(const std::wstring& value);

	/// Creates a copy of an existing string
	String(const OSM::String& original);

	/// Wraps an existing string instance, will automatically
	/// call awe_string_destroy if you set 'ownsInstance' to true
	String(awe_string* instance, bool ownsInstance);

	~String();

	/// Get the length of the string.
	size_t length() const;

	/// Whether or not this string is empty.
	bool empty() const;

	/// Gets a copy of string as a UTF-8 string
	std::string str() const;

	/// Gets a copy of string as a wide string
	std::wstring wstr() const;

	/// Get this string as an awe_string (C API) instance.
	const awe_string* getInstance() const;
};

// Use this macro in your code whenever you need to pass a
// string literal to a function that takes an awe_string.
#define OSM_STR(x) OSM::String(x).getInstance()

// Use this macro in your code whenever you need to pass an
// empty string to a function that takes an awe_string.
#define OSM_EMPTY() awe_string_empty()

// This class wraps awe_jsvalue with a friendly STL interface.
class _NaviExport JSValue
{
	awe_jsvalue* instance;
	bool ownsInstance;
public:
	
	typedef std::map<std::wstring, JSValue> Object;
	typedef std::vector<JSValue> Array;
	
	/// Creates a null JSValue.
	JSValue();
	
	/// Creates a JSValue initialized with a boolean.
	JSValue(bool value);
	
	/// Creates a JSValue initialized with an integer.
	JSValue(int value);
	
	/// Creates a JSValue initialized with a double.
	JSValue(double value);
	
	/// Creates a JSValue initialized with a string.
	JSValue(const OSM::String& value);

	/// Creates a JSValue initialized with a string.
	JSValue(const char* value);
	
	/// Creates a JSValue initialized with a string.
	JSValue(const std::string& value);

	/// Creates a JSValue initialized with a string.
	JSValue(const wchar_t* value);
	
	/// Creates a JSValue initialized with a string.
	JSValue(const std::wstring& value);
	
	/// Creates a JSValue initialized with an object.
	JSValue(const Object& value);
	
	/// Creates a JSValue initialized with an array.
	JSValue(const Array& value);
	
	/// Copy constructor (makes a deep copy)
	JSValue(const JSValue& original);

	/// Wraps an existing jsvalue instance, will automatically
	/// call awe_jsvalue_destroy if you set 'ownsInstance' to true
	JSValue(awe_jsvalue* instance, bool ownsInstance);
	
	~JSValue();
	
	/// Returns whether or not this JSValue is a boolean.
	bool isBoolean() const;
	
	/// Returns whether or not this JSValue is an integer.
	bool isInteger() const;
	
	/// Returns whether or not this JSValue is a double.
	bool isDouble() const;
	
	/// Returns whether or not this JSValue is a number (integer or double).
	bool isNumber() const;
	
	/// Returns whether or not this JSValue is a string.
	bool isString() const;
	
	/// Returns whether or not this JSValue is an array.
	bool isArray() const;
	
	/// Returns whether or not this JSValue is an object.
	bool isObject() const;
	
	/// Returns whether or not this JSValue is null.
	bool isNull() const;
	
	/// Returns this JSValue as a string.
	OSM::String toString() const;
	
	/// Returns this JSValue as an integer (converting if necessary).
	int toInteger() const;
	
	/// Returns this JSValue as a double (converting if necessary).
	double toDouble() const;
	
	/// Returns this JSValue as a boolean (converting if necessary).
	bool toBoolean() const;
	
	/// Gets a constant reference to this JSValue's array value (will assert 
	/// if not an array type)
	Array getArray() const;
	
	/// Gets a constant reference to this JSValue's object value (will 
	/// assert if not an object type)
	Object getObject() const;

	awe_jsvalue* getInstance() const;
};

// Use this function to convert a JSArray (C API instance) to a C++ JSValue::Array
JSValue::Array _NaviExport ConvertJSArray(const awe_jsarray* value);

typedef JSValue::Array JSArguments;

// Inherit from this class to handle webview callbacks. Please see
// WebViewEventHandler for information on binding callbacks to a listener.
class _NaviExport WebViewListener
{
public:
	virtual void onBeginNavigation(awe_webview* caller, 
								   const OSM::String& url, 
								   const OSM::String& frameName) = 0;
	
	virtual void onBeginLoading(awe_webview* caller, 
								const OSM::String& url, 
								const OSM::String& frameName, 
								int statusCode, 
								const OSM::String& mimeType) = 0;
	
	virtual void onFinishLoading(awe_webview* caller) = 0;

	virtual void onJSCallback(awe_webview* caller, 
							const OSM::String& objectName, 
							const OSM::String& callbackName, 
							const OSM::JSArguments& args) = 0;
	
	virtual void onReceiveTitle(awe_webview* caller, 
								const OSM::String& title, 
								const OSM::String& frameName) = 0;
	
	virtual void onChangeTooltip(awe_webview* caller, 
								 const OSM::String& tooltip) = 0;
	
	virtual void onChangeCursor(awe_webview* caller, 
								awe_cursor_type cursor) = 0;
	
	virtual void onChangeKeyboardFocus(awe_webview* caller, 
									   bool isFocused) = 0;

	virtual void onChangeTargetURL(awe_webview* caller, 
								   const OSM::String& url) = 0;
	
	virtual void onOpenExternalLink(awe_webview* caller, 
									const OSM::String& url, 
									const OSM::String& source) = 0;

	virtual void onRequestDownload(awe_webview* caller,
									const OSM::String& url) = 0;
	
	virtual void onWebViewCrashed(awe_webview* caller) = 0;
	
	virtual void onPluginCrashed(awe_webview* caller, 
								 const OSM::String& pluginName) = 0;
	
	virtual void onRequestMove(awe_webview* caller, 
							   int x, int y) = 0;
	
	virtual void onGetPageContents(awe_webview* caller, 
								   const OSM::String& url, 
								   const OSM::String& contents) = 0;
	
	virtual void onDOMReady(awe_webview* caller) = 0;

	virtual void onRequestFileChooser(awe_webview* caller,
									  bool selectMultipleFiles,
									  const OSM::String& title,
									  const OSM::String& defaultPath) = 0;

	virtual void onGetScrollData(awe_webview* caller,
								 int contentWidth,
								 int contentHeight,
								 int preferredWidth,
								 int scrollX,
								 int scrollY) = 0;

	virtual void onJSConsoleMessage(awe_webview* caller,
									const OSM::String& message,
									int lineNumber,
									const OSM::String& source) = 0;

	virtual void onGetFindResults(awe_webview* caller,
                                  int requestID,
                                  int numMatches,
                                  awe_rect selection,
                                  int curMatch,
                                  bool finalUpdate) = 0;

	virtual void onUpdateIME(awe_webview* caller,
                             awe_ime_state imeState,
                             awe_rect caretRect) = 0;
};

/**
 * Use this singleton to bind WebView callbacks directly to a
 * class inherited from WebViewListener
 *
 * 1. Make your class inherit from WebViewListener.
 * 2. Call this to bind callbacks: 
 *        WebViewEventHelper::instance().addListener(webView, myListenerClass);
 * 3. Call this to unbind callbacks:
 *        WebViewEventHelper::instance().removeListener(webView);
 */
class _NaviExport WebViewEventHelper
{
	WebViewEventHelper();
	~WebViewEventHelper();
	std::map<awe_webview*, WebViewListener*> listenerMap;
public:
	static WebViewEventHelper& instance();

	void addListener(awe_webview* webView, WebViewListener* listener);
	void removeListener(awe_webview* webView);

	WebViewListener* getListener(awe_webview* webView);
};

}

#endif