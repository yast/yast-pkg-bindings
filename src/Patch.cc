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

   File:	Patch.cc

   Author:	Klaus Kaempf <kkaempf@suse.de>
   Maintainer:  Klaus Kaempf <kkaempf@suse.de>
   Summary:     Access to Patch Manager
   Namespace:   Pkg
   Purpose:	Access to PMPatchManager
		Handles YOU related Pkg::function (list_of_arguments) calls
		from WFMInterpreter.
/-*/


#include <PkgFunctions.h>

#include <ycp/YCPVoid.h>

#include "log.h"

//-------------------------------------------------------------
/**   
   @builtin YouStatus
   @short obsoleted, do not use
   @return map empty map
*/
YCPValue
PkgFunctions::YouStatus ()
{
    y2warning("Pkg::YouStatus is obsoleted, do not use");
    return YCPVoid();
}

/**
   @builtin YouSetServer
   @short obsoleted, do not use
   @return string empty string
*/
YCPValue
PkgFunctions::YouSetServer (const YCPMap& servers)
{
    y2warning("Pkg::YouSetServer is obsoleted, do not use");
    return YCPVoid();
}


/**
   @builtin YouGetUserPassword
   @short obsoleted, do not use
   @return map empty map
*/
YCPValue
PkgFunctions::YouGetUserPassword ()
{
    y2warning("Pkg::YouGetUserPassword is obsoleted, do not use");
    return YCPVoid();
}

/**
   @builtin YouSetUserPassword
   @short obsoleted, do not use
   @return string empty string
*/
YCPValue
PkgFunctions::YouSetUserPassword (const YCPString& user, const YCPString& passwd, const YCPBoolean& p)
{
    y2warning("Pkg::YouSetUserPassword is obsoleted, do not use");
    return YCPVoid();
}

// ------------------------
/**
   @builtin YouGetServers
   @short obsoleted, do not use
   @return string empty string
*/
YCPValue
PkgFunctions::YouGetServers (const YCPList&)
{
    y2warning("Pkg::YouGetServers is obsoleted, do not use");
    return YCPVoid();
}




/**
  @builtin YouGetDirectory
   @short obsoleted, do not use
  @return string empty string
*/
YCPValue
PkgFunctions::YouGetDirectory ()
{
    y2warning("Pkg::YouGetDirectory is obsoleted, do not use");
    return YCPVoid();
}

/**   
  @builtin YouRetrievePatchInfo
   @short obsoleted, do not use
  @return string empty string
*/
YCPValue
PkgFunctions::YouRetrievePatchInfo (const YCPBoolean& download, const YCPBoolean& sig)
{
    y2warning("Pkg::YouRetrievePatchInfo is obsoleted, do not use");
    return YCPVoid();
}

/**   
   @builtin YouProcessPatche
   @short obsoleted, do not use
   @return boolean true
*/
YCPValue
PkgFunctions::YouProcessPatches ()
{
    y2warning("Pkg::YouProcessPatches is obsoleted, do not use");
    return YCPVoid();
}

/**   
   @builtin YouSelectPatches
   @short obsoleted, do not use
   @return void
*/
YCPValue
PkgFunctions::YouSelectPatches ()
{
    y2warning("Pkg::YouSelectPatches is obsoleted, do not use");
    return YCPVoid();
}

/**
   @builtin YouRemovePackages
   @short  - obsoleted, do not use
   @return boolean true
*/
YCPValue
PkgFunctions::YouRemovePackages ()
{
    y2warning("Pkg::YouRemovePackages is obsoleted, do not use");
    return YCPVoid();
}
