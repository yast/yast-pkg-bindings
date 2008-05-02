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
   Summary:     Patch related functions
   Namespace:   Pkg
*/

#include "PkgFunctions.h"
#include "ProvideProcess.h"
#include "log.h"
#include <y2util/Y2SLog.h>

#include <sstream>

#include <ycp/YCPVoid.h>
#include <ycp/YCPInteger.h>
#include <ycp/YCPSymbol.h>

#include <zypp/Patch.h>
#include <zypp/base/Algorithm.h>

/**
   @builtin ResolvableCountPatches
   @short Count patches which will be selected by ResolvablePreselectPatches() function
   @description
   Only non-optional patches are selected (even when `all parameter is passed!)
   @param kind_r kind of preselected patches, can be `all, `interactive, `reboot_needed or `affects_pkg_manager
   @return integer number of preselected patches
*/
YCPValue
PkgFunctions::ResolvableCountPatches (const YCPSymbol& kind_r)
{
    // only count the patches
    return ResolvableSetPatches(kind_r, false);
}

/**
   @builtin ResolvablePreselectPatches
   @short Preselect patches for auto online update during the installation
   @description
   Only non-optional patches are selected (even when `all parameter is passed!)
   @param kind_r kind of preselected patches, can be `all, `interactive, `reboot_needed or `affects_pkg_manager
   @return integer number of preselected patches
*/
YCPValue
PkgFunctions::ResolvablePreselectPatches (const YCPSymbol& kind_r)
{
    // preselect the patches
    return ResolvableSetPatches(kind_r, true);
}

// helper function
YCPValue
PkgFunctions::ResolvableSetPatches (const YCPSymbol& kind_r, bool preselect)
{
    std::string kind = kind_r->symbol();

    if (kind != "all" && kind != "interactive" && kind != "reboot_needed"
	 && kind != "affects_pkg_manager")
    {
	return YCPError("Pkg::ResolvablePreselectPatches: Wrong parameter '" + kind + "', use: `all, `interactive, `reboot_needed or `affects_pkg_manager", YCPInteger(0LL));
    }

    y2milestone("Required kind of patches: %s", kind.c_str());

    std::set<zypp::PoolItem> patches_toinst;

    const zypp::ResPool & pool = zypp_ptr()->pool();

    try
    {
	zypp::ResPool::byKind_iterator
	    b = pool.byKindBegin<zypp::Patch>(),
	    e = pool.byKindEnd<zypp::Patch>(),
	    i = pool.byKindBegin<zypp::Patch>();

	for (; i != e; ++i)
	{
	    zypp::Patch::constPtr pch = zypp::asKind<zypp::Patch>(i->resolvable());
	    std::string name(pch->name());

	    y2debug("Procesing patch %s: interactive: %s, affects_pkg_manager: %s, reboot_needed: %s", name.c_str(),
		pch->interactive() ? "true" : "false",
		pch->affects_pkg_manager() ? "true" : "false",
		pch->reboot_needed() ? "true" : "false"
	    );

	    // the best patch for the current arch, no preferred version, only needed patches
	    ProvideProcess info(zypp::ZConfig::instance().systemArchitecture(), "", true);

	    // search the best version of the patch to install
	    try
	    {
		info.whoWantsIt = whoWantsIt;

		invokeOnEach( pool.byIdentBegin<zypp::Patch>(name),
			      pool.byIdentEnd<zypp::Patch>(name),
			      zypp::functor::functorRef<bool,zypp::PoolItem> (info)
		);

		if (!info.item)
		{
		    y2milestone("Patch %s is not applicable", name.c_str());
		    continue;
		}
		else
		{
		    MIL << "Best version of patch " << i->resolvable() << ": " << info.item.resolvable() << std::endl;
		}
	    }
	    catch (...)
	    {
		y2error("Search for best patch '%s' failed", name.c_str());
		continue;
	    }

	    zypp::Patch::constPtr patch = zypp::asKind<zypp::Patch>(info.item.resolvable());

	    // dont auto-install optional patches
	    if (patch->category () != "optional")
	    {
		if (kind == "all"
		    || (kind == "interactive" && patch->interactive())
		    || (kind == "affects_pkg_manager" && patch->affects_pkg_manager())
		    || (kind == "reboot_needed" && patch->reboot_needed())
		)
		{
		    // remember the patch
		    patches_toinst.insert(info.item);
		}
		else
		{
		    y2milestone("Patch (id) %s has not required flag", patch->id().c_str());
		}
	    }
	    else
	    {
		y2milestone("Ignoring optional patch (id): %s", patch->id().c_str());
	    }
	}
    }
    catch (...)
    {
	y2error("An error occurred during patch selection.");
    }

    MIL << "Found " << patches_toinst.size() << " patches" << std::endl;

    if (preselect)
    {
	std::set<zypp::PoolItem>::iterator
	    b(patches_toinst.begin()),
	    e(patches_toinst.end()),
	    i(patches_toinst.begin());

	for (i = b; i != e; ++i)
	{
	    if (i->status().setToBeInstalled(whoWantsIt))
	    {
		MIL << "Selecting patch " << i->resolvable() << std::endl;
	    }
	}
    }

    return YCPInteger(patches_toinst.size());
}

