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


#include "Y2PkgFunction.h"

#include <ycp/YCPBoolean.h>
#include <ycp/YCPValue.h>
#include <ycp/YCPList.h>
#include <ycp/YCPMap.h>
#include <ycp/YCPSymbol.h>
#include <ycp/YCPString.h>
#include <ycp/YCPInteger.h>
#include <ycp/YCPVoid.h>

#include <ycp/y2log.h>

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
