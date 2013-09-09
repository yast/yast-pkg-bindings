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
   Summary:     Functions for downloading files from a repository
   Namespace:   Pkg
*/

#include <Callbacks.h>
#include <Callbacks.YCP.h>

#include <PkgFunctions.h>
#include <PkgProgress.h>
#include "log.h"

#include <HelpTexts.h>

#include <zypp/Fetcher.h>

/*
  Textdomain "pkg-bindings"
*/

YCPValue PkgFunctions::SourceProvideFileCommon(const YCPInteger &id,
					       const YCPInteger &mid,
					       const YCPString& f,
					       const bool optional,
					       const bool check_signatures,
					       const bool digested)
{
    if (id.isNull() || mid.isNull() || f.isNull())
    {
	y2error("ProvideFile: nil argument!");
	return YCPVoid();
    }

    CallInitDownload(std::string(_("Downloading ") + f->value()));

    bool found = true;
    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
      found = false;

    extern ZyppRecipients::MediaChangeSensitivity _silent_probing;
    // remember the current value
    ZyppRecipients::MediaChangeSensitivity _silent_probing_old = _silent_probing;

    // disable media change callback for optional file
    if (optional)
	_silent_probing = ZyppRecipients::MEDIA_CHANGE_OPTIONALFILE;

    y2milestone("Downloading %s%sfile %s from repository %lld, medium %lld",
	(optional ? "optional " : ""), 
	(check_signatures ? (digested ? "digested " : "signed ") : ""),
	f->value().c_str(),
	id->value(),
	mid->value()
    );

    zypp::filesystem::Pathname path; // FIXME use ManagedMedia
    if (found)
    {
	try
	{
	    if (check_signatures)
	    {
		// use a Fetcher for downloading signed files (see bnc#409927)
		zypp::Fetcher fch;
		fch.reset();
		fch.setOptions(zypp::Fetcher::AutoAddIndexes);

		// path - add "/" to the beginning if it's missing there
		std::string media_path(f->value());
		if (media_path.size() >= 1 && media_path[0] != '/')
		{
		    media_path = "/" + media_path;
		}

		zypp::OnMediaLocation mloc(media_path, mid->value());
		mloc.setOptional(optional);

		// create the tmpdir in <_download_area>/var/tmp
		zypp::filesystem::TmpDir tmpdir(_download_area / zypp::filesystem::TmpDir::defaultLocation());

		// keep a reference to the tmpdir so the directory is not deleted at the and of the block		
		tmp_dirs.push_back(tmpdir);
		path = tmpdir.path();

		if (digested)
		{
		    fch.enqueueDigested(mloc);
		}
		else
		{
		    fch.addIndex(mloc);
		}

		fch.start(path, *repo->mediaAccess()); // uses MediaAccess to retrieve
		fch.reset();
		path /= f->value();
	    }
	    else
	    {
		path = repo->mediaAccess()->provideFile(f->value(), mid->value());
		y2milestone("local path: '%s'", path.asString().c_str());
	    }
	}
	catch (const zypp::Exception& excpt)
	{
	    found = false;

	    if (!optional)
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
	// check if the file really exists
	struct stat buf;
	int status = ::stat(path.asString().c_str(), &buf);

	y2debug("File: %s, status: %d", path.asString().c_str(), status);

	if (status != 0)
	{
	    if (errno == ENOENT)
	    {
		y2milestone("File not found: %s, download failed", path.asString().c_str());
	    }
	    else
	    {
		y2error("Cannot check status of the downloaded file: %s", strerror(errno));
	    }

	    return YCPVoid();
	}

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
 * Warning: The downloaded files are removed in Pkg::SourceReleaseAll()!
 *
 * @param integer SrcId	Specifies the InstSrc .
 * @param integer medianr Number of the media the file is located on ('1' for the 1st media).
 * @param string file Filename relative to the media root.
 *
 * @return string local path as string
 **/
YCPValue
PkgFunctions::SourceProvideFile (const YCPInteger& id, const YCPInteger& mid, const YCPString& f)
{
    return SourceProvideFileCommon(id, mid, f, false /*optional*/, false /* signed */, true /* digested, doesn't matter in this case*/);
}

/****************************************************************************************
 * @builtin SourceProvideOptionalFile
 *
 * @short Make an optional file available at the local filesystem
 * @description
 * Let an InstSrc provide some file (make it available at the local filesystem).
 * If the file doesn't exist don't ask user for another medium and return nil
 * Warning: The downloaded files are removed in Pkg::SourceReleaseAll()!
 *
 * @param integer SrcId	Specifies the InstSrc .
 * @param integer medianr Number of the media the file is located on ('1' for the 1st media).
 * @param string file Filename relative to the media root.
 *
 * @return string local path as string
 **/
YCPValue
PkgFunctions::SourceProvideOptionalFile (const YCPInteger& id, const YCPInteger& mid, const YCPString& f)
{
    return SourceProvideFileCommon(id, mid, f, true /*optional*/, false /* signed */, true /* digested, doesn't matter in this case*/);
}

/****************************************************************************************
 * @builtin SourceProvideSignedFile
 *
 * @short Make a signed file available at the local filesystem
 * @description
 * Make a signed Let an InstSrc provide some file (make it available at the local filesystem).
 * The signature is read from <filename>.asc file, the GPG key is read from <filename>.key file.
 * Warning: The downloaded files are removed in Pkg::SourceReleaseAll()!
 *
 * @param integer id Source ID
 * @param integer mid Number of the media the file is located on ('1' for the 1st media).
 * @param string f Filename relative to the media root.
 * @param boolean optional true if the file can be missing on the medium, if an optional file doesn't exist do not ask user for another medium and return nil.
 *
 * @return string local path as string
 **/
YCPValue
PkgFunctions::SourceProvideSignedFile (const YCPInteger& id, const YCPInteger& mid, const YCPString& f, const YCPBoolean &optional)
{
    if (optional.isNull())
    {
	y2error("SourceProvideSignedFile: argument 'optional' is nil!");
	return YCPVoid();
    }

    return SourceProvideFileCommon(id, mid, f, optional->value() /*optional*/, true /* signed */, false /* not digested = signed*/);
}

/****************************************************************************************
 * @builtin SourceProvideDigestedFile
 *
 * @short Make a digested file available at the local filesystem
 * @description
 * Make a signed Let an InstSrc provide some file (make it available at the local filesystem).
 * The checksum is stored either in /content file or in SHA1SUMS file.
 * Warning: The downloaded files are removed in Pkg::SourceReleaseAll()!
 *
 * @param integer id Source ID
 * @param integer mid Number of the media the file is located on ('1' for the 1st media).
 * @param string f Filename relative to the media root.
 * @param boolean optional true if the file can be missing on the medium, if an optional file doesn't exist do not ask user for another medium and return nil.
 *
 * @return string local path as string
 **/
YCPValue
PkgFunctions::SourceProvideDigestedFile (const YCPInteger& id, const YCPInteger& mid, const YCPString& f, const YCPBoolean &optional)
{
    if (optional.isNull())
    {
	y2error("SourceProvideDigestedFile: argument 'optional' is nil!");
	return YCPVoid();
    }

    return SourceProvideFileCommon(id, mid, f, optional->value() /*optional*/, true /* signed */, true /* digested */);
}

/****************************************************************************************
 * @builtin SourceProvideDirectory
 * @short make a directory available at the local filesystem
 * @description
 * Download a directory from repository (make it available at the local filesystem) and
 * all the files within it.
 * Warning: The downloaded files are removed in Pkg::SourceReleaseAll()!
 *
 * @param integer id repository to use (id)
 * @param integer mid Number of the media where the directory is located on ('1' for the 1st media).
 * @param string d Directory name relative to the media root.
 * @param boolean optional set to true if the directory may not exist (do not report errors)
 * @param boolean recursive set to true to provide all subdirectories recursively
 * @return string local path as string or nil when an error occured
 */
YCPValue
PkgFunctions::SourceProvideDirectory(const YCPInteger& id, const YCPInteger& mid, const YCPString& d, const YCPBoolean &optional, const YCPBoolean &recursive)
{
    // dowanload the directory, do not check the signatures
    return SourceProvideDirectoryInternal(id, mid, d, optional, recursive, false);
}

/****************************************************************************************
 * @builtin SourceProvideSignedDirectory
 * @short make a directory available at the local filesystem
 * @description
 * Download a directory from repository (make it available at the local filesystem) and
 * all the files within it. Requires that all files have been signed with SHA1 checksum.
 * If there is no checksum or the checksum doesn't match the download fails.
 * Warning: The downloaded files are removed in Pkg::SourceReleaseAll()!
 *
 * @param integer id repository to use (id)
 * @param integer mid Number of the media where the directory is located on ('1' for the 1st media).
 * @param string d Directory name relative to the media root.
 * @param boolean optional set to true if the directory may not exist (do not report errors)
 * @param boolean recursive set to true to provide all subdirectories recursively
 * @return string local path as string or nil when an error occured
 */
YCPValue
PkgFunctions::SourceProvideSignedDirectory(const YCPInteger& id, const YCPInteger& mid, const YCPString& d, const YCPBoolean &optional, const YCPBoolean &recursive)
{
    // dowanload the directory, force signature checks
    return SourceProvideDirectoryInternal(id, mid, d, optional, recursive, true);
}

YCPValue
PkgFunctions::SourceProvideDirectoryInternal(const YCPInteger& id, const YCPInteger& mid, const YCPString& d, const YCPBoolean &optional, const YCPBoolean &recursive, bool check_signatures)
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
	    if (check_signatures)
	    {
		// use a Fetcher for downloading signed files (see bnc#409927)
		zypp::Fetcher f;
		f.reset();
		zypp::OnMediaLocation mloc(d->value(), mid->value());
		// create the tmpdir in <_root>/var/tmp
		zypp::filesystem::TmpDir tmpdir(_download_area / zypp::filesystem::TmpDir::defaultLocation() );

		// keep the reference to the tmpdir so the directory is not deleted at the and of the block		
		tmp_dirs.push_back(tmpdir);
		path = tmpdir.path();
		f.setOptions(zypp::Fetcher::AutoAddIndexes);
		f.enqueueDigestedDir(mloc, recursive->value());
		f.start(path, *repo->mediaAccess()); // uses MediaAccess to retrieve
		f.reset();
	    }
	    else
	    {
		path = repo->mediaAccess()->provideDir(d->value(), recursive->value(), mid->value());
	    }
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



YCPValue
PkgFunctions::SourceRefreshHelper (const YCPInteger& id, bool forced)
{
    y2milestone("Forced refresh : %s", forced ? "true" : "false");

    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
	return YCPBoolean(false);

    PkgProgress pkgprogress(_callbackHandler);
    std::list<std::string> stages;

    // stages: "download", "build cache"
    stages.push_back(_("Refresh Metadata"));
    stages.push_back(_("Rebuild Cache"));

    // two steps
    zypp::ProgressData prog_total(2);
    prog_total.sendTo(pkgprogress.Receiver());

    // 3 steps per repository (download, cache rebuild, load resolvables)
    pkgprogress.Start(_("Refreshing Repository..."), stages, _(HelpTexts::refresh_help));

    try
    {
	zypp::RepoManager* repomanager = CreateRepoManager();
	y2milestone("Refreshing metadata '%s'", repo->repoInfo().alias().c_str());
	RefreshWithCallbacks(repo->repoInfo(), zypp::ProgressData::ReceiverFnc(), forced ? zypp::RepoManager::RefreshForced : zypp::RepoManager::RefreshIfNeeded);

	// next stage, increase progress
	prog_total.incr();
	pkgprogress.NextStage();

	y2milestone("Caching source '%s'...", repo->repoInfo().alias().c_str());
	repomanager->buildCache(repo->repoInfo(), forced ? zypp::RepoManager::BuildForced : zypp::RepoManager::BuildIfNeeded);
    }
    catch ( const zypp::Exception & expt )
    {
	y2error ("Error while refreshing the source: %s", expt.asString().c_str());
	_last_error.setLastError(repo->repoInfo().alias() + ": " + ExceptionAsString(expt));
	return YCPBoolean(false);
    }

    pkgprogress.Done();

    return YCPBoolean( true );
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
PkgFunctions::SourceRefreshNow (const YCPInteger& id)
{
    // refresh if needed
    return SourceRefreshHelper(id);
}

/****************************************************************************************
 * @builtin SourceForceRefreshNow
 * @short Unconditionally refresh a Source
 * @description
 * Downloads repository metadata and rebuilds the cache
 *
 * @param integer SrcId Specifies the InstSrc.
 *
 * @return boolean
 **/
YCPValue
PkgFunctions::SourceForceRefreshNow (const YCPInteger& id)
{
    // force refresh
    return SourceRefreshHelper(id, true);
}
