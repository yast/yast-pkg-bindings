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
   Summary:     Functions related to error handling
   Namespace:   Pkg
*/

#include "PkgFunctions.h"
#include "log.h"

#include "Callbacks.h"

#include <ycp/YCPInteger.h>
#include <ycp/YCPString.h>
#include <ycp/YCPBoolean.h>
#include <ycp/YCPMap.h>
#include <ycp/YCPVoid.h>

#include <zypp/ZYppFactory.h>

// sleep
#include <unistd.h>

// textdomain
#include <libintl.h>                                                                                                                                           

/////////////////////////////////////////////////////////////////////////////


const zypp::ResStatus::TransactByValue PkgFunctions::whoWantsIt = zypp::ResStatus::APPL_HIGH;

/**
 * Constructor.
 */
PkgFunctions::PkgFunctions () :
      _target_root( "/" )
    , _target_loaded(false)
    , zypp_pointer(NULL)
    ,_callbackHandler( *new CallbackHandler(*this) )
    ,target_log_set(false)
{
    const char *domain = "pkg-bindings";
    bindtextdomain( domain, LOCALEDIR );
    bind_textdomain_codeset( domain, "utf8" );
    textdomain( domain );

    // Make change known.
    {
	extern int _nl_msg_cat_cntr;
	++_nl_msg_cat_cntr;
    }
}

/**
 * Connect to Zypp library if it is not already connected.
 */
zypp::ZYpp::Ptr PkgFunctions::zypp_ptr()
{
    if (zypp_pointer != NULL)
    {
	return zypp_pointer;
    }

    int max_count = 5;
    unsigned int seconds = 3;

    while (zypp_pointer == NULL && max_count > 0)
    {
	try
	{
	    y2milestone("Initializing Zypp library...");
	    zypp_pointer = zypp::getZYpp();
	    return zypp_pointer;
	}
	catch (...)
	{
	}

	max_count--;

	if (zypp_pointer == NULL && max_count > 0)
	{
	    sleep(seconds);
	}
    }

    // still not initialized, throw an exception
    ZYPP_THROW (zypp::Exception(std::string("Cannot connect to the package manager")));

    return zypp_pointer;
}


/**
 * Destructor.
 */
PkgFunctions::~PkgFunctions ()
{
    delete &_callbackHandler;

    if (zypp_pointer != NULL)
    {
	y2milestone("Releasing the zypp pointer...");
	zypp_pointer = NULL;
	y2milestone("Zypp pointer released");
    }
}

// ------------------------------------------------------------------
// general

/**
 * @builtin Connect
 * @short Explicitly connect to the package manager, if it is already connected do nothing
 * @description Checks whether the package manager is connected to Yast,
 * if not try connecting to it.
 * @return boolean true if the package manager is connected
 */
YCPValue
PkgFunctions::Connect()
{
    try
    {
	return YCPBoolean(zypp_ptr() != NULL);
    }
    catch(...)
    {
    }

    return YCPBoolean(false);
}

/**
 * @builtin LastError
 *
 * @short get current error as string
 * @return string
 */
YCPValue
PkgFunctions::LastError ()
{
    return YCPString(_last_error.lastError());
}

/**
 * @builtin LastErrorDetails
 *
 * @short get current error details as string
 * @return string Error Details
 */
YCPValue
PkgFunctions::LastErrorDetails ()
{
    return YCPString (_last_error.lastErrorDetails());
}

/**
 * @builtin LastErrorId
 * @short Obsoleted function, do not use
 * @return string
 */
YCPValue
PkgFunctions::LastErrorId ()
{

    /* TODO FIXME
    int errorId = _last_error;
    switch ( errorId ) {
        case PMError::E_ok:
            return YCPString( "ok" );
        case InstSrcError::E_isrc_cache_duplicate:
            return YCPString( "instsrc_duplicate" );
        default:
            return YCPString( "error" );
    }
    */

    return YCPString( "ok" );
}

/**
 * @builtin Init
 * @short completely initialize package management (currently it is empty)
 * @return boolean true on success
 */
YCPValue
PkgFunctions::Init ()
{
#warning  FIXME can be Init() empty??
    return YCPBoolean(true);
}

zypp::RepoManager PkgFunctions::CreateRepoManager()
{
    // set path option, use root dir as a prefix for the default directory
    zypp::RepoManagerOptions repo_options(_target_root);

    y2milestone("Path to repository files: %s", repo_options.knownReposPath.asString().c_str());

    return zypp::RepoManager(repo_options);
}

// convert Exception object to string represenatation
std::string PkgFunctions::ExceptionAsString(const zypp::Exception &e)
{
    std::string ret = e.asUserString();

    if (e.historySize() > 0)
    {
	ret += "\n" + e.historyAsString();
    }

    y2debug("Error message: %s", ret.c_str());

    return ret;
}

