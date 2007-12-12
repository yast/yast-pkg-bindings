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
   Summary:     YRepo class is a Yast representaion of a repository
*/

#include <YRepo.h>

#define y2log_component "Pkg"
#include <ycp/y2log.h>

IMPL_PTR_TYPE(YRepo);

YRepo::YRepo(zypp::RepoInfo & repo)
    : _repo(repo), _deleted(false)
{}

YRepo::~YRepo()
{
    if (_maccess)
    {
        try { _maccess->release(); }
        catch (const zypp::media::MediaException & ex)
	{
	    y2error("Error in ~Yrepo(): %s", ex.asString().c_str());
	}
    }
}

zypp::MediaSetAccess_Ptr & YRepo::mediaAccess()
{
    if (!_maccess)
    {
        y2milestone("Creating new MediaSetAccess for url %s",
            (*_repo.baseUrlsBegin()).asString().c_str());
        _maccess = new zypp::MediaSetAccess(*_repo.baseUrlsBegin()); // FIXME handle multiple baseUrls
    }

    return _maccess;
}

const YRepo YRepo::NOREPO;

