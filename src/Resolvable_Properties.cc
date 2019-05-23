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

   **Obsolete**

   This call is obsolete, use `Resolvables()` call instead, it has more filtering
   options and allows to return only the selected keys (saves memory and time).

   **Warning**

   Calling `ResolvableProperties("", :package, "")` variant is memory expansive
   esp. when there are repositories with too many packages (e.g. the OpenSUSE
   OSS repository contains ~40,000 packages).

   It is recommended to use the `Resolvables()` call instead and use a more specific
   input filter. If you only need a boolean result if a certain resolvable exists
   then use the `AnyResolvable` call.

   See bsc#106768.

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
     the solver which cannot distinguish between the state USER,APPL_HIGH and APPL_LOW.

     "version" value contains edition with all components in form "[epoch:]version[-release]",
     "version_epoch", "version_version" and "version_release" values contain the parts of the edition.

	 The "kind" attribute contains the kind of the resolvable, it can be either :package, :patch, :product, :srcpackage or :pattern.

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
	y2warning("Pkg::ResolvableProperties() is obsolete.");
	y2warning("Use Pkg::Resolvables({name: ..., kind: ...}, [...]) instead.");

	return ResolvablePropertiesEx (name, kind_r, version, true, false, YCPList());
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
	y2warning("Pkg::ResolvableDependencies() is obsolete.");
	y2warning("Use Pkg::Resolvables({name: ..., kind: ...}, [:dependencies, ...]) instead.");

	return ResolvablePropertiesEx (name, kind_r, version, true, true, YCPList());
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

YCPMap PkgFunctions::Resolvable2YCPMap(const zypp::PoolItem &item, bool all, bool deps, const YCPList &attrs)
{
    YCPMap info;

// define some helper macros
#define ADD_STRING(X, V) \
	if (all || attrs->contains(YCPSymbol(X))) \
		info->add(YCPString(X), YCPString(V));
#define ADD_BOOLEAN(X, V) \
	if (all || attrs->contains(YCPSymbol(X))) \
		info->add(YCPString(X), YCPBoolean(V));
#define ADD_INTEGER(X, V) \
	if (all || attrs->contains(YCPSymbol(X))) \
		info->add(YCPString(X), YCPInteger(V));
#define ADD_SYMBOL(X, V) \
	if (all || attrs->contains(YCPSymbol(X))) \
		info->add(YCPSymbol(X), YCPSymbol(V));

	ADD_STRING("name", item->name());
    // complete edition: [epoch:]version[-release]
	ADD_STRING("version", item->edition().asString());
	ADD_STRING("version_version", item->edition().version());
	ADD_STRING("version_release", item->edition().release());

    // parts of the edition
	if (all || attrs->contains(YCPSymbol("version_epoch")))
	{
		if (item->edition().epoch() == zypp::Edition::noepoch)
			info->add(YCPString("version_epoch"), YCPVoid());
		else
			info->add(YCPString("version_epoch"), YCPInteger(item->edition().epoch()));
	}

	ADD_STRING("arch", item->arch().asString());
	ADD_STRING("description", item->description());

    std::string resolvable_summary = item->summary();
	if ((all && !resolvable_summary.empty()) || attrs->contains(YCPSymbol("summary")))
		info->add(YCPString("summary"), YCPString(resolvable_summary));

    zypp::ResStatus status = item.status();

    // status
	if (all || attrs->contains(YCPSymbol("status")))
	{
		std::string stat;

		if (status.isToBeInstalled())
			stat = "selected";
		else if (status.isInstalled() || status.isSatisfied())
			stat = (status.isToBeUninstalled()) ? "removed" : "installed";
		else
			stat = "available";

	    info->add(YCPString("status"), YCPSymbol(stat));
	}

	if (all || attrs->contains(YCPSymbol("transact_by")))
    	info->add(YCPString("transact_by"), YCPSymbol(TransactToString(status.getTransactByValue())));

	ADD_BOOLEAN("on_system_by_user", item.satSolvable().onSystemByUser());
    // is the resolvable locked? (Locked or Taboo in the UI)
	ADD_BOOLEAN("locked", status.isLocked());

    // source
	ADD_INTEGER("source", logFindAlias(item->repoInfo().alias()));

    // add license info if it is defined
    std::string license = item->licenseToConfirm();
    if ((all && !license.empty()) || attrs->contains(YCPSymbol("license_confirmed")))
	{
		info->add(YCPString("license_confirmed"), YCPBoolean(item.status().isLicenceConfirmed()));
		info->add(YCPString("license"), YCPString(license));
	}

	if (all || attrs->contains(YCPSymbol("download_size")))
    	info->add(YCPString("download_size"), YCPInteger(item->downloadSize()));
	if (all || attrs->contains(YCPSymbol("inst_size")))
    	info->add(YCPString("inst_size"), YCPInteger(item->installSize()));

	if (all || attrs->contains(YCPSymbol("medium_nr")))
    	info->add(YCPString("medium_nr"), YCPInteger(item->mediaNr()));
	if (all || attrs->contains(YCPSymbol("vendor")))
    	info->add(YCPString("vendor"), YCPString(item->vendor()));


    // package specific info
	zypp::Package::constPtr pkg = zypp::asKind<zypp::Package>(item.resolvable());
	if (pkg)
	{
	    if (all || attrs->contains(YCPSymbol("kind")))
			info->add(YCPString("kind"), YCPSymbol("package"));

		std::string path = pkg->location().filename().asString();
	    if ((all && !path.empty()) || attrs->contains(YCPSymbol("path")))
			info->add(YCPString("path"), YCPString(path));

		std::string location = pkg->location().filename().basename();
	    if ((all && !location.empty()) || attrs->contains(YCPSymbol("location")))
			info->add(YCPString("location"), YCPString(location));
	}

	zypp::SrcPackage::constPtr src_pkg = zypp::asKind<zypp::SrcPackage>(item.resolvable());
	if (src_pkg)
	{
	    if (all || attrs->contains(YCPSymbol("kind")))
			info->add(YCPString("kind"), YCPSymbol("srcpackage"));

		std::string path = src_pkg->location().filename().asString();
	    if ((all && !path.empty()) || attrs->contains(YCPSymbol("path")))
			info->add(YCPString("path"), YCPString(path));

		std::string location = src_pkg->location().filename().basename();
	    if ((all && !location.empty()) || attrs->contains(YCPSymbol("location")))
			info->add(YCPString("location"), YCPString(location));

		if (all || attrs->contains(YCPSymbol("src_type")))
			info->add(YCPString("src_type"), YCPString(src_pkg->sourcePkgType()));
	}

	zypp::Product::constPtr product = zypp::asKind<zypp::Product>(item.resolvable());
	if ( product )
	{
	    if (all || attrs->contains(YCPSymbol("kind")))
			info->add(YCPString("kind"), YCPSymbol("product"));

		std::string category(product->isTargetDistribution() ? "base" : "addon");

		if (all || attrs->contains(YCPSymbol("category")))
			info->add(YCPString("category"), YCPString(category));
		if (all || attrs->contains(YCPSymbol("type")))
			info->add(YCPString("type"), YCPString(category));
		if (all || attrs->contains(YCPSymbol("relnotes_url")))
			info->add(YCPString("relnotes_url"), YCPString(product->releaseNotesUrls().first().asString()));

		std::string product_summary = product->summary();
		if (all || attrs->contains(YCPSymbol("display_name")))
			info->add(YCPString("display_name"), YCPString(product_summary));

		if (all || attrs->contains(YCPSymbol("short_name")))
		{
			std::string product_shortname = product->shortName();
			if (!product_shortname.empty())
				info->add(YCPString("short_name"), YCPString(product_shortname));
			else if (!product_summary.empty())
				// use summary for the short name if it's defined
				info->add(YCPString("short_name"), YCPString(product_summary));
		}

		if ((all && product->endOfLife() > 0) || attrs->contains(YCPSymbol("eol")))
          info->add(YCPString("eol"), YCPInteger(product->endOfLife()));

		if (all || attrs->contains(YCPSymbol("update_urls")))
		{
			YCPList updateUrls(asYCPList(product->updateUrls()));
			info->add(YCPString("update_urls"), updateUrls);
		}

		if (all || attrs->contains(YCPSymbol("flags")))
		{
			YCPList flags;
			std::list<std::string> pflags = product->flags();
			for (std::list<std::string>::const_iterator flag_it = pflags.begin();
				flag_it != pflags.end(); ++flag_it)
			{
				flags->add(YCPString(*flag_it));
			}
			info->add(YCPString("flags"), flags);
		}

		YCPList extraUrls( asYCPList(product->extraUrls()) );
		if ((all && !extraUrls.isEmpty()) || attrs->contains(YCPSymbol("extra_urls")))
			info->add(YCPString("extra_urls"), extraUrls);

		YCPList optionalUrls( asYCPList(product->optionalUrls()) );
		if ((all && !optionalUrls.isEmpty()) || attrs->contains(YCPSymbol("optional_urls")))
			info->add(YCPString("optional_urls"), optionalUrls);

		YCPList registerUrls( asYCPList(product->registerUrls()) );
		if ((all && !registerUrls.isEmpty()) || attrs->contains(YCPSymbol("register_urls")))
			info->add(YCPString("register_urls"), registerUrls);

		YCPList smoltUrls( asYCPList(product->smoltUrls()));
		if ((all && !smoltUrls.isEmpty()) || attrs->contains(YCPSymbol("smolt_urls")))
			info->add(YCPString("smolt_urls"), smoltUrls);

		YCPList relNotesUrls(asYCPList(product->releaseNotesUrls()));
		if ((all && !relNotesUrls.isEmpty()) || attrs->contains(YCPSymbol("relnotes_urls")))
			info->add(YCPString("relnotes_urls"), relNotesUrls);

		// registration data
		if (all || attrs->contains(YCPSymbol("register_target")))
			info->add(YCPString("register_target"), YCPString(product->registerTarget()));
		if (all || attrs->contains(YCPSymbol("register_release")))
			info->add(YCPString("register_release"), YCPString(product->registerRelease()));
		if (all || attrs->contains(YCPSymbol("register_flavor")))
			info->add(YCPString("register_flavor"), YCPString(product->registerFlavor()));
		if (all || attrs->contains(YCPSymbol("product_line")))
			info->add(YCPString("product_line"), YCPString(product->productLine()));

		// Live CD, FTP Edition...
		if (all || attrs->contains(YCPSymbol("flavor")))
			info->add(YCPString("flavor"), YCPString(product->flavor()));

		// get the installed Products it would replace.
		zypp::Product::ReplacedProducts replaced(product->replacedProducts());
		if ((all && !replaced.empty()) || attrs->contains(YCPSymbol("replaces")))
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
					rprod->add(YCPString("display_name"), YCPString(product_summary));

				std::string product_shortname = replacedProduct->shortName();
				if (product_shortname.size() > 0)
					rprod->add(YCPString("short_name"), YCPString(product_shortname));
				// use summary for the short name if it's defined
				else if (product_summary.size() > 0)
					rprod->add(YCPString("short_name"), YCPString(product_summary));
			}

			info->add(YCPString("replaces"), rep_prods);
		}

		std::string product_file;

		// add reference file in /etc/products.d
		if (status.isInstalled() && (all || attrs->contains(YCPSymbol("upgrades"))))
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

		if (all || attrs->contains(YCPSymbol("product_file")))
		{
			if (product_file.empty())
				y2warning("The product file has not been found");
			else
				y2milestone("Found product file %s", product_file.c_str());

			if ((all && !product_file.empty()) || attrs->contains(YCPSymbol("product_file")))			
				info->add(YCPString("product_file"), YCPString(product_file));
		}
	}

    // pattern specific info
	zypp::Pattern::constPtr pattern = zypp::asKind<zypp::Pattern>(item.resolvable());
    if (pattern) {
	    if (all || attrs->contains(YCPSymbol("kind")))
			info->add(YCPString("kind"), YCPSymbol("pattern"));

		if (all || attrs->contains(YCPSymbol("category")))
			info->add(YCPString("category"), YCPString(pattern->category()));
		if (all || attrs->contains(YCPSymbol("user_visible")))
			info->add(YCPString("user_visible"), YCPBoolean(pattern->userVisible()));
		if (all || attrs->contains(YCPSymbol("default")))
			info->add(YCPString("default"), YCPBoolean(pattern->isDefault()));
		if (all || attrs->contains(YCPSymbol("icon")))
			info->add(YCPString("icon"), YCPString(pattern->icon().asString()));
		if (all || attrs->contains(YCPSymbol("script")))
			info->add(YCPString("script"), YCPString(pattern->script().asString()));
		if (all || attrs->contains(YCPSymbol("order")))
			info->add(YCPString("order"), YCPString(pattern->order()));
    }

    // patch specific info
	zypp::Patch::constPtr patch_ptr = zypp::asKind<zypp::Patch>(item.resolvable());
	if (patch_ptr)
	{
	    if (all || attrs->contains(YCPSymbol("kind")))
			info->add(YCPString("kind"), YCPSymbol("patch"));

		if (all || attrs->contains(YCPSymbol("interactive")))
			info->add(YCPString("interactive"), YCPBoolean(patch_ptr->interactive()));
		if (all || attrs->contains(YCPSymbol("reboot_needed")))
			info->add(YCPString("reboot_needed"), YCPBoolean(patch_ptr->rebootSuggested()));
		if (all || attrs->contains(YCPSymbol("relogin_needed")))
			info->add(YCPString("relogin_needed"), YCPBoolean(patch_ptr->reloginSuggested()));
		if (all || attrs->contains(YCPSymbol("affects_pkg_manager")))
			info->add(YCPString("affects_pkg_manager"), YCPBoolean(patch_ptr->restartSuggested()));
		if (all || attrs->contains(YCPSymbol("is_needed")))
			info->add(YCPString("is_needed"), YCPBoolean(item.isBroken()));

        // names and versions of packages, contained in the patch
		if (all || attrs->contains(YCPSymbol("contents")))
		{
			YCPMap contents;
			zypp::Patch::Contents c( patch_ptr->contents() );
			for_( it, c.begin(), c.end() )
			{
				contents->add (YCPString (it->name()), YCPString (it->edition().c_str()));
			}
			info->add(YCPString("contents"), contents);
		}
    }

    // dependency info
    if (deps || attrs->contains(YCPSymbol("dependencies")))
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
                    y2debug("Empty kind or name: kind: %s, name: %s", d->kind().asString().c_str(), d->name().c_str());
                else
                {
                    YCPMap ycpdep;
                    ycpdep->add (YCPString ("res_kind"), YCPString (d->kind().asString()));
                    ycpdep->add (YCPString ("name"), YCPString (d->name()));
                    ycpdep->add (YCPString ("dep_kind"), YCPString (*kind_it));

                    if (!ycpdeps.contains(ycpdep))
                        ycpdeps->add (ycpdep);
                }
            }
		}

		if (ycpdeps.size() > 0)
			info->add (YCPString ("dependencies"), ycpdeps);

		if (rawdeps.size() > 0)
			info->add (YCPString ("deps"), rawdeps);
    }

    return info;
}

YCPValue
PkgFunctions::ResolvablePropertiesEx(const YCPString& name, const YCPSymbol& kind_r, const YCPString& version, bool all_attrs, bool deps, const YCPList &attrs = YCPList())
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
                                ret->add(Resolvable2YCPMap(*inst_it, all_attrs, deps, attrs));
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
                                ret->add(Resolvable2YCPMap(*avail_it, all_attrs, deps, attrs));
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
			return true;
		else if (!to_install && fate == zypp::ui::Selectable::TO_DELETE)
			return true;
    }

    return false;
}

/**
   @builtin IsAnyResolvable
   @short Is there any resolvable in the required state?
   @param symbol kind_r kind of resolvable, can be `product, `patch, `package, `pattern or `any for any kind of resolvable
   @param symbol status status of resolvable, can be `to_install or `to_remove
   @return boolean true if a resolvable with the requested status was found

   **Obsolete**

   This call is obsolete, use `AnyResolvable()` call instead, it has more filtering options.

*/
YCPValue
PkgFunctions::IsAnyResolvable(const YCPSymbol& kind_r, const YCPSymbol& status)
{
	y2warning("Pkg::IsAnyResolvable() is obsolete.");
	y2warning("Use Pkg::AnyResolvable({kind: ..., status: ...}) instead.");

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

// A custom filter for filtering the libzypp resolvables.
struct ResolvableFilter
{
	// The constructor, convert the input filters into the internal
	// structure to make the filtering process faster and simpler.
	ResolvableFilter(const YCPMap &attributes, const PkgFunctions &pf)
		: pkg(pf), check_repo(false), check_transact_by(false), check_vendor(false),
		check_locked(false), check_on_system(false), check_license_confirmed(false),
		medium_nr(-1)
	{
		YCPValue kind_symbol = attributes->value(YCPSymbol("kind"));
		if (!kind_symbol.isNull() && kind_symbol->isSymbol())
			kind = kind_symbol->asSymbol()->symbol();

		YCPValue name_value = attributes->value(YCPSymbol("name"));
		if (!name_value.isNull() && name_value->isString())
			name = name_value->asString()->value();

		YCPValue status_symbol = attributes->value(YCPSymbol("status"));
		if (!status_symbol.isNull() && status_symbol->isSymbol())
			status_str = status_symbol->asSymbol()->symbol();

		YCPValue source_value = attributes->value(YCPSymbol("source"));
		if (!source_value.isNull() && source_value->isInteger()) {
			check_repo = true;
			repo = source_value->asInteger()->value();
		}

		YCPValue medium_nr_value = attributes->value(YCPSymbol("medium_nr"));
		if (!medium_nr_value.isNull() && medium_nr_value->isInteger())
			medium_nr = medium_nr_value->asInteger()->value();

		YCPValue transact_by_value = attributes->value(YCPSymbol("transact_by"));
		if (!transact_by_value.isNull() && transact_by_value->isSymbol()) {
			check_transact_by = true;
			std::string transact_by_str = transact_by_value->asSymbol()->symbol();

			if (transact_by_str == "user")
				transact_by = zypp::ResStatus::USER;
			else if (transact_by_str == "app_high")
				transact_by = zypp::ResStatus::APPL_HIGH;
			else if (transact_by_str == "app_low")
				transact_by = zypp::ResStatus::APPL_LOW;
			else if (transact_by_str == "solver")
				transact_by = zypp::ResStatus::SOLVER;
			else {
				y2warning("Invalid 'transact_by' value: %s", transact_by_str.c_str());
				check_transact_by = false;
			}
		}

		YCPValue arch_value = attributes->value(YCPSymbol("arch"));
		if (!arch_value.isNull() && arch_value->isString())
			arch_str = arch_value->asString()->value();

		YCPValue version_value = attributes->value(YCPSymbol("version"));
		if (!version_value.isNull() && version_value->isString())
			version_str = version_value->asString()->value();

		YCPValue vendor_value = attributes->value(YCPSymbol("vendor"));
		if (!vendor_value.isNull() && vendor_value->isString())
		{
			check_vendor = true;
			vendor = vendor_value->asString()->value();
		}

		YCPValue locked_value = attributes->value(YCPSymbol("locked"));
		if (!locked_value.isNull() && locked_value->isBoolean())
		{
			check_locked = true;
			locked = vendor_value->asBoolean()->value();
		}

		YCPValue on_system_value = attributes->value(YCPSymbol("on_system_by_user"));
		if (!on_system_value.isNull() && on_system_value->isBoolean())
		{
			check_on_system = true;
			on_system = on_system_value->asBoolean()->value();
		}

		YCPValue license_confirmed_value = attributes->value(YCPSymbol("license_confirmed"));
		if (!license_confirmed_value.isNull() && license_confirmed_value->isBoolean())
		{
			check_license_confirmed = true;
			license_confirmed = license_confirmed_value->asBoolean()->value();
		}
	}

	// The main filtering function, returns true/false for each resolvable in the pool
	// whether it matches the required criteria.
	bool operator()(const zypp::PoolItem &r) const
	{
		// check the kind
		if (!kind.empty() && kind != r->kind())
			return false;

		// check the name
		if (!name.empty() && name != r->name())
			return false;

		// check the version
		if (!version_str.empty() && version_str != r->edition().asString())
			return false;

		// check the architecture
		if (!arch_str.empty() && arch_str != r->arch().asString())
			return false;

		// check the vendor
		if (check_vendor && vendor != r->vendor())
			return false;

		// check the lock status
		if (check_locked && locked != r.status().isLocked())
			return false;

		// check the license status
		if (check_license_confirmed && license_confirmed != r.status().isLicenceConfirmed())
			return false;

		// check the status
		if (!status_str.empty()) {
			zypp::ResStatus status = r.status();

			if (!status.isToBeInstalled() && status_str == "selected")
				return false;
			if (!status.isToBeUninstalled() && status_str == "removed")
				return false;
			if (!(status.isInstalled() || status.isSatisfied()) && status_str == "installed")
				return false;
			// otherwise the resolvable has status available
			if ((status.isToBeInstalled() || status.isInstalled() || status.isSatisfied()) && status_str == "available")
				return false;
		}

		// check who changed the status
		if (check_transact_by && r.status().getTransactByValue() != transact_by)
			return false;

		// check the repository
		if (check_repo && pkg.logFindAlias(r->repoInfo().alias()) != repo)
			return false;

		// check if on system by user
		if (check_on_system && on_system != r.satSolvable().onSystemByUser())
			return false;

		// check the medium number
		if (medium_nr >= 0 && medium_nr != r->mediaNr())
			return false;

		return true;
	}

	// reference to PkgFunctions, we need to call PkgFunctions::logFindAlias()
	const PkgFunctions &pkg;

	std::string kind, name, status_str, transact_by_str, arch_str, version_str;

	bool check_repo;
	PkgFunctions::RepoId repo;

	bool check_transact_by;
	zypp::ResStatus::TransactByValue transact_by;

	bool check_vendor;
	std::string vendor;

	bool check_locked, locked;
	bool check_on_system, on_system;
	bool check_license_confirmed, license_confirmed;
	long long medium_nr;
};

/**
   @builtin Resolvables
   @short Is there any resolvable matching the input filter?
   @param map filter
   @param list attrs the list of required attributes
   @return boolean true if a resolvable was found, false otherwise

   See the ResolvableProperties() call for the accepted filtering keys
   and returned attributes.
*/
YCPValue PkgFunctions::Resolvables(const YCPMap& filter, const YCPList& attrs)
{
	if (attrs.isEmpty())
		y2warning("Passed empty attribute list, empty maps will be returned");

	YCPList ret;

	for (const auto &r : zypp::ResPool::instance().filter(ResolvableFilter(filter, *this)) )
		ret->add(Resolvable2YCPMap(r, false, false, attrs));

	return ret;
}

/**
   @builtin AnyResolvable
   @short Is there any resolvable matching the input filter?
   @param map filter
   @return boolean true if a resolvable was found, false otherwise

   See the ResolvableProperties() call for the accepted filtering keys.
*/
YCPValue PkgFunctions::AnyResolvable(const YCPMap& filter)
{
	for (const auto &r : zypp::ResPool::instance().filter(ResolvableFilter(filter, *this)) )
		return YCPBoolean(true);

	return YCPBoolean(false);
}
