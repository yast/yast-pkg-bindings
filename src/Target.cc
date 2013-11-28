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

   File:	PkgModuleFunctionsTarget.cc

   Author:	Klaus Kaempf <kkaempf@suse.de>
   Maintainer:  Klaus Kaempf <kkaempf@suse.de>
   Summary:     Access to Installation Target
   Namespace:      Pkg
   Purpose:	Access to InstTarget
		Handles target related Pkg::function (list_of_arguments) calls
		from WFMInterpreter.
/-*/


#include <ycp/y2log.h>
#include <PkgModule.h>
#include <PkgModuleFunctions.h>

#include <ycp/YCPVoid.h>
#include <ycp/YCPBoolean.h>
#include <ycp/YCPInteger.h>
#include <ycp/YCPSymbol.h>
#include <ycp/YCPString.h>
#include <ycp/YCPList.h>
#include <ycp/YCPMap.h>

#include <sys/statvfs.h>

#include <zypp/target/rpm/RpmDb.h>
#include <zypp/Product.h>
#include <zypp/RepoManager.h>
#include <zypp/Locks.h>
#include <zypp/DiskUsageCounter.h>

#include "PkgProgress.h"
#include "HelpTexts.h"

using namespace zypp;

/** ------------------------
 *
 * @builtin TargetInit
 * @short Initialize Target and load resolvables
 * @param string root Root Directory
 * @param boolean unused Dummy option, only for backward compatibility
 * @return boolean
 */
YCPValue
PkgModuleFunctions::TargetInit (const YCPString& root, const YCPBoolean & /*unused_and_broken*/)
{
    bool rebuild_rpmdb = false;
    const std::string r(root->value());

    // display the progress if the target is changed or if the resolvables haven't been loaded
    // otherwise there will be a quick flashing progress with no real action
    if (_target_root == r && _target_loaded)
    {
	y2milestone("Target %s is already initialized", r.c_str());
	return YCPBoolean(true);
    }

    std::list<std::string> stages;
    stages.push_back(_("Initialize the Target System"));
    stages.push_back(_("Read Installed Packages"));

    PkgProgress pkgprogress(_callbackHandler);
    pkgprogress.Start(_("Loading the Package Manager..."), stages, _(HelpTexts::load_resolvables));

    try
    {
	if (rebuild_rpmdb)
	{
	    y2milestone("Initializing the target with rebuild");
	}

	zypp_ptr()->initializeTarget(r, rebuild_rpmdb);
	//pkgprogress.NextStage();
        zypp_ptr()->target()->load();
	_target_loaded = true;
    }
    catch (zypp::Exception & excpt)
    {
	_last_error.setLastError(ExceptionAsString(excpt));
	y2error("TargetInit has failed: %s", excpt.msg().c_str() );
        return YCPError(excpt.msg().c_str(), YCPBoolean(false));
    }

    _target_root = zypp::Pathname(r);

    // locks are optional, might not be present on the target
    zypp::Pathname lock_file(_target_root + zypp::ZConfig::instance().locksFile());
    try
    {
	// read and apply the persistent locks
	y2milestone("Reading locks from %s", lock_file.asString().c_str());
	zypp::Locks::instance().readAndApply(lock_file);
    }
    catch (zypp::Exception & excpt)
    {
	y2warning("Error reading persistent locks from %s", lock_file.asString().c_str());
    }

    pkgprogress.Done();

    return YCPBoolean(true);
}

/** ------------------------
 *
 * @builtin TargetInitialize
 * @short Initialize Target, read the keys.
 * @param string root Root Directory
 * @return boolean
 */
YCPValue
PkgModuleFunctions::TargetInitialize (const YCPString& root)
{
    std::string r = root->value();

    try
    {
        zypp_ptr()->initializeTarget(r);
    }
    catch (zypp::Exception & excpt)
    {
        _last_error.setLastError(ExceptionAsString(excpt));
        y2error("TargetInit has failed: %s", excpt.msg().c_str() );
        return YCPError(excpt.msg().c_str(), YCPBoolean(false));
    }

    _target_root = zypp::Pathname(root->value());

    return YCPBoolean(true);
}

/** ------------------------
 *
 * @builtin TargetLoad
 * @short Load target resolvables into the pool
 * @return boolean
 */
YCPValue
PkgModuleFunctions::TargetLoad ()
{
    if (_target_loaded)
    {
	y2milestone("The target system is already loaded");
	return YCPBoolean(true);
    }

    std::list<std::string> stages;
    stages.push_back(_("Read Installed Packages"));

    PkgProgress pkgprogress(_callbackHandler);

    pkgprogress.Start(_("Loading the Package Manager..."), stages, _(HelpTexts::load_resolvables));

    try
    {
        zypp_ptr()->target()->load();
	_target_loaded = true;
    }
    catch (zypp::Exception & excpt)
    {
        _last_error.setLastError(ExceptionAsString(excpt));
        y2error("TargetLoad has failed: %s", excpt.msg().c_str() );
        return YCPError(excpt.msg().c_str(), YCPBoolean(false));
    }

    pkgprogress.Done();

    return YCPBoolean(true);
}

/** ------------------------
 *
 * @builtin TargetDisableSources
 * @short Disable all sources configured on the target. Typically
 * used on upgrade.
 * @return boolean true on success.
 */
YCPBoolean
PkgModuleFunctions::TargetDisableSources ()
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
 * @builtin TargetFinish
 *
 * @short finish target usage
 * @return boolean
 */
YCPBoolean
PkgModuleFunctions::TargetFinish ()
{
    try
    {
	zypp_ptr()->finishTarget();

	zypp::Pathname lock_file(_target_root + zypp::ZConfig::instance().locksFile());
	zypp::Locks::instance().save(lock_file);
    }
    catch (zypp::Exception & excpt)
    {
	_last_error.setLastError(ExceptionAsString(excpt));
	y2error("TargetFinish has failed: %s", excpt.msg().c_str() );
        return YCPBoolean(false);
    }

    // reset the target
    _target_root = zypp::Pathname();
    _target_loaded = false;

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
PkgModuleFunctions::TargetInstall(const YCPString& filename)
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
PkgModuleFunctions::TargetRemove(const YCPString& name)
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
PkgModuleFunctions::TargetLogfile (const YCPString& name)
{
    y2warning("Pkg::TargetLogfile() is obsoleted, the log file is now entirely handled by libzypp. See http://en.opensuse.org/Libzypp/Package_History");
    return YCPBoolean (true);
}


/** ------------------------
 * INTERNAL
 * get_disk_stats
 *
 * return capacity and usage of partition at directory
 */
static void
get_disk_stats (const char *fs, long long *used, long long *size, long long *bsize)
{
    struct statvfs sb;
    if (statvfs (fs, &sb) < 0)
    {
	*used = *size = *bsize = -1;
	return;
    }
    *bsize = sb.f_frsize ? : sb.f_bsize;		// block size
    *size = sb.f_blocks * *bsize;			// total size
    *used = (sb.f_blocks - sb.f_bfree) * *bsize;	// used size
}


/** ------------------------
 *
 * @builtin TargetCapacity
 *
 * @short return capacity of partition at directory
 * @param string directory
 * @return integer
 */
YCPInteger
PkgModuleFunctions::TargetCapacity (const YCPString& dir)
{
    long long used, size, bsize;
    get_disk_stats (dir->value().c_str(), &used, &size, &bsize);

    return YCPInteger (size);
}

/** ------------------------
 *
 * @builtin TargetUsed
 *
 * @short Return usage of partition at directory
 * @param string directory
 * @return integer
 *
 */
YCPInteger
PkgModuleFunctions::TargetUsed (const YCPString& dir)
{
    long long used, size, bsize;
    get_disk_stats (dir->value().c_str(), &used, &size, &bsize);

    return YCPInteger (used);
}

/** ------------------------
 *
 * @builtin TargetBlockSize
 *
 * @short Return block size of partition at directory
 * @param string directory
 * @return integer
 *
 */
YCPInteger
PkgModuleFunctions::TargetBlockSize (const YCPString& dir)
{
    long long used, size, bsize;
    get_disk_stats (dir->value().c_str(), &used, &size, &bsize);

    return YCPInteger (bsize);
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
PkgModuleFunctions::TargetProducts ()
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
PkgModuleFunctions::TargetRebuildDB ()
{
    try
    {
	zypp_ptr()->target()->rpmDb().rebuildDatabase();
    }
    catch (zypp::Exception & excpt)
    {
	_last_error.setLastError(excpt.asUserString());
	ycperror("TargetRebuildDB has failed: %s", excpt.msg().c_str() );
        return YCPBoolean(false);
    }

    return YCPBoolean(true);
}

// helper funtion
// initialize the disk usage counter with the current values from the system
void PkgModuleFunctions::SetCurrentDU()
{
    // read data from system
    zypp::DiskUsageCounter::MountPointSet system = zypp::DiskUsageCounter::detectMountPoints();

    // set the mount points
    zypp_ptr()->setPartitions(system);
}


/** ------------------------
 *
 * @builtin TargetInitDU
 *
 * @short Initialize Disk Usage Calculation
 * @description
 * init DU calculation for given directories
 *
 * <code>
 * parameter: [ $["name":"dir-without-leading-slash",
 *                "free":int_free,
 *		  "used":int_used,
 *		  "readonly":bool] ]
 *
 * </code>
 * @param list<map> param
 * @return void
 */
YCPValue
PkgModuleFunctions::TargetInitDU (const YCPList& dirlist)
{
    // remember partitioning
    if (dirlist->size() == 0)
    {
	y2milestone("Initializing Disk Usage couter from the system");
	SetCurrentDU();
	return YCPVoid();
    }

    zypp::DiskUsageCounter::MountPointSet mount_points;

    for (int i = 0; i < dirlist->size(); ++i)
    {
	bool good = true;
	YCPMap partmap;
	std::string dname;
	long long bsize = 4096LL;
	long long dfree = 0LL;
	long long dused = 0LL;
	bool readonly = false;

	if (dirlist->value(i)->isMap())
	{
	    partmap = dirlist->value(i)->asMap();
	}
	else
	{
	   good = false;
	}

	if (good
	    && !partmap->value(YCPString("name")).isNull()
	    && partmap->value(YCPString("name"))->isString())
	{
	    dname = partmap->value(YCPString("name"))->asString()->value();
	    if (dname[0] == '/' && dname.size() > 1)
	    {
		// remove the first character (/)
		dname.erase(dname.begin());
	    }
	}
	else
	{
	    y2error("Pkg::TargetInitDU: \"name\" key is missing");
	    good = false;
	}

	if (good
	    && !partmap->value(YCPString("free")).isNull()
	    && partmap->value(YCPString("free"))->isInteger())
	{
	    dfree = partmap->value(YCPString("free"))->asInteger()->value();
	}
	else
	{
	    y2error("Pkg::TargetInitDU: \"free\" key is missing");
	    good = false;
	}

	if (good
	    && !partmap->value(YCPString("used")).isNull()
	    && partmap->value(YCPString("used"))->isInteger())
	{
	    dused = partmap->value(YCPString("used"))->asInteger()->value();
	}
	else
	{
	    y2error("Pkg::TargetInitDU: \"used\" key is missing");
	    good = false;
	}

	if (good
	    && !partmap->value(YCPString("readonly")).isNull()
	    && partmap->value(YCPString("readonly"))->isBoolean())
	{
	    readonly = partmap->value(YCPString("readonly"))->asBoolean()->value();
	}
	// else: optional arg, using default

	if (!good)
	{
	    y2error ("TargetDUInit: bad item %d: %s", i, dirlist->value(i)->toString().c_str());
	    continue;
	}

	y2milestone("Adding %s", dname.c_str());

	long long dirsize = dfree + dused;

	// pkg_size is 0
	zypp::DiskUsageCounter::MountPoint mpoint(dname, bsize, dirsize, dused, 0LL, readonly);
	mount_points.insert(mpoint);
    }

    try
    {
	zypp_ptr()->setPartitions(mount_points);
    }
    catch (...)
    {
    }

    return YCPVoid();
}


/** ------------------------
 *
 * @builtin TargetGetDU
 *
 * @short return current DU calculations
 * @description
 * <code>
 * $[ "dir" : [ total, used, pkgusage, readonly ], .... ]
 * </code>
 *
 * total == total size for this partition
 *
 * used == current used size on target
 *
 * pkgusage == future used size on target based on current package selection
 *
 * readonly == true/false telling whether the partition is mounted readonly
 *
 * @return map
 */
YCPValue
PkgModuleFunctions::TargetGetDU ()
{
    YCPMap dirmap;

    try
    {
	zypp::DiskUsageCounter::MountPointSet mps = zypp_ptr()->diskUsage();

	if (mps.empty())
	{
	    // mount points have not been defined
	    y2warning("Pkg::TargetDUInit() has not been called, using data from system...");

	    // set the values from the system
	    SetCurrentDU();

	    // try it again
	    mps = zypp_ptr()->diskUsage();
	}

	// create result data structure from the stored info and calculated disk usage
	for (zypp::DiskUsageCounter::MountPointSet::const_iterator mpit = mps.begin();
	    mpit != mps.end();
	    mpit++)
	{
	    YCPList sizelist;
	    // partition size
	    sizelist->add (YCPInteger (mpit->total_size));
	    // already used
	    sizelist->add (YCPInteger (mpit->used_size));
	    // used size after PkgCommit()
	    sizelist->add (YCPInteger (mpit->pkg_size));
	    // readonly flag
	    sizelist->add (YCPInteger (mpit->readonly ? 1 : 0));

	    std::string dir = mpit->dir;
	    if (dir.size() > 1 && dir[0] != '/')
	    {
		dir.insert(dir.begin(), '/');
	    }

	    // add the map
	    dirmap->add (YCPString(mpit->dir), sizelist);
	}
    }
    catch (...)
    {
    }

    return dirmap;
}



/**
   @builtin TargetFileHasOwner

   @short returns true if the file is owned by a package
   @param string filepath
   @return boolean
*/

YCPBoolean
PkgModuleFunctions::TargetFileHasOwner (const YCPString& filepath)
{
    try
    {
	return YCPBoolean (zypp_ptr()->target()->whoOwnsFile(filepath->value()).c_str());
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
PkgModuleFunctions::TargetStoreRemove(const YCPString& root, const YCPSymbol& kind_r)
{
    return YCPBoolean(true);
}
