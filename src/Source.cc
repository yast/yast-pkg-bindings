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

   File:	PkgModuleFunctionsSource.cc

   Author:	Klaus Kaempf <kkaempf@suse.de>
   Maintainer:  Klaus Kaempf <kkaempf@suse.de>

   Summary:     Access to Installation Sources
   Namespace:   Pkg
   Purpose:	Access to InstSrc
		Handles source related Pkg::function (list_of_arguments) calls
		from WFMInterpreter.
/-*/
//#include <unistd.h>
//#include <sys/statvfs.h>

#include <iostream>
#include <ycp/y2log.h>

#include <ycpTools.h>
#include <PkgModule.h>
#include <PkgModuleFunctions.h>

#include <Callbacks.h>
#include <Callbacks.YCP.h>

#include <zypp/Product.h>
#include <zypp/target/store/PersistentStorage.h>
#include <zypp/media/MediaManager.h>
#include <zypp/Pathname.h>
#include <zypp/MediaProducts.h>

#include <zypp/RepoInfo.h>
#include <zypp/RepoManager.h>
#include <zypp/Fetcher.h>
#include <zypp/repo/RepoType.h>

#include <zypp/base/String.h>

#include <stdio.h> // snprintf


/*
  Textdomain "pkg-bindings"
*/

void PkgModuleFunctions::CallSourceReportStart(const std::string &text)
{
    // get the YCP callback handler
    Y2Function* ycp_handler = _callbackHandler._ycpCallbacks.createCallback(CallbackHandler::YCPCallbacks::CB_SourceReportStart);

    // is the callback registered?
    if (ycp_handler != NULL)
    {
	// add parameters
	ycp_handler->appendParameter( YCPInteger(0LL) );
	ycp_handler->appendParameter( YCPString("") );
	ycp_handler->appendParameter( YCPString(text) );
	// evaluate the callback function
	ycp_handler->evaluateCall();
    }
}

void PkgModuleFunctions::CallSourceReportEnd(const std::string &text)
{
    // get the YCP callback handler for end event
    Y2Function* ycp_handler = _callbackHandler._ycpCallbacks.createCallback(CallbackHandler::YCPCallbacks::CB_SourceReportEnd);

    // is the callback registered?
    if (ycp_handler != NULL)
    {
	// add parameters
	ycp_handler->appendParameter( YCPInteger(0LL) );
	ycp_handler->appendParameter( YCPString("") );
	ycp_handler->appendParameter( YCPString(text) );
	ycp_handler->appendParameter( YCPString("NO_ERROR") );
	ycp_handler->appendParameter( YCPString("") );
	// evaluate the callback function
	ycp_handler->evaluateCall();
    }
}

void PkgModuleFunctions::CallSourceReportInit()
{
    // get the YCP callback handler for init event
    Y2Function* ycp_handler = _callbackHandler._ycpCallbacks.createCallback(CallbackHandler::YCPCallbacks::CB_SourceReportInit);

    // is the callback registered?
    if (ycp_handler != NULL)
    {
	// evaluate the callback function
	ycp_handler->evaluateCall();
    }
}

void PkgModuleFunctions::CallSourceReportDestroy()
{
    // get the YCP callback handler for destroy event
    Y2Function* ycp_handler = _callbackHandler._ycpCallbacks.createCallback(CallbackHandler::YCPCallbacks::CB_SourceReportDestroy);

    // is the callback registered?
    if (ycp_handler != NULL)
    {
	// evaluate the callback function
	ycp_handler->evaluateCall();
    }
}

/**
 * Logging helper:
 * call zypp::SourceManager::sourceManager()->findSource
 * and in case of exception, log error and setLastError AND RETHROW
 */
YRepo_Ptr PkgModuleFunctions::logFindRepository(std::vector<YRepo_Ptr>::size_type id)
{
    try
    {
	if (!repos[id])
	{
	    // not found
	    throw(std::exception());
	}

	if (repos[id]->isDeleted())
	{
	    y2error("Source %d has been deleted, the ID is not valid", id);
	    return YRepo_Ptr();
	}

	return repos[id];
    }
    catch (...)
    {
	y2error("Cannot find source with ID: %d", id);
	// TODO: improve the error message
	_last_error.setLastError(_("Cannot find source"));
    }

    // not found, return empty pointer
    return YRepo_Ptr();
}

std::vector<YRepo_Ptr>::size_type PkgModuleFunctions::logFindAlias(const std::string &alias)
{
    std::vector<YRepo_Ptr>::size_type index = 0;

    for(std::vector<YRepo_Ptr>::const_iterator it = repos.begin(); it != repos.end() ; ++it, ++index)
    {
	if (!(*it)->isDeleted() && (*it)->repoInfo().alias() == alias)
	    return index;
    }

    return -1;
}


/****************************************************************************************
 * @builtin SourceSetRamCache
 * @short Obsoleted function, do not use
 * @param boolean
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceSetRamCache (const YCPBoolean& a)
{
    y2warning( "Pkg::SourceSetRamCache is obsolete and does nothing");
    return YCPBoolean( true );
}


/****************************************************************************************
 * @builtin SourceRestore
 *
 * @short Restore the sources from the persistent store
 * @description
 * Make sure the Source Manager is up and knows all available installation sources.
 * It's safe to call this multiple times, and once the installation sources are
 * actually enabled, it's even cheap
 *
 * @return boolean True on success
 **/
YCPValue
PkgModuleFunctions::SourceRestore()
{
    if (repos.size() > 0)
    {
	y2warning("Number of registered repositories: %d, skipping repository load!", repos.size());
	return YCPBoolean(true);
    }

    bool success = true;

    try
    {
	zypp::RepoManager repomanager = CreateRepoManager();
	std::list<zypp::RepoInfo> reps = repomanager.knownRepositories();

	repos.clear();
	for (std::list<zypp::RepoInfo>::iterator it = reps.begin();
	    it != reps.end(); ++it)
	{
	    repos.push_back(new YRepo(*it));
	}
    }
    catch (const zypp::Exception& excpt)
    {
	// FIXME: assuming the sources are already initialized
	y2error ("Error in SourceRestore: %s", excpt.asString().c_str());
	_last_error.setLastError(excpt.asUserString());
	success = false;
    }

    return YCPBoolean(success);
}

/****************************************************************************************
 * @builtin SourceGetBrokenSources
 *
 * @short Return list of broken sources (sources which failed to restore)
 * @description
 * Get list of all sources which could not have been restored.
 * @return list<string> list of aliases (product names or URLs)
 **/
YCPValue PkgModuleFunctions::SourceGetBrokenSources()
{
    y2warning("Pkg::SourceGetBrokenSources() is obsoleted, it's not needed anymore.");
    return YCPList();
}


/****************************************************************************************
 * @builtin SourceLoad
 *
 * @short Load resolvables from the installation sources
 * @description
 * Refresh the pool - Add/remove resolvables from the enabled/disabled sources.
 * @return boolean True on success
 **/
YCPValue
PkgModuleFunctions::SourceLoad()
{
    bool success = true;

    for (std::vector<YRepo_Ptr>::iterator it = repos.begin();
       it != repos.end(); ++it)
    {
	// load resolvables only from enabled repos
	if ((*it)->repoInfo().enabled())
	{
	    zypp::RepoManager repomanager = CreateRepoManager();

	    // autorefresh the source
	    if ((*it)->repoInfo().autorefresh())
	    {
		y2milestone("Autorefreshing source: %s", (*it)->repoInfo().alias().c_str());
		repomanager.refreshMetadata((*it)->repoInfo());
		# warning TODO?: Do we need to rebuild the cache after refresh??
	    }

	    try 
	    {
		// build cache if needed
		if (!repomanager.isCached((*it)->repoInfo()))
		{
		    y2milestone("Caching source '%s'...", (*it)->repoInfo().alias().c_str());
		    repomanager.buildCache((*it)->repoInfo());
		}

		zypp::Repository repository = repomanager.createFromCache((*it)->repoInfo());

		// load resolvables
		zypp::ResStore store = repository.resolvables();
		zypp_ptr()->addResolvables(store);

		y2milestone("Found %d resolvables", store.size());
	    }
	    catch(const zypp::repo::RepoNotCachedException &excpt )
	    {
		std::string alias = (*it)->repoInfo().alias();
		y2error ("Resolvables from '%s' havn't been loaded: %s", alias.c_str(), excpt.asString().c_str());
		_last_error.setLastError("'" + alias + "': " + excpt.asUserString());
		success = false;

		// FIXME ??
		/*
		// disable the source
		y2error("Disabling source %s", url.c_str());
		repo->disable();
		*/
	    }
	    catch (const zypp::Exception& excpt)
	    {
		y2internal("Caught unknown error");
		success = false;
	    }
	}
    }

    return YCPBoolean(success);
}


/****************************************************************************************
 * @builtin SourceStartManager
 *
 * @short Start the source manager - restore the sources and load the resolvables
 * @description
 * Calls SourceRestore(), if argument enable is true SourceLoad() is called.
 * @param boolean enable If true the resolvables are loaded from the enabled sources
 *
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceStartManager (const YCPBoolean& enable)
{
    YCPValue success = SourceRestore();

    if( enable->value() )
    {
	if (!success->asBoolean()->value())
	{
	    y2warning("SourceStartManager: Some sources have not been restored, loading only the active sources...");
	}

	// enable all sources and load the resolvables
	success = YCPBoolean(SourceLoad()->asBoolean()->value() && success->asBoolean()->value());
    }

    return success;
}

/****************************************************************************************
 * @builtin SourceStartCache
 *
 * @short Make sure the InstSrcManager is up, and return the list of SrcIds.
 * @description
 * Make sure the InstSrcManager is up, and return the list of SrcIds.
 * In fact nothing more than:
 *
 * <code>
 *   SourceStartManager( enabled_only );
 *   return SourceGetCurrent( enabled_only );
 * </code>
 *
 * @param boolean enabled_only If true, make sure all InstSrces are enabled according to
 * their default, and return the Ids of enabled InstSrces only. If false, return
 * the Ids of all known InstSrces.
 *
 * @return list<integer> list of SrcIds
 **/
YCPValue
PkgModuleFunctions::SourceStartCache (const YCPBoolean& enabled)
{
    try
    {
	SourceStartManager(enabled);

	return SourceGetCurrent(enabled);
    }
    catch (const zypp::Exception& excpt)
    {
	y2error ("Error in SourceStartCache: %s", excpt.asString().c_str());
	_last_error.setLastError(excpt.asUserString());
    }
    // catch an exception from boost (e.g. a file cannot be read by non-root user)
    catch (const std::exception& err)
    {
	y2error ("Error in SourceStartCache: %s", err.what());
	_last_error.setLastError(err.what());
    }
    catch (...)
    {
	y2error("Unknown error in SourceStartCache");
    }

    return YCPList();
}

/****************************************************************************************
 * @builtin SourceCleanupBroken
 *
 * @short Clean up all sources that were not properly restored on the last
 * call of SourceStartManager or SourceStartCache.
 *
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceCleanupBroken ()
{
    y2warning("Pkg::SourceCleanupBroken() is obsoleted, it's not needed anymore.");
    return YCPBoolean(true);
}

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

    unsigned long index = 0;
    for( std::vector<YRepo_Ptr>::const_iterator it = repos.begin(); it != repos.end() ; ++it )
    {
	index++;

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

/****************************************************************************************
 * @builtin SourceReleaseAll
 *
 * @short Release all medias hold by all sources
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceReleaseAll ()
{
    y2milestone("Releasing all sources...");

    for (std::vector<YRepo_Ptr>::iterator it = repos.begin();
	it != repos.end(); ++it)
    {
        try
        {
            (*it)->mediaAccess()->release();
        }
        catch (const zypp::media::MediaException & ex)
        {
            y2warning("Failed to release media for repo: %s", ex.msg().c_str());
        }
    }

    return YCPBoolean(true);
}

/******************************************************************************
 * @builtin SourceSaveAll
 *
 * @short Save all InstSrces.
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceSaveAll ()
{
    y2milestone("Saving the source setup...");

    zypp::RepoManager repomanager = CreateRepoManager();

    // TODO: can it be better?
    // Note: we have to be very careful here, user could do _very_ strange actions
    // e.g. rename repo A to B, add a new repo with alias A, remove it and
    //      then rename C to A...

    // remove deleted repos (the old configuration)
    for (std::vector<YRepo_Ptr>::iterator it = repos.begin();
	it != repos.end(); ++it)
    {
	// the repo has been removed or has been renamed
	if ((*it)->isDeleted() || (*it)->repoInfo().alias() != (*it)->origRepoAlias())
	{
	    // remove the cache
	    if (repomanager.isCached((*it)->repoInfo()))
	    {
		y2milestone("Removing cache for '%s'...", (*it)->repoInfo().alias().c_str());
		repomanager.cleanCache((*it)->repoInfo());
	    }

	    try
	    {
		repomanager.getRepositoryInfo((*it)->origRepoAlias());
		y2milestone("Removing repository '%s'", (*it)->repoInfo().alias().c_str());
		repomanager.removeRepository(zypp::RepoInfo().setAlias((*it)->origRepoAlias()));
	    }
	    catch (const zypp::repo::RepoNotFoundException &ex)
	    {
		y2warning("No such repository: %s", (*it)->origRepoAlias().c_str());
	    }
	    catch (const zypp::Exception & excpt)
	    {
		y2error("Pkg::SourceSaveAll has failed: %s", excpt.msg().c_str() );
		_last_error.setLastError(excpt.asUserString());
		return YCPBoolean(false);
	    }
	}
    }

    // save all repos (the current configuration)
    for (std::vector<YRepo_Ptr>::iterator it = repos.begin();
	it != repos.end(); ++it)
    {
	if (!(*it)->isDeleted())
	{
	    std::string current_alias = (*it)->repoInfo().alias();

	    try
	    {
		try
		{
		    // if the repository already exists then just modify it
		    repomanager.getRepositoryInfo(current_alias);
		    y2milestone("Modifying repository '%s'", current_alias.c_str());
		    repomanager.modifyRepository(current_alias, (*it)->repoInfo());
		}
		catch (const zypp::repo::RepoNotFoundException &ex)
		{
		    // the repository was not found, add it
		    y2milestone("Adding repository '%s'", current_alias.c_str());
		    repomanager.addRepository((*it)->repoInfo());
		}
	    }
	    catch (zypp::Exception & excpt)
	    {
		y2error("Pkg::SourceSaveAll has failed: %s", excpt.msg().c_str() );
		_last_error.setLastError(excpt.asUserString());
		return YCPBoolean(false);
	    }
	}
    }

    y2milestone("All sources have been saved");

    return YCPBoolean(true);
}

/****************************************************************************************
 * @builtin SourceFinishAll
 *
 * @short Save and then disable all InstSrces.
 * @description
 * If there are no enabled sources, do nothing
 * (idempotence hack, broken design: #155459, #176013, use SourceSaveAll).
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceFinishAll ()
{
    try
    {
	bool found_enabled = false;
	for (std::vector<YRepo_Ptr>::iterator it = repos.begin();
	    it != repos.end(); ++it)
	{
	    if ((*it)->repoInfo().enabled() && !(*it)->isDeleted())
	    {
		found_enabled = true;
		break;
	    }
	}

	if (!found_enabled)
	{
	    y2milestone( "No enabled sources, skipping SourceFinishAll()" );
	    return YCPBoolean( true );
	}

	SourceSaveAll();

	y2milestone( "Disabling all sources...") ;
	for (std::vector<YRepo_Ptr>::iterator it = repos.begin();
	    it != repos.end(); ++it)
	{
            (*it)->repoInfo().setEnabled(false);
	}
	// TODO FIXME remove all resolvables??
    }
    catch (zypp::Exception & excpt)
    {
	y2error("Pkg::SourceFinishAll has failed: %s", excpt.msg().c_str() );
	_last_error.setLastError(excpt.asUserString());
	return YCPBoolean(false);
    }

    y2milestone("All sources have been saved and disabled");

    return YCPBoolean(true);
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

    if (type_conversion_table.empty())
    {
      // initialize the conversion map
      type_conversion_table["rpm-md"] = "YUM";
      type_conversion_table["yast2"] = "YaST";
      type_conversion_table["plaindir"] = "Plaindir";
    }

    // convert type to the old strings ("YaST", "YUM" or "Plaindir")
    std::string srctype = repo->repoInfo().type().asString();
    std::map<std::string,std::string>::const_iterator it = type_conversion_table.find(srctype);

    // found in the conversion table
    if (it != type_conversion_table.end())
    {
	srctype = it->second;
    }

    data->add( YCPString("enabled"),		YCPBoolean(repo->repoInfo().enabled()));
    data->add( YCPString("autorefresh"),	YCPBoolean(repo->repoInfo().autorefresh()));
    data->add( YCPString("type"),		YCPString(srctype));
#warning FIXME: "product_dir" is always "/"
//    data->add( YCPString("product_dir"),	YCPString(repo->path().asString()));
    data->add( YCPString("product_dir"),	YCPString("/"));
    
    // check if there is an URL
    if (repo->repoInfo().baseUrlsBegin() != repo->repoInfo().baseUrlsEnd())
    {
	data->add( YCPString("url"),		YCPString(repo->repoInfo().baseUrlsBegin()->asString()));
    }

    data->add( YCPString("alias"),		YCPString(repo->repoInfo().alias()));

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

#warning FIXME SourceMediaData is NOT implemented!!!
/*  data->add( YCPString("media_count"),	YCPInteger(src.numberOfMedia()));
  data->add( YCPString("media_id"),	YCPString(src.unique_id()));
  data->add( YCPString("media_vendor"),	YCPString(src.vendor()));
*/
#warning SourceMediaData returns URL without password

    if (repo->repoInfo().baseUrlsBegin() != repo->repoInfo().baseUrlsEnd())
    {
	data->add( YCPString("url"),	YCPString(repo->repoInfo().baseUrlsBegin()->asString()));
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


YCPValue PkgModuleFunctions::SourceProvideFileCommon(const YCPInteger &id,
					       const YCPInteger &mid,
					       const YCPString& f,
					       const YCPBoolean & optional)
{
    CallSourceReportInit();
    CallSourceReportStart(_("Downloading file..."));

    bool found = true;
    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
      found = false;

    extern ZyppRecipients::MediaChangeSensitivity _silent_probing;
    // remember the current value
    ZyppRecipients::MediaChangeSensitivity _silent_probing_old = _silent_probing;

    // disable media change callback for optional file
    if (optional->value())
	_silent_probing = ZyppRecipients::MEDIA_CHANGE_OPTIONALFILE;

    zypp::filesystem::Pathname path; // FIXME use ManagedMedia
    if (found)
    {
	try
	{
	    path = repo->mediaAccess()->provideFile(f->value(), mid->value());
	    y2milestone("local path: '%s'", path.asString().c_str());
	}
	catch (const zypp::Exception& excpt)
	{
	    found = false;

	    if (!optional->value())
	    {
		_last_error.setLastError(excpt.asUserString());
		y2milestone("File not found: %s", f->value_cstr());
	    }
	}
    }

    // set the original probing value
    _silent_probing = _silent_probing_old;

    CallSourceReportEnd(_("Downloading file..."));
    CallSourceReportDestroy();

    if (found)
    {
	return YCPString(path.asString());
    }
    else
    {
	return YCPVoid();
    }
}


/****************************************************************************************
 * @builtin SourceProvideFile
 *
 * @short Make a file available at the local filesystem
 * @description
 * Let an InstSrc provide some file (make it available at the local filesystem).
 *
 * @param integer SrcId	Specifies the InstSrc .
 * @param integer medianr Number of the media the file is located on ('1' for the 1st media).
 * @param string file Filename relative to the media root.
 *
 * @return string local path as string
 **/
YCPValue
PkgModuleFunctions::SourceProvideFile (const YCPInteger& id, const YCPInteger& mid, const YCPString& f)
{
    return SourceProvideFileCommon(id, mid, f, false /*optional*/);
}

/****************************************************************************************
 * @builtin SourceProvideOptionalFile
 *
 * @short Make an optional file available at the local filesystem
 * @description
 * Let an InstSrc provide some file (make it available at the local filesystem).
 * If the file doesn't exist don't ask user for another medium and return nil
 *
 * @param integer SrcId	Specifies the InstSrc .
 * @param integer medianr Number of the media the file is located on ('1' for the 1st media).
 * @param string file Filename relative to the media root.
 *
 * @return string local path as string
 **/
YCPValue
PkgModuleFunctions::SourceProvideOptionalFile (const YCPInteger& id, const YCPInteger& mid, const YCPString& f)
{
    return SourceProvideFileCommon(id, mid, f, true /*optional*/);
}

/****************************************************************************************
 * @builtin SourceProvideDir
 * @short make a directory available at the local filesystem
 * @description
 * Let an InstSrc provide some directory (make it available at the local filesystem) and
 * all the files within it (non recursive).
 *
 * @param integer SrcId	Specifies the InstSrc .
 * @param integer medianr Number of the media the file is located on ('1' for the 1st media).
 * @param string dir Directoryname relative to the media root.
 * @return string local path as string
 */
YCPValue
PkgModuleFunctions::SourceProvideDir (const YCPInteger& id, const YCPInteger& mid, const YCPString& d)
{
    bool found = true;
    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
      found = false;

    zypp::filesystem::Pathname path; // FIXME user ManagedMedia

    if (found)
    {
	try
	{
	    path = repo->mediaAccess()->provideDir(d->value(), true, mid->value());
	}
	catch (const zypp::Exception& excpt)
	{
            _last_error.setLastError(excpt.asUserString());
            y2milestone ("Directory not found: %s", d->value_cstr());
            found = false;
	}
    }

    if (found)
    {
	return YCPString(path.asString());
    }
    else
    {
	return YCPVoid();
    }
}

/****************************************************************************************
 * @builtin SourceChangeUrl
 * @short Change Source URL
 * @description
 * Change url of an InstSrc. Used primarely when re-starting during installation
 * and a cd-device changed from hdX to srX since ide-scsi was activated.
 * @param integer SrcId Specifies the InstSrc.
 * @param string url The new url to use.
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceChangeUrl (const YCPInteger& id, const YCPString& u)
{
    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
	return YCPBoolean(false);

    try
    {
        if (repo->repoInfo().baseUrlsSize() > 1)
        {
            // store current urls
            std::set<zypp::Url> baseUrls;
            for (std::set<zypp::Url>::const_iterator i = repo->repoInfo().baseUrlsBegin();
                    i != repo->repoInfo().baseUrlsEnd(); ++i)
                baseUrls.insert(*i);
            
            // reset url list and store the new one there
            repo->repoInfo().setBaseUrl(zypp::Url(u->value()));

            // add the rest of base urls
            for (std::set<zypp::Url>::const_iterator i = baseUrls.begin();
                    i != baseUrls.end(); ++i)
                repo->repoInfo().addBaseUrl(*i);
        }
        else
            repo->repoInfo().setBaseUrl(zypp::Url(u->value()));
    }
    catch (const zypp::Exception & excpt)
    {
        _last_error.setLastError(excpt.asUserString());
        y2error ("Cannot set the new URL for source %s (%lld): %s",
            repo->repoInfo().alias().c_str(), id->asInteger()->value(), excpt.msg().c_str());
	return YCPBoolean(false);
    }

    return YCPBoolean(true);
}

/****************************************************************************************
 * @builtin SourceInstallOrder
 *
 * @short not implemented, do not use (Explicitly set an install order.)
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceInstallOrder (const YCPMap& ord)
{
    /* TODO FIXME
  YCPList args;
  args->add (ord);

  //-------------------------------------------------------------------------------------//
  YcpArgLoad decl(__FUNCTION__);

  YCPMap & order_map( decl.arg<YT_MAP, YCPMap>() );

  if ( ! decl.load( args ) ) {
    return pkgError_bad_args;
  }
  //-------------------------------------------------------------------------------------//

  InstSrcManager::InstOrder order;
  order.reserve( order_map->size() );
  bool error = false;

  for ( YCPMapIterator it = order_map->begin(); it != order_map->end(); ++it ) {

    if ( it.value()->isInteger() ) {
      InstSrc::UniqueID uId( it.value()->asInteger()->value() );
      InstSrcManager::ISrcId source_id( _y2pm.instSrcManager().getSourceByID( uId ) );
      if ( source_id ) {
	if ( source_id->enabled() ) {
	  order.push_back( uId );  // finaly ;)

	} else {
	  y2error ("order map entry '%s:%s': source not enabled",
		    it.key()->toString().c_str(),
		    it.value()->toString().c_str() );
	  error = true;
	}
      } else {
	y2error ("order map entry '%s:%s': bad source id",
		  it.key()->toString().c_str(),
		  it.value()->toString().c_str() );
	error = true;
      }
    } else {
      y2error ("order map entry '%s:%s': integer value expected",
		it.key()->toString().c_str(),
		it.value()->toString().c_str() );
      error = true;
    }
  }
  if ( error ) {
    return pkgError( Error::E_bad_args );
  }

  // store new instorder
  _y2pm.instSrcManager().setInstOrder( order );
*/

#warning SourceInstallOrder is not implemented
  return YCPBoolean( true );
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


/** Create a Source and immediately put it into the SourceManager.
 * \return the SourceId
 * \throws Exception if Source creation fails
*/
std::vector<zypp::RepoInfo>::size_type
PkgModuleFunctions::createManagedSource( const zypp::Url & url_r,
		     const zypp::Pathname & path_r,
		     const bool base_source,
		     const std::string& type )
{
    // parse URL
    y2milestone ("Original URL: %s", url_r.asString().c_str());

    // #158850#c17, if the URL contains an alias, we use that
    zypp::Url url;

    std::string alias = removeAlias(url_r, url);
    y2milestone("Alias from URL: '%s'", alias.c_str());

#warning FIXME: use base_source (base_source vs. addon)
#warning FIXME: use path_r (product directory)
/*
    FIXME: add the product dir to the URL?
    std::string prod_dir = path_r.asString();
    if (!prod_dir.empty() && prod_dir != "/")
    {
	y2milestone("Using product directory: %s", prod_dir.c_str());
	std::string path = url.getPathName();
	path = path + "/"
    }
*/

    // repository type
    zypp::repo::RepoType repotype;
    zypp::RepoManager repomanager = CreateRepoManager();

    if (!type.empty())
    {
	try
	{
	    // do conversion from the Yast type ("YaST", "YUM", "Plaindir")
	    // to libzypp type ("yast", "yum", "plaindir")
	    // we can simply use toLower instead of a conversion table
	    // in this case
	    repotype.parse(zypp::str::toLower(type));
	}
	catch (zypp::repo::RepoUnknownTypeException &e)
	{
	    y2warning("Unknown source type '%s'", type.c_str());
	}
    }

    // the type is not specified or is wrong, autoprobe the type 
    if (repotype == zypp::repo::RepoType::NONE)
    {
	y2milestone("Probing source type: '%s'", url.asString().c_str());

	// autoprobe type of the repository 
	repotype = repomanager.probe(url);
    }

    y2milestone("Using source type: %s", repotype.asString().c_str());


    // create source definition object
    zypp::RepoInfo repo;

    repo.setAlias(alias.empty() ? timestamp() : alias);
    repo.setType(repotype);
    repo.addBaseUrl(url);
    repo.setEnabled(true);
    repo.setAutorefresh(true);

    y2milestone("Adding source '%s' (%s)", repo.alias().c_str(), url.asString().c_str());

/*    try
    {*/

//	repomanager.addRepository(repo);

	repomanager.refreshMetadata(repo);

	// TODO FIXME handle existing alias better

/*    }
    catch (const zypp::MediaException &e)
    {
	y2error("Cannot read source data");
    }
    catch (const ParseException &e)
    {
	y2error("Cannot parse source data");
    }
    catch (const RepoAlreadyExistsException &e)
    {
	y2error("Source '%s' already exists", repo.alias().c_str());
    }
*/

    // build cache if needed
    if (!repomanager.isCached(repo))
    {
	y2milestone("Caching source '%s'...", repo.alias().c_str());
	repomanager.buildCache(repo);
    }

    repos.push_back(new YRepo(repo));

    y2milestone("Added source '%s': '%s', enabled: %s, autorefresh: %s",
	repo.alias().c_str(),
	repo.baseUrlsBegin()->asString().c_str(),
	repo.enabled() ? "true" : "false",
	repo.autorefresh() ? "true" : "false"
    );

/*
  // if the source has empty alias use a time stamp
  if (newsrc.alias().empty())
  {
    // use product name+edition as the alias
    // (URL is not enough for different sources in the same DVD drive)
    // alias must be unique, add timestamp
    zypp::ResStore products = newsrc.resolvables (zypp::Product::TraitsType::kind);
    zypp::ResStore::iterator
      b = products.begin (),
      e = products.end ();  
    if (b != e)
    {
      zypp::ResObject::Ptr p = *b;
      alias = p->name () + '-' + p->edition ().asString () + '-';
    }
    alias += timestamp ();
  
    newsrc.setAlias( alias );
    y2milestone("Using string '%s' as the alias", alias.c_str());
  }

  zypp::SourceManager::SourceId id = zypp::SourceManager::sourceManager()->addSource( newsrc );
  y2milestone("Added source %lu: %s %s (alias %s)", id, new_url.asString().c_str(), path_r.asString().c_str(), alias.c_str() );
*/

    // the source is at the end of the list
    return repos.size() - 1;
}

/****************************************************************************************
 * @builtin SourceCacheCopyTo
 *
 * @short Copy cache data of all installation sources to the target
 * @description
 * Copy cache data of all installation sources to the target located below 'dir'.
 * To be called at end of initial installation.
 *
 * @param string dir Root directory of target.
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceCacheCopyTo (const YCPString& dir)
{
  y2warning( "Pkg::SourceCacheCopyTo is obsolete now, it does nothing" );

  return YCPBoolean( true );
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
    _last_error.setLastError(expt.asUserString());
    return YCPList();
  }

  zypp::Pathname pn(pd->value ());

  YCPList ids;
  std::vector<zypp::RepoInfo>::size_type id;

  if ( pd->value().empty() ) {

    // scan all sources
    zypp::MediaProductSet products;

    try
    {
	y2milestone("Scanning products in %s ...", url.asString().c_str());
	zypp::productsInMedia(url, products);
    }
    catch ( const zypp::Exception& excpt)
    {
	y2error("Scanning products for '%s' has failed"
		, url.asString().c_str());
	_last_error.setLastError(excpt.asUserString());
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
	    id = createManagedSource(url, it->_dir, false, "");
	    ids->add( YCPInteger(id) );
	}
	catch ( const zypp::Exception& excpt)
	{
	    y2error("SourceScan for '%s' product '%s' has failed"
		, url.asString().c_str(), pn.asString().c_str());
	    _last_error.setLastError(excpt.asUserString());
	}
    }
  } else {
    y2debug("Creating source...");

    try
    {
	id = createManagedSource(url, pn, false, "");
	ids->add( YCPInteger(id) );
    }
    catch ( const zypp::Exception& excpt)
    {
	y2error("SourceScan for '%s' product '%s' has failed"
	    , url.asString().c_str(), pn.asString().c_str());
	_last_error.setLastError(excpt.asUserString());
    }
  }

  y2milestone("Found sources: %s", ids->toString().c_str() );

  return ids;
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
    _last_error.setLastError(expt.asUserString());
    return YCPInteger (-1);
  }


  YCPList ids;
  std::vector<zypp::RepoInfo>::size_type ret = -1;

  const std::string type = source_type->value();

  if ( pd->value().empty() ) {
    // scan all sources
    zypp::MediaProductSet products;

    try {
	y2milestone("Scanning products in %s ...", url.asString().c_str());
	zypp::productsInMedia(url, products);
    }
    catch ( const zypp::Exception& excpt)
    {
	_last_error.setLastError(excpt.asUserString());
	y2error( "Cannot read the product list from the media" );
	return YCPInteger(ret);
    }

    if( products.empty() )
    {
	// no products found, use the base URL instead
	zypp::MediaProductEntry entry ;
	products.insert( entry );
    }

    for( zypp::MediaProductSet::const_iterator it = products.begin();
	it != products.end() ; ++it )
    {
	try
	{
	    std::vector<zypp::RepoInfo>::size_type id = createManagedSource(url, it->_dir, base, type);

	    YRepo_Ptr repo = logFindRepository(id);
	    repo->repoInfo().setEnabled(true);
	
	    zypp::RepoManager repomanager = CreateRepoManager();

	    // update Repository
	    zypp::Repository r = repomanager.createFromCache(repo->repoInfo());

	    // add resolvables
	    zypp_ptr()->addResolvables(r.resolvables());

	    CallSourceReportInit();
	    CallSourceReportStart(_("Parsing files..."));
    	    zypp_ptr()->addResolvables (r.resolvables());
	    CallSourceReportEnd(_("Parsing files..."));
	    CallSourceReportDestroy();

	    // return the id of the first product
	    if ( it == products.begin() )
		ret = id;

	}
	catch ( const zypp::Exception& excpt)
	{
	    y2error("SourceCreate for '%s' product '%s' has failed"
		, url.asString().c_str(), pn.asString().c_str());
	    _last_error.setLastError(excpt.asUserString());
	}
    }
  } else {
    y2debug("Creating source...");

    try
    {
	ret = createManagedSource(url, pn, base, type);

	YRepo_Ptr repo = logFindRepository(ret);
	repo->repoInfo().setEnabled(true);

	zypp::RepoManager repomanager = CreateRepoManager();
	// update Repository
	zypp::Repository r = repomanager.createFromCache(repo->repoInfo());

	CallSourceReportInit();
	CallSourceReportStart(_("Parsing files..."));
    	zypp_ptr()->addResolvables (r.resolvables());
	CallSourceReportEnd(_("Parsing files..."));
	CallSourceReportDestroy();
    }
    catch ( const zypp::Exception& excpt)
    {
	y2error("SourceCreate for '%s' product '%s' has failed"
	    , url.asString().c_str(), pn.asString().c_str());
	_last_error.setLastError(excpt.asUserString());
    }
  }

  PkgFreshen();
  return YCPInteger(repos.size() - 1);
}


/****************************************************************************************
 * @builtin SourceSetEnabled
 *
 * @short Set the default activation state of an InsrSrc.
 * @param integer SrcId Specifies the InstSrc.
 * @param boolean enabled Default activation state of source.
 *
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceSetEnabled (const YCPInteger& id, const YCPBoolean& e)
{
    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
	return YCPBoolean(false);

    // no change required
    bool enable = e->value();
    if ((enable && repo->repoInfo().enabled())
	|| (!enable && !repo->repoInfo().enabled()))
    return YCPBoolean(true);

    bool success = true;

    try
    {
	repo->repoInfo().setEnabled(enable);

	// FIXME load (remove) resolvables only when they are missing (present)
	zypp::RepoManager repomanager = CreateRepoManager();
	// update Repository
	zypp::Repository r = repomanager.createFromCache(repo->repoInfo());

	// add/remove resolvables
	if (enable)
	    zypp_ptr()->addResolvables(r.resolvables());
	else
	    zypp_ptr()->removeResolvables(r.resolvables());

	PkgFreshen();
    }
    catch (const zypp::Exception& excpt)
    {
	std::string alias = repo->repoInfo().alias();
	y2error ("Error for '%s': %s", alias.c_str(), excpt.asString().c_str());
	_last_error.setLastError(alias + ": " + excpt.asUserString());
	success = false;
    }

    return YCPBoolean(success);
}

/****************************************************************************************
 * @builtin SourceSetAutorefresh
 *
 * @short Set whether this source should automaticaly refresh it's
 * meta data when it gets enabled. (default true, if not CD/DVD)
 * @param integer SrcId Specifies the InstSrc.
 * @param boolean enabled Whether autorefresh should be turned on or off.
 *
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceSetAutorefresh (const YCPInteger& id, const YCPBoolean& e)
{
    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
	return YCPBoolean(false);

    repo->repoInfo().setAutorefresh(e->value());

    return YCPBoolean( true );
}

/****************************************************************************************
 * @builtin SourceFinish
 * @short Disable an Installation Source
 * @param integer SrcId Specifies the InstSrc.
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceFinish (const YCPInteger& id)
{
    return SourceSetEnabled(id, false);
}

/****************************************************************************************
 * @builtin SourceRefreshNow
 * @short Attempt to immediately refresh a Source
 * @description
 * The InsrSrc will be encouraged to check and refresh all metadata
 * cached on disk.
 *
 * @param integer SrcId Specifies the InstSrc.
 *
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceRefreshNow (const YCPInteger& id)
{
    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
	return YCPBoolean(false);

    try
    {
	zypp::RepoManager repomanager = CreateRepoManager();
	repomanager.refreshMetadata(repo->repoInfo());
    }
    catch ( const zypp::Exception & expt )
    {
	y2error ("Error while refreshing the source: %s", expt.asString().c_str());
	return YCPBoolean(false);
    }

    return YCPBoolean( true );
}

/****************************************************************************************
 * @builtin SourceDelete
 * @short Delete a Source
 * @description
 * Delete an InsrSrc. The InsrSrc together with all metadata cached on disk
 * is removed. The SrcId passed becomes invalid (other SrcIds stay valid).
 *
 * @param integer SrcId Specifies the InstSrc.
 *
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceDelete (const YCPInteger& id)
{
    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
	return YCPBoolean(false);

    bool success = true;

    try
    {

#warning FIXME: SourceDelete: remove resolvables if they have been loaded
/*
	// update Repository
	zypp::Repository r = repomanager.createFromCache(src);

	// remove resolvables
    	zypp_ptr()->removeResolvables(r.resolvables());
*/
	// update 'repos'

	repo->setDeleted();

	PkgFreshen();
    }
    catch (const zypp::Exception& excpt)
    {
	std::string alias = repo->repoInfo().alias();
	y2error ("Error for '%s': %s", alias.c_str(), excpt.asString().c_str());
	_last_error.setLastError(alias + ": " + excpt.asUserString());
	success = false;
    }

    return YCPBoolean(success);
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
	    src_map->add(YCPString("alias"), YCPString((*it)->repoInfo().alias()));

	    ret->add(src_map);
	}
    }

    return ret;
}

/****************************************************************************************
 * @builtin SourceEditSet
 *
 * @short Configure properties of installation sources
 * @description
 * Set states of installation sources. Note: Enabling/disabling a source does not
 * (un)load the packages from the source! Use SourceSetEnabled() if you need to refresh
 * the packages in the pool.
 *
 * @param list source_states List of source states. Same format as returned by
 * @see SourceEditGet.
 *
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceEditSet (const YCPList& states)
{
  bool error = false;

  for (int index = 0; index < states->size(); index++ )
  {
    if( ! states->value(index)->isMap() )
    {
	ycperror( "Pkg::SourceEditSet, entry not a map at index %d", index);
	error = true;
	continue;
    }

    YCPMap descr = states->value(index)->asMap();

    if (descr->value( YCPString("SrcId") ).isNull() || !descr->value(YCPString("SrcId"))->isInteger())
    {
	ycperror( "Pkg::SourceEditSet, SrcId not defined for a source description at index %d", index);
	error = true;
	continue;
    }

    std::vector<YRepo_Ptr>::size_type id = descr->value( YCPString("SrcId") )->asInteger()->value();

    YRepo_Ptr repo = logFindRepository(id);
    if (!repo)
    {
	ycperror( "Pkg::SourceEditSet, source %d not found", index);
	error = true;
	continue;
    }

    // now, we have the source
    if( ! descr->value( YCPString("enabled")).isNull() && descr->value(YCPString("enabled"))->isBoolean ())
    {
	bool enable = descr->value(YCPString("enabled"))->asBoolean ()->value();

	if (repo->repoInfo().enabled() != enable)
	{
	    ycpwarning("Pkg::SourceEditSet() does not refresh the pool (src: %d, state: %s)", id, enable ? "disabled -> enabled" : "enabled -> disabled");
	}

        y2debug("set enabled: %d", enable);
	repo->repoInfo().setEnabled(enable);
    }

    if( !descr->value(YCPString("autorefresh")).isNull() && descr->value(YCPString("autorefresh"))->isBoolean ())
    {
        bool autorefresh = descr->value(YCPString("autorefresh"))->asBoolean()->value();
        y2debug("set autorefresh: %d", autorefresh);
	repo->repoInfo().setAutorefresh( autorefresh );
    }

    if( !descr->value(YCPString("alias")).isNull() && descr->value(YCPString("alias"))->isString())
    {
	// rename the source
        y2debug("set alias: %s", descr->value(YCPString("alias"))->asString()->value().c_str());
	repo->repoInfo().setAlias(descr->value(YCPString("alias"))->asString()->value());
    }

#warning SourceEditSet ordering not implemented yet
  }

  PkgFreshen();
  return YCPBoolean( !error );
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// DEPRECATED
//
/////////////////////////////////////////////////////////////////////////////////////////

/****************************************************************************************
 * Pkg::SourceRaisePriority (integer SrcId) -> bool
 *
 * Raise priority of source.
 *
 * @param SrcId Specifies the InstSrc.
 *
 * @return bool
 **/
YCPValue
PkgModuleFunctions::SourceRaisePriority (const YCPInteger& id)
{
#warning SourceRaisePriority is not implemented
    y2warning("SourceRaisePriority is NOT implemented");
/*    zypp::Source_Ref src;

    try {
	src = logFindRepository(id->value());
    }
    catch (const zypp::Exception& excpt)
    {
	return YCPBoolean(false);
    }

    // raise priority by one
    src.setPriority(src.priority() + 1);
*/

    return YCPBoolean( true );

}

/****************************************************************************************
 * Pkg::SourceLowerPriority (integer SrcId) -> void
 *
 * Lower priority of source.
 *
 * @param SrcId Specifies the InstSrc.
 *
 * @return bool
 */
YCPValue
PkgModuleFunctions::SourceLowerPriority (const YCPInteger& id)
{
#warning SourceLowerPriority is not implemented
    y2warning("SourceLowerPriority is NOT implemented");
/*
    zypp::Source_Ref src;

    try {
	src = logFindRepository(id->value());
    }
    catch (const zypp::Exception& excpt)
    {
	return YCPBoolean(false);
    }

    // lower priority by one
    src.setPriority(src.priority() - 1);
*/
    return YCPBoolean( true );
}

/****************************************************************************************
 * Pkg::SourceSaveRanks () -> boolean
 *
 * @short Obsoleted function, do not use
 * @return boolean true
 **/
YCPValue
PkgModuleFunctions::SourceSaveRanks ()
{
  y2error( "SourceSaveRanks not implemented" );

  return YCPBoolean( true );
}

/****************************************************************************************
 * @builtin SourceMoveDownloadArea
 *
 * @short Move download area of CURL-based sources to specified directory
 * @param path specifies the path to move the download area to
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceMoveDownloadArea (const YCPString & path)
{
#warning SourceMoveDownloadArea is NOT implemented
// TODO FIXME
    /*
    try
    {
	y2milestone( "Moving download area of all sources to %s", path->value().c_str()) ;
	zypp::SourceManager::sourceManager()->reattachSources (path->value());
    }
    catch (zypp::Exception & excpt)
    {
	_last_error.setLastError(excpt.asUserString());
	y2error("Pkg::SourceMoveDownloadArea has failed: %s", excpt.msg().c_str() );
	return YCPBoolean(false);
    }

    y2milestone( "Download areas moved");
*/
    return YCPBoolean(true);
}


