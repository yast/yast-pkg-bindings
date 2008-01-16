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
   Purpose:	Access to the Package Manager - GPG key management related functions

/-*/

#include <list>
#include <set>

#include "PkgFunctions.h"
#include "GPGMap.h"
#include "log.h"

#include <ycp/YCPVoid.h>
#include <ycp/YCPBoolean.h>
#include <ycp/YCPString.h>
#include <ycp/YCPList.h>
#include <ycp/YCPInteger.h>

#include <zypp/KeyRing.h>
#include <zypp/PublicKey.h>
#include <zypp/Pathname.h>

/*
  Textdomain "pkg-bindings"
*/

/****************************************************************************************
 * @builtin ImportGPGKey
 * @short Import a GPG key into the keyring
 * @description
 * Import a GPG key into the keyring in the package manager.
 *
 * @param string filename Path to the key file
 * @param boolean trusted Set to true if the key is trusted
 * @return boolean true on success
 **/
YCPValue
PkgFunctions::ImportGPGKey(const YCPString& filename, const YCPBoolean& trusted)
{
    const bool trusted_key = trusted->value();
    const std::string file = filename->value();

    y2milestone("importing %s key: %s", (trusted_key) ? "trusted" : "untrusted", file.c_str());

    try
    {
	zypp::Pathname pname(file);
	zypp::PublicKey pubkey(pname);

	zypp_ptr()->keyRing()->importKey(pubkey, trusted_key);
    }
    catch (...)
    {
	y2error("Key %s: Import failed", file.c_str());
	return YCPBoolean(false);
    }

    return YCPBoolean(true);
}

// A helper class
// converts PublicKey to YCPMap and adds it to an YCPList
class PublicKeyAdder : public std::unary_function<const zypp::PublicKey &, void>
{
    public:

	// where to add the public key maps, which keys are trusted
	PublicKeyAdder(YCPList &lst, bool trusted_keyring)
	    : list(lst), trusted(trusted_keyring)
	{
	}

	void operator() (const zypp::PublicKey &key)
	{
	    GPGMap gpgmap(key);
	    gpgmap.setTrusted(trusted);

	    return list->add(gpgmap.getMap());
	}

    private:

	// where to add the converted value
	YCPList &list;
	// processing trusted or known GPG keys
	const bool trusted;
};

/****************************************************************************************
 * @builtin GPGKeys
 * @short Read the GPG keys in the package manager keyring
 * @description
 * Read known or trusted keys from the package manager
 *
 * @param boolean trusted If set to true trusted keys are returned, 
 * @return list List of maps $[ "id" : string , "name" : string, "fingerprint" : string, "trusted" : boolean,
 *    "created" : string, "created_raw" : integer, "expires" : string, "expires_raw" : integer ] or nil when an error occurred
 **/
YCPValue PkgFunctions::GPGKeys(const YCPBoolean& trusted)
{
    try
    {
	YCPList ret;
	const bool trusted_only = trusted->value();
	const zypp::KeyRing_Ptr keyring(zypp_ptr()->keyRing());

	// use the required keyring
	std::list<zypp::PublicKey> key_list(
	    trusted_only ? keyring->trustedPublicKeys()
	    : keyring->publicKeys()
	);

	// convert std::list<PublicKey> to YCPList, pass the known/trusted flag
	for_each(key_list.begin(), key_list.end(), PublicKeyAdder(ret, trusted_only));

	return ret;
    }
    catch (const zypp::Exception& excpt)
    {
	y2error("Cannot read GPG keys: %s", excpt.asString().c_str());
	_last_error.setLastError(ExceptionAsString(excpt));
	return YCPVoid();
    }
}

/****************************************************************************************
 * @builtin DeleteGPGKey
 * @short Remove the GPG key from the package manager keyring
 * @description
 * Remove the GPG key from the known or trusted keyring
 *
 * @param key_id GPG key ID of the key to remove
 * @param trusted If set to true the key will be removed from the trusted keyring otherwise it's removed from the known keyring
 * @return boolean true on success
 **/
YCPValue PkgFunctions::DeleteGPGKey(const YCPString& key_id, const YCPBoolean& trusted)
{
    bool ret;
    try
    {
	zypp_ptr()->keyRing()->deleteKey(key_id->value(), trusted->value());
	ret = true;
    }
    catch(const zypp::Exception &excpt)
    {
	y2error("Cannot delete GPG key %s from %s keying: %s",
	    key_id->value().c_str(),
	    trusted->value() ? "trusted" : "known",
	    excpt.asString().c_str()
	);

	_last_error.setLastError(ExceptionAsString(excpt));
	ret = false;
    }

    return YCPBoolean(ret);
}

/****************************************************************************************
 * @builtin CheckGPGKeyFile
 * @short Check whether the file contains a valid GPG key
 *
 * @param keyfile path to the file
 * @return true if the GPG key is valid
 **/
YCPValue PkgFunctions::CheckGPGKeyFile(const YCPString& keyfile)
{
    try
    {
	zypp::PublicKey key(keyfile->value());

	GPGMap gpgmap(key);
	return gpgmap.getMap();
    }
    catch(...)
    {}

    return YCPVoid();
}

