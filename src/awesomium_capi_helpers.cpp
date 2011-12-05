#include "awesomium_capi_helpers.h"

using namespace OSM;

awe_jsvalue* CreateJSValueFromArray(const JSValue::Array& value)
{
	awe_jsvalue* instance = 0;

	if(value.empty())
	{
		awe_jsarray* jsarray = awe_jsarray_create(0, 0);
		instance = awe_jsvalue_create_array_value(jsarray);
		awe_jsarray_destroy(jsarray);
	}
	else
	{
		awe_jsvalue** valArray = new awe_jsvalue*[value.size()];

		size_t idx = 0;
		for(JSValue::Array::const_iterator i = value.begin(); i != value.end(); i++)
		{
			valArray[idx++] = i->getInstance();
		}

		awe_jsarray* jsarray = awe_jsarray_create((const awe_jsvalue**)valArray, value.size());
		instance = awe_jsvalue_create_array_value(jsarray);
		awe_jsarray_destroy(jsarray);
	}

	return instance;
}

awe_jsvalue* CreateJSValueFromObject(const JSValue::Object& value)
{
	awe_jsvalue* instance = 0;

	awe_jsobject* jsobject = awe_jsobject_create();
	for(JSValue::Object::const_iterator i = value.begin(); i != value.end(); i++)
	{
		String propName(i->first);

		awe_jsobject_set_property(jsobject, propName.getInstance(), i->second.getInstance());
	}

	instance = awe_jsvalue_create_object_value(jsobject);

	awe_jsobject_destroy(jsobject);

	return instance;
}

awe_jsvalue* CreateJSValueFromCopy(const JSValue& original)
{
	awe_jsvalue* instance = 0;
	awe_jsvalue_type srcType = awe_jsvalue_get_type(original.getInstance());

	switch(srcType)
	{
	case JSVALUE_TYPE_NULL:
		{
			instance = awe_jsvalue_create_null_value();
			break;
		}
	case JSVALUE_TYPE_BOOLEAN:
		{
			instance = awe_jsvalue_create_bool_value(original.toBoolean());
			break;
		}
	case JSVALUE_TYPE_INTEGER:
		{
			instance = awe_jsvalue_create_integer_value(original.toInteger());
			break;
		}
	case JSVALUE_TYPE_DOUBLE:
		{
			instance = awe_jsvalue_create_double_value(original.toDouble());
			break;
		}
	case JSVALUE_TYPE_STRING:
		{
			OSM::String srcString = original.toString();
			instance = awe_jsvalue_create_string_value(srcString.getInstance());
			break;
		}
	case JSVALUE_TYPE_ARRAY:
		{
			instance = CreateJSValueFromArray(original.getArray());
			break;
		}
	case JSVALUE_TYPE_OBJECT:
		{
			instance = CreateJSValueFromObject(original.getObject());
			break;
		}
	default:
		{
			instance = awe_jsvalue_create_null_value();
		}
	};

	return instance;
}

/// Creates an empty string.
String::String()
{
	instance = awe_string_create_from_ascii("", 0);
	ownsInstance = true;
}

/// Creates an ASCII string.
String::String(const char* value)
{
	instance = awe_string_create_from_ascii(value, strlen(value));
	ownsInstance = true;
}

/// Creates an ASCII string.
String::String(const std::string& value)
{
	instance = awe_string_create_from_ascii(value.c_str(), value.length());
	ownsInstance = true;
}

/// Creates a wide string.
String::String(const wchar_t* value)
{
	instance = awe_string_create_from_wide(value, wcslen(value));
	ownsInstance = true;
}

/// Creates a wide string.
String::String(const std::wstring& value)
{
	instance = awe_string_create_from_wide(value.c_str(), value.length());
	ownsInstance = true;
}

String::String(const String& original)
{
	if(!original.empty())
	{
		instance = awe_string_create_from_utf16(
			awe_string_get_utf16(original.getInstance()),
			original.length());
	}
	else
	{
		instance = 0;
	}
	ownsInstance = true;
}

/// Wraps an existing string instance
String::String(awe_string* instance, bool ownsInstance)
{
	this->instance = instance;
	this->ownsInstance = ownsInstance;
}

String::~String()
{
	if(instance && ownsInstance)
		awe_string_destroy(instance);
}

size_t String::length() const
{
	if(instance)
		return awe_string_get_length(instance);

	return 0;
}

bool String::empty() const
{
	if(instance == 0)
		return true;

	return length() == 0;
}

std::string String::str() const
{
	if(empty())
		return std::string();

	int bufSize = awe_string_to_utf8(instance, 0, 0);

	if(bufSize > 0)
	{
		char* stringBuffer = new char[bufSize];
		awe_string_to_utf8(instance, stringBuffer, bufSize);

		std::string result;
		result.assign(stringBuffer, bufSize);

		delete[] stringBuffer;

		return result;
	}
	
	return std::string();
}

std::wstring String::wstr() const
{
	if(empty())
		return std::wstring();

	int bufSize = awe_string_to_wide(instance, 0, 0);

	if(bufSize > 0)
	{
		wchar_t* stringBuffer = new wchar_t[bufSize];
		awe_string_to_wide(instance, stringBuffer, bufSize);

		std::wstring result;
		result.assign(stringBuffer, bufSize);

		delete[] stringBuffer;

		return result;
	}
	
	return std::wstring();
}

const awe_string* String::getInstance() const
{
	return instance;
}

/// Creates a null JSValue.
JSValue::JSValue()
{
	instance = awe_jsvalue_create_null_value();
	ownsInstance = true;
}
	
/// Creates a JSValue initialized with a boolean.
JSValue::JSValue(bool value)
{
	instance = awe_jsvalue_create_bool_value(value);
	ownsInstance = true;
}

/// Creates a JSValue initialized with an integer.
JSValue::JSValue(int value)
{
	instance = awe_jsvalue_create_integer_value(value);
	ownsInstance = true;
}

/// Creates a JSValue initialized with a double.
JSValue::JSValue(double value)
{
	instance = awe_jsvalue_create_double_value(value);
	ownsInstance = true;
}

/// Creates a JSValue initialized with an ASCII string.
JSValue::JSValue(const String& value)
{
	instance = awe_jsvalue_create_string_value(value.getInstance());
	ownsInstance = true;
}

/// Creates a JSValue initialized with a string.
JSValue::JSValue(const char* value)
{
	instance = awe_jsvalue_create_string_value(OSM_STR(value));
	ownsInstance = true;
}

/// Creates a JSValue initialized with a string.
JSValue::JSValue(const std::string& value)
{
	instance = awe_jsvalue_create_string_value(OSM_STR(value));
	ownsInstance = true;
}

/// Creates a JSValue initialized with a string.
JSValue::JSValue(const wchar_t* value)
{
	instance = awe_jsvalue_create_string_value(OSM_STR(value));
	ownsInstance = true;
}

/// Creates a JSValue initialized with a string.
JSValue::JSValue(const std::wstring& value)
{
	instance = awe_jsvalue_create_string_value(OSM_STR(value));
	ownsInstance = true;
}

/// Creates a JSValue initialized with an object.
JSValue::JSValue(const Object& value)
{
	instance = CreateJSValueFromObject(value);
	ownsInstance = true;
}

/// Creates a JSValue initialized with an array.
JSValue::JSValue(const Array& value)
{
	instance = CreateJSValueFromArray(value);
	ownsInstance = true;
}

JSValue::JSValue(const JSValue& original)
{
	instance = CreateJSValueFromCopy(original);
	ownsInstance = true;
}

JSValue::JSValue(awe_jsvalue* instance, bool ownsInstance)
{
	this->instance = instance;
	this->ownsInstance = ownsInstance;
}

JSValue::~JSValue()
{
	if(instance && ownsInstance)
		awe_jsvalue_destroy(instance);
}

JSValue& JSValue::operator=(const JSValue& rhs)
{
	if(instance && ownsInstance)
		awe_jsvalue_destroy(instance);

	instance = CreateJSValueFromCopy(rhs);
	ownsInstance = true;
	return *this;
}

/// Returns whether or not this JSValue is a boolean.
bool JSValue::isBoolean() const
{
	return awe_jsvalue_get_type(instance) == JSVALUE_TYPE_BOOLEAN;
}

/// Returns whether or not this JSValue is an integer.
bool JSValue::isInteger() const
{
	return awe_jsvalue_get_type(instance) == JSVALUE_TYPE_INTEGER;
}

/// Returns whether or not this JSValue is a double.
bool JSValue::isDouble() const
{
	return awe_jsvalue_get_type(instance) == JSVALUE_TYPE_DOUBLE;
}

/// Returns whether or not this JSValue is a number (integer or double).
bool JSValue::isNumber() const
{
	return isInteger() || isDouble();
}

/// Returns whether or not this JSValue is a string.
bool JSValue::isString() const
{
	return awe_jsvalue_get_type(instance) == JSVALUE_TYPE_STRING;
}

/// Returns whether or not this JSValue is an array.
bool JSValue::isArray() const
{
	return awe_jsvalue_get_type(instance) == JSVALUE_TYPE_ARRAY;
}

/// Returns whether or not this JSValue is an object.
bool JSValue::isObject() const
{
	return awe_jsvalue_get_type(instance) == JSVALUE_TYPE_OBJECT;
}

/// Returns whether or not this JSValue is null.
bool JSValue::isNull() const
{
	return awe_jsvalue_get_type(instance) == JSVALUE_TYPE_NULL;
}

/// Returns this JSValue as a string.
String JSValue::toString() const
{
	return String(awe_jsvalue_to_string(instance), true);
}

/// Returns this JSValue as an integer (converting if necessary).
int JSValue::toInteger() const
{
	return awe_jsvalue_to_integer(instance);
}

/// Returns this JSValue as a double (converting if necessary).
double JSValue::toDouble() const
{
	return awe_jsvalue_to_double(instance);
}

/// Returns this JSValue as a boolean (converting if necessary).
bool JSValue::toBoolean() const
{
	return awe_jsvalue_to_boolean(instance);
}

/// Gets a constant reference to this JSValue's array value (will assert 
/// if not an array type)
JSValue::Array JSValue::getArray() const
{
	return ConvertJSArray(awe_jsvalue_get_array(instance));
}

/// Gets a constant reference to this JSValue's object value (will 
/// assert if not an object type)
JSValue::Object JSValue::getObject() const
{
	JSValue::Object result;

	const awe_jsobject* jsobject = awe_jsvalue_get_object(instance);

	awe_jsarray* keys = awe_jsobject_get_keys(const_cast<awe_jsobject*>(jsobject));

	size_t len = awe_jsarray_get_size(keys);

	for(size_t i = 0; i < len; i++)
	{
		const awe_jsvalue* keyVal = awe_jsarray_get_element(keys, i);
		awe_string* keyStr = awe_jsvalue_to_string(keyVal);
		const awe_jsvalue* propVal = awe_jsobject_get_property(jsobject, keyStr);

		String keyString(keyStr, true);

		result[keyString.wstr()] = JSValue(const_cast<awe_jsvalue*>(propVal), false);
	}

	awe_jsarray_destroy(keys);

	return result;
}

awe_jsvalue* JSValue::getInstance() const
{
	return instance;
}

JSValue::Array OSM::ConvertJSArray(const awe_jsarray* value)
{
	JSValue::Array result;

	size_t arraySize = awe_jsarray_get_size(value);

	for(size_t i = 0; i < arraySize; i++)
		result.push_back(JSValue(const_cast<awe_jsvalue*>(awe_jsarray_get_element(value, i)), false));

	return result;
}

//////////////////////////////////////
//////////////////////////////////////

#define GET_LISTENER() 	WebViewListener* listener = \
	WebViewEventHelper::instance().getListener(caller); \
	if(!listener) \
		return

#define STR(x) OSM::String(const_cast<awe_string*>(x), false)

void handle_callback_begin_navigation(awe_webview* caller,
							                 const awe_string* url,
							                 const awe_string* frame_name)
{
	GET_LISTENER();

	listener->onBeginNavigation(caller, STR(url), STR(frame_name));
}

void handle_callback_begin_loading(awe_webview* caller,
							                 const awe_string* url,
											 const awe_string* frame_name,
											 int status_code,
											 const awe_string* mime_type)
{
	GET_LISTENER();

	listener->onBeginLoading(caller, STR(url), STR(frame_name), status_code, STR(mime_type));
}

void handle_callback_finish_loading(awe_webview* caller)
{
	GET_LISTENER();

	listener->onFinishLoading(caller);
}

void handle_callback_js_callback(awe_webview* caller,
							                 const awe_string* object_name,
											 const awe_string* callback_name,
											 const awe_jsarray* arguments)
{
	GET_LISTENER();

	listener->onJSCallback(caller, STR(object_name), STR(callback_name), ConvertJSArray(arguments));
}

void handle_callback_receive_title(awe_webview* caller,
							                 const awe_string* title,
											 const awe_string* frame_name)
{
	GET_LISTENER();

	listener->onReceiveTitle(caller, STR(title), STR(frame_name));
}

void handle_callback_change_tooltip(awe_webview* caller,
							                 const awe_string* tooltip)
{
	GET_LISTENER();

	listener->onChangeTooltip(caller, STR(tooltip));
}

void handle_callback_change_cursor(awe_webview* caller,
							                 awe_cursor_type cursor)
{
	GET_LISTENER();

	listener->onChangeCursor(caller, cursor);
}

void handle_callback_change_keyboard_focus(awe_webview* caller,
							                 bool is_focused)
{
	GET_LISTENER();

	listener->onChangeKeyboardFocus(caller, is_focused);
}

void handle_callback_change_target_url(awe_webview* caller,
							                 const awe_string* url)
{
	GET_LISTENER();

	listener->onChangeTargetURL(caller, STR(url));
}

void handle_callback_open_external_link(awe_webview* caller,
							                 const awe_string* url,
											 const awe_string* source)
{
	GET_LISTENER();

	listener->onOpenExternalLink(caller, STR(url), STR(source));
}

void handle_callback_request_download(awe_webview* caller,
							                 const awe_string* download)
{
	GET_LISTENER();

	listener->onRequestDownload(caller, STR(download));
}

void handle_callback_web_view_crashed(awe_webview* caller)
{
	GET_LISTENER();

	listener->onWebViewCrashed(caller);
}

void handle_callback_plugin_crashed(awe_webview* caller,
							                 const awe_string* plugin_name)
{
	GET_LISTENER();

	listener->onPluginCrashed(caller, STR(plugin_name));
}

void handle_callback_request_move(awe_webview* caller,
							                 int x,
											 int y)
{
	GET_LISTENER();

	listener->onRequestMove(caller, x, y);
}

void handle_callback_get_page_contents(awe_webview* caller,
							                 const awe_string* url,
											 const awe_string* contents)
{
	GET_LISTENER();

	listener->onGetPageContents(caller, STR(url), STR(contents));
}

void handle_callback_dom_ready(awe_webview* caller)
{
	GET_LISTENER();

	listener->onDOMReady(caller);
}

void handle_callback_request_file_chooser(awe_webview* caller,
							                 bool select_multiple_files,
											 const awe_string* title,
											 const awe_string* default_path)
{
	GET_LISTENER();

	listener->onRequestFileChooser(caller, select_multiple_files, STR(title), STR(default_path));
}

void handle_callback_get_scroll_data(awe_webview* caller,
											 int contentWidth,
                            				 int contentHeight,
                            				 int preferredWidth,
                            				 int scrollX,
                            				 int scrollY)
{
	GET_LISTENER();

	listener->onGetScrollData(caller, contentWidth, contentHeight, preferredWidth, scrollX, scrollY);
}
	
void handle_callback_js_console_message(awe_webview* caller,
										   const awe_string* message,
										   int line_number,
										   const awe_string* source)
{
	GET_LISTENER();

	listener->onJSConsoleMessage(caller, STR(message), line_number, STR(source));
}

void handle_callback_get_find_results(awe_webview* caller,
                                           int request_id,
                                           int num_matches,
                                           awe_rect selection,
                                           int cur_match,
                                           bool finalUpdate)
{
	GET_LISTENER();

	listener->onGetFindResults(caller, request_id, num_matches, selection, cur_match, finalUpdate);
}

void handle_callback_update_ime(awe_webview* caller,
                                           awe_ime_state state,
                                           awe_rect caret_rect)
{
	GET_LISTENER();

	listener->onUpdateIME(caller, state, caret_rect);
}

WebViewEventHelper::WebViewEventHelper()
{
}

WebViewEventHelper::~WebViewEventHelper()
{
}

WebViewEventHelper& WebViewEventHelper::instance()
{
	static WebViewEventHelper i;
	return i;
}

#define BIND(xxx) awe_webview_set_callback_ ## xxx (webView, handle_callback_ ## xxx )
#define UNBIND(xxx) awe_webview_set_callback_ ## xxx (webView, 0)

void WebViewEventHelper::addListener(awe_webview* webView, WebViewListener* listener)
{
	removeListener(webView);

	listenerMap[webView] = listener;

	BIND(begin_navigation);
	BIND(begin_loading);
	BIND(finish_loading);
	BIND(js_callback);
	BIND(receive_title);
	BIND(change_tooltip);
	BIND(change_cursor);
	BIND(change_keyboard_focus);
	BIND(change_target_url);
	BIND(open_external_link);
	BIND(request_download);
	BIND(web_view_crashed);
	BIND(plugin_crashed);
	BIND(request_move);
	BIND(get_page_contents);
	BIND(dom_ready);
	BIND(request_file_chooser);
	BIND(get_scroll_data);
	BIND(js_console_message);
	BIND(get_find_results);
	BIND(update_ime);
}

void WebViewEventHelper::removeListener(awe_webview* webView)
{
	std::map<awe_webview*, WebViewListener*>::iterator i = listenerMap.find(webView);

	if(i != listenerMap.end())
		listenerMap.erase(i);

	UNBIND(begin_navigation);
	UNBIND(begin_loading);
	UNBIND(finish_loading);
	UNBIND(js_callback);
	UNBIND(receive_title);
	UNBIND(change_tooltip);
	UNBIND(change_cursor);
	UNBIND(change_keyboard_focus);
	UNBIND(change_target_url);
	UNBIND(open_external_link);
	UNBIND(request_download);
	UNBIND(web_view_crashed);
	UNBIND(plugin_crashed);
	UNBIND(request_move);
	UNBIND(get_page_contents);
	UNBIND(dom_ready);
	UNBIND(request_file_chooser);
	UNBIND(get_scroll_data);
	UNBIND(js_console_message);
	UNBIND(get_find_results);
	UNBIND(update_ime);
}

WebViewListener* WebViewEventHelper::getListener(awe_webview *webView)
{
	std::map<awe_webview*, WebViewListener*>::iterator i = listenerMap.find(webView);

	if(i != listenerMap.end())
		return i->second;

	return 0;
}