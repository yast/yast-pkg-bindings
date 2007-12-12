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
   Summary:     Functions for changing properties of a repository 
*/

#include <Callbacks.h>
#include <Callbacks.YCP.h>

#include <PkgModule.h>
#include <PkgModuleFunctions.h>

#include <PkgProgress.h>

/*
  Textdomain "pkg-bindings"
*/

/****************************************************************************************
 * @builtin SourceSetEnabled
 *
 * @short Set the default activation state of an InsrSrc.
 * @param integer SrcId Specifies the InstSrc.
 * @param boolean enabled Default activation state of source.
 *
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceSetEnabled (const YCPInteger& id, const YCPBoolean& e)
{
    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
	return YCPBoolean(false);

    // no change required
    bool enable = e->value();
    if ((enable && repo->repoInfo().enabled())
	|| (!enable && !repo->repoInfo().enabled()))
    return YCPBoolean(true);

    bool success = true;

    try
    {
	repo->repoInfo().setEnabled(enable);

	// add/remove resolvables
	if (enable)
	{
	    // load resolvables only when they are missing
	    if (!AnyResolvableFrom(repo->repoInfo().alias()))
	    {
		PkgProgress pkgprogress(_callbackHandler);
		// TODO: start pkgprogress
		success = LoadResolvablesFrom(repo->repoInfo(), pkgprogress.Receiver());
	    }
	}
	else
	{
	    // the source has been disabled, remove resolvables from the pool
	    RemoveResolvablesFrom(repo->repoInfo().alias());
	}

	PkgFreshen();
    }
    catch (const zypp::Exception& excpt)
    {
	std::string alias = repo->repoInfo().alias();
	y2error ("Error for '%s': %s", alias.c_str(), excpt.asString().c_str());
	_last_error.setLastError(alias + ": " + ExceptionAsString(excpt));
	success = false;
    }

    return YCPBoolean(success);
}

/****************************************************************************************
 * @builtin SourceSetAutorefresh
 *
 * @short Set whether this source should automaticaly refresh it's
 * meta data when it gets enabled. (default true, if not CD/DVD)
 * @param integer SrcId Specifies the InstSrc.
 * @param boolean enabled Whether autorefresh should be turned on or off.
 *
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceSetAutorefresh (const YCPInteger& id, const YCPBoolean& e)
{
    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
	return YCPBoolean(false);

    repo->repoInfo().setAutorefresh(e->value());

    return YCPBoolean( true );
}

/****************************************************************************************
 * @builtin SourceEditSet
 *
 * @short Configure properties of installation sources
 * @description
 * Set states of installation sources. Note: Enabling/disabling a source does not
 * (un)load the packages from the source! Use SourceSetEnabled() if you need to refresh
 * the packages in the pool.
 *
 * @param list source_states List of source states. Same format as returned by
 * @see SourceEditGet.
 *
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceEditSet (const YCPList& states)
{
  bool error = false;

  for (int index = 0; index < states->size(); index++ )
  {
    if( ! states->value(index)->isMap() )
    {
	ycperror( "Pkg::SourceEditSet, entry not a map at index %d", index);
	error = true;
	continue;
    }

    YCPMap descr = states->value(index)->asMap();

    if (descr->value( YCPString("SrcId") ).isNull() || !descr->value(YCPString("SrcId"))->isInteger())
    {
	ycperror( "Pkg::SourceEditSet, SrcId not defined for a source description at index %d", index);
	error = true;
	continue;
    }

    std::vector<YRepo_Ptr>::size_type id = descr->value( YCPString("SrcId") )->asInteger()->value();

    YRepo_Ptr repo = logFindRepository(id);
    if (!repo)
    {
	ycperror( "Pkg::SourceEditSet, source %d not found", index);
	error = true;
	continue;
    }

    // now, we have the source
    if( ! descr->value( YCPString("enabled")).isNull() && descr->value(YCPString("enabled"))->isBoolean ())
    {
	bool enable = descr->value(YCPString("enabled"))->asBoolean ()->value();

	if (repo->repoInfo().enabled() != enable)
	{
	    ycpwarning("Pkg::SourceEditSet() does not refresh the pool (src: %zd, state: %s)", id, enable ? "disabled -> enabled" : "enabled -> disabled");
	}

        y2debug("set enabled: %d", enable);
	repo->repoInfo().setEnabled(enable);
    }

    if( !descr->value(YCPString("autorefresh")).isNull() && descr->value(YCPString("autorefresh"))->isBoolean ())
    {
        bool autorefresh = descr->value(YCPString("autorefresh"))->asBoolean()->value();
        y2debug("set autorefresh: %d", autorefresh);
	repo->repoInfo().setAutorefresh( autorefresh );
    }

    if( !descr->value(YCPString("name")).isNull() && descr->value(YCPString("name"))->isString())
    {
	// rename the source
        y2debug("set name: %s", descr->value(YCPString("name"))->asString()->value().c_str());
	repo->repoInfo().setName(descr->value(YCPString("name"))->asString()->value());
    }

#warning SourceEditSet ordering not implemented yet
  }

  PkgFreshen();
  return YCPBoolean( !error );
}

/****************************************************************************************
 * @builtin SourceChangeUrl
 * @short Change Source URL
 * @description
 * Change url of an InstSrc. Used primarely when re-starting during installation
 * and a cd-device changed from hdX to srX since ide-scsi was activated.
 * @param integer SrcId Specifies the InstSrc.
 * @param string url The new url to use.
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceChangeUrl (const YCPInteger& id, const YCPString& u)
{
    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
	return YCPBoolean(false);

    try
    {
        if (repo->repoInfo().baseUrlsSize() > 1)
        {
            // store current urls
            std::set<zypp::Url> baseUrls (repo->repoInfo().baseUrlsBegin(), repo->repoInfo().baseUrlsEnd());
            
            // reset url list and store the new one there
            repo->repoInfo().setBaseUrl(zypp::Url(u->value()));

            // add the rest of base urls
            for (std::set<zypp::Url>::const_iterator i = baseUrls.begin();
                    i != baseUrls.end(); ++i)
                repo->repoInfo().addBaseUrl(*i);
        }
        else
            repo->repoInfo().setBaseUrl(zypp::Url(u->value()));
    }
    catch (const zypp::Exception & excpt)
    {
        _last_error.setLastError(ExceptionAsString(excpt));
        y2error ("Cannot set the new URL for source %s (%lld): %s",
            repo->repoInfo().alias().c_str(), id->asInteger()->value(), excpt.msg().c_str());
	return YCPBoolean(false);
    }

    return YCPBoolean(true);
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// DEPRECATED
//
/////////////////////////////////////////////////////////////////////////////////////////

/****************************************************************************************
 * Pkg::SourceRaisePriority (integer SrcId) -> bool
 *
 * Raise priority of source.
 *
 * @param SrcId Specifies the InstSrc.
 *
 * @return bool
 **/
YCPValue
PkgModuleFunctions::SourceRaisePriority (const YCPInteger& id)
{
#warning SourceRaisePriority is not implemented
    y2warning("SourceRaisePriority is NOT implemented");
/*    zypp::Source_Ref src;

    try {
	src = logFindRepository(id->value());
    }
    catch (const zypp::Exception& excpt)
    {
	return YCPBoolean(false);
    }

    // raise priority by one
    src.setPriority(src.priority() + 1);
*/

    return YCPBoolean( true );

}

/****************************************************************************************
 * Pkg::SourceLowerPriority (integer SrcId) -> void
 *
 * Lower priority of source.
 *
 * @param SrcId Specifies the InstSrc.
 *
 * @return bool
 */
YCPValue
PkgModuleFunctions::SourceLowerPriority (const YCPInteger& id)
{
#warning SourceLowerPriority is not implemented
    y2warning("SourceLowerPriority is NOT implemented");
/*
    zypp::Source_Ref src;

    try {
	src = logFindRepository(id->value());
    }
    catch (const zypp::Exception& excpt)
    {
	return YCPBoolean(false);
    }

    // lower priority by one
    src.setPriority(src.priority() - 1);
*/
    return YCPBoolean( true );
}

/****************************************************************************************
 * Pkg::SourceSaveRanks () -> boolean
 *
 * @short Obsoleted function, do not use
 * @return boolean true
 **/
YCPValue
PkgModuleFunctions::SourceSaveRanks ()
{
  y2error( "SourceSaveRanks not implemented" );

  return YCPBoolean( true );
}

/****************************************************************************************
 * @builtin SourceInstallOrder
 *
 * @short not implemented, do not use (Explicitly set an install order.)
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceInstallOrder (const YCPMap& ord)
{
#warning SourceInstallOrder is not implemented
  return YCPBoolean( true );
}


/****************************************************************************************
 * @builtin SourceDelete
 * @short Delete a Source
 * @description
 * Delete an InsrSrc. The InsrSrc together with all metadata cached on disk
 * is removed. The SrcId passed becomes invalid (other SrcIds stay valid).
 *
 * @param integer SrcId Specifies the InstSrc.
 *
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceDelete (const YCPInteger& id)
{
    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
	return YCPBoolean(false);

    bool success = true;

    try
    {
	// the resolvables cannot be used anymore, remove them
	RemoveResolvablesFrom(repo->repoInfo().alias());

	// update 'repos'
	repo->setDeleted();

	PkgFreshen();
    }
    catch (const zypp::Exception& excpt)
    {
	std::string alias = repo->repoInfo().alias();
	y2error ("Error for '%s': %s", alias.c_str(), excpt.asString().c_str());
	_last_error.setLastError(alias + ": " + ExceptionAsString(excpt));
	success = false;
    }

    return YCPBoolean(success);
}


