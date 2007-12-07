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
   File:	$Id:$
   Author:	Ladislav Slezák <lslezak@novell.com>
   Summary:     Functions for reading repository properties
*/

#include <Callbacks.h>
#include <Callbacks.YCP.h>

#include <PkgModule.h>
#include <PkgModuleFunctions.h>

#include <zypp/Product.h>

/*
  Textdomain "pkg-bindings"
*/

/****************************************************************************************
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
PkgModuleFunctions::SourceGetCurrent (const YCPBoolean& enabled)
{
    YCPList res;

    std::vector<YRepo_Ptr>::size_type index = 0;
    for( std::vector<YRepo_Ptr>::const_iterator it = repos.begin(); it != repos.end() ; ++it, ++index )
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

/****************************************************************************************
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
 * "alias"	: YCPString,
 * "name"	: YCPString,
 * ];
 *
 * </code>
 * @param integer SrcId Specifies the InstSrc to query.
 * @return map
 **/
YCPValue
PkgModuleFunctions::SourceGeneralData (const YCPInteger& id)
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
    if (repo->repoInfo().baseUrlsBegin() != repo->repoInfo().baseUrlsEnd())
    {
	data->add( YCPString("url"),		YCPString(repo->repoInfo().baseUrlsBegin()->asString()));
    }

    data->add( YCPString("alias"),		YCPString(repo->repoInfo().alias()));
    data->add( YCPString("name"),		YCPString(repo->repoInfo().name()));

    YCPList base_urls;
    for( zypp::RepoInfo::urls_const_iterator it = repo->repoInfo().baseUrlsBegin(); it != repo->repoInfo().baseUrlsEnd(); ++it)
    {
	base_urls->add(YCPString(it->asString()));
    }
    data->add( YCPString("base_urls"),		base_urls);

    data->add( YCPString("mirror_list"),	YCPString(repo->repoInfo().mirrorListUrl().asString()));

    return data;
}

/******************************************************************************
 * @builtin SourceURL
 *
 * @short Get full source URL, including password
 * @param integer SrcId Specifies the InstSrc to query.
 * @return string or nil on failure
 **/
YCPValue
PkgModuleFunctions::SourceURL (const YCPInteger& id)
{

    const YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
        return YCPVoid();

    std::string url;

    if (repo->repoInfo().baseUrlsBegin() != repo->repoInfo().baseUrlsEnd())
    {
        // #186842
        url = repo->repoInfo().baseUrlsBegin()->asCompleteString();
    }

    return YCPString(url);
}

/****************************************************************************************
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
PkgModuleFunctions::SourceMediaData (const YCPInteger& id)
{
    YCPMap data;
    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
        return YCPVoid ();

    std::string alias = repo->repoInfo().alias();
    bool found_resolvable = false;
    int max_medium = 1;

    for (zypp::ResPool::const_iterator it = zypp_ptr()->pool().begin()
	; it != zypp_ptr()->pool().end()
	; ++it)
    {
	if (it->resolvable()->repository().info().alias() == alias)
	{
	    int medium = it->resolvable()->mediaNr();

	    if (medium > max_medium)
	    {
		max_medium = medium;
	    }
	}
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
    if (repo->repoInfo().baseUrlsBegin() != repo->repoInfo().baseUrlsEnd())
    {
	data->add( YCPString("url"),	YCPString(repo->repoInfo().baseUrlsBegin()->asString()));

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

/****************************************************************************************
 * @builtin SourceProductData
 * @short Return Product data about the source
 * @param integer SrcId Specifies the InstSrc to query.
 * @description
 * Product data about the source as a map:
 *
 * <code>
 * $[
 * "label"		: YCPString,
 * "vendor"		: YCPString,
 * "productname"	: YCPString,
 * "productversion"	: YCPString,
 * "relnotesurl"	: YCPString,
 * ];
 * </code>
 *
 * @return map
 **/
YCPValue
PkgModuleFunctions::SourceProductData (const YCPInteger& src_id)
{
    YCPMap ret;

    try
    {
	// find a product for the given source
	zypp::ResPool::byKind_iterator it = zypp_ptr()->pool().byKindBegin(zypp::ResTraits<zypp::Product>::kind);

	for( ; it != zypp_ptr()->pool().byKindEnd(zypp::ResTraits<zypp::Product>::kind) ; ++it) 
	{
	    zypp::Product::constPtr product = boost::dynamic_pointer_cast<const zypp::Product>( it->resolvable() );

	    if( logFindAlias(product->repository().info().alias()) == src_id->value() )
	    {
		ret->add( YCPString("label"),		YCPString( product->summary() ) );
		ret->add( YCPString("vendor"),		YCPString( product->vendor() ) );
		ret->add( YCPString("productname"),	YCPString( product->name() ) );
		ret->add( YCPString("productversion"),	YCPString( product->edition().version() ) );
		ret->add( YCPString("relnotesurl"), 	YCPString( product->releaseNotesUrl().asString()));

		#warning SourceProductData not finished
		/*
		  data->add( YCPString("datadir"),		YCPString( descr->content_datadir().asString() ) );
		TODO (?): "baseproductname", "baseproductversion", "defaultbase", "architectures",
		"requires", "linguas", "labelmap", "language", "timezone", "descrdir", "datadir"
		*/

		break;
	    }
	}

	if( it == zypp_ptr()->pool().byKindEnd(zypp::ResTraits<zypp::Product>::kind) )
	{
	    y2error ("Product for source '%lld' not found", src_id->value());
	}
    }
    catch (...)
    {
    }

    return ret;
}

/****************************************************************************************
 * @builtin SourceProduct
 * @short Obsoleted function, do not use, see SourceProductData builtin
 * @deprecated
 * @param integer
 * @return map empty map
 **/
YCPValue
PkgModuleFunctions::SourceProduct (const YCPInteger& id)
{
    /* TODO FIXME */
  y2error("Pkg::SourceProduct() is obsoleted, use Pkg::SourceProductData() instead!");
  return YCPMap();
}

/****************************************************************************************
 * @builtin SourceEditGet
 *
 * @short Get state of Sources
 * @description
 * Return a list of states for all known InstSources sorted according to the
 * source priority (highest first). A source state is a map:
 * $[
 * "SrcId"	: YCPInteger,
 * "enabled"	: YCPBoolean
 * "autorefresh": YCPBoolean
 * ];
 *
 * @return list<map> list of source states (map)
 **/
YCPValue
PkgModuleFunctions::SourceEditGet ()
{
    YCPList ret;

    unsigned long index = 0;
    for( std::vector<YRepo_Ptr>::const_iterator it = repos.begin(); it != repos.end(); ++it, ++index)
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

	    ret->add(src_map);
	}
    }

    return ret;
}

