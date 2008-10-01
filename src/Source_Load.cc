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
   Summary:     Functions for initializing the package manager
   Namespace:   Pkg
*/

#include <Callbacks.h>
#include <Callbacks.YCP.h>

#include <PkgFunctions.h>
#include "log.h"

#include <PkgProgress.h>
#include <HelpTexts.h>

/*
  Textdomain "pkg-bindings"
*/

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
PkgFunctions::SourceRestore()
{

    if (repos.size() > 0)
    {
	y2warning("Number of registered repositories: %zd, skipping repository load!", repos.size());
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
YCPValue PkgFunctions::SourceGetBrokenSources()
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
PkgFunctions::SourceLoad()
{
    std::list<std::string> stages;
    stages.push_back(_("Refresh Sources"));
    stages.push_back(_("Rebuild Cache"));
    stages.push_back(_("Load Data"));

    PkgProgress pkgprogress(_callbackHandler);

    // 3 steps per repository (download, cache rebuild, load resolvables)
    pkgprogress.Start(_("Loading the Package Manager..."), stages, HelpTexts::load_resolvables);

    YCPValue ret = SourceLoadImpl(pkgprogress);

    pkgprogress.Done();

    return ret;
}

void PkgFunctions::CallRefreshStarted()
{
    // get the YCP callback handler for destroy event
    Y2Function* ycp_handler = _callbackHandler._ycpCallbacks.createCallback(CallbackHandler::YCPCallbacks::CB_StartSourceRefresh);

    // is the callback registered?
    if (ycp_handler != NULL)
    {
	// evaluate the callback function
	ycp_handler->evaluateCall();
    }
}

void PkgFunctions::CallRefreshDone()
{
    // get the YCP callback handler for destroy event
    Y2Function* ycp_handler = _callbackHandler._ycpCallbacks.createCallback(CallbackHandler::YCPCallbacks::CB_DoneSourceRefresh);

    // is the callback registered?
    if (ycp_handler != NULL)
    {
	// evaluate the callback function
	ycp_handler->evaluateCall();
    }
}

YCPValue PkgFunctions::SkipRefresh()
{
    autorefresh_skipped = true;
    return YCPVoid();
}

YCPValue
PkgFunctions::SourceLoadImpl(PkgProgress &progress)
{
    bool success = true;

    int repos_to_load = 0;
    int repos_to_refresh = 0;
    for (RepoCont::iterator it = repos.begin();
       it != repos.end(); ++it)
    {
	if ((*it)->repoInfo().enabled() && !(*it)->isDeleted())
	{
	    repos_to_load++;

	    if ((*it)->repoInfo().autorefresh())
	    {
		repos_to_refresh++;
	    }
	}
    }

    y2debug("repos_to_load: %d, repos_to_refresh: %d", repos_to_load, repos_to_refresh);

    // set max. value (3 steps per repository - refresh, rebuild, load)
    zypp::ProgressData prog_total(repos_to_load * 3 * 100);
    prog_total.sendTo(progress.Receiver());
    y2debug("Progress status: %lld", prog_total.val());

    zypp::RepoManager repomanager = CreateRepoManager();

    autorefresh_skipped = false;
    CallRefreshStarted();

    bool network_is_running = NetworkDetected();

    if (repos_to_refresh > 0)
    {
	// refresh metadata
	for (RepoCont::iterator it = repos.begin();
	   it != repos.end(); ++it)
	{
	    // load resolvables only from enabled repos which are not deleted
	    if ((*it)->repoInfo().enabled() && !(*it)->isDeleted())
	    {
		// sub tasks
		zypp::CombinedProgressData refresh_subprogress(prog_total, 100);
		zypp::ProgressData prog(100);
		prog.sendTo(refresh_subprogress);
		y2debug("Progress status: %lld", prog_total.val());

		if (AnyResolvableFrom((*it)->repoInfo().alias()))
		{
		    y2milestone("Resolvables from '%s' are already present, not loading", (*it)->repoInfo().alias().c_str());
		}
		else
		{
		    zypp::RepoStatus raw_metadata_status = repomanager.metadataStatus((*it)->repoInfo());

		    // autorefresh the source
		    if ((*it)->repoInfo().autorefresh() || raw_metadata_status.empty())
		    {
			// do not autorefresh remote repositories when the network is not running
			if (!network_is_running)
			{
			    zypp::Url url = *((*it)->repoInfo().baseUrlsBegin());
			    
			    if (remoteRepo(url))
			    {
				y2warning("No network connection, skipping autorefresh of remote repository %s (%s)",
				    (*it)->repoInfo().alias().c_str(), url.asString().c_str());
				continue;
			    }
			}

			try
			{
			    y2milestone("Autorefreshing source: %s", (*it)->repoInfo().alias().c_str());
			    // refresh the repository
			    RefreshWithCallbacks((*it)->repoInfo(), prog.receiver());
			}
			// NOTE: subtask progresses are reported as done in the destructor
			// no need to handle them in the exception code
			catch (const zypp::Exception& excpt)
			{
			    if (autorefresh_skipped)
			    {
				y2warning("autorefresh_skipped, ignoring the exception");
			    }
			    else
			    {
				y2error ("Error in SourceLoad: %s", excpt.asString().c_str());
				_last_error.setLastError(ExceptionAsString(excpt));
				success = false;
			    }
			}

			if (autorefresh_skipped)
			{
			    y2warning("Skipping autorefresh for the rest of repositories");
			    break;
			}
			else
			{
			    y2debug("Continuing with autorefresh");
			}
		    }
		}
		prog.toMax();
		y2debug("Progress status: %lld", prog_total.val());
	    }
	}
    }

    progress.NextStage();

    // rebuild cache
    for (RepoCont::iterator it = repos.begin();
       it != repos.end(); ++it)
    {
	// load resolvables only from enabled repos which are not deleted
	if ((*it)->repoInfo().enabled() && !(*it)->isDeleted())
	{
	    // sub tasks
	    zypp::CombinedProgressData rebuild_subprogress(prog_total, 100);
	    zypp::ProgressData prog(100);
	    prog.sendTo(rebuild_subprogress);

	    y2debug("Progress status: %lld", prog_total.val());
	    if (AnyResolvableFrom((*it)->repoInfo().alias()))
	    {
		y2milestone("Resolvables from '%s' are already present, not rebuilding the cache", (*it)->repoInfo().alias().c_str());
	    }
	    else
	    {
		// autorefresh the source
		if ((*it)->repoInfo().autorefresh())
		{
		    zypp::RepoStatus raw_metadata_status = repomanager.metadataStatus((*it)->repoInfo());

		    // autorefresh the source
		    if (raw_metadata_status.empty() )
		    {
			y2error("Missinga metadata, not rebuilding the cache");
			continue;
		    }

		    try
		    {
			// rebuild cache (the default policy is "if needed")
			y2milestone("Rebuilding cache for '%s'...", (*it)->repoInfo().alias().c_str());

			//repomanager.buildCache((*it)->repoInfo(), zypp::RepoManager::BuildIfNeeded, prog.receiver());
			repomanager.buildCache((*it)->repoInfo(), zypp::RepoManager::BuildIfNeeded, rebuild_subprogress);
		    }
		    // NOTE: subtask progresses are reported as done in the descructor
		    // no need to handle them in the exception code
		    catch (const zypp::Exception& excpt)
		    {
			if (autorefresh_skipped)
			{
			    y2warning("autorefresh_skipped, ignoring the exception");
			}
			else
			{
			    y2error ("Error in SourceLoad: %s", excpt.asString().c_str());
			    _last_error.setLastError(ExceptionAsString(excpt));
			    success = false;
			}
		    }

		    if (autorefresh_skipped)
		    {
			y2warning("Skipping autorefresh for the rest of repositories");
			break;
		    }
		    else
		    {
			y2debug("Continuing with autorefresh");
		    }
		}
	    }
	    prog.toMax();
	    y2debug("Progress status: %lld", prog_total.val());
	}
    }

    CallRefreshDone();

    progress.NextStage();

    for (RepoCont::iterator it = repos.begin();
       it != repos.end(); ++it)
    {
	// load resolvables only from enabled repos which are not deleted
	if ((*it)->repoInfo().enabled() && !(*it)->isDeleted())
	{
	    y2debug("Loading: %s, Status: %lld", (*it)->repoInfo().alias().c_str(), prog_total.val());

	    // subtask
	    zypp::CombinedProgressData load_subprogress(prog_total, 100);

	    if (AnyResolvableFrom((*it)->repoInfo().alias()))
	    {
		y2milestone("Resolvables from '%s' are already present, not loading", (*it)->repoInfo().alias().c_str());
	    }
	    else
	    {
		// load objects, do network status check
		success = LoadResolvablesFrom((*it)->repoInfo(), load_subprogress, true) && success;
	    }
	}
	y2debug("Progress status: %lld", prog_total.val());
    }

    // report 100%
    prog_total.toMax();

    autorefresh_skipped = false;
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
PkgFunctions::SourceStartManager (const YCPBoolean& enable)
{
    PkgProgress pkgprogress(_callbackHandler);

    // display the progress only when sources will be loaded
    if (enable->value() && repos.size() == 0)
    {
	std::list<std::string> stages;
	stages.push_back(_("Load Sources"));
	stages.push_back(_("Refresh Sources"));
	stages.push_back(_("Rebuild Cache"));
	stages.push_back(_("Load Data"));
    
	// 3 steps per repository (download, cache rebuild, load resolvables)
	pkgprogress.Start(_("Loading the Package Manager..."), stages, HelpTexts::load_resolvables);
    }

    YCPValue ret = SourceStartManagerImpl(enable, pkgprogress);

    if (enable->value())
    {
	pkgprogress.Done();
    }

    return ret;
}

/****************************************************************************************
 * Helper function
 * @short Start the source manager - restore the sources and load the resolvables
 * @description
 * Calls SourceRestore(), if argument enable is true SourceLoad() is called.
 * @param boolean enable If true the resolvables are loaded from the enabled sources
 *
 * @return boolean
 **/
YCPValue
PkgFunctions::SourceStartManagerImpl(const YCPBoolean& enable, PkgProgress &progress)
{
    YCPValue success = SourceRestore();

    progress.NextStage();

    if( enable->value() )
    {
	if (!success->asBoolean()->value())
	{
	    y2warning("SourceStartManager: Some sources have not been restored, loading only the active sources...");
	}

	// enable all sources and load the resolvables
	success = YCPBoolean(SourceLoadImpl(progress)->asBoolean()->value() && success->asBoolean()->value());
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
PkgFunctions::SourceStartCache (const YCPBoolean& enabled)
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
 * @builtin SourceCleanupBroken - obsoleted, do not use!
 *
 * @short Clean up all sources that were not properly restored on the last
 * call of SourceStartManager or SourceStartCache.
 *
 * @return boolean
 **/
YCPValue
PkgFunctions::SourceCleanupBroken ()
{
    y2warning("Pkg::SourceCleanupBroken() is obsoleted, it's not needed anymore.");
    return YCPBoolean(true);
}
