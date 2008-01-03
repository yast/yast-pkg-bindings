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
   Summary:	PkgModulesFunctions constructor, destructor and error handling

/-*/


#include "PkgModuleFunctions.h"
#include "Y2PkgFunction.h"

#include <y2/Y2Function.h>
#include <ycp/YCPVoid.h>
#include "log.h"

/////////////////////////////////////////////////////////////////////////////


/**
 * Constructor.
 */
PkgModuleFunctions::PkgModuleFunctions ()
    : Y2Namespace()
{
    registerFunctions ();
}

/**
 * Destructor.
 */
PkgModuleFunctions::~PkgModuleFunctions()
{
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

    return new Y2PkgFunction (name, &pkg_functions, it - _registered_functions.begin ());
}

YCPValue PkgModuleFunctions::evaluate(bool cse)
{
    if (cse) return YCPNull ();
    else return YCPVoid ();
}

void PkgModuleFunctions::registerFunctions()
{
#include "PkgBuiltinTable.h"
}

