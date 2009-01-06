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

#include <zypp/Locks.h>
#include <zypp/ZConfig.h>

/*
  Textdomain "pkg-bindings"
*/

YCPValue
PkgFunctions::TargetInitInternal(const YCPString& root, bool rebuild_rpmdb)
{
    const std::string r(root->value());

    // display the progress if the target is changed or if the resolvables haven't been loaded
    // otherwise there will be a quick flashing progress with no real action
    if (_target_root == r && _target_loaded)
    {
	y2milestone("Target %s is already initialized", r.c_str());
	return YCPBoolean(true);
    }

    std::list<std::string> stages;
    stages.push_back(_("Initialize the Target System"));
    stages.push_back(_("Read Installed Packages"));

    PkgProgress pkgprogress(_callbackHandler);
    pkgprogress.Start(_("Loading the Package Manager..."), stages, _(HelpTexts::load_target));

    try
    {
	if (rebuild_rpmdb)
	{
	    y2milestone("Initializing the target with rebuild");
	}

	zypp_ptr()->initializeTarget(r, rebuild_rpmdb);
	pkgprogress.NextStage();
        zypp_ptr()->target()->load();
	_target_loaded = true;
    }
    catch (zypp::Exception & excpt)
    {
	_last_error.setLastError(ExceptionAsString(excpt));
	y2error("TargetInit has failed: %s", excpt.msg().c_str() );
        return YCPError(excpt.msg().c_str(), YCPBoolean(false));
    }
    
    _target_root = zypp::Pathname(r);

    // locks are optional, might not be present on the target
    zypp::Pathname lock_file(_target_root + zypp::ZConfig::instance().locksFile());
    try
    {
	// read and apply the persistent locks
	y2milestone("Reading locks from %s", lock_file.asString().c_str());
	zypp::Locks::instance().readAndApply(lock_file);
    }
    catch (zypp::Exception & excpt)
    {
	y2warning("Error reading persistent locks from %s", lock_file.asString().c_str());
    }

    pkgprogress.Done();
    
    return YCPBoolean(true);
}

/** ------------------------
 *
 * @builtin TargetInit
 * @short Initialize Target and load resolvables
 * @param string root Root Directory
 * @param boolean unused Dummy option, only for backward compatibility
 * @return boolean
 */
YCPValue
PkgFunctions::TargetInit (const YCPString& root, const YCPBoolean & /*unused_and_broken*/)
{
    return TargetInitInternal(root, false);
}

/** ------------------------
 *
 * @builtin TargetRebuildInit
 * @short Similar to TargetInit, but the RPM DB is rebuilt ('rpm --rebuilddb') before reading it
 * @param string root Root Directory
 * @return boolean
 */
YCPValue
PkgFunctions::TargetRebuildInit(const YCPString& root)
{
    return TargetInitInternal(root, true);
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
    if (_target_loaded)
    {
	y2milestone("The target system is already loaded");
	return YCPBoolean(true);
    }

    std::list<std::string> stages;
    stages.push_back(_("Read Installed Packages"));

    PkgProgress pkgprogress(_callbackHandler);

    pkgprogress.Start(_("Loading the Package Manager..."), stages, _(HelpTexts::load_target));

    try
    {
        zypp_ptr()->target()->load();
	_target_loaded = true;
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
    
	zypp::Pathname lock_file(_target_root + zypp::ZConfig::instance().locksFile());
	zypp::Locks::instance().save(lock_file);
    }
    catch (zypp::Exception & excpt)
    {
	_last_error.setLastError(ExceptionAsString(excpt));
	y2error("TargetFinish has failed: %s", excpt.msg().c_str() );
        return YCPBoolean(false);
    }

    // reset the target
    _target_root = zypp::Pathname();
    _target_loaded = false;

    return YCPBoolean(true);
}


