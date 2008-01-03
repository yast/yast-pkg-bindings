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
   Summary:     Functions for initializing the package manager (target system)
   Namespace:   Pkg
*/

#include <PkgFunctions.h>
#include "log.h"

#include <ycp/YCPBoolean.h>
#include <ycp/YCPString.h>

#include "PkgProgress.h"
#include <HelpTexts.h>

/*
  Textdomain "pkg-bindings"
*/

/** ------------------------
 *
 * @builtin TargetInit
 * @deprecated
 * @short Initialize Target and load resolvables
 * @param string root Root Directory
 * @param boolean unused Dummy option, only for backward compatibility
 * @return boolean
 */
YCPValue
PkgFunctions::TargetInit (const YCPString& root, const YCPBoolean & /*unused_and_broken*/)
{
    std::string r = root->value();

    std::list<std::string> stages;
    stages.push_back(_("Initialize the Target System"));
    stages.push_back(_("Read Installed Packages"));

    PkgProgress pkgprogress(_callbackHandler);
    pkgprogress.Start(_("Loading the Package Manager..."), stages, HelpTexts::load_target);

    try
    {
	zypp_ptr()->initializeTarget(r);
	pkgprogress.NextStage();
        zypp_ptr()->addResolvables( zypp_ptr()->target()->resolvables(), true );
    }
    catch (zypp::Exception & excpt)
    {
	_last_error.setLastError(ExceptionAsString(excpt));
	y2error("TargetInit has failed: %s", excpt.msg().c_str() );
        return YCPError(excpt.msg().c_str(), YCPBoolean(false));
    }
    
    _target_root = zypp::Pathname(root->value());

    pkgprogress.Done();
    
    return YCPBoolean(true);
}

/** ------------------------
 *
 * @builtin TargetInitialize
 * @short Initialize Target, read the keys.
 * @param string root Root Directory
 * @return boolean
 */
YCPValue
PkgFunctions::TargetInitialize (const YCPString& root)
{
    std::string r = root->value();

    try
    {
        zypp_ptr()->initializeTarget(r);
    }
    catch (zypp::Exception & excpt)
    {
        _last_error.setLastError(ExceptionAsString(excpt));
        y2error("TargetInit has failed: %s", excpt.msg().c_str() );
        return YCPError(excpt.msg().c_str(), YCPBoolean(false));
    }
    
    _target_root = zypp::Pathname(root->value());
    
    return YCPBoolean(true);
}

/** ------------------------
 *
 * @builtin TargetLoad
 * @short Load target resolvables into the pool
 * @return boolean
 */
YCPValue
PkgFunctions::TargetLoad ()
{
    std::list<std::string> stages;
    stages.push_back(_("Read Installed Packages"));

    PkgProgress pkgprogress(_callbackHandler);

    pkgprogress.Start(_("Loading the Package Manager..."), stages, HelpTexts::load_target);

    try
    {
        zypp_ptr()->addResolvables( zypp_ptr()->target()->resolvables(), true );
    }
    catch (zypp::Exception & excpt)
    {
        _last_error.setLastError(ExceptionAsString(excpt));
        y2error("TargetLoad has failed: %s", excpt.msg().c_str() );
        return YCPError(excpt.msg().c_str(), YCPBoolean(false));
    }

    pkgprogress.Done();
    
    return YCPBoolean(true);
}

/** ------------------------
 *
 * @builtin TargetFinish
 *
 * @short finish target usage
 * @return boolean
 */
YCPBoolean
PkgFunctions::TargetFinish ()
{
    try
    {
	zypp_ptr()->finishTarget();
    }
    catch (zypp::Exception & excpt)
    {
	_last_error.setLastError(ExceptionAsString(excpt));
	y2error("TargetFinish has failed: %s", excpt.msg().c_str() );
        return YCPBoolean(false);
    }

    return YCPBoolean(true);
}


