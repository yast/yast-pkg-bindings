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
#include <zypp/media/Mount.h>
#include <zypp/Pathname.h>

#include <zypp/RepoInfo.h>
#include <zypp/RepoManager.h>
#include <zypp/Fetcher.h>
#include <zypp/repo/RepoType.h>
#include <zypp/MediaProducts.h>

#include <zypp/base/String.h>

#include <sstream> // ostringstream


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

void PkgModuleFunctions::CallInitDownload(const std::string &task)
{
    // get the YCP callback handler for destroy event
    Y2Function* ycp_handler = _callbackHandler._ycpCallbacks.createCallback(CallbackHandler::YCPCallbacks::CB_InitDownload);

    // is the callback registered?
    if (ycp_handler != NULL)
    {
	ycp_handler->appendParameter(YCPString(task));
	// evaluate the callback function
	ycp_handler->evaluateCall();
    }
}

void PkgModuleFunctions::CallDestDownload()
{
    // get the YCP callback handler for destroy event
    Y2Function* ycp_handler = _callbackHandler._ycpCallbacks.createCallback(CallbackHandler::YCPCallbacks::CB_DestDownload);

    // is the callback registered?
    if (ycp_handler != NULL)
    {
	// evaluate the callback function
	ycp_handler->evaluateCall();
    }
}

// this method should be used instead of RepoManager::refreshMetadata()
void PkgModuleFunctions::RefreshWithCallbacks(const zypp::RepoInfo &repo)
{
    CallInitDownload(std::string(_("Refreshing repository ") + repo.alias()));

    try
    {
	zypp::RepoManager repomanager = CreateRepoManager();
	repomanager.refreshMetadata(repo);
    }
    catch(...)
    {
	// call the final event even in case of exception
	CallDestDownload();
	// rethrow the execption
	throw;
    }

    CallDestDownload();
}

// this method should be used instead of RepoManager::probe()
zypp::repo::RepoType PkgModuleFunctions::ProbeWithCallbacks(const zypp::Url &url)
{
    CallInitDownload(std::string(_("Probing repository ") + url.asString()));

    zypp::repo::RepoType repotype;

    extern ZyppRecipients::MediaChangeSensitivity _silent_probing;
    // remember the current value
    ZyppRecipients::MediaChangeSensitivity _silent_probing_old = _silent_probing;

    // disable media change callback for optional file
    _silent_probing = ZyppRecipients::MEDIA_CHANGE_DISABLE;

    try
    {
	// probe type of the repository 
	zypp::RepoManager repomanager = CreateRepoManager();
	repotype = repomanager.probe(url);
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

    return repotype;
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
	if (id < 0 || id >= repos.size())
	{
	    // not found
	    throw(std::exception());
	}

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

long long PkgModuleFunctions::logFindAlias(const std::string &alias) const
{
    std::vector<YRepo_Ptr>::size_type index = 0;

    for(std::vector<YRepo_Ptr>::const_iterator it = repos.begin(); it != repos.end() ; ++it, ++index)
    {
	if (!(*it)->isDeleted() && (*it)->repoInfo().alias() == alias)
	    return index;
    }

    return -1LL;
}

bool PkgModuleFunctions::aliasExists(const std::string &alias) const
{
    for(std::vector<YRepo_Ptr>::const_iterator it = repos.begin(); it != repos.end() ; ++it)
    {
	if (!(*it)->isDeleted() && (*it)->repoInfo().alias() == alias)
	    return true;
    }

    return false;
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
	_last_error.setLastError(ExceptionAsString(excpt));
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
		try
		{
		    y2milestone("Autorefreshing source: %s", (*it)->repoInfo().alias().c_str());
		    RefreshWithCallbacks((*it)->repoInfo());

		    // rebuild cache (the default policy is "if needed")
		    y2milestone("Rebuilding cache for '%s'...", (*it)->repoInfo().alias().c_str());
		    repomanager.buildCache((*it)->repoInfo());
		}
		catch (const zypp::Exception& excpt)
		{
		    // FIXME: assuming the sources are already initialized
		    y2error ("Error in SourceLoad: %s", excpt.asString().c_str());
		    _last_error.setLastError(ExceptionAsString(excpt));
		    success = false;
		}
	    }

	    if (AnyResolvableFrom((*it)->repoInfo().alias()))
	    {
		y2milestone("Resolvables from '%s' are already present, not loading", (*it)->repoInfo().alias().c_str());
	    }
	    else
	    {
		success = success && LoadResolvablesFrom((*it)->repoInfo());
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
	_last_error.setLastError(ExceptionAsString(excpt));
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
    bool ret = true;

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
	    ret = false;
        }
    }

    return YCPBoolean(ret);
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

    // remove deleted repos (the old configurations) at first
    for (std::vector<YRepo_Ptr>::iterator it = repos.begin();
	it != repos.end(); ++it)
    {
	// the repo has been removed
	if ((*it)->isDeleted())
	{
	    std::string repo_alias = (*it)->repoInfo().alias();

	    try
	    {
		// remove the metadata
		zypp::RepoStatus raw_metadata_status = repomanager.metadataStatus((*it)->repoInfo());
		if (!raw_metadata_status.empty())
		{
		    y2milestone("Removing metadata for source '%s'...", repo_alias.c_str());
		    repomanager.cleanMetadata((*it)->repoInfo());
		}

		// remove the cache
		if (repomanager.isCached((*it)->repoInfo()))
		{
		    y2milestone("Removing cache for '%s'...", repo_alias.c_str());
		    repomanager.cleanCache((*it)->repoInfo());
		}

		repomanager.getRepositoryInfo(repo_alias);
		y2milestone("Removing repository '%s'", repo_alias.c_str());
		repomanager.removeRepository((*it)->repoInfo());
	    }
	    catch (const zypp::repo::RepoNotFoundException &ex)
	    {
		// repository not found -- not critical, continue
		y2warning("No such repository: %s", repo_alias.c_str());
	    }
	    catch (const zypp::Exception & excpt)
	    {
		y2error("Pkg::SourceSaveAll has failed: %s", excpt.msg().c_str() );
		_last_error.setLastError(ExceptionAsString(excpt));
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
		_last_error.setLastError(ExceptionAsString(excpt));
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
	_last_error.setLastError(ExceptionAsString(excpt));
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


YCPValue PkgModuleFunctions::SourceProvideFileCommon(const YCPInteger &id,
					       const YCPInteger &mid,
					       const YCPString& f,
					       const YCPBoolean & optional)
{
    CallInitDownload(std::string(_("Downloading ") + f->value()));

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
		_last_error.setLastError(ExceptionAsString(excpt));
		y2milestone("File not found: %s", f->value_cstr());
	    }
	}
    }

    // set the original probing value
    _silent_probing = _silent_probing_old;

    CallDestDownload();

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
    y2warning("Pkg::SourceProvideDir() is obsoleted use Pkg::SourceProvideDirectory() instead");
    // non optional, non recursive
    return SourceProvideDirectory(id, mid, d, false, false);
}


/****************************************************************************************
 * @builtin SourceProvideDirectory
 * @short make a directory available at the local filesystem
 * @description
 * Download a directory from repository (make it available at the local filesystem) and
 * all the files within it.
 *
 * @param integer id repository to use (id)
 * @param integer mid Number of the media where the directory is located on ('1' for the 1st media).
 * @param string d Directory name relative to the media root.
 * @param boolean optional set to true if the directory may not exist (do not report errors)
 * @param boolean recursive set to true to provide all subdirectories recursively
 * @return string local path as string or nil when an error occured
 */
YCPValue
PkgModuleFunctions::SourceProvideDirectory(const YCPInteger& id, const YCPInteger& mid, const YCPString& d, const YCPBoolean &optional, const YCPBoolean &recursive)
{
    CallInitDownload(std::string(_("Downloading ") + d->value()));

    bool found = true;
    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
      found = false;

    zypp::filesystem::Pathname path; // FIXME user ManagedMedia

    extern ZyppRecipients::MediaChangeSensitivity _silent_probing;
    // remember the current value
    ZyppRecipients::MediaChangeSensitivity _silent_probing_old = _silent_probing;

    // disable media change callback for optional file
    if (optional->value())
	_silent_probing = ZyppRecipients::MEDIA_CHANGE_OPTIONALFILE;

    if (found)
    {
	try
	{
	    path = repo->mediaAccess()->provideDir(d->value(), recursive->value(), mid->value());
	}
	catch (const zypp::Exception& excpt)
	{
            _last_error.setLastError(ExceptionAsString(excpt));
            y2milestone ("Directory not found: %s", d->value_cstr());
            found = false;
	}
    }

    // set the original probing value
    _silent_probing = _silent_probing_old;

    CallDestDownload();

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
            std::set<zypp::Url> baseUrls (repo->repoInfo().baseUrlsBegin(), repo->repoInfo().baseUrlsEnd());
            
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
        _last_error.setLastError(ExceptionAsString(excpt));
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

std::string PkgModuleFunctions::UniqueAlias(const std::string &alias)
{
    // make a copy
    std::string ret = alias;

    unsigned int id = 0;
    while(aliasExists(ret))
    {
	y2milestone("Alias %s already found: %lld", ret.c_str(), logFindAlias(ret));

	// the alias already exists - add a counter 
	std::ostringstream ostr;
	ostr << alias << "_" << id;

	ret = ostr.str();

	y2milestone("Using alias %s", ret.c_str());
	++id;
    }

    return ret;
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

// helper function
zypp::Url PkgModuleFunctions::shortenUrl(const zypp::Url &url)
{
    std::string url_path = url.getPathName();
    std::string begin_path;
    std::string end_path;

    // try to convert 'http://server/dir1/dir2/dir3/dir4' -> 'http://server/dir1/.../dir4'
    unsigned int pos_first = url_path.find("/");
    if (pos_first == 0)
    {
	pos_first = url_path.find("/", 1);
    }

    if (pos_first == std::string::npos)
    {
	const int num = 5;

	// "/" not found use the beginning and the end part
	// 'http://server/very_long_directory_name' -> 'http://server/very_..._name'
	begin_path = std::string(url_path, 0, num);
	end_path = std::string(url_path, url_path.size() - num - 1, num);
    }
    else
    {
	unsigned int pos_last = url_path.rfind("/");
	if (pos_last == url_path.size() - 1)
	{
	    pos_last = url_path.rfind("/", url_path.size() - 1);
	}

	if (pos_last == pos_first || pos_last < pos_first)
	{
	    const int num = 5;

	    // "/" not found use the beginning and the end part
	    begin_path = std::string(url_path, 0, num);
	    end_path = std::string(url_path, url_path.size() - num - 1, num);
	}
	else
	{
	    begin_path = std::string(url_path, 0, pos_first + 1);
	    end_path = std::string(url_path, pos_last);
	}
    }

    std::string new_path = begin_path + "..." + end_path;
    zypp::Url ret(url);

    // use the shorter path
    ret.setPathName(new_path);
    // remove query parameters
    ret.setQueryString("");
    // remove fragmet
    ret.setFragment("");

    y2milestone("Using shortened URL: '%s' -> '%s'", url.asString().c_str(), ret.asString().c_str());
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
		     const std::string &alias_r )
{
    // parse URL
    y2milestone ("Original URL: %s", url_r.asString().c_str());

    // #158850#c17, if the URL contains an alias, we use that
    zypp::Url url;

    std::string alias = removeAlias(url_r, url);
    y2milestone("Alias from URL: '%s'", alias.c_str());

#warning FIXME: use base_source (base_source vs. addon) (will be probably not needed)

    // repository type
    zypp::repo::RepoType repotype;
    zypp::RepoManager repomanager = CreateRepoManager();

    if (!type.empty())
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
	y2milestone("Probing source type: '%s'", url.asString().c_str());

	// autoprobe type of the repository 
	repotype = ProbeWithCallbacks(url);
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

    y2milestone("Adding source '%s' (%s)", repo.alias().c_str(), url.asString().c_str());
    // note: exceptions should be caught by the calling code
    RefreshWithCallbacks(repo);

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

    repos.push_back(new YRepo(repo));

    // the new source is at the end of the list
    return YCPInteger(repos.size() - 1);
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
    _last_error.setLastError(ExceptionAsString(expt));
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
	    id = createManagedSource(url, it->_dir, false, "", it->_name);
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
	id = createManagedSource(url, pn, false, "", "");
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

  if ( pd->value().empty() ) {
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
	    std::vector<zypp::RepoInfo>::size_type id = createManagedSource(url, it->_dir, base, type, it->_name);
	    YRepo_Ptr repo = logFindRepository(id);

	    LoadResolvablesFrom(repo->repoInfo());

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

    try
    {
	ret = createManagedSource(url, pn, base, type, "");

	YRepo_Ptr repo = logFindRepository(ret);
	repo->repoInfo().setEnabled(true);

	LoadResolvablesFrom(repo->repoInfo());
    }
    catch ( const zypp::Exception& excpt)
    {
	y2error("SourceCreate for '%s' product '%s' has failed"
	    , url.asString().c_str(), pn.asString().c_str());
	_last_error.setLastError(ExceptionAsString(excpt));
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

	// add/remove resolvables
	if (enable)
	{
	    // load resolvables only when they are missing
	    if (!AnyResolvableFrom(repo->repoInfo().alias()))
	    {
		LoadResolvablesFrom(repo->repoInfo());
	    }
	}
	else
	{
	    // the source has been disables, remove resolvables from the pool
	    RemoveResolvablesFrom(repo->repoInfo().alias());
	}

	PkgFreshen();
    }
    catch (const zypp::Exception& excpt)
    {
	std::string alias = repo->repoInfo().alias();
	y2error ("Error for '%s': %s", alias.c_str(), excpt.asString().c_str());
	_last_error.setLastError(alias + ": " + ExceptionAsString(excpt));
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
	y2milestone("Refreshing metadata '%s'", repo->repoInfo().alias().c_str());
	RefreshWithCallbacks(repo->repoInfo());

	y2milestone("Caching source '%s'...", repo->repoInfo().alias().c_str());
	repomanager.buildCache(repo->repoInfo());
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
	// the resolvables cannot be used anymore, remove them
	RemoveResolvablesFrom(repo->repoInfo().alias());

	// update 'repos'
	repo->setDeleted();

	PkgFreshen();
    }
    catch (const zypp::Exception& excpt)
    {
	std::string alias = repo->repoInfo().alias();
	y2error ("Error for '%s': %s", alias.c_str(), excpt.asString().c_str());
	_last_error.setLastError(alias + ": " + ExceptionAsString(excpt));
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
	    src_map->add(YCPString("name"), YCPString((*it)->repoInfo().name()));

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

    if( !descr->value(YCPString("name")).isNull() && descr->value(YCPString("name"))->isString())
    {
	// rename the source
        y2debug("set name: %s", descr->value(YCPString("name"))->asString()->value().c_str());
	repo->repoInfo().setName(descr->value(YCPString("name"))->asString()->value());
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
    try
    {
	y2milestone("Moving download area of all sources to %s", path->value().c_str());
	zypp::media::MediaManager manager;
	manager.setAttachPrefix(path->value());
    }
    catch (zypp::Exception & excpt)
    {
	_last_error.setLastError(ExceptionAsString(excpt));
	y2error("Pkg::SourceMoveDownloadArea has failed: %s", excpt.msg().c_str());
	return YCPBoolean(false);
    }

    y2milestone( "Download areas moved");

    return YCPBoolean(true);
}

/*
 * A helper function - remove all resolvables from the repository from the pool
 */
void PkgModuleFunctions::RemoveResolvablesFrom(const std::string &alias)
{
    // remove the resolvables if they have been loaded
    // FIXME: can be implemented better? we need a ResStore object for removing,
    // which means search for a resolvable in the pool and get the Repository
    // object which can be asked for all resolvables in it
    for (zypp::ResPool::const_iterator it = zypp_ptr()->pool().begin()
	; it != zypp_ptr()->pool().end()
	; ++it)
    {
	if (it->resolvable()->repository().info().alias() == alias)
	{
	    y2milestone("Removing all resolvables from '%s' from the pool...", alias.c_str());
	    zypp_ptr()->removeResolvables(it->resolvable()->repository().resolvables());
	    break;
	}
    }
}

/*
 * A helper function - is there any resolvable from the repository in the pool?
 */
bool PkgModuleFunctions::AnyResolvableFrom(const std::string &alias)
{
    for (zypp::ResPool::const_iterator it = zypp_ptr()->pool().begin()
	; it != zypp_ptr()->pool().end()
	; ++it)
    {
	if (it->resolvable()->repository().info().alias() == alias)
	{
	    return true;
	}
    }

    return false;
}

/*
 * A helper function - load resolvable from the repository into the pool
 * Warning: Use AnyResolvableFrom() method for checing if the resolvables might be already loaded
 */
bool PkgModuleFunctions::LoadResolvablesFrom(const zypp::RepoInfo &repoinfo)
{
    bool success = true;
    unsigned int size_start = zypp_ptr()->pool().size();
    y2milestone("Loading resolvables from '%s', pool size at start: %d", repoinfo.alias().c_str(), size_start);

    try 
    {
	zypp::RepoManager repomanager = CreateRepoManager();

	// build cache if needed
	if (!repomanager.isCached(repoinfo))
	{
	    zypp::RepoStatus raw_metadata_status = repomanager.metadataStatus(repoinfo);
	    if (raw_metadata_status.empty())
	    {
		y2milestone("Missing metadata for source '%s', downloading...", repoinfo.alias().c_str());
		RefreshWithCallbacks(repoinfo);
	    }

	    y2milestone("Caching source '%s'...", repoinfo.alias().c_str());
	    repomanager.buildCache(repoinfo);
	}

	zypp::Repository repository = repomanager.createFromCache(repoinfo);
	const zypp::ResStore &store = repository.resolvables();

	// load resolvables
	zypp_ptr()->addResolvables(store);

	y2milestone("Loaded %d resolvables", store.size());
    }
    catch(const zypp::repo::RepoNotCachedException &excpt )
    {
	std::string alias = repoinfo.alias();
	y2error ("Resolvables from '%s' havn't been loaded: %s", alias.c_str(), excpt.asString().c_str());
	_last_error.setLastError("'" + alias + "': " + ExceptionAsString(excpt));
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

    unsigned int size_end = zypp_ptr()->pool().size();
    y2milestone("Pool size at end: %d (loaded %d resolvables)", size_end, size_end - size_start);
    return success;
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

// convert libzypp type to yast strings ("YaST", "YUM" or "Plaindir")
std::string PkgModuleFunctions::zypp2yastType(const std::string &type)
{
    std::string ret(type);

    if (type_conversion_table.empty())
    {
      // initialize the conversion map
      type_conversion_table["rpm-md"] = "YUM";
      type_conversion_table["yast2"] = "YaST";
      type_conversion_table["plaindir"] = "Plaindir";
      type_conversion_table["NONE"] = "NONE";
    }

    std::map<std::string,std::string>::const_iterator it = type_conversion_table.find(type);

    // found in the conversion table
    if (it != type_conversion_table.end())
    {
	ret = it->second;
    }
    else
    {
	y2error("Cannot convert type '%s'", type.c_str());
    }

    return ret;
}

std::string PkgModuleFunctions::yast2zyppType(const std::string &type)
{
    // do conversion from the Yast type ("YaST", "YUM", "Plaindir")
    // to libzypp type ("yast", "yum", "plaindir")
    // we can simply use toLower instead of a conversion table
    // in this case
    return zypp::str::toLower(type);
}


