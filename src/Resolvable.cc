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

#include <zypp/SourceManager.h>
#include <zypp/Product.h>
#include <zypp/Pattern.h>
#include <zypp/base/PtrTypes.h>

///////////////////////////////////////////////////////////////////

// ------------------------
/**
   @builtin ResolvableInstall
   @short Install all resolvables with selected name and kind
   @param name_r name of the resolvable, if empty ("") install all resolvables of the kind 
   @param kind_r kind of resolvable, can be `product, `patch, `package, `selection or `pattern
   @return boolean false if failed
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
    else
    {
	y2error("Pkg::ResolvableInstall: unknown symbol: %s", req_kind.c_str());
	return YCPBoolean(false);
    }

    return YCPBoolean(
	(name_r->value().empty())
	    ? DoProvideAllKind(kind)
	    : DoProvideNameKind (name_r->value(), kind)
    );
}


// ------------------------
/**
   @builtin ResolvableRemove
   @short Removes all resolvables with selected name and kind
   @param name_r name of the resolvable, if empty ("") remove all resolvables of the kind 
   @param kind_r kind of resolvable, can be `product, `patch, `package, `selection or `pattern
   @return boolean false if failed
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
    else
    {
	y2error("Pkg::ResolvableRemove: unknown symbol: %s", req_kind.c_str());
	return YCPBoolean(false);
    }

    return YCPBoolean(
	(name_r->value().empty())
	    ? DoRemoveAllKind(kind)
	    : DoRemoveNameKind (name_r->value(), kind)
    );
}



/**
   @builtin ResolvableProperties

   @short Return properties of resolvable
   @description
   return list of resolvables of selected kind with required name
 
   @param name name of the resolvable, if empty returns all resolvables of the kind
   @param kind_r kind of resolvable, can be `product, `patch, `package, `selection or `pattern
   @param version version of the resolvable, if empty all versions are returned

   @return list<map<string,any>> list of $[ "name":string, "version":string, "arch":string, "source":integer, "status":symbol ] maps
   status is `installed, `selected or `available, source is source ID or -1 if the resolvable is installed in the target
*/

YCPValue
PkgModuleFunctions::ResolvableProperties(const YCPString& name, const YCPSymbol& kind_r, const YCPString& version)
{
    zypp::Resolvable::Kind kind;
    std::string req_kind = kind_r->symbol ();
    std::string nm = name->value();
    std::string vers = version->value();
    YCPList ret;

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
    else
    {
	y2error("Pkg::ResolvableProperties: unknown symbol: %s", req_kind.c_str());
	return ret;
    }

    std::list<zypp::SourceManager::SourceId> source_ids = zypp::SourceManager::sourceManager()->enabledSources();
    
    for (zypp::ResPool::byKind_iterator it = zypp_ptr->pool().byKindBegin(kind);
	it != zypp_ptr->pool().byKindEnd(kind);
	++it)
    {
	if ((nm.empty() || nm == (*it)->name())
	    && (vers.empty() || vers == (*it)->edition().asString()))
	{
	    YCPMap info;

	    info->add(YCPString("name"), YCPString((*it)->name()));
	    info->add(YCPString("version"), YCPString((*it)->edition().asString()));
	    info->add(YCPString("arch"), YCPString((*it)->arch().asString()));
	    info->add(YCPString("description"), YCPString((*it)->description()));
	    info->add(YCPString("summary"), YCPString((*it)->summary()));

	    // status
	    std::string stat;

	    if (it->status().isInstalled())
	    {
		stat = "installed";
	    }
	    else if (it->status().isToBeInstalled())
	    {
		stat = "selected";
	    }
	    else
	    {
		stat = "available";
	    }

	    info->add(YCPString("status"), YCPSymbol(stat));

	    // source
	    zypp::Source_Ref res_src = (*it)->source();
	    unsigned src_index; 
	    bool found = false;

	    // find index of the source
	    // TODO optimize it by using hash<Source, index>?
	    for (std::list<zypp::SourceManager::SourceId>::const_iterator idxit = source_ids.begin()
		; idxit != source_ids.end()
		; idxit++)
	    {
		zypp::Source_Ref src = zypp::SourceManager::sourceManager()->findSource((*idxit));
		
		if (src == res_src)
		{
		    found = true;
		    src_index = (*idxit);
		    break;
		}
	    }

	    info->add(YCPString("source"), YCPInteger((found) ? src_index : -1LL));

	    // product specific info
	    if( req_kind == "product" ) {
		zypp::Product::constPtr product = boost::dynamic_pointer_cast<const zypp::Product>(it->resolvable());
		info->add(YCPString("category"), YCPString(product->category()));
		info->add(YCPString("vendor"), YCPString(product->vendor()));
		info->add(YCPString("relnotes_url"), YCPString(product->releaseNotesUrl().asString()));
		info->add(YCPString("display_name"), YCPString(product->displayName()));
	    }

	    // pattern specific info
	    if ( req_kind == "pattern" ) {
		zypp::Pattern::constPtr pattern = boost::dynamic_pointer_cast<const zypp::Pattern>(it->resolvable());
		info->add(YCPString("category"), YCPString(pattern->category()));
		info->add(YCPString("user_visible"), YCPBoolean(pattern->userVisible()));
		info->add(YCPString("default"), YCPBoolean(pattern->isDefault()));
		info->add(YCPString("icon"), YCPString(pattern->icon().asString()));
		info->add(YCPString("script"), YCPString(pattern->script().asString()));
	    }

	    ret->add(info);
	}
    }

    return ret;
}


