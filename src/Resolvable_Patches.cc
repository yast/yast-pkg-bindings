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

#include "PkgModuleFunctions.h"

#include <sstream>

#include <ycp/YCPVoid.h>
#include <ycp/YCPInteger.h>
#include <ycp/YCPSymbol.h>

#include <zypp/Patch.h>

/**
   @builtin ResolvableCountPatches
   @short Count patches which will be selected by ResolvablePreselectPatches() function
   @description
   Only non-optional patches are selected (even when `all parameter is passed!)
   @param kind_r kind of preselected patches, can be `all, `interactive, `reboot_needed or `affects_pkg_manager
   @return integer number of preselected patches
*/
YCPValue
PkgModuleFunctions::ResolvableCountPatches (const YCPSymbol& kind_r)
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
PkgModuleFunctions::ResolvablePreselectPatches (const YCPSymbol& kind_r)
{
    // preselect the patches
    return ResolvableSetPatches(kind_r, true);
}

// helper function
YCPValue
PkgModuleFunctions::ResolvableSetPatches (const YCPSymbol& kind_r, bool preselect)
{
    long long selected_patches = 0LL;
    std::string kind = kind_r->symbol();

    if (kind != "all" && kind != "interactive" && kind != "reboot_needed"
	 && kind != "affects_pkg_manager")
    {
	return YCPError("Pkg::ResolvablePreselectPatches: Wrong parameter '" + kind + "', use: `all, `interactive, `reboot_needed or `affects_pkg_manager", YCPInteger(0LL));
    }

    try
    {
	const zypp::ResPool & pool = zypp_ptr()->pool();

	// pseudo code from
	// http://svn.suse.de/trac/zypp/wiki/ZMD/YaST/update/yast

	zypp::ResPool::const_iterator
	    b = pool.begin (),
	    e = pool.end (),
	    i;
	for (i = b; i != e; ++i)
	{
	    zypp::Patch::constPtr pch = zypp::asKind<zypp::Patch>(i->resolvable());

	    // is it a patch?
	    if (pch)
	    {
	    	if (i->status().isNeeded())	// uninstalled
		{
		    // dont auto-install optional patches
		    if (pch->category () != "optional")
		    {
			if (kind == "all"
			    || (kind == "interactive" && pch->interactive())
			    || (kind == "affects_pkg_manager" && pch->affects_pkg_manager())
			    || (kind == "reboot_needed" && pch->reboot_needed())
			)
			{
			    if (!preselect)
			    {
				selected_patches++;
			    }
			    else if (DoProvideNameKind(pch->name(), pch->kind(), pch->arch(),"",
						       true // only Needed patches
						       ))
				// schedule for installation
				// but take the best edition. Bug #206927
			    {
				std::ostringstream str; 
				str << *i << std::endl;
				y2milestone( "Setting '%s' to transact", str.str().c_str() );
				// selected successfully - increase the counter
				selected_patches++;
			    }
			}
			else
			{
			    y2milestone("Ignoring patch id: %s", pch->id().c_str());
			}
		    }
		    else
		    {
			y2milestone("Ignoring optional patch (id): %s", pch->id().c_str());
		    }
		}
		else
		{
		    y2milestone("Patch %s is not applicable", pch->id().c_str());
		}
	    }
	}
    }
    catch (...)
    {
	y2error("An error occurred during patch selection.");
    }

    return YCPInteger(selected_patches);
}



