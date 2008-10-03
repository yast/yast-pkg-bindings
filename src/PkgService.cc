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


#include "PkgService.h"

PkgService::PkgService() : _deleted(false), _old_alias()
{
}

PkgService::PkgService(const zypp::ServiceInfo &s, const std::string &old_alias) :
    zypp::ServiceInfo(s), _deleted(false), _old_alias(old_alias)
{
}

PkgService::PkgService(const PkgService &s) :
    zypp::ServiceInfo(s), _deleted(s._deleted), _old_alias(s._old_alias)
{
}

PkgService::~PkgService()
{
}

bool PkgService::isDeleted() const
{
    return _deleted;
}

void PkgService::setDeleted()
{
    _deleted = true;
}

std::string PkgService::origAlias() const
{
    return _old_alias;
}

