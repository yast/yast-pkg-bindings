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
	+ "register_flavor" -> string: kind of the product (module/extension/...)
        or empty string if not defined
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

std::string PkgFunctions::TransactToString(zypp::ResStatus::TransactByValue trans)
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
#define ADD_STRING(K, V) \
	if (all || attrs->contains(YCPSymbol(K))) \
		info->add(YCPString(K), YCPString(V));
#define ADD_BOOLEAN(K, V) \
	if (all || attrs->contains(YCPSymbol(K))) \
		info->add(YCPString(K), YCPBoolean(V));
#define ADD_INTEGER(K, V) \
	if (all || attrs->contains(YCPSymbol(K))) \
		info->add(YCPString(K), YCPInteger(V));
#define ADD_SYMBOL(K, V) \
	if (all || attrs->contains(YCPSymbol(K))) \
		info->add(YCPString(K), YCPSymbol(V));
#define ADD_NOT_EMPTY_LIST(K, V) \
		if ((all && !(V).isEmpty()) || attrs->contains(YCPSymbol(K))) \
			info->add(YCPString(K), V);
#define ADD_NOT_EMPTY_STRING(K, V) \
		if ((all && !(V).empty()) || attrs->contains(YCPSymbol(K))) \
			info->add(YCPString(K), YCPString(V));

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
	ADD_NOT_EMPTY_STRING("summary", resolvable_summary);

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

	ADD_SYMBOL("transact_by", TransactToString(status.getTransactByValue()));
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

	ADD_INTEGER("download_size", item->downloadSize());
	ADD_INTEGER("inst_size", item->installSize());
	ADD_INTEGER("medium_nr", item->mediaNr());
	ADD_STRING("vendor", item->vendor());

    // package specific info
	zypp::Package::constPtr pkg = zypp::asKind<zypp::Package>(item.resolvable());
	if (pkg)
	{
		ADD_SYMBOL("kind", "package");

		std::string path = pkg->location().filename().asString();
		ADD_NOT_EMPTY_STRING("path", path);

		std::string location = pkg->location().filename().basename();
		ADD_NOT_EMPTY_STRING("location", location);
	}

	zypp::SrcPackage::constPtr src_pkg = zypp::asKind<zypp::SrcPackage>(item.resolvable());
	if (src_pkg)
	{
		ADD_SYMBOL("kind", "srcpackage");

		std::string path = src_pkg->location().filename().asString();
		ADD_NOT_EMPTY_STRING("path", path);

		std::string location = src_pkg->location().filename().basename();
		ADD_NOT_EMPTY_STRING("location", location);

		ADD_STRING("src_type", src_pkg->sourcePkgType());
	}

	zypp::Product::constPtr product = zypp::asKind<zypp::Product>(item.resolvable());
	if ( product )
	{
		ADD_SYMBOL("kind", "product");

		std::string category(product->isTargetDistribution() ? "base" : "addon");

		ADD_STRING("category", category);
		ADD_STRING("type", category);
		ADD_STRING("relnotes_url", product->releaseNotesUrls().first().asString());

		std::string product_summary = product->summary();
		ADD_STRING("display_name", product_summary);

		if (all || attrs->contains(YCPSymbol("short_name")))
		{
			std::string product_shortname = product->shortName();
			ADD_NOT_EMPTY_STRING("short_name", product_shortname)
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

			for (auto const &flag : product->flags())
				flags->add(YCPString(flag));

			info->add(YCPString("flags"), flags);
		}

		YCPList extraUrls( asYCPList(product->extraUrls()) );
		ADD_NOT_EMPTY_LIST("extra_urls", extraUrls);

		YCPList optionalUrls( asYCPList(product->optionalUrls()) );
		ADD_NOT_EMPTY_LIST("optional_urls", optionalUrls);

		YCPList registerUrls( asYCPList(product->registerUrls()) );
		ADD_NOT_EMPTY_LIST("register_urls", registerUrls);

		YCPList smoltUrls( asYCPList(product->smoltUrls()));
		ADD_NOT_EMPTY_LIST("smolt_urls", smoltUrls);

		YCPList relNotesUrls(asYCPList(product->releaseNotesUrls()));
		ADD_NOT_EMPTY_LIST("relnotes_urls", relNotesUrls);

		// registration data
		ADD_STRING("register_target", product->registerTarget());
		ADD_STRING("register_release", product->registerRelease());
		ADD_STRING("register_flavor", product->registerFlavor());
		ADD_STRING("product_line", product->productLine());
		// Live CD, FTP Edition...
		ADD_STRING("flavor", product->flavor());

		// get the installed Products it would replace.
		zypp::Product::ReplacedProducts replaced(product->replacedProducts());
		if ((all && !replaced.empty()) || attrs->contains(YCPSymbol("replaces")))
		{
			YCPList rep_prods;

			// add the products to the list
			for (auto const &replacedProduct : replaced)
			{
				if (!replacedProduct) continue;

				YCPMap rprod;
				rprod->add(YCPString("name"), YCPString(replacedProduct->name()));
				rprod->add(YCPString("version"), YCPString(replacedProduct->edition().asString()));
				rprod->add(YCPString("arch"), YCPString(replacedProduct->arch().asString()));
				rprod->add(YCPString("description"), YCPString(replacedProduct->description()));

				std::string product_summary = replacedProduct->summary();
				ADD_NOT_EMPTY_STRING("display_name", product_summary);

				std::string product_shortname = replacedProduct->shortName();
				ADD_NOT_EMPTY_STRING("short_name", product_shortname)
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

			for (const auto &upgrade : productFileData.upgrades())
			{
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
					ADD_STRING("product_package", refpkg->name());

					// get the package files
					zypp::Package::FileList files( refpkg->filelist() );
					y2milestone("The reference package has %d files", files.size());

					zypp::str::smatch what;
					const zypp::str::regex product_file_regex("^/etc/products\\.d/(.*\\.prod)$");

					// find the product file
					for(const auto &f : files)
					{
						if (zypp::str::regex_match(f, what, product_file_regex))
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

			ADD_NOT_EMPTY_STRING("product_file", product_file);
		}
	}

    // pattern specific info
	zypp::Pattern::constPtr pattern = zypp::asKind<zypp::Pattern>(item.resolvable());
    if (pattern) {
		ADD_SYMBOL("kind", "pattern");

		ADD_STRING("category", pattern->category());
		ADD_BOOLEAN("user_visible", pattern->userVisible());

		ADD_BOOLEAN("default", pattern->isDefault());
		ADD_STRING("icon", pattern->icon().asString());
		ADD_STRING("script", pattern->script().asString());
		ADD_STRING("order", pattern->order());
    }

    // patch specific info
	zypp::Patch::constPtr patch_ptr = zypp::asKind<zypp::Patch>(item.resolvable());
	if (patch_ptr)
	{
		ADD_SYMBOL("kind", "patch");

		ADD_BOOLEAN("interactive", patch_ptr->interactive());
		ADD_BOOLEAN("reboot_needed", patch_ptr->rebootSuggested());
		ADD_BOOLEAN("relogin_needed", patch_ptr->reloginSuggested());
		ADD_BOOLEAN("affects_pkg_manager", patch_ptr->restartSuggested());
		ADD_BOOLEAN("is_needed", item.isBroken());

        // names and versions of packages, contained in the patch
		if (all || attrs->contains(YCPSymbol("contents")))
		{
			YCPMap contents;
			for (const auto &res : patch_ptr->contents())
				contents->add (YCPString (res.name()), YCPString (res.edition().c_str()));
			info->add(YCPString("contents"), contents);
		}
    }

    // dependency info
    if (deps || attrs->contains(YCPSymbol("dependencies")) || attrs->contains(YCPSymbol("deps")))
    {
		std::set<std::string> _kinds = {
			"provides", "prerequires", "requires", "conflicts", "obsoletes",
			"recommends", "suggests", "enhances", "supplements"
		};

		YCPList ycpdeps;
		YCPList rawdeps;
		for (const auto &kind : _kinds)
		{
            zypp::Dep depkind(kind);
            zypp::Capabilities deps = item.resolvable()->dep(depkind);

            // add raw dependencies
			for (const auto &d : deps)
            {
                YCPMap rawdep;
                rawdep->add(YCPString(kind), YCPString(d.asString()));
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
                    ycpdep->add (YCPString ("dep_kind"), YCPString (kind));

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

			if (status_str == "selected")
			{
				if (!status.isToBeInstalled()) return false;
			}
			else if (status_str == "installed")
			{
				if (!(status.staysInstalled() || status.isSatisfied())) return false;
			}
			else if (status_str == "available")
			{
				if (!(status.staysUninstalled() || !status.isSatisfied())) return false;
			}
			else if (status_str == "removed")
			{
				if (!status.isToBeUninstalled()) return false;
			}
			else
			{
				y2warning("Ignoring unknown status: %s", status_str.c_str());
			}
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
	return YCPBoolean(!zypp::ResPool::instance().filter(ResolvableFilter(filter, *this)).empty());
}
