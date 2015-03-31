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
#include "ycpTools.h"

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
#include <zypp/SrcPackage.h>
#include <zypp/ui/Status.h>

#include <zypp/Dep.h>
#include <zypp/sat/LocaleSupport.h>
#include <zypp/parser/ProductFileReader.h>
#include <zypp/base/Regex.h>

/**
   @builtin ResolvableProperties

   @short Return properties of resolvable
   @description
   return list of resolvables of selected kind with required name

   @param name name of the resolvable, if empty returns all resolvables of the kind
   @param kind_r kind of resolvable, can be `product, `patch, `package, `pattern or `language
   @param version version of the resolvable, if empty all versions are returned

   @return list<map<string,any>> list of $[ "name":string, "version":string,
   "version_epoch":integer (nil if not defined), "version_version":string, "version_release":string,
   "arch":string, "source":integer, "status":symbol, "locked":boolean, "on_system_by_user":boolean ] maps
   status is `installed, `removed, `selected or `available, source is source ID or -1 if the resolvable is installed in the target
   if status is `available and locked is true then the object is set to taboo,
   if status is `installed and locked is true then the object locked
   if status is `selected or `removed there is extra key "transact_by" : symbol, where symbol is `user (the highest level),
       `app_high (selected by Yast), `app_low and `solver (the lowest level)
   on_system_by_user shows if the resolvable has been installed by user(USER,APPL_HIGH,APPL_LOW) or due solved dependencies. This information comes from
     the solver which cannot distinguis between the state USER,APPL_HIGH and APPL_LOW.

     "version" value contains edition with all components in form "[epoch:]version[-release]",
     "version_epoch", "version_version" and "version_release" values contain the parts of the edition.

     Additionally to keys returned for all resolvables, there also some
     resolvable-specific ones:

     `product keys:
        + "category"
        + "display_name"
        + "short_name"
        + "eol" (optional) -> integer: Unix time when the product reaches EndOfLife,
                (i.e. is out of support)
        + "update_urls"
        + "flags"
        + "extra_urls"
        + "optional_urls"
        + "register_urls"
	+ "relnotes_urls"
        + "smolt_urls"
	+ "register_target"
	+ "register_release"
	+ "flavor"
        + "replaces"
          + "name"
          + "version"
          + "arch"
          + "description"
          + "display_name"
          + "short_name"
	+ "product_file" -> string : product file (full file name with target root prefix) (only for installed products)
	+ "product_package" -> string : package name providing the product (only for available products)
	+ "upgrades" -> list<map> : parsed data from the product file (upgrades section)
	  + "name" -> string
	  + "summary" -> string
	  + "repository" -> string : URL path
	  + "notify"  -> boolean
	  + "status" -> string
     `patch keys:
        + "interactive"
        + "reboot_needed"
        + "relogin_needed"
        + "affects_pkg_manager"
        + "is_needed"
     `package keys:
        + "path"
        + "location"
     `srcpackage keys:
        + "path"
        + "location"
	+ "src_type" -> string : "src" or "nosrc" type
     `pattern keys:
        + "category"
        + "user_visible"
        + "default"
        + "icon"
        + "script"
        + "order"
     `language keys:
        + "code"
        + "packages"
        + "requested"

     If dependencies are requested, this keys are additionally used:
        + "provides"
        + "prerequires"
        + "requires"
        + "conflicts"
        + "obsoletes"
        + "recommends"
        + "suggests"
        + "enhances"
        + "supplements"

     All the dependencies use maps with these keys:
        + "res_kind"
        + "name"
        + "dep_kind"
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

YCPMap PkgFunctions::Resolvable2YCPMap(const zypp::PoolItem &item, const std::string &req_kind, bool dependencies)
{
    YCPMap info;

    info->add(YCPString("name"), YCPString(item->name()));
    // complete edition: [epoch:]version[-release]
    info->add(YCPString("version"), YCPString(item->edition().asString()));

    // parts of the edition
    if (item->edition().epoch() == zypp::Edition::noepoch)
        info->add(YCPString("version_epoch"), YCPVoid());
    else
        info->add(YCPString("version_epoch"), YCPInteger(item->edition().epoch()));
    info->add(YCPString("version_version"), YCPString(item->edition().version()));
    info->add(YCPString("version_release"), YCPString(item->edition().release()));

    info->add(YCPString("arch"), YCPString(item->arch().asString()));
    info->add(YCPString("description"), YCPString(item->description()));

    std::string resolvable_summary = item->summary();
    if (resolvable_summary.size() > 0)
    {
	info->add(YCPString("summary"), YCPString(resolvable_summary));
    }

    // status
    std::string stat;

    zypp::ResStatus status = item.status();

    if (status.isToBeInstalled())
    {
	stat = "selected";
    }
    else if (status.isInstalled() || status.isSatisfied())
    {
	if (status.isToBeUninstalled())
	{
	    stat = "removed";
	}
	else
	{
	    stat = "installed";
	}
    }
    else
    {
	stat = "available";
    }

    info->add(YCPString("transact_by"), YCPSymbol(TransactToString(status.getTransactByValue())));

    info->add(YCPString("on_system_by_user"), YCPBoolean(item.satSolvable().onSystemByUser()));

    info->add(YCPString("status"), YCPSymbol(stat));

    // is the resolvable locked? (Locked or Taboo in the UI)
    info->add(YCPString("locked"), YCPBoolean(status.isLocked()));

    // source
    info->add(YCPString("source"), YCPInteger(logFindAlias(item->repoInfo().alias())));

    // add license info if it is defined
    std::string license = item->licenseToConfirm();
    if (!license.empty())
    {
	info->add(YCPString("license_confirmed"), YCPBoolean(item.status().isLicenceConfirmed()));
	info->add(YCPString("license"), YCPString(license));
    }

    info->add(YCPString("download_size"), YCPInteger(item->downloadSize()));
    info->add(YCPString("inst_size"), YCPInteger(item->installSize()));

    info->add(YCPString("medium_nr"), YCPInteger(item->mediaNr()));
    info->add(YCPString("vendor"), YCPString(item->vendor()));


    // package specific info
    if( req_kind == "package" )
    {
	zypp::Package::constPtr pkg = boost::dynamic_pointer_cast<const zypp::Package>(item.resolvable());
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
	    y2error("package %s is not a package", item->name().c_str() );
	}
    }
    else if( req_kind == "srcpackage" )
    {
	zypp::SrcPackage::constPtr pkg = boost::dynamic_pointer_cast<const zypp::SrcPackage>(item.resolvable());
	if (pkg)
	{
	    std::string tmp(pkg->location().filename().asString());
	    if (!tmp.empty())
	    {
		info->add(YCPString("path"), YCPString(tmp));
	    }

	    tmp = pkg->location().filename().basename();
	    if (!tmp.empty())
	    {
		info->add(YCPString("location"), YCPString(tmp));
	    }

	    info->add(YCPString("src_type"), YCPString(pkg->sourcePkgType()));
	}
	else
	{
	    y2error("%s is not a srcpackage", item->name().c_str() );
	}
    }
    // product specific info
    else if( req_kind == "product" ) {
	zypp::Product::constPtr product = boost::dynamic_pointer_cast<const zypp::Product>(item.resolvable());
	if ( !product )
	{
	    y2error("product %s is not a product", item->name().c_str() );
	    return YCPMap();
	}

	std::string category(product->isTargetDistribution() ? "base" : "addon");

	info->add(YCPString("category"), YCPString(category));
	info->add(YCPString("type"), YCPString(category));
	info->add(YCPString("relnotes_url"), YCPString(product->releaseNotesUrls().first().asString()));

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

        zypp::Date eol = product->endOfLife();
        if (eol > 0)
        {
          info->add(YCPString("eol"), YCPInteger(eol));
        }

	YCPList updateUrls(asYCPList(product->updateUrls()));
	info->add(YCPString("update_urls"), updateUrls);

	YCPList flags;
	std::list<std::string> pflags = product->flags();
	for (std::list<std::string>::const_iterator flag_it = pflags.begin();
	    flag_it != pflags.end(); ++flag_it)
	{
	    flags->add(YCPString(*flag_it));
	}
	info->add(YCPString("flags"), flags);

	YCPList extraUrls( asYCPList(product->extraUrls()) );
	if ( extraUrls.size() )
	{
	  info->add(YCPString("extra_urls"), extraUrls);
	}

	YCPList optionalUrls( asYCPList(product->optionalUrls()) );
	if ( optionalUrls.size() )
	{
	  info->add(YCPString("optional_urls"), optionalUrls);
	}

	YCPList registerUrls( asYCPList(product->registerUrls()) );
	if ( registerUrls.size() )
	{
	  info->add(YCPString("register_urls"), registerUrls);
	}

	YCPList smoltUrls( asYCPList(product->smoltUrls()) );
	if ( smoltUrls.size() )
	{
	  info->add(YCPString("smolt_urls"), smoltUrls);
	}

	YCPList relNotesUrls(asYCPList(product->releaseNotesUrls()));
	if ( relNotesUrls.size() )
	{
	  info->add(YCPString("relnotes_urls"), relNotesUrls);
	}

	// registration data
	info->add(YCPString("register_target"), YCPString(product->registerTarget()));
	info->add(YCPString("register_release"), YCPString(product->registerRelease()));

	// Live CD, FTP Edition...
	info->add(YCPString("flavor"), YCPString(product->flavor()));

	// get the installed Products it would replace.
	zypp::Product::ReplacedProducts replaced(product->replacedProducts());

	if (!replaced.empty())
	{
	    YCPList rep_prods;

	    // add the products to the list
	    for_( it, replaced.begin(), replaced.end() )
	    {
		// The current replaced Product.
		zypp::Product::constPtr replacedProduct(*it);

		if (!replacedProduct) continue;

		YCPMap rprod;
		rprod->add(YCPString("name"), YCPString(replacedProduct->name()));
		rprod->add(YCPString("version"), YCPString(replacedProduct->edition().asString()));
		rprod->add(YCPString("arch"), YCPString(replacedProduct->arch().asString()));
		rprod->add(YCPString("description"), YCPString(replacedProduct->description()));

		std::string product_summary = replacedProduct->summary();
		if (product_summary.size() > 0)
		{
		    rprod->add(YCPString("display_name"), YCPString(product_summary));
		}

		std::string product_shortname = replacedProduct->shortName();
		if (product_shortname.size() > 0)
		{
		    rprod->add(YCPString("short_name"), YCPString(product_shortname));
		}
		// use summary for the short name if it's defined
		else if (product_summary.size() > 0)
		{
		    rprod->add(YCPString("short_name"), YCPString(product_summary));
		}
	    }

	    info->add(YCPString("replaces"), rep_prods);
	}

	std::string product_file;

	// add reference file in /etc/products.d
	if (status.isInstalled())
	{
	    product_file = (_target_root + "/etc/products.d/" + product->referenceFilename()).asString();
	    y2milestone("Parsing product file %s", product_file.c_str());
	    const zypp::parser::ProductFileData productFileData = zypp::parser::ProductFileReader::scanFile(product_file);

	    YCPList upgrade_list;

	    for_( upit, productFileData.upgrades().begin(), productFileData.upgrades().end() )
	    {
	      const zypp::parser::ProductFileData::Upgrade & upgrade( *upit );

	      YCPMap upgrades;
	      upgrades->add(YCPString("name"), YCPString(upgrade.name()));
	      upgrades->add(YCPString("summary"), YCPString(upgrade.summary()));
	      upgrades->add(YCPString("repository"), YCPString(upgrade.repository()));
	      upgrades->add(YCPString("notify"), YCPBoolean(upgrade.notify()));
	      upgrades->add(YCPString("status"), YCPString(upgrade.status()));
	      upgrades->add(YCPString("product"), YCPString(upgrade.product()));

	      upgrade_list->add(upgrades);
	    }

	    info->add(YCPString("upgrades"), upgrade_list);
	}
	else
	{
	    // get the package
	    zypp::sat::Solvable refsolvable = product->referencePackage();

	    if (refsolvable != zypp::sat::Solvable::noSolvable)
	    {
		// create a package pointer from the SAT solvable
		zypp::Package::Ptr refpkg(zypp::make<zypp::Package>(refsolvable));

		if (refpkg)
		{
		    info->add(YCPString("product_package"), YCPString(refpkg->name()));

		    // get the package files
		    zypp::Package::FileList files( refpkg->filelist() );
		    y2milestone("The reference package has %d files", files.size());

		    zypp::str::smatch what;
		    const zypp::str::regex product_file_regex("^/etc/products\\.d/(.*\\.prod)$");

		    // find the product file
		    for_(iter, files.begin(), files.end())
		    {
			if (zypp::str::regex_match(*iter, what, product_file_regex))
			{
			    product_file = what[1];
			    break;
			}
		    }
		}
	    }
	}

	if (product_file.empty())
	{
	    y2warning("The product file has not been found");
	}
	else
	{
	    y2milestone("Found product file %s", product_file.c_str());
	    info->add(YCPString("product_file"), YCPString(product_file));
	}
    }
    // pattern specific info
    else if ( req_kind == "pattern" ) {
	zypp::Pattern::constPtr pattern = boost::dynamic_pointer_cast<const zypp::Pattern>(item.resolvable());
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
	zypp::Patch::constPtr patch_ptr = boost::dynamic_pointer_cast<const zypp::Patch>(item.resolvable());

	info->add(YCPString("interactive"), YCPBoolean(patch_ptr->interactive()));
	info->add(YCPString("reboot_needed"), YCPBoolean(patch_ptr->rebootSuggested()));
	info->add(YCPString("relogin_needed"), YCPBoolean(patch_ptr->reloginSuggested()));
	info->add(YCPString("affects_pkg_manager"), YCPBoolean(patch_ptr->restartSuggested()));
	info->add(YCPString("is_needed"), YCPBoolean(item.isBroken()));
        // names and versions of packages, contained in the patch
        YCPMap contents;
        zypp::Patch::Contents c( patch_ptr->contents() );
        for_( it, c.begin(), c.end() )
        {
          contents->add (YCPString (it->name()), YCPString (it->edition().c_str()));
        }
        info->add(YCPString("contents"), contents);
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
	YCPList rawdeps;
	for (std::set<std::string>::const_iterator kind_it = _kinds.begin();
	    kind_it != _kinds.end(); ++kind_it)
	{
            zypp::Dep depkind(*kind_it);
            zypp::Capabilities deps = item.resolvable()->dep(depkind);

            // add raw dependencies
            for_(it, deps.begin(), deps.end())
            {
                YCPMap rawdep;
                rawdep->add(YCPString(*kind_it), YCPString(it->asString()));
                rawdeps->add(rawdep);
            }

            zypp::sat::WhatProvides prv(deps);

            // resolve dependencies
            for (zypp::sat::WhatProvides::const_iterator d = prv.begin(); d != prv.end(); ++d)
            {
                if (d->kind().asString().empty() || d->name().empty())
                {
                    y2debug("Empty kind or name: kind: %s, name: %s", d->kind().asString().c_str(), d->name().c_str());
                }
                else
                {
                    YCPMap ycpdep;
                    ycpdep->add (YCPString ("res_kind"), YCPString (d->kind().asString()));
                    ycpdep->add (YCPString ("name"), YCPString (d->name()));
                    ycpdep->add (YCPString ("dep_kind"), YCPString (*kind_it));

                    if (!ycpdeps.contains(ycpdep))
                    {
                        ycpdeps->add (ycpdep);
                    }
                }
            }
	}

	if (ycpdeps.size() > 0)
	{
	    info->add (YCPString ("dependencies"), ycpdeps);
	}

	if (rawdeps.size() > 0)
	{
	    info->add (YCPString ("deps"), rawdeps);
	}
    }

    return info;
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
    else if ( req_kind == "srcpackage" ) {
	kind = zypp::ResKind::srcpackage;
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
	    zypp::ui::Selectable::Ptr s = (*it);

	    if (nm.empty() || nm == s->name())
	    {
                try
                {
                    if (!s->installedEmpty())
                    {
                        // iterate over all installed packages
                        for_(inst_it, s->installedBegin(), s->installedEnd())
                        {
                            // check version if required
                            if (vers.empty() || vers == inst_it->resolvable()->edition().asString())
                            {
                                ret->add(Resolvable2YCPMap(*inst_it, req_kind, dependencies));
                            }
                        }
                    }

                    if (!s->availableEmpty())
                    {
                        // iterate over all available packages
                        for_(avail_it, s->availableBegin(), s->availableEnd())
                        {
                            // check version if required
                            if (vers.empty() || vers == avail_it->resolvable()->edition().asString())
                            {
                                ret->add(Resolvable2YCPMap(*avail_it, req_kind, dependencies));
                            }
                        }
                    }
                }
                catch(const zypp::Exception &expt)
                {
                    y2error("ResolvableProperties for \"%s\" failed: %s",
                        s->name().c_str(), expt.asString().c_str());
                    _last_error.setLastError(ExceptionAsString(expt));
                    return YCPVoid();
                }
            }
	}
    }
    catch(const zypp::Exception &expt)
    {
        y2error("ResolvableProperties failed: %s", expt.asString().c_str());
        _last_error.setLastError(ExceptionAsString(expt));
        return YCPVoid();
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

