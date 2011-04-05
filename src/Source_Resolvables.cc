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
   Summary:     Functions for adding/removing resolvables in the pool
   Namespace:   Pkg
*/

#include <Callbacks.h>
#include <Callbacks.YCP.h>

#include <PkgFunctions.h>
#include "log.h"

#include <zypp/sat/Pool.h>

/*
  Textdomain "pkg-bindings"
*/

/*
 * A helper function - remove all resolvables from the repository from the pool
 */
void PkgFunctions::RemoveResolvablesFrom(YRepo_Ptr repo)
{
    const std::string &alias = repo->repoInfo().alias();
    y2milestone("Removing resolvables from '%s'", alias.c_str());
    // remove the resolvables if they have been loaded
    zypp::sat::Pool::instance().reposErase(alias);

    repo->resetLoaded();
}

/*
 * A helper function - load resolvable from the repository into the pool
 */
bool PkgFunctions::LoadResolvablesFrom(YRepo_Ptr repo, const zypp::ProgressData::ReceiverFnc &progressrcv, bool network_check)
{
    if (repo->isLoaded())
    {
	y2milestone("Repository is already loaded");
	return true;
    }

    const zypp::RepoInfo &repoinfo = repo->repoInfo();
    bool success = true;
    unsigned int size_start = zypp_ptr()->pool().size();
    y2milestone("Loading resolvables from '%s', pool size at start: %d", repoinfo.alias().c_str(), size_start);

    // sub tasks
    zypp::ProgressData prog(100);
    prog.sendTo(progressrcv);
    zypp::CombinedProgressData load_subprogress(prog, 100);

    try 
    {
	zypp::RepoManager repomanager = CreateRepoManager();
	bool refresh = true;

	// build cache if needed
	if (!repomanager.isCached(repoinfo) && !autorefresh_skipped)
	{
	    zypp::RepoStatus raw_metadata_status = repomanager.metadataStatus(repoinfo);
	    if (raw_metadata_status.empty())
	    {
		if (network_check)
		{
		    zypp::Url url = *(repoinfo.baseUrlsBegin());

		    if (remoteRepo(url) && !NetworkDetected())
		    {
			y2warning("No network connection, skipping autorefresh of remote repository %s (%s)",
			    repoinfo.alias().c_str(), url.asString().c_str());

			refresh = false;
		    }
		}

		if (refresh)
		{
		    y2milestone("Missing metadata for source '%s', downloading...", repoinfo.alias().c_str());

		    CallRefreshStarted();

		    RefreshWithCallbacks(repoinfo);

		    CallRefreshDone();
		}
	    }

	    if (refresh)
	    {
		y2milestone("Caching source '%s'...", repoinfo.alias().c_str());
		repomanager.buildCache(repoinfo, zypp::RepoManager::BuildIfNeeded, load_subprogress);
	    }
	}

	repomanager.loadFromCache(repoinfo);
	repo->setLoaded();
	//y2milestone("Loaded %zd resolvables", store.size());
    }
    catch(const zypp::repo::RepoNotCachedException &excpt )
    {
	if (!autorefresh_skipped)
	{
	    std::string alias = repoinfo.alias();
	    zypp::Url url(repoinfo.url());
	    y2error ("Resolvables from '%s' havn't been loaded: %s", alias.c_str(), excpt.asString().c_str());
	    _last_error.setLastError("'" + alias + "': " + url.asString() + "\n" + ExceptionAsString(excpt));
	    success = false;
	}
	else
	{
	    y2internal("Autorefresh disabled, the cache is missing -> cannot load resolvables");
	}

	// FIXME ??
	/*
	// disable the source
	y2error("Disabling source %s", url.c_str());
	repo->disable();
	*/
    }
    catch (const zypp::Exception& excpt)
    {
	std::string alias = repoinfo.alias();
	y2internal("Error: Loading resolvables failed: %s", ExceptionAsString(excpt).c_str());
	_last_error.setLastError("'" + alias + "': " + ExceptionAsString(excpt));
	success = false;
    }

    unsigned int size_end = zypp_ptr()->pool().size();
    y2milestone("Pool size at end: %d (loaded %d resolvables)", size_end, size_end - size_start);

    prog.toMax();

    return success;
}
