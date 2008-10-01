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


#include "log.h"
#include "ServiceManager.h"

#include <zypp/RepoManager.h>

ServiceManager::ServiceManager() : _services_loaded(false)
{
}

ServiceManager::~ServiceManager()
{
}

bool ServiceManager::LoadServices(const zypp::RepoManager &repomgr)
{
    bool ret = true;

    if (!_services_loaded)
    {
	try
	{
	    for_ (it, repomgr.serviceBegin(), repomgr.serviceEnd())
	    {
		// set the original alias to the current one
		PkgService s(*it, it->alias());
		y2milestone("Loaded service %s (%s)", s.alias().c_str(), s.url().asString().c_str());
		_known_services.insert(std::make_pair(s.alias(), s));
	    }

	    _services_loaded = true;
	}
	catch(...)
	{
	    ret = false;
	}
    }
    else
    {
	y2warning("Services have already been loaded, skipping load");
    }

    return ret;
}

bool ServiceManager::SaveServices(zypp::RepoManager &repomgr) const
{
    bool ret = true;

    for_ (it, _known_services.begin(), _known_services.end())
    {
	std::string alias(it->second.alias());

	try
	{
	    // delete
	    if (it->second.isDeleted())
	    {
		y2milestone("Removing service %s", alias.c_str());
		repomgr.removeService(alias);
	    }
	}
	catch(...)
	{
	    y2error("Cannot delete service %s", alias.c_str());
	    ret = false;
	}
    }

    for_ (it, _known_services.begin(), _known_services.end())
    {
	std::string alias(it->second.alias());

	try
	{
	    // save
	    if (!it->second.isDeleted())
	    {
		if (it->second != repomgr.getService(alias))
		{
		    y2milestone("Saving modified service %s", alias.c_str());
		    // FIXME use old alias
		    repomgr.modifyService(alias, it->second);
		}
	    }
	}
	catch(...)
	{
	    y2error("Cannot save service %s", alias.c_str());
	    ret = false;
	}
    }

    return ret;
}

bool ServiceManager::RefreshService(const std::string &alias, zypp::RepoManager &repomgr)
{
    try
    {
	PkgServices::iterator serv_it = _known_services.find(alias);

	if (serv_it == _known_services.end())
	{
	    y2error("Service '%s' does not exist", alias.c_str());
	    return false;
	}

	repomgr.refreshService(serv_it->second);
	return true;
    }
    catch(...)
    {
	return false;
    }
}

ServiceManager::Services ServiceManager::GetServices() const
{
    Services ret;

    for_ (it, _known_services.begin(), _known_services.end())
    {
	// return only valid services
	if (!it->second.isDeleted())
	{
	    ret.push_back(it->second);
	}
    }

    return ret;
}

bool ServiceManager::AddService(const std::string &alias, const std::string &url)
{
    try
    {
	if (alias.empty())
	{
	    y2error("Empty alias for service %s", url.c_str());
	    return false;
	}

	PkgServices::iterator serv_it = _known_services.find(alias);
	std::string orig_alias;

	if (serv_it != _known_services.end())
	{
	    if (serv_it->second.isDeleted())
	    {
		// remember the original alias
		// adding a removed service is the same as changing existing one
		orig_alias = serv_it->second.alias();

		// we are adding an already removed service,
		// remove the existing service
		_known_services.erase(serv_it);
	    }
	    else
	    {
		y2error("Service with alias %s already exists", alias.c_str());
		return false;
	    }
	}

	zypp::ServiceInfo info;
	info.setAlias(alias);
	info.setUrl(url);

        PkgService srv(info, orig_alias);

	_known_services.insert(std::make_pair(alias, srv));
    }
    catch(...)
    {
	return false;
    }

    return true;
}

bool ServiceManager::RemoveService(const std::string &alias)
{
    try
    {
	PkgServices::iterator serv_it = _known_services.find(alias);

	if (serv_it != _known_services.end())
	{
	    if (serv_it->second.isDeleted())
	    {
		y2warning("Service %s has been already removed", alias.c_str());
		return true;
	    }
	    else
	    {
		serv_it->second.setDeleted();
		y2milestone("Service %s has been marked as deleted", alias.c_str());
		return true;
	    }
	}
	else
	{
	    y2error("Service %s does not exist", alias.c_str());
	    return false;
	}
    }
    catch(...)
    {
	return false;
    }

    return true;
}

void ServiceManager::Reset()
{
    y2milestone("Resetting known services...");
    // reset the internal structures
    _known_services.clear();
    _services_loaded = false;
}


zypp::ServiceInfo ServiceManager::GetService(const std::string &alias) const
{
    try
    {
	PkgServices::const_iterator serv_it = _known_services.find(alias);

	if (serv_it != _known_services.end())
	{
	    if (serv_it->second.isDeleted())
	    {
		y2warning("Service %s has been removed", alias.c_str());
		return zypp::ServiceInfo::noService;
	    }
	    else
	    {
		return serv_it->second;
	    }
	}
	else
	{
	    y2error("Service %s does not exist", alias.c_str());
	    return zypp::ServiceInfo::noService;
	}
    }
    catch(...)
    {
    }

    return zypp::ServiceInfo::noService;
}

bool ServiceManager::SetService(const std::string old_alias, const zypp::ServiceInfo &srv)
{
    try
    {
	PkgServices::const_iterator serv_it = _known_services.find(old_alias);

	if (serv_it != _known_services.end())
	{
	    if (serv_it->second.isDeleted())
	    {
		y2warning("Service %s has been removed", old_alias.c_str());
		return false;
	    }
	    else
	    {
		// if the alias has been already changed then keep the original alias
		if (old_alias == serv_it->second.alias() )
		{
		    PkgService s(srv, old_alias);
		    _known_services[srv.alias()] = s;
		}
		else
		{
		    // keep the old alias
		    PkgService s(srv, serv_it->second.origAlias());
		    _known_services[srv.alias()] = s;
		}

		return true;
	    }
	}
	else
	{
	    y2error("Service %s does not exist", old_alias.c_str());
	    return false;
	}
    }
    catch(...)
    {
    }

    return false;
}

std::string ServiceManager::Probe(const zypp::Url &url, const zypp::RepoManager &repomgr) const
{
    y2milestone("Probing service at %s...", url.asString().c_str());
    std::string ret(repomgr.probeService(url).asString());
    y2milestone("Detected service type: %s", ret.c_str());

    return ret;
}

