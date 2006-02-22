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

#include <unistd.h>
#include <sys/statvfs.h>

#include <zypp/target/rpm/RpmDb.h>
#include <zypp/Product.h>

using std::string;

/** ------------------------
 *
 * @builtin TargetInit
 * @short Initialize Target
 * @param string root Root Directory
 * @param boolean new If true, initialize new rpm database
 * @return boolean
 */
YCPValue
PkgModuleFunctions::TargetInit (const YCPString& root, const YCPBoolean& /*unused*/ )
{
    std::string r = root->value();

    try
    {
	zypp_ptr->initTarget(r);
	zypp_ptr->target()->enableStorage(r);
    }
    catch (zypp::Exception & excpt)
    {
	ycperror("TargetInit has failed: %s", excpt.msg().c_str() );
        return YCPError(excpt.msg().c_str(), YCPBoolean(false));
    }
    
    _target_root = zypp::Pathname(root->value());

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
	zypp_ptr->finishTarget();
    }
    catch (zypp::Exception & excpt)
    {
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
	zypp_ptr->target()->rpmDb().installPackage(filename->value());
    }
    catch (zypp::Exception & excpt)
    {
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
	zypp_ptr->target()->rpmDb().removePackage(name->value());
    }
    catch (zypp::Exception & excpt)
    {
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
	return YCPBoolean (zypp_ptr->target()->setInstallationLogfile (name->value()));
    }
    catch (zypp::Exception & excpt)
    {
	ycperror("TargetRemove has failed: %s", excpt.asString().c_str());
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
 * @return list
 */

YCPList
PkgModuleFunctions::TargetProducts ()
{
    YCPList products;

    for(zypp::ResPool::byKind_iterator it = zypp_ptr->pool().byKindBegin(zypp::ResTraits<zypp::Product>::kind)
	; it != zypp_ptr->pool().byKindEnd(zypp::ResTraits<zypp::Product>::kind)
	; ++it)
    {
	zypp::Product::constPtr product = boost::dynamic_pointer_cast<const zypp::Product>( it->resolvable() );

	if (it->status().isInstalled() )
	{
	    YCPMap prod;
#warning TargetProducts doesn't return all keys
	    prod->add( YCPString("name"), YCPString( product->name() ) );
	    prod->add( YCPString("relnotesurl"), YCPString( product->releaseNotesUrl().asString() ) );
	    products->add(prod);
	}
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
	zypp_ptr->target()->rpmDb().rebuildDatabase();
    }
    catch (zypp::Exception & excpt)
    {
	ycperror("TargetRebuildDB has failed: %s", excpt.msg().c_str() );
        return YCPBoolean(false);
    }

    return YCPBoolean(true);
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
    // clear the old settings
    _mount_points.clear();

    // remember partitioning
    if (dirlist->size() == 0)
    {
	return YCPError ("Bad args to Pkg::TargetInitDU");
    }

    for (int i = 0; i < dirlist->size(); ++i)
    {
	bool good = true;
	YCPMap partmap;
	std::string dname;
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
	    if (dname[0] == '/')
		dname.erase(0,1); // remove leading /
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

	y2internal("Adding %s", dname.c_str());

	long long dirsize = dfree + dused;

	// pkg_size is 0
	MountPoint mpoint(dname, dirsize, dused, 0LL, readonly);
	_mount_points.insert(mpoint);
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

    // check whether disk usage is initialized
    if (_mount_points.empty())
    {
	y2error("Disk usage has not been initalized, use Pkg::TargetDUInit()");
	return dirmap;	
    }
    
    // reset all package counters (pkg_size values)
    for (std::set<MountPoint>::iterator mpit = _mount_points.begin();
	mpit != _mount_points.end();
	mpit++)
    {
	mpit->pkg_size = 0LL;
    }

    // iterate through all packages
    for (zypp::ResPool::byKind_iterator it = zypp_ptr->pool().byKindBegin(zypp::ResTraits<zypp::Package>::kind);
	it != zypp_ptr->pool().byKindEnd(zypp::ResTraits<zypp::Package>::kind);
	++it)
    {
	if (!it->status().isToBeInstalled() && !it->status().isToBeUninstalled())
	{
	    // shortcut - the package is not selected for installation or removing
	    // so it can't affect disk usage at all
	    continue;
	}

	zypp::Package::constPtr pkg = boost::dynamic_pointer_cast<const zypp::Package>( it->resolvable() );
	zypp::DiskUsage du = pkg->diskusage();

	y2debug("Package: %s", (*it)->name().c_str());

	// TODO remove this, it's only useful for #152761 debugging
	for (zypp::DiskUsage::iterator di = du.begin();
	    di != du.end();
	    di++)
	{
	    y2debug("read DU dir: %s, used: %d", di->path.c_str(), di->_size);
	}

	// iterate trough all mount points, add usage to each directory
	// directory tree must be processed from leaves to the root directory
	// so iterate in reverse order so e.g. /usr is used before /
	for (std::set<MountPoint>::reverse_iterator mpit = _mount_points.rbegin();
	    mpit != _mount_points.rend();
	    mpit++)
	{
	    // get usage for the mount point
	    zypp::DiskUsage::Entry entry = du.extract(mpit->dir);

	    // add or subtract it to the current value
	    if (it->status().isToBeInstalled())
	    {
		y2debug("mountpoint: %s, added: %d", mpit->dir.c_str(), entry._size);
		mpit->pkg_size += entry._size;
	    }
	    else if (it->status().isToBeUninstalled())
	    {
		y2debug("mountpoint: %s, removed: %d", mpit->dir.c_str(), entry._size);
		mpit->pkg_size -= entry._size;
	    }
	}

    }

    // create result data structure
    for (std::set<MountPoint>::const_iterator mpit = _mount_points.begin();
	mpit != _mount_points.end();
	mpit++)
    {
	YCPList sizelist;
	// partition size
	sizelist->add (YCPInteger (mpit->total_size));
	// already used
	sizelist->add (YCPInteger (mpit->used_size));
	// used size after PkgCommit()
	sizelist->add (YCPInteger (mpit->pkg_size + mpit->used_size));
	// readonly flag
	sizelist->add (YCPInteger (mpit->readonly ? 1 : 0));

	// add slash (which is not used by zypp::DiskUsage)
	dirmap->add (YCPString("/" + mpit->dir), sizelist);
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
    return YCPBoolean (zypp_ptr->target()->whoOwnsFile(filepath->value()));
}

