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
   Summary:     Install or remove resolvables
   Namespace:   Pkg
*/

#include "PkgFunctions.h"
#include "log.h"

#include <ycp/YCPVoid.h>
#include <ycp/YCPBoolean.h>
#include <ycp/YCPSymbol.h>
#include <ycp/YCPString.h>
#include <ycp/YCPInteger.h>

/**
   @builtin ResolvableInstallArchVersion
   @short Install all resolvables with selected name, architecture and kind. Use it only in a special case, ResolvableInstall() should be prefrerred.
   @param name_r name of the resolvable
   @param kind_r kind of resolvable, can be `product, `patch, `package, `srcpackage or `pattern
   @param arch architecture of the resolvable, empty means the best architecture
   @param vers Required version of the resolvable, empty string means the best version
   @return boolean false if failed
*/
YCPValue
PkgFunctions::ResolvableInstallArchVersion( const YCPString& name_r, const YCPSymbol& kind_r, const YCPString& arch, const YCPString& vers )
{
    std::string name(name_r.isNull() ? "" : name_r->value());

    if (name.empty())
    {
	y2error("Empty name");
	return YCPBoolean(false);
    }

    zypp::Resolvable::Kind kind;
    
    std::string req_kind = kind_r->symbol ();

    std::string arch_str = arch->value();
    if (arch_str.empty())
	return YCPBoolean (false);

    // ensure installation of the required architecture
    zypp::Arch architecture(arch_str);

    if( req_kind == "product" ) {
	kind = zypp::ResKind::product;
    }
    else if ( req_kind == "patch" ) {
    	kind = zypp::ResKind::patch;
    }
    else if ( req_kind == "package" ) {
	kind = zypp::ResKind::package;
    }
    else if ( req_kind == "srcpackage" ) {
	kind = zypp::ResKind::srcpackage;
    }
    else if ( req_kind == "pattern" ) {
	kind = zypp::ResKind::pattern;
    }
    else
    {
	y2error("Pkg::ResolvableInstall: unknown symbol: %s", req_kind.c_str());
	return YCPBoolean(false);
    }

    std::string version_str = vers->value();
    bool ret = false;

    zypp::ResPoolProxy selectablePool(zypp::ResPool::instance().proxy());
    zypp::ui::Selectable::Ptr s = zypp::ui::Selectable::get(kind, name);

    if (s)
    {
	// install the required version and arch
	for_(avail_it, s->availableBegin(), s->availableEnd())
	{
	    // get the resolvable
	    zypp::Resolvable::constPtr res = avail_it->resolvable();

	    // check version and arch
	    if (res->arch() == architecture && res->edition() == version_str)
	    {
		// install the preselected candidate
		s->setCandidate(*avail_it);
		ret = s->setToInstall(whoWantsIt);
		break;
	    }
	}

	if (!ret)
	{
	    y2error("Required version and arch of %s:%s was not found", req_kind.c_str(), name.c_str());
	}
    }
    else
    {
	y2error("Required selectable %s:%s was not found", req_kind.c_str(), name.c_str());
    }

    return YCPBoolean(ret);
}

// helper function, returns true on success
bool InstallSelectableFromRepo(zypp::ui::Selectable::Ptr s, const std::string &alias)
{
    bool ret = false;

    if (s)
    {
	// install from the required repository
	for_(avail_it, s->availableBegin(), s->availableEnd())
	{
	    // get the resolvable
	    zypp::ResObject::constPtr res = *avail_it;

	    // check repository
	    if (res && res->repoInfo().alias() == alias)
	    {
		// install the preselected candidate
		s->setCandidate(res);
		ret = s->setToInstall(zypp::ResStatus::APPL_HIGH);
		break;
	    }
	}
    }

    return ret;
}

/**
   @builtin ResolvableInstallRepo
   @short Install all resolvables with selected name, from the specified repository
   @param name_r name of the resolvable, if empty ("") install all resolvables of the kind 
   @param kind_r kind of resolvable, can be `product, `patch, `package, `srcpackage or `pattern
   @param repo_r ID of the repository
   @return boolean false if failed
*/
YCPValue
PkgFunctions::ResolvableInstallRepo( const YCPString& name_r, const YCPSymbol& kind_r, const YCPInteger& repo_r )
{
    zypp::Resolvable::Kind kind;
    
    std::string req_kind = kind_r->symbol ();

    if( req_kind == "product" ) {
	kind = zypp::ResKind::product;
    }
    else if ( req_kind == "patch" ) {
    	kind = zypp::ResKind::patch;
    }
    else if ( req_kind == "package" ) {
	kind = zypp::ResKind::package;
    }
    else if ( req_kind == "srcpackage" ) {
	kind = zypp::ResKind::srcpackage;
    }
    else if ( req_kind == "pattern" ) {
	kind = zypp::ResKind::pattern;
    }
    else
    {
	y2error("Pkg::ResolvableInstallRepo: unknown symbol: %s", req_kind.c_str());
	return YCPBoolean(false);
    }

    if (repo_r.isNull())
    {
	y2error("Required repository is 'nil'");
	return YCPBoolean(false);
    }

    long long repo_id = repo_r->value();
    YRepo_Ptr repo = logFindRepository(repo_id);
    if (!repo)
	return YCPVoid();

    std::string alias = repo->repoInfo().alias();

    bool ret;

    try
    {
	std::string name(name_r.isNull() ? "" : name_r->value());

	if (name.empty())
	{
	    ret = true;
	    for (zypp::ResPoolProxy::const_iterator it = zypp_ptr()->poolProxy().byKindBegin(kind);
		it != zypp_ptr()->poolProxy().byKindEnd(kind);
		++it)
	    {
		ret = InstallSelectableFromRepo(*it, alias) && ret;
	    }
	}
	else
	{
	    zypp::ui::Selectable::Ptr s = zypp::ui::Selectable::get(kind, name);

	    ret = InstallSelectableFromRepo(s, alias);

	    if (!ret)
	    {
		y2error("Resolvable %s:%s from repository %lld (%s) was not found", req_kind.c_str(), name.c_str(),
		    repo_id, alias.c_str());
	    }
	}
    }
    catch (...)
    {
	return YCPBoolean(false);
    }

    return YCPBoolean(ret);
}

bool ResolvableInstallOrDelete(const YCPString& name_r, const YCPSymbol& kind_r, bool install)
{
    zypp::Resolvable::Kind kind;
    
    std::string req_kind = kind_r->symbol ();

    if( req_kind == "product" ) {
	kind = zypp::ResKind::product;
    }
    else if ( req_kind == "patch" ) {
    	kind = zypp::ResKind::patch;
    }
    else if ( req_kind == "package" ) {
	kind = zypp::ResKind::package;
    }
    else if ( req_kind == "srcpackage" ) {
	kind = zypp::ResKind::srcpackage;
    }
    else if ( req_kind == "pattern" ) {
	kind = zypp::ResKind::pattern;
    }
    else
    {
	y2error("Unknown symbol: %s", req_kind.c_str());
	return false;
    }

    std::string name(name_r.isNull() ? "" : name_r->value());

    if (name.empty())
    {
	y2error("Empty resolvable name");
	return false;
    }

    zypp::ui::Selectable::Ptr s = zypp::ui::Selectable::get(kind, name);

    if (s)
    {
	y2milestone("%s: %s:%s ", install ? "Installing" : "Removing", req_kind.c_str(), name.c_str());
	return install ? s->setToInstall(zypp::ResStatus::APPL_HIGH):
	    s->setToDelete(zypp::ResStatus::APPL_HIGH);
    }
    else
    {
	y2error("Resolvable %s:%s was not found", req_kind.c_str(), name.c_str());
    }

    return false;
}

// ------------------------
/**
   @builtin ResolvableInstall
   @short Install all resolvables with selected name and kind
   @param name_r name of the resolvable 
   @param kind_r kind of resolvable, can be `product, `patch, `package, `srcpackage or `pattern
   @return boolean false if failed
*/
YCPValue
PkgFunctions::ResolvableInstall( const YCPString& name_r, const YCPSymbol& kind_r )
{
    // true = install
    return YCPBoolean(ResolvableInstallOrDelete(name_r, kind_r, true));
}

// ------------------------
/**
   @builtin ResolvableRemove
   @short Removes all resolvables with selected name and kind
   @param name_r name of the resolvable
   @param kind_r kind of resolvable, can be `product, `patch, `package, `srcpackage or `pattern
   @return boolean false if failed
*/
YCPValue
PkgFunctions::ResolvableRemove ( const YCPString& name_r, const YCPSymbol& kind_r )
{
    // false = remove
    return YCPBoolean(ResolvableInstallOrDelete(name_r, kind_r, false));
}

// ------------------------
/**
   @builtin ResolvableNeutral
   @short Remove all transactions from all resolvables with selected name and kind
   @param name_r name of the resolvable, if empty ("") use all resolvables of the kind 
   @param kind_r kind of resolvable, can be `product, `patch, `package, `srcpackage or `pattern
   @param force_r remove the transactions even on USER level - default is APPL_HIGH (use true value only if really needed!)
   @return boolean false if failed
*/
YCPValue
PkgFunctions::ResolvableNeutral ( const YCPString& name_r, const YCPSymbol& kind_r, const YCPBoolean& force_r )
{
    zypp::Resolvable::Kind kind;
    
    std::string req_kind = kind_r->symbol();
    std::string name = name_r->value();
    bool force = force_r->value();

    if( req_kind == "product" ) {
	kind = zypp::ResKind::product;
    }
    else if ( req_kind == "patch" ) {
    	kind = zypp::ResKind::patch;
    }
    else if ( req_kind == "package" ) {
	kind = zypp::ResKind::package;
    }
    else if ( req_kind == "srcpackage" ) {
	kind = zypp::ResKind::srcpackage;
    }
    else if ( req_kind == "pattern" ) {
	kind = zypp::ResKind::pattern;
    }
    else
    {
	y2error("Pkg::ResolvableNeutral: unknown symbol: %s", req_kind.c_str());
	return YCPBoolean(false);
    }

    bool ret = true;

    try
    {
	if (name.empty())
	{
	    for (zypp::ResPoolProxy::const_iterator it = zypp_ptr()->poolProxy().byKindBegin(kind);
		it != zypp_ptr()->poolProxy().byKindEnd(kind);
		++it)
	    {
		ret = (*it)->unset(force ? zypp::ResStatus::USER : whoWantsIt) && ret;
	    }
	}
	else
	{
	    zypp::ui::Selectable::Ptr s = zypp::ui::Selectable::get(kind, name);
	    ret = s ? s->unset(force ? zypp::ResStatus::USER : whoWantsIt) : false;
	}
    }
    catch (...)
    {
	return YCPBoolean(false);
    }

    return YCPBoolean(ret);
}

// ------------------------
/**
   @builtin ResolvableSetSoftLock
   @short Soft lock - it prevents the solver from re-selecting item
   if it's recommended (if it's required it will be selected).
   @param name_r name of the resolvable, if empty ("") use all resolvables of the kind 
   @param kind_r kind of resolvable, can be `product, `patch, `package, `srcpackage or `pattern
   @return boolean false if failed
*/
YCPValue
PkgFunctions::ResolvableSetSoftLock ( const YCPString& name_r, const YCPSymbol& kind_r )
{
    zypp::Resolvable::Kind kind;
    
    std::string req_kind = kind_r->symbol();
    std::string name = name_r->value();

    if( req_kind == "product" ) {
	kind = zypp::ResKind::product;
    }
    else if ( req_kind == "patch" ) {
    	kind = zypp::ResKind::patch;
    }
    else if ( req_kind == "package" ) {
	kind = zypp::ResKind::package;
    }
    else if ( req_kind == "srcpackage" ) {
	kind = zypp::ResKind::package;
    }
    else if ( req_kind == "pattern" ) {
	kind = zypp::ResKind::pattern;
    }
    else
    {
	y2error("Pkg::ResolvableSetSoftLock: unknown symbol: %s", req_kind.c_str());
	return YCPBoolean(false);
    }

    bool ret = true;

    try
    {
	if (name.empty())
	{
	    for (zypp::ResPoolProxy::const_iterator it = zypp_ptr()->poolProxy().byKindBegin(kind);
		it != zypp_ptr()->poolProxy().byKindEnd(kind);
		++it)
	    {
		ret = (*it)->theObj().status().setSoftLock(whoWantsIt) && ret;
	    }
	}
	else
	{
	    zypp::ui::Selectable::Ptr s = zypp::ui::Selectable::get(kind, name);
	    ret = s ? s->theObj().status().setSoftLock(whoWantsIt) : false;
	}
    }
    catch (...)
    {
	return YCPBoolean(false);
    }

    return YCPBoolean(ret);
}

