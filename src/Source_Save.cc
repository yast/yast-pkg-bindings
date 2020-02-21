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
   Summary:     Functions for saving repository configuration
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

/**
 * @builtin SourceReleaseAll
 *
 * @short Release all medias hold by all sources
 * Warning: It also deletes the downloaded files!
 * @return boolean
 **/
YCPValue
PkgFunctions::SourceReleaseAll ()
{
    y2milestone("Releasing all sources...");
    bool ret = true;

    y2milestone("Removing all tmp directories");
    tmp_dirs.clear();

    for (RepoCont::iterator it = repos.begin();
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

/**
 * @builtin SourceSaveAll
 *
 * @short Save all InstSrces.
 * @return boolean
 **/
YCPValue
PkgFunctions::SourceSaveAll ()
{
    y2milestone("Saving the source setup...");
    bool ret = true;

    // nothing to save, return success
    if (repos.empty() && service_manager.empty())
    {
	y2debug("No repository or service defined, saving skipped");
	return YCPBoolean(ret);
    }

    zypp::RepoManager* repomanager = CreateRepoManager();

    // save the services
    try
    {
	service_manager.SaveServices(*repomanager);
	y2milestone("All services have been saved");
    }
    catch (const zypp::Exception& excpt)
    {
	_last_error.setLastError(ExceptionAsString(excpt));
	ret = false;
    }

    // count removed repos
    int removed_repos = 0;
    for (RepoCont::iterator it = repos.begin();
	it != repos.end(); ++it)
    {
	// the repo has been removed
	if ((*it)->isDeleted())
	{
	    removed_repos++;
	}
    }

    y2debug("Found %d removed repositories", removed_repos);

    // number of steps:
    //   for removed repository: 3 (remove metadata, remove from cache, remove .repo file)
    //   for other repositories: 1 (just save/update .repo file)
    //   (so there are 2 extra steps for removed repos)
    int save_steps = 2*removed_repos + repos.size();

    PkgProgress pkgprogress(_callbackHandler);
    std::list<std::string> stages;

    if (removed_repos > 0)
    {
	stages.push_back(_("Remove Repositories"));
    }


    // stages: "download", "build cache"
    stages.push_back(_("Save Repositories"));

    // set number of step
    zypp::ProgressData prog_total(save_steps);
    // set the receiver
    prog_total.sendTo(pkgprogress.Receiver());

    // start the process
    pkgprogress.Start(_("Saving Repositories..."), stages, _(HelpTexts::save_help));

    // remove deleted repos (the old configurations) at first
    for (RepoCont::iterator it = repos.begin();
	it != repos.end(); ++it)
    {
	// the repo has been removed
	if ((*it)->isDeleted())
	{
	    std::string repo_alias = (*it)->repoInfo().alias();

	    try
	    {
		// remove the metadata
		zypp::RepoStatus raw_metadata_status = repomanager->metadataStatus((*it)->repoInfo());
		if (!raw_metadata_status.empty())
		{
		    y2milestone("Removing metadata for source '%s'...", repo_alias.c_str());
		    repomanager->cleanMetadata((*it)->repoInfo());
		}
		prog_total.incr();

		// remove the cache
		if (repomanager->isCached((*it)->repoInfo()))
		{
		    y2milestone("Removing cache for '%s'...", repo_alias.c_str());
		    repomanager->cleanCache((*it)->repoInfo());
		}
		prog_total.incr();

		// does the repository exist?
		repomanager->getRepositoryInfo(repo_alias);
		y2milestone("Removing repository '%s'", repo_alias.c_str());
		repomanager->removeRepository((*it)->repoInfo());
		prog_total.incr();
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

    if (removed_repos > 0)
    {
	pkgprogress.NextStage();
    }

    // save all repos (the current configuration)
    for (RepoCont::iterator it = repos.begin();
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
		    repomanager->getRepositoryInfo(current_alias);
		    y2milestone("Modifying repository '%s'", current_alias.c_str());
		    repomanager->modifyRepository(current_alias, (*it)->repoInfo());
		}
		catch (const zypp::repo::RepoNotFoundException &ex)
		{
		    // the repository was not found, add it
		    y2milestone("Adding repository '%s'", current_alias.c_str());
		    repomanager->addRepository((*it)->repoInfo());
		}
	    }
	    catch (zypp::Exception & excpt)
	    {
		y2error("Pkg::SourceSaveAll has failed: %s", excpt.msg().c_str() );
		_last_error.setLastError(ExceptionAsString(excpt));
		return YCPBoolean(false);
	    }

	    prog_total.incr();
	}
    }

    y2milestone("All sources have been saved");

    return YCPBoolean(ret);
}

/**
 * @builtin SourceFinishAll
 *
 * @short Release all instalation sources
 * @description
 * Release all known installation repositories. Releasing is done automaticaly in Pkg::
 * destructor, but can be done explicitly to force reloading of registered repositories.
 * Upgrade repositories are automatically removed from the solver.
 * Use SourceSaveAll() to not loose the new registered sources before calling SourceFinishAll()!
 * @return boolean true on success
 **/
YCPValue
PkgFunctions::SourceFinishAll ()
{
    try
    {
	y2milestone( "Unregistering all sources...") ;

    	// remove all resolvables
	for (RepoCont::iterator it = repos.begin();
	    it != repos.end(); ++it)
	{
	    RemoveResolvablesFrom(*it);
	}

	// remove all upgrading repositories from the solver before destructing them
	for_(it, zypp::ResPool::instance().knownRepositoriesBegin(), zypp::ResPool::instance().knownRepositoriesEnd())
	{
	    if (zypp_ptr()->resolver()->upgradingRepo(*it))
	    {
		y2milestone("Removing upgrade repository '%s' (%s)", it->name().c_str(), it->alias().c_str());
		zypp_ptr()->resolver()->removeUpgradeRepo(*it);
	    }
	}

	// release all repositories
	repos.clear();

	// release all services
	service_manager.Reset();

	if (repo_manager)
	{
		y2milestone("Releasing the repo manager...");
		delete repo_manager;
		repo_manager = nullptr;
	}
    }
    catch (zypp::Exception & excpt)
    {
	y2error("Pkg::SourceFinishAll has failed: %s", excpt.msg().c_str() );
	_last_error.setLastError(ExceptionAsString(excpt));
	return YCPBoolean(false);
    }

    y2milestone("All sources and services have been unregistered");
    // reset the source loaded flag
    _source_loaded = false;

    return YCPBoolean(true);
}

