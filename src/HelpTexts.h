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
   Summary:     Help texts for callbacks
*/

/*
  Textdomain "pkg-bindings"
*/

#include <i18n.h>

namespace HelpTexts
{
	// help text
	static const char *load_resolvables = _("<P><BIG><B>Loading Available Packages</B></BIG></P>"
"<P>Loading available objects from the configured repositories is in progress. This "
"may take a while...</P>");


	// help text
	static const char *load_target = _("<P><BIG><B>Loading Installed Packages</B></BIG></P>"
"<P>The package manager is reading installed packages...</P>");


	// help text
	static const char *create_help = _("<P><BIG><B>Registering a New Repository</B></BIG></P>"
"<P>A new repository is being registered. The package manager is reading "
"the list of available packages in the repository...</P>");


	static const char *save_help = _("<P><BIG><B>Saving Repositories</B></BIG></P>"
"<P>The package manager is updating configured repositories...</P>");

	static const char *refresh_help = _("<P><BIG><B>Refreshing the Repository</B></BIG></P>"
"<P>The package manager is updating the repository content...</P>");
}

