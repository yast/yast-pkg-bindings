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

struct YaSTZyppLogger : public zypp::base::LogControl::LineWriter
{
  virtual void writeOut( const std::string & formated_r )
  {
    y2lograw((formated_r+"\n").c_str());
  }
};


PkgModule* PkgModule::instance ()
{
    if (current_pkg == NULL)
    {
	y2milestone("Redirecting ZYPP log to y2log");

        boost::shared_ptr<YaSTZyppLogger> myLogger( new YaSTZyppLogger );
        zypp::base::LogControl::instance().setLineWriter( myLogger );

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

