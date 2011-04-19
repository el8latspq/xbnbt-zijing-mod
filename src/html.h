//
// Copyright (C) 2003-2005 Trevor Hogan
//

#ifndef HTML_H
 #define HTML_H

#include "config.h"
#include "tracker.h"
// tphogan - reference http://www.greenend.org.uk/rjk/2003/03/inline.html

static inline string HTML_MakeURLFromFQDN( const string &cstrFQDN, const unsigned int &cuiPort  )
{
	if( !cstrFQDN.empty( ) && cuiPort > 0 )
		return string( "http://" + cstrFQDN + ":" + CAtomInt( cuiPort ).toString( ) + RESPONSE_STR_SEPERATOR );
	else
		return string( );
}

static inline string JS_Noscript( )
{
	return "<noscript>\n<p class=\"js_warning\">" + gmapLANG_CFG["js_enable_support"] + "</p>\n</noscript>\n\n";
}

// Submit Button
static inline string Button_Submit( const string &cstrID, const string &cstrLabel )
{
	return "<span style=\"display:none\"><label for=\"" + cstrID + "\">" + cstrLabel + "</label></span><input name=\"" + cstrID + "_button\" id=\"" + cstrID + "\" alt=\"[" + cstrLabel + "]\" type=submit value=\"" + cstrLabel + "\">";
// 	return "<span style=\"display:none\"><label for=\"" + cstrID + "\">" + cstrLabel + "</label></span><input name=\"" + cstrID + "_button\" id=\"" + cstrID + "\" alt=\"[" + cstrLabel + "]\" type=submit value=\"" + STR_SUBMIT + "\">";
}

// Reset Button
static inline string Button_Reset( const string &cstrID, const string &cstrLabel )
{
	return "<span style=\"display:none\"><label for=\"" + cstrID + "\">" + cstrLabel + "</label></span><input name=\"" + cstrID + "_button\" id=\"" + cstrID + "\" alt=\"[" + cstrLabel + "]\" type=reset value=\"" + cstrLabel + "\">";
}

// Back Button
static inline string Button_Back( const string &cstrID, const string &cstrLabel )
{
	return "<span style=\"display:none\"><label for=\"" + cstrID + "\">" + cstrLabel + "</label></span><input name=\"" + cstrID + "_button\" id=\"" + cstrID + "\" alt=\"[" + cstrLabel + "]\" type=button value=\"" + cstrLabel + "\" onClick=\"" + string( JS_BACK ) + "\">";
}

// Link Button
static inline string Button_JS_Link( const string &cstrID, const string &cstrLabel, const string &cstrJSFunction )
{
	return "<span style=\"display:none\"><label for=\"" + cstrID + "\">" + cstrLabel + "</label></span><input name=\"" + cstrID + "_button\" id=\"" + cstrID + "\" alt=\"[" + cstrLabel + "]\" type=button value=\"" + cstrLabel + "\" onClick=\"" + cstrJSFunction + "\">";
}

// The target Javascript
static inline string JS_Target( const string &cstrRel, const string &cstrTarget )
{
	string strTarget = string( );

	// Display a message if javascript not supported by browser
	strTarget = JS_Noscript( );

	if( !cstrRel.empty( ) && !cstrTarget.empty( ) )
	{
		// This function facilitates the replacement of target="_blank" with an equivalent
		// in javascript fot HTML 4.01 Strict compliance
		// Usage:
		// where you used <a target="_blank" ....
		// now use <a rel="external" ...
		// then when called, the java script will intepret rel="external" as requires new browser
		strTarget += "<script type=\"text/javascript\">\n\n";
		strTarget += "<!--\n";
		strTarget += "function externalLinks() {\n";
		strTarget += "  if (!document.getElementsByTagName) return;\n";
		strTarget += "      var anchors = document.getElementsByTagName(\"a\");\n";
		strTarget += "  for (var itTags=0; itTags<anchors.length; itTags++) {\n";
		strTarget += "      var anchor = anchors[itTags];\n";
		strTarget += "      if (anchor.getAttribute(\"href\") && anchor.getAttribute(\"rel\") == \"" + cstrRel + "\")\n";
		strTarget += "      anchor.target = \"" + cstrTarget + "\";\n";
		strTarget += "  }\n";
		strTarget += "}\n";
		strTarget += "window.onload = externalLinks;\n";
		strTarget += "//-->\n\n";
		strTarget += "</script>\n\n";
	}

	return strTarget;
}

#endif
