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
   Summary:     Functions for downloading files from a repository
*/

#include <Callbacks.h>
#include <Callbacks.YCP.h>

#include <PkgModule.h>
#include <PkgModuleFunctions.h>

/*
  Textdomain "pkg-bindings"
*/

YCPValue PkgModuleFunctions::SourceProvideFileCommon(const YCPInteger &id,
					       const YCPInteger &mid,
					       const YCPString& f,
					       const YCPBoolean & optional)
{
    CallInitDownload(std::string(_("Downloading ") + f->value()));

    bool found = true;
    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
      found = false;

    extern ZyppRecipients::MediaChangeSensitivity _silent_probing;
    // remember the current value
    ZyppRecipients::MediaChangeSensitivity _silent_probing_old = _silent_probing;

    // disable media change callback for optional file
    if (optional->value())
	_silent_probing = ZyppRecipients::MEDIA_CHANGE_OPTIONALFILE;

    zypp::filesystem::Pathname path; // FIXME use ManagedMedia
    if (found)
    {
	try
	{
	    path = repo->mediaAccess()->provideFile(f->value(), mid->value());
	    y2milestone("local path: '%s'", path.asString().c_str());
	}
	catch (const zypp::Exception& excpt)
	{
	    found = false;

	    if (!optional->value())
	    {
		_last_error.setLastError(ExceptionAsString(excpt));
		y2milestone("File not found: %s", f->value_cstr());
	    }
	}
    }

    // set the original probing value
    _silent_probing = _silent_probing_old;

    CallDestDownload();

    if (found)
    {
	return YCPString(path.asString());
    }
    else
    {
	return YCPVoid();
    }
}


/****************************************************************************************
 * @builtin SourceProvideFile
 *
 * @short Make a file available at the local filesystem
 * @description
 * Let an InstSrc provide some file (make it available at the local filesystem).
 *
 * @param integer SrcId	Specifies the InstSrc .
 * @param integer medianr Number of the media the file is located on ('1' for the 1st media).
 * @param string file Filename relative to the media root.
 *
 * @return string local path as string
 **/
YCPValue
PkgModuleFunctions::SourceProvideFile (const YCPInteger& id, const YCPInteger& mid, const YCPString& f)
{
    return SourceProvideFileCommon(id, mid, f, false /*optional*/);
}

/****************************************************************************************
 * @builtin SourceProvideOptionalFile
 *
 * @short Make an optional file available at the local filesystem
 * @description
 * Let an InstSrc provide some file (make it available at the local filesystem).
 * If the file doesn't exist don't ask user for another medium and return nil
 *
 * @param integer SrcId	Specifies the InstSrc .
 * @param integer medianr Number of the media the file is located on ('1' for the 1st media).
 * @param string file Filename relative to the media root.
 *
 * @return string local path as string
 **/
YCPValue
PkgModuleFunctions::SourceProvideOptionalFile (const YCPInteger& id, const YCPInteger& mid, const YCPString& f)
{
    return SourceProvideFileCommon(id, mid, f, true /*optional*/);
}

/****************************************************************************************
 * @builtin SourceProvideDir
 * @short make a directory available at the local filesystem
 * @description
 * Let an InstSrc provide some directory (make it available at the local filesystem) and
 * all the files within it (non recursive).
 *
 * @param integer SrcId	Specifies the InstSrc .
 * @param integer medianr Number of the media the file is located on ('1' for the 1st media).
 * @param string dir Directoryname relative to the media root.
 * @return string local path as string
 */
YCPValue
PkgModuleFunctions::SourceProvideDir (const YCPInteger& id, const YCPInteger& mid, const YCPString& d)
{
    y2warning("Pkg::SourceProvideDir() is obsoleted use Pkg::SourceProvideDirectory() instead");
    // non optional, non recursive
    return SourceProvideDirectory(id, mid, d, false, false);
}


/****************************************************************************************
 * @builtin SourceProvideDirectory
 * @short make a directory available at the local filesystem
 * @description
 * Download a directory from repository (make it available at the local filesystem) and
 * all the files within it.
 *
 * @param integer id repository to use (id)
 * @param integer mid Number of the media where the directory is located on ('1' for the 1st media).
 * @param string d Directory name relative to the media root.
 * @param boolean optional set to true if the directory may not exist (do not report errors)
 * @param boolean recursive set to true to provide all subdirectories recursively
 * @return string local path as string or nil when an error occured
 */
YCPValue
PkgModuleFunctions::SourceProvideDirectory(const YCPInteger& id, const YCPInteger& mid, const YCPString& d, const YCPBoolean &optional, const YCPBoolean &recursive)
{
    CallInitDownload(std::string(_("Downloading ") + d->value()));

    bool found = true;
    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
      found = false;

    zypp::filesystem::Pathname path; // FIXME user ManagedMedia

    extern ZyppRecipients::MediaChangeSensitivity _silent_probing;
    // remember the current value
    ZyppRecipients::MediaChangeSensitivity _silent_probing_old = _silent_probing;

    // disable media change callback for optional file
    if (optional->value())
	_silent_probing = ZyppRecipients::MEDIA_CHANGE_OPTIONALFILE;

    if (found)
    {
	try
	{
	    path = repo->mediaAccess()->provideDir(d->value(), recursive->value(), mid->value());
	}
	catch (const zypp::Exception& excpt)
	{
            _last_error.setLastError(ExceptionAsString(excpt));
            y2milestone ("Directory not found: %s", d->value_cstr());
            found = false;
	}
    }

    // set the original probing value
    _silent_probing = _silent_probing_old;

    CallDestDownload();

    if (found)
    {
	return YCPString(path.asString());
    }
    else
    {
	return YCPVoid();
    }
}


/****************************************************************************************
 * @builtin SourceRefreshNow
 * @short Attempt to immediately refresh a Source
 * @description
 * The InsrSrc will be encouraged to check and refresh all metadata
 * cached on disk.
 *
 * @param integer SrcId Specifies the InstSrc.
 *
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceRefreshNow (const YCPInteger& id)
{
    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
	return YCPBoolean(false);

    try
    {
	zypp::RepoManager repomanager = CreateRepoManager();
	y2milestone("Refreshing metadata '%s'", repo->repoInfo().alias().c_str());
	RefreshWithCallbacks(repo->repoInfo());

	y2milestone("Caching source '%s'...", repo->repoInfo().alias().c_str());
	repomanager.buildCache(repo->repoInfo());
    }
    catch ( const zypp::Exception & expt )
    {
	y2error ("Error while refreshing the source: %s", expt.asString().c_str());
	_last_error.setLastError(repo->repoInfo().alias() + ": " + ExceptionAsString(expt));
	return YCPBoolean(false);
    }

    return YCPBoolean( true );
}

