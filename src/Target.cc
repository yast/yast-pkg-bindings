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

   File:	PkgFunctionsTarget.cc

   Author:	Klaus Kaempf <kkaempf@suse.de>
   Maintainer:  Klaus Kaempf <kkaempf@suse.de>
   Summary:     Access to Installation Target
   Namespace:      Pkg
   Purpose:	Access to InstTarget
		Handles target related Pkg::function (list_of_arguments) calls
		from WFMInterpreter.
/-*/


#include <PkgFunctions.h>

#include <ycp/YCPVoid.h>
#include <ycp/YCPBoolean.h>
#include <ycp/YCPInteger.h>
#include <ycp/YCPSymbol.h>
#include <ycp/YCPString.h>
#include <ycp/YCPList.h>
#include <ycp/YCPMap.h>

#include <zypp/Product.h>
#include <zypp/target/rpm/RpmDb.h>
#include <zypp/target/store/PersistentStorage.h>

#include "log.h"

using namespace zypp;

/** ------------------------
 *
 * @builtin TargetDisableSources
 * @short Disable all sources configured on the target. Typically
 * used on upgrade.
 * @return boolean true on success.
 */
YCPBoolean
PkgFunctions::TargetDisableSources ()
{
    try
    {
// FIXME: should it also remove from pool?

	zypp::RepoManager repomanager = CreateRepoManager();
	std::list<zypp::RepoInfo> all_sources = repomanager.knownRepositories();

	for (std::list<zypp::RepoInfo>::iterator it = all_sources.begin(); it != all_sources.end(); ++it)
	{
	    y2milestone("Disabling source '%s'", it->alias().c_str());
	    it->setAutorefresh(false);

	    repomanager.modifyRepository(it->alias(), *it);
	}
    }
    catch (zypp::Exception & excpt)
    {
	_last_error.setLastError(ExceptionAsString(excpt));
	y2error("TargetDisableSources has failed: %s", excpt.msg().c_str() );
        return YCPBoolean(false);
    }

    return YCPBoolean(true);
}

/** ------------------------
 *
 * @builtin TargetInstall
 *
 * @short install rpm package by filename
 * @description
 * the filename must be an absolute path to a file which can
 * be accessed by the package manager.
 *
 * @note This builtin uses callbacks * You should do an 'import "PackageCallbacks"' before calling this.
 * @param string filename
 * @return boolean
 */
YCPBoolean
PkgFunctions::TargetInstall(const YCPString& filename)
{
    try
    {
	zypp_ptr()->target()->rpmDb().installPackage(filename->value());
    }
    catch (zypp::Exception & excpt)
    {
	_last_error.setLastError(ExceptionAsString(excpt));
	y2error("TargetInstall has failed: %s", excpt.asString().c_str());
        return YCPBoolean(false);
    }

    return YCPBoolean(true);
}


/** ------------------------
 *
 * @builtin TargetRemove
 *
 * @short Install package by name
 * @description
 * Install package by name
 * @note This builtin uses callbacks * You should do an 'import "PackageCallbacks"' before calling this.
 * @param string name
 * @return boolean
 */
YCPBoolean
PkgFunctions::TargetRemove(const YCPString& name)
{
    try
    {
	zypp_ptr()->target()->rpmDb().removePackage(name->value());
    }
    catch (zypp::Exception & excpt)
    {
	_last_error.setLastError(ExceptionAsString(excpt));
	y2error("TargetRemove has failed: %s", excpt.asString().c_str());
        return YCPBoolean(false);
    }

    return YCPBoolean (true);
}


/** ------------------------
 *
 * @builtin TargetLogfile
 * @short init logfile for target
 * @param string name
 * @return boolean
 */
YCPBoolean
PkgFunctions::TargetLogfile (const YCPString& name)
{
    try
    {
	return YCPBoolean (zypp_ptr()->target()->setInstallationLogfile (name->value()));
    }
    catch (zypp::Exception & excpt)
    {
	_last_error.setLastError(ExceptionAsString(excpt));
	y2error("TargetLogfile has failed: %s", excpt.asString().c_str());
        return YCPBoolean(false);
    }
    return YCPBoolean (true); // never reached
}


/** ------------------------
 *
 * @builtin TargetProducts
 *
 * @short Return list of maps of all installed products
 * @description
 * return list of maps of all installed products in reverse
 * installation order (product installed last comes first)
 *
 * Deprecated, will be replaced by ResolvableProperties() in the future.
 *
 * @return list
 */

YCPList
PkgFunctions::TargetProducts ()
{
    YCPList products;

    try
    {
        for (ResStore::resfilter_const_iterator it = zypp_ptr()->target()->byKindBegin(ResTraits<Product>::kind); it != zypp_ptr()->target()->byKindEnd(ResTraits<Product>::kind); ++it)
        {
          zypp::Product::constPtr product = asKind<const zypp::Product>( *it );
#warning TargetProducts does not return all keys
          YCPMap prod;
          // see also PkgFunctions::Descr2Map and Product.ycp::Product
// FIXME unify with code in Pkg::ResolvablePropertiesEx
          prod->add( YCPString("name"), YCPString( product->name() ) );
          prod->add( YCPString("version"), YCPString( product->edition().version() ) );
	  prod->add(YCPString("category"), YCPString(product->category()));
	  prod->add(YCPString("vendor"), YCPString(product->vendor()));
	  prod->add(YCPString("relnotes_url"), YCPString(product->releaseNotesUrl().asString()));
	  std::string product_summary = product->summary();
	  if (product_summary.size() > 0)
	  {
	    prod->add(YCPString("display_name"), YCPString(product_summary));
	  }
	  std::string product_shortname = product->shortName();
	  if (product_shortname.size() > 0)
	  {
	    prod->add(YCPString("short_name"), YCPString(product_shortname));
	  }
	  // use summary for the short name if it's defined
	  else if (product_summary.size() > 0)
	  {
	    prod->add(YCPString("short_name"), YCPString(product_summary));
	  }
	  prod->add(YCPString("description"), YCPString((*it)->description()));

	  std::string resolvable_summary = (*it)->summary();
	  if (resolvable_summary.size() > 0)
	  {
	    prod->add(YCPString("summary"), YCPString((*it)->summary()));
	  }
	  YCPList updateUrls;
	  std::list<zypp::Url> pupdateUrls = product->updateUrls();
	  for (std::list<zypp::Url>::const_iterator it = pupdateUrls.begin(); it != pupdateUrls.end(); ++it)
	  {
	    updateUrls->add(YCPString(it->asString()));
	  }
	  prod->add(YCPString("update_urls"), updateUrls);

	  YCPList flags;
	  std::list<std::string> pflags = product->flags();
	  for (std::list<std::string>::const_iterator flag_it = pflags.begin();
	    flag_it != pflags.end(); ++flag_it)
	  {
	    flags->add(YCPString(*flag_it));
	  }
	  prod->add(YCPString("flags"), flags);

          products->add(prod);
        }
    }
    catch(...)
    {
    }

    return products;
}

/** ------------------------
 *
 * @builtin TargetRebuildDB
 *
 * @short call "rpm --rebuilddb"
 * @return boolean
 */

YCPBoolean
PkgFunctions::TargetRebuildDB ()
{
    try
    {
	zypp_ptr()->target()->rpmDb().rebuildDatabase();
    }
    catch (zypp::Exception & excpt)
    {
	_last_error.setLastError(ExceptionAsString(excpt));
	y2error("TargetRebuildDB has failed: %s", excpt.msg().c_str() );
        return YCPBoolean(false);
    }

    return YCPBoolean(true);
}


/**
   @builtin TargetFileHasOwner

   @short returns true if the file is owned by a package
   @param string filepath
   @return boolean
*/

YCPBoolean
PkgFunctions::TargetFileHasOwner (const YCPString& filepath)
{
    try
    {
	return YCPBoolean (zypp_ptr()->target()->whoOwnsFile(filepath->value()));
    }
    catch (...)
    {
    }

    return YCPBoolean(false);
}


/**
   @builtin TargetStoreRemove

   @short remove all resolvables from the DB in the target system (removes only metadata in the package manager!)
   @description
   Use this function only in a special case, direct access to the DB should not be used in general!
   Warning: this built in can cause inconsistency between the package manager and the system!
   @param root path to the root directory
   @param kind_r kind of resolvable, can be `product, `patch, `package, `selection, `pattern or `language
   @return boolean true = success
*/
YCPBoolean
PkgFunctions::TargetStoreRemove(const YCPString& root, const YCPSymbol& kind_r)
{
    zypp::Resolvable::Kind kind;
    std::string req_kind = kind_r->symbol();

    if( req_kind == "product" ) {
	kind = zypp::ResTraits<zypp::Product>::kind;
    }
    else if ( req_kind == "patch" ) {
    	kind = zypp::ResTraits<zypp::Patch>::kind;
    }
    else if ( req_kind == "package" ) {
	kind = zypp::ResTraits<zypp::Package>::kind;
    }
    else if ( req_kind == "selection" ) {
	kind = zypp::ResTraits<zypp::Selection>::kind;
    }
    else if ( req_kind == "pattern" ) {
	kind = zypp::ResTraits<zypp::Pattern>::kind;
    }
    else if ( req_kind == "language" ) {
	kind = zypp::ResTraits<zypp::Language>::kind;
    }
    else
    {
	y2error("Pkg::TargetStoreRemove: unknown symbol: %s", req_kind.c_str());
	return YCPBoolean(false);
    }

    bool success = true;

    std::string target_root = root->value();
    if (target_root.empty())
    {
	y2error("Pkg::TargetStoreRemove: parameter root is empty");
	return YCPBoolean(false);
    }

    try
    {
	// create a storage
	zypp::storage::PersistentStorage store;
	store.init( target_root );

	// get all resolvables of the required kind
	std::list<ResObject::Ptr> objects = store.storedObjects(kind);

	y2warning("Removing %zd objects of kind '%s' from %s", objects.size(), req_kind.c_str(), target_root.c_str());

	// remove the resolvables
	for( std::list<ResObject::Ptr>::const_iterator it = objects.begin(); it != objects.end(); ++it)
	{
	    try {
		store.deleteObject(*it);
	    } catch( const zypp::Exception& excpt )
	    {
		y2error("TargetStoreRemove has failed: %s", excpt.msg().c_str());
		success = false;
	    }
	}
    }
    catch( const zypp::Exception& excpt )
    {
	y2error("TargetStoreRemove has failed: %s", excpt.msg().c_str());
	success = false;
    }
    
    return YCPBoolean(success);
}
