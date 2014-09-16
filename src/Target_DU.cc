/* ------------------------------------------------------------------------------
 * Copyright (c) 2007 Novell, Inc. All Rights Reserved.
 *
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of version 2 of the GNU General Public License as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, contact Novell, Inc.
 *
 * To contact Novell about this file by physical or electronic mail, you may find
 * current contact information at www.novell.com.
 * ------------------------------------------------------------------------------
 */

/*
   File:	$Id$
   Author:	Ladislav Slezák <lslezak@novell.com>
   Summary:     Disk usage statistics
   Namespace:   Pkg
*/

#include <PkgFunctions.h>
#include "log.h"

#include <ycp/YCPVoid.h>
#include <ycp/YCPBoolean.h>
#include <ycp/YCPInteger.h>
#include <ycp/YCPString.h>
#include <ycp/YCPList.h>
#include <ycp/YCPMap.h>

#include <sys/statvfs.h>

#include <zypp/DiskUsageCounter.h>

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
PkgFunctions::TargetCapacity (const YCPString& dir)
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
PkgFunctions::TargetUsed (const YCPString& dir)
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
PkgFunctions::TargetAvailable(const YCPString& dir)
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
PkgFunctions::TargetBlockSize (const YCPString& dir)
{
    long long used, size, bsize, avail;
    get_disk_stats (dir->value().c_str(), &used, &size, &bsize, &avail);

    return YCPInteger (bsize);
}

// helper funtion
// initialize the disk usage counter with the current values from the system
void PkgFunctions::SetCurrentDU()
{
    // read data from system
    zypp::DiskUsageCounter::MountPointSet system = zypp::DiskUsageCounter::detectMountPoints();

    // set the mount points
    zypp_ptr()->setPartitions(system);
}

YCPMap PkgFunctions::MPS2YCPMap(const zypp::DiskUsageCounter::MountPointSet &mps)
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
 * parameter: [ $["name":"directory",
 *                "free":int_free,
 *		  "used":int_used,
 *		  "filesystem":string (file system type ("ext4", "btrfs", ...), optional),
 *		  "readonly":bool (read only file system flag , optional),
 *		  "growonly":bool (grow only flag, set true if the fs can only grow,
 *                     e.g. the old files will still take the space at package upgrage
 *                     because they are part of a snapshot)
 *            ] ]
 *
 * </code>
 * @param list<map> param
 * @return void
 */
YCPValue
PkgFunctions::TargetInitDU (const YCPList& dirlist)
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
	std::string filesystem;
	long long dfree = 0LL;
	long long dused = 0LL;
        zypp::DiskUsageCounter::MountPoint::HintFlags flags = zypp::DiskUsageCounter::MountPoint::NoHint;

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
	    && partmap->value(YCPString("readonly"))->isBoolean()
            && partmap->value(YCPString("readonly"))->asBoolean()->value())
	{
            y2milestone("Setting read only flag");
            flags = flags | zypp::DiskUsageCounter::MountPoint::Hint_readonly;
	}

	if (good
	    && !partmap->value(YCPString("growonly")).isNull()
	    && partmap->value(YCPString("growonly"))->isBoolean()
            && partmap->value(YCPString("growonly"))->asBoolean()->value())
	{
            y2milestone("Setting grow only flag");
            flags = flags | zypp::DiskUsageCounter::MountPoint::Hint_growonly;
	}

	if (good
	    && !partmap->value(YCPString("filesystem")).isNull()
	    && partmap->value(YCPString("filesystem"))->isString())
	{
	    filesystem = partmap->value(YCPString("filesystem"))->asString()->value();
	}

	if (!good)
	{
	    y2error ("TargetDUInit: bad item %d: %s", i, dirlist->value(i)->toString().c_str());
	    continue;
	}

	y2milestone("Adding %s", dname.c_str());

	long long totalsize = dfree + dused;

        long long bsize = 4096LL;
        long long pkg_size = 0LL;
	zypp::DiskUsageCounter::MountPoint mpoint(dname, filesystem, bsize,
                totalsize, dused, pkg_size, flags);

	mount_points.insert(mpoint);
    }

    try
    {
	zypp_ptr()->setPartitions(mount_points);
    }
    catch(const zypp::Exception &excpt)
    {
        y2error("Setting disk usage failed: %s", excpt.asString().c_str());
        _last_error.setLastError(ExceptionAsString(excpt));
        // TODO FIXME: change return type to boolean to allow error reporting
        return YCPVoid();
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
PkgFunctions::TargetGetDU ()
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
    catch(const zypp::Exception &excpt)
    {
        y2error("Reading disk usage failed: %s", excpt.asString().c_str());
        _last_error.setLastError(ExceptionAsString(excpt));
        return YCPVoid();
    }

    return dirmap;
}

