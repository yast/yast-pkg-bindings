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
   Namespace:   Pkg
*/

#include <Callbacks.h>
#include <Callbacks.YCP.h>

#include <PkgFunctions.h>
#include "log.h"

#include <PkgProgress.h>
#include <HelpTexts.h>

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
PkgFunctions::SourceSetEnabled (const YCPInteger& id, const YCPBoolean& e)
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
	    // load resolvables only when they haven't been loaded yet
	    if (!repo->isLoaded())
	    {
		std::list<std::string> stages;
		stages.push_back(_("Load Data"));

		PkgProgress pkgprogress(_callbackHandler);
		zypp::ProgressData prog_total(100);
		prog_total.sendTo(pkgprogress.Receiver());
		zypp::CombinedProgressData load_subprogress(prog_total, 100);

		pkgprogress.Start(_("Loading the Package Manager..."), stages, _(HelpTexts::load_resolvables));

		success = LoadResolvablesFrom(repo, load_subprogress);
	    }
	}
	else
	{
	    // the source has been disabled, remove resolvables from the pool
	    RemoveResolvablesFrom(repo);
	}
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
PkgFunctions::SourceSetAutorefresh (const YCPInteger& id, const YCPBoolean& e)
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
PkgFunctions::SourceEditSet (const YCPList& states)
{
  bool error = false;

  for (int index = 0; index < states->size(); index++ )
  {
    if( ! states->value(index)->isMap() )
    {
	y2error( "Pkg::SourceEditSet, entry not a map at index %d", index);
	error = true;
	continue;
    }

    YCPMap descr = states->value(index)->asMap();

    if (descr->value( YCPString("SrcId") ).isNull() || !descr->value(YCPString("SrcId"))->isInteger())
    {
	y2error( "Pkg::SourceEditSet, SrcId not defined for a source description at index %d", index);
	error = true;
	continue;
    }

    RepoId id = descr->value( YCPString("SrcId") )->asInteger()->value();

    YRepo_Ptr repo = logFindRepository(id);
    if (!repo)
    {
	y2error( "Pkg::SourceEditSet, source %d not found", index);
	error = true;
	continue;
    }

    // now, we have the source
    if( ! descr->value( YCPString("enabled")).isNull() && descr->value(YCPString("enabled"))->isBoolean ())
    {
	bool enable = descr->value(YCPString("enabled"))->asBoolean ()->value();

	if (repo->repoInfo().enabled() != enable)
	{
	    y2warning("Pkg::SourceEditSet() does not refresh the pool (src: %lld, state: %s)", id, enable ? "disabled -> enabled" : "enabled -> disabled");
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

    if( !descr->value(YCPString("priority")).isNull() && descr->value(YCPString("priority"))->isInteger())
    {
	unsigned int priority = descr->value(YCPString("priority"))->asInteger()->value();

	// set the priority
	repo->repoInfo().setPriority(priority);
	y2debug("set priority: %d", priority);
    }

    if(!descr->value(YCPString("keeppackages")).isNull() && descr->value(YCPString("keeppackages"))->isBoolean())
    {
        bool keeppackages = descr->value(YCPString("keeppackages"))->asBoolean()->value();
        y2debug("set keeppackages: %d", keeppackages);
	repo->repoInfo().setKeepPackages( keeppackages );
    }
  }

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
PkgFunctions::SourceChangeUrl (const YCPInteger& id, const YCPString& u)
{
    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
	return YCPBoolean(false);

    try
    {
        if (repo->repoInfo().baseUrlsSize() > 1)
        {
            // store current urls
	    const zypp::RepoInfo::url_set & baseUrls(( repo->repoInfo().baseUrls() ));

            // reset url list and store the new one there
            repo->repoInfo().setBaseUrl(zypp::Url(u->value()));

            // add the rest of base urls
            for ( const auto & url : baseUrls )
                repo->repoInfo().addBaseUrl(url);
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
PkgFunctions::SourceRaisePriority (const YCPInteger& id)
{
    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
	return YCPBoolean(false);

    // check the current priority
    unsigned int prio = repo->repoInfo().priority();

    // maximum priority already reached - priority is 1 (max.) to 99 (min.)
    if (prio <= 1)
    {
	return YCPBoolean(false);
    }
    else
    {
	// increase the priority
	prio--;

	// set the new priority
	repo->repoInfo().setPriority(prio);
    }

    return YCPBoolean(true);
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
PkgFunctions::SourceLowerPriority (const YCPInteger& id)
{
    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
	return YCPBoolean(false);

    // check the current priority
    unsigned int prio = repo->repoInfo().priority();

    // manimum priority already reached - priority is 1 (max.) to 99 (min.)
    if (prio >= 99)
    {
	return YCPBoolean(false);
    }
    else
    {
	// decrease the priority
	prio++;

	// set the new priority
	repo->repoInfo().setPriority(prio);
    }

    return YCPBoolean(true);
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
PkgFunctions::SourceDelete (const YCPInteger& id)
{
    YRepo_Ptr repo = logFindRepository(id->value());
    if (!repo)
	return YCPBoolean(false);

    bool success = true;
    const std::string repo_alias(repo->repoInfo().alias());

    try
    {

	// the resolvables cannot be used anymore, remove them
	RemoveResolvablesFrom(repo);

	// update 'repos'
	repo->setDeleted();

	// removing the base product repository?
	if (base_product && base_product->repo_alias == repo_alias)
	{
	    y2warning("Resetting the base product, the base product repository has been removed");
            delete base_product;
	    base_product = NULL;
	}
    }
    catch (const zypp::Exception& excpt)
    {
	y2error ("Error for '%s': %s", repo_alias.c_str(), excpt.asString().c_str());
	_last_error.setLastError(repo_alias + ": " + ExceptionAsString(excpt));
	success = false;
    }

    return YCPBoolean(success);
}


