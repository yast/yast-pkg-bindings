/*---------------------------------------------------------------------\
|								       |
|		       __   __	  ____ _____ ____		       |
|		       \ \ / /_ _/ ___|_   _|___ \		       |
|			\ V / _` \___ \ | |   __) |		       |
|			 | | (_| |___) || |  / __/		       |
|			 |_|\__,_|____/ |_| |_____|		       |
|								       |
|				core system			       |
|							 (C) SuSE GmbH |
\----------------------------------------------------------------------/

   File:	PkgModuleFunctions.cc

   Author:	Klaus Kaempf <kkaempf@suse.de>
		Stanislav Visnovsky <visnov@suse.cz>
   Maintainer:  Klaus Kaempf <kkaempf@suse.de>
   Namespace:    Pkg
   Summary:	PkgModuleFunctions constructor, destructor and error handling

/-*/


#include <y2/Y2Function.h>
#include "PkgModuleFunctions.h"

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

#include "Y2PkgFunction.h"


/////////////////////////////////////////////////////////////////////////////


const zypp::ResStatus::TransactByValue PkgModuleFunctions::whoWantsIt = zypp::ResStatus::APPL_HIGH;

/**
 * Constructor.
 */
PkgModuleFunctions::PkgModuleFunctions ()
    : Y2Namespace()
    , _target_root( "/" )
    , zypp_pointer(NULL)
    ,_callbackHandler( *new CallbackHandler(*this) )
{
    registerFunctions ();

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
zypp::ZYpp::Ptr PkgModuleFunctions::zypp_ptr()
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
PkgModuleFunctions::~PkgModuleFunctions ()
{
    delete &_callbackHandler;

    if (zypp_pointer != NULL)
    {
	try
	{
	    y2milestone("Finishing the target");
	    zypp_pointer->finishTarget();
	}
	catch(...)
	{
	    y2error("finishTarget() has failed");
	}

	y2milestone("Releasing the zypp pointer...");
	zypp_pointer = NULL;
	y2milestone("Zypp pointer released");
    }
}

Y2Function* PkgModuleFunctions::createFunctionCall (const string name, constFunctionTypePtr type)
{
    vector<string>::iterator it = find (_registered_functions.begin ()
	, _registered_functions.end (), name);
    if (it == _registered_functions.end ())
    {
	y2error ("No such function %s", name.c_str ());
	return NULL;
    }

    return new Y2PkgFunction (name, this, it - _registered_functions.begin ());
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
PkgModuleFunctions::Connect()
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
PkgModuleFunctions::LastError ()
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
PkgModuleFunctions::LastErrorDetails ()
{
    return YCPString (_last_error.lastErrorDetails());
}

/**
 * @builtin LastErrorId
 * @short Obsoleted function, do not use
 * @return string
 */
YCPValue
PkgModuleFunctions::LastErrorId ()
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
PkgModuleFunctions::Init ()
{
#warning  FIXME can be Init() empty??
    return YCPBoolean(true);
}

zypp::RepoManager PkgModuleFunctions::CreateRepoManager()
{
    // set path option, use root dir as a prefix for the default directory
    zypp::RepoManagerOptions repo_options;
    repo_options.knownReposPath = zypp::Pathname(_target_root) + repo_options.knownReposPath;

    y2milestone("Path to repository files: %s", repo_options.knownReposPath.asString().c_str());

    return zypp::RepoManager(repo_options);
}

// convert Exception object to string represenatation
std::string PkgModuleFunctions::ExceptionAsString(const zypp::Exception &e)
{
    std::string ret = e.asUserString();

    if (e.historySize() > 0)
    {
	ret += "\n" + e.historyAsString();
    }

    y2debug("Error message: %s", ret.c_str());

    return ret;
}


YCPValue PkgModuleFunctions::evaluate (bool cse)
{
    if (cse) return YCPNull ();
    else return YCPVoid ();
}
