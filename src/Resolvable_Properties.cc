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
#include <ycp/YCPVoid.h>

#include <zypp/Product.h>
#include <zypp/Patch.h>
#include <zypp/Pattern.h>
#include <zypp/Package.h>
#include <zypp/ui/Status.h>

#include <zypp/Dep.h>
#include <zypp/sat/LocaleSupport.h>

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
   if status is `selected or `removed there is extra key "transact_by" : symbol, where symbol is `user (the highest level),
       `app_high (selected by Yast), `app_low and `solver (the lowest level)
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

std::string TransactToString(zypp::ResStatus::TransactByValue trans)
{
    std::string ret;

    switch(trans)
    {
	case zypp::ResStatus::USER : ret = "user"; break;
	case zypp::ResStatus::APPL_HIGH : ret = "app_high"; break;
	case zypp::ResStatus::APPL_LOW : ret = "app_low"; break;
	case zypp::ResStatus::SOLVER : ret = "solver"; break;
    }

    return ret;
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
    	kind = zypp::ResKind::product;
    }
    else if ( req_kind == "patch" ) {
    	kind = zypp::ResKind::patch;
    }
    else if ( req_kind == "package" ) {
	kind = zypp::ResKind::package;
    }
    else if ( req_kind == "pattern" ) {
	kind = zypp::ResKind::pattern;
    }
    else if ( req_kind == "language" )
    {
	try
	{
	    const zypp::LocaleSet &avlocales( zypp::ResPool::instance().getAvailableLocales() );

	    for_( it, avlocales.begin(), avlocales.end() )
	    {
		zypp::sat::LocaleSupport myLocale( *it );

		YCPMap lang_map;

		lang_map->add(YCPString("name"), YCPString(myLocale.locale().name()));
		lang_map->add(YCPString("code"), YCPString(myLocale.locale().code()));
		lang_map->add(YCPString("packages"), YCPBoolean(myLocale.isAvailable()));
		lang_map->add(YCPString("requested"), YCPBoolean(myLocale.isRequested()));

		ret->add(lang_map);
	    }
	}
	catch(const zypp::Exception &expt)
	{
	    y2error("Caught exception: %s", expt.asString().c_str());
	    _last_error.setLastError(ExceptionAsString(expt));
	    return YCPVoid();
	}

	return ret;
    }
    else
    {
	y2error("Pkg::ResolvableProperties: unknown symbol: %s", req_kind.c_str());
	return ret;
    }

   try
   {
	for (zypp::ResPoolProxy::const_iterator it = zypp_ptr()->poolProxy().byKindBegin(kind);
	    it != zypp_ptr()->poolProxy().byKindEnd(kind);
	    ++it)
	{
	    if ((nm.empty() || nm == (*it)->theObj()->name())
		&& (vers.empty() || vers == (*it)->theObj()->edition().asString()))
	    {
		YCPMap info;

		info->add(YCPString("name"), YCPString((*it)->theObj()->name()));
		info->add(YCPString("version"), YCPString((*it)->theObj()->edition().asString()));
		info->add(YCPString("arch"), YCPString((*it)->theObj()->arch().asString()));
		info->add(YCPString("description"), YCPString((*it)->theObj()->description()));

		std::string resolvable_summary = (*it)->theObj()->summary();
		if (resolvable_summary.size() > 0)
		{
		    info->add(YCPString("summary"), YCPString(resolvable_summary));
		}

		// status
		std::string stat;

		// selectable status
		zypp::ui::Status status = (*it)->status();

		switch((*it)->fate())
		{
		    case zypp::ui::Selectable::TO_INSTALL:
			stat = "selected";
			// add the transaction data
			info->add(YCPString("transact_by"), YCPSymbol(TransactToString((*it)->theObj().status().getTransactByValue())));
			break;
		    case zypp::ui::Selectable::TO_DELETE:
			stat = "removed";
			// add the transaction data
			info->add(YCPString("transact_by"), YCPSymbol(TransactToString((*it)->theObj().status().getTransactByValue())));
			break;
		    case zypp::ui::Selectable::UNMODIFIED:
			stat = (status == zypp::ui::S_KeepInstalled
			     || status == zypp::ui::S_Protected) ? "installed" : "available";
			break;
		}
			
		info->add(YCPString("status"), YCPSymbol(stat));
		info->add(YCPString("status_detail"), YCPSymbol(zypp::ui::asString(status)));

		// is the resolvable locked? (Locked or Taboo in the UI)
		info->add(YCPString("locked"), YCPBoolean(status == zypp::ui::S_Protected
		    || status == zypp::ui::S_Taboo));

		// source
		info->add(YCPString("source"), YCPInteger(logFindAlias((*it)->theObj()->repoInfo().alias())));

		// add license info if it is defined
		std::string license = (*it)->theObj()->licenseToConfirm();
		if (!license.empty())
		{
		    info->add(YCPString("license_confirmed"), YCPBoolean((*it)->theObj().status().isLicenceConfirmed()));
		    info->add(YCPString("license"), YCPString(license));
		}

		info->add(YCPString("download_size"), YCPInteger((*it)->theObj()->downloadSize()));
		info->add(YCPString("inst_size"), YCPInteger((*it)->theObj()->installSize()));

		info->add(YCPString("medium_nr"), YCPInteger((*it)->theObj()->mediaNr()));
		info->add(YCPString("vendor"), YCPString((*it)->theObj()->vendor()));


		// package specific info
		if( req_kind == "package" )
		{
		    zypp::Package::constPtr pkg = boost::dynamic_pointer_cast<const zypp::Package>((*it)->theObj().resolvable());
                    if ( pkg )
		    {
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
                    } else
                    {
                        y2error("package %s is not a package", (*it)->name().c_str() );
                    }
		}
		// product specific info
		else if( req_kind == "product" ) {
		    zypp::Product::constPtr product = boost::dynamic_pointer_cast<const zypp::Product>((*it)->theObj().resolvable());
                    if ( !product )
                    {
                        y2error("product %s is not a product", (*it)->name().c_str() );
                        continue;
                    }
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
		    zypp::Pattern::constPtr pattern = boost::dynamic_pointer_cast<const zypp::Pattern>((*it)->theObj().resolvable());
		    info->add(YCPString("category"), YCPString(pattern->category()));
		    info->add(YCPString("user_visible"), YCPBoolean(pattern->userVisible()));
		    info->add(YCPString("default"), YCPBoolean(pattern->isDefault()));
		    info->add(YCPString("icon"), YCPString(pattern->icon().asString()));
		    info->add(YCPString("script"), YCPString(pattern->script().asString()));
		    info->add(YCPString("order"), YCPString(pattern->order()));
		}
		// patch specific info
		else if ( req_kind == "patch" )
		{
		    zypp::Patch::constPtr patch_ptr = boost::dynamic_pointer_cast<const zypp::Patch>((*it)->theObj().resolvable());

		    info->add(YCPString("interactive"), YCPBoolean(patch_ptr->interactive()));
		    info->add(YCPString("reboot_needed"), YCPBoolean(patch_ptr->rebootSuggested()));
		    info->add(YCPString("affects_pkg_manager"), YCPBoolean(patch_ptr->restartSuggested()));
                    info->add(YCPString("is_needed"), YCPBoolean((*it)->theObj().isBroken()));
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
		    _kinds.insert("enhances");
		    _kinds.insert("supplements");
		    YCPList ycpdeps;
		    for (std::set<std::string>::const_iterator kind_it = _kinds.begin();
			kind_it != _kinds.end(); ++kind_it)
		    {
			try {
			    zypp::Dep depkind(*kind_it);
			    zypp::Capabilities deps = (*it)->theObj().resolvable()->dep(depkind);

			    zypp::sat::WhatProvides prv(deps);

			    for (zypp::sat::WhatProvides::const_iterator d = prv.begin(); d != prv.end(); ++d)
			    {
				YCPMap ycpdep;
				ycpdep->add (YCPString ("res_kind"), YCPString (d->kind().asString()));
				ycpdep->add (YCPString ("name"), YCPString (d->name()));
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

bool AnyResolvableHelper(zypp::Resolvable::Kind kind, bool to_install)
{
    for (zypp::ResPoolProxy::const_iterator it = zypp::ResPool::instance().proxy().byKindBegin(kind);
	it != zypp::ResPool::instance().proxy().byKindEnd(kind);
	++it)
    {
	zypp::ui::Selectable::Fate fate = (*it)->fate();

	if (to_install && fate == zypp::ui::Selectable::TO_INSTALL)
	{
	    return true;
	}
	else if (!to_install && fate == zypp::ui::Selectable::TO_DELETE)
	{
	    return true;
	}
    }

    return false;
}

/**
   @builtin IsAnyResolvable
   @short Is there any resolvable in the requried state?
   @param symbol kind_r kind of resolvable, can be `product, `patch, `package, `pattern or `any for any kind of resolvable
   @param symbol status status of resolvable, can be `to_install or `to_remove
   @return boolean true if a resolvable with the requested status was found
*/
YCPValue
PkgFunctions::IsAnyResolvable(const YCPSymbol& kind_r, const YCPSymbol& status)
{
    zypp::Resolvable::Kind kind;

    if (kind_r.isNull() || status.isNull())
    {
	y2error("Invalid nil parameter!");
	return YCPVoid();
    }

    std::string req_kind = kind_r->symbol ();
    std::string stat_str = status->symbol();

    if (stat_str != "to_install" && stat_str != "to_remove")
    {
	y2error("Invalid status parameter: %s", stat_str.c_str());
	return YCPVoid();
    }

    bool to_install = stat_str == "to_install";

    if( req_kind == "product" ) {
	kind = zypp::ResKind::product;
    }
    else if ( req_kind == "patch" ) {
    	kind = zypp::ResKind::patch;
    }
    else if ( req_kind == "package" ) {
	kind = zypp::ResKind::package;
    }
    else if ( req_kind == "pattern" ) {
	kind = zypp::ResKind::pattern;
    }
    else if ( req_kind == "any" ) {
	try
	{
	    return YCPBoolean(
		AnyResolvableHelper(zypp::ResKind::package, to_install)
		|| AnyResolvableHelper(zypp::ResKind::patch, to_install)
		|| AnyResolvableHelper(zypp::ResKind::product, to_install)
		|| AnyResolvableHelper(zypp::ResKind::pattern, to_install)
	    );
	}
	catch (...)
	{
	    y2error("An error occurred during resolvable search.");
	    return YCPVoid();
	}
    }
    else
    {
	y2error("Pkg::IsAnyResolvable: unknown symbol: %s", req_kind.c_str());
	return YCPVoid();
    }

    try
    {
	return YCPBoolean(AnyResolvableHelper(kind, to_install));
    }
    catch (...)
    {
	y2error("An error occurred during resolvable search.");
	return YCPVoid();
    }
}

