/* ------------------------------------------------------------------------------
 * Copyright (c) 2008 Novell, Inc. All Rights Reserved.
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
   Summary:     Search the best available resolvable
   Namespace:   Pkg
*/

#ifndef ProvideProcess_h
#define ProvideProcess_h

#include <zypp/ResPool.h>
#include <zypp/ResStatus.h>
#include <zypp/Arch.h>

#include <string>

struct ProvideProcess
{
    zypp::PoolItem item;
    zypp::Arch _architecture;
    zypp::ResStatus::TransactByValue whoWantsIt;
    std::string version;
    bool onlyNeeded;

    ProvideProcess(const zypp::Arch &arch, const std::string &vers, const bool oNeeded);
//	: _architecture(arch), whoWantsIt(zypp::ResStatus::APPL_HIGH), version(vers), onlyNeeded(oNeeded);

    bool operator()( zypp::PoolItem provider );
};

#endif

