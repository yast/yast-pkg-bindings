
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

#ifndef YRepo_h
#define YRepo_h

#include <zypp/RepoInfo.h>
#include <zypp/MediaSetAccess.h>
#include <zypp/base/ReferenceCounted.h>

DEFINE_PTR_TYPE(YRepo);
class YRepo : public zypp::base::ReferenceCounted
{
private:
    zypp::RepoInfo _repo;
    zypp::MediaSetAccess_Ptr _maccess;
    bool _deleted;
    bool _added;

    YRepo() {}

public:
    YRepo(zypp::RepoInfo & repo);
    ~YRepo();

    const zypp::RepoInfo & repoInfo() const { return _repo; }
    zypp::RepoInfo & repoInfo() { return _repo; }
    zypp::MediaSetAccess_Ptr & mediaAccess();

    bool isDeleted() {return _deleted;}
    void setDeleted() {_deleted = true;}

    bool isAdded() {return _added;}
    void setAdded() {_added = true;}
    void resetAdded() {_added = false;}

public:
    static const YRepo NOREPO;
};

#endif
