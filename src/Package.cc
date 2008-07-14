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

   File:	Package.cc

   Author:	Klaus Kaempf <kkaempf@suse.de>
   Maintainer:  Klaus Kaempf <kkaempf@suse.de>
   Namespace:   Pkg

   Summary:	Access to packagemanager
   Purpose:     Handles package related Pkg::function (list_of_arguments) calls
		from WFMInterpreter.
/-*/

#include "PkgFunctions.h"
#include "ProvideProcess.h"
#include "log.h"

#include <ycp/YCPVoid.h>
#include <ycp/YCPBoolean.h>
#include <ycp/YCPInteger.h>
#include <ycp/YCPSymbol.h>
#include <ycp/YCPString.h>
#include <ycp/YCPList.h>
#include <ycp/YCPMap.h>

#include <ycp/Type.h>

#include <zypp/base/Algorithm.h>
#include <zypp/ResFilters.h>
#include <zypp/ResStatus.h>

#include <zypp/ResPool.h>
#include <zypp/UpgradeStatistics.h>
#include <zypp/target/rpm/RpmDb.h>
#include <zypp/target/TargetException.h>
#include <zypp/ZYppCommit.h>
#include <zypp/ZConfig.h>

#include <zypp/sat/WhatProvides.h>

#include <fstream>
#include <sstream>

///////////////////////////////////////////////////////////////////

namespace zypp {
	typedef std::list<PoolItem> PoolItemList;
}

// ------------------------
/**
 *  @builtin PkgQueryProvides
 *  @short List all package instances providing 'tag'
 *  @description
 *  A package instance is itself a list of three items:
 *
 *  - string name: The package name
 *
 *  - symbol instance: Specifies which instance of the package contains a match.
 *
 *      'NONE no match
 *
 *      'INST the installed package
 *
 *      'CAND the candidate package
 *
 *      'BOTH both packages
 *
 *  - symbol onSystem: Tells which instance of the package would be available
 *    on the system, if PkgCommit was called right now. That way you're able to
 *    tell whether the tag will be available on the system after PkgCommit.
 *    (e.g. if onSystem != 'NONE && ( onSystem == instance || instance == 'BOTH ))
 *
 *      'NONE stays uninstalled or is deleted
 *
 *      'INST the installed one remains untouched
 *
 *      'CAND the candidate package will be installed
 *
 *  @param string tag
 *  @return list of all package instances providing 'tag'.
 *  @usage Pkg::PkgQueryProvides( string tag ) -> [ [string, symbol, symbol], [string, symbol, symbol], ...]
 */
YCPList
PkgFunctions::PkgQueryProvides( const YCPString& tag )
{
    YCPList ret;
    std::string name = tag->value();

    zypp::Capability cap(name, zypp::ResKind::package);
    zypp::sat::WhatProvides possibleProviders(cap);

    for_(iter, possibleProviders.begin(), possibleProviders.end())
    {
	zypp::PoolItem provider = zypp::ResPool::instance().find(*iter);

	// cast to Package object
	zypp::Package::constPtr package = zypp::dynamic_pointer_cast<const zypp::Package>(provider.resolvable());

	if (!package)
	{
	    y2internal("Casting to Package failed!");
	    continue;
	}

	std::string pkgname = package->name();

	// get instance status
	bool installed = provider.status().staysInstalled();
	std::string instance;

# warning Status `NONE and `INST is not supported
	// TODO FIXME the other values??
	if (installed)
	{
	    instance = "BOTH";
	}
	else
	{
	    instance = "CAND";
	}

	// get status on the system
	bool uninstalled = provider.status().staysUninstalled() || provider.status().isToBeUninstalled();
	std::string onSystem;

	if (uninstalled)
	{
	    onSystem = "NONE";
	}
	else if (installed)
	{
	    onSystem = "INST";
	}
	else
	{
	    onSystem = "CAND";
	}

	// create list item
	YCPList item;
	item->add(YCPString(pkgname));
	item->add(YCPSymbol(instance));
	item->add(YCPSymbol(onSystem));

	// add the item to the result
	ret->add(item);
    }

    return ret;
}

///////////////////////////////////////////////////////////////////

/******************************************************************
**
**
**	FUNCTION NAME : join
**	FUNCTION TYPE : string
*/
inline std::string join( const std::list<std::string> & lines_r, const std::string & sep_r = "\n" )
{
  std::string ret;
  std::list<std::string>::const_iterator it = lines_r.begin();

  if ( it != lines_r.end() ) {
    ret = *it;
    for( ++it; it != lines_r.end(); ++it ) {
      ret += sep_r + *it;
    }
  }

  return ret;
}


// ------------------------
/**
 *  @builtin PkgMediaNames
 *  @short Return names of sources in installation order
 *  @return list< list<any> > Names and ids of Sources
 *  @usage Pkg::PkgMediaNames () -> [ ["source_1_name", source_1_id] , ["source_2_name", source_2_id], ...]
 */
YCPValue
PkgFunctions::PkgMediaNames ()
{
# warning No installation order
    YCPList res;

    RepoId index = 0;
    // for all enabled sources
    for( RepoCont::const_iterator repoit = repos.begin();
	repoit != repos.end(); ++repoit,++index)
    {
	if ((*repoit)->repoInfo().enabled() && !(*repoit)->isDeleted())
	{
	    try
	    {
		std::string repo_name = (*repoit)->repoInfo().name();
		YCPList src_desc;

		if (repo_name.empty())
		{
		    y2warning("Name of repository '%zd' is empty, using URL", index);

		    // use URL as the product name
		    std::string name;
		    if ((*repoit)->repoInfo().baseUrlsBegin() != (*repoit)->repoInfo().baseUrlsEnd())
		    {
			name = (*repoit)->repoInfo().baseUrlsBegin()->asString();
		    }

		    // use alias if url is unknown
		    if (name.empty())
		    {
			name = (*repoit)->repoInfo().alias();
		    }

		    src_desc->add( YCPString( name ));
		    src_desc->add( YCPInteger( index ) );

		    res->add( src_desc );
		}
		else
		{
		    src_desc->add( YCPString( repo_name ));
		    src_desc->add( YCPInteger( index ) );

		    res->add( src_desc );
		}
	    }
	    catch (...)
	    {
		return res;
	    }
	}
    }

    y2milestone( "Pkg::PkgMediaNames result: %s", res->toString().c_str());

    return res;
}



YCPValue
PkgFunctions::PkgMediaSizesOrCount (bool sizes, bool download_size)
{
    // all enabled sources
    std::list<RepoId> source_ids;

    RepoId index = 0;
    for(RepoCont::const_iterator it = repos.begin(); it != repos.end() ; ++it, ++index)
    {
	// ignore disabled or delted sources
	if (!(*it)->repoInfo().enabled() || (*it)->isDeleted())
	    continue;

	source_ids.push_back(index);
    }


    // map SourceId -> [ number_of_media, total_size ]
    std::map<RepoId, std::vector<zypp::ByteCount> > result;

    // map alias -> SourceID
    std::map<std::string, RepoId> source_map;

    // initialize the structures
    for( std::list<RepoId>::const_iterator sit = source_ids.begin();
	sit != source_ids.end(); ++sit, ++index)
    {
	RepoId id = *sit;

	YRepo_Ptr repo = logFindRepository(id);
	if (!repo)
	    continue;

	// we don't know the number of media in advance
	// the vector is dynamically resized during package search
	result[id] = std::vector<zypp::ByteCount>();
	source_map[ repo->repoInfo().alias() ] = id;
    }

    for( zypp::ResPool::byKind_iterator it = zypp_ptr()->pool().byKindBegin<zypp::Package>()
	; it != zypp_ptr()->pool().byKindEnd<zypp::Package>()
	; ++it )
    {
	zypp::Package::constPtr pkg = boost::dynamic_pointer_cast<const zypp::Package>(it->resolvable());

	if( pkg && it->status().isToBeInstalled())
	{
	    unsigned int medium = pkg->mediaNr();
	    if (medium == 0)
	    {
		medium = 1;
	    }

	    if (medium > 0)
	    {
		zypp::ByteCount size = sizes ? (download_size ? pkg->downloadSize() : pkg->installSize()) : zypp::ByteCount(1); //count only

		// refence to the found media array
		std::vector<zypp::ByteCount> &ref = result[source_map[pkg->repoInfo().alias()]];
		int add_count = (medium - 1) - ref.size() + 1;

		// resize media array - the found index is out of array
		if (add_count > 0)
		{
                    ref.insert(ref.end(), add_count, zypp::ByteCount(0));
		}

		ref[medium - 1] += size ;	// media are numbered from 1
	    }
	}
    }

    YCPList res;

    for(std::map<RepoId, std::vector<zypp::ByteCount> >::const_iterator it =
	result.begin(); it != result.end() ; ++it)
    {
	const std::vector<zypp::ByteCount> &values = it->second;
	YCPList source;

	for( unsigned i = 0 ; i < values.size() ; i++ )
	{
	    source->add( YCPInteger( values[i] ) );
	}

	res->add( source );
    }

    y2milestone( "Pkg::%s result: %s", sizes ? (download_size ? "PkgMediaPackageSizes" : "PkgMediaSizes" ): "PkgMediaCount", res->toString().c_str());

    return res;
}

// ------------------------
/**
 *  @builtin PkgMediaSizes
 *  @short Return size of packages to be installed
 *  @description
 *  return cumulated sizes (in bytes !) to be installed from different sources and media
 *
 *  Returns the install size, not the archivesize !!
 *
 *  @return list<list<integer>>
 *  @usage Pkg::PkgMediaSizes () -> [ [src1_media_1_size, src1_media_2_size, ...], ..]
 */
YCPValue
PkgFunctions::PkgMediaSizes()
{
    return PkgMediaSizesOrCount (true);
}

// ------------------------
/**
 *  @builtin PkgMediaPackageSizes
 *  @short Return size of packages to be installed
 *  @description
 *  return cumulated sizes (in bytes !) to be installed from different sources and media
 *
 *  Returns the archive sizes!
 *
 *  @return list<list<integer>>
 *  @usage Pkg::PkgMediaPackageSizes () -> [ [src1_media_1_size, src1_media_2_size, ...], ..]
 */
YCPValue
PkgFunctions::PkgMediaPackageSizes()
{
    return PkgMediaSizesOrCount (true, true);
}

// ------------------------
/**
 *  @builtin PkgMediaCount
 *  @short Return number of packages to be installed
 *  @description
 *    return number of packages to be installed from different sources and media
 *
 *  @return list<list<integer>>
 *  @usage Pkg::PkgMediaCount() -> [ [src1_media_1_count, src1_media_2_count, ...], ...]
 */
YCPValue
PkgFunctions::PkgMediaCount()
{
    return PkgMediaSizesOrCount (false);
}

// ------------------------
/**
 *  @builtin IsProvided
 *
 *  @short returns  'true' if the tag is provided by a package in the installed system
 *  @description
 *  tag can be a package name, a string from requires/provides
 *  or a file name (since a package implictly provides all its files)
 *
 *  @param string tag
 *  @return boolean
 *  @usage Pkg::IsProvided ("glibc") -> true
*/
YCPValue
PkgFunctions::IsProvided (const YCPString& tag)
{
    std::string name = tag->value ();
    if (name.empty())
	return YCPBoolean (false);

    // look for packages
    zypp::Capability cap(name, zypp::ResKind::package);
    zypp::sat::WhatProvides possibleProviders(cap);

    for_(iter, possibleProviders.begin(), possibleProviders.end())
    {
	zypp::PoolItem provider = zypp::ResPool::instance().find(*iter);

	// is it installed?
	if (provider.status().isInstalled())
	{
	    y2milestone("Tag %s is provided by %s", name.c_str(), provider->name().c_str());
	    return YCPBoolean(true);
	}
    }

    y2milestone("Tag %s is not provided", name.c_str());

    return YCPBoolean(false);
}


// ------------------------
/**
 *  @builtin IsSelected
 *  @short  returns a 'true' if the tag is selected for installation
 *  @description
 *
 *  tag can be a package name, a string from requires/provides
 *  or a file name (since a package implictly provides all its files)
 *  @param string tag
 *  @return boolean
 *  @usage Pkg::IsSelected ("yast2") -> true
*/
YCPValue
PkgFunctions::IsSelected (const YCPString& tag)
{
    std::string name = tag->value ();
    if (name.empty())
	return YCPBoolean (false);

    // look for packages
    zypp::Capability cap(name, zypp::ResKind::package);
    zypp::sat::WhatProvides possibleProviders(cap);

    for_(iter, possibleProviders.begin(), possibleProviders.end())
    {
	zypp::PoolItem provider = zypp::ResPool::instance().find(*iter);

	// is it installed?
	if (provider.status().isToBeInstalled())
	{
	    y2milestone("Tag %s provided by %s is selected to install", name.c_str(), provider->name().c_str());
	    return YCPBoolean(true);
	}
    }

    y2milestone("Tag %s is not selected to install", name.c_str());

    return YCPBoolean(false);
}

// ------------------------
/**
 *  @builtin IsAvailable
 *  @short Check if package (tag) is available
 *  @description
 *  returns a 'true' if the tag is available on any of the currently
 *  active installation sources. (i.e. it is installable)

 *  tag can be a package name, a string from requires/provides
 *  or a file name (since a package implictly provides all its files)
 *
 *  @param string tag
 *  @return boolean
 *  @usage Pkg::IsAvailable ("yast2") -> true
*/
YCPValue
PkgFunctions::IsAvailable (const YCPString& tag)
{
    std::string name = tag->value ();
    if (name.empty())
	return YCPBoolean (false);

    // look for packages
    zypp::Capability cap(name, zypp::ResKind::package);
    zypp::sat::WhatProvides possibleProviders(cap);

    for_(iter, possibleProviders.begin(), possibleProviders.end())
    {
	zypp::PoolItem provider = zypp::ResPool::instance().find(*iter);

	// is it installed?
	if (!provider.status().isInstalled())
	{
	    y2milestone("Tag %s provided by %s is available to install", name.c_str(), provider->name().c_str());
	    return YCPBoolean(true);
	}
    }

    y2milestone("Tag %s is not available to install", name.c_str());

    return YCPBoolean(false);
}

YCPValue
PkgFunctions::searchPackage(const YCPString &package, bool installed)
{
    bool found = false;

    std::string pkgname = package->value();

    if (pkgname.empty())
    {
	y2warning("Pkg::%s: Package name is empty", installed ? "PkgInstalled" : "PkgAvailable");
	return YCPBoolean(false);
    }


    try
    {
	for (zypp::ResPool::byIdent_iterator it = zypp_ptr()->pool().byIdentBegin<zypp::Package>(pkgname);
	    it != zypp_ptr()->pool().byIdentEnd<zypp::Package>(pkgname);
	    ++it)
	{
	    zypp::Package::constPtr pkg = zypp::asKind<zypp::Package>( it->resolvable() );

	    if (pkg != NULL)
	    {
		if (installed == pkg->isSystem())
		{
		    found = true;
		    break;
		}
	    }
	}
    }
    catch (...)
    {
    }

    y2milestone("Package '%s' %s: %s", pkgname.c_str(), installed ? "installed" : "available", found ? "true" : "false");

    return YCPBoolean(found);
}

/**
 *  @builtin PkgInstalled
 *
 *  @short returns 'true' if the package is installed in the system
 *  @description
 *  tag can be a package name, a string from requires/provides
 *  or a file name (since a package implictly provides all its files)
 *
 *  @param string package name of the package
 *  @return boolean
 *  @usage Pkg::PkgInstalled("glibc") -> true
*/
YCPValue
PkgFunctions::PkgInstalled(const YCPString& package)
{
    return searchPackage(package, true);
}

// ------------------------
/**
 *  @builtin PkgAvailable
 *  @short Check if package is available
 *  @description
 *  returns 'true' if the package is available on any of the currently
 *  active installation sources. (i.e. it is installable)
 *
 *  @param string package name of the package
 *  @return boolean
 *  @usage Pkg::PkgInstalled("yast2") -> true
*/
YCPValue
PkgFunctions::PkgAvailable(const YCPString& package)
{
    return searchPackage(package, false);
}

/**
 * helper function, install a resolvable with a specific name and kind
*/

bool
PkgFunctions::DoProvideNameKind( const std::string & name, zypp::Resolvable::Kind kind, zypp::Arch architecture,
				       const std::string & version ,const bool onlyNeeded, int repo_id)
{
    try
    {
	std::string repo_alias;

	if (repo_id >= 0)
	{
	    YRepo_Ptr repo = logFindRepository(repo_id);
	    if (!repo)
	    {
		y2warning("Repository %d not found", repo_id);
		return false;
	    }

	    repo_alias = repo->repoInfo().alias();
	    y2milestone("Installing only from repository '%s'", repo_alias.c_str());
	}

	ProvideProcess info( architecture, version, onlyNeeded, repo_alias);
	info.whoWantsIt = whoWantsIt;

	invokeOnEach( zypp_ptr()->pool().byIdentBegin( kind, name ),
		      zypp_ptr()->pool().byIdentEnd( kind, name ),
		      zypp::functor::functorRef<bool,zypp::PoolItem> (info)
	);

	if (!info.item)
	    return false;

	bool result = info.item.status().setToBeInstalled( whoWantsIt );
	if (!result) {
	    std::ostringstream os;
	    os << info.item;
	    y2milestone( "DoProvideNameKind failed for %s\n", os.str().c_str() );
	}
	return true;
    }
    catch (...)
    {
    }

    return false;
}

/*
 * Helper function
 */
bool
PkgFunctions::DoAllKind(zypp::Resolvable::Kind kind, bool provide)
{
    bool ret = true;

    try
    {
	for (zypp::ResPool::byKind_iterator it = zypp_ptr()->pool().byKindBegin(kind);
	    it != zypp_ptr()->pool().byKindEnd(kind); ++it)
	{
	    bool res = provide ? it->status().setToBeInstalled( whoWantsIt )
		: (it->status().isInstalled() && it->status().setToBeUninstalled( whoWantsIt ));

	    y2milestone ("%s %s -> %s\n", (provide ? "Install" : "Remove"), (*it)->name().c_str(), (res ? "Ok" : "Failed"));
	    ret = ret && res;
	}
    }
    catch (...)
    {
	ret = false;
    }

    return ret;
}

bool
PkgFunctions::DoProvideAllKind(zypp::Resolvable::Kind kind)
{
    return DoAllKind(kind, true);
}

bool
PkgFunctions::DoRemoveAllKind(zypp::Resolvable::Kind kind)
{
    return DoAllKind(kind, false);
}



/**
 * helper function, deinstall a kind which provides tag (as a
 *   kind name, a provided tag, or a provided file
 *
 * !!!!**** WARNING: This is different from DoProvideNameKind ****!!!!
*/

bool
PkgFunctions::DoRemoveNameKind( const std::string & name, zypp::Resolvable::Kind kind)
{
    zypp::Capability cap(name, kind);
    zypp::sat::WhatProvides possibleProviders(cap);

    bool ret = false;

    for_(iter, possibleProviders.begin(), possibleProviders.end())
    {
	zypp::PoolItem provider = zypp::ResPool::instance().find(*iter);

	// get instance status
	bool installed = provider.status().isInstalled();

	if (installed)
	{
	    bool result = provider.status().setToBeUninstalled(whoWantsIt);
	    y2milestone("DoRemoveNameKind %s -> %s\n", name.c_str(), (result ? "Ok" : "Bad"));
	    ret = true;
	}
    }

    return ret;
}

// ------------------------
/**
   @builtin DoProvide
   @short Install a list of packages to the system
   @description
   Provides (read: installs) a list of tags to the system

   tag is a package name

   returns a map of tag,reason pairs if tags could not be provided.
   Usually this map should be empty (all required packages are
   installed)

   If tags could not be provided (due to package install failures or
   conflicts), the tag is listed as a key and the value describes
   the reason for the failure (as an already translated string).
   @param list tags
   @return map
*/
YCPValue
PkgFunctions::DoProvide (const YCPList& tags)
{
    YCPMap ret;
    if (tags->size() > 0)
    {
        for (int i = 0; i < tags->size(); ++i)
        {
            if (tags->value(i)->isString())
            {
                DoProvideNameKind (tags->value(i)->asString()->value(), zypp::ResTraits<zypp::Package>::kind, zypp::ZConfig::instance().systemArchitecture(), "");
            }
            else
            {
                y2error ("Pkg::DoProvide not string '%s'", tags->value(i)->toString().c_str());
            }
        }
    }
# warning error handling - return value
    return ret;
}


// ------------------------
/**
   @builtin DoRemove

   @short Removes a list of packges from the system
   @description
   tag is a package name

   returns a map of tag,reason pairs if tags could not be removed.
   Usually this map should be empty (all required packages are
   removed)

   If a tag could not be removed (because other packages still
   require it), the tag is listed as a key and the value describes
   the reason for the failure (as an already translated string).
   @param list tags
   @return list Result
*/
YCPValue
PkgFunctions::DoRemove (const YCPList& tags)
{
    YCPMap ret;
    if (tags->size() > 0)
    {
	for (int i = 0; i < tags->size(); ++i)
	{
	    if (tags->value(i)->isString())
	    {
		DoRemoveNameKind( tags->value(i)->asString()->value(), zypp::ResTraits<zypp::Package>::kind);
	    }
	    else
	    {
		y2error ("Pkg::DoRemove not string '%s'", tags->value(i)->toString().c_str());
	    }
	}
    }
    return ret;
}

// ------------------------

struct ItemMatch
{
    zypp::PoolItem item;

    bool operator() (const zypp::PoolItem i )
    {
	item = i;
	return false;
    }
};

static zypp::Package::constPtr
find_package( const string & name, zypp::PoolItem & item, const zypp::ResPool &pool )
{
    if (name.empty())
	return NULL;

    ItemMatch match;

    // look for uninstalled packages first, because they probably have
    // translated summary

    invokeOnEach( pool.byIdentBegin<zypp::Package>( name ),
		  pool.byIdentEnd<zypp::Package>( name ),
		  zypp::resfilter::ByUninstalled(),
		  zypp::functor::functorRef<bool,zypp::PoolItem>( match ) );

    // if no match yet, try again with installed package
    //  (but those only provide an english summary)

    if (!match.item) {
	invokeOnEach( pool.byIdentBegin<zypp::Package>(name),
		      pool.byIdentEnd<zypp::Package>(name),
		      zypp::resfilter::ByInstalled(),
		      zypp::functor::functorRef<bool,zypp::PoolItem>( match ) );
    }

    if (!match.item)
	return NULL;

    item = match.item;

    return zypp::asKind<zypp::Package>( match.item.resolvable() );
}


/**
   @builtin PkgSummary

   @short Get summary (aka label) of a package
   @param string package
   @return string Summary
   @usage Pkg::PkgSummary (string package) -> "This is a nice package"

*/
#warning This is bogus, as we have multiple matching packages

YCPValue
PkgFunctions::PkgSummary (const YCPString& p)
{
    try
    {
	zypp::PoolItem item;
	zypp::Package::constPtr pkg = find_package( p->value(), item, zypp_ptr()->pool() );

	if (pkg == NULL)
	{
	    return YCPVoid();
	}

	return YCPString( pkg->summary() );
    }
    catch (...)
    {
    }

    return YCPVoid();
}

// ------------------------
/**
   @builtin PkgVersion

   @short Get version (better: edition) of a package
   @param string package
   @return string
   @usage Pkg::PkgVersion (string package) -> "1.42-39"

*/

#warning This is bogus, as we have multiple matching packages

YCPValue
PkgFunctions::PkgVersion (const YCPString& p)
{
    try
    {
	zypp::PoolItem item;
	zypp::Package::constPtr pkg = find_package( p->value(), item, zypp_ptr()->pool() );

	if (pkg == NULL)
	{
	    return YCPVoid();
	}

	return YCPString( pkg->edition().asString() );
    }
    catch (...)
    {
    }

    return YCPVoid();
}

// ------------------------
/**
   @builtin PkgSize

   @short Get (installed) size of a package
   @param string package
   @return integer
   @usage Pkg::PkgSize (string package) -> 12345678

*/
#warning This is bogus, as we have multiple matching packages

YCPValue
PkgFunctions::PkgSize (const YCPString& p)
{
    try
    {
	zypp::PoolItem item;
	zypp::Package::constPtr pkg = find_package( p->value(), item, zypp_ptr()->pool() );

	if (pkg == NULL)
	{
	    return YCPVoid();
	}

	return YCPInteger( pkg->installSize() );
    }
    catch (...)
    {
    }

    return YCPVoid();
}

// ------------------------
/**
   @builtin PkgGroup

   @short Get rpm group of a package
   @param string package
   @return string
   @usage Pkg::PkgGroup (string package) -> string

*/
#warning This is bogus, as we have multiple matching packages

YCPValue
PkgFunctions::PkgGroup (const YCPString& p)
{
    try
    {
	zypp::PoolItem item;
	zypp::Package::constPtr pkg = find_package( p->value(), item, zypp_ptr()->pool() );

	if (pkg == NULL)
	{
	    return YCPVoid();
	}

	return YCPString( pkg->group() );
    }
    catch (...)
    {
    }

    return YCPVoid();
}


YCPValue
PkgFunctions::PkgProp( zypp::PoolItem item )
{
    YCPMap data;
    zypp::Package::constPtr pkg = zypp::asKind<zypp::Package>( item.resolvable() );
    if (pkg == NULL) {
	y2error( "Not a package: %s", item->name().c_str() );
	return data;
    }

    data->add( YCPString("arch"), YCPString( pkg->arch().asString() ) );
    data->add( YCPString("medianr"), YCPInteger( pkg->mediaNr() ) );

    long long sid = logFindAlias(pkg->repoInfo().alias());
    y2debug("srcId: %lld", sid );
    data->add( YCPString("srcid"), YCPInteger( sid ) );

    std::string status("available");
    if (item.status().isInstalled())
    {
	status = "installed";
    }
    else if (item.status().isToBeInstalled())
    {
	status = "selected";
    }
    else if (item.status().isToBeUninstalled())
    {
	status = "removed";
    }
    data->add( YCPString("status"), YCPSymbol(status));

    data->add( YCPString("location"), YCPString( pkg->location().filename().basename() ) );
    data->add( YCPString("path"), YCPString( pkg->location().filename().asString() ) );

    return data;
}

// ------------------------
/**
 * @builtin PkgProperties
 * @short Return information about a package
 * @description
 * Return Data about package location, source and which
 *  media contains the package
 *
 * <code>
 * $["srcid"    : YCPInteger,
 *   "location" : YCPString
 *   "medianr"  : YCPInteger
 *   "arch"     : YCPString
 *   ]
 * </code>
 * @param package name
 * @return map
 * @usage Pkg::PkgProperties (string package) -> map
 */

#warning This is bogus, as we have multiple matching packages

YCPValue
PkgFunctions::PkgProperties (const YCPString& p)
{
    try
    {
	YCPMap data;
	zypp::PoolItem item;
	zypp::Package::constPtr pkg = find_package( p->value(), item, zypp_ptr()->pool() );

	if (item)
	    return PkgProp( item );
    }
    catch (...)
    {
    }

    return YCPVoid();
}

YCPValue
PkgFunctions::PkgPropertiesAll (const YCPString& p)
{
    std::string pkgname = p->value();
    YCPList data;

    if (!pkgname.empty())
    {
	try
	{
	    for (zypp::ResPool::byIdent_iterator it = zypp_ptr()->pool().byIdentBegin<zypp::Package>(pkgname);
		it != zypp_ptr()->pool().byIdentEnd<zypp::Package>(pkgname); ++it)
	    {
		data->add( PkgProp( *it ) );
	    }
	}
	catch (...)
	{
	}
    }

    return data;
}

/* helper function */
YCPValue
PkgFunctions::GetPkgLocation (const YCPString& p, bool full_path)
{
    zypp::PoolItem item;

    try
    {
	zypp::Package::constPtr pkg = find_package( p->value(), item, zypp_ptr()->pool() );

	YCPMap data;

	if (item) {
	    return (full_path) ? YCPString( pkg->location().filename().asString() )
			       : YCPString( pkg->location().filename().basename() );
	}
    }
    catch (...)
    {
    }

    return YCPVoid();
}

// ------------------------
/**
   @builtin PkgLocation

   @short Get file location of a package in the source
   @param string package
   @return string Package Location
   @usage Pkg::PkgLocation (string package) -> string

*/
YCPValue
PkgFunctions::PkgLocation (const YCPString& p)
{
    return GetPkgLocation(p, false);
}


// ------------------------
/**
   @builtin PkgPath

   @short Path to a package path in the source
   @param string package name
   @return string Relative package path to source root directory
   @usage Pkg::PkgPath (string package) -> string

*/
YCPValue
PkgFunctions::PkgPath (const YCPString& p)
{
    return GetPkgLocation(p, true);
}



/**
 *  @builtin PkgGetFilelist
 *  @short Get File List of a package
 *  @description
 *  Return, if available, the filelist of package 'name'. Symbol 'which'
 *  specifies the package instance to query:
 *
 *  <code>
 *  `installed	query the installed package
 *  `candidate	query the candidate package
 *  `any	query the candidate or the installed package (dependent on what's available)
 *  </code>
 *
 *  @param string name Package Name
 *  @param symbol which Which packages
 *  @return list file list
 *
 **/
YCPList PkgFunctions::PkgGetFilelist( const YCPString & package, const YCPSymbol & which )
{
    std::string pkgname = package->value();
    std::string type = which->symbol();
    YCPList ret;

    if (type != "any" && type != "installed" && type != "candidate")
    {
	y2error("PkgGetFilelist: Unknown parameter, use `any, `installed or `candidate");
	return ret;
    }

    if (!pkgname.empty())
    {
	try
	{
	    for (zypp::ResPool::byIdent_iterator it = zypp_ptr()->pool().byIdentBegin<zypp::Package>(pkgname);
		it != zypp_ptr()->pool().byIdentEnd<zypp::Package>(pkgname); ++it)
	    {
		if (type == "any" ||
		    (type == "installed" && it->status().isInstalled()) ||
		    (type == "candidate" && it->status().isToBeInstalled())
		)
		{
		    // cast to Package object
		    zypp::Package::constPtr package = zypp::dynamic_pointer_cast<const zypp::Package>(it->resolvable());
		    std::list<std::string> files = package->filenames();

		    // insert the file names
		    for (std::list<string>::iterator it = files.begin(); it != files.end(); ++it)
		    {
			ret->add(YCPString(*it));
		    }

#warning PkgGetFilelist has different semantics for `any optinon - the result depends on item order in the pool, first found is returned
		    // finish the loop, we have found required package
		    break;
		}
	    }
	}
	catch (...)
	{
	}
    }

    return ret;
}

bool state_saved = false;

// ------------------------
/**
   @builtin SaveState
   @short Save the current selection state, can be restored later using Pkg::RestoreState()
   @description
   Save the current status of all resolvables for later restoration via Pkg::RestoreState() function. Only one state is stored, the following call will rewrite the saved status.

   @return boolean
   @see Pkg::RestoreState

*/
YCPValue
PkgFunctions::SaveState ()
{
    // a state has been already saved, it will be lost...
    if (state_saved)
    {
	y2warning("Pkg::SaveState() has been already called, rewriting the saved state...");
    }

    y2milestone("Saving status...");
    zypp_ptr()->poolProxy().saveState();
    state_saved = true;

    return YCPBoolean(true);
}

// ------------------------
/**
   @builtin RestoreState

   @short Restore Saved state - restore the state saved by Pkg::SaveState()
   @description
   restore the package selection status from a former
   call to Pkg::SaveState()

   If called with argument (true), it only checks the saved
   against the current status and returns true if they differ.
   @param boolean check_only
   @return boolean Returns false if there is no saved state (no Pkg::SaveState()
   called before)
   @see Pkg::SaveState

*/
YCPValue
PkgFunctions::RestoreState (const YCPBoolean& ch)
{
    bool ret = false;

    if (!ch.isNull () && ch->value () == true)
    {
	// check only
	ret = zypp_ptr()->poolProxy().diffState();
    }
    else
    {
	if (!state_saved)
	{
	    y2error("No previous state saved, state cannot be restored");
	}
	else
	{
	    y2milestone("Restoring the saved status...");
	    zypp_ptr()->poolProxy().restoreState();
	    ret = true;
	}
    }

    return YCPBoolean(ret);
}

// ------------------------
/**
   @builtin ClearSaveState

   @short Clear the saved state - do not use, does nothing (the saved state cannot be removed, it is part of each resolvable object)
   @return boolean

*/
YCPValue
PkgFunctions::ClearSaveState ()
{
    return YCPBoolean (true);
}

// ------------------------
/**
   @builtin IsManualSelection

   @short Check Status of Selections and if they have changed
   @description
   return true if the original list of packages (since the
   last Pkg::SetSelection()) was changed.
   @return boolean

*/
YCPValue
PkgFunctions::IsManualSelection ()
{
    try
    {
	for (zypp::ResPool::byKind_iterator it = zypp_ptr()->pool().byKindBegin(zypp::ResTraits<zypp::Package>::kind);
	    it != zypp_ptr()->pool().byKindEnd(zypp::ResTraits<zypp::Package>::kind);
	    ++it)
	{
	    // return true if there is a package installed/removed by user
	    if ((it->status().isToBeInstalled() || it->status().isToBeUninstalled()) && it->status().isByUser())
	    {
		return YCPBoolean(true);
	    }
	}
    }
    catch (...)
    {
    }

    return YCPBoolean(false);
}

// ------------------------
/**
   @builtin PkgAnyToDelete

   @short Check if there are any package to be deleted
   @description
   return true if any packages are to be deleted
   @return boolean

*/
YCPValue
PkgFunctions::PkgAnyToDelete ()
{
    bool ret = false;

    y2warning("Pkg::PkgAnyToDelete() is obsoleted, use Pkg::IsAnyResolvable(`package, `to_remove) instead");

    try
    {
	for (zypp::ResPool::byKind_iterator it = zypp_ptr()->pool().byKindBegin(zypp::ResTraits<zypp::Package>::kind);
	    it != zypp_ptr()->pool().byKindEnd(zypp::ResTraits<zypp::Package>::kind);
	    ++it)
	{
	    if (it->status().isToBeUninstalled())
	    {
		ret = true;
		break;
	    }
	}
    }
    catch (...)
    {
    }

    return YCPBoolean(ret);
}

// ------------------------
/**
   @builtin AnyToInstall

   @short Check if there are any package to be installed
   @description
   return true if any packages are to be installed
   @return boolean

*/
YCPValue
PkgFunctions::PkgAnyToInstall ()
{
    bool ret = false;

    y2warning("Pkg::PkgAnyToInstall() is obsoleted, use Pkg::IsAnyResolvable(`package, `to_install) instead");

    try
    {
	for (zypp::ResPool::byKind_iterator it = zypp_ptr()->pool().byKindBegin(zypp::ResTraits<zypp::Package>::kind);
	    it != zypp_ptr()->pool().byKindEnd(zypp::ResTraits<zypp::Package>::kind);
	    ++it)
	{
	    if (it->status().isToBeInstalled())
	    {
		ret = true;
		break;
	    }
	}
    }
    catch (...)
    {
    }

    return YCPBoolean(ret);
}

// ------------------------


/* helper function */
static void
pkg2list (YCPList &list, const zypp::ResPool::byKind_iterator& it, bool names_only)
{
    if (names_only)
    {
	list->add(YCPString((*it)->name()));
    }
    else
    {
	string fullname = (*it)->name();
	fullname += (" " + (*it)->edition().version());
	fullname += (" " + (*it)->edition().release());
	fullname += (" " + (*it)->arch().asString());
	list->add (YCPString (fullname));
    }
    return;
}


/**
   @builtin FilterPackages

   @short Get list of packages depending on how they were selected
   @description
   return list of filtered packages (["pkg1", "pkg2", ...] if names_only==true,
    ["pkg1 version release arch", "pkg1 version release arch", ... if
    names_only == false]

    @param boolean byAuto packages you get by dependencies
    @param boolean byApp packages you get by selections
    @param boolean byUser packages the user explicitly requested
    @return list<string>


*/
YCPValue
PkgFunctions::FilterPackages(const YCPBoolean& y_byAuto, const YCPBoolean& y_byApp, const YCPBoolean& y_byUser, const YCPBoolean& y_names_only)
{
    bool byAuto = y_byAuto->value();
    bool byApp  = y_byApp->value();
    bool byUser = y_byUser->value();
    bool names_only = y_names_only->value();

    YCPList packages;

    try
    {
	for (zypp::ResPool::byKind_iterator it = zypp_ptr()->pool().byKindBegin(zypp::ResTraits<zypp::Package>::kind);
	    it != zypp_ptr()->pool().byKindEnd(zypp::ResTraits<zypp::Package>::kind);
	    ++it)
	{
	    // check status and causer
	    if (it->status().isToBeInstalled() &&
		((byAuto && it->status().isBySolver()) ||
#warning FilterPackages: APPL_LOW and APPL_HIGH are treated as one level for now
		    (byApp && (it->status().isByApplHigh() || it->status().isByApplLow())) ||
		    (byUser && it->status().isByUser())
		))
		{
		    pkg2list(packages, it, names_only);
		}
	}
    }
    catch (...)
    {
    }

    return packages;
}

/**
   @builtin GetPackages

   @short Get list of packages (installed, selected, available, to be removed)
   @description
   return list of packages (["pkg1", "pkg2", ...] if names_only==true,
   ["pkg1 version release arch", "pkg1 version release arch", ... if
   names_only == false]

   @param symbol 'which' defines which packages are returned: `installed all installed packages,
   `selected returns all selected but not yet installed packages, `available returns all
   available packages (from the installation source), `removed all packages selected for removal,
   `locked all locked packages (locked, installed), `taboo all taboo packages (locked, not installed)
   @param boolean names_only If true, return package names only
   @return list<string>

*/

YCPValue
PkgFunctions::GetPackages(const YCPSymbol& y_which, const YCPBoolean& y_names_only)
{
    string which = y_which->symbol();
    bool names_only = y_names_only->value();

    YCPList packages;

    try
    {
	for (zypp::ResPool::byKind_iterator it = zypp_ptr()->pool().byKindBegin(zypp::ResTraits<zypp::Package>::kind);
	    it != zypp_ptr()->pool().byKindEnd(zypp::ResTraits<zypp::Package>::kind);
	    ++it)
	{
	    if (which == "installed")
	    {
		if (it->status().isInstalled())
		{
		    pkg2list(packages, it, names_only);
		}
	    }
	    else if (which == "selected")
	    {
		if (it->status().isToBeInstalled())
		{
		    pkg2list(packages, it, names_only);
		}
	    }
	    else if (which == "removed")
	    {
		if (it->status().isToBeUninstalled())
		{
		    pkg2list(packages, it, names_only);
		}
	    }
	    else if (which == "available")
	    {
		if (!it->status().isInstalled())
		{
		    pkg2list(packages, it, names_only);
		}
	    }
	    else if (which == "locked")
	    {
		if (it->status().isLocked() && it->status().isInstalled())
		{
		    pkg2list(packages, it, names_only);
		}
	    }
	    else if (which == "taboo")
	    {
		if (it->status().isLocked() && !it->status().isInstalled())
		{
		    pkg2list(packages, it, names_only);
		}
	    }
	    else
	    {
		return YCPError ("Wrong parameter for Pkg::GetPackages");
	    }
	}
    }
    catch (...)
    {
    }

    return packages;
}


/**
 * @builtin PkgUpdateAll
 * @param map<string,any> update_options Options for the solver. All parameters are optional, if a parameter is missing the default value from the package manager (libzypp) is used. Currently supported options: <tt>$["delete_unmaintained":boolean, "silent_downgrades":boolean, "keep_installed_patches":boolean]</tt>
 * @short Update installed packages
 * @description
 * Mark all packages for installation which are installed and have
 * an available candidate for update.
 *
 * This will mark packages for installation *and* for deletion (if a
 * package provides/obsoletes another package)
 *
 * This function does not solve dependencies.
 *
 * Symbols and integer values returned:
 *
 * <b>ProblemListSze</b>: Number of taboo and dropped packages found.
 *
 * <b>DeleteUnmaintained</b>: Whether delete_unmaintained arg was true or false.
 * Dependent on this, <b>SumDropped</b> below either denotes packages to delete
 * (if true) or packages to keep (if false).
 *
 * <b>SumProcessed</b>: TOTAL number of installed packages we processed.
 *
 * <b>SumToInstall</b>: TOTAL number of packages now tagged as to install.
 * Summs <b>Ipreselected</b>, <b>Iupdate</b>, <b>Idowngrade</b>, <b>Ireplaced</b>.
 *
 * <b>Ipreselected</b>: Packages which were already taged to install.
 *
 * <b>Iupdate</b>: Packages set to install as update to a newer version.
 *
 * <b>Idowngrade</b>: Packages set to install performing a version downgrade.
 *
 * <b>Ireplaced</b>: Packages set to install as they replace an installed package.
 *
 * <b>SumToDelete</b>: TOTAL number of packages now tagged as to delete.
 * Summs <b>Dpreselected</b>, <b>SumDropped</b> if <b>DeleteUnmaintained</b>
 * was set.
 *
 * <b>Dpreselected</b>: Packages which were already taged to delete.
 *
 * <b>SumToKeep</b>: TOTAL number of packages which remain unchanged.
 * Summs <b>Ktaboo</b>, <b>Knewer</b>, <b>Ksame</b>, <b>SumDropped</b>
 * if <b>DeleteUnmaintained</b> was not set.
 *
 * <b>Ktaboo</b>: Packages which are set taboo.
 *
 * <b>Knewer</b>: Packages kept because only older versions are available.
 *
 * <b>Ksame</b>: Packages kept because they are up to date.
 *
 * <b>SumDropped</b>: TOTAL number of dropped packages found. Dependent
 * on the delete_unmaintained arg, they are either tagged as to delete or
 * remain unchanged.
 *
 * @return map<symbol,integer> summary of the update
 */

YCPValue
PkgFunctions::PkgUpdateAll (const YCPMap& options)
{
    zypp::UpgradeStatistics stats;

    YCPValue delete_unmaintained = options->value(YCPString("delete_unmaintained"));
    if(!delete_unmaintained.isNull())
    {
	y2error("'delete_unmaintained' flag is obsoleted and should not be used, check the code!");
    }

    YCPValue silent_downgrades = options->value(YCPString("silent_downgrades"));
    if(!silent_downgrades.isNull())
    {
	if (silent_downgrades->isBoolean())
	{
	    stats.silent_downgrades = silent_downgrades->asBoolean()->value();
	}
	else
	{
	    y2error("unexpected type of 'silent_downgrades' key: %s, must be a boolean!",
		Type::vt2type(silent_downgrades->valuetype())->toString().c_str());
	}
    }

    YCPValue keep_installed_patches = options->value(YCPString("keep_installed_patches"));
    if(!keep_installed_patches.isNull())
    {
	y2error("'keep_installed_patches' flag is obsoleted and should not be used, check the code!");
    }


    YCPMap data;

    try
    {
	// solve upgrade, get statistics
	zypp_ptr()->resolver()->doUpgrade(stats);
    }
    catch (...)
    {
	return data;
    }

    data->add( YCPSymbol("ProblemListSze"), YCPInteger(stats.chk_is_taboo + stats.chk_dropped));

    // packages to install; sum and details
    data->add( YCPSymbol("SumToInstall"), YCPInteger( stats.totalToInstall() ) );
    data->add( YCPSymbol("Ipreselected"), YCPInteger( stats.chk_already_toins ) );
    data->add( YCPSymbol("Iupdate"),      YCPInteger( stats.chk_to_update ) );
    data->add( YCPSymbol("Idowngrade"),   YCPInteger( stats.chk_to_downgrade ) );
    data->add( YCPSymbol("Ireplaced"),    YCPInteger( stats.chk_replaced
						      + stats.chk_replaced_guessed
						      + stats.chk_add_split ) );
    // packages to delete; sum and details (! see dropped packages)
    data->add( YCPSymbol("SumToDelete"),  YCPInteger( stats.totalToDelete() ) );
    data->add( YCPSymbol("Dpreselected"), YCPInteger( stats.chk_already_todel ) );

    // packages to delete; sum and details (! see dropped packages)
    data->add( YCPSymbol("SumToKeep"),    YCPInteger( stats.totalToKeep() ) );
    data->add( YCPSymbol("Ktaboo"),       YCPInteger( stats.chk_is_taboo ) );
    data->add( YCPSymbol("Knewer"),       YCPInteger( stats.chk_to_keep_downgrade ) );
    data->add( YCPSymbol("Ksame"),        YCPInteger( stats.chk_to_keep_installed  ) );

    // dropped packages; dependent on the delete_unmaintained
    // option set for doUpdate, dropped packages count as ToDelete
    // or ToKeep.
    data->add( YCPSymbol("SumDropped"),   YCPInteger( stats.chk_dropped ) );

    // Total mumber of installed packages processed
    data->add( YCPSymbol("SumProcessed"), YCPInteger( stats.chk_installed_total ) );

    return data;
}


/**
   @builtin PkgInstall
   @short Select package for installation
   @param string package
   @return boolean

*/
YCPValue
PkgFunctions::PkgInstall (const YCPString& p)
{
    std::string name = p->value();
    if (name.empty())
	return YCPBoolean (false);

    // ensure installation of the 'best' architecture

    return YCPBoolean( DoProvideNameKind( name, zypp::ResTraits<zypp::Package>::kind, zypp::ZConfig::instance().systemArchitecture(), "") );
}

/**
   @builtin PkgSrcInstall

   @short Select source of package for installation
   @param string package
   @return boolean

*/
YCPValue
PkgFunctions::PkgSrcInstall (const YCPString& p)
{
    std::string name = p->value();
    if (name.empty())
	return YCPBoolean (false);

    // ensure installation of the 'best' architecture

    return YCPBoolean( DoProvideNameKind( name, zypp::ResTraits<zypp::SrcPackage>::kind, zypp::ZConfig::instance().systemArchitecture(), "" ) );
}


/**
   @builtin PkgDelete

   @short Select package for deletion (deletes all installed instances of the package)
   @param string package
   @return boolean

*/

YCPValue
PkgFunctions::PkgDelete (const YCPString& p)
{
    std::string name = p->value();
    if (name.empty())
	return YCPBoolean (false);

    bool ret = true;

    try
    {
	bool found = false;

	for (zypp::ResPool::byIdent_iterator it = zypp_ptr()->pool().byIdentBegin<zypp::Package>(name);
	    it != zypp_ptr()->pool().byIdentEnd<zypp::Package>(name); ++it)
	{
	    // is it an installed package?
	    if (it->status().isInstalled())
	    {
		found = true;
		ret = it->status().setToBeUninstalled(whoWantsIt) && ret;
	    }
	}

	if (!found)
	{
	    ret = false;
	}
    }
    catch (...)
    {
	ret = false;
    }

    return YCPBoolean(ret);
}

/**
   @builtin PkgTaboo

   @short Set package to taboo (sets all instances of the package - all versions, from all repositories)
   @param string package
   @return boolean

*/

YCPValue
PkgFunctions::PkgTaboo (const YCPString& p)
{
    std::string name = p->value();
    if (name.empty())
	return YCPBoolean (false);

    bool ret = true;

    try
    {
	bool found = false;

	for (zypp::ResPool::byIdent_iterator it = zypp_ptr()->pool().byIdentBegin<zypp::Package>(name);
	    it != zypp_ptr()->pool().byIdentEnd<zypp::Package>(name); ++it)
	{
	    // installed package cannot be set to taboo
	    if (!it->status().isInstalled())
	    {
		found = true;

		bool res = it->status().resetTransact(whoWantsIt)
		// lock the package at the USER level (bug #186205)
		&& it->status().resetTransact(zypp::ResStatus::USER)
		&& it->status().setLock(true, zypp::ResStatus::USER);

		ret = ret && res;
	    }
	}

	if (!found)
	{
	    ret = false;
	}
    }
    catch (...)
    {
	ret = false;
    }

    return YCPBoolean(ret);
}

/**
   @builtin PkgNeutral

   @short Set package to neutral (drop install/delete flags) (sets all instances of the package - all versions, from all repositories)
   @param string package
   @return boolean

*/

YCPValue
PkgFunctions::PkgNeutral (const YCPString& p)
{
    std::string name = p->value();
    if (name.empty())
	return YCPBoolean (false);

    bool ret = true;

    try
    {
	bool found = false;

	for (zypp::ResPool::byIdent_iterator it = zypp_ptr()->pool().byIdentBegin<zypp::Package>(name);
	    it != zypp_ptr()->pool().byIdentEnd<zypp::Package>(name); ++it)
	{
	    found = true;
	    ret = it->status().resetTransact(whoWantsIt) && ret;
	}

	if (!found)
	{
	    ret = false;
	}
    }
    catch (...)
    {
	ret = false;
    }

    return YCPBoolean(ret);
}


/**
 * @builtin Reset
 *
 * @short Reset most internal stuff on the package manager.
   @return boolean
 */

YCPValue
PkgFunctions::PkgReset ()
{
    try
    {
	for (zypp::ResPool::const_iterator it = zypp_ptr()->pool().begin()
	    ; it != zypp_ptr()->pool().end()
	    ; ++it)
	{
	    // reset all transaction flags
	    it->statusReset();
	}

	return YCPBoolean (true);
    }
    catch (...)
    {
    }

    return YCPBoolean (false);
}


/**
 * @builtin Reset
 *
 * @short Reset most internal stuff on the package manager.
   Reset only packages set by the application, not by the user
   @return boolean
 */

YCPValue
PkgFunctions::PkgApplReset ()
{
    try
    {
	for (zypp::ResPool::const_iterator it = zypp_ptr()->pool().begin()
	    ; it != zypp_ptr()->pool().end()
	    ; ++it)
	{
	    // reset all transaction flags
	    it->status().resetTransact(zypp::ResStatus::APPL_HIGH);
	}

	return YCPBoolean (true);
    }
    catch (...)
    {
    }

    return YCPBoolean (false);
}


/**
   @builtin PkgSolve
   @short Solve current package dependencies
   @optarg boolean filter  unused, only for backward compatibility
   (installed packages will be preferred)
   @return boolean

*/
YCPBoolean
PkgFunctions::PkgSolve (const YCPBoolean& filter)
{
    bool result = false;

    try
    {
	result = zypp_ptr()->resolver()->resolvePool();
    }
    catch (const zypp::Exception& excpt)
    {
	y2error("An error occurred during Pkg::PkgSolve.");
	_last_error.setLastError(excpt.asUserString(), "See /var/log/YaST2/badlist for more information.");
    }

    // save information about failed dependencies to file
    if (!result)
    {
# warning PkgSolve: filter option is not used
/*	bool filter_conflicts_with_installed = false;

	if (! filter.isNull())
	{
	    filter_conflicts_with_installed = filter->value();
	}
*/

	try
	{
	    zypp::ResolverProblemList problems = zypp_ptr()->resolver()->problems();
	    int problem_size = problems.size();

	    if (problem_size > 0)
	    {
		y2error ("PkgSolve: %d packages failed (see /var/log/YaST2/badlist)", problem_size);

		std::ofstream out ("/var/log/YaST2/badlist");

		out << problem_size << " packages failed" << std::endl;
		for(zypp::ResolverProblemList::const_iterator p = problems.begin();
		     p != problems.end(); ++p )
		{
		    out << (*p)->description() << std::endl;
		}
	    }
	}
	catch (...)
	{
	    return YCPBoolean(false);
	}
    }

    return YCPBoolean(result);
}


/**
   @builtin PkgEstablish
   @short establish the pool state
   @return boolean

   Returns true. (If no pool item 'transacts')

   The pool should NOT have any items set to 'transact' (scheduled for installation
   or removal)
   If it has, dependencies will be solved and the returned result might be false.

*/
YCPBoolean
PkgFunctions::PkgEstablish ()
{
    return YCPBoolean(false);
}


/**
   @builtin PkgFreshen
   @short check all package freshens and schedule matching ones for installation
   @return boolean

   Returns true. (If no pool item 'transacts')

   The pool should NOT have any items set to 'transact' (scheduled for installation
   or removal)
   If it has, dependencies will be solved and the returned result might be false.

*/

YCPBoolean
PkgFunctions::PkgFreshen()
{
    return YCPBoolean(true);
}


/**
   @builtin PkgSolveCheckTargetOnly

   @short Solve packages currently installed on target system.
   @description
   Solve packages currently installed on target system. Packages status
   remains unchanged, i.e. does not select/deselect any packages to
   resolve failed dependencies.

   @return boolean
*/
YCPBoolean
PkgFunctions::PkgSolveCheckTargetOnly()
{
    try
    {
	zypp_ptr()->target()->load();
    }
    catch (...)
    {
	return YCPBoolean(false);
    }

    bool result = false;

    try
    {
	// verify consistency of system
	result = zypp_ptr()->resolver()->verifySystem();
    }
    catch (const zypp::Exception& excpt)
    {
	y2error("An error occurred during Pkg::PkgSolveCheckTargetOnly");
	_last_error.setLastError(ExceptionAsString(excpt));
    }

    return YCPBoolean(result);
}


/**
   @builtin PkgSolveErrors
   @short Returns number of fails
   @description
   only valid after a call of PkgSolve/PkgSolveCheckTargetOnly that returned false
   return number of fails

   @return integer
*/
YCPValue
PkgFunctions::PkgSolveErrors()
{
    try
    {
	return YCPInteger(zypp_ptr()->resolver()->problems().size());
    }
    catch (...)
    {
    }

    return YCPVoid();
}

/**
   @builtin PkgCommit

   @short Commit package changes (actually install/delete packages)
   @description

   if medianr == 0, all packages regardless of media are installed

   if medianr > 0, only packages from this media are installed

   @param integer medianr Media Number
   @return list [ int successful, list failed, list remaining, list srcremaining ]
   The 'successful' value will be negative, if installation was aborted !

*/
YCPValue
PkgFunctions::PkgCommit (const YCPInteger& media)
{
    int medianr = media->value ();

    if (medianr < 0)
    {
	return YCPError ("Bad args to Pkg::PkgCommit");
    }

    zypp::ZYpp::CommitResult result;

    // clean the last reported source
    // DownloadResolvableReceive::last_source_id = -1;
    // DownloadResolvableReceive::last_source_media = -1;

    try
    {
	// if the target log is not set use the default to not loose the information
	if (!target_log_set)
	{
	    std::string default_path(_target_root.asString() + "/var/log/YaST2/y2logRPM");
	    y2warning("Pkg::TargetLogFile() has not been called, using %s for logging", default_path.c_str());
	    zypp_ptr()->target()->setInstallationLogfile(default_path);
	}

	zypp::ZYppCommitPolicy policy;
	policy.restrictToMedia( medianr );
	result = zypp_ptr()->commit(policy);
    }
    catch (const zypp::target::TargetAbortedException & excpt)
    {
	y2milestone ("Installation aborted by user");
	YCPList ret;
	ret->add(YCPInteger(-1));
	return ret;
    }
    catch (const zypp::Exception& excpt)
    {
	y2error("Pkg::Commit has failed: ZYpp::commit has failed");
	_last_error.setLastError(ExceptionAsString(excpt));
	return YCPVoid();
    }

    SourceReleaseAll();

    YCPList ret;

    ret->add(YCPInteger(result._result));

    YCPList errlist;
    for (zypp::PoolItemList::const_iterator it = result._errors.begin(); it != result._errors.end(); ++it)
    {
	errlist->add(YCPString(it->resolvable()->name()));
    }
    ret->add(errlist);

    YCPList remlist;
    for (zypp::PoolItemList::const_iterator it = result._remaining.begin(); it != result._remaining.end(); ++it)
    {
	YCPMap resolvable;
	resolvable->add (YCPString ("name"),
	    YCPString(it->resolvable()->name()));
	if (zypp::isKind<zypp::Product>(it->resolvable()))
	    resolvable->add (YCPString ("kind"), YCPSymbol ("product"));
	else if (zypp::isKind<zypp::Pattern>(it->resolvable()))
	    resolvable->add (YCPString ("kind"), YCPSymbol ("pattern"));
	else if (zypp::isKind<zypp::Patch>(it->resolvable()))
	    resolvable->add (YCPString ("kind"), YCPSymbol ("patch"));
	else
	    resolvable->add (YCPString ("kind"), YCPSymbol ("package"));
	resolvable->add (YCPString ("arch"),
	    YCPString (it->resolvable()->arch().asString()));
	resolvable->add (YCPString ("version"),
	    YCPString (it->resolvable()->edition().asString()));
	remlist->add(resolvable);
    }
    ret->add(remlist);

    YCPList srclist;
    for (zypp::PoolItemList::const_iterator it = result._srcremaining.begin(); it != result._srcremaining.end(); ++it)
    {
	srclist->add(YCPString(it->resolvable()->name()));
    }
    ret->add(srclist);

    return ret;
}

/**
   @builtin GetBackupPath

   @short get current path for update backup of rpm config files
   @return string Path to backup
*/

YCPValue
PkgFunctions::GetBackupPath ()
{
    try
    {
	return YCPString(zypp_ptr()->target()->rpmDb().getBackupPath().asString());
    }
    catch (...)
    {
    }

    return YCPVoid();
}

/**
   @builtin SetBackupPath
   @short set current path for update backup of rpm config files
   @param string path
   @return void

*/
YCPValue
PkgFunctions::SetBackupPath (const YCPString& p)
{
    try
    {
	zypp_ptr()->target()->rpmDb().setBackupPath(p->value());
    }
    catch (...)
    {
    }

    return YCPVoid();
}


/**
   @builtin CreateBackups
   @short whether to create package backups during install or removal
   @param boolean flag
   @return void
*/
YCPValue
PkgFunctions::CreateBackups (const YCPBoolean& flag)
{
    try
    {
	zypp_ptr()->target()->rpmDb().createPackageBackups(flag->value());
    }
    catch (...)
    {
    }

    return YCPVoid ();
}


/**
   @builtin PkgGetLicenseToConfirm

   @short Return Licence Text
   @description
   Return the candidate packages license text. Returns an empty string if
   package is unknown or has no license.
   @param string package
   @return string
*/
YCPString PkgFunctions::PkgGetLicenseToConfirm( const YCPString & package )
{
    std::string pkgname = package->value();

    if (!pkgname.empty())
    {
	try
	{
	    for (zypp::ResPool::byIdent_iterator it = zypp_ptr()->pool().byIdentBegin<zypp::Package>(pkgname);
		it != zypp_ptr()->pool().byIdentEnd<zypp::Package>(pkgname);
		++it)
	    {
		// is it a package?
		zypp::Package::constPtr package = zypp::dynamic_pointer_cast<const zypp::Package>(it->resolvable());

		// a package scheduled for installation, with unconfirmed license, not installed yet
		if (package && it->status().isToBeInstalled() && !it->status().isLicenceConfirmed()
		    && !it->status().isInstalled())
		{
		    // get the license
		    string license = package->licenseToConfirm();
		    return YCPString(license);
		}
	    }
	}
	catch (...)
	{
	}
    }

    return YCPString("");
}

/**
   @builtin PkgGetLicensesToConfirm

   @short Return Licence Text of several packages
   @description
   Return a map<package,license> for all candidate packages in list, which do have a
   license. Unknown packages and those without license text are not returned.
   @param list<string> packages
   @return map<string,string>

*/
YCPMap PkgFunctions::PkgGetLicensesToConfirm( const YCPList & packages )
{
    YCPMap ret;

    for ( int i = 0; i < packages->size(); ++i ) {
	YCPString license = PkgGetLicenseToConfirm(packages->value(i)->asString());

	// found a license to confirm?
	if (!license->value().empty())
	{
	    ret->add(packages->value(i), license);
	}
    }

    return ret;
}

/**
   @builtin PkgMarkLicenseConfirmed

   @short Mark licence of the package confirmed
   @param string name of a package
   @return boolean true if the license has been successfuly confirmed
*/
YCPBoolean PkgFunctions::PkgMarkLicenseConfirmed (const YCPString & package)
{
    std::string pkgname = package->value();

    if (!pkgname.empty())
    {
	try
	{
	    for (zypp::ResPool::byIdent_iterator it = zypp_ptr()->pool().byIdentBegin<zypp::Package>(pkgname);
		it != zypp_ptr()->pool().byIdentEnd<zypp::Package>(pkgname);
		++it)
	    {
		// is it a package?
		zypp::Package::constPtr package = zypp::asKind<zypp::Package>( it->resolvable() );

		// a package scheduled for installation, with unconfirmed license, not installed yet
		if (package && it->status().isToBeInstalled() && !it->status().isLicenceConfirmed()
		    && !it->status().isInstalled())
		{
		    // confirm the license
		    it->status().setLicenceConfirmed(true);
		    return YCPBoolean( true );
		}
	    }
	}
	catch (...)
	{
	}
    }

    return YCPBoolean( false );
}

/****************************************************************************************
 * @builtin RpmChecksig
 * @short Check signature of RPM
 * @param string filename
 * @return boolean True if filename is a rpm package with valid signature.
 **/
YCPBoolean PkgFunctions::RpmChecksig( const YCPString & filename )
{
    try
    {
	return YCPBoolean(zypp_ptr()->target()->rpmDb().checkPackage(filename->value()) == 0);
    }
    catch (...)
    {
    }

    return YCPBoolean(false);
}


YCPValue
PkgFunctions::PkgDU(const YCPString& package)
{
    // get partitioning
    zypp::DiskUsageCounter::MountPointSet mps = zypp_ptr()->getPartitions();

    zypp::PoolItem item;
    zypp::Package::constPtr pkg = find_package(package->value(), item, zypp_ptr()->pool() );

    // the package was not found
    if (pkg == NULL)
    {
	return YCPVoid();
    }

    zypp::DiskUsage du = pkg->diskusage();

    if (du.size() == 0)
    {
	y2warning("Disk usage for package %s is unknown", package->value().c_str());
	return YCPVoid();
    }

    // iterate trough all mount points, add usage to each directory
    // directory tree must be processed from leaves to the root directory
    // so iterate in reverse order so e.g. /usr is used before /
    for (zypp::DiskUsageCounter::MountPointSet::reverse_iterator mpit = mps.rbegin(); mpit != mps.rend(); mpit++)
    {
	// get usage for the mount point
	zypp::DiskUsage::Entry entry = du.extract(mpit->dir);

	mpit->pkg_size += entry._size;
    }

    return MPS2YCPMap(mps);
}
