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
#include <y2util/y2log.h>
#include <zypp/base/LogControl.h>
#include <zypp/Pathname.h>

PkgModule* PkgModule::current_pkg = NULL;

PkgModule* PkgModule::instance ()
{
    if (current_pkg == NULL)
    {
	std::string yast_log = get_log_filename();
	
	int pos = yast_log.rfind("y2log");

	if( pos >= 0){	
		yast_log = yast_log.erase(pos) + "zypp.log";
		y2milestone( "Setting ZYPP log to: %s",  yast_log.c_str ());

		zypp::base::LogControl::instance().logfile( zypp::Pathname(yast_log) );
	}

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

