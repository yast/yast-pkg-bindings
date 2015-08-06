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

#include <ostream>

#include "PkgFunctions.h"
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
   @param kind_r kind of preselected patches, can be `all, `interactive, `reboot_needed, `relogin_needed or `affects_pkg_manager
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
   @param kind_r kind of preselected patches, can be `all, `interactive, `reboot_needed, `relogin_needed or `affects_pkg_manager
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
	 && kind != "affects_pkg_manager" && kind != "relogin_needed")
    {
	return YCPError("Pkg::ResolvablePreselectPatches: Wrong parameter '" + kind + "', use: `all, `interactive, `reboot_needed, `relogin_needed or `affects_pkg_manager", YCPInteger(0LL));
    }

    y2milestone("Required kind of patches: %s", kind.c_str());

    long long needed_patches = 0LL;

    try
    {
	// access to the Pool of Selectables
	zypp::ResPoolProxy selectablePool(zypp::ResPool::instance().proxy());
	      
	// Iterate it's Products...
	for_(it, selectablePool.byKindBegin<zypp::Patch>(), selectablePool.byKindEnd<zypp::Patch>())
	{
	    y2milestone("Procesing patch %s", (*it)->name().c_str());
	    zypp::ui::Selectable::Ptr s = *it;

	    if (s && s->isNeeded() && !s->isUnwanted())
	    {
		zypp::Patch::constPtr patch = zypp::asKind<zypp::Patch>
		    (s->candidateObj().resolvable());

		// dont auto-install optional patches
		if (patch->category() != "optional")
		{
		    if (kind == "all"
			|| (kind == "interactive" && patch->interactive())
			|| (kind == "affects_pkg_manager" && patch->restartSuggested())
			|| (kind == "reboot_needed" && patch->rebootSuggested())
			|| (kind == "relogin_needed" && patch->reloginSuggested())
		    )
		    {
			if (preselect)
			{
			    s->setToInstall(whoWantsIt);
			}

			// count the patch
			needed_patches++;
		    }
		    else
		    {
			y2milestone("Patch %s has not required flag", s->name().c_str());
		    }
		}
		else
		{
		    y2milestone("Ignoring optional patch : %s", s->name().c_str());
		}
	    }
	}
    }
    catch (...)
    {
	y2error("An error occurred during patch selection.");
    }

    return YCPInteger(needed_patches);
}

