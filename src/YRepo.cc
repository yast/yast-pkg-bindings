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

static int yrepo_no = 0;

YRepo::YRepo(zypp::RepoInfo & repo)
    : _repo(repo), _deleted(false), _loaded(false)
{
    _url = _repo.url().asString();
    _repo_no = ++yrepo_no;

    if ( _url.empty() )
        _url = "<empty>";

    y2milestone("Creating YRepo #%d for %s", _repo_no, _url.c_str());
}

YRepo::YRepo()
{
    _url = "<default CTOR>";
    _repo_no = ++yrepo_no;

    y2milestone("YRepo #%d default CTOR ", _repo_no);
}


YRepo::YRepo(const YRepo &other)
{
    _repo_no = ++yrepo_no;
    _url = other.debugUrl();

    y2milestone("Copying YRepo #%d from #%d for %s", _repo_no, other.debugNo(), _url.c_str());
}


YRepo::~YRepo()
{
    y2milestone("Deleting YRepo #%d for %s", _repo_no, _url.c_str());

    if (_maccess)
    {
        y2milestone("Releasing MediaAccess for YRepo #%d for %s", _repo_no, _url.c_str());

        try
        {
            y2milestone("Before _maccess->release()");
            _maccess->release();
            y2milestone("After_maccess->release()");
        }
        catch (const zypp::media::MediaException & ex)
	{
	    y2error("Error in ~Yrepo(): %s", ex.asString().c_str());
	}
    }

    y2milestone("DONE ~YRepo #%d for %s", _repo_no, _url.c_str());
}

zypp::MediaSetAccess_Ptr & YRepo::mediaAccess()
{
    if (!_maccess)
    {
        y2milestone("Creating new MediaSetAccess for url %s",
            _repo.url().asString().c_str());
        _maccess = new zypp::MediaSetAccess(_repo.name(), _repo.url()); // FIXME handle multiple baseUrls
    }

    return _maccess;
}

const YRepo YRepo::NOREPO;

