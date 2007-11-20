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
#include <zypp/DiskUsageCounter.h>
#include <zypp/target/store/PersistentStorage.h>

using namespace zypp;

/** ------------------------
 *
 * @builtin TargetInit
 * @deprecated
 * @short Initialize Target and load resolvables
 * @param string root Root Directory
 * @param boolean new If true, initialize new rpm database
 * @return boolean
 */
YCPValue
PkgModuleFunctions::TargetInit (const YCPString& root, const YCPBoolean & /*unused_and_broken*/)
{
    std::string r = root->value();

    try
    {
	zypp_ptr()->initTarget(r);
        zypp_ptr()->addResolvables( zypp_ptr()->target()->resolvables(), true );
    }
    catch (zypp::Exception & excpt)
    {
	_last_error.setLastError(ExceptionAsString(excpt));
	ycperror("TargetInit has failed: %s", excpt.msg().c_str() );
        return YCPError(excpt.msg().c_str(), YCPBoolean(false));
    }
    
    _target_root = zypp::Pathname(root->value());
    
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
        ycperror("TargetInit has failed: %s", excpt.msg().c_str() );
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
    try
    {
        zypp_ptr()->addResolvables( zypp_ptr()->target()->resolvables(), true );
    }
    catch (zypp::Exception & excpt)
    {
        _last_error.setLastError(ExceptionAsString(excpt));
        ycperror("TargetLoad has failed: %s", excpt.msg().c_str() );
        return YCPError(excpt.msg().c_str(), YCPBoolean(false));
    }
    
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
	ycperror("TargetDisableSources has failed: %s", excpt.msg().c_str() );
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
    }
    catch (zypp::Exception & excpt)
    {
	_last_error.setLastError(ExceptionAsString(excpt));
	ycperror("TargetFinish has failed: %s", excpt.msg().c_str() );
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
PkgModuleFunctions::TargetInstall(const YCPString& filename)
{
    try
    {
	zypp_ptr()->target()->rpmDb().installPackage(filename->value());
    }
    catch (zypp::Exception & excpt)
    {
	_last_error.setLastError(ExceptionAsString(excpt));
	ycperror("TargetInstall has failed: %s", excpt.asString().c_str());
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
	ycperror("TargetRemove has failed: %s", excpt.asString().c_str());
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
    try
    {
	return YCPBoolean (zypp_ptr()->target()->setInstallationLogfile (name->value()));
    }
    catch (zypp::Exception & excpt)
    {
	_last_error.setLastError(ExceptionAsString(excpt));
	ycperror("TargetLogfile has failed: %s", excpt.asString().c_str());
        return YCPBoolean(false);
    }
    return YCPBoolean (true); // never reached
}


/** ------------------------
 * INTERNAL
 * get_disk_stats
 *
 * return capacity and usage of partition at directory
 */
static void
get_disk_stats (const char *fs, long long *used, long long *size, long long *bsize, long long *available)
{
    struct statvfs sb;
    if (statvfs (fs, &sb) < 0)
    {
	*used = *size = *bsize = *available = -1;
	y2error("statvfs() failed: %s", strerror(errno));
	return;
    }
    *bsize = sb.f_frsize ? : sb.f_bsize;		// block size
    *size = sb.f_blocks * *bsize;			// total size
    *used = (sb.f_blocks - sb.f_bfree) * *bsize;
    *available = sb.f_bavail * *bsize;			// available for non-root user

    y2debug("stavfs: dir: %s, sb.f_frsize: %lu, sb.f_bsize: %lu, sb.f_blocks: %lu, sb.f_bfree: %lu, sb.f_bavail: %lu, bsize: %lld, size: %lld, used: %lld, available: %lld", fs, sb.f_frsize, sb.f_bsize, sb.f_blocks, sb.f_bfree, sb.f_bavail, *bsize, *size, *used, *available);
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
    long long used, size, bsize, avail;
    get_disk_stats (dir->value().c_str(), &used, &size, &bsize, &avail);

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
    long long used, size, bsize, avail;
    get_disk_stats (dir->value().c_str(), &used, &size, &bsize, &avail);

    return YCPInteger (used);
}

/** ------------------------
 *
 * @builtin TargetAvailable
 *
 * @short Return free space for non-root user (use Pkg::TargetCapacity(dir) - Pkg::TargetUsed(dir) to get free space including the space reserved for root user).
 * @param string directory
 * @return integer
 *
 */
YCPInteger
PkgModuleFunctions::TargetAvailable(const YCPString& dir)
{
    long long used, size, bsize, avail;
    get_disk_stats (dir->value().c_str(), &used, &size, &bsize, &avail);

    return YCPInteger (avail);
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
    long long used, size, bsize, avail;
    get_disk_stats (dir->value().c_str(), &used, &size, &bsize, &avail);

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
        for (ResStore::resfilter_const_iterator it = zypp_ptr()->target()->byKindBegin(ResTraits<Product>::kind); it != zypp_ptr()->target()->byKindEnd(ResTraits<Product>::kind); ++it)
        {
          zypp::Product::constPtr product = asKind<const zypp::Product>( *it );
#warning TargetProducts does not return all keys
          YCPMap prod;
          // see also PkgModuleFunctions::Descr2Map and Product.ycp::Product
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
PkgModuleFunctions::TargetRebuildDB ()
{
    try
    {
	zypp_ptr()->target()->rpmDb().rebuildDatabase();
    }
    catch (zypp::Exception & excpt)
    {
	_last_error.setLastError(ExceptionAsString(excpt));
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

YCPMap PkgModuleFunctions::MPS2YCPMap(const zypp::DiskUsageCounter::MountPointSet &mps)
{
    YCPMap dirmap;

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

    return dirmap;
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

	dirmap = MPS2YCPMap(mps);
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
PkgModuleFunctions::TargetStoreRemove(const YCPString& root, const YCPSymbol& kind_r)
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

	y2warning("Removing %d objects of kind '%s' from %s", objects.size(), req_kind.c_str(), target_root.c_str());

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
