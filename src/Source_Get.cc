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
   Author:	Ladislav Slezák <lslezak@novell.com>
   Summary:     Functions for reading repository properties
   Namespace:   Pkg
*/

#include <PkgFunctions.h>
#include "log.h"
#include "ycpTools.h"

#include <zypp/Product.h>
#include <zypp/Repository.h>
#include <zypp/media/CredentialManager.h>
#include <zypp/TriBool.h>

#include <ycp/YCPBoolean.h>
#include <ycp/YCPMap.h>
#include <ycp/YCPInteger.h>
#include <ycp/YCPVoid.h>
#include <ycp/YCPString.h>
#include <ycp/YCPList.h>

/*
  Textdomain "pkg-bindings"
*/

/**
 * @builtin SourceGetCurrent
 *
 * @short Return the list of all InstSrc Ids.
 *
 * @param boolean enabled_only If true, or omitted, return the Ids of all enabled InstSrces.
 * If false, return the Ids of all known InstSrces.
 *
 * @return list<integer> list of SrcIds (integer)
 **/
YCPValue
PkgFunctions::SourceGetCurrent (const YCPBoolean& enabled)
{
    YCPList res;

    RepoId index = 0LL;
    for( RepoCont::const_iterator it = repos.begin(); it != repos.end() ; ++it, ++index )
    {
	// ignore disabled sources if requested
	if (enabled->value())
	{
	    // Note: enabled() is tribool!
	    if ((*it)->repoInfo().enabled())
	    {
	    }
	    else if (!(*it)->repoInfo().enabled())
	    {
		continue;
	    }
	    else
	    {
		continue;
	    }
	}

	// ignore deleted sources
	if ((*it)->isDeleted())
	{
	    continue;
	}

	res->add( YCPInteger(index) );
    }

    return res;
}

/////////////////////////////////////////////////////////////////////////////////////////
// Query individual sources
/////////////////////////////////////////////////////////////////////////////////////////

/**
 * @builtin SourceGeneralData
 *
 * @short Get general data about the source
 * @description
 * Return general data about the source as a map:
 *
 * <code>
 * $[
 * "enabled"	: YCPBoolean,
 * "autorefresh": YCPBoolean,
 * "product_dir": YCPString,
 * "type"	: YCPString,
 * "url"	: YCPString (without password, but see SourceURL),
 * "raw_url"	: YCPString (without password, but see SourceRawURL, raw URL without variable replacement),
 * "alias"	: YCPString,
 * "name"	: YCPString,
 * "raw_name"	: YCPString (raw name without variable replacement),
 * "service"	: YCPString, (service to which the repo belongs, empty if there is no service assigned)
 * "keeppackages" : YCPBoolean,
 * "is_update_repo" : YCPBoolean, (true if this is an update repo - this requires loaded objects in pool otherwise the flag is not returned! The value is stored in repo metadata, not in .repo file!)
 * "valid_repo_signature" : YCPBoolean or YCPVoid (boolean: nil=unsigned, false=bad signature, true=good signature)
 * ];
 *
 * </code>
 * @param integer SrcId Specifies the InstSrc to query.
 * @return map
 **/
YCPValue
PkgFunctions::SourceGeneralData (const YCPInteger& id)
{
    YCPMap data;

    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
	return YCPVoid ();

    // convert type to the old strings ("YaST", "YUM" or "Plaindir")
    std::string srctype = zypp2yastType(repo->repoInfo().type().asString());

    data->add( YCPString("enabled"),		YCPBoolean(repo->repoInfo().enabled()));
    data->add( YCPString("autorefresh"),	YCPBoolean(repo->repoInfo().autorefresh()));
    data->add( YCPString("type"),		YCPString(srctype));
    data->add( YCPString("product_dir"),	YCPString(repo->repoInfo().path().asString()));

    // check if there is an URL
    if (!repo->repoInfo().baseUrlsEmpty())
    {
        data->add( YCPString("url"),		YCPString(repo->repoInfo().url().asString()));
        data->add( YCPString("raw_url"),	YCPString(repo->repoInfo().rawUrl().asString()));
    }

    data->add( YCPString("alias"),		YCPString(repo->repoInfo().alias()));

    data->add( YCPString("name"),		YCPString(repo->repoInfo().name()));
    data->add( YCPString("raw_name"),		YCPString(repo->repoInfo().rawName()));

    YCPList base_urls;
    for( zypp::RepoInfo::urls_const_iterator it = repo->repoInfo().baseUrlsBegin(); it != repo->repoInfo().baseUrlsEnd(); ++it)
    {
	base_urls->add(YCPString(it->asString()));
    }
    data->add( YCPString("base_urls"),		base_urls);

    data->add( YCPString("mirror_list"),	YCPString(repo->repoInfo().mirrorListUrl().asString()));

    data->add( YCPString("priority"),	YCPInteger(repo->repoInfo().priority()));

    data->add( YCPString("service"),	YCPString(repo->repoInfo().service()));

    data->add( YCPString("keeppackages"),	YCPBoolean(repo->repoInfo().keepPackages()));

    // handle tribool, return nil for the indeterminate state
    zypp::TriBool vrs = repo->repoInfo().validRepoSignature();
    if (zypp::indeterminate(vrs))
        data->add(YCPString("valid_repo_signature"), YCPVoid());
    else
        data->add(YCPString("valid_repo_signature"), YCPBoolean((bool)vrs));

    // add Repository data
    zypp::Repository repository(zypp::ResPool::instance().reposFind(repo->repoInfo().alias()));

    if (repository != zypp::Repository::noRepository)
    {
	y2debug("adding zypp::Repository info");
	data->add( YCPString("is_update_repo"), YCPBoolean(repository.isUpdateRepo()));
    }

    return data;
}

/**
 * @builtin SourceURL
 *
 * @short Get full source URL, including password
 * @param integer SrcId Specifies the InstSrc to query.
 * @return string or nil on failure
 **/
YCPValue
PkgFunctions::SourceURL (const YCPInteger& id)
{
    return GetSourceUrl(id, false);
}

/**
 * @builtin SourceRawURL
 *
 * @short Get full source raw URL (no variable replacement), including password
 * @param integer SrcId Specifies the InstSrc to query.
 * @return string or nil on failure
 **/
YCPValue
PkgFunctions::SourceRawURL (const YCPInteger& id)
{
    return GetSourceUrl(id, true);
}


/**
 * @builtin SourceMediaData
 * @short Return media data about the source
 * @description
 * Return media data about the source as a map:
 *
 * <code>
 * $["media_count": YCPInteger,
 * "media_id"	  : YCPString,
 * "media_vendor" : YCPString,
 * "url"	  : YCPString,
 * ];
 * </code>
 *
 * @param integer SrcId Specifies the InstSrc to query.
 * @return map
 **/
YCPValue
PkgFunctions::SourceMediaData (const YCPInteger& id)
{
    YCPMap data;
    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
        return YCPVoid ();

    std::string alias = repo->repoInfo().alias();
    bool found_resolvable = false;
    int max_medium = 1;

    // search the maximum source number of a package in the repository
    try
    {
	for (zypp::ResPoolProxy::const_iterator it = zypp_ptr()->poolProxy().byKindBegin(zypp::ResKind::package);
	    it != zypp_ptr()->poolProxy().byKindEnd(zypp::ResKind::package);
	    ++it)
	{
	    // search in available products
	    for (zypp::ui::Selectable::available_iterator aval_it = (*it)->availableBegin();
		aval_it != (*it)->availableEnd();
		++aval_it)
	    {
		zypp::Package::constPtr pkg = zypp::asKind<zypp::Package>(aval_it->resolvable());

		if (pkg && pkg->repoInfo().alias() == alias)
		{
		    found_resolvable = true;

		    int medium = pkg->mediaNr();

		    if (medium > max_medium)
		    {
			max_medium = medium;
		    }
		}
	    }
	}
    }
    catch (...)
    {
    }

    if (found_resolvable)
    {
	data->add( YCPString("media_count"), YCPInteger(max_medium));
    }
    else
    {
	y2error("No resolvable from repository '%s' found, cannot get number of media (use Pkg::SourceLoad() to load the resolvables)", alias.c_str());
    }

    y2warning("Pkg::SourceMediaData() doesn't return \"media_id\" and \"media_vendor\" values anymore.");

    // SourceMediaData returns URLs without password
    if (!repo->repoInfo().baseUrlsEmpty())
    {
	data->add( YCPString("url"),	YCPString(repo->repoInfo().url().asString()));

	// add all base URLs
	YCPList base_urls;
	for( zypp::RepoInfo::urls_const_iterator it = repo->repoInfo().baseUrlsBegin(); it != repo->repoInfo().baseUrlsEnd(); ++it)
	{
	    base_urls->add(YCPString(it->asString()));
	}
	data->add( YCPString("base_urls"),		base_urls);
    }

  return data;
}

/**
 * @builtin SourceProductData
 * @short Return Product data about the source
 * @param integer SrcId Specifies the InstSrc to query.
 * @description
 * Product data about the source as a map:
 *
 * <code>
 * $[
 * "label"		: string,
 * "vendor"		: string,
 * "productname"	: string,
 * "productversion"	: string,
 * "relnotesurl"	: string,
 * "relnotes_urls"	: list<string>
 * "register_urls"	: list<string>
 * "smolt_urls"		: list<string>
 * "update_urls"	: list<string>
 * "extra_urls"		: list<string>
 * "optional_urls"	: list<string>
 * ];
 * </code>
 *
 * @return map
 **/
YCPValue
PkgFunctions::SourceProductData (const YCPInteger& src_id)
{
    YCPMap ret;

    YRepo_Ptr repo = logFindRepository(src_id->value());
    if (!repo)
        return YCPVoid ();

    std::string alias = repo->repoInfo().alias();

    try
    {
	for (zypp::ResPoolProxy::const_iterator it = zypp_ptr()->poolProxy().byKindBegin(zypp::ResKind::product);
	    it != zypp_ptr()->poolProxy().byKindEnd(zypp::ResKind::product);
	    ++it)
	{
	    zypp::Product::constPtr product = NULL;

	    // search in available products
	    for (zypp::ui::Selectable::available_iterator aval_it = (*it)->availableBegin();
		aval_it != (*it)->availableEnd();
		++aval_it)
	    {
		zypp::Product::constPtr prod = zypp::asKind<zypp::Product>(aval_it->resolvable());
		if (prod && prod->repoInfo().alias() == alias)
		{
		    product = prod;
		    break;
		}
	    }

	    if (product)
	    {
		ret->add( YCPString("label"),		YCPString( product->summary() ) );
		ret->add( YCPString("vendor"),		YCPString( product->vendor() ) );
		ret->add( YCPString("productname"),	YCPString( product->name() ) );
		ret->add( YCPString("productversion"),	YCPString( product->edition().version() ) );
		ret->add( YCPString("relnotesurl"), 	YCPString( product->releaseNotesUrls().first().asString()));
		
		ret->add( YCPString("relnotes_urls"), 	asYCPList(product->releaseNotesUrls()));
		ret->add( YCPString("register_urls"), 	asYCPList(product->registerUrls()));
		ret->add( YCPString("smolt_urls"), 	asYCPList(product->smoltUrls()));
		ret->add( YCPString("update_urls"), 	asYCPList(product->updateUrls()));
		ret->add( YCPString("extra_urls"), 	asYCPList(product->extraUrls()));
		ret->add( YCPString("optional_urls"), 	asYCPList(product->optionalUrls()));

		break;
	    }
	}

	if(ret->size() == 0)
	{
	    y2warning("Product for source '%lld' not found", src_id->value());
	}
    }
    catch (...)
    {
    }

    return ret;
}

/**
 * @builtin SourceEditGet
 *
 * @short Get state of Sources
 * @description
 * Return a list of states for all known InstSources sorted according to the
 * source priority (highest first). A source state is a map:
 * $[
 * "SrcId"	: YCPInteger,
 * "enabled"	: YCPBoolean,
 * "autorefresh": YCPBoolean,
 * "name"	: YCPString,
 * "raw_name"	: YCPString,
 * "service"	: YCPString,
 * "keeppackages" : YCPBoolean,
 * ];
 *
 * @return list<map> list of source states (map)
 **/
YCPValue
PkgFunctions::SourceEditGet ()
{
    YCPList ret;

    unsigned long index = 0;
    for( RepoCont::const_iterator it = repos.begin(); it != repos.end(); ++it, ++index)
    {
	if (!(*it)->isDeleted())
	{
	    YCPMap src_map;

	    src_map->add(YCPString("SrcId"), YCPInteger(index));
	    // Note: enabled() is tribool
	    src_map->add(YCPString("enabled"), YCPBoolean((*it)->repoInfo().enabled()));
	    // Note: autorefresh() is tribool
	    src_map->add(YCPString("autorefresh"), YCPBoolean((*it)->repoInfo().autorefresh()));
	    src_map->add(YCPString("name"), YCPString((*it)->repoInfo().name()));
	    src_map->add(YCPString("raw_name"), YCPString((*it)->repoInfo().rawName()));
	    src_map->add(YCPString("priority"), YCPInteger((*it)->repoInfo().priority()));
	    src_map->add(YCPString("service"), YCPString((*it)->repoInfo().service()));
	    src_map->add(YCPString("keeppackages"), YCPBoolean((*it)->repoInfo().keepPackages()));

	    ret->add(src_map);
	}
    }

    return ret;
}

// Helper with common code to SourceURL and SourceRawUrl
YCPValue
PkgFunctions::GetSourceUrl (const YCPInteger& id, bool raw)
{
    const YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
        return YCPVoid();

    zypp::Url url;

    if (!repo->repoInfo().baseUrlsEmpty())
    {
        if (raw) {
            url = repo->repoInfo().rawUrl();
        } else {
            // #186842
            url = repo->repoInfo().url();
        }
    }

    return YCPString(url.asCompleteString());
}
