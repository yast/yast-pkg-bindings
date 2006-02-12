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

#include <zypp/ResPool.h>
#include <zypp/Package.h>
#include <zypp/SourceManager.h>
#include <zypp/UpgradeStatistics.h>
#include <zypp/target/rpm/RpmDb.h>

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
    for (zypp::ResPool::const_indexiterator it = zypp_ptr->pool().providesbegin(name);
	it != zypp_ptr->pool().providesend(name);
	++it)
    {
	// is it a package?
	if (zypp::isKind<zypp::Package>(it->second.second.resolvable()))
	{

	    // cast to Package object
	    zypp::Package::constPtr package = zypp::dynamic_pointer_cast<const zypp::Package>(it->second.second.resolvable());
	    std::string pkgname = package->name();

	    // get instance status
	    bool installed = it->second.second.status().staysInstalled();
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
	    bool uninstalled = it->second.second.status().staysUninstalled() || it->second.second.status().isToBeUninstalled();
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
# warning PkgMediaNames is not implemented
    // get sources in installation order
// FIXME    InstSrcManager::ISrcIdList inst_order( _y2pm.instSrcManager().instOrderSources() );

    YCPList ycpnames;

/* TODO FIXME
    for (InstSrcManager::ISrcIdList::const_iterator it = inst_order.begin(); it != inst_order.end(); ++it)
    {
	ycpnames->add (YCPString ((*it)->descr()->content_product().asPkgNameEd().name));
    }
*/
    return ycpnames;
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
# warning PkgMediaSizes is not implemented
/* TODO FIXME
    // get sources in installation order
    InstSrcManager::ISrcIdList inst_order( _y2pm.instSrcManager().instOrderSources() );

    // compute max number of media across all sources
    // need a source rank map here since the package reveals the source rank as
    // a nonambiguous id for its source.
    // we will later re-map it back to the installation order. This is why InstSrcManager::ISrcId
    // is included.

    // map of < source rank, < src id, vector of media sizes >

    map <unsigned int, pair <InstSrcManager::ISrcId, vector<FSize> > > mediasizes;

    for (InstSrcManager::ISrcIdList::const_iterator it = inst_order.begin(); it != inst_order.end(); ++it)
    {
        vector<FSize> vf ((*it)->descr()->media_count(), FSize (0));
        mediasizes[(*it)->descr()->default_rank()] = std::make_pair (*it, vf);
    }

    // scan over all packages

    for (PMPackageManager::PMSelectableVec::const_iterator pkg = _y2pm.packageManager().begin();
         pkg != _y2pm.packageManager().end(); ++pkg)
    {
      if ( ! ( (*pkg)->to_install() || (*pkg)->source_install() ) )
	continue;

      PMPackagePtr package = (*pkg)->candidateObj();
      if (!package)
      {
	continue;
      }

      map <unsigned int, pair <InstSrcManager::ISrcId, vector<FSize> > >::iterator mediapair = mediasizes.find (package->instSrcRank());
      if (mediapair == mediasizes.end())
      {
	y2error ("Unknown rank %d for '%s'", package->instSrcRank(), (*pkg)->name()->c_str() );
	continue;
      }

      if ( (*pkg)->to_install() ) {
	unsigned int medianr = package->medianr();
	FSize size = package->size();
	if (medianr > mediapair->second.second.size())
        {
	  y2error ("resize needed %d", medianr);
	  mediapair->second.second.resize (medianr);
	}
	mediapair->second.second[medianr-1] += size;
      }

      if ( (*pkg)->source_install() ) {
	unsigned int medianr = atoi( package->sourceloc().c_str() );
	FSize size = package->sourcesize();
	if (medianr > mediapair->second.second.size())
        {
	  y2error ("resize needed %d", medianr);
	  mediapair->second.second.resize (medianr);
	}
	mediapair->second.second[medianr-1] += size;
      }
    }
*/
    // now convert back to list of lists in installation order

    YCPList ycpsizes;
/*
    for (InstSrcManager::ISrcIdList::const_iterator it = inst_order.begin(); it != inst_order.end(); ++it)
    {
        for (map <unsigned int, pair <InstSrcManager::ISrcId, vector<FSize> > >::iterator mediapair = mediasizes.begin();
             mediapair != mediasizes.end(); ++mediapair)
        {
            if (mediapair->second.first == *it)
            {
                YCPList msizes;

                for (unsigned int i = 0; i < mediapair->second.second.size(); ++i)
                {
                    msizes->add (YCPInteger (((long long)(mediapair->second.second[i]))));
                }
                ycpsizes->add (msizes);
            }
        }
    }
*/
    return ycpsizes;
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
# warning PkgMediaCount is not implemented
/* TODO FIXME
    // get sources in installation order
    InstSrcManager::ISrcIdList inst_order( _y2pm.instSrcManager().instOrderSources() );

    // compute max number of media across all sources
    // need a source rank map here since the package reveals the source rank as
    // a nonambiguous id for its source.
    // we will later re-map it back to the installation order. This is why InstSrcManager::ISrcId
    // is included.

    // map of < source rank, < src id, vector of package counts>

    map <unsigned int, pair <InstSrcManager::ISrcId, vector<int> > > media_counts;

    for (InstSrcManager::ISrcIdList::const_iterator it = inst_order.begin(); it != inst_order.end(); ++it)
    {
        vector<int> vf ((*it)->descr()->media_count(), 0);
        media_counts[(*it)->descr()->default_rank()] = std::make_pair (*it, vf);
    }

    // scan over all packages

    for (PMPackageManager::PMSelectableVec::const_iterator pkg = _y2pm.packageManager().begin();
         pkg != _y2pm.packageManager().end(); ++pkg)
    {
      if ( ! ( (*pkg)->to_install() || (*pkg)->source_install() ) )
	continue;

      PMPackagePtr package = (*pkg)->candidateObj();

      if (!package)
	continue;

      map <unsigned int, pair <InstSrcManager::ISrcId, vector<int> > >::iterator mediapair = media_counts.find (package->instSrcRank());
      if (mediapair == media_counts.end())
      {
	y2error ("Unknown rank %d for '%s'", package->instSrcRank(), (*pkg)->name()->c_str());
	continue;
      }

      if ( (*pkg)->to_install() )
      {
	unsigned int medianr = package->medianr();
	if (medianr > mediapair->second.second.size())
        {
	  y2error ("resize needed %d", medianr);
	  mediapair->second.second.resize (medianr);
	}
	mediapair->second.second[medianr-1] += 1;
      }

      if ( (*pkg)->source_install() ) {
	unsigned int medianr = atoi( package->sourceloc().c_str() );
	FSize size = package->sourcesize();
	if (medianr > mediapair->second.second.size())
        {
	  y2error ("resize needed %d", medianr);
	  mediapair->second.second.resize (medianr);
	}
	mediapair->second.second[medianr-1] += 1;
      }
    }
*/
    // now convert back to list of lists in installation order

    YCPList ycpsizes;

/*
    for (InstSrcManager::ISrcIdList::const_iterator it = inst_order.begin(); it != inst_order.end(); ++it)
    {
        for (map <unsigned int, pair <InstSrcManager::ISrcId, vector<int> > >::iterator mediapair = media_counts.begin();
             mediapair != media_counts.end(); ++mediapair)
        {
            if (mediapair->second.first == *it)
            {
                YCPList msizes;

                for (unsigned int i = 0; i < mediapair->second.second.size(); ++i)
                {
                    msizes->add (YCPInteger (((long long)(mediapair->second.second[i]))));
                }
                ycpsizes->add (msizes);
            }
        }
    }
*/

    return ycpsizes;
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


/**
 * helper function, install a package which provides tag (as a
 *   package name, a provided tag, or a provided file
*/

bool
PkgModuleFunctions::DoProvideNameKind( const std::string & name, zypp::Resolvable::Kind kind)
{
    zypp::ResPool::byName_iterator it = std::find_if (
	zypp_ptr->pool().byNameBegin(name)
	, zypp_ptr->pool().byNameEnd(name)
	, zypp::resfilter::ByKind(kind)
    );
		
    if (it == zypp_ptr->pool().byNameEnd(name)) 
	return false;

    // this might not be exact - it could be APPLICATION
    bool result = it->status().setToBeInstalled(zypp::ResStatus::USER);
    y2milestone ("DoProvideNameKind %s -> %s\n", name.c_str(), (result ? "Ok" : "Bad"));
    return true;
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
	, zypp::resfilter::ByKind(kind)
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
	    // cast to Package object
	    zypp::Package::constPtr package = zypp::dynamic_pointer_cast<const zypp::Package>(it->resolvable());

	    data->add(YCPString("arch"), YCPString((*it)->arch().asString()));
	    data->add( YCPString("medianr"), YCPInteger(package->mediaId()));
    
	    zypp::Source_Ref pkg_src = (*it)->source();
	    unsigned int srcid = 0;
	    bool found = false;

	    // search source
	    while(!found)
	    {
		zypp::Source_Ref src = zypp::SourceManager::sourceManager()->findSource(srcid);
    
		if (src)
		{
		    if (src == pkg_src)
		    {
			found = true;
		    }
		    else
		    {
			srcid++;
		    }
		}
		else
		{
		    break;
		}
	    }

	    if (found)
	    {
		// src has been found
		data->add( YCPString("srcid"), YCPInteger(srcid));
	    }
    
	    /* FIXME: not implemented in zypp yet
	    data->add( YCPString("location"), YCPString(package->location()));
	    */

	    return data;
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

#warning PkgLocation is not implemented yet
	    return YCPString("package->location()");
	}
    }
    
    return YCPVoid();
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
    // TODO FIXME
    return YCPBoolean (false /*_y2pm.packageManager().anythingByUser()
			|| _y2pm.selectionManager().anythingByUser()*/);
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
# warning PkgTaboo is not implemented
    /* TODO FIXME
    PMSelectablePtr selectable = getPackageSelectable (p->value ());
    if (!selectable)
    {
	return YCPBoolean (false);
    }
    return YCPBoolean (selectable->user_set_taboo());
    */
    return YCPBoolean (true);
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
# warning PkgReset is not implemented
    // TODO FIXME
    //_y2pm.selectionManager().setNothingSelected();
    //_y2pm.packageManager().setNothingSelected();

    // FIXME also reset "conflict ignore list" in UI

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
	    y2error ("PkgSolve: %zd packages failed (see /var/log/YaST2/badlist)", problem_size);

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

    zypp::PoolItemList errors;
    zypp::PoolItemList remaining;
    zypp::PoolItemList srcremaining;
    int success = 0;

    try
    {
	success = zypp_ptr->target()->commit(zypp_ptr->pool(), medianr, errors, remaining, srcremaining);
    }
    catch (...)
    {
	y2error("Pkg::Commit has failed");
    }

    YCPList ret;

    ret->add(YCPInteger(success));

    YCPList errlist;
    for (zypp::PoolItemList::const_iterator it = errors.begin(); it != errors.end(); ++it)
    {
	errlist->add(YCPString(it->resolvable()->name()));
    }
    ret->add(errlist);

    YCPList remlist;
    for (zypp::PoolItemList::const_iterator it = remaining.begin(); it != remaining.end(); ++it)
    {
	remlist->add(YCPString(it->resolvable()->name()));
    }
    ret->add(remlist);

    YCPList srclist;
    for (zypp::PoolItemList::const_iterator it = srcremaining.begin(); it != srcremaining.end(); ++it)
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

	if (it != zypp_ptr->pool().byNameEnd(pkgname))
	{
	    // cast to Package object
	    zypp::Package::constPtr package = zypp::dynamic_pointer_cast<const zypp::Package>(it->resolvable());

	    zypp::License license = package->licenseToConfirm();

	    return YCPString(license);
	}
    }
  
# warning Has the license already been confirmed?
    // TODO FIXME - check whether the license has been already confirmed?
/*  if (! PMPackagePtr(selectable->candidateObj())->hasLicenseToConfirm ()) {
    // license was confirmed before
    return YCPString ("");
  }
  */

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

	    if (it != zypp_ptr->pool().byNameEnd(pkgname))
	    {
		// cast to Package object
		zypp::Package::constPtr package = zypp::dynamic_pointer_cast<const zypp::Package>(it->resolvable());

# warning Has the license already been confirmed?
		// TODO FIXME - check whether the licenses have been already confirmed?
		zypp::License license = package->licenseToConfirm();
		if (!license.empty())
		{
		    ret->add(packages->value(i), YCPString(license));
		}
	    }
	}
    }

    return ret;
}

YCPBoolean PkgModuleFunctions::PkgMarkLicenseConfirmed (const YCPString & package)
{
# warning PkgMarkLicenseConfirmed is not implemented
  return YCPBoolean( true );
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
