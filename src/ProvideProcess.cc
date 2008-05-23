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
   Summary:     Search the best available resolvable
   Namespace:   Pkg
*/


#include "ProvideProcess.h"
#include "log.h"

ProvideProcess::ProvideProcess(const zypp::Arch &arch, const std::string &vers, const bool oNeeded, const std::string &from_repo)
    : _architecture(arch), whoWantsIt(zypp::ResStatus::APPL_HIGH), version(vers), onlyNeeded(oNeeded), repo_alias(from_repo)
{ }

bool ProvideProcess::operator()( zypp::PoolItem provider )
{
    // 1. compatible arch
    // 2. best arch
    // 3. best edition
    //  see QueueItemRequire in zypp/solver/detail, RequireProcess

    // check the version if it's specified
    if (!version.empty() && version != provider->edition().asString())
    {
	y2milestone("Skipping version %s (requested: %s)", provider->edition().asString().c_str(), version.c_str());
	return true;
    }

    // a specific repository is requested
    if (!repo_alias.empty() && provider.resolvable()->repoInfo().alias() != repo_alias)
    {
	return true;
    }

    
    if (( (!provider.isSatisfied()) && provider.isRelevant() )
        )
        //&& (!onlyNeeded) ) // take only needed items (e.G. needed patches)
    {
	// deselect the item if it's already selected,
	// only one item should be selected
	if (provider.status().isToBeInstalled())
	{
	    provider.status().resetTransact(whoWantsIt);
	}

	// regarding items which are installable only
	if (!provider->arch().compatibleWith( _architecture )) {
	    y2milestone ("provider %s has incompatible arch '%s'", provider->name().c_str(), provider->arch().asString().c_str());
	}
	else if (!item) {						// no provider yet
	    item = provider;
	}
	else if (item->arch().compare( provider->arch() ) < 0) {	// provider has better arch
	    item = provider;
	}
	else if (item->edition().compare( provider->edition() ) < 0) {
	    item = provider;						// provider has better edition
	}
    }

    return true;
}

