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
   File:	$Id: Source_Load.cc 49526 2008-07-30 13:28:18Z mvidner $
   Author:	Ladislav Slezák <lslezak@novell.com>
   Summary:     Network related functions
   Namespace:   Pkg
*/

#include "log.h"

// system()
#include <cstdlib>

#include "PkgModuleFunctions.h"

/*
  Textdomain "pkg-bindings"
*/

/*
  A helper function
  Detect whether there is a network connection.
  See isNetworkRunning() function in NetworkService.ycp
*/
bool PkgModuleFunctions::NetworkDetected()
{
    y2milestone("Checking the network status...");
    // check IPv4 network
    int result = ::system("ip addr|grep -v '127.0.0\\|inet6'|grep -q inet &> /dev/null");
    y2milestone("Network is running: %s", (result == 0) ? "yes" : "no");

    return !result;
}

/*
  A helper function
  Is the URL remote?
*/
bool PkgModuleFunctions::remoteRepo(const zypp::Url &url)
{
    // is it a remote repository?
    return url.schemeIsRemote();
}
