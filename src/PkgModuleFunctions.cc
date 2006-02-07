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

#include <zypp/SourceManager.h>
#include <zypp/ZYppFactory.h>
#include <zypp/ResPool.h>

class Y2PkgFunction: public Y2Function 
{
    unsigned int m_position;
    PkgModuleFunctions* m_instance;
    YCPValue m_param1;
    YCPValue m_param2;
    YCPValue m_param3;
    YCPValue m_param4;
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
	y2internal ("appendParameter > 3 not implemented");
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
	switch (m_position) {
#include "PkgBuiltinCalls.h"
	}
	
	return YCPNull ();
    }
    
    bool Y2PkgFunction::reset ()
    {
	m_param1 = YCPNull ();
	m_param2 = YCPNull ();
	m_param3 = YCPNull ();
	m_param4 = YCPNull ();

	return true;
    }

    string Y2PkgFunction::name () const
    {
	    return m_name;
    }

/**
 * Constructor.
 */
PkgModuleFunctions::PkgModuleFunctions ()
    : Y2Namespace()
    ,_callbackHandler( *new CallbackHandler( ) )
    , _target_root( "/" )
{
  zypp::ZYppFactory factory;

  zypp_ptr = factory.getZYpp();

  registerFunctions ();
}

/**
 * Destructor.
 */
PkgModuleFunctions::~PkgModuleFunctions ()
{
    SourceFinishAll ();
    TargetFinish ();
    delete &_callbackHandler;
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
 * @builtin InstSysMode
 * @short Set packagemanager to "inst-sys" mode
 * @description Set packagemanager to "inst-sys" mode - dont use local caches (ramdisk!)
 * FIXME ? <b>CAUTION</b>:Can only be called ONCE  MUST be called before any other function
 * @return void
 */
YCPValue
PkgModuleFunctions::InstSysMode ()
{
    // TODO FIXME _y2pm.setNotRunningFromSystem();
    return YCPVoid();
}


/**
 * @builtin SetLocale 
 * @short Set Prefered Locale
 * @description
 * set the given locale as the "preferred" locale
 * @param string locale Locale
 * @return void
 */
YCPValue
PkgModuleFunctions::SetLocale (const YCPString &locale)
{
// TODO FIXME	LangCode langcode(locale->value());
//		_y2pm.setPreferredLocale (langcode);
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
    // TODO FIXME
    // return YCPString (_y2pm.getPreferredLocale().code());
    return YCPString("en_US");
}


/**
 * @builtin SetAdditionalLocales
 *
 * @short set list of additional locales
 * @param list<string> locales List of additional locales
 * @return void
 * @usage Pkg::SetAdditionalLocales([de_DE]);
 */
YCPValue
PkgModuleFunctions::SetAdditionalLocales (YCPList langycplist)
{
/*  TODO FIXME
    Y2PM::LocaleSet langcodelist;
    int i = 0;
    while (i < langycplist->size())
    {
	if (langycplist->value (i)->isString())
	{
	    langcodelist.insert (LangCode (langycplist->value (i)->asString()->value()));
	}
	else
	{
	    y2error ("Pkg::SetAdditionalLocales ([...,%s,...]) not string", langycplist->value (i)->toString().c_str());
	}
	i++;
    }
    _y2pm.setRequestedLocales (langcodelist);
*/
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
/*  TODO FIXME
    const Y2PM::LocaleSet & langcodelist = _y2pm.getRequestedLocales();
    for (Y2PM::LocaleSet::const_iterator it = langcodelist.begin();
	 it != langcodelist.end(); ++it)
    {
      langycplist->add (YCPString (it->code()));
    }
*/
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
    // TODO FIXME
    return YCPString("");
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
    // TODO FIXME
    return YCPString ("");
}

/**
 * @builtin LastErrorId
 * @short get current error as id string
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

