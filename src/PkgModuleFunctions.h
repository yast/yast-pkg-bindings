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

   File:	PkgModuleFunctions.h

   Author:	Klaus Kaempf <kkaempf@suse.de>
		Stanislav Visnovsky <visnov@suse.cz>
   Maintainer:  Klaus Kaempf <kkaempf@suse.de>

   Purpose:	Access to packagemanager
		Handles Pkg::function (list_of_arguments) calls
/-*/

#ifndef PkgModuleFunctions_h
#define PkgModuleFunctions_h

#include <string>
#include <y2/Y2Namespace.h>
#include "PkgFunctions.h"

/**
 * A simple class for package management access
 */
class PkgModuleFunctions : public Y2Namespace
{
    public:
	/**
	 * Constructor.
	 */
	PkgModuleFunctions ();
      
	/**
	 * Destructor.
	 */
	virtual ~PkgModuleFunctions ();

	virtual const std::string name () const
	{
    	    return "Pkg";
	}

	virtual const std::string filename () const
	{
    	    return "Pkg";
	}

	virtual std::string toString () const
	{
    	    return "// not possible toString";
	}

	virtual YCPValue evaluate (bool cse = false );

	virtual Y2Function* createFunctionCall (const std::string name, constFunctionTypePtr type);

    private:

	void registerFunctions ();

	PkgFunctions pkg_functions;
        std::vector<std::string> _registered_functions;
};
#endif // PkgModuleFunctions_h
