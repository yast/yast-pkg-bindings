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
   File:	$Id:$
   Author:	Ladislav Slezák <lslezak@novell.com>
   Summary:     Functions used during system installation
*/

#include <Callbacks.h>
#include <Callbacks.YCP.h>

#include <PkgModule.h>
#include <PkgModuleFunctions.h>

#include <zypp/ExternalProgram.h>

/****************************************************************************************
 * @builtin SourceSetRamCache
 * @short Obsoleted function, do not use
 * @param boolean
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceSetRamCache (const YCPBoolean& a)
{
    y2warning( "Pkg::SourceSetRamCache is obsolete and does nothing");
    return YCPBoolean( true );
}



/****************************************************************************************
 * @builtin SourceCacheCopyTo
 *
 * @short Copy cache data of all installation sources to the target
 * @description
 * Copy cache data of all installation sources to the target located below 'dir'.
 * To be called at end of initial installation.
 *
 * @param string dir Root directory of target.
 * @return boolean true on success
 **/
YCPValue
PkgModuleFunctions::SourceCacheCopyTo (const YCPString& dir)
{
    // error message (followed by detailed description)
    const std::string msg = _("Error: Cannot copy the cache to the target directory\n");

    std::string d = dir->value();
    y2milestone("Copying source cache to '%s'...", d.c_str());

    if (d.empty())
    {
	y2error("Empty parameter in Pkg::SourceCacheCopyTo()!");
	return YCPBoolean(false);
    }

    std::string target = d + "/var/cache";

    // create the target dir
    const char* argv[] =
    {
      "mkdir",
      // create parent dir
      "-p",
      // finish parameter list
      "--",
      // target
      target.c_str(),
      NULL
    };

    // discard stderr, no pty, stderr_fd = -1, use the default locale
    zypp::ExternalProgram prog(argv, zypp::ExternalProgram::Discard_Stderr, false, -1, true);

    int code = prog.close();

    if (code)
    {
	// error message (followed by directory name)
	_last_error.setLastError(msg + _("Cannot create directory ") + target);
	y2error("Cannot create target directory %s", target.c_str());
	return YCPBoolean(false);
    }

    // copy /var/cache/zypp to the target system
    const char* argv2[] =
    {
      "cp",
      // preserve time stamps
      "-a",
      // recursive
      "-r",
      // finish parameter list
      "--",
      // source
      "/var/cache/zypp",
      // target
      target.c_str(),
      NULL
    };

    // discard stderr, no pty, stderr_fd = -1, use the default locale
    zypp::ExternalProgram prog2(argv2, zypp::ExternalProgram::Discard_Stderr, false, -1, true);

    int code2 = prog2.close();

    if (code2)
    {
	// error message
	_last_error.setLastError(msg + _("Copying failed"));
	y2error("Cannot copy /var/cache/zypp to %s", d.c_str());
    }

    return YCPBoolean(!code2);
}
/****************************************************************************************
 * @builtin SourceMoveDownloadArea
 *
 * @short Move download area of CURL-based sources to specified directory
 * @param path specifies the path to move the download area to
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceMoveDownloadArea (const YCPString & path)
{
    try
    {
	y2milestone("Moving download area of all sources to %s", path->value().c_str());
	zypp::media::MediaManager manager;
	manager.setAttachPrefix(path->value());
    }
    catch (zypp::Exception & excpt)
    {
	_last_error.setLastError(ExceptionAsString(excpt));
	y2error("Pkg::SourceMoveDownloadArea has failed: %s", excpt.msg().c_str());
	return YCPBoolean(false);
    }

    y2milestone( "Download areas moved");

    return YCPBoolean(true);
}

/**
 * @builtin InstSysMode
 * @short obsoleted - do not use
 * @return void
 */
YCPValue
PkgModuleFunctions::InstSysMode ()
{
    y2warning("Pkg::InstSysMode() is obsoleted, it's not needed anymore");
    return YCPVoid();
}


