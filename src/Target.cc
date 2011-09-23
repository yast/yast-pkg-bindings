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

#include <zypp/base/Easy.h>
#include <zypp/Product.h>
#include <zypp/target/rpm/RpmDb.h>

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
        zypp::ResPool pool( zypp::ResPool::instance() ); // ResPool is a global singleton

        for_( it, pool.byKindBegin<zypp::Product>(), pool.byKindEnd<zypp::Product>() )
        {
          if ( ! it->status().isInstalled() )
            continue;
          zypp::Product::constPtr product = asKind<zypp::Product>( it->resolvable() );

          #warning TargetProducts does not return all keys
          YCPMap prod;
          // see also PkgFunctions::Descr2Map and Product.ycp::Product
          // FIXME unify with code in Pkg::ResolvablePropertiesEx
          prod->add( YCPString("name"), YCPString( product->name() ) );
          prod->add( YCPString("version"), YCPString( product->edition().version() ) );

          std::string category(product->isTargetDistribution() ? "base" : "addon");
          prod->add(YCPString("type"), YCPString(category));
          prod->add(YCPString("category"), YCPString(category));

          prod->add(YCPString("vendor"), YCPString(product->vendor()));
          prod->add(YCPString("relnotes_url"), YCPString(product->releaseNotesUrls().first().asString()));
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
          zypp::Product::UrlList pupdateUrls = product->updateUrls();
          for_( it, pupdateUrls.begin(), pupdateUrls.end() )
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
	return YCPBoolean (!zypp_ptr()->target()->whoOwnsFile(filepath->value()).empty());
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
    return YCPBoolean(true);
}
