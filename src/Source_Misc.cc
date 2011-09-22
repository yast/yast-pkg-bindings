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
   Summary:     Generic functions for accessing repositories from Yast
   Namespace:   Pkg
*/

#include <PkgFunctions.h>
#include "log.h"

#include <sstream> // ostringstream

/*
  Textdomain "pkg-bindings"
*/

/**
 * Logging helper:
 * call zypp::SourceManager::sourceManager()->findSource
 * and in case of exception, log error and setLastError AND RETHROW
 */
YRepo_Ptr PkgFunctions::logFindRepository(RepoId id)
{
    try
    {
	if (id < 0 || id >= (long long)repos.size())
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
	    y2error("Source %lld has been deleted, the ID is not valid", id);
	    return YRepo_Ptr();
	}

	return repos[id];
    }
    catch (...)
    {
	y2error("Cannot find source with ID: %lld", id);
	// TODO: improve the error message
	_last_error.setLastError(_("Cannot find source"));
    }

    // not found, return empty pointer
    return YRepo_Ptr();
}

PkgFunctions::RepoId PkgFunctions::logFindAlias(const std::string &alias) const
{
    RepoId index = 0LL;

    for(RepoCont::const_iterator it = repos.begin(); it != repos.end() ; ++it, ++index)
    {
	if (!(*it)->isDeleted() && (*it)->repoInfo().alias() == alias)
	    return index;
    }

    return -1LL;
}

bool PkgFunctions::aliasExists(const std::string &alias, const std::list<zypp::RepoInfo> &reps) const
{
    // search in loaded repositories
    for(RepoCont::const_iterator it = repos.begin(); it != repos.end() ; ++it)
    {
	if (!(*it)->isDeleted() && (*it)->repoInfo().alias() == alias)
	    return true;
    }

    // search in stored repositories
    for (std::list<zypp::RepoInfo>::const_iterator it = reps.begin();
	it != reps.end(); ++it)
    {
	if (it->alias() == alias)
	    return true;
    }

    return false;
}

// convert libzypp type to yast strings ("YaST", "YUM" or "Plaindir")
std::string PkgFunctions::zypp2yastType(const std::string &type)
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

std::string PkgFunctions::yast2zyppType(const std::string &type)
{
    // do conversion from the Yast type ("YaST", "YUM", "Plaindir")
    // to libzypp type ("yast", "yum", "plaindir")
    // we can simply use toLower instead of a conversion table
    // in this case
    return zypp::str::toLower(type);
}

std::string PkgFunctions::UniqueAlias(const std::string &alias)
{
    // make a copy
    std::string ret = alias;

    unsigned int id = 0;

    // search in stored repositories
    std::list<zypp::RepoInfo> reps = CreateRepoManager().knownRepositories();

    while(aliasExists(ret, reps))
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

// helper function
zypp::Url PkgFunctions::shortenUrl(const zypp::Url &url)
{
    std::string url_path = url.getPathName();
    std::string begin_path;
    std::string end_path;

    // try to convert 'http://server/dir1/dir2/dir3/dir4' -> 'http://server/dir1/.../dir4'
    std::string::size_type pos_first = url_path.find("/");
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

