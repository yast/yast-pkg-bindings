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

   File:	Resolvable.cc

   Author:	Stanislav Visnovsky <visnov@suse.de>
   Maintainer:  Stanislav Visnovsky <visnov@suse.de>
   Namespace:   Pkg

   Summary:	Access to packagemanager
   Purpose:     Handles package related Pkg::function (list_of_arguments) calls.
/-*/

#include <ycp/y2log.h>
#include "PkgModuleFunctions.h"

#include <ycp/YCPVoid.h>
#include <ycp/YCPBoolean.h>
#include <ycp/YCPInteger.h>
#include <ycp/YCPSymbol.h>
#include <ycp/YCPString.h>
#include <ycp/YCPList.h>
#include <ycp/YCPMap.h>

#include <zypp/ResPool.h>
#include <zypp/Package.h>
#include <zypp/Product.h>
#include <zypp/SourceManager.h>
#include <zypp/UpgradeStatistics.h>
#include <zypp/target/rpm/RpmDb.h>

#include <fstream>

///////////////////////////////////////////////////////////////////

namespace zypp {
	typedef std::list<PoolItem> PoolItemList; 
}


// ------------------------
/**
   @builtin DoProvide
   @short Install a list of packages to the system
   @description
   Provides (read: installs) a list of tags to the system

   tag is a package name

   returns a map of tag,reason pairs if tags could not be provided.
   Usually this map should be empty (all required packages are
   installed)

   If tags could not be provided (due to package install failures or
   conflicts), the tag is listed as a key and the value describes
   the reason for the failure (as an already translated string).
   @param list tags
   @return map
*/
YCPValue
PkgModuleFunctions::ResolvableInstall( const YCPString& name_r, const YCPSymbol& kind_r )
{
    zypp::Resolvable::Kind kind;
    
    std::string req_kind = kind_r->symbol ();

    if( req_kind == "product" ) {
	kind = zypp::ResTraits<zypp::Product>::kind;
    }
    else if ( req_kind == "patch" ) {
    	kind = zypp::ResTraits<zypp::Patch>::kind;
    }
    else if ( req_kind == "package" ) {
	kind = zypp::ResTraits<zypp::Package>::kind;
    }
    else if ( req_kind == "selection" ) {
	kind = zypp::ResTraits<zypp::Selection>::kind;
    }
    else if ( req_kind == "pattern" ) {
	kind = zypp::ResTraits<zypp::Pattern>::kind;
    }

    // FIXME: `any?


    return YCPBoolean(
	DoProvideNameKind (name_r->value(), kind)
    );
}


// ------------------------
/**
   @builtin DoRemove

   @short Removes a list of packges from the system
   @description
   tag is a package name

   returns a map of tag,reason pairs if tags could not be removed.
   Usually this map should be empty (all required packages are
   removed)

   If a tag could not be removed (because other packages still
   require it), the tag is listed as a key and the value describes
   the reason for the failure (as an already translated string).
   @param list tags
   @return list Result
*/
YCPValue
PkgModuleFunctions::ResolvableRemove ( const YCPString& name_r, const YCPSymbol& kind_r )
{
    zypp::Resolvable::Kind kind;
    
    std::string req_kind = kind_r->symbol ();

    if( req_kind == "product" ) {
	kind = zypp::ResTraits<zypp::Product>::kind;
    }
    else if ( req_kind == "patch" ) {
    	kind = zypp::ResTraits<zypp::Patch>::kind;
    }
    else if ( req_kind == "package" ) {
	kind = zypp::ResTraits<zypp::Package>::kind;
    }
    else if ( req_kind == "selection" ) {
	kind = zypp::ResTraits<zypp::Selection>::kind;
    }
    else if ( req_kind == "pattern" ) {
	kind = zypp::ResTraits<zypp::Pattern>::kind;
    }

    // FIXME: `any?


    return YCPBoolean(
	DoRemoveNameKind (name_r->value(), kind)
    );
}

/**
   @builtin GetPackages

   @short Get list of packages (installed, selected, available)
   @description
   return list of packages (["pkg1", "pkg2", ...] if names_only==true,
   ["pkg1 version release arch", "pkg1 version release arch", ... if
   names_only == false]

   @param symbol 'which' defines which packages are returned: `installed all installed packages,
   `selected returns all selected but not yet installed packages, `available returns all
   available packages (from the installation source)
   @param boolean names_only If true, return package names only
   @return list<string>

*/
/*
YCPValue
PkgModuleFunctions::GetPackages(const YCPSymbol& y_which, const YCPBoolean& y_names_only)
{
    string which = y_which->symbol();
    bool names_only = y_names_only->value();

    YCPList packages;

    for (zypp::ResPool::byKind_iterator it = zypp_ptr->pool().byKindBegin(zypp::ResTraits<zypp::Package>::kind);
	it != zypp_ptr->pool().byKindEnd(zypp::ResTraits<zypp::Package>::kind);
	++it)
    {
	if (which == "installed")
	{
	    if (it->status().wasInstalled())
	    {
		pkg2list(packages, it, names_only);
	    }
	}
	else if (which == "selected")
	{
	    if (it->status().isToBeInstalled())
	    {
		pkg2list(packages, it, names_only);
	    }
	}
	else if (which == "available")
	{
	    pkg2list(packages, it, names_only);
	}
	else
	{
	    return YCPError ("Wrong parameter for Pkg::GetPackages");
	}
    }

    return packages;
}

*/
