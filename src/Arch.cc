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
#include <ycp/YCPBoolean.h>

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

/**
 * @builtin SetArchitecture
 * @short 
 * @description Change the architecture. The package manager (libzypp)
 * does not expect that the system architecture will change at runtime.
 * It should be set as soon as possible before using other commands.
 * @param string architecture the new architecture, e.g. "i386", "ppc", "s390x"...
 * @return boolean true on success
 */
YCPValue
PkgFunctions::SetArchitecture(const YCPString &architecture)
{
    std::string arch(architecture->value());

    try
    {
	y2warning("Switching architecture to: %s", arch.c_str());
	zypp::Arch new_arch(arch);
	zypp::ZConfig::instance().setSystemArchitecture(new_arch);
    }
    catch (const zypp::Exception& excpt)
    {
	y2error("Switching to architecture %s failed: %s", arch.c_str(), excpt.asString().c_str());
	_last_error.setLastError(ExceptionAsString(excpt));
	return YCPBoolean(false);
    }

    return YCPBoolean(true);
}

/**
 * @builtin SystemArchitecture
 * @short
 * @description
 * @return string default system architecture for the system
 */
YCPValue
PkgFunctions::SystemArchitecture()
{
    std::string def_arch(zypp::ZConfig::defaultSystemArchitecture().asString());
    y2milestone("Default system architecture: %s", def_arch.c_str());

    return YCPString(def_arch);
}

