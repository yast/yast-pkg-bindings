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
   Summary:	Functions for configuring system architecture
*/

#include "PkgFunctions.h"
#include "log.h"

#include <string>

#include <ycp/YCPString.h>

#include <zypp/ZConfig.h>


/**
 * @builtin GetArchitecture
 * @short Get the current architecture
 * @description Returns the current architecture used by the package manager
 * @return string architecture
 */
YCPValue
PkgFunctions::GetArchitecture()
{
    std::string arch(zypp::ZConfig::instance().systemArchitecture().asString());

    y2milestone("Current system architecture: %s", arch.c_str());
    return YCPString(arch);
}
