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
   Summary:	PkgModuleFunctions constructor, destructor and call handling

/-*/


#include <ycp/y2log.h>
#include <ycp/YExpression.h>
#include <ycp/YBlock.h>
#include "PkgModuleFunctions.h"

#include "Callbacks.h"

#include <ycp/YCPInteger.h>
#include <ycp/YCPString.h>
#include <ycp/YCPBoolean.h>
#include <ycp/YCPMap.h>
#include <ycp/YCPVoid.h>

#include <zypp/ZYppFactory.h>
#include <zypp/ResPool.h>
#include <zypp/RepoInfo.h>
#include <zypp/MediaSetAccess.h>

// sleep
#include <unistd.h>

// exit
#include <cstdlib>

// textdomain
#include <libintl.h>                                                                                                                                           

class Y2PkgFunction: public Y2Function
{
    unsigned int m_position;
    PkgModuleFunctions* m_instance;
    YCPValue m_param1;
    YCPValue m_param2;
    YCPValue m_param3;
    YCPValue m_param4;
    YCPValue m_param5;
    string m_name;
public:

    Y2PkgFunction (string name, PkgModuleFunctions* instance, unsigned int pos);
    bool attachParameter (const YCPValue& arg, const int position);
    constTypePtr wantedParameterType () const;
    bool appendParameter (const YCPValue& arg);
    bool finishParameters ();
    YCPValue evaluateCall ();
    bool reset ();
    string name () const;
};


    Y2PkgFunction::Y2PkgFunction (string name, PkgModuleFunctions* instance, unsigned int pos) :
	m_position (pos)
	, m_instance (instance)
	, m_param1 ( YCPNull () )
	, m_param2 ( YCPNull () )
	, m_param3 ( YCPNull () )
	, m_param4 ( YCPNull () )
	, m_param5 ( YCPNull () )
	, m_name (name)
    {
    };

    bool Y2PkgFunction::attachParameter (const YCPValue& arg, const int position)
    {
	switch (position)
	{
	    case 0: m_param1 = arg; break;
	    case 1: m_param2 = arg; break;
	    case 2: m_param3 = arg; break;
	    case 3: m_param4 = arg; break;
	    case 4: m_param5 = arg; break;
	    default: return false;
	}

	return true;
    }

    constTypePtr Y2PkgFunction::wantedParameterType () const
    {
	y2internal ("wantedParameterType not implemented");
	return Type::Unspec;
    }

    bool Y2PkgFunction::appendParameter (const YCPValue& arg)
    {
	if (m_param1.isNull ())
	{
	    m_param1 = arg;
	    return true;
	} else if (m_param2.isNull ())
	{
	    m_param2 = arg;
	    return true;
	} else if (m_param3.isNull ())
	{
	    m_param3 = arg;
	    return true;
	} else if (m_param4.isNull ())
	{
	    m_param4 = arg;
	    return true;
	}
	else if (m_param5.isNull ())
	{
	    m_param5 = arg;
	    return true;
	}
	y2internal ("appendParameter > 5 not implemented");
	return false;
    }

    bool Y2PkgFunction::finishParameters ()
    {
	y2internal ("finishParameters not implemented");
	return true;
    }

    YCPValue Y2PkgFunction::evaluateCall ()
    {
	ycpmilestone ("Pkg Builtin called: %s", name().c_str() );

	try
	{
	    switch (m_position) {
#include "PkgBuiltinCalls.h"
	    }
	}
	catch (const std::exception& excpt)
	{
	    y2internal("Caught an unhandled exception: %s", excpt.what());
	}
	catch (...)
	{
	    y2internal("Caught an unhandled exception");
	}

	return YCPNull ();
    }

    bool Y2PkgFunction::reset ()
    {
	m_param1 = YCPNull ();
	m_param2 = YCPNull ();
	m_param3 = YCPNull ();
	m_param4 = YCPNull ();
	m_param5 = YCPNull ();

	return true;
    }

    string Y2PkgFunction::name () const
    {
	    return m_name;
    }

/////////////////////////////////////////////////////////////////////////////
//
// Class: YRepo
//

IMPL_PTR_TYPE(YRepo);

YRepo::YRepo(zypp::RepoInfo & repo)
    : _repo(repo), _deleted(false)
{}

YRepo::~YRepo()
{
    if (_maccess)
    {
        try { _maccess->release(); }
        catch (const zypp::media::MediaException & ex)
	{
	    y2error("Error in ~Yrepo(): %s", ex.asString().c_str());
	}
    }
}

zypp::MediaSetAccess_Ptr & YRepo::mediaAccess()
{
    if (!_maccess)
    {
        y2milestone("Creating new MediaSetAccess for url %s",
            (*_repo.baseUrlsBegin()).asString().c_str());
        _maccess = new zypp::MediaSetAccess(*_repo.baseUrlsBegin()); // FIXME handle multiple baseUrls
    }

    return _maccess;
}

const YRepo YRepo::NOREPO;

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

void PkgModuleFunctions::registerFunctions()
{
#include "PkgBuiltinTable.h"
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

/**
 * @builtin SetTextLocale
 * @short Set Package Manager Locale
 * @description
 * Set the given locale as the output locale -- all messages from the package manager (errors, warnings,...)
 * will be returned in the selected language. This built-in does not change package selection in any way.
 * @param string locale Locale
 * @return void
 */
YCPValue
PkgModuleFunctions::SetTextLocale (const YCPString &locale)
{
    try
    {
	zypp::Locale loc = zypp::Locale(locale->value());
	zypp_ptr()->setTextLocale(loc);
    }
    catch (const std::exception& excpt)
    {
	y2error("Caught an exception: %s", excpt.what());
    }
    catch (...)
    {
	y2internal("Caught an unknown exception");
    }

    return YCPVoid();
}

/**
 * @builtin SetPackageLocale
 * @short Select locale for installation
 * @description
 * Select the main locale for installation, call Pkg::PkgSolve() to select the respective packages.
 * @param string locale Locale
 * @return void
 */
YCPValue
PkgModuleFunctions::SetPackageLocale (const YCPString &locale)
{
    try
    {
	zypp::Locale loc = zypp::Locale(locale->value());

	// add packages for the preferred locale, preserve additional locales
	zypp::ZYpp::LocaleSet lset = zypp_ptr()->getRequestedLocales();

	// remove the previous locale
	if (preferred_locale != zypp::Locale::noCode)
	{
	    lset.erase(preferred_locale);
	}

	// add the new locale
	lset.insert(loc);
	zypp_ptr()->setRequestedLocales(lset);

	// remember the main locale
	preferred_locale = loc;
    }
    catch(...)
    {
    }

    return YCPVoid();
}

/**
 * @builtin SetLocale
 * @short Set The Main (Preferred) Locale -- OBSOLETED!
 * @description
 * OBSOLETED, DO NOT USE! It has been replaced by SetTextLocale() and SetMainLocale() calls (see bug #223624)
 * @param string locale Locale
 * @return void
 */
YCPValue
PkgModuleFunctions::SetLocale (const YCPString &locale)
{
    y2warning("Pkg::SetLocale() is obsoleted, use Pkg::SetTextLocale() and/or Pkg::SetPackageLocale() instead. Pkg::SetLocale() currently calls both functions");

    SetTextLocale(locale);
    SetPackageLocale(locale);

    return YCPVoid();
}

/**
 * @builtin GetTextLocale
 * @short get the currently preferred locale
 * @return string locale
 * @usage Pkg::GetTextLocale() -> "en_US"
 */
YCPValue
PkgModuleFunctions::GetTextLocale ()
{
    try
    {
	return YCPString(zypp_ptr()->getTextLocale().code());
    }
    catch (...)
    {
    }

    return YCPVoid();
}

/**
 * @builtin GetLocale
 * @short get the currently preferred locale
 * @return string locale
 * @usage Pkg::GetLocale () -> "en_US"
 */
YCPValue
PkgModuleFunctions::GetLocale ()
{
    y2warning("Pkg::GetLocale() is obsoleted, use Pkg::GetTextLocale() or Pkg::GetPackageLocale() instead. Pkg::GetLocale() currently calls Pkg::GetTextLocale()");
    return GetTextLocale();
}

/**
 * @builtin GetPackageLocale
 * @short get the locale set by Pkg::SetPackageLocale() call
 * @return string locale
 * @usage Pkg::GetPackageLocale () -> "en_US"
 */
YCPValue
PkgModuleFunctions::GetPackageLocale ()
{
    // the locale hasn't been initialized
    if (preferred_locale == zypp::Locale::noCode)
    {
	y2warning("The package locale hasn't been set, call Pkg::SetPackageLocale() before Pkg::GetPackageLocale()");
    }

    return YCPString(preferred_locale.code());
}

/**
 * @builtin SetAdditionalLocales
 *
 * @short set list of additional locales
 * @description
 * Select additional languages for installation. Call Pkg::Solve() to select the respective packages.
 * @param list<string> locales List of additional locales
 * @return void
 * @usage Pkg::SetAdditionalLocales(["de_DE"]);
 */
YCPValue
PkgModuleFunctions::SetAdditionalLocales (YCPList langycplist)
{
    zypp::ZYpp::LocaleSet lset;

    int i = 0;
    while (i < langycplist->size())
    {
	if (langycplist->value(i)->isString())
	{
	    lset.insert(zypp::Locale(langycplist->value(i)->asString()->value()));
	}
	else
	{
	    y2error("Pkg::SetAdditionalLocales ([...,%s,...]) not string", langycplist->value(i)->toString().c_str());
	}
	i++;
    }

    // add the main locale if it's initialized
    if (preferred_locale != zypp::Locale::noCode)
    {
	lset.insert(preferred_locale);
    }

    try
    {
	zypp_ptr()->setRequestedLocales(lset);
    }
    catch(...)
    {
    }

    return YCPVoid();
}

/**
 * @builtin GetAdditionalLocales
 *
 * @short return list of additional locales
 * @return list<string>
 * @usage Pkg::GetAdditionalLocales() -> ["de_DE"];
 *
 */
YCPValue
PkgModuleFunctions::GetAdditionalLocales ()
{
    YCPList langycplist;

    try
    {
	zypp::ZYpp::LocaleSet lset = zypp_ptr()->getRequestedLocales();

	for (zypp::ZYpp::LocaleSet::const_iterator it = lset.begin();
	     it != lset.end(); ++it)
	{
	  // ignore the main locale
	  if (*it != preferred_locale)
	  {
	    langycplist->add (YCPString(it->code()));
	  }
	}
    }
    catch(...)
    {
    }

    return langycplist;
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


/** ------------------------
 * Convert InstSrcDescr to product info YCPMap:
 * <TABLE>
 * <TR><TD>$[<TD>"product"             <TD>: YCPString (name' 'version)
 * <TR><TD>,<TD>"vendor"               <TD>: YCPString
 * <TR><TD>,<TD>"requires"             <TD>: YCPString
 * <TR><TD>,<TD>"name"                 <TD>: YCPString
 * <TR><TD>,<TD>"version"              <TD>: YCPString
 * <TR><TD>,<TD>"flags"                        <TD>: YCPString
 * <TR><TD>,<TD>"relnotesurl"          <TD>: YCPString
 * <TR><TD>,<TD>"distproduct"          <TD>: YCPString
 * <TR><TD>,<TD>"distversion"          <TD>: YCPString
 * <TR><TD>,<TD>"baseproduct"          <TD>: YCPString
 * <TR><TD>,<TD>"baseversion"          <TD>: YCPString
 * <TR><TD>];
 * </TABLE>


YCPMap
PkgModuleFunctions::Descr2Map (constInstSrcDescrPtr descr)
{
    YCPMap map;

    map->add (YCPString ("product"),  YCPString (descr->content_product().asPkgNameEd().name.asString()
						  + " "
						  + descr->content_product().asPkgNameEd().edition.version()));
    map->add (YCPString ("vendor"), YCPString (descr->content_vendor()));
    map->add (YCPString ("requires"), YCPString (descr->content_requires().asString()));

    // for installation/modules/Product.ycp
    map->add (YCPString ("name"),        YCPString (descr->content_product().asPkgNameEd().name));
    map->add (YCPString ("version"), YCPString (descr->content_product().asPkgNameEd().edition.version()));
    map->add (YCPString ("flags"), YCPString (descr->content_flags()));
    map->add (YCPString ("relnotesurl"), YCPString (descr->content_relnotesurl()));

    // vendor already in map

    map->add (YCPString ("distproduct"), YCPString (descr->content_distproduct().name));
    map->add (YCPString ("distversion"), YCPString (descr->content_distproduct().edition.version()));

    map->add (YCPString ("baseproduct"), YCPString (descr->content_baseproduct().asPkgNameEd().name));
    map->add (YCPString ("baseversion"), YCPString (descr->content_baseproduct().asPkgNameEd().edition.version()));

    map->add (YCPString ("defaultbase"), YCPString (descr->content_defaultbase()));

    return map;
}
*/

