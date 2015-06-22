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

#include <ostream>

#include "log.h"
#include "ServiceManager.h"

#include <zypp/RepoManager.h>
#include <zypp/PathInfo.h>
#include <y2util/Y2SLog.h>

ServiceManager::ServiceManager() : _services_loaded(false)
{
}

ServiceManager::~ServiceManager()
{
}

void ServiceManager::LoadServices(const zypp::RepoManager &repomgr)
{
    if (!_services_loaded)
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
    else
    {
	y2warning("Services have already been loaded, skipping load");
    }
}

void ServiceManager::SaveServices(zypp::RepoManager &repomgr)
{
    for (PkgServices::iterator it = _known_services.begin(); it != _known_services.end();)
    {
        if (it->second.isDeleted())
        {
            std::string alias(it->second.alias());

            zypp::ServiceInfo info = repomgr.getService(alias);
            // check if the service file exists to avoid raising an exception
            if (zypp::PathInfo(info.filepath()).isExist())
            {
                y2milestone("Removing service %s", alias.c_str());
                repomgr.removeService(alias);
            }

            // erase the removed service, not needed anymore after the final removal
            _known_services.erase(it++);
        }
        else
        {
          ++it;
        }
    }

    for_ (it, _known_services.begin(), _known_services.end())
    {
        SavePkgService(it->second, repomgr);
    }
}

bool ServiceManager::SaveService(const std::string &alias, zypp::RepoManager &repomgr)
{
    PkgServices::iterator serv_it = _known_services.find(alias);

    if (serv_it == _known_services.end())
    {
	y2error("Service '%s' does not exist", alias.c_str());
	return false;
    }

    if (serv_it->second.isDeleted())
    {
	y2error("Service '%s' has been deleted", alias.c_str());
	return false;
    }

    SavePkgService(serv_it->second, repomgr);

    return true;
}

bool ServiceManager::RefreshService(const std::string &alias, zypp::RepoManager &repomgr)
{
    PkgServices::iterator serv_it = _known_services.find(alias);

    if (serv_it == _known_services.end() || serv_it->second.isDeleted())
    {
	y2error("Service '%s' does not exist", alias.c_str());
	return false;
    }

    repomgr.refreshService(serv_it->second);

    // load the service from disk
    PkgService new_service(repomgr.getService(alias), alias);
    DBG << "Reloaded service: " << new_service;

    // remove the old service
    _known_services.erase(serv_it);
    // insert new
    _known_services.insert(std::make_pair(alias, new_service));

    return true;
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
    if (alias.empty())
    {
	y2error("Empty alias for service %s", url.c_str());
	return false;
    }

    PkgServices::iterator serv_it = _known_services.find(alias);
    std::string orig_alias = alias;

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
    y2milestone("Adding service %s (orig alias: %s)", alias.c_str(), srv.origAlias().c_str());

    _known_services.insert(std::make_pair(alias, srv));

    return true;
}

bool ServiceManager::RemoveService(const std::string &alias)
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

void ServiceManager::Reset()
{
    y2milestone("Resetting known services...");
    // reset the internal structures
    _known_services.clear();
    _services_loaded = false;
}


zypp::ServiceInfo ServiceManager::GetService(const std::string &alias) const
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

    return zypp::ServiceInfo::noService;
}

bool ServiceManager::SetService(const std::string &old_alias, const zypp::ServiceInfo &srv)
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
	    y2milestone("Setting service: %s (orig alias: %s)", old_alias.c_str(), serv_it->second.origAlias().c_str());
	    DBG << "Properties: " << srv << std::endl;
	    PkgService s(srv, serv_it->second.origAlias());
	    _known_services[srv.alias()] = s;
	    return true;
	}
    }
    else
    {
	y2error("Service %s does not exist", old_alias.c_str());
	return false;
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

bool ServiceManager::empty() const
{
    return _known_services.empty();
}


ServiceManager::Services::size_type ServiceManager::size() const
{
    return _known_services.size();
}

void ServiceManager::SavePkgService(PkgService &s_known, zypp::RepoManager &repomgr) const
{
    const std::string alias(s_known.alias());
    const zypp::ServiceInfo s_stored = repomgr.getService(alias);

    DBG << "Known Service: " << s_known << std::endl;
    DBG << "Stored Service: " << s_stored << std::endl;

    std::string orig_alias(s_known.origAlias());

    y2debug("orig_alias: %s", orig_alias.c_str());

    // already saved?
    if (s_stored == zypp::ServiceInfo::noService)
    {
        y2milestone("Adding new service %s", alias.c_str());
        // add the service
        repomgr.addService(s_known);
        // set the old alias to properly save it next time
        s_known.setOrigAlias(alias);
    }
    else
    {
        y2milestone("Saving service %s", alias.c_str());
        // use the old alias
        repomgr.modifyService(orig_alias, s_known);
    }
}
