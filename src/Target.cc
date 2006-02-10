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
    /* TODO FIXME
    return YCPBoolean (_y2pm.instTarget().setInstallationLogfile (name->value()));
    */
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
 * @return list
 */

YCPList
PkgModuleFunctions::TargetProducts ()
{
    YCPList products;

    zypp::ResStore store = zypp_ptr->target()->resolvables();

    y2internal("store.size: %d", store.size());

    for (zypp::ResStore::const_iterator it = store.begin();
       it != store.end();
       it++)
    {
	YCPMap prod;

	if (zypp::isKind<zypp::Product>(*it))
	{
#warning TargetProducts doesn't return all keys
	    prod->add( YCPString("name"), YCPString( (*it)->name() ) );
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

    if (dirlist->size() == 0)
    {
	return YCPError ("Bad args to Pkg::TargetInitDU");
    }

//    std::set<PkgDuMaster::MountPoint> mountpoints;
#warning TargetInitDU doesn't pass DU list to zypp

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
	    && partmap->value(YCPString("name"))->isString())
	{
	    dname = partmap->value(YCPString("name"))->asString()->value();
	    if (dname[0] != '/')
		dname = string("/") + dname;
	}
	else
	{
	    good = false;
	}

	if (good
	    && partmap->value(YCPString("free"))->isInteger())
	{
	    dfree = partmap->value(YCPString("free"))->asInteger()->value();
	}
	else
	{
	    good = false;
	}

	if (good
	    && partmap->value(YCPString("used"))->isInteger())
	{
	    dused = partmap->value(YCPString("used"))->asInteger()->value();
	}
	else
	{
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

	long long dirsize = dfree + dused;

//	PkgDuMaster::MountPoint point (dname, FSize (4096), FSize (dirsize), FSize (dused), readonly);
//	mountpoints.insert (point);
    }
    // TODO FIXME    _y2pm.packageManager().setMountPoints(mountpoints);

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
# warning TargetGetDU is not implemented
    /*
    std::set<PkgDuMaster::MountPoint> mountpoints; // TODO FIXME = _y2pm.packageManager().updateDu().mountpoints();

    for (std::set<PkgDuMaster::MountPoint>::iterator it = mountpoints.begin();
	 it != mountpoints.end(); ++it)
    {
	YCPList sizelist;
	sizelist->add (YCPInteger ((long long)(it->total())));
	sizelist->add (YCPInteger ((long long)(it->initial_used())));
	sizelist->add (YCPInteger ((long long)(it->pkg_used())));
	sizelist->add (YCPInteger (it->readonly() ? 1 : 0));
	dirmap->add (YCPString (it->_mountpoint), sizelist);
    }
    */
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

