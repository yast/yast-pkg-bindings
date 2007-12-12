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
   Summary:     Functions related to repository registration
*/

#include <Callbacks.h>
#include <Callbacks.YCP.h>

#include <PkgModule.h>
#include <PkgModuleFunctions.h>
#include <PkgProgress.h>

#include <zypp/MediaProducts.h>
#include <zypp/media/Mount.h>

/*
  Textdomain "pkg-bindings"
*/

// scanned available products
// hack: zypp/MediaProducts.h cannot be included in PkgModuleFunctions.h
zypp::MediaProductSet available_products;

// this method should be used instead of zypp::productsInMedia()
// it initializes the download callbacks
void PkgModuleFunctions::ScanProductsWithCallBacks(const zypp::Url &url)
{
    CallInitDownload(std::string(_("Scanning products in ") + url.asString()));

    extern ZyppRecipients::MediaChangeSensitivity _silent_probing;
    // remember the current value
    ZyppRecipients::MediaChangeSensitivity _silent_probing_old = _silent_probing;

    // disable media change callback for optional file
    _silent_probing = ZyppRecipients::MEDIA_CHANGE_DISABLE;

    y2milestone("Scanning products in %s ...", url.asString().c_str());

    try
    {
	available_products.clear();
	zypp::productsInMedia(url, available_products);
    }
    catch(...)
    {
	// call the final event even in case of exception
	CallDestDownload();

	// restore the probing flag
	_silent_probing = _silent_probing_old;

	// rethrow the execption
	throw;
    }

    CallDestDownload();

    // restore the probing flag
    _silent_probing = _silent_probing_old;
}

static std::string timestamp ()
{
    time_t t = time(NULL);
    struct tm * tmp = localtime(&t);

    if (tmp == NULL) {
	return "";
    }

    char outstr[50];
    if (strftime(outstr, sizeof(outstr), "%Y%m%d-%H%M%S", tmp) == 0) {
	return "";
    }
    return outstr;
}

/**
 * Take the ?alias=foo part of old_url, if any, and return it,
 * putting the rest to new_url.
 * \throws Exception on malformed URLs I guess
 */
static std::string removeAlias (const zypp::Url & old_url,
				zypp::Url & new_url)
{
  std::string alias;
  new_url = old_url;
  zypp::url::ParamMap query = new_url.getQueryStringMap ();
  zypp::url::ParamMap::iterator alias_it = query.find ("alias");
  if (alias_it != query.end ())
  {
    alias = alias_it->second;
    query.erase (alias_it);
    new_url.setQueryStringMap (query);
  }
  return alias;
}


/**
 * helper - add "mountoptions=ro" for mountable URL schemes if "mountoptions" option is not empty and
 * "rw" or "ro" option is missing
 */
zypp::Url addRO(const zypp::Url &url)
{
    zypp::Url ret(url);
    std::string scheme = zypp::str::toLower(url.getScheme());

    if (scheme == "nfs"
	|| scheme == "hd"
	|| scheme == "smb"
	|| scheme == "iso"
	|| scheme == "cd"
	|| scheme == "dvd"
    )
    {
	const std::string mountoptions = "mountoptions";
	zypp::media::Mount::Options options(url.getQueryParam(mountoptions));

	y2debug("Current mountoptions: %s", options.asString().c_str());

	// if mountoptions are empty lizypp uses "ro" by default
	// don't override "rw" option from application
	// don't add "ro" if it's already present
	if (!options.empty() && !options.has("rw") && !options.has("ro"))
	{
	    options["ro"];

	    ret.setQueryParam(mountoptions, options.asString());
	    y2milestone("Adding read only mount option: '%s' -> '%s'", url.asString().c_str(), ret.asString().c_str());
	}
    }

    return ret;
}

/** Create a Source and immediately put it into the SourceManager.
 * \return the SourceId
 * \throws Exception if Source creation fails
*/
std::vector<zypp::RepoInfo>::size_type
PkgModuleFunctions::createManagedSource( const zypp::Url & url_r,
		     const zypp::Pathname & path_r,
		     const bool base_source,
		     const std::string& type,
		     const std::string &alias_r,
		     PkgProgress &progress,
		     const zypp::ProgressData::ReceiverFnc & progressrcv)
{
    // parse URL
    y2milestone ("Original URL: %s, product directory: %s", url_r.asString().c_str(), path_r.asString().c_str());

    // #158850#c17, if the URL contains an alias, we use that
    zypp::Url url;

    std::string alias = removeAlias(url_r, url);
    y2milestone("Alias from URL: '%s'", alias.c_str());

#warning FIXME: use base_source (base_source vs. addon) (will be probably not needed)

    // repository type
    zypp::repo::RepoType repotype;
    zypp::RepoManager repomanager = CreateRepoManager();

    const bool do_probing = type.empty();

    if (!do_probing)
    {
	try
	{
	    // do conversion from the Yast type ("YaST", "YUM", "Plaindir")
	    // to libzypp type ("yast", "yum", "plaindir")
	    repotype = repotype.parse(yast2zyppType(type));
	}
	catch (zypp::repo::RepoUnknownTypeException &e)
	{
	    y2warning("Unknown source type '%s'", type.c_str());
	}
    }

    // the type is not specified or is wrong, autoprobe the type 
    if (repotype == zypp::repo::RepoType::NONE)
    {
        zypp::Url probe_url(url_r);

	if (!path_r.asString().empty())
	{
	    zypp::Pathname pth(probe_url.getPathName());
	    pth /= path_r;

	    probe_url.setPathName(pth.asString());
	}

	y2milestone("Probing source type: '%s'", probe_url.asString().c_str());

	// autoprobe type of the repository 
	repotype = ProbeWithCallbacks(probe_url);

	if (do_probing)
	{
	    progress.NextStage();
	}
    }

    y2milestone("Using source type: %s", repotype.asString().c_str());


    // create source definition object
    zypp::RepoInfo repo;

    std::string name;

    // set alias and name
    if (alias.empty())
    {
	// alias not set via URL, use the passed alias or the URL path
	if (alias_r.empty())
	{
	    // use the last path element in URL 
	    std::string url_path = url.getPathName();

	    std::string::size_type pos_begin = url_path.rfind("/");
	    std::string::size_type pos_end = std::string::npos;

	    // ignore the trailing slash
	    if (pos_begin == url_path.size() - 1)
	    {
		pos_begin = url_path.rfind("/", url_path.size() - 2);

		if (pos_begin != std::string::npos)
		{
		    pos_end = url_path.size() - pos_begin - 2;
		}
	    }
	    else
	    {
		pos_end = url_path.size() - pos_begin - 1;
	    }

	    // ignore the found slash character
	    pos_begin++;

	    alias = std::string(url_path, pos_begin, pos_end);

	    y2milestone("Alias from URL path: %s", alias.c_str());

	    // fallback
	    if (alias.empty())
	    {
		y2milestone("URL alias is empty using 'Repository'");
		alias = "Repository";
	    }
	}
	else
	{
	    alias = alias_r;
	}

	name = alias;
    }
    else
    {
	name = alias;
    }


    y2milestone("Name of the repository: '%s'", name.c_str());

    // alias must be unique, add a suffix if it's already used
    alias = UniqueAlias(alias);

    // add read only mount option to the URL if needed
    url = addRO(url);

    bool autorefresh = true;

    std::string scheme = zypp::str::toLower(url.getScheme());
    if (scheme == "cd" || scheme == "dvd")
    {
	y2milestone("Disabling autorefresh for CD/DVD repository");
	autorefresh = false;
    }

    repo.setAlias(alias);
    repo.setName(name);
    repo.setType(repotype);
    repo.addBaseUrl(url);
    repo.setPath(path_r);
    repo.setEnabled(true);
    repo.setAutorefresh(autorefresh);

{
    zypp::ProgressData prg(90);
    prg.sendTo(progressrcv);
    prg.toMin();

    zypp::CombinedProgressData subprogrcv_ref(prg, 20);
    zypp::CombinedProgressData subprogrcv_build(prg, 70);

    // set metadata path (#293428)
    zypp::Pathname metadatapath = repomanager.metadataPath(repo);
    repo.setMetadataPath(metadatapath);

    y2milestone("Adding source '%s' (%s, dir: %s)", repo.alias().c_str(), url.asString().c_str(), path_r.asString().c_str());
    // note: exceptions should be caught by the calling code
    RefreshWithCallbacks(repo, subprogrcv_ref);
    progress.NextStage();

    // build cache if needed
    if (!repomanager.isCached(repo))
    {
	y2milestone("Caching source '%s'...", repo.alias().c_str());

	repomanager.buildCache(repo, zypp::RepoManager::BuildIfNeeded, subprogrcv_build);
    }
    progress.NextStage();

    prg.toMax();
}
    repos.push_back(new YRepo(repo));

    y2milestone("Added source '%s': '%s', enabled: %s, autorefresh: %s",
	repo.alias().c_str(),
	repo.baseUrlsBegin()->asString().c_str(),
	repo.enabled() ? "true" : "false",
	repo.autorefresh() ? "true" : "false"
    );

    // the source is at the end of the list
    return repos.size() - 1;
}

/****************************************************************************************
 * @builtin RepositoryAdd
 *
 * @short Register a new repository
 * @description
 * Adds a new repository to the internal structures. The repository is only registered,
 * metadata is not downloaded, use Pkg::SourceRefreshNow() for that. The metadata is also loaded
 * automatically when loading the repository content (Pkg::SourceLoad())
 *
 * @param map map with repository parameters: $[ "enabled" : boolean, "autorefresh" : boolean, "name" : string,
 *   "alias" : string, "base_urls" : list<string>, "prod_dir" : string, "type" : string ] 
 * @return integer Repository ID or nil on error
 **/
YCPValue PkgModuleFunctions::RepositoryAdd(const YCPMap &params)
{
    zypp::RepoInfo repo;

    // turn on the repo by default
    repo.setEnabled(true);
    // enable autorefresh by default
    repo.setAutorefresh(true);

    if (!params->value( YCPString("enabled") ).isNull() && params->value(YCPString("enabled"))->isBoolean())
    {
	repo.setEnabled(params->value(YCPString("enabled"))->asBoolean()->value());
    }

    if (!params->value( YCPString("autorefresh") ).isNull() && params->value(YCPString("autorefresh"))->isBoolean())
    {
	repo.setAutorefresh(params->value(YCPString("autorefresh"))->asBoolean()->value());
    }

    std::string alias;

    if (!params->value( YCPString("alias") ).isNull() && params->value(YCPString("alias"))->isString())
    {
	alias = params->value(YCPString("alias"))->asString()->value();
    }

    if (alias.empty())
    {
	alias = timestamp();

	// the alias must be unique
	alias = UniqueAlias(alias);
    }
    else
    {
	if (aliasExists(alias))
	{
	    ycperror("alias %s already exists", alias.c_str());
	    return YCPVoid();
	}
    }
   
    repo.setAlias(alias);

    // use the first base URL as a fallback name 
    std::string first_url;

    if (!params->value( YCPString("base_urls") ).isNull() && params->value(YCPString("base_urls"))->isList())
    {
	YCPList lst(params->value(YCPString("base_urls"))->asList());

	for (int index = 0; index < lst->size(); ++index)
	{
	    if( ! lst->value(index)->isString() )
	    {
		ycperror( "Pkg::RepositoryAdd(): entry not a string at index %d: %s", index, lst->toString().c_str());
		return YCPVoid();
	    }

	    zypp::Url url;

	    try
	    {
		url = lst->value(index)->asString()->value();
		zypp::Url url_new;

		std::string name = removeAlias(url, url_new);

		if (!name.empty())
		{
		    repo.setName(name);
		    url = url_new;
		}

		// add read only mount option to the URL if needed
		url = addRO(url);
	    }
	    catch(const zypp::Exception & expt)
	    {
		y2error("Invalid URL: %s", expt.asString().c_str());
		_last_error.setLastError(ExceptionAsString(expt));
		return YCPVoid();
	    }

	    if (index == 0)
	    {
		first_url = url.asString();
	    }

	    repo.addBaseUrl(url);
	}
    }
    else
    {
	ycperror("Missing \"base_urls\" key in the map");
	return YCPVoid();
    }

    // check name parameter
    if (!params->value( YCPString("name") ).isNull() && params->value(YCPString("name"))->isString())
    {
	repo.setName(params->value(YCPString("name"))->asString()->value());
    }
    else
    {
	// if the key "name" is missing and the name hasn't been set by ?alias= in the URL
	// then use the first URL as the name
	if (repo.name().empty())
	{
	    repo.setName(first_url);
	}
    }

    if (!params->value( YCPString("type") ).isNull() && params->value(YCPString("type"))->isString())
    {
	std::string type = yast2zyppType(params->value(YCPString("type"))->asString()->value());

	try
	{
	    zypp::repo::RepoType repotype(type);
	    repo.setType(repotype);
	}
	catch (zypp::repo::RepoUnknownTypeException &e)
	{
	    ycperror("Unknown source type '%s': %s", type.c_str(), e.asString().c_str());
	    _last_error.setLastError(ExceptionAsString(e));
	    return YCPVoid();
	}

    }

    if (!params->value( YCPString("prod_dir") ).isNull() && params->value(YCPString("prod_dir"))->isString())
    {
	repo.setPath(params->value(YCPString("prod_dir"))->asString()->value());
    }

    // set metadata path (#293428)
    zypp::RepoManager repomanager = CreateRepoManager();
    zypp::Pathname metadatapath = repomanager.metadataPath(repo);
    repo.setMetadataPath(metadatapath);

    repos.push_back(new YRepo(repo));

    // the new source is at the end of the list
    return YCPInteger(repos.size() - 1);
}

/****************************************************************************************
 * @builtin SourceCreate
 *
 * @short Create a Source
 * @description
 * Load and enable all InstSrces found at media_url, i.e. all sources mentioned in /media.1/products.
 * If no /media.1/products is available, InstSrc is expected to be located directly below
 * media_url (product_dir: /).
 *
 * If a product_dir is provided, only the InstSrc located at media_url/product_dir is loaded
 * and enabled.
 *
 * @param string url The media to scan.
 * @optarg string product_dir Restrict scan to a certain InstSrc located in media_url/product_dir.
 *
 * @return integer The source_id of the first InstSrc found on the media.
 **/
YCPValue
PkgModuleFunctions::SourceCreate (const YCPString& media, const YCPString& pd)
{
  // not base product, autoprobe source type
  return SourceCreateEx (media, pd, false, YCPString(""));
}

YCPValue
PkgModuleFunctions::SourceCreateBase (const YCPString& media, const YCPString& pd)
{
  // base product, autoprobe source type
  return SourceCreateEx (media, pd, true, YCPString(""));
}

/**
 * @builtin SourceCreateType
 * @short Create source of required type
 * @description
 * Create a source without autoprobing the source type. This builtin should be used only for "Plaindir" sources, because Plaindir sources are not automatically probed in SourceCreate() builtin.
 * @param media URL of the source
 * @param pd product directory (if empty the products will be searched)
 * @param type type of the source ("YaST", "YUM" or "Plaindir")
*/

YCPValue
PkgModuleFunctions::SourceCreateType (const YCPString& media, const YCPString& pd, const YCPString& type)
{
  // not base product, autoprobe source type
  return SourceCreateEx (media, pd, false, type);
}

YCPValue
PkgModuleFunctions::SourceCreateEx (const YCPString& media, const YCPString& pd, bool base, const YCPString& source_type)
{
  y2debug("Creating source...");

  zypp::Pathname pn(pd->value ());

  zypp::Url url;

  try {
    url = zypp::Url(media->value ());
  }
  catch(const zypp::Exception & expt )
  {
    y2error ("Invalid URL: %s", expt.asString().c_str());
    _last_error.setLastError(ExceptionAsString(expt));
    return YCPInteger (-1LL);
  }


  YCPList ids;
  std::vector<zypp::RepoInfo>::size_type ret = -1;

  const std::string type = source_type->value();
  const bool scan = pd->value().empty();

  // steps: (scan), download, build cache, load resolvables
  PkgProgress pkgprogress(_callbackHandler);
  std::list<std::string> stages;

  // display the scan stage only when needed
  if (scan)
  {
    stages.push_back(_("Search Available Products"));
  }

  if (source_type->value().empty())
  {
    stages.push_back(_("Probe Source Type"));
  }

  stages.push_back(_("Download Descriptions"));
  stages.push_back(_("Rebuild Cache"));
  stages.push_back(_("Load Data"));

  pkgprogress.Start(_("Adding the Repository..."), stages, _("TODO: help"));

  zypp::ProgressData prg(100);
  prg.sendTo(pkgprogress.Receiver());
  prg.toMin();

  if (scan) {
    // scan all sources
    zypp::MediaProductSet products;

    try {
	ScanProductsWithCallBacks(url);
	products = available_products;
    }
    catch ( const zypp::Exception& excpt)
    {
	_last_error.setLastError(ExceptionAsString(excpt));
	y2error( "Cannot read the product list from the media" );
	return YCPInteger(-1LL);
    }

    if( products.empty() )
    {
	// no products found, use the base URL instead
	zypp::MediaProductEntry entry ;
	products.insert( entry );
    }

    // scanning has been finished
    prg.set(5);
    pkgprogress.NextStage();

    zypp::CombinedProgressData subprogrcv(prg, 85);
    zypp::CombinedProgressData subprogrcv2(prg, 10);

    // TODO FIXME - fix the behavior for multiple repos
    for( zypp::MediaProductSet::const_iterator it = products.begin();
	it != products.end() ; ++it )
    {
	try
	{
	    std::vector<zypp::RepoInfo>::size_type id = createManagedSource(url, it->_dir, base, type, it->_name, pkgprogress, subprogrcv);
	    YRepo_Ptr repo = logFindRepository(id);

	    // no progress needed, refresh has been done in createManagedSource()
	    LoadResolvablesFrom(repo->repoInfo(), subprogrcv2);

	    // TODO FIXME: 'ret' is not used
	    // return the id of the first product
	    if ( it == products.begin() )
		ret = id;
	}
	catch ( const zypp::Exception& excpt)
	{
	    y2error("SourceCreate for '%s' product '%s' has failed"
		, url.asString().c_str(), pn.asString().c_str());
	    _last_error.setLastError(ExceptionAsString(excpt));
	    return YCPInteger(-1LL);
	}
    }
  } else {
    y2debug("Creating source...");

    zypp::CombinedProgressData subprogrcv_create(prg, 80);
    zypp::CombinedProgressData subprogrcv_load(prg, 20);

    try
    {
	ret = createManagedSource(url, pn, base, type, "", pkgprogress, subprogrcv_create);
	pkgprogress.NextStage();

	YRepo_Ptr repo = logFindRepository(ret);
	repo->repoInfo().setEnabled(true);

	// load the resolvables
	LoadResolvablesFrom(repo->repoInfo(), subprogrcv_load);
    }
    catch ( const zypp::Exception& excpt)
    {
	y2error("SourceCreate for '%s' product '%s' has failed"
	    , url.asString().c_str(), pn.asString().c_str());
	_last_error.setLastError(ExceptionAsString(excpt));
    }
  }

  prg.toMax();

  PkgFreshen();

  return YCPInteger(repos.size() - 1);
}

/****************************************************************************************
 * @builtin RepositoryProbe
 *
 * @short Probe type of the repository
 * @param url specifies the path to the repository
 * @param prod_dir product directory (if empty the url is probed directly)
 * @return string repository type ("NONE" if type could be determined, nil if an error occurred (e.g. resolving the hostname)
 **/
YCPValue PkgModuleFunctions::RepositoryProbe(const YCPString& url, const YCPString& prod_dir)
{
    y2milestone("Probing repository type: '%s'...", url->value().c_str());
    zypp::RepoManager repomanager = CreateRepoManager();
    std::string ret;

    try
    {
	zypp::Url probe_url(url->value());

	// add the product directory
	std::string prod = prod_dir->value();

	if (!prod.empty())
	{
	    // add "/" at the begining if it's missing
	    if (std::string(prod, 0, 1) != "/")
	    {
		prod = "/" + prod;
	    }

	    // merge the URL path and the product path
	    std::string path = probe_url.getPathName();
	    path += prod;

	    y2milestone("Using probing path: %s", path.c_str());
	    probe_url.setPathName(path);
	}

	// add "ro" mount option
	probe_url = addRO(probe_url);

	// autoprobe type of the repository 
	zypp::repo::RepoType repotype = ProbeWithCallbacks(probe_url);

	ret = zypp2yastType(repotype.asString());
	y2milestone("Detected type: '%s'...", ret.c_str());
    }
    catch (const zypp::Exception& excpt)
    {
	_last_error.setLastError(ExceptionAsString(excpt));
	y2error( "Cannot detect the repository type" );
	return YCPVoid();
    }

    return YCPString(ret);
}


/****************************************************************************************
 * @builtin SourceScan
 * @short Scan a Source Media
 * @description
 * Load all InstSrces found at media_url, i.e. all sources mentioned in /media.1/products.
 * If no /media.1/products is available, InstSrc is expected to be located directly
 * below media_url (product_dir: /).
 *
 * If a product_dir is provided, only the InstSrc located at media_url/product_dir is loaded.
 *
 * In contrary to @ref SourceCreate, InstSrces are loaded into the InstSrcManager,
 * but not enabled (packages and selections are not provided to the PackageManager),
 * and the SrcIds of <b>all</b> InstSrces found are returned.
 *
 * @param string url The media to scan.
 * @optarg string product_dir Restrict scan to a certain InstSrc located in media_url/product_dir.
 *
 * @return list<integer> list of SrcIds (integer).
 **/
YCPValue
PkgModuleFunctions::SourceScan (const YCPString& media, const YCPString& pd)
{
  zypp::Url url;

  try {
    url = zypp::Url(media->value ());
  }
  catch(const zypp::Exception & expt )
  {
    y2error ("Invalid URL: %s", expt.asString().c_str());
    _last_error.setLastError(ExceptionAsString(expt));
    return YCPList();
  }

  zypp::Pathname pn(pd->value ());

  YCPList ids;
  std::vector<zypp::RepoInfo>::size_type id;

  // TODO add total progress
  PkgProgress pkgprogress(_callbackHandler);

  if ( pd->value().empty() ) {

    // scan all sources
    zypp::MediaProductSet products;

    try
    {
	y2milestone("Scanning products in %s ...", url.asString().c_str());
	ScanProductsWithCallBacks(url);
	products = available_products;
    }
    catch ( const zypp::Exception& excpt)
    {
	y2error("Scanning products for '%s' has failed"
		, url.asString().c_str());
	_last_error.setLastError(ExceptionAsString(excpt));
	return ids;
    }
    
    if( products.empty() )
    {
	// no products found, use the base URL instead
	zypp::MediaProductEntry entry;
	products.insert(entry);
    }
    
    for( zypp::MediaProductSet::const_iterator it = products.begin();
	it != products.end() ; ++it )
    {
	try
	{
	    y2milestone("Using product %s in directory %s", it->_name.c_str(), it->_dir.c_str());
	    id = createManagedSource(url, it->_dir, false, "", it->_name, pkgprogress);
	    ids->add( YCPInteger(id) );
	}
	catch ( const zypp::Exception& excpt)
	{
	    y2error("SourceScan for '%s' product '%s' has failed"
		, url.asString().c_str(), pn.asString().c_str());
	    _last_error.setLastError(ExceptionAsString(excpt));
	}
    }
  } else {
    y2debug("Creating source...");

    try
    {
	id = createManagedSource(url, pn, false, "", "", pkgprogress);
	ids->add( YCPInteger(id) );
    }
    catch ( const zypp::Exception& excpt)
    {
	y2error("SourceScan for '%s' product '%s' has failed"
	    , url.asString().c_str(), pn.asString().c_str());
	_last_error.setLastError(ExceptionAsString(excpt));
    }
  }

  y2milestone("Found sources: %s", ids->toString().c_str() );

  return ids;
}

/****************************************************************************************
 * @builtin RepositoryScan
 *
 * @short Scan available products in the repository
 * @param url specifies the path to the repository
 * @return list<list<string>> list of the products: [ [ <product_name_1> <directory_1> ], ...]
 **/
YCPValue PkgModuleFunctions::RepositoryScan(const YCPString& url)
{
    zypp::MediaProductSet products;

    try
    {
	zypp::Url baseurl(url->value());

	baseurl = addRO(baseurl);

	ScanProductsWithCallBacks(baseurl);
	products = available_products;
    }
    catch ( const zypp::Exception& excpt)
    {
	_last_error.setLastError(ExceptionAsString(excpt));
	y2error( "Cannot read the product list from the media" );
	return YCPList();
    }

    YCPList ret;

    for( zypp::MediaProductSet::const_iterator it = products.begin();
	it != products.end() ; ++it )
    {
	YCPList prod;

	// add product directory
	prod.add(YCPString(it->_name));
	// add product name
	prod.add(YCPString(it->_dir.asString()));

	ret.add(prod);
    }

    y2milestone("Found products: %s", ret->toString().c_str());

    return ret;
}

