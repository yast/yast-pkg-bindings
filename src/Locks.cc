/* ------------------------------------------------------------------------------
 * Copyright (c) 2008 Novell, Inc. All Rights Reserved.
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
   Namespace:   Pkg
   Summary:	Functions for accessing locks in the package manager
*/


#include "PkgFunctions.h"
#include "log.h"

#include <y2util/Y2SLog.h>

#include <ycp/YCPList.h>
#include <ycp/YCPMap.h>
#include <ycp/YCPString.h>
#include <ycp/YCPVoid.h>
#include <ycp/YCPBoolean.h>
#include <ycp/YCPInteger.h>

#include <zypp/Locks.h>

// SEARCH_* constants (SEARCH_STRING, SEARCH_SUBSTRING,...)
extern "C"
{
#include "satsolver/repo.h"
}


/**
 * @builtin AddLock
 * @short Addd a lock to the package manager
 * @description
 * Set the given locale as the output locale -- all messages from the package manager (errors, warnings,...)
 * will be returned in the selected language. This built-in does not change package selection in any way.
 * see http://en.opensuse.org/Libzypp/Locksfile for more information
 * @param string lock Definition of the lock
 * @return void
 */
YCPValue
PkgFunctions::AddLock(const YCPMap &lock)
{
    zypp::PoolQuery query;

    try
    {
	for_(map_it, lock.begin(), lock.end())
	{
	    YCPValue key(map_it.key());
	    YCPValue val(map_it.value());

	    if (key.isNull())
	    {
		y2warning("Warning: ignoring 'nil' key in lock map");
		continue;
	    }

	    if (val.isNull())
	    {
		y2warning("Warning: ignoring 'nil' value in lock map");
		continue;
	    }

	    if (key->isString())
	    {
		std::string key_str(key->asString()->value());

		// add kind
		if (key_str == "kind")
		{
		    if (val->isList())
		    {
			YCPList items(val->asList());

			int index = 0;
			int list_size = items.size();

			while(index < list_size)
			{
			    YCPValue list_item(items->value(index));

			    if (!list_item.isNull() && list_item->isString())
			    {
				query.addKind(zypp::ResKind(list_item->asString()->value())); 
			    }
			    else
			    {
				y2error("Invalid item at index %d in \"kind\" list", index);
				return YCPBoolean(false);
			    }

			    index++;
			}
		    }
		    else
		    {
			y2error("Error %s is not list", val->toString().c_str());
			return YCPBoolean(false);
		    }
		}
		else if (key_str == "install_status")
		{
		    if (val->isString())
		    {
			std::string status_str(val->asString()->value());

			if (status_str == "installed")
			{
			    query.setInstalledOnly();
			}
			else if (status_str == "uninstalled")
			{
			    query.setUninstalledOnly();
			}
			else if (status_str == "all")
			{
			    query.setStatusFilterFlags(zypp::PoolQuery::ALL);
			}
			else
			{
			    y2error("Unknown install_status status value: %s", status_str.c_str());
			    return YCPBoolean(false);
			}
		    }
		    else
		    {
			y2error("Type of key 'install_status' must be string, found: %s", val->toString().c_str());
			return YCPBoolean(false);
		    }
		}
		else if (key_str == "repo_id")
		{
		    if (val->isList())
		    {
			YCPList items(val->asList());

			int index = 0;
			int list_size = items.size();

			while(index < list_size)
			{
			    YCPValue list_item(items->value(index));

			    if (!list_item.isNull() && list_item->isInteger())
			    {
				RepoId repo_id = list_item->asInteger()->value();

				YRepo_Ptr repo_ptr = logFindRepository(repo_id);

				if (repo_ptr)
				{
				    query.addRepo(repo_ptr->repoInfo().alias());
				}
				else
				{
				    y2error("Repository %zd not found", repo_id);
				    return YCPBoolean(false);
				}
			    }
			    else
			    {
				y2error("Invalid item at index %d in \"repo_id\" list", index);
				return YCPBoolean(false);
			    }

			    index++;
			}
		    }
		    else
		    {
			y2error("Error: 'repo_id' value is not list: %s", val->toString().c_str());
			return YCPBoolean(false);
		    }
		}
		else if (key_str == "case_sensitive")
		{
		    if (val->isBoolean())
		    {
			bool cs = val->asBoolean()->value();
			query.setCaseSensitive(cs);
		    }
		    else
		    {
			y2error("Type of key 'case_sensitive' must be boolean, found: %s", val->toString().c_str());
			return YCPBoolean(false);
		    }
		}
		else if (key_str == "global_string")
		{
		    if (val->isList())
		    {
			YCPList items(val->asList());

			int index = 0;
			int list_size = items.size();

			while(index < list_size)
			{
			    YCPValue list_item(items->value(index));

			    if (!list_item.isNull() && list_item->isString())
			    {
				std::string global_str = list_item->asString()->value();
				query.addString(global_str);
			    }
			    else
			    {
				y2error("Invalid item at index %d in \"global_string\" list: %s, string expected", index, list_item->toString().c_str());
				return YCPBoolean(false);
			    }

			    index++;
			}
		    }
		    else
		    {
			y2error("Error: 'global_string' value is not list: %s", val->toString().c_str());
			return YCPBoolean(false);
		    }
		}
		else if (key_str == "string_type")
		{
		    if (val->isString())
		    {
			std::string str_type = val->asString()->value();

			INT << "string_type: " << str_type << std::endl;

			if (str_type == "exact")
			    query.setMatchExact();
			else if (str_type == "substring")
			    query.setMatchSubstring();
			else if (str_type == "glob")
			    query.setMatchGlob();
			else if (str_type == "regex")
			    query.setMatchRegex();
			else
			{
			    y2error("Unknown 'string_type' value: %s", val->toString().c_str());
			    return YCPBoolean(false);
			}
		    }
		    else
		    {
			y2error("Type of key 'string_type' must be string, found: %s", val->toString().c_str());
			return YCPBoolean(false);
		    }
		}
		// it is probably an attribute
		else if (std::string(key_str, 0, 9) == "solvable:")
		{
		    if (val->isList())
		    {
			YCPList items(val->asList());

			int index = 0;
			int list_size = items.size();

			y2internal("list_size %d", list_size);

			while(index < list_size)
			{
			    YCPValue list_item(items->value(index));

			    if (!list_item.isNull() && list_item->isString())
			    {
				std::string attr = list_item->asString()->value();

				query.addAttribute(zypp::sat::SolvAttr(key_str), attr);
			    }
			    else
			    {
				y2error("Invalid item at index %d at \"%s\" key: %s, string expected", index, key_str.c_str(), list_item->toString().c_str());
				return YCPBoolean(false);
			    }

			    index++;
			}
		    }
		    else
		    {
			y2error("Error: value '%s' in list at key '%s' is not a list", val->toString().c_str(), key_str.c_str());
			return YCPBoolean(false);
		    }
		}
	    }
	    else
	    {
		y2error("Key %s is not string", key->toString().c_str());
	    }
	}

	// and finally add the lock
	zypp::Locks &locks = zypp::Locks::instance();

	DBG << "Adding query: " << query << std::endl;

	locks.addLock(query);

//      TODO FIXME: huh, the config must be saved? (otherwise it doesn't work)
	zypp::Pathname lock_file(_target_root + zypp::ZConfig::instance().locksFile());
	locks.save(lock_file);
    }
    catch (zypp::Exception & excpt)
    {
	y2warning("Error while parsing lock map %s: %s", lock->toString().c_str(), excpt.asString().c_str());
	return YCPBoolean(false);
    }

    return YCPBoolean(true);
}


YCPMap PkgFunctions::PoolQuery2YCPMap(const zypp::PoolQuery &pool_query)
{
    YCPMap lock;

    zypp::PoolQuery::AttrRawStrMap attrs(pool_query.attributes());

    // add attributes - name, summary...
    if (!attrs.empty())
    {
	for_(attr_it, attrs.begin(), attrs.end())
	{
	    YCPList attr_list;

	    for_(attr_list_it, attr_it->second.begin(), attr_it->second.end())
	    {
		attr_list->add(YCPString(*attr_list_it));
	    }

	    lock->add(YCPString(attr_it->first.asString()), attr_list);
	}

    }
    
    // add "kind" attribute
    if (!pool_query.kinds().empty())
    {
	YCPList kind_list;

	for_(kind_it, pool_query.kinds().begin(), pool_query.kinds().end())
	{
	    kind_list->add(YCPString(kind_it->asString()));
	}

	lock->add(YCPString("kind"), kind_list);
    }

    // add "install_status" attribute
    std::string status;
    
    switch(pool_query.statusFilterFlags())
    {
	case zypp::PoolQuery::ALL : status = "all"; break;
	case zypp::PoolQuery::INSTALLED_ONLY : status = "installed"; break;
	case zypp::PoolQuery::UNINSTALLED_ONLY : status = "uninstalled"; break;
    }

    if (!status.empty())
    {
	lock->add(YCPString("install_status"), YCPString(status));
    }

    // add "repo" attribute
    if (!pool_query.repos().empty())
    {
	YCPList repo_id_list;

	for_(repo_it, pool_query.repos().begin(), pool_query.repos().end())
	{
	    repo_id_list->add(YCPInteger(logFindAlias(*repo_it)));
	}

	lock->add(YCPString("repo_id"), repo_id_list);
    }


    // add "case_sensitive" attribute
    lock->add(YCPString("case_sensitive"), YCPBoolean(pool_query.caseSensitive()));

    // add "global_string" attribute
    if (!pool_query.strings().empty())
    {
	YCPList glob_string;

	for_(string_it, pool_query.strings().begin(), pool_query.strings().end())
	{
	    glob_string->add(YCPString(*string_it));
	}

	lock->add(YCPString("global_string"), glob_string);
    }

    // add "string_type" attribute
    std::string str_type;

    switch(pool_query.matchType())
    {
	case SEARCH_STRING : str_type = "exact"; break;
	case SEARCH_SUBSTRING : str_type = "substring"; break;
	case SEARCH_GLOB : str_type = "glob"; break;
	case SEARCH_REGEX : str_type = "regex"; break;
    }

    if (!str_type.empty())
    {
	lock->add(YCPString("string_type"), YCPString(str_type));
    }

    return lock;
}

YCPValue PkgFunctions::GetLocks()
{
    YCPList ret;

    zypp::Locks &locks = zypp::Locks::instance();

    for_(it, locks.begin(), locks.end())
    {
	ret->add(PoolQuery2YCPMap(*it));
    }

    return ret;
}


YCPValue PkgFunctions::RemoveLock(const YCPMap &lock)
{
    zypp::Locks &locks = zypp::Locks::instance();

    for_(it, locks.begin(), locks.end())
    {
	YCPMap it_map = PoolQuery2YCPMap(*it);

	// is it the same lock?
	if (it_map.compare(lock) == YO_EQUAL)
	{
	    locks.removeLock(*it);
	    return YCPBoolean(true);
	}
    }

    return YCPBoolean(false);
}

