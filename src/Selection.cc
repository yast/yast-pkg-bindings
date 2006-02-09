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
#include <zypp/Selection.h>

using std::string;



// ------------------------
/**
   @builtin GetSelections

   @short Return list of selections matching given status
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
    string status = stat->symbol();
    string category = cat->value();

    // FIXME: this should be done more nicely
    if ( category == "base" ) 
	category = "baseconf";

    YCPList selections;

    for (zypp::ResPool::byKind_iterator it 
	= zypp_ptr->pool().byKindBegin(zypp::ResTraits<zypp::Selection>::kind);
	it != zypp_ptr->pool().byKindEnd(zypp::ResTraits<zypp::Selection>::kind) ; ++it )
    {
	string selection;

	if (status == "all" || status == "available")
	{
	    selection = it->resolvable()->name();
	}
	else if (status == "selected")
	{
	    if( it->status().isToBeInstalled() )
	        selection = it->resolvable()->name();
	}
	else if (status == "installed")
	{
	    if( it->status().wasInstalled() )
	        selection = it->resolvable()->name();
	}
	else
	{
	    y2warning (string ("Unknown status in Pkg::GetSelections("+status+", ...)").c_str());
	    break;
	}

	if (selection.empty())
	{
	    continue;
	}

	string selection_cat = zypp::dynamic_pointer_cast<const zypp::Selection>(it->resolvable())->category();
	if (category != "")
	{
	    if ( selection_cat != category)
	    {
		continue;			// asked for explicit category
	    }
	}
	else	// category == ""
	{
	    if (selection_cat == "baseconf")
	    {
		continue;			// asked for non-base
	    }
	}
	selections->add (YCPString (selection));
    }
    return selections;
}



// ------------------------
/**
   @builtin SelectionData

   @short Get Selection Data
   @description

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
    string name = sel->value();

    /* TODO FIXME
    PMSelectablePtr selectable = _y2pm.selectionManager().getItem(name);
    if (!selectable)
    {
	return YCPError ("Selection '"+name+"' not found", data);
    }
    PMSelectionPtr selection = selectable->theObject();
    if (!selection)
    {
	return YCPError ("Selection '"+name+"' no object", data);
    }

    data->add (YCPString ("summary"), YCPString (selection->summary(_y2pm.getPreferredLocale())));
    data->add (YCPString ("category"), YCPString (selection->category()));
    data->add (YCPString ("visible"), YCPBoolean (selection->visible()));

    std::list<std::string> recommends = selection->recommends();
    YCPList recommendslist;
    for (std::list<std::string>::iterator recIt = recommends.begin();
	recIt != recommends.end(); ++recIt)
    {
	if (!((*recIt).empty()))
	    recommendslist->add (YCPString (*recIt));
    }
    data->add (YCPString ("recommends"), recommendslist);

    std::list<std::string> suggests = selection->suggests();
    YCPList suggestslist;
    for (std::list<std::string>::iterator sugIt = suggests.begin();
	sugIt != suggests.end(); ++sugIt)
    {
	if (!((*sugIt).empty()))
	    suggestslist->add (YCPString (*sugIt));
    }
    data->add (YCPString ("suggests"), suggestslist);

    data->add (YCPString ("archivesize"), YCPInteger ((long long) (selection->archivesize())));
    data->add (YCPString ("order"), YCPString (selection->order()));

    tiny_helper_no1 (&data, "requires", selection->requires ());
    tiny_helper_no1 (&data, "conflicts", selection->conflicts ());
    tiny_helper_no1 (&data, "provides", selection->provides ());
    tiny_helper_no1 (&data, "obsoletes", selection->obsoletes ());
    */

   zypp::ResPool::byName_iterator it = std::find_if (
        zypp_ptr->pool().byNameBegin(name)
        , zypp_ptr->pool().byNameEnd(name)
        , zypp::resfilter::ByKind(zypp::ResTraits<zypp::Selection>::kind)
    );

    if ( it != zypp_ptr->pool().byNameEnd(name) )
    {
	zypp::Selection::constPtr selection = 
	    zypp::dynamic_pointer_cast<const zypp::Selection>(it->resolvable ());

	// selection found
	data->add (YCPString ("summary"), YCPString (selection->summary()));
	data->add (YCPString ("category"), YCPString (selection->category()));
	data->add (YCPString ("visible"), YCPBoolean (selection->visible()));
	data->add (YCPString ("order"), YCPString (selection->order()));
#warning Report also requires, provides, conflicts and obsoletes
    }
    else
    {
	ycperror("Selection '%s' not found", name.c_str());
    }

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
    string name = sel->value();

#warning Pkg::SelectionContent not ported yet

    /* TODO FIXME
    PMSelectablePtr selectable = _y2pm.selectionManager().getItem(name);
    if (!selectable)
    {
	return YCPError ("Selection '"+name+"' not found", data);
    }
    PMSelectionPtr selection = selectable->theObject();
    if (!selection)
    {
	return YCPError ("Selection '"+name+"' no object", data);
    }

    std::list<std::string> pacnames;
    */
    YCPList paclist;
/*    LangCode locale (lang->value());

    if (to_delete->value() == false)			// inspacks
    {
	pacnames = selection->inspacks (locale);
    }
    else
    {
	pacnames = selection->delpacks (locale);
    }

    for (std::list<std::string>::iterator pacIt = pacnames.begin();
	pacIt != pacnames.end(); ++pacIt)
    {
	if (!((*pacIt).empty()))
	    paclist->add (YCPString (*pacIt));
    }
*/

    ycpinternal ("Pkg::SelectionContents not ported yet");
    return paclist;
}


// ------------------------


/**
   @builtin SetSelection

   @short Set a new selection
   @description
   If the selection is a base selection,
   this effetively resets the current package selection to
   the packages of the newly selected base selection
   Usually returns true
   @param string selection
   @return boolean Returns false if the given string does not match
   a known selection.

*/
YCPBoolean
PkgModuleFunctions::SetSelection (const YCPString& selection)
{
    return DoProvideNameKind( selection->value(), ResTraits<zypp::Selection>::kind);
}

// ------------------------
/**
   @builtin ClearSelection

   @short Clear a selected selection
   @param string selection
   @return boolean


*/
YCPValue
PkgModuleFunctions::ClearSelection (const YCPString& selection)
{
    ycpwarning( "Pkg::ClearSelection does not reset add-on selections anymore");
    return YCPBoolean( DoRemoveNameKind( selection->value(), ResTraits<zypp::Selection>::kind ) );
}


// ------------------------
/**
   @builtin ActivateSelections

   @short Activate all selected selections
   @description
   To be called when user is done with selections and wants
   to continue on the package level (or finish)

   This will transfer the selection status to package status

   @return boolean
   
   @deprecated Use Pkg::PkgSolve instead, selections are solvable now
*/
YCPBoolean
PkgModuleFunctions::ActivateSelections ()
{
    y2warning ("Pkg::ActivateSelections is obsolete. Use Pkg::PkgSolve instead");

    return YCPBoolean (true);
}

