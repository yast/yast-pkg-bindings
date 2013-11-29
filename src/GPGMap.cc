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
   Summary:     Class for converting zypp::PublicKey to YCPMap
*/

#include "GPGMap.h"
#include "i18n.h"

#include <ycp/YCPString.h>
#include <ycp/YCPInteger.h>
#include <ycp/YCPBoolean.h>

#include <zypp/Date.h>
#include <zypp/PublicKey.h>

/*
  Textdomain "pkg-bindings"
*/

GPGMap::GPGMap(const zypp::PublicKey &key)
{
    gpg_map->add(YCPString("id"), YCPString(key.id()));
    gpg_map->add(YCPString("name"), YCPString(key.name()));
    gpg_map->add(YCPString("fingerprint"), YCPString(key.fingerprint()));
    gpg_map->add(YCPString("path"), YCPString(key.path().asString()));

    zypp::Date date(key.created());
    // %x = date only, see man strftime
    gpg_map->add(YCPString("created"), YCPString(date.form("%x")));
    gpg_map->add(YCPString("created_raw"), YCPInteger(zypp::Date::ValueType(date)));

    date = key.expires();
    std::string expires((date == 0) ? _("Never") : date.form("%x"));
    gpg_map->add(YCPString("expires"), YCPString(expires));
    gpg_map->add(YCPString("expires_raw"), YCPInteger(zypp::Date::ValueType(date)));
}

void GPGMap::setTrusted(bool trusted)
{
    // is the key trusted?
    gpg_map->add(YCPString("trusted"), YCPBoolean(trusted));
}
