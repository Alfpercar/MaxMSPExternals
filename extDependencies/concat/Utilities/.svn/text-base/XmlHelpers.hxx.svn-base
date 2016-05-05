#ifndef INCLUDED_CONCAT_XMLHELPERS_HXX
#define INCLUDED_CONCAT_XMLHELPERS_HXX

#include "tinyxml/tinyxml.h"
#include <sstream>
#include <string>
#include "concat/Utilities/Optional.hxx"
#include "concat/Utilities/StringHelpers.hxx"

#include <cassert>
#include <fstream>
#include <algorithm>
#include <windows.h>

#include "Logging.hxx"

namespace concat
{
	// taken from tiny xml
	// buf = source buffer (null-terminated)
	// length = source buffer length
	// data = output buffer/string
	inline void normalizeEol(const char *buf, int length, std::string &data)
	{
		const char* lastPos = buf;
		const char* p = buf;

//		buf[length] = 0;
		while( *p ) {
			assert( p < (buf+length) );
			if ( *p == 0xa ) {
				// Newline character. No special rules for this. Append all the characters
				// since the last string, and include the newline.
				data.append( lastPos, (p-lastPos+1) );	// append, include the newline
				++p;									// move past the newline
				lastPos = p;							// and point to the new buffer (may be 0)
				assert( p <= (buf+length) );
			}
			else if ( *p == 0xd ) {
				// Carriage return. Append what we have so far, then
				// handle moving forward in the buffer.
				if ( (p-lastPos) > 0 ) {
					data.append( lastPos, p-lastPos );	// do not add the CR
				}
				data += (char)0xa;						// a proper newline

				if ( *(p+1) == 0xa ) {
					// Carriage return - new line sequence
					p += 2;
					lastPos = p;
					assert( p <= (buf+length) );
				}
				else {
					// it was followed by something else...that is presumably characters again.
					++p;
					lastPos = p;
					assert( p <= (buf+length) );
				}
			}
			else {
				++p;
			}
		}
		// Handle any left over characters.
		if ( p-lastPos ) {
			data.append( lastPos, p-lastPos );
		}
//		delete [] buf;
//		buf = 0;
	}

	inline bool loadXml(const char *filename, TiXmlDocument &target)
	{
		// Open file:
		FILE *file = fopen(filename, "rb"); // binary to normalize eol's
		if (!file)
		{
			LOG_DEBUG_N("concat.xml_loader", "Failed opening file!");
			return false;
		}

		// Get length in bytes:
		int size = 0;
		fseek(file, 0, SEEK_END);
		size = ftell(file);
		fseek(file, 0, SEEK_SET);

		if (size == 0)
		{
			LOG_DEBUG_N("concat.xml_loader", "Size is zero!");
			return false;
		}

		// Allocate buffer and read entire file to buffer:
		char *buffer = new char[size + 1]; // plus null termination
		if (fread(buffer, size, 1, file) != 1)
		{
			delete[] buffer;
			fclose(file);
			LOG_DEBUG_N("concat.xml_loader", "Failed read!");
			return false;
		}

		// Close file:
		fclose(file);

		// Null terminate:
		buffer[size] = '\0';

		// Detect UTF-16 or UTF-8:
		char utf16Mark[12]; // XXX: can also be lowercase, etc. (use predicate of std::search)
		utf16Mark[0] = 'U';
		utf16Mark[1] = '\0';
		utf16Mark[2] = 'T';
		utf16Mark[3] = '\0';
		utf16Mark[4] = 'F';
		utf16Mark[5] = '\0';
		utf16Mark[6] = '-';
		utf16Mark[7] = '\0';
		utf16Mark[8] = '1';
		utf16Mark[9] = '\0';
		utf16Mark[10] = '6';
		utf16Mark[11] = '\0';

		const char *searchBegin = buffer;
		const char *searchEnd = buffer + size;
		const char *searchResult = std::search(buffer, buffer + size, utf16Mark, utf16Mark + 12);

		bool isUtf16;
		if (searchResult != searchEnd)
		{
			LOG_DEBUG_N("concat.xml_loader", "Detected UTF-16 file.");
			isUtf16 = true;
		}
		else
		{
			LOG_DEBUG_N("concat.xml_loader", "Detected UTF-8 file.");
			isUtf16 = false;
		}

		// Convert UTF-16 to UTF-8 if needed:
		char *utf8Data = NULL;
		int utf8Size = 0;
		if (isUtf16)
		{
			LPCWSTR lpWideCharStr = (LPCWSTR)(buffer);
			assert((size % 2) == 0);
			int requiredSizeBytes = WideCharToMultiByte(CP_UTF8, 0, lpWideCharStr, size/2, NULL, 0, NULL, NULL);
			utf8Data = new char[requiredSizeBytes + 1];
			int result = WideCharToMultiByte(CP_UTF8, 0, lpWideCharStr, size/2, utf8Data, requiredSizeBytes, NULL, NULL);
			utf8Data[requiredSizeBytes] = '\0';
			delete[] buffer;

			if (result == 0)
			{
				delete[] utf8Data;
				LOG_DEBUG_N("concat.xml_loader", "Failed converting UTF-16 to UTF-8.");
				return false;
			}

			utf8Size = requiredSizeBytes;
		}
		else
		{
			utf8Data = buffer;
			utf8Size = size;
		}

		// Normalize EOL:
		std::string data;
		normalizeEol(utf8Data, utf8Size, data);

		// Cleanup:
		delete[] utf8Data;

		// Parse in-memory XML data:
		target.Parse(data.c_str(), 0, TIXML_DEFAULT_ENCODING);

		if (target.Error())
		{
			LOG_DEBUG_N("concat.xml_loader", formatStr("TinyXML error: %s", target.ErrorDesc()));
			LOG_DEBUG_N("concat.xml_loader", formatStr("TinyXML error row: %d", target.ErrorRow()));
			LOG_DEBUG_N("concat.xml_loader", formatStr("TinyXML error col: %d", target.ErrorCol()));
			return false;
		}
		else
		{
			LOG_DEBUG_N("concat.xml_loader", "Ok!");
			return true;
		}
	}
}

namespace concat
{
	typedef Optional<bool> BoolWithFlag;
	typedef Optional<int> IntWithFlag;
	typedef Optional<float> FloatWithFlag;
	typedef Optional<double> DoubleWithFlag;
	typedef Optional<std::string> StringWithFlag;
}

namespace concat
{
	inline void getElementTextAsDouble(TiXmlElement *element, DoubleWithFlag &target)
	{
		if (element == NULL)
			return;

		std::stringstream str2f_conv;
		str2f_conv << element->GetText();
		double v;
		str2f_conv >> v;

		target = v;
	}

	inline void getElementTextAsFloat(TiXmlElement *element, FloatWithFlag &target)
	{
		if (element == NULL)
			return;

		std::stringstream str2f_conv;
		str2f_conv << element->GetText();
		float v;
		str2f_conv >> v;

		target = v;
	}

	// -----------------------------------------------------------------------------------

	inline void getFirstChildElementTextAsDouble(TiXmlElement *parent, const char *childName, DoubleWithFlag &target)
	{
		if (parent == NULL)
			return;

		TiXmlElement *child = parent->FirstChildElement(childName);
		if (child == NULL)
			return;

		getElementTextAsDouble(child, target);
	}
	
	inline void getFirstChildElementTextAsFloat(TiXmlElement *parent, const char *childName, FloatWithFlag &target)
	{
		if (parent == NULL)
			return;

		TiXmlElement *child = parent->FirstChildElement(childName);
		if (child == NULL)
			return;

		getElementTextAsFloat(child, target);
	}

	inline void getFirstChildElementTextAsString(TiXmlElement *parent, const char *childName, StringWithFlag &target)
	{
		if (parent == NULL)
			return;

		TiXmlElement *child = parent->FirstChildElement(childName);
		if (child == NULL)
			return;

		target = child->GetText();
	}

	// -----------------------------------------------------------------------------------

	inline void queryDoubleAttribute(TiXmlElement *element, const char *attributeName, DoubleWithFlag &target)
	{
		if (element == NULL)
			return;

		double tmp;
		if (element->QueryDoubleAttribute(attributeName, &tmp) == TIXML_SUCCESS)
			target = tmp;
	}

	inline void queryFloatAttribute(TiXmlElement *element, const char *attributeName, FloatWithFlag &target)
	{
		if (element == NULL)
			return;

		float tmp;
		if (element->QueryFloatAttribute(attributeName, &tmp) == TIXML_SUCCESS)
			target = tmp;
	}

	inline void queryBoolAttribute(TiXmlElement *element, const char *attributeName, BoolWithFlag &target)
	{
		if (element == NULL)
			return;

		const char *attributeStr = element->Attribute(attributeName);

		if (attributeStr != NULL)
		{
			if (ciStrEqual(attributeStr, "true"))
				target = true;
			else if (ciStrEqual(attributeStr, "false"))
				target = false;
		}
	}

	inline void queryStringAttribute(TiXmlElement *element, const char *attributeName, StringWithFlag &target)
	{
		if (element == NULL)
			return;

		const char *attributeStr = element->Attribute(attributeName);
		if (attributeStr == NULL)
			return;

		target = attributeStr;
	}

	inline void queryIntAttribute(TiXmlElement *element, const char *attributeName, IntWithFlag &target)
	{
		if (element == NULL)
			return;

		int tmp;
		if (element->QueryIntAttribute(attributeName, &tmp) == TIXML_SUCCESS)
			target = tmp;
	}

	// -----------------------------------------------------------------------------------

	inline TiXmlElement *getFirstChildElementWithSpecificStringAttribute(TiXmlElement *parent, const char *childElementName, const char *attributeName, const char *attributeValue)
	{
		TiXmlElement *result = NULL;

		for (TiXmlElement *child = parent->FirstChildElement(childElementName); child != NULL; child = child->NextSiblingElement(childElementName))
		{
			const char *attributeStr = child->Attribute(attributeName);
			if (attributeStr == NULL)
				continue;

			if (std::string(attributeStr) == std::string(attributeValue))
			{
				result = child;
				break;
			}
		}

		return result;
	}
}

#endif