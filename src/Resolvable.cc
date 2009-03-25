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
#include <y2util/Y2SLog.h>
#include "PkgModuleFunctions.h"
#include "ProvideProcess.h"

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
#include <zypp/Language.h>
#include <zypp/base/PtrTypes.h>
#include <zypp/Dep.h>
#include <zypp/CapSet.h>
#include <zypp/PoolItem.h>
#include <zypp/base/Algorithm.h>

#include <set>

///////////////////////////////////////////////////////////////////

// ------------------------
/**
   @builtin ResolvableInstallArchVersion
   @short Install all resolvables with selected name, architecture and kind. Use it only in a special case, ResolvableInstall() should be prefrerred.
   @param name_r name of the resolvable, if empty ("") install all resolvables of the kind 
   @param kind_r kind of resolvable, can be `product, `patch, `package, `selection or `pattern
   @param arch architecture of the resolvable
   @param vers Required version of the resolvable, empty string means any version
   @return boolean false if failed
*/
YCPValue
PkgModuleFunctions::ResolvableInstallArchVersion( const YCPString& name_r, const YCPSymbol& kind_r, const YCPString& arch, const YCPString& vers )
{
    zypp::Resolvable::Kind kind;
    
    std::string req_kind = kind_r->symbol ();

    std::string arch_str = arch->value();
    if (arch_str.empty())
	return YCPBoolean (false);

    // ensure installation of the required architecture
    zypp::Arch architecture(arch_str);

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

   std::string version_str = vers->value();

    return YCPBoolean(
	(name_r->value().empty())
	    ? DoProvideAllKind(kind)
	    : DoProvideNameKind (name_r->value(), kind, architecture, version_str)
    );
}

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
    return ResolvableInstallArchVersion(name_r, kind_r, YCPString(zypp_ptr()->architecture().asString()), YCPString(""));
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
   @param force_r remove the transactions even on USER level - default is APPL_HIGH (use true value only if really needed!)
   @return boolean false if failed
*/
YCPValue
PkgModuleFunctions::ResolvableNeutral ( const YCPString& name_r, const YCPSymbol& kind_r, const YCPBoolean& force_r )
{
    zypp::Resolvable::Kind kind;
    
    std::string req_kind = kind_r->symbol();
    std::string name = name_r->value();
    bool force = force_r->value();

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
		if (!it->status().resetTransact(whoWantsIt))
		{
		    ret = false;
		}

		// force neutralization on the user level
		if (force && !it->status().resetTransact(zypp::ResStatus::USER))
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
   @param kind_r kind of resolvable, can be `product, `patch, `package, `selection, `pattern or `language
   @param version version of the resolvable, if empty all versions are returned

   @return list<map<string,any>> list of $[ "name":string, "version":string, "arch":string, "source":integer, "status":symbol, "locked":boolean ] maps
   status is `installed, `selected or `available, source is source ID or -1 if the resolvable is installed in the target
   if status is `available and locked is true then the object is set to taboo,
   if status is `installed and locked is true then the object locked
*/

YCPValue
PkgModuleFunctions::ResolvableProperties(const YCPString& name, const YCPSymbol& kind_r, const YCPString& version)
{
    return ResolvablePropertiesEx (name, kind_r, version, false);
}

/*
   @builtin ResolvableDependencies
   @description
   return list of resolvables with dependencies

   @see ResolvableProperties for more information
*/
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
    else if ( req_kind == "language" ) {
	kind = zypp::ResTraits<zypp::Language>::kind;
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

		std::string resolvable_summary = (*it)->summary();
		if (resolvable_summary.size() > 0)
		{
		    info->add(YCPString("summary"), YCPString((*it)->summary()));
		}

		// status
		std::string stat;

		if (it->status().isToBeInstalled())
		{
		    stat = "selected";
		}
		else if (it->status().isInstalled() || it->status().isSatisfied())
		{
		    stat = it->status().isToBeUninstalled() ? "removed" : "installed";
		}
		else
		{
		    stat = "available";
		}

		info->add(YCPString("status"), YCPSymbol(stat));

		// is the resolvable locked? (Locked or Taboo in the UI)
		info->add(YCPString("locked"), YCPBoolean(it->status().isLocked()));

		// source
		zypp::Source_Ref res_src = (*it)->source();
		info->add(YCPString("source"), YCPInteger(res_src.numericId()));

		// product specific info
		if( req_kind == "product" ) {
		    zypp::Product::constPtr product = boost::dynamic_pointer_cast<const zypp::Product>(it->resolvable());
		    info->add(YCPString("category"), YCPString(product->category()));
		    info->add(YCPString("vendor"), YCPString(product->vendor()));
		    info->add(YCPString("relnotes_url"), YCPString(product->releaseNotesUrl().asString()));

		    std::string product_summary = product->summary();
		    if (product_summary.size() > 0)
		    {
			info->add(YCPString("display_name"), YCPString(product_summary));
		    }

		    std::string product_shortname = product->shortName();
		    if (product_shortname.size() > 0)
		    {
			info->add(YCPString("short_name"), YCPString(product_shortname));
		    }
		    // use summary for the short name if it's defined
		    else if (product_summary.size() > 0)
		    {
			info->add(YCPString("short_name"), YCPString(product_summary));
		    }


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

		    std::list<zypp::Url> pextraUrls = product->extraUrls();
		    if (pextraUrls.size() > 0)
		    {
			YCPList extraUrls;
			for (std::list<zypp::Url>::const_iterator it = pextraUrls.begin(); it != pextraUrls.end(); ++it)
			{
			    extraUrls->add(YCPString(it->asString()));
			}
			info->add(YCPString("extra_urls"), extraUrls);
		    }

		    std::list<zypp::Url> poptionalUrls = product->optionalUrls();
		    if (poptionalUrls.size() > 0)
		    {
			YCPList optionalUrls;
			for (std::list<zypp::Url>::const_iterator it = poptionalUrls.begin(); it != poptionalUrls.end(); ++it)
			{
			    optionalUrls->add(YCPString(it->asString()));
			}
			info->add(YCPString("optional_urls"), optionalUrls);
		    }
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

		// patch specific info
		if ( req_kind == "patch" )
		{
		    zypp::Patch::constPtr patch_ptr = boost::dynamic_pointer_cast<const zypp::Patch>(it->resolvable());
		    
		    info->add(YCPString("interactive"), YCPBoolean(patch_ptr->interactive()));
		    info->add(YCPString("reboot_needed"), YCPBoolean(patch_ptr->reboot_needed()));
		    info->add(YCPString("affects_pkg_manager"), YCPBoolean(patch_ptr->affects_pkg_manager()));
		    info->add(YCPString("is_needed"), YCPBoolean(it->status().isNeeded()));
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

/**
   @builtin ResolvableCountPatches
   @short Count patches which will be selected by ResolvablePreselectPatches() function
   @description
   Only non-optional patches are selected (even when `all parameter is passed!)
   @param kind_r kind of preselected patches, can be `all, `interactive, `reboot_needed or `affects_pkg_manager
   @return integer number of preselected patches
*/
YCPValue
PkgModuleFunctions::ResolvableCountPatches (const YCPSymbol& kind_r)
{
    // only count the patches
    return ResolvableSetPatches(kind_r, false);
}

/**
   @builtin ResolvablePreselectPatches
   @short Preselect patches for auto online update during the installation
   @description
   Only non-optional patches are selected (even when `all parameter is passed!)
   @param kind_r kind of preselected patches, can be `all, `interactive, `reboot_needed or `affects_pkg_manager
   @return integer number of preselected patches
*/
YCPValue
PkgModuleFunctions::ResolvablePreselectPatches (const YCPSymbol& kind_r)
{
    // preselect the patches
    return ResolvableSetPatches(kind_r, true);
}

// helper function
YCPValue
PkgModuleFunctions::ResolvableSetPatches (const YCPSymbol& kind_r, bool preselect)
{
    std::string kind = kind_r->symbol();

    if (kind != "all" && kind != "interactive" && kind != "reboot_needed"
	 && kind != "affects_pkg_manager")
    {
	return YCPError("Pkg::ResolvablePreselectPatches: Wrong parameter '" + kind + "', use: `all, `interactive, `reboot_needed or `affects_pkg_manager", YCPInteger(0LL));
    }

    y2milestone("Required kind of patches: %s", kind.c_str());

    std::set<zypp::PoolItem> patches_toinst;

    const zypp::ResPool & pool = zypp_ptr()->pool();

    try
    {
	zypp::ResPool::byKind_iterator
	    b = pool.byKindBegin<zypp::Patch>(),
	    e = pool.byKindEnd<zypp::Patch>(),
	    i = pool.byKindBegin<zypp::Patch>();

	for (; i != e; ++i)
	{
	    zypp::Patch::constPtr pch = zypp::asKind<zypp::Patch>(i->resolvable());
	    std::string name(pch->name());

	    // the best patch for the current arch, no preferred version, only needed patches
	    ProvideProcess info(zypp_ptr()->architecture(), "", true);

	    // search the best version of the patch to install
	    try
	    {
		info.whoWantsIt = whoWantsIt;

		invokeOnEach( pool.byNameBegin( name ),
			      pool.byNameEnd( name ),
			      zypp::resfilter::ByKind( zypp::ResTraits<zypp::Patch>::kind ),
			      zypp::functor::functorRef<bool,zypp::PoolItem> (info)
		);

		if (!info.item)
		{
		    y2milestone("Patch %s is not applicable", name.c_str());
		    continue;
		}
		else
		{
		    MIL << "Best version of patch " << i->resolvable() << ": " << info.item.resolvable() << std::endl;
		}
	    }
	    catch (...)
	    {
		y2error("Search for best patch '%s' failed", name.c_str());
		continue;
	    }

	    zypp::Patch::constPtr patch = zypp::asKind<zypp::Patch>(info.item.resolvable());

	    // dont auto-install optional patches
	    if (patch->category () != "optional")
	    {
		if (kind == "all"
		    || (kind == "interactive" && patch->interactive())
		    || (kind == "affects_pkg_manager" && patch->affects_pkg_manager())
		    || (kind == "reboot_needed" && patch->reboot_needed())
		)
		{
		    patches_toinst.insert(info.item);
		}
		else
		{
		    y2milestone("Patch (id) %s has not required flag", patch->id().c_str());
		}
	    }
	    else
	    {
		y2milestone("Ignoring optional patch (id): %s", patch->id().c_str());
	    }
	}
    }
    catch (...)
    {
	y2error("An error occurred during patch selection.");
    }

    y2milestone("Found %d patches", patches_toinst.size());

    if (preselect)
    {
	std::set<zypp::PoolItem>::iterator
	    b(patches_toinst.begin()),
	    e(patches_toinst.end()),
	    i(patches_toinst.begin());

	for (i = b; i != e; ++i)
	{
	    if (i->status().setToBeInstalled(whoWantsIt))
	    {
		MIL << "Selecting patch " << i->resolvable() << std::endl;
	    }
	}
    }

    return YCPInteger(patches_toinst.size());
}


/**
   @builtin IsAnyResolvable
   @short Is there any resolvable in the requried state?
   @param symbol kind_r kind of resolvable, can be `product, `patch, `package, `selection, `pattern, `language or `any for any kind of resolvable
   @param symbol status status of resolvable, can be `to_install or `to_remove
   @return boolean true if a resolvable with the requested status was found
*/
YCPValue
PkgModuleFunctions::IsAnyResolvable(const YCPSymbol& kind_r, const YCPSymbol& status)
{
    zypp::Resolvable::Kind kind;

    std::string req_kind = kind_r->symbol ();
    std::string stat_str = status->symbol();

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
    else if ( req_kind == "language" ) {
	kind = zypp::ResTraits<zypp::Language>::kind;
    }
    else if ( req_kind == "any" ) {
	try
	{ 
	    for (zypp::ResPool::const_iterator it = zypp_ptr()->pool().begin();
		it != zypp_ptr()->pool().end();
		++it)
	    {
		if (stat_str == "to_install" && it->status().isToBeInstalled())
		{
		    return YCPBoolean(true);
		}
		else if (stat_str == "to_remove" && it->status().isToBeUninstalled())
		{
		    return YCPBoolean(true);
		}
	    }
	}
	catch (...)
	{
	    y2error("An error occurred during resolvable search.");
	    return YCPNull();
	}

	return YCPBoolean(false); 
    }
    else
    {
	y2error("Pkg::IsAnyResolvable: unknown symbol: %s", req_kind.c_str());
	return YCPNull();
    }


    try
    { 
	for (zypp::ResPool::byKind_iterator it = zypp_ptr()->pool().byKindBegin(kind);
	    it != zypp_ptr()->pool().byKindEnd(kind);
	    ++it)
	{
	    if (stat_str == "to_install" && it->status().isToBeInstalled())
	    {
		return YCPBoolean(true);
	    }
	    else if (stat_str == "to_remove" && it->status().isToBeUninstalled())
	    {
		return YCPBoolean(true);
	    }
	}
    }
    catch (...)
    {
	y2error("An error occurred during resolvable search.");
	return YCPNull();
    }

    return YCPBoolean(false); 
}
