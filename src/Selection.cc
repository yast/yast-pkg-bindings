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

   File:	PkgModuleFunctionsSelection.cc

   Author:	Klaus Kaempf <kkaempf@suse.de>
   Maintainer:  Klaus Kaempf <kkaempf@suse.de>
   Summary:     Access to Package Selection Manager
   Namespace:   Pkg

   Purpose:	Access to PMSelectionManager
		Handles selection related Pkg::function (list_of_arguments) calls
		from WFMInterpreter.
/-*/

#include <fstream>

#include <ycp/y2log.h>
#include <PkgModule.h>
#include <PkgModuleFunctions.h>

#include <ycp/YCPVoid.h>
#include <ycp/YCPBoolean.h>
#include <ycp/YCPInteger.h>
#include <ycp/YCPSymbol.h>
#include <ycp/YCPString.h>
#include <ycp/YCPList.h>
#include <ycp/YCPMap.h>

#include <zypp/ResPool.h>
#include <zypp/ResTraits.h>
#include <zypp/Resolvable.h>
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
PkgModuleFunctions::GetSelections (const YCPSymbol& stat, const YCPString& cat)
{
    YCPList selections;
    y2warning("Pkg::GetSelections is obsoleted, selections have been replaced by patterns");
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
PkgModuleFunctions::GetPatterns(const YCPSymbol& stat, const YCPString& cat)
{
        std::string status = stat->symbol();
    std::string category = cat->value();

    YCPList patterns;

    try
    {
	for (zypp::ResPoolProxy::const_iterator it = zypp_ptr()->poolProxy().byKindBegin(zypp::ResKind::pattern);
	    it != zypp_ptr()->poolProxy().byKindEnd(zypp::ResKind::pattern);
	    ++it)
	{

	    if (status == "all" || (status == "available" && (*it)->hasCandidateObj())
		// TODO FIXME isSatified() should be here???
		|| (status == "installed" && (*it)->hasInstalledObj())
		|| (status == "selected" && (*it)->fate() == zypp::ui::Selectable::TO_INSTALL)
		)
	    {
		// check the required category
		std::string pattern_cat = zypp::dynamic_pointer_cast<const zypp::Pattern>((*it)->theObj().resolvable())->category();

		if (!category.empty() && pattern_cat != category)
		{
		    continue;	// asked for explicit category, but it doesn't match
		}

		patterns->add(YCPString((*it)->name()));
	    }
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
PkgModuleFunctions::PatternData (const YCPString& pat)
{
        YCPMap ret;

    try
    {
	YCPValue value = ResolvableProperties(pat, YCPSymbol("pattern"), YCPString(""));
	YCPList lst;

	if (!value.isNull() && value->isList())
	{
	    lst = value->asList();
	}

	if (lst->size() >= 1)
	{
	    YCPValue first = lst->value(0);

	    if (!first.isNull() && first->isMap())
	    {
		ret = first->asMap();
	    }

	    if (!ret.isNull())
	    {
		YCPValue val = ret->value(YCPString("source"));
		ret->add(YCPString("srcid"), val);

		val = ret->value(YCPString("user_visible"));
		ret->add(YCPString("visible"), val);
	    }
	    else
	    {
		return YCPError ("Pattern '" + pat->value() + "' not found", ret);
	    }
	}
	else
	{
	    return YCPError ("Pattern '" + pat->value() + "' not found", ret);
	}
    }
    catch (...)
    {
    }

    return ret;
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
PkgModuleFunctions::SelectionData (const YCPString& sel)
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
PkgModuleFunctions::SelectionContent (const YCPString& sel, const YCPBoolean& to_delete, const YCPString& lang)
{
    YCPList data;
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
PkgModuleFunctions::SetSelection (const YCPString& selection)
{
    y2warning("Pkg::SetSelection is obsoleted, selections have been replaced by patterns");
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
PkgModuleFunctions::ClearSelection (const YCPString& selection)
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
PkgModuleFunctions::ActivateSelections ()
{
    y2warning ("Pkg::ActivateSelections is obsolete. Use Pkg::PkgSolve instead");
    return YCPBoolean (true);
}

