/*---------------------------------------------------------------------\
|								       |
|		       __   __	  ____ _____ ____		       |
|		       \ \ / /_ _/ ___|_   _|___ \		       |
|			\ V / _` \___ \ | |   __) |		       |
|			 | | (_| |___) || |  / __/		       |
|			 |_|\__,_|____/ |_| |_____|		       |
|								       |
|				core system			       |
|							 (C) SuSE GmbH |
\----------------------------------------------------------------------/

   File:	Selection.cc

   Author:	Klaus Kaempf <kkaempf@suse.de>
   Maintainer:  Klaus Kaempf <kkaempf@suse.de>
   Summary:     Access to Package Selection Manager
   Namespace:   Pkg

   Purpose:	Access to PMSelectionManager
		Handles selection related Pkg::function (list_of_arguments) calls
		from WFMInterpreter.
/-*/

#include <fstream>

#include <PkgFunctions.h>
#include "log.h"

#include <ycp/YCPBoolean.h>
#include <ycp/YCPInteger.h>
#include <ycp/YCPSymbol.h>
#include <ycp/YCPString.h>
#include <ycp/YCPList.h>
#include <ycp/YCPMap.h>

#include <zypp/ResPool.h>
#include <zypp/Pattern.h>

using std::string;

// ------------------------
/**
   @builtin GetSelections

   @short Return list of selections matching given status - obsoleted, use ResolvableProperties() instead
   @description
   returns a list of selection names matching the status symbol
     and the category.

   If category == "base", base selections are returned

   If category == "", addon selections are returned
   else selections matching the given category are returned

   status can be:
   <code>
   `all		: all known selections<br>
   `available	: available selections<br>
   `selected	: selected but not yet installed selections<br>
   `installed	: installed selection<br>
   </code>
   @param symbol status `all,`selected or `installed
   @param string category base or empty string for addons
   @return list<string>

*/
YCPValue
PkgFunctions::GetSelections (const YCPSymbol& stat, const YCPString& cat)
{
    YCPList selections;
    return selections;
}


// ------------------------
/**
   @builtin GetPatterns

   @short Return list of patterns matching given status (obsoleted, use ResolvableProperties())
   @description
   returns a list of pattern names matching the status symbol
     and the category.

   If category == "base", base patterns are returned

   If category == "", addon patterns are returned
   else patterns matching the given category are returned

   status can be:
   <code>
   `all		: all known patterns<br>
   `available	: available patterns<br>
   `selected	: selected but not yet installed patterns<br>
   `installed	: installed pattern<br>
   </code>
   @param symbol status `all,`selected or `installed
   @param string category base or empty string for addons
   @return list<string>

*/
YCPValue
PkgFunctions::GetPatterns(const YCPSymbol& stat, const YCPString& cat)
{
    std::string status = stat->symbol();
    std::string category = cat->value();

    YCPList patterns;

    try
    {
	for (zypp::ResPool::byKind_iterator it
	    = zypp_ptr()->pool().byKindBegin(zypp::ResKind::pattern);
	    it != zypp_ptr()->pool().byKindEnd(zypp::ResKind::pattern) ; ++it )
	{
	    std::string pattern;

	    if (status == "all")
	    {
		pattern = it->resolvable()->name();
	    }
	    else if (status == "available")
	    {
		// ignore installed patterns
		if( !it->status().wasInstalled() )
		    pattern = it->resolvable()->name();
	    }
	    else if (status == "selected")
	    {
		if( it->status().isToBeInstalled() )
		    pattern = it->resolvable()->name();
	    }
	    else if (status == "installed")
	    {
		if( it->status().wasInstalled() )
		    pattern = it->resolvable()->name();
	    }
	    else
	    {
		y2warning("Unknown status in Pkg::GetPatterns(%s, ...)", status.c_str());
		break;
	    }

	    if (pattern.empty())
	    {
		continue;
	    }

	    std::string pattern_cat = zypp::dynamic_pointer_cast<const zypp::Pattern>(it->resolvable())->category();

	    if (!category.empty() && pattern_cat != category)
	    {
		continue;	// asked for explicit category, but it doesn't match
	    }

	    patterns->add (YCPString (pattern));
	}
    }
    catch (...)
    {
    }

    return patterns;
}

// ------------------------
/**
   @builtin PatternData

   @short Get Pattern Data
   @description

   This builtin will be replaced by ResolvableProperties() in the future.
   Return information about pattern

   <code>
  	->	$[
		"arch" : "i586",
  		"category" : "Base Technologies",
		"description" : "Description"
		"icon":"",
		"order":"1020",
		"script":"",
		"summary":"Base System",
		"version":"10-5",
		"visible":true
		]
   </code>

   Get summary (aka label), category, visible, recommends, suggests, archivesize,
   order attributes of a pattern, requires, conflicts, provides and obsoletes.

   @param string pattern
   @return map Returns an empty map if no pattern found and
   Returns nil if called with wrong arguments

*/

YCPValue
PkgFunctions::PatternData (const YCPString& pat)
{
    YCPMap data;
    std::string name = pat->value();

    try
    {
	zypp::ResPool::byIdent_iterator it = zypp_ptr()->pool().byIdentBegin<zypp::Pattern>(name);

	if ( it != zypp_ptr()->pool().byIdentEnd<zypp::Pattern>(name) )
	{
	    zypp::Pattern::constPtr pattern =
		zypp::dynamic_pointer_cast<const zypp::Pattern>(it->resolvable ());

	    // pattern found
	    data->add (YCPString ("summary"), YCPString (pattern->summary()));
	    data->add (YCPString ("category"), YCPString (pattern->category()));
	    data->add (YCPString ("visible"), YCPBoolean (pattern->userVisible()));
	    data->add (YCPString ("order"), YCPString (pattern->order()));
	    data->add (YCPString ("description"), YCPString (pattern->description()));
	    data->add (YCPString ("default"), YCPBoolean (pattern->isDefault()));
	    data->add (YCPString ("icon"), YCPString (pattern->icon().asString()));
	    data->add (YCPString ("script"), YCPString (pattern->script().asString()));
	    data->add (YCPString ("version"), YCPString((*it)->edition().asString()));
	    data->add (YCPString ("arch"), YCPString((*it)->arch().asString()));
	    data->add (YCPString ("srcid"), YCPInteger(logFindAlias((*it)->repoInfo().alias())));
	}
	else
	{
	    return YCPError ("Pattern '" + name + "' not found", data);
	}
    }
    catch (...)
    {
    }

    return data;
}


// ------------------------
/**
   @builtin SelectionData

   @short Get Selection Data
   @description

   This builtin will be replaced by ResolvableProperties() in the future.

   Return information about selection

   <code>
  	->	$["summary" : "This is a nice selection",
  		"category" : "Network",
  		"visible" : true,
  		"recommends" : ["sel1", "sel2", ...],
  		"suggests" : ["sel1", "sel2", ...],
  		"archivesize" : 12345678
  		"order" : "042",
		"requires" : ["a", "b"],
		"conflicts" : ["c"],
		"provides" : ["d"],
		"obsoletes" : ["e", "f"],
		]
   </code>

   Get summary (aka label), category, visible, recommends, suggests, archivesize,
   order attributes of a selection, requires, conflicts, provides and obsoletes.

   @param string selection
   @return map Returns an empty map if no selection found and
   Returns nil if called with wrong arguments

*/

YCPValue
PkgFunctions::SelectionData (const YCPString& sel)
{
    YCPMap data;
    return data;
}


// ------------------------
/**
   @builtin SelectionContent
   @short Get list of packages listed in a selection
   @param string selection name of selection
   @param boolean to_delete if false, return packages to be installed
   if true, return packages to be deleted
   @param string language if "" (empty), return only non-language specific packages
   else return only packages machting the language
   @return list Returns an empty list if no matching selection found
   Returns nil if called with wrong arguments

*/

YCPValue
PkgFunctions::SelectionContent (const YCPString& sel, const YCPBoolean& to_delete, const YCPString& lang)
{
#warning Pkg::SelectionContent to_delete is not supported
    YCPList data;
#warning implemented correctly?
    return data;
}


// ------------------------


/**
   @builtin SetSelection

   @short Set a new selection - obsoleted, use ResolvableInstall() instead
   @description
   Mark the selection for installation
   @param string selection
   @return boolean Returns false if the given string does not match
   a known selection.
*/
YCPBoolean
PkgFunctions::SetSelection (const YCPString& selection)
{
    return YCPBoolean(false);
}

// ------------------------
/**
   @builtin ClearSelection

   @short Clear a selected selection - obsoleted, use ResolvableRemove() instead
   @param string selection
   @return boolean
*/
YCPValue
PkgFunctions::ClearSelection (const YCPString& selection)
{
    y2warning( "Pkg::ClearSelection does not reset add-on selections anymore");
    return YCPBoolean(false);
}


// ------------------------
/**
   @builtin ActivateSelections

   @short Activate all selected selections - obsoleted, use PkgSolve() instead

   @return boolean true

   @deprecated Use Pkg::PkgSolve instead, selections are solvable now
*/
YCPBoolean
PkgFunctions::ActivateSelections ()
{
    y2warning ("Pkg::ActivateSelections is obsolete. Use Pkg::PkgSolve instead");

    return YCPBoolean (true);
}

