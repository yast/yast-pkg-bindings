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
#include <zypp/Product.h>
#include <zypp/SourceManager.h>
#include <zypp/UpgradeStatistics.h>
#include <zypp/target/rpm/RpmDb.h>
#include <zypp/ZYppCommitResult.h>

#include <fstream>

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
    for (zypp::ResPool::byCapabilityIndex_iterator it = zypp_ptr->pool().byCapabilityIndexBegin(name, zypp::Dep::PROVIDES);
	it != zypp_ptr->pool().byCapabilityIndexEnd(name, zypp::Dep::PROVIDES);
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
 *  @return list<string> Names of Sources
 *  @usage Pkg::PkgMediaNames () -> [ "source_1_name", "source_2_name", ...]
 */
YCPValue
PkgModuleFunctions::PkgMediaNames ()
{
# warning No installation order

    std::list<unsigned> source_ids = zypp::SourceManager::sourceManager()->enabledSources();

    YCPList res;
    
    // initialize    
    for( list<unsigned>::const_iterator sit = source_ids.begin();
	sit != source_ids.end(); ++sit)
    {
	unsigned id = *sit;
	
	zypp::Source_Ref src = zypp::SourceManager::sourceManager()->findSource( id );

	// find a product for the given source
	zypp::ResPool::byKind_iterator it = zypp_ptr->pool().byKindBegin(zypp::ResTraits<zypp::Product>::kind);

	for( ; it != zypp_ptr->pool().byKindEnd(zypp::ResTraits<zypp::Product>::kind)
    	    ; ++it) {
	    zypp::Product::constPtr product = boost::dynamic_pointer_cast<const zypp::Product>( it->resolvable() );

	    y2debug ("Checking product: %s", product->displayName().c_str());
	    if( product->source() == src )
	    {
		y2debug ("Found");
		
		res->add( YCPString ( product->displayName() ) );
 		break; 
	    }
	}

	if( it == zypp_ptr->pool().byKindEnd(zypp::ResTraits<zypp::Product>::kind) )
	{
	    y2error ("Product for source '%d' not found", id);
	    
	    res->add( YCPString( src.url().asString() ) );
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

    std::list<unsigned> source_ids = zypp::SourceManager::sourceManager()->enabledSources();
    
    std::map<unsigned, std::vector<unsigned> > result;
    
    std::map<zypp::Source_Ref, unsigned> source_map;

    // initialize    
    for( list<unsigned>::const_iterator sit = source_ids.begin();
	sit != source_ids.end(); ++sit)
    {
	unsigned id = *sit;
	
	zypp::Source_Ref src = zypp::SourceManager::sourceManager()->findSource( id );
	unsigned media = src.numberOfMedia();
	
	result[id] = std::vector<unsigned>(media,0);
	
	source_map[ src ] = id;
    }
    
    for( zypp::ResPool::byKind_iterator it = zypp_ptr->pool().byKindBegin<zypp::Package>()
	; it != zypp_ptr->pool().byKindEnd<zypp::Package>()
	; ++it )
    {
	zypp::Package::constPtr pkg = boost::dynamic_pointer_cast<const zypp::Package>(it->resolvable());

	if( it->status().isToBeInstalled() )
	{
	    long long size = pkg->size();
	    result[ source_map[pkg->source()] ]
	      [pkg->mediaId()-1] += size ;	// media are numbered from 1
	}
    }
    
    YCPList res;
    
    for(std::map<unsigned, std::vector<unsigned> >::const_iterator it =
	result.begin(); it != result.end() ; ++it)
    {
	std::vector<unsigned> values = it->second;
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

    std::list<unsigned> source_ids = zypp::SourceManager::sourceManager()->enabledSources();
    
    std::map<unsigned, std::vector<unsigned> > result;
    
    std::map<zypp::Source_Ref, unsigned> source_map;

    // initialize    
    for( list<unsigned>::const_iterator sit = source_ids.begin();
	sit != source_ids.end(); ++sit)
    {
	unsigned id = *sit;
	
	zypp::Source_Ref src = zypp::SourceManager::sourceManager()->findSource( id );
	unsigned media = src.numberOfMedia();
	
	result[id] = std::vector<unsigned>(media,0);
	
	source_map[ src ] = id;
    }
    
    for( zypp::ResPool::byKind_iterator it = zypp_ptr->pool().byKindBegin<zypp::Package>()
	; it != zypp_ptr->pool().byKindEnd<zypp::Package>()
	; ++it )
    {
	zypp::Package::constPtr pkg = boost::dynamic_pointer_cast<const zypp::Package>(it->resolvable());

	if( it->status().isToBeInstalled() )
	    result[ source_map[pkg->source()] ]
	      [pkg->mediaId()-1]++ ;	// media are numbered from 1
    }
    
    YCPList res;
    
    for(std::map<unsigned, std::vector<unsigned> >::const_iterator it =
	result.begin(); it != result.end() ; ++it)
    {
	std::vector<unsigned> values = it->second;
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
/**
 *  @builtin IsProvided
 *
 *  @short returns  'true' if the tag is provided in the installed system
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

# warning support tags, not package only
    zypp::ResPool::byName_iterator it = std::find_if (
	zypp_ptr->pool().byNameBegin(name)
	, zypp_ptr->pool().byNameEnd(name)
	, zypp::resfilter::ByInstalled()
    );

    return YCPBoolean( it != zypp_ptr->pool().byNameEnd(name) );
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

# warning support tags, not package only
    zypp::ResPool::byName_iterator it = std::find_if (
	zypp_ptr->pool().byNameBegin(name)
	, zypp_ptr->pool().byNameEnd(name)
	, zypp::functor::true_c()
    );

    return YCPBoolean( (it != zypp_ptr->pool().byNameEnd(name)) 
	&& it->status().isToBeInstalled() );
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

#warning search for tags as well, we do package name only
    zypp::ResPool::byName_iterator it = std::find_if (
	zypp_ptr->pool().byNameBegin(name)
	, zypp_ptr->pool().byNameEnd(name)
	, zypp::resfilter::ByKind(zypp::ResTraits<zypp::Package>::kind)
    );

    return YCPBoolean( it != zypp_ptr->pool().byNameEnd(name) ); 
}


struct ProvideProcess
{
    zypp::PoolItem_Ref item;
    zypp::Arch _architecture;

    ProvideProcess( zypp::Arch arch )
	: _architecture( arch )
    { }

    bool operator()( zypp::PoolItem provider )
    {
        if (!provider->arch().compatibleWith( _architecture )) {
            MIL << "provider " << provider << " has incompatible arch '" << provider->arch() << "'" << endl;
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

	return true;
    }

};


/**
 * helper function, install a package which provides tag (as a
 *   package name, a provided tag, or a provided file
*/

bool
PkgModuleFunctions::DoProvideNameKind( const std::string & name, zypp::Resolvable::Kind kind)
{
    ProvideProcess info( zypp_ptr->architecture() );

    invokeOnEach( zypp_ptr->pool().byNameBegin( name ),
		  zypp_ptr->pool().byNameEnd( name ),
		  zypp::resfilter::ByKind( kind ),
		  zypp::functor::functorRef<bool,zypp::PoolItem> (info) 
    );
		
    if (!info.item) 
	return false;

    // this might not be exact - it could be APPLICATION
    bool result = info.item.status().setToBeInstalled(zypp::ResStatus::USER);
    y2milestone ("DoProvideNameKind %s -> %s\n", name.c_str(), (result ? "Ok" : "Bad"));
    return true;
}

/*
 * Helper function
 */
bool
PkgModuleFunctions::DoAllKind(zypp::Resolvable::Kind kind, bool provide)
{
    bool ret = true;

    for (zypp::ResPool::byKind_iterator it = zypp_ptr->pool().byKindBegin(kind);
	it != zypp_ptr->pool().byKindEnd(kind); ++it)
    {
	bool res = provide ? it->status().setToBeInstalled(zypp::ResStatus::USER)
	    : it->status().setToBeUninstalled(zypp::ResStatus::USER);
	
	y2milestone ("%s %s -> %s\n", (provide ? "Install" : "Remove"), (*it)->name().c_str(), (res ? "Ok" : "Failed"));
	ret = ret && res;
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
 * helper function, deinstall a package which provides tag (as a
 *   package name, a provided tag, or a provided file
*/

bool
PkgModuleFunctions::DoRemoveNameKind( const std::string & name, zypp::Resolvable::Kind kind)
{
#warning support tags, not package only
    zypp::ResPool::byName_iterator it = std::find_if (
	zypp_ptr->pool().byNameBegin(name)
	, zypp_ptr->pool().byNameEnd(name)
	, zypp::functor::chain( zypp::resfilter::ByInstalled(), zypp::resfilter::ByKind(kind) )
    );
		
    if 	(it == zypp_ptr->pool().byNameEnd(name)) 
	return false;
	
    // this might not be exact - it could be APPLICATION	
    bool result = it->status().setToBeUninstalled(zypp::ResStatus::USER);
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
/**
   @builtin PkgSummary

   @short Get summary (aka label) of a package
   @param string package
   @return string Summary
   @usage Pkg::PkgSummary (string package) -> "This is a nice package"

*/
YCPValue
PkgModuleFunctions::PkgSummary (const YCPString& p)
{
    std::string name = p->value();

    zypp::ResPool::byName_iterator it = std::find_if (
	zypp_ptr->pool().byNameBegin(name)
	, zypp_ptr->pool().byNameEnd(name)
	, zypp::resfilter::ByKind(zypp::ResTraits<zypp::Package>::kind)
    );

    if (it == zypp_ptr->pool().byNameEnd(name))
    {
        return YCPVoid();
    }

    return YCPString((*it)->summary());
}

// ------------------------
/**
   @builtin PkgVersion

   @short Get version (better: edition) of a package
   @param string package
   @return string
   @usage Pkg::PkgVersion (string package) -> "1.42-39"

*/
YCPValue
PkgModuleFunctions::PkgVersion (const YCPString& p)
{
    std::string name = p->value();

    zypp::ResPool::byName_iterator it = std::find_if (
	zypp_ptr->pool().byNameBegin(name)
	, zypp_ptr->pool().byNameEnd(name)
	, zypp::resfilter::ByKind(zypp::ResTraits<zypp::Package>::kind)
    );

    if (it == zypp_ptr->pool().byNameEnd(name))
    {
        return YCPVoid();
    }

    // return edition (version-release)
    return YCPString((*it)->edition().asString());
}

// ------------------------
/**
   @builtin PkgSize

   @short Get (installed) size of a package
   @param string package
   @return integer
   @usage Pkg::PkgSize (string package) -> 12345678

*/
YCPValue
PkgModuleFunctions::PkgSize (const YCPString& p)
{
    std::string pkgname = p->value();
    zypp::ByteCount ret;

    if (!pkgname.empty())
    {
	// find the package
	zypp::ResPool::byName_iterator it = std::find_if (
	    zypp_ptr->pool().byNameBegin(pkgname)
	    , zypp_ptr->pool().byNameEnd(pkgname)
	    , zypp::resfilter::ByKind(zypp::ResTraits<zypp::Package>::kind)
	);

	if (it != zypp_ptr->pool().byNameEnd(pkgname))
	{
	    // size() returns unpacked size of the package
	    ret = (*it)->size();
	}
    }
    else
    {
	return YCPVoid();
    }

    return YCPInteger (ret);
}

// ------------------------
/**
   @builtin PkgGroup

   @short Get rpm group of a package
   @param string package
   @return string
   @usage Pkg::PkgGroup (string package) -> string

*/
YCPValue
PkgModuleFunctions::PkgGroup (const YCPString& p)
{
    std::string pkgname = p->value();
    zypp::PackageGroup ret;

    if (!pkgname.empty())
    {
	// find the package
	zypp::ResPool::byName_iterator it = std::find_if (
	    zypp_ptr->pool().byNameBegin(pkgname)
	    , zypp_ptr->pool().byNameEnd(pkgname)
	    , zypp::resfilter::ByKind(zypp::ResTraits<zypp::Package>::kind)
	);

	if (it != zypp_ptr->pool().byNameEnd(pkgname))
	{
	    // cast to Package object
	    zypp::Package::constPtr package = zypp::dynamic_pointer_cast<const zypp::Package>(it->resolvable());
	    ret = package->group();
	}
    }
    else
    {
	return YCPVoid();
    }

    return YCPString(ret);
}


YCPValue
PkgModuleFunctions::PkgProp(zypp::ResPool::byName_iterator it)
{
    YCPMap data;
    
    // cast to Package object
    zypp::Package::constPtr package = zypp::dynamic_pointer_cast<const zypp::Package>(it->resolvable());

    data->add(YCPString("arch"), YCPString((*it)->arch().asString()));
    data->add( YCPString("medianr"), YCPInteger(package->mediaId()));

    zypp::Source_Ref pkg_src = (*it)->source();
    unsigned int srcid = 0;
    bool found = false;
    std::list<unsigned> enabled_srcs = zypp::SourceManager::sourceManager()->enabledSources();

    // search source
    for( std::list<unsigned>::const_iterator src_it = enabled_srcs.begin()
	; src_it != enabled_srcs.end()
	; src_it++)
    {
	zypp::Source_Ref src;

	try
	{
	    src = zypp::SourceManager::sourceManager()->findSource(*src_it);
	}
	catch (...)
	{
	    y2error("cannot find source %d", *src_it);
	    continue;
	}

	if (src == pkg_src)
	{
	    found = true;
	    srcid = *src_it;
	    break;
	}
    }

    if (found)
    {
	// src has been found
	data->add( YCPString("srcid"), YCPInteger(srcid));
    }

    std::string status("available");
    if (it->status().isInstalled())
    {
	status = "installed";
    }
    else if (it->status().isToBeInstalled())
    {
	status = "selected";
    }
    else if (it->status().isToBeUninstalled())
    {
	status = "removed";
    }
    data->add( YCPString("status"), YCPSymbol(status));

    data->add( YCPString("location"), YCPString(package->location().basename()));
    data->add( YCPString("path"), YCPString(package->location().asString()));

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
YCPValue
PkgModuleFunctions::PkgProperties (const YCPString& p)
{
    std::string pkgname = p->value();
    YCPMap data;

    if (!pkgname.empty())
    {
	// find the package
	zypp::ResPool::byName_iterator it = std::find_if (
	    zypp_ptr->pool().byNameBegin(pkgname)
	    , zypp_ptr->pool().byNameEnd(pkgname)
	    , zypp::resfilter::ByKind(zypp::ResTraits<zypp::Package>::kind)
	);

	if (it != zypp_ptr->pool().byNameEnd(pkgname))
	{
	    return PkgProp(it);
	}
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
	for (zypp::ResPool::byName_iterator it = zypp_ptr->pool().byNameBegin(pkgname);
	    it != zypp_ptr->pool().byNameEnd(pkgname); ++it)
	{
	    // is it a package?
	    if (zypp::isKind<zypp::Package>(it->resolvable()))
	    {
		data->add(PkgProp(it));
	    }
	}
    }
    
    return data;
}

/* helper function */
YCPValue
PkgModuleFunctions::GetPkgLocation (const YCPString& p, bool full_path)
{
    std::string pkgname = p->value();
    YCPMap data;

    if (!pkgname.empty())
    {
	// find the package
	zypp::ResPool::byName_iterator it = std::find_if (
	    zypp_ptr->pool().byNameBegin(pkgname)
	    , zypp_ptr->pool().byNameEnd(pkgname)
	    , zypp::resfilter::ByKind(zypp::ResTraits<zypp::Package>::kind)
	);

	if (it != zypp_ptr->pool().byNameEnd(pkgname))
	{
	    // cast to Package object
	    zypp::Package::constPtr package = zypp::dynamic_pointer_cast<const zypp::Package>(it->resolvable());

	    return (full_path) ? YCPString(package->location().asString()) :
		YCPString(package->location().basename());
	}
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
	for (zypp::ResPool::byName_iterator it = zypp_ptr->pool().byNameBegin(pkgname);
	    it != zypp_ptr->pool().byNameEnd(pkgname); ++it)
	{
	    // is it a package?
	    if (zypp::isKind<zypp::Package>(it->resolvable()))
	    {
		if (type == "any" ||
		    (type == "installed" && it->status().wasInstalled()) ||
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
    for (zypp::ResPool::byKind_iterator it = zypp_ptr->pool().byKindBegin(zypp::ResTraits<zypp::Package>::kind);
	it != zypp_ptr->pool().byKindEnd(zypp::ResTraits<zypp::Package>::kind);
	++it)
    {
	// return true if there is a package installed/removed by user
	if ((it->status().isToBeInstalled() || it->status().isToBeUninstalled()) && it->status().isByUser())
	{
	    return YCPBoolean(true);
	}
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
    
    for (zypp::ResPool::byKind_iterator it = zypp_ptr->pool().byKindBegin(zypp::ResTraits<zypp::Package>::kind);
	it != zypp_ptr->pool().byKindEnd(zypp::ResTraits<zypp::Package>::kind);
	++it)
    {
	if (it->status().isToBeUninstalled())
	{
	    ret = true;
	    break;
	}
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
    
    for (zypp::ResPool::byKind_iterator it = zypp_ptr->pool().byKindBegin(zypp::ResTraits<zypp::Package>::kind);
	it != zypp_ptr->pool().byKindEnd(zypp::ResTraits<zypp::Package>::kind);
	++it)
    {
	if (it->status().isToBeInstalled())
	{
	    ret = true;
	    break;
	}
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

    for (zypp::ResPool::byKind_iterator it = zypp_ptr->pool().byKindBegin(zypp::ResTraits<zypp::Package>::kind);
	it != zypp_ptr->pool().byKindEnd(zypp::ResTraits<zypp::Package>::kind);
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

    return packages;
}

/**
   @builtin GetPackages

   @short Get list of packages (installed, selected, available)
   @description
   return list of packages (["pkg1", "pkg2", ...] if names_only==true,
   ["pkg1 version release arch", "pkg1 version release arch", ... if
   names_only == false]

   @param symbol 'which' defines which packages are returned: `installed all installed packages,
   `selected returns all selected but not yet installed packages, `available returns all
   available packages (from the installation source)
   @param boolean names_only If true, return package names only
   @return list<string>

*/

YCPValue
PkgModuleFunctions::GetPackages(const YCPSymbol& y_which, const YCPBoolean& y_names_only)
{
    string which = y_which->symbol();
    bool names_only = y_names_only->value();

    YCPList packages;

    for (zypp::ResPool::byKind_iterator it = zypp_ptr->pool().byKindBegin(zypp::ResTraits<zypp::Package>::kind);
	it != zypp_ptr->pool().byKindEnd(zypp::ResTraits<zypp::Package>::kind);
	++it)
    {
	if (which == "installed")
	{
	    if (it->status().wasInstalled())
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
	else if (which == "available")
	{
	    pkg2list(packages, it, names_only);
	}
	else
	{
	    return YCPError ("Wrong parameter for Pkg::GetPackages");
	}
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

    // solve upgrade, get statistics
    zypp_ptr->resolver()->doUpgrade(stats);

    YCPMap data;

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

    // find the package
    zypp::ResPool::byName_iterator it = std::find_if (
	zypp_ptr->pool().byNameBegin(name)
	, zypp_ptr->pool().byNameEnd(name)
	, zypp::resfilter::ByKind(zypp::ResTraits<zypp::Package>::kind)
    );

    // set the status to installed
    return YCPBoolean( (it != zypp_ptr->pool().byNameEnd(name))
	// set installed by user
	&& it->status().setToBeInstalled(zypp::ResStatus::USER) );
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
#warning PkgSrcInstall is not implemented
    /* TODO FIXME
    PMSelectablePtr selectable = getPackageSelectable (p->value ());

    if (!selectable)
    {
	return YCPBoolean (false);
    }
    return YCPBoolean (selectable->set_source_install(true));
    */
    return YCPBoolean(true);
}


/**
   @builtin PkgDelete

   @short Select package for deletion
   @param string package
   @return boolean

*/
YCPValue
PkgModuleFunctions::PkgDelete (const YCPString& p)
{
    std::string name = p->value();
    if (name.empty())
	return YCPBoolean (false);

    // find the package
    zypp::ResPool::byName_iterator it = std::find_if (
	zypp_ptr->pool().byNameBegin(name)
	, zypp_ptr->pool().byNameEnd(name)
	, zypp::resfilter::ByKind(zypp::ResTraits<zypp::Package>::kind)
    );

    
    // set the status to uninstalled
    return YCPBoolean( (it != zypp_ptr->pool().byNameEnd(name)) 
	// set uninstalled by user
	&& it->status().setToBeUninstalled(zypp::ResStatus::USER) );
}


/**
   @builtin PkgTaboo

   @short Set package to taboo
   @param string package
   @return boolean

*/
YCPValue
PkgModuleFunctions::PkgTaboo (const YCPString& p)
{
    std::string name = p->value();
    if (name.empty())
	return YCPBoolean (false);

    // find the package
    zypp::ResPool::byName_iterator it = std::find_if (
	zypp_ptr->pool().byNameBegin(name)
	, zypp_ptr->pool().byNameEnd(name)
	, zypp::resfilter::ByKind(zypp::ResTraits<zypp::Package>::kind)
    );

    // lock the status
    return YCPBoolean( (it != zypp_ptr->pool().byNameEnd(name)) 
	// set locked by user
	&& it->status().setLock(true, zypp::ResStatus::USER) );
}

/**
   @builtin PkgNeutral

   @short Set package to neutral (drop install/delete flags)
   @param string package
   @return boolean

*/
YCPValue
PkgModuleFunctions::PkgNeutral (const YCPString& p)
{
    std::string name = p->value();
    if (name.empty())
	return YCPBoolean (false);

    // find the package
    zypp::ResPool::byName_iterator it = std::find_if (
	zypp_ptr->pool().byNameBegin(name)
	, zypp_ptr->pool().byNameEnd(name)
	, zypp::resfilter::ByKind(zypp::ResTraits<zypp::Package>::kind)
    );

    // reset all transactions
    return YCPBoolean( (it != zypp_ptr->pool().byNameEnd(name)) 
	// set neutral by user
	&& it->status().setTransact(false, zypp::ResStatus::USER) );
}


/**
 * @builtin Reset
 *
 * @short Reset most internal stuff on the package manager.
   @param string package
   @return boolean
 */
YCPValue
PkgModuleFunctions::PkgReset ()
{
    for (zypp::ResPool::const_iterator it = zypp_ptr->pool().begin()
	; it != zypp_ptr->pool().end()
	; ++it)
    {
	// reset all transaction flags
	it->status().setTransact(false, zypp::ResStatus::USER) ;
	it->status().setTransact(false, zypp::ResStatus::APPL_HIGH) ;
	it->status().setTransact(false, zypp::ResStatus::APPL_LOW) ;
	it->status().setTransact(false, zypp::ResStatus::SOLVER) ;
    }

    return YCPBoolean (true);
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
	result = zypp_ptr->resolver()->resolvePool();
    }
    catch (...)
    {
	y2error("An error occurred during Pkg::PkgSolve.");
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
	zypp::ResolverProblemList problems = zypp_ptr->resolver()->problems();
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
#warning PkgSolveCheckTargetOnly is not implemented
/* TODO FIXME
  PkgDep::ErrorResultList bad;

  if ( ! _y2pm.packageManager().solveConsistent( bad ) )
  {
    _solve_errors = bad.size();
    y2error ("SolveCheckTarget: %zd packages failed (see /var/log/YaST2/badlist)", bad.size());

    std::ofstream out ("/var/log/YaST2/badlist");
    out << bad.size() << " packages failed" << std::endl;
    for( PkgDep::ErrorResultList::const_iterator p = bad.begin();
	 p != bad.end(); ++p )
    {
      out << *p << std::endl;
    }

    return YCPBoolean (false);
  }
  */
  return YCPBoolean (true);
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
    return YCPInteger(zypp_ptr->resolver()->problems().size());
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

    try
    {
	result = zypp_ptr->commit(medianr);
    }
    catch (...)
    {
	y2error("Pkg::Commit has failed");
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
    return YCPString(zypp_ptr->target()->rpmDb().getBackupPath().asString());
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
    zypp_ptr->target()->rpmDb().setBackupPath(p->value());
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
    zypp_ptr->target()->rpmDb().createPackageBackups(flag->value());
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
	// find the package
	zypp::ResPool::byName_iterator it = std::find_if (
	    zypp_ptr->pool().byNameBegin(pkgname)
	    , zypp_ptr->pool().byNameEnd(pkgname)
	    , zypp::resfilter::ByKind(zypp::ResTraits<zypp::Package>::kind)
	);

	if (it != zypp_ptr->pool().byNameEnd(pkgname) && !it->status().isLicenceConfirmed())
	{
	    // cast to Package object
	    zypp::Package::constPtr package = zypp::dynamic_pointer_cast<const zypp::Package>(it->resolvable());

	    // get the license
	    zypp::License license = package->licenseToConfirm();
	    return YCPString(license);
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
	    // find the package
	    zypp::ResPool::byName_iterator it = std::find_if (
		zypp_ptr->pool().byNameBegin(pkgname)
		, zypp_ptr->pool().byNameEnd(pkgname)
		, zypp::resfilter::ByKind(zypp::ResTraits<zypp::Package>::kind)
	    );

	    // is the found package selected?
	    if (it != zypp_ptr->pool().byNameEnd(pkgname) && !it->status().isToBeInstalled())
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
    }

    return ret;
}

/**
   @builtin PkgMarkLicenseConfirmed

   @short Mark licence of the package confirmed
   @param string name of a package
   @return boolean true if the license has been successfuly confirmed
*/
YCPBoolean PkgModuleFunctions::PkgMarkLicenseConfirmed (const YCPString & package)
{
    std::string pkgname = package->value();
    
    if (!pkgname.empty())
    {
	// find the package
	zypp::ResPool::byName_iterator it = std::find_if (
	    zypp_ptr->pool().byNameBegin(pkgname)
	    , zypp_ptr->pool().byNameEnd(pkgname)
	    , zypp::resfilter::ByKind(zypp::ResTraits<zypp::Package>::kind)
	);

	if (it != zypp_ptr->pool().byNameEnd(pkgname))
	{
	    // confirm the license
	    it->status().setLicenceConfirmed(true);
	    return YCPBoolean( true );
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
    return YCPBoolean(zypp_ptr->target()->rpmDb().checkPackage(filename->value()) == 0);
}
