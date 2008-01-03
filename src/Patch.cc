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

//-------------------------------------------------------------
/**   
   @builtin YouStatus
   @short obsoleted, do not use
   @return map empty map
*/
YCPValue
PkgFunctions::YouStatus ()
{
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
    return YCPVoid();
}
