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

   File:	PkgModuleFunctionsPackage.cc

   Author:	Klaus Kaempf <kkaempf@suse.de>
   Maintainer:  Klaus Kaempf <kkaempf@suse.de>
   Namespace:   Pkg

   Summary:	Access to packagemanager
   Purpose:     Handles package related Pkg::function (list_of_arguments) calls
		from WFMInterpreter.
/-*/

#include <ycp/y2log.h>
#include "PkgModuleFunctions.h"

#include <ycp/YCPVoid.h>
#include <ycp/YCPBoolean.h>
#include <ycp/YCPInteger.h>
#include <ycp/YCPSymbol.h>
#include <ycp/YCPString.h>
#include <ycp/YCPList.h>
#include <ycp/YCPMap.h>

#include <zypp/base/Algorithm.h>
#include <zypp/ResFilters.h>
#include <zypp/CapFilters.h>
#include <zypp/ResStatus.h>

#include <zypp/ResPool.h>
#include <zypp/Package.h>
#include <zypp/SrcPackage.h>
#include <zypp/Product.h>
#include <zypp/SourceManager.h>
#include <zypp/UpgradeStatistics.h>
#include <zypp/target/rpm/RpmDb.h>
#include <zypp/target/TargetException.h>
#include <zypp/ZYppCommit.h>
#include <zypp/ResPoolManager.h>

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
PkgModuleFunctions::PkgQueryProvides( const YCPString& tag )
{
    YCPList ret;
    std::string name = tag->value();

    // 'it' is 'const struct std::pair<const std::string, std::pair<zypp::Capability, zypp::PoolItem> >'
    for (zypp::ResPool::byCapabilityIndex_iterator it = zypp_ptr()->pool().byCapabilityIndexBegin(name, zypp::Dep::PROVIDES);
	it != zypp_ptr()->pool().byCapabilityIndexEnd(name, zypp::Dep::PROVIDES);
	++it)
    {
	// is it a package?
	if (zypp::isKind<zypp::Package>(it->item.resolvable()))
	{

	    // cast to Package object
	    zypp::Package::constPtr package = zypp::dynamic_pointer_cast<const zypp::Package>(it->item.resolvable());
	    std::string pkgname = package->name();

	    // get instance status
	    bool installed = it->item.status().staysInstalled();
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
	    bool uninstalled = it->item.status().staysUninstalled() || it->item.status().isToBeUninstalled();
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
PkgModuleFunctions::PkgMediaNames ()
{
# warning No installation order

    std::list<zypp::SourceManager::SourceId> source_ids = zypp::SourceManager::sourceManager()->enabledSources();

    YCPList res;

    // initialize
    for( std::list<zypp::SourceManager::SourceId>::const_iterator sit = source_ids.begin();
	sit != source_ids.end(); ++sit)
    {
	unsigned id = *sit;

	zypp::Source_Ref src = zypp::SourceManager::sourceManager()->findSource( id );

	try
	{
	    // find a product for the given source
	    zypp::ResPool::byKind_iterator it = zypp_ptr()->pool().byKindBegin(zypp::ResTraits<zypp::Product>::kind);

	    for( ; it != zypp_ptr()->pool().byKindEnd(zypp::ResTraits<zypp::Product>::kind)
		; ++it) {
		zypp::Product::constPtr product = boost::dynamic_pointer_cast<const zypp::Product>( it->resolvable() );

		y2debug ("Checking product: %s", product->summary().c_str());
		if( product->source() == src )
		{
		    y2debug ("Found");

		    YCPList src_desc;
		    src_desc->add( YCPString( product->summary() ) );
		    src_desc->add( YCPInteger( src.numericId() ) );

		    res->add( src_desc );
		    break;
		}
	    }

	    if( it == zypp_ptr()->pool().byKindEnd(zypp::ResTraits<zypp::Product>::kind) )
	    {
		y2error ("Product for source '%d' not found", id);

		YCPList src_desc;
		src_desc->add( YCPString( src.url().asString() ) );
		src_desc->add( YCPInteger( src.numericId() ) );

		res->add( src_desc );
	    }
	}
	catch (...)
	{
	    return res;
	}
    }

    y2milestone( "Pkg::PkgMediaNames result: %s", res->toString().c_str());

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
PkgModuleFunctions::PkgMediaSizes ()
{
# warning No installation order

    // all enabled sources
    std::list<zypp::SourceManager::SourceId> source_ids = zypp::SourceManager::sourceManager()->enabledSources();

    // map SourceId -> [ number_of_media, total_size ]
    std::map<zypp::SourceManager::SourceId, std::vector<zypp::ByteCount> > result;

    // map zypp::Source -> SourceID
    std::map<zypp::Source_Ref, zypp::SourceManager::SourceId> source_map;

    // initialize
    for( std::list<zypp::SourceManager::SourceId>::const_iterator sit = source_ids.begin();
	sit != source_ids.end(); ++sit)
    {
	zypp::SourceManager::SourceId id = *sit;

	zypp::Source_Ref src = zypp::SourceManager::sourceManager()->findSource( id );
	unsigned media = src.numberOfMedia();

	result[id] = std::vector<zypp::ByteCount>(media,0);

	source_map[ src ] = id;
    }

    for( zypp::ResPool::byKind_iterator it = zypp_ptr()->pool().byKindBegin<zypp::Package>()
	; it != zypp_ptr()->pool().byKindEnd<zypp::Package>()
	; ++it )
    {
	zypp::Package::constPtr pkg = boost::dynamic_pointer_cast<const zypp::Package>(it->resolvable());

	if( it->status().isToBeInstalled() && pkg->sourceMediaNr() > 0)
	{
	    zypp::ByteCount size = pkg->size();
	    result[ source_map[pkg->source()] ]
	      [pkg->sourceMediaNr()-1] += size ;	// media are numbered from 1
	}
    }

    YCPList res;

    for(std::map<zypp::SourceManager::SourceId, std::vector<zypp::ByteCount> >::const_iterator it =
	result.begin(); it != result.end() ; ++it)
    {
	std::vector<zypp::ByteCount> values = it->second;
	YCPList source;

	for( unsigned i = 0 ; i < values.size() ; i++ )
	{
	    source->add( YCPInteger( values[i] ) );
	}

	res->add( source );
    }

    y2milestone( "Pkg::PkgMediaSize result: %s", res->toString().c_str());

    return res;
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
PkgModuleFunctions::PkgMediaCount()
{
# warning No installation order

    // all enabled sources
    std::list<zypp::SourceManager::SourceId> source_ids = zypp::SourceManager::sourceManager()->enabledSources();

    // map SourceId -> [ number_of_media, total_size ]
    std::map<zypp::SourceManager::SourceId, std::vector<zypp::ByteCount> > result;

    // map zypp::Source -> SourceID
    std::map<zypp::Source_Ref, zypp::SourceManager::SourceId> source_map;


    // initialize
    for( std::list<zypp::SourceManager::SourceId>::const_iterator sit = source_ids.begin();
	sit != source_ids.end(); ++sit)
    {
	zypp::SourceManager::SourceId id = *sit;

	zypp::Source_Ref src = zypp::SourceManager::sourceManager()->findSource( id );
	unsigned media = src.numberOfMedia();

	result[id] = std::vector<zypp::ByteCount>(media,0);

	source_map[ src ] = id;
    }

    for( zypp::ResPool::byKind_iterator it = zypp_ptr()->pool().byKindBegin<zypp::Package>()
	; it != zypp_ptr()->pool().byKindEnd<zypp::Package>()
	; ++it )
    {
	zypp::Package::constPtr pkg = boost::dynamic_pointer_cast<const zypp::Package>(it->resolvable());

	if( pkg && it->status().isToBeInstalled() && pkg->sourceMediaNr() > 0)
	    result[ source_map[pkg->source()] ]
	      [pkg->sourceMediaNr()-1]++ ;	// media are numbered from 1
    }

    YCPList res;

    for(std::map<zypp::SourceManager::SourceId, std::vector<zypp::ByteCount> >::const_iterator it =
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

    y2milestone( "Pkg::PkgMediaCount result: %s", res->toString().c_str());

    return res;
}

// ------------------------

struct CaIMatch
{
    zypp::PoolItem_Ref item;

    bool operator() (const zypp::CapAndItem & cai )
    {
	item = cai.item;
	return false;
    }
};

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
PkgModuleFunctions::IsProvided (const YCPString& tag)
{
    std::string name = tag->value ();
    if (name.empty())
	return YCPBoolean (false);

    y2milestone("IsProvided called");

    zypp::Dep dep( zypp::Dep::PROVIDES );
    CaIMatch match;

    // look for installed packages

    try
    {
	invokeOnEach( zypp_ptr()->pool().byCapabilityIndexBegin( name, dep ),
		      zypp_ptr()->pool().byCapabilityIndexEnd( name, dep ),
		      zypp::functor::chain( zypp::resfilter::ByCaIInstalled (),
				      zypp::resfilter::ByCaIKind( zypp::ResTraits<zypp::Package>::kind ) ),
		      zypp::functor::functorRef<bool,zypp::CapAndItem>( match ) );
    }
    catch (...)
    {
    }

    return YCPBoolean( match.item );
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
PkgModuleFunctions::IsSelected (const YCPString& tag)
{
    std::string name = tag->value ();
    if (name.empty())
	return YCPBoolean (false);

    zypp::Dep dep( zypp::Dep::PROVIDES );
    CaIMatch match;

    try
    {
	// look for to-be-installed packages
	invokeOnEach( zypp_ptr()->pool().byCapabilityIndexBegin( name, dep ),
		      zypp_ptr()->pool().byCapabilityIndexEnd( name, dep ),
		      zypp::functor::chain( zypp::resfilter::ByCaIUninstalled(),
			  zypp::functor::chain( zypp::resfilter::ByCaITransact(),
					  zypp::resfilter::ByCaIKind( zypp::ResTraits<zypp::Package>::kind ) ) ),
		      zypp::functor::functorRef<bool,zypp::CapAndItem>( match ) );
    }
    catch (...)
    {
    }

    return YCPBoolean( match.item );
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
PkgModuleFunctions::IsAvailable (const YCPString& tag)
{
    y2milestone("IsAvailable called");
    std::string name = tag->value ();
    if (name.empty())
	return YCPBoolean (false);

    zypp::Dep dep( zypp::Dep::PROVIDES );
    CaIMatch match;

    try
    {
	// look for uninstalled packages
	invokeOnEach( zypp_ptr()->pool().byCapabilityIndexBegin( name, dep ),
		      zypp_ptr()->pool().byCapabilityIndexEnd( name, dep ),
		      zypp::functor::chain( zypp::resfilter::ByCaIUninstalled(),
				      zypp::resfilter::ByCaIKind( zypp::ResTraits<zypp::Package>::kind ) ),
		      zypp::functor::functorRef<bool,zypp::CapAndItem>( match ) );
    }
    catch (...)
    {
    }

    return YCPBoolean( match.item );
}


struct ProvideProcess
{
    zypp::PoolItem_Ref item;
    zypp::Arch _architecture;
    zypp::ResStatus::TransactByValue whoWantsIt;

    ProvideProcess( zypp::Arch arch )
	: _architecture( arch ), whoWantsIt(zypp::ResStatus::APPL_HIGH)
    { }

    bool operator()( zypp::PoolItem provider )
    {
        // 1. compatible arch
        // 2. best arch
        // 3. best edition
        //  see QueueItemRequire in zypp/solver/detail, RequireProcess

	if (!provider.status().isInstalled())
	{
	    // deselect the item if it's already selected,
	    // only one item should be selected
	    if (provider.status().isToBeInstalled())
	    {
		provider.status().resetTransact(whoWantsIt);
	    }

	    // regarding items which are installable only
	    if (!provider->arch().compatibleWith( _architecture )) {
		y2milestone ("provider %s has incompatible arch '%s'", provider->name().c_str(), provider->arch().asString().c_str());
	    }
	    else if (!item) {						// no provider yet
		item = provider;
	    }
	    else if (item->arch().compare( provider->arch() ) < 0) {	// provider has better arch
		item = provider;
	    }
	    else if (item->edition().compare( provider->edition() ) < 0) {
		item = provider;						// provider has better edition
	    }
	}

	return true;
    }

};


/**
 * helper function, install a resolvable with a specific name and kind
*/

bool
PkgModuleFunctions::DoProvideNameKind( const std::string & name, zypp::Resolvable::Kind kind)
{
    try
    {
	ProvideProcess info( zypp_ptr()->architecture() );
	info.whoWantsIt = whoWantsIt;

	invokeOnEach( zypp_ptr()->pool().byNameBegin( name ),
		      zypp_ptr()->pool().byNameEnd( name ),
		      zypp::resfilter::ByKind( kind ),
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
PkgModuleFunctions::DoAllKind(zypp::Resolvable::Kind kind, bool provide)
{
    bool ret = true;

    try
    {
	for (zypp::ResPool::byKind_iterator it = zypp_ptr()->pool().byKindBegin(kind);
	    it != zypp_ptr()->pool().byKindEnd(kind); ++it)
	{
	    bool res = provide ? it->status().setToBeInstalled( whoWantsIt )
		: it->status().setToBeUninstalled( whoWantsIt );

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
PkgModuleFunctions::DoProvideAllKind(zypp::Resolvable::Kind kind)
{
    return DoAllKind(kind, true);
}

bool
PkgModuleFunctions::DoRemoveAllKind(zypp::Resolvable::Kind kind)
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
PkgModuleFunctions::DoRemoveNameKind( const std::string & name, zypp::Resolvable::Kind kind)
{
    zypp::Dep dep( zypp::Dep::PROVIDES );
    CaIMatch match;

    try
    {
	// look for installed packages
	invokeOnEach( zypp_ptr()->pool().byCapabilityIndexBegin( name, dep ),
		      zypp_ptr()->pool().byCapabilityIndexEnd( name, dep ),
		      zypp::functor::chain( zypp::resfilter::ByCaIInstalled(),
				      zypp::resfilter::ByCaIKind( kind ) ),
		      zypp::functor::functorRef<bool,zypp::CapAndItem>( match ) );
    }
    catch (...)
    {
	return false;
    }

    if 	(!match.item)
	return false;

    bool result = match.item.status().setToBeUninstalled( whoWantsIt );
    y2milestone ("DoRemoveNameKind %s -> %s\n", name.c_str(), (result ? "Ok" : "Bad"));

    return true;
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
PkgModuleFunctions::DoProvide (const YCPList& tags)
{
    YCPMap ret;
    if (tags->size() > 0)
    {
        for (int i = 0; i < tags->size(); ++i)
        {
            if (tags->value(i)->isString())
            {
                DoProvideNameKind (tags->value(i)->asString()->value(), zypp::ResTraits<zypp::Package>::kind);
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
PkgModuleFunctions::DoRemove (const YCPList& tags)
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
    zypp::PoolItem_Ref item;

    bool operator() (const zypp::PoolItem_Ref i )
    {
	item = i;
	return false;
    }
};

static zypp::Package::constPtr
find_package( const string & name, zypp::PoolItem_Ref & item, zypp::ResPool pool )
{
    if (name.empty())
	return NULL;

    ItemMatch match;

    // look for uninstalled packages first, because they probably have
    // translated summary

    invokeOnEach( pool.byNameBegin( name ),
		  pool.byNameEnd( name ),
		  zypp::functor::chain( zypp::resfilter::ByUninstalled(),
				  zypp::resfilter::ByKind( zypp::ResTraits<zypp::Package>::kind ) ),
		  zypp::functor::functorRef<bool,zypp::PoolItem_Ref>( match ) );

    // if no match yet, try again with installed package
    //  (but those only provide an english summary)

    if (!match.item) {
	invokeOnEach( pool.byNameBegin( name ),
		      pool.byNameEnd( name ),
		      zypp::functor::chain( zypp::resfilter::ByInstalled(),
				      zypp::resfilter::ByKind( zypp::ResTraits<zypp::Package>::kind ) ),
		      zypp::functor::functorRef<bool,zypp::PoolItem_Ref>( match ) );
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
PkgModuleFunctions::PkgSummary (const YCPString& p)
{
    try
    {
	zypp::PoolItem_Ref item;
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
PkgModuleFunctions::PkgVersion (const YCPString& p)
{
    try
    {
	zypp::PoolItem_Ref item;
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
PkgModuleFunctions::PkgSize (const YCPString& p)
{
    try
    {
	zypp::PoolItem_Ref item;
	zypp::Package::constPtr pkg = find_package( p->value(), item, zypp_ptr()->pool() );

	if (pkg == NULL)
	{
	    return YCPVoid();
	}

	return YCPInteger( pkg->size() );
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
PkgModuleFunctions::PkgGroup (const YCPString& p)
{
    try
    {
	zypp::PoolItem_Ref item;
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
PkgModuleFunctions::PkgProp( zypp::PoolItem_Ref item )
{
    YCPMap data;
    zypp::Package::constPtr pkg = zypp::asKind<zypp::Package>( item.resolvable() );
    if (pkg == NULL) {
	y2error( "Not a package: %s", item->name().c_str() );
	return data;
    }

    data->add( YCPString("arch"), YCPString( pkg->arch().asString() ) );
    data->add( YCPString("medianr"), YCPInteger( pkg->sourceMediaNr() ) );

    y2internal("getting the ID...");
    y2internal("srcId: %ld", pkg->source().numericId() );
    data->add( YCPString("srcid"), YCPInteger( pkg->source().numericId() ) );

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

    data->add( YCPString("location"), YCPString( pkg->location().basename() ) );
    data->add( YCPString("path"), YCPString( pkg->location().asString() ) );

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
PkgModuleFunctions::PkgProperties (const YCPString& p)
{
    try
    {
	YCPMap data;
	zypp::PoolItem_Ref item;
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
PkgModuleFunctions::PkgPropertiesAll (const YCPString& p)
{
    std::string pkgname = p->value();
    YCPList data;

    if (!pkgname.empty())
    {
	try
	{
	    for (zypp::ResPool::byName_iterator it = zypp_ptr()->pool().byNameBegin(pkgname);
		it != zypp_ptr()->pool().byNameEnd(pkgname); ++it)
	    {
		// is it a package?
		if (zypp::isKind<zypp::Package>(it->resolvable()))
		{
		    data->add( PkgProp( *it ) );
		}
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
PkgModuleFunctions::GetPkgLocation (const YCPString& p, bool full_path)
{
    zypp::PoolItem_Ref item;

    try
    {
	zypp::Package::constPtr pkg = find_package( p->value(), item, zypp_ptr()->pool() );

	YCPMap data;

	if (item) {
	    return (full_path) ? YCPString( pkg->location().asString() )
			       : YCPString( pkg->location().basename() );
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
PkgModuleFunctions::PkgLocation (const YCPString& p)
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
PkgModuleFunctions::PkgPath (const YCPString& p)
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
YCPList PkgModuleFunctions::PkgGetFilelist( const YCPString & package, const YCPSymbol & which )
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
	    for (zypp::ResPool::byName_iterator it = zypp_ptr()->pool().byNameBegin(pkgname);
		it != zypp_ptr()->pool().byNameEnd(pkgname); ++it)
	    {
		// is it a package?
		if (zypp::isKind<zypp::Package>(it->resolvable()))
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
	}
	catch (...)
	{
	}
    }

    return ret;
}

// ------------------------
/**
   @builtin SaveState
   @short Save the current selection state
   @description
   save the current package selection status for later
   retrieval via Pkg::RestoreState()

   @return boolean
   @see Pkg::RestoreState

*/
YCPValue
PkgModuleFunctions::SaveState ()
{
# warning SaveState is not implemented
    return YCPBoolean (true);
}

// ------------------------
/**
   @builtin RestoreState

   @short Restore Saved state
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
PkgModuleFunctions::RestoreState (const YCPBoolean& ch)
{
# warning RestoreState is not implemented
    if (!ch.isNull () && ch->value () == true)
    {
	// return YCPBoolean (_y2pm.packageSelectionDiffState());
    }
    return YCPBoolean(false /*_y2pm.packageSelectionRestoreState()*/);
}

// ------------------------
/**
   @builtin ClearSaveState

   @short clear a saved state (to reduce memory consumption)
   @return boolean

*/
YCPValue
PkgModuleFunctions::ClearSaveState ()
{
# warning ClearSaveState is not implemented
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
PkgModuleFunctions::IsManualSelection ()
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
PkgModuleFunctions::PkgAnyToDelete ()
{
    bool ret = false;

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
PkgModuleFunctions::PkgAnyToInstall ()
{
    bool ret = false;

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
PkgModuleFunctions::FilterPackages(const YCPBoolean& y_byAuto, const YCPBoolean& y_byApp, const YCPBoolean& y_byUser, const YCPBoolean& y_names_only)
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
		    byUser && it->status().isByUser()
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
   available packages (from the installation source), `removed all packages selected for removal
   @param boolean names_only If true, return package names only
   @return list<string>

*/

YCPValue
PkgModuleFunctions::GetPackages(const YCPSymbol& y_which, const YCPBoolean& y_names_only)
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
		pkg2list(packages, it, names_only);
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
 * @short Update Packages marked for installation
 * @description
 * mark all packages for installation which are installed and have
 * an available candidate.
 *
 * This will mark packages for installation *and* for deletion (if a
 * package provides/obsoletes another package)
 *
 * This function does not solve dependencies
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
 * @return map<symbol,integer>
 */

YCPMap
PkgModuleFunctions::PkgUpdateAll (const YCPBoolean& del)
{
    zypp::UpgradeStatistics stats;
    stats.delete_unmaintained = del->value();

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
    data->add( YCPSymbol("DeleteUnmaintained"),YCPInteger( stats.delete_unmaintained ) );

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
PkgModuleFunctions::PkgInstall (const YCPString& p)
{
    std::string name = p->value();
    if (name.empty())
	return YCPBoolean (false);

    // ensure installation of the 'best' architecture

    return YCPBoolean( DoProvideNameKind( name, zypp::ResTraits<zypp::Package>::kind) );
}


/**
   @builtin PkgSrcInstall

   @short Select source of package for installation
   @param string package
   @return boolean

*/
YCPValue
PkgModuleFunctions::PkgSrcInstall (const YCPString& p)
{
    std::string name = p->value();
    if (name.empty())
	return YCPBoolean (false);

    // ensure installation of the 'best' architecture

    return YCPBoolean( DoProvideNameKind( name, zypp::ResTraits<zypp::SrcPackage>::kind) );
}


/**
   @builtin PkgDelete

   @short Select package for deletion
   @param string package
   @return boolean

*/
#warning This is bogus, as we have multiple matching (kernel) packages

YCPValue
PkgModuleFunctions::PkgDelete (const YCPString& p)
{
    std::string name = p->value();
    if (name.empty())
	return YCPBoolean (false);

    try
    {
	// find the package
	zypp::ResPool::byName_iterator it = std::find_if (
	    zypp_ptr()->pool().byNameBegin(name)
	    , zypp_ptr()->pool().byNameEnd(name)
	    , zypp::functor::chain (
		zypp::resfilter::ByInstalled (),
		zypp::resfilter::ByKind( zypp::ResTraits<zypp::Package>::kind )
	      )
	);


	// set the status to uninstalled
	return YCPBoolean( (it != zypp_ptr()->pool().byNameEnd(name))
	    && it->status().setToBeUninstalled(whoWantsIt) );
    }
    catch (...)
    {
    }

    return YCPBoolean (false);
}


/**
   @builtin PkgTaboo

   @short Set package to taboo
   @param string package
   @return boolean

*/
#warning This is bogus, as we have multiple matching packages

YCPValue
PkgModuleFunctions::PkgTaboo (const YCPString& p)
{
    std::string name = p->value();
    if (name.empty())
	return YCPBoolean (false);

    try
    {
	// find the package
	zypp::ResPool::byName_iterator it = std::find_if (
	    zypp_ptr()->pool().byNameBegin(name)
	    , zypp_ptr()->pool().byNameEnd(name)
	    , zypp::resfilter::ByKind(zypp::ResTraits<zypp::Package>::kind)
	);

	// remove the transactions, lock the status
	return YCPBoolean( (it != zypp_ptr()->pool().byNameEnd(name))
	    && it->status().resetTransact(whoWantsIt)
	    && it->status().setLock(true, whoWantsIt)
	);
    }
    catch (...)
    {
    }

    return YCPBoolean (false);
}

/**
   @builtin PkgNeutral

   @short Set package to neutral (drop install/delete flags)
   @param string package
   @return boolean

*/
#warning This is bogus, as we have multiple matching packages

YCPValue
PkgModuleFunctions::PkgNeutral (const YCPString& p)
{
    std::string name = p->value();
    if (name.empty())
	return YCPBoolean (false);

    try
    {
	// find the package
	zypp::ResPool::byName_iterator it = std::find_if (
	    zypp_ptr()->pool().byNameBegin(name)
	    , zypp_ptr()->pool().byNameEnd(name)
	    , zypp::resfilter::ByKind(zypp::ResTraits<zypp::Package>::kind)
	);

	// reset all transactions
	return YCPBoolean( (it != zypp_ptr()->pool().byNameEnd(name))
	    && it->status().resetTransact(whoWantsIt) );
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
   @return boolean
 */

YCPValue
PkgModuleFunctions::PkgReset ()
{
    try
    {
	for (zypp::ResPool::const_iterator it = zypp_ptr()->pool().begin()
	    ; it != zypp_ptr()->pool().end()
	    ; ++it)
	{
	    // reset all transaction flags
             if (it->status().isByUser())
                 it->status().setLock(false, zypp::ResStatus::USER);
	    it->status().resetTransact(zypp::ResStatus::USER);
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
PkgModuleFunctions::PkgApplReset ()
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
   @optarg booean filter  filter all conflicts with installed packages
   (installed packages will be preferred)
   @return boolean

*/
YCPBoolean
PkgModuleFunctions::PkgSolve (const YCPBoolean& filter)
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
PkgModuleFunctions::PkgEstablish ()
{
    bool result = false;

    try
    {
	result = zypp_ptr()->resolver()->establishPool();
    }
    catch (const zypp::Exception& excpt)
    {
	y2error("An error occurred during Pkg::PkgEstablish.");
	_last_error.setLastError(excpt.asUserString(), "See /var/log/YaST2/badlist for more information.");
    }

    // save information about failed dependencies to file
    if (!result)
    {
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
   @builtin PkgFreshen
   @short check all package freshens and schedule matching ones for installation
   @return boolean

   Returns true. (If no pool item 'transacts')

   The pool should NOT have any items set to 'transact' (scheduled for installation
   or removal)
   If it has, dependencies will be solved and the returned result might be false.

*/

YCPBoolean
PkgModuleFunctions::PkgFreshen()
{
    bool result = false;

    try
    {
	result = zypp_ptr()->resolver()->freshenPool();
    }
    catch (const zypp::Exception& excpt)
    {
	y2error("An error occurred during Pkg::PkgFreshen.");
	_last_error.setLastError(excpt.asUserString(), "See /var/log/YaST2/badlist for more information.");
    }

    // save information about failed dependencies to file
    if (!result)
    {
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
   @builtin PkgSolveCheckTargetOnly

   @short Solve packages currently installed on target system.
   @description
   Solve packages currently installed on target system. Packages status
   remains unchanged, i.e. does not select/deselect any packages to
   resolve failed dependencies.

   @return boolean
*/
YCPBoolean
PkgModuleFunctions::PkgSolveCheckTargetOnly()
{
    // create pool just with objects from target
    zypp::ResStore store;

    try
    {
	store = zypp_ptr()->target()->resolvables();
    }
    catch (...)
    {
	return YCPBoolean(false);
    }

    zypp::ResPoolManager pool;
    pool.insert(store.begin(), store.end(), true);

    // create resolver
    zypp::Resolver solver(pool.accessor());

    bool result = false;

    try
    {
	// verify consistency of system
	result = solver.verifySystem();
    }
    catch (const zypp::Exception& excpt)
    {
	y2error("An error occurred during Pkg::PkgSolveCheckTargetOnly");
	_last_error.setLastError(excpt.asUserString());
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
PkgModuleFunctions::PkgSolveErrors()
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
PkgModuleFunctions::PkgCommit (const YCPInteger& media)
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
	_last_error.setLastError(excpt.asUserString());
	return YCPVoid();
    }

    try {
	zypp::SourceManager::sourceManager()->releaseAllSources();
    }
    catch (const zypp::Exception& excpt)
    {
	y2error("Pkg::Commit has failed: cannot release all sources");
	_last_error.setLastError(excpt.asUserString());
    }

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
	remlist->add(YCPString(it->resolvable()->name()));
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
PkgModuleFunctions::GetBackupPath ()
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
PkgModuleFunctions::SetBackupPath (const YCPString& p)
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
PkgModuleFunctions::CreateBackups (const YCPBoolean& flag)
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
YCPString PkgModuleFunctions::PkgGetLicenseToConfirm( const YCPString & package )
{
    std::string pkgname = package->value();

    if (!pkgname.empty())
    {
	try
	{
	    // find the uninstalled (!) package
	    zypp::ResPool::byName_iterator it = std::find_if (
		zypp_ptr()->pool().byNameBegin(pkgname)
		, zypp_ptr()->pool().byNameEnd(pkgname)
		, zypp::functor::chain(
		    zypp::resfilter::ByUninstalled (),
		    zypp::resfilter::ByKind( zypp::ResTraits<zypp::Package>::kind ) )
	    );

	    if (it != zypp_ptr()->pool().byNameEnd(pkgname) && !it->status().isLicenceConfirmed())
	    {
		// cast to Package object
		zypp::Package::constPtr package = zypp::dynamic_pointer_cast<const zypp::Package>(it->resolvable());

		// get the license
		zypp::License license = package->licenseToConfirm();
		return YCPString(license);
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
YCPMap PkgModuleFunctions::PkgGetLicensesToConfirm( const YCPList & packages )
{
    YCPMap ret;

    for ( int i = 0; i < packages->size(); ++i ) {
	std::string pkgname = packages->value(i)->asString()->value();

	if (!pkgname.empty())
	{
	    try
	    {
		// find the uninstalled (!) package
		zypp::ResPool::byName_iterator it = std::find_if(
		    zypp_ptr()->pool().byNameBegin(pkgname)
		    , zypp_ptr()->pool().byNameEnd(pkgname)
		    , zypp::functor::chain(
			zypp::resfilter::ByUninstalled (),
			zypp::resfilter::ByKind( zypp::ResTraits<zypp::Package>::kind ) )
		);

		// found a package?
		if (it != zypp_ptr()->pool().byNameEnd(pkgname))
		{
		    // cast to Package object
		    zypp::Package::constPtr package = zypp::dynamic_pointer_cast<const zypp::Package>(it->resolvable());
		    zypp::License license = package->licenseToConfirm();

		    // has the license already been confirmed?
		    if (!license.empty() && !it->status().isLicenceConfirmed())
		    {
			ret->add(packages->value(i), YCPString(license));
		    }
		}
	    }
	    catch (...)
	    {
	    }
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
#warning This is bogus, as we have multiple matching packages
YCPBoolean PkgModuleFunctions::PkgMarkLicenseConfirmed (const YCPString & package)
{
    std::string pkgname = package->value();

    if (!pkgname.empty())
    {
	try
	{
	    // find the package
	    zypp::ResPool::byName_iterator it = std::find_if(
		zypp_ptr()->pool().byNameBegin(pkgname)
		, zypp_ptr()->pool().byNameEnd(pkgname)
		, zypp::functor::chain(
		    zypp::resfilter::ByUninstalled (),
		    zypp::resfilter::ByKind( zypp::ResTraits<zypp::Package>::kind ) )
	    );

	    if (it != zypp_ptr()->pool().byNameEnd(pkgname))
	    {
		// confirm the license
		it->status().setLicenceConfirmed(true);
		return YCPBoolean( true );
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
YCPBoolean PkgModuleFunctions::RpmChecksig( const YCPString & filename )
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
