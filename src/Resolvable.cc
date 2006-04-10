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

#include <sstream>

#include <ycp/YCPVoid.h>
#include <ycp/YCPBoolean.h>
#include <ycp/YCPInteger.h>
#include <ycp/YCPSymbol.h>
#include <ycp/YCPString.h>
#include <ycp/YCPList.h>
#include <ycp/YCPMap.h>

#include <zypp/SourceManager.h>
#include <zypp/Product.h>
#include <zypp/Patch.h>
#include <zypp/Pattern.h>
#include <zypp/base/PtrTypes.h>
#include <zypp/Dep.h>
#include <zypp/CapSet.h>
#include <zypp/PoolItem.h>

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

// ------------------------
/**
   @builtin ResolvableNeutral
   @short Remove all transactions from all resolvables with selected name and kind
   @param name_r name of the resolvable, if empty ("") use all resolvables of the kind 
   @param kind_r kind of resolvable, can be `product, `patch, `package, `selection or `pattern
   @return boolean false if failed
*/
YCPValue
PkgModuleFunctions::ResolvableNeutral ( const YCPString& name_r, const YCPSymbol& kind_r )
{
    zypp::Resolvable::Kind kind;
    
    std::string req_kind = kind_r->symbol();
    std::string name = name_r->value();

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
	y2error("Pkg::ResolvableNeutral: unknown symbol: %s", req_kind.c_str());
	return YCPBoolean(false);
    }

    bool ret = true;

    try
    {
	for (zypp::ResPool::byKind_iterator it = zypp_ptr()->pool().byKindBegin(kind);
	    it != zypp_ptr()->pool().byKindEnd(kind);
	    ++it)
	{
	    if (name.empty() || (*it)->name() == name)
	    {
		if (!it->status().setTransact(false, whoWantsIt))
		{
		    ret = false;
		}
	    }
	}
    }
    catch (...)
    {
	return YCPBoolean(false);
    }

    return YCPBoolean(ret);
}

// ------------------------
/**
   @builtin ResolvableSetSoftLock
   @short Soft lock - it prevents the solver from re-selecting item
   if it's recommended (if it's required it will be selected).
   @param name_r name of the resolvable, if empty ("") use all resolvables of the kind 
   @param kind_r kind of resolvable, can be `product, `patch, `package, `selection or `pattern
   @return boolean false if failed
*/
YCPValue
PkgModuleFunctions::ResolvableSetSoftLock ( const YCPString& name_r, const YCPSymbol& kind_r )
{
    zypp::Resolvable::Kind kind;
    
    std::string req_kind = kind_r->symbol();
    std::string name = name_r->value();

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
	y2error("Pkg::ResolvableSetSoftLock: unknown symbol: %s", req_kind.c_str());
	return YCPBoolean(false);
    }

    bool ret = true;

    try
    {
	for (zypp::ResPool::byKind_iterator it = zypp_ptr()->pool().byKindBegin(kind);
	    it != zypp_ptr()->pool().byKindEnd(kind);
	    ++it)
	{
	    if (name.empty() || (*it)->name() == name)
	    {
		if (!it->status().setSoftLock(whoWantsIt))
		{
		    ret = false;
		}
	    }
	}
    }
    catch (...)
    {
	return YCPBoolean(false);
    }

    return YCPBoolean(ret);
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
    return ResolvablePropertiesEx (name, kind_r, version, false);
}

YCPValue
PkgModuleFunctions::ResolvableDependencies(const YCPString& name, const YCPSymbol& kind_r, const YCPString& version)
{
    return ResolvablePropertiesEx (name, kind_r, version, true);
}

YCPValue
PkgModuleFunctions::ResolvablePropertiesEx(const YCPString& name, const YCPSymbol& kind_r, const YCPString& version, bool dependencies)
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
   
   try
   { 
	for (zypp::ResPool::byKind_iterator it = zypp_ptr()->pool().byKindBegin(kind);
	    it != zypp_ptr()->pool().byKindEnd(kind);
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
		    info->add(YCPString("display_name"), YCPString(product->summary()));
		    info->add(YCPString("short_name"), product->shortName().size() > 0 ? YCPString(product->shortName()) : YCPString(product->summary()));

		    YCPList updateUrls;
		    std::list<zypp::Url> pupdateUrls = product->updateUrls();
		    for (std::list<zypp::Url>::const_iterator it = pupdateUrls.begin(); it != pupdateUrls.end(); ++it)
		    {
			updateUrls->add(YCPString(it->asString()));
		    }
		    info->add(YCPString("update_urls"), updateUrls);

		    YCPList flags;
		    std::list<std::string> pflags = product->flags();
		    for (std::list<std::string>::const_iterator flag_it = pflags.begin();
			flag_it != pflags.end(); ++flag_it)
		    {
			flags->add(YCPString(*flag_it));
		    }
		    info->add(YCPString("flags"), flags);
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

		// dependency info
		if (dependencies)
		{
		    std::set<std::string> _kinds;
		    _kinds.insert("provides");
		    _kinds.insert("prerequires");
		    _kinds.insert("requires");
		    _kinds.insert("conflicts");
		    _kinds.insert("obsoletes");
		    _kinds.insert("recommends");
		    _kinds.insert("suggests");
		    _kinds.insert("freshens");
		    _kinds.insert("enhances");
		    _kinds.insert("supplements");
		    YCPList ycpdeps;
		    for (std::set<std::string>::const_iterator kind_it = _kinds.begin();
			kind_it != _kinds.end(); ++kind_it)
		    {
			try {
			    zypp::Dep depkind(*kind_it);
			    zypp::CapSet deps = it->resolvable()->dep(depkind);
			    for (zypp::CapSet::const_iterator d = deps.begin(); d != deps.end(); ++d)
			    {
				YCPMap ycpdep;
				ycpdep->add (YCPString ("res_kind"), YCPString (d->refers().asString()));
				ycpdep->add (YCPString ("name"), YCPString (d->asString()));
				ycpdep->add (YCPString ("dep_kind"), YCPString (*kind_it));
				ycpdeps->add (ycpdep);
			    }
	
			}
			catch (...)
			{}
		    }
		    info->add (YCPString ("dependencies"), ycpdeps);
		}
		ret->add(info);
	    }
	}
    }
    catch (...)
    {
    }

    return ret;
}


// ------------------------
/**
   @builtin ResolvablePreselectPatches
   @short Preselect patches for auto online update during the installation
   @return boolean false if failed
*/
YCPValue
PkgModuleFunctions::ResolvablePreselectPatches ()
{
    YCPBoolean ret = true;

    try
    {
	const zypp::ResPool & pool = zypp_ptr()->pool();

	// pseudo code from
	// http://svn.suse.de/trac/zypp/wiki/ZMD/YaST/update/yast

	zypp::ResPool::const_iterator
	    b = pool.begin (),
	    e = pool.end (),
	    i;
	for (i = b; i != e; ++i)
	{
	    if (i->status().isNeeded()) {	// uninstalled
		zypp::Patch::constPtr pch = zypp::asKind<zypp::Patch>(i->resolvable());
		if (pch && pch->category () != "optional") {
		    // dont auto-install optional patches

		    stringstream str; 
		    str << *i << endl;
		    y2milestone( "Setting '%s' to transact", str.str().c_str() );
		    i->status().setTransact(true, whoWantsIt); // schedule for installation
		}
		
	    }
	}
    }
    catch (...)
    {
	ret = false;
    }

    return ret;
}
