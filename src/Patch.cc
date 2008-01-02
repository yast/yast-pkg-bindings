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

   File:	PkgModuleFunctionsPatch.cc

   Author:	Klaus Kaempf <kkaempf@suse.de>
   Maintainer:  Klaus Kaempf <kkaempf@suse.de>
   Summary:     Access to Patch Manager
   Namespace:   Pkg
   Purpose:	Access to PMPatchManager
		Handles YOU related Pkg::function (list_of_arguments) calls
		from WFMInterpreter.
/-*/


#include <PkgModuleFunctions.h>

#include <ycp/YCPVoid.h>

//-------------------------------------------------------------
/**   
   @builtin YouStatus
   @short obsoleted, do not use
   @return map empty map
*/
YCPValue
PkgModuleFunctions::YouStatus ()
{
    return YCPVoid();
}

/**
   @builtin YouSetServer
   @short obsoleted, do not use
   @return string empty string
*/
YCPValue
PkgModuleFunctions::YouSetServer (const YCPMap& servers)
{
    return YCPVoid();
}


/**
   @builtin YouGetUserPassword
   @short obsoleted, do not use
   @return map empty map
*/
YCPValue
PkgModuleFunctions::YouGetUserPassword ()
{
    return YCPVoid();
}

/**
   @builtin YouSetUserPassword
   @short obsoleted, do not use
   @return string empty string
*/
YCPValue
PkgModuleFunctions::YouSetUserPassword (const YCPString& user, const YCPString& passwd, const YCPBoolean& p)
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
PkgModuleFunctions::YouGetServers (const YCPList&)
{
    return YCPVoid();
}




/**
  @builtin YouGetDirectory
   @short obsoleted, do not use
  @return string empty string
*/
YCPValue
PkgModuleFunctions::YouGetDirectory ()
{
    return YCPVoid();
}

/**   
  @builtin YouRetrievePatchInfo
   @short obsoleted, do not use
  @return string empty string
*/
YCPValue
PkgModuleFunctions::YouRetrievePatchInfo (const YCPBoolean& download, const YCPBoolean& sig)
{
    return YCPVoid();
}

/**   
   @builtin YouProcessPatche
   @short obsoleted, do not use
   @return boolean true
*/
YCPValue
PkgModuleFunctions::YouProcessPatches ()
{
    return YCPVoid();
}

/**   
   @builtin YouSelectPatches
   @short obsoleted, do not use
   @return void
*/
YCPValue
PkgModuleFunctions::YouSelectPatches ()
{
    return YCPVoid();
}

/**
   @builtin YouRemovePackages
   @short  - obsoleted, do not use
   @return boolean true
*/
YCPValue
PkgModuleFunctions::YouRemovePackages ()
{
    return YCPVoid();
}
