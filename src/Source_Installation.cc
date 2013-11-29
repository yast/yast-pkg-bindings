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
   File:	$Id: Source_Installation.cc 54407 2009-01-06 16:23:19Z lslezak $
   Author:	Ladislav Slezák <lslezak@novell.com>
   Summary:     Functions used during system installation
   Namespace:   Pkg
*/

#include <PkgModuleFunctions.h>
#include "log.h"

#include <zypp/ExternalProgram.h>

#include <ycp/YCPBoolean.h>
#include <ycp/YCPVoid.h>
#include <ycp/YCPString.h>

extern "C"
{
// stat()
#include <unistd.h>
// errno
#include <errno.h>
}

// getenv()
#include <cstdlib>

/*
  Textdomain "pkg-bindings"
*/

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

bool PkgModuleFunctions::CreateDir(const std::string &path)
{
    if (path.empty())
    {
	y2error("Empty directory path");
	return false;
    }

    // check if the target exists
    struct stat stat_buf;
    if (::stat(path.c_str(), &stat_buf) == 0)
    {
	// it exists, is it a directory?
	if (S_ISDIR(stat_buf.st_mode))
	{
	    return true;
	}
	else
	{
	    // error message (followed by directory name)
	    _last_error.setLastError(_("Target is not a directory: ") + path);
	    y2error("Target %s exists but it's not a directory", path.c_str());
	    return false;
	}
    }
    else
    {
	// "No such file or directory" error, the target doesn't exist
	if (errno == ENOENT)
	{
	    y2milestone("Creating directory %s...", path.c_str());

	    const char* argv[] =
	    {
	      "mkdir",
	      // create parent dir
	      "-p",
	      // finish parameter list
	      "--",
	      // target
	      path.c_str(),
	      NULL
	    };

	    // discard stderr, no pty, stderr_fd = -1, use the default locale
	    zypp::ExternalProgram prog(argv, zypp::ExternalProgram::Discard_Stderr, false, -1, true);

	    if (prog.close())
	    {
		// error message (followed by directory name)
		_last_error.setLastError(_("Cannot create directory ") + path);
		y2error("Cannot create target directory %s", path.c_str());
		return false;
	    }
	}
	// other error
	else
	{
	    // error message (followed by directory name)
	    _last_error.setLastError(_("Cannot check status of directory ") + path);
	    y2error("Cannot stat %s: %s", path.c_str(), ::strerror(errno));
	    return false;
	}
    }

    return true;
}

bool PkgModuleFunctions::CopyToDir(const std::string &source, const std::string &target, bool backup, bool recursive)
{
    if (source.empty())
    {
	y2error("CopyToDir: Empty source parameter");
	return false;
    }

    if (target.empty())
    {
	y2error("CopyToDir: Empty target parameter");
	return false;
    }

    // check if the source exists
    struct stat stat_buf;
    if (::stat(source.c_str(), &stat_buf) != 0 && errno == ENOENT)
    {
	y2milestone("Source %s does not exist, skipping", source.c_str());
	return true;
    }

    // create the target directory
    if (!CreateDir(target))
    {
	return false;
    }

    const char* argv[] =
    {
      "cp",
      // preserve the time stamps and permissions
      "-a",
      // fill the parameters later
      NULL,
      NULL,
      NULL,
      NULL,
      NULL,
      NULL
    };

    int pos = 2;

    if (recursive)
    {
	argv[pos++] = "-r";
    }

    if (backup)
    {
	argv[pos++] = "-b";
    }

    // finish parameter list
    argv[pos++] = "--";

    // source
    argv[pos++] = source.c_str();

    // target
    argv[pos++] = target.c_str();

    // discard stderr, no pty, stderr_fd = -1, use the default locale
    zypp::ExternalProgram prog(argv, zypp::ExternalProgram::Discard_Stderr, false, -1, true);

    if (prog.close())
    {
	// error message (followed by detailed description)
	const std::string msg = _("Error: Cannot copy the cache to the target directory\n");

	// error message
	_last_error.setLastError(msg + _("Copying failed"));
	y2error("Cannot copy %s to %s", source.c_str(), target.c_str());
	return false;
    }

    return true;
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

    std::string d(dir->value());
    y2milestone("Copying source cache to '%s'...", d.c_str());

    if (d.empty())
    {
	y2error("Empty parameter in Pkg::SourceCacheCopyTo()!");
	return YCPBoolean(false);
    }

    if (!CreateDir(d))
    {
	return YCPBoolean(false);
    }

    std::string target(d + "/var/cache");

    // copy /var/cache/zypp to the target system
    if (!CopyToDir("/var/cache/zypp", target))
    {
	return YCPBoolean(false);
    }

    // copy optional files in /etc/zypp/credentials.d directory
    std::string source_cred("/etc/zypp/credentials.d");
    std::string target_cred(d + "/etc/zypp");

    // true = backup target files
    if (!CopyToDir(source_cred, target_cred, true))
    {
	return YCPBoolean(false);
    }

    // copy user's credentials
    char *homedir = ::getenv("HOME");
    if (homedir)
    {
	// see file zypp/media/CredentialManager.cc for the user's file definition
	source_cred = std::string(homedir) + "/.zypp/credentials.cat";
	target_cred = d + homedir + "/.zypp";

	// copy optional files in $HOME/.zypp/credentials.cat file
	// true = backup target files
	if (!CopyToDir(source_cred, target_cred, true))
	{
	    return YCPBoolean(false);
	}
    }

    return YCPBoolean(true);
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
    if (path.isNull())
    {
	y2error("Error: Pkg::SourceMoveDownloadArea(): nil argument");
	return YCPBoolean(false);
    }

    try
    {
	y2milestone("Moving download area of all sources to %s", path->value().c_str());
	zypp::media::MediaManager manager;
	manager.setAttachPrefix(path->value());
	_download_area = path->value();
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


