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

   File:	PkgModule.cc

   Author:	Klaus Kaempf <kkaempf@suse.de>
   Maintainer:  Klaus Kaempf <kkaempf@suse.de>

   Purpose:	Access to packagemanager
		Handles Pkg::function calls from rest of YaST.
/-*/


#include <PkgModule.h>

PkgModule* PkgModule::current_pkg = NULL;

PkgModule* PkgModule::instance ()
{
    if (current_pkg == NULL)
    {
	current_pkg = new PkgModule ();
    }
    
    return current_pkg;
}

//-------------------------------------------------------------------
// PkgModule

PkgModule::PkgModule ()
    : PkgModuleFunctions ()
{
}

/**
 * Destructor.
 */
PkgModule::~PkgModule ()
{
}

