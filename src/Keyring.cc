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

   File:	Keyring.cc

   Summary:     Access to the Package Manager
   Namespace:   Pkg
   Purpose:	Access to the Package Manager - keyring related bindings

/-*/

#include <ycp/y2log.h>
#include "PkgModuleFunctions.h"

#include <ycp/YCPVoid.h>
#include <ycp/YCPBoolean.h>
#include <ycp/YCPInteger.h>
#include <ycp/YCPSymbol.h>
#include <ycp/YCPString.h>
#include <ycp/YCPList.h>
#include <ycp/YCPMap.h>

#include <zypp/KeyRing.h>

/****************************************************************************************
 * @builtin ImportGPGKey
 * @short Import a GPG key into the keyring
 * @description
 * Import a GPG key into the keyring in the package manager.
 *
 * @param string filename Path to the key file
 * @param boolean trusted Set to true if the key is trusted
 * @return void
 **/
YCPValue
PkgModuleFunctions::ImportGPGKey(const YCPString& filename, const YCPBoolean& trusted)
{
    bool trusted_key = trusted->value();
    std::string file = filename->value();

    y2milestone("importing %s key: %s", (trusted_key) ? "trusted" : "untrusted", file.c_str());

    try
    {
	zypp_ptr()->keyRing()->importKey(file, trusted_key);
    }
    catch (...)
    {
	y2error("Key %s: Import failed", file.c_str());
    }

    return YCPVoid();
}

