/* ------------------------------------------------------------------------------
 * Copyright (c) 2007 Novell, Inc. All Rights Reserved.
 *
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of version 2 of the GNU General Public License as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, contact Novell, Inc.
 *
 * To contact Novell about this file by physical or electronic mail, you may find
 * current contact information at www.novell.com.
 * ------------------------------------------------------------------------------
 */

/*
   File:	$Id$
   Author:	Ladislav Slezák <lslezak@novell.com>
   Summary:     Resolvable properties
   Namespace:   Pkg
*/

#include "PkgFunctions.h"
#include "log.h"

#include <ycp/YCPBoolean.h>
#include <ycp/YCPInteger.h>
#include <ycp/YCPSymbol.h>
#include <ycp/YCPString.h>
#include <ycp/YCPList.h>
#include <ycp/YCPMap.h>

#include <zypp/Product.h>
#include <zypp/Patch.h>
#include <zypp/Pattern.h>
#include <zypp/Package.h>

#include <zypp/Dep.h>

/**
   @builtin ResolvableProperties

   @short Return properties of resolvable
   @description
   return list of resolvables of selected kind with required name
 
   @param name name of the resolvable, if empty returns all resolvables of the kind
   @param kind_r kind of resolvable, can be `product, `patch, `package, `pattern or `language
   @param version version of the resolvable, if empty all versions are returned

   @return list<map<string,any>> list of $[ "name":string, "version":string, "arch":string, "source":integer, "status":symbol, "locked":boolean ] maps
   status is `installed, `removed, `selected or `available, source is source ID or -1 if the resolvable is installed in the target
   if status is `available and locked is true then the object is set to taboo,
   if status is `installed and locked is true then the object locked
*/

YCPValue
PkgFunctions::ResolvableProperties(const YCPString& name, const YCPSymbol& kind_r, const YCPString& version)
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
PkgFunctions::ResolvableDependencies(const YCPString& name, const YCPSymbol& kind_r, const YCPString& version)
{
    return ResolvablePropertiesEx (name, kind_r, version, true);
}

YCPValue
PkgFunctions::ResolvablePropertiesEx(const YCPString& name, const YCPSymbol& kind_r, const YCPString& version, bool dependencies)
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
    else if ( req_kind == "pattern" ) {
	kind = zypp::ResTraits<zypp::Pattern>::kind;
    }
    else
    {
	y2error("Pkg::ResolvableProperties: unknown symbol: %s", req_kind.c_str());
	return ret;
    }

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

		if (it->status().isInstalled())
		{
		    stat = (it->status().isToBeUninstalled()) ? "removed" : "installed";
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

		// is the resolvable locked? (Locked or Taboo in the UI)
		info->add(YCPString("locked"), YCPBoolean(it->status().isLocked()));

		// source
		zypp::Repository repo = (*it)->repository();
		info->add(YCPString("source"), YCPInteger(logFindAlias(repo.info().alias())));

		// add license info if it is defined
		std::string license = (*it)->licenseToConfirm();
		if (!license.empty())
		{
		    info->add(YCPString("license_confirmed"), YCPBoolean(it->status().isLicenceConfirmed()));
		    info->add(YCPString("license"), YCPString(license));
		}

		info->add(YCPString("download_size"), YCPInteger((*it)->downloadSize()));
		info->add(YCPString("inst_size"), YCPInteger((*it)->size()));

		info->add(YCPString("medium_nr"), YCPInteger((*it)->mediaNr()));
		info->add(YCPString("vendor"), YCPString((*it)->vendor()));

		
		// package specific info
		if( req_kind == "package" )
		{
		    zypp::Package::constPtr pkg = boost::dynamic_pointer_cast<const zypp::Package>(it->resolvable());

		    std::string tmp = pkg->location().filename().asString();
		    if (!tmp.empty())
		    {
			info->add(YCPString("path"), YCPString(tmp));
		    }

		    tmp = pkg->location().filename().basename();
		    if (!tmp.empty())
		    {
			info->add(YCPString("location"), YCPString(tmp));
		    }
		}
		// product specific info
		else if( req_kind == "product" ) {
		    zypp::Product::constPtr product = boost::dynamic_pointer_cast<const zypp::Product>(it->resolvable());
#warning "Product::category is deprecated, remove from YCP code and this map"
                    info->add(YCPString("category"), YCPString(product->type()));
                    info->add(YCPString("type"), YCPString(product->type()));
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

		    info->add(YCPString("type"), YCPString(product->type()));

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
		else if ( req_kind == "pattern" ) {
		    zypp::Pattern::constPtr pattern = boost::dynamic_pointer_cast<const zypp::Pattern>(it->resolvable());
		    info->add(YCPString("category"), YCPString(pattern->category()));
		    info->add(YCPString("user_visible"), YCPBoolean(pattern->userVisible()));
		    info->add(YCPString("default"), YCPBoolean(pattern->isDefault()));
		    info->add(YCPString("icon"), YCPString(pattern->icon().asString()));
		    info->add(YCPString("script"), YCPString(pattern->script().asString()));
		}
		// patch specific info
		else if ( req_kind == "patch" )
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
			    zypp::Capabilities deps = it->resolvable()->dep(depkind);
			    for (zypp::Capabilities::const_iterator d = deps.begin(); d != deps.end(); ++d)
			    {
				YCPMap ycpdep;
				//FIXME ycpdep->add (YCPString ("res_kind"), YCPString (d.kind().asString()));
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
   @builtin IsAnyResolvable
   @short Is there any resolvable in the requried state?
   @param symbol kind_r kind of resolvable, can be `product, `patch, `package, `pattern, `language or `any for any kind of resolvable
   @param symbol status status of resolvable, can be `to_install or `to_remove
   @return boolean true if a resolvable with the requested status was found
*/
YCPValue
PkgFunctions::IsAnyResolvable(const YCPSymbol& kind_r, const YCPSymbol& status)
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
    else if ( req_kind == "pattern" ) {
	kind = zypp::ResTraits<zypp::Pattern>::kind;
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

