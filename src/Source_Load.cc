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

#include <PkgModuleFunctions.h>

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
PkgModuleFunctions::SourceRestore()
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


/****************************************************************************************
 * @builtin SourceLoad
 *
 * @short Load resolvables from the installation sources
 * @description
 * Refresh the pool - Add/remove resolvables from the enabled/disabled sources.
 * @return boolean True on success
 **/
YCPValue
PkgModuleFunctions::SourceLoadImpl(PkgProgress &progress)
{
    bool success = true;

    int repos_to_load = 0;
    for (RepoCont::iterator it = repos.begin();
       it != repos.end(); ++it)
    {
	if ((*it)->repoInfo().enabled() && !(*it)->isDeleted())
	{
	    repos_to_load++;
	}
    }

    y2debug("repos_to_load: %d", repos_to_load);

    // set max. value (3 steps per repository - refresh, rebuild, load)
    zypp::ProgressData prog_total(repos_to_load * 3 * 100);
    prog_total.sendTo(progress.Receiver());
    y2debug("Progress status: %lld", prog_total.val());

    zypp::RepoManager repomanager = CreateRepoManager();

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
		// autorefresh the source
		if ((*it)->repoInfo().autorefresh())
		{
		    try
		    {
			y2milestone("Autorefreshing source: %s", (*it)->repoInfo().alias().c_str());
			// refresh the repository
			RefreshWithCallbacks((*it)->repoInfo(), prog.receiver());
		    }
		    // NOTE: subtask progresses are reported as done in the descructor
		    // no need to handle them in the exception code
		    catch (const zypp::Exception& excpt)
		    {
			y2error ("Error in SourceLoad: %s", excpt.asString().c_str());
			_last_error.setLastError(ExceptionAsString(excpt));
			success = false;
		    }
		}
	    }
	    prog.toMax();
	    y2debug("Progress status: %lld", prog_total.val());
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
		y2milestone("Resolvables from '%s' are already present, not loading", (*it)->repoInfo().alias().c_str());
	    }
	    else
	    {
		// autorefresh the source
		if ((*it)->repoInfo().autorefresh())
		{
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
			y2error ("Error in SourceLoad: %s", excpt.asString().c_str());
			_last_error.setLastError(ExceptionAsString(excpt));
			success = false;
		    }
		}
	    }
	    prog.toMax();
	    y2debug("Progress status: %lld", prog_total.val());
	}
    }

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
		// load objects
		success = LoadResolvablesFrom((*it)->repoInfo(), load_subprogress) && success;
	    }
	}
	y2debug("Progress status: %lld", prog_total.val());
    }

    // report 100%
    prog_total.toMax();

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
    PkgProgress pkgprogress(_callbackHandler);

    // display the progress only when 'enable' is true
    if (enable->value())
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
PkgModuleFunctions::SourceStartManagerImpl(const YCPBoolean& enable, PkgProgress &progress)
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
