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

#include <iostream>
#include <fstream>
#include <utility>

#include <ycp/y2log.h>
#include <PkgModuleFunctions.h>

#include <y2util/Url.h>
#include <y2util/FSize.h>
#include <y2pm/InstData.h>
#include <y2pm/InstSrcDescr.h>
#include <y2pm/InstTarget.h>
#include <y2pm/PMObject.h>
#include <y2pm/PMSelectable.h>
#include <y2pm/PMPackageManager.h>
#include <y2pm/PMSelectionManager.h>
#include <y2pm/InstSrcManager.h>
#include <y2pm/RpmHeader.h>

#include <ycp/YCPVoid.h>
#include <ycp/YCPBoolean.h>
#include <ycp/YCPInteger.h>
#include <ycp/YCPSymbol.h>
#include <ycp/YCPString.h>
#include <ycp/YCPList.h>
#include <ycp/YCPMap.h>

using std::string;

///////////////////////////////////////////////////////////////////
//
//	CLASS NAME : SelQueryResult
/**
 *
 **/
struct SelQueryResult {
  enum Instance {
    NONE = 0x00,
    INST = 0x01,
    CAND = 0x02,
    BOTH = (INST|CAND)
  };

  PMSelectablePtr _sel;
  Instance        _match;

  bool setMatchBit( Instance bit_r, const bool & val_r ) {
    if ( val_r ) {
      _match = Instance( _match | bit_r );
    } else {
      _match = Instance( _match & ~bit_r );
    }
    return val_r;
  }

  SelQueryResult( PMSelectablePtr sel_r = 0 )
    : _sel( sel_r )
    , _match( NONE )
  {}

  bool setInstMatch( const bool & val_r ) {
    return setMatchBit( INST, val_r );
  }
  bool setCandMatch( const bool & val_r ) {
    return setMatchBit( CAND, val_r );
  }

  Instance onSystem() const {
    if ( _sel && _sel->is_onSystem() ) {
      return( _sel->to_install() ? CAND : INST );
    }
    return NONE;
  }
};

inline YCPSymbol asYCPSymbol( const SelQueryResult::Instance & obj_r ) {
  switch ( obj_r ) {
  case SelQueryResult::NONE: return YCPSymbol( "NONE" );
  case SelQueryResult::INST: return YCPSymbol( "INST" );
  case SelQueryResult::CAND: return YCPSymbol( "CAND" );
  case SelQueryResult::BOTH: return YCPSymbol( "BOTH" );
  };
  return YCPSymbol( "NONE" );
};

inline YCPList asYCPList( const SelQueryResult & obj_r ) {
  YCPList ret;
  ret->add( YCPString( obj_r._sel ? obj_r._sel->name()->c_str() : "" ) );
  ret->add( asYCPSymbol( obj_r._match ) );
  ret->add( asYCPSymbol( obj_r.onSystem() ) );
  return ret;
}

inline YCPList asYCPList( const list<SelQueryResult> & obj_r ) {
  YCPList ret;
  for ( list<SelQueryResult>::const_iterator it = obj_r.begin(); it != obj_r.end(); ++it ) {
    ret->add( asYCPList( *it ) );
  }
  return ret;
}

///////////////////////////////////////////////////////////////////

typedef SelQueryResult (*queryStringFnc)( PMSelectablePtr sel_r, const string & tag_r );

inline list<SelQueryResult> queryString( queryStringFnc query_r, const string & tag_r ) {
  list<SelQueryResult> ret;
  for ( PMPackageManager::PMSelectableVec::const_iterator sel = Y2PM::packageManager().begin();
	sel != Y2PM::packageManager().end(); ++sel ) {
    SelQueryResult res = query_r( *sel, tag_r );
    if ( res._match ) {
      ret.push_back( res );
    }
  }
  return ret;
}

///////////////////////////////////////////////////////////////////

/**
 * Test whether a certain tag is provided by the selectables
 * installed or candidate object, and return the query result.
 **/
static SelQueryResult queryProvides( PMSelectablePtr sel_r, const string & tag_r ) {
  SelQueryResult ret( sel_r );
  if ( sel_r ) {
    PkgName tag( tag_r );
    if ( sel_r->name() == tag_r ) {
      ret._match = SelQueryResult::BOTH;
    } else {
      PkgRelation rel( tag );
      ret.setInstMatch( sel_r->has_installed()
  		        && sel_r->installedObj()->doesProvide( rel ) );
      ret.setCandMatch( sel_r->has_candidate()
  		        && sel_r->candidateObj()->doesProvide( rel ) );
    }
  }
  return ret;
}

///////////////////////////////////////////////////////////////////

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
  return asYCPList( queryString( queryProvides, tag->value() ) );
}

///////////////////////////////////////////////////////////////////

/******************************************************************
**
**
**	FUNCTION NAME : join
**	FUNCTION TYPE : string
*/
inline string join( const list<string> & lines_r, const string & sep_r = "\n" )
{
  string ret;
  list<string>::const_iterator it = lines_r.begin();

  if ( it != lines_r.end() ) {
    ret = *it;
    for( ++it; it != lines_r.end(); ++it ) {
      ret += sep_r + *it;
    }
  }

  return ret;
}

/******************************************************************
**
**
**	FUNCTION NAME : lookupSelectable
**	FUNCTION TYPE : PMSelectablePtr
*/
inline PMSelectablePtr lookupSelectable( const string & name_r )
{
  return Y2PM::packageManager()[name_r];
}
inline PMSelectablePtr lookupSelectable( const YCPString & name_r )
{
  return lookupSelectable( name_r->value() );
}

/******************************************************************
**
**
**	FUNCTION NAME : lookupInstalledPackage
**	FUNCTION TYPE : PMPackagePtr
*/
inline PMPackagePtr lookupInstalledPackage( const string & name_r )
{
  PMSelectablePtr sel( lookupSelectable( name_r ) );
  if ( sel ) {
    return sel->installedObj();
  }
  return 0;
}
inline PMPackagePtr lookupInstalledPackage( const YCPString & name_r )
{
  return lookupInstalledPackage( name_r->value() );
}

/******************************************************************
**
**
**	FUNCTION NAME : lookupCandidatePackage
**	FUNCTION TYPE : PMPackagePtr
*/
inline PMPackagePtr lookupCandidatePackage( const string & name_r )
{
  PMSelectablePtr sel( lookupSelectable( name_r ) );
  if ( sel ) {
    return sel->candidateObj();
  }
  return 0;
}
inline PMPackagePtr lookupCandidatePackage( const YCPString & name_r )
{
  return lookupCandidatePackage( name_r->value() );
}

/******************************************************************
**
**
**	FUNCTION NAME : lookupPackage
**	FUNCTION TYPE : PMPackagePtr
*/
inline PMPackagePtr lookupPackage( const string & name_r )
{
  PMSelectablePtr sel( lookupSelectable( name_r ) );
  if ( sel ) {
    return sel->theObject();
  }
  return 0;
}
inline PMPackagePtr lookupPackage( const YCPString & name_r )
{
  return lookupPackage( name_r->value() );
}

/******************************************************************
**
**
**	FUNCTION NAME : lookupPackage
**	FUNCTION TYPE : PMPackagePtr
*/
inline PMPackagePtr lookupPackage( const YCPString & name_r, const YCPSymbol & which_r )
{
  string which = which_r->symbol();
  if (        which == "installed" ) {
    return lookupInstalledPackage( name_r );
  } else if ( which == "candidate" ) {
    return lookupCandidatePackage( name_r );
  } else if ( which == "any" ) {
    return lookupPackage( name_r );
  } else {
    y2error( "Unknown YCPSymbol '%s'", which.c_str() );
  }
  return 0;
}

/**
 * helper function, get selectable by name
 */

PMSelectablePtr
PkgModuleFunctions::getPackageSelectable (const std::string& name)
{
    PMSelectablePtr selectable;
    if (!name.empty())
        selectable = _y2pm.packageManager().getItem(name);
    return selectable;
}


/**
 * helper function, get the current object of a selectable
 */

static PMPackagePtr
getTheObject (PMSelectablePtr selectable)
{
    PMPackagePtr package;
    if (selectable)
    {
        package = selectable->theObject();
        if (!package)
        {
            y2error ("Package '%s' not found", selectable->name()->c_str());
        }
    }
    return package;
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
    // get sources in installation order
    InstSrcManager::ISrcIdList inst_order( _y2pm.instSrcManager().instOrderSources() );

    YCPList ycpnames;

    for (InstSrcManager::ISrcIdList::const_iterator it = inst_order.begin(); it != inst_order.end(); ++it)
    {
	ycpnames->add (YCPString ((*it)->descr()->content_product().asPkgNameEd().name));
    }
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

    // now convert back to list of lists in installation order

    YCPList ycpsizes;

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

    // now convert back to list of lists in installation order

    YCPList ycpsizes;

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

    // check package name first, then tag, then file
    if (!_y2pm.instTarget().hasPackage (PkgName (name)))		// package
    {
	if (!_y2pm.instTarget().hasProvides (name))			// provided tag
	{
	    return YCPBoolean (_y2pm.instTarget().hasFile (name));	// file name
	}
    }
    return YCPBoolean (true);
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

    PMSelectablePtr selectable = getPackageSelectable (name);
    if (!selectable)
    {
	selectable = PkgModuleFunctions::WhoProvidesString (name);
	if (!selectable)
	    return YCPBoolean (false);
    }
    if (selectable->to_install())
    {
	return YCPBoolean (true);
    }
    return YCPBoolean (false);
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
    std::string name = tag->value ();
    if (name.empty())
	return YCPBoolean (false);

    PMSelectablePtr selectable = getPackageSelectable (name);
    if (!selectable)
    {
        selectable = PkgModuleFunctions::WhoProvidesString (name);
        if (!selectable)
            return YCPBoolean (false);
    }
    if (selectable->candidateObj() == 0)
        return YCPBoolean (false);
    return YCPBoolean (true);
}


/**
 * helper function, find a package which provides tag (as a
 *   provided tag or a provided file)
 *
 */

PMSelectablePtr
PkgModuleFunctions::WhoProvidesString (std::string tag)
{
    for (PMPackageManager::PMSelectableVec::const_iterator sel = _y2pm.packageManager().begin();
	 sel != _y2pm.packageManager().end(); ++sel)
    {
	if ((*sel)->has_installed()
	    && (*sel)->installedObj()->doesProvide (PkgRelation (PkgName (tag))))
	{
	    return *sel;
	}
	else if ((*sel)->has_candidate()
	    && (*sel)->candidateObj()->doesProvide (PkgRelation (PkgName (tag))))
	{
	    return *sel;
	}
    }
    return 0;
}


/**
 * helper function, install a package which provides tag (as a
 *   package name, a provided tag, or a provided file
 *
 */

bool
PkgModuleFunctions::DoProvideString (std::string name)
{
    PMSelectablePtr selectable = getPackageSelectable (name);		// by name
    if (!selectable)
    {
	selectable = WhoProvidesString (name);				// by tag
	if (!selectable)
	{
	    return false;						// no package found
	}
    }
    selectable->appl_set_install();
    return true;
}

// ------------------------
/**
   @builtin DoProvide
   @short Install a list of tags to the system
   @description
   Provides (read: installs) a list of tags to the system

   tag can be a package name, a string from requires/provides
   or a file name (since a package implictly provides all its files)

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
                DoProvideString (tags->value(i)->asString()->value());
            }
            else
            {
                y2error ("Pkg::DoProvide not string '%s'", tags->value(i)->toString().c_str());
            }
        }
    }
    return ret;
}

// internal
bool
PkgModuleFunctions::DoRemoveString (std::string name)
{
    PMSelectablePtr selectable = getPackageSelectable(name);
    if (!selectable)
    {
	y2error ("Remove: package '%s' not found", name.c_str());
	return false;
    }
    selectable->appl_set_delete();
    return true;
}

// ------------------------
/**
   @builtin DoRemove

   @short Removes a list of tags from the system
   @description
   tag can be a package name, a string from requires/provides
   or a file name (since a package implictly provides all its files)

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
    _y2pm.packageManager ();

    YCPMap ret;
    if (tags->size() > 0)
    {
	for (int i = 0; i < tags->size(); ++i)
	{
	    if (tags->value(i)->isString())
	    {
		DoRemoveString (tags->value(i)->asString()->value());
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
    PMPackagePtr package = getTheObject (getPackageSelectable (p->value ()));
    if (!package)
    {
        return YCPVoid();
    }

    return YCPString (package->summary());

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
    PMPackagePtr package = getTheObject (getPackageSelectable (p->value ()));
    if (!package)
    {
        return YCPVoid();
    }

    return YCPString (package->edition().asString());

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
    PMPackagePtr package = getTheObject (getPackageSelectable (p->value ()));
    if (!package)
    {
	return YCPVoid();
    }

    return YCPInteger ((long long)(package->size()));
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
    PMPackagePtr package = getTheObject (getPackageSelectable (p->value ()));
    if (!package)
    {
	return YCPVoid();
    }

    return YCPString (package->group());
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
    PMPackagePtr package = getTheObject (getPackageSelectable (p->value()));
    if (!package)
    {
	return YCPVoid();
    }

    constInstSrcPtr source( package->source() );

    YCPMap data;
    data->add( YCPString("srcid"), YCPInteger(source->srcID() ));
    data->add( YCPString("location"), YCPString (package->location() ));
    data->add( YCPString("medianr"), YCPInteger (package->medianr() ));
    data->add( YCPString("arch"), YCPString (package->arch() ));

    return data;
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
    PMPackagePtr package = getTheObject (getPackageSelectable (p->value()));
    if (!package)
    {
       return YCPVoid();
    }

    return YCPString (package->location());
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
  constPMPackagePtr pkg = lookupPackage( package, which );
  if ( pkg ) {
    return asYCPList( pkg->filenames() );
  }
  return YCPList();
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
    _y2pm.packageSelectionSaveState();
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
    if (!ch.isNull () && ch->value () == true)
    {
	return YCPBoolean (_y2pm.packageSelectionDiffState());
    }
    return YCPBoolean (_y2pm.packageSelectionRestoreState());
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
    _y2pm.packageSelectionClearSaveState();
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
    return YCPBoolean (_y2pm.packageManager().anythingByUser()
			|| _y2pm.selectionManager().anythingByUser());
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
    return YCPBoolean (_y2pm.packageManager().anythingToDelete ());
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
    return YCPBoolean (_y2pm.packageManager().anythingToInstall ());
}

// ------------------------

/* helper functions */
static void
pgk2list (YCPList &list, const PMObjectPtr& package, bool names_only)
{
    if (names_only)
    {
	list->add (YCPString (package->name()));
    }
    else
    {
	string fullname = package->name();
	fullname += (" " + package->version());
	fullname += (" " + package->release());
	fullname += (" " + package->arch().asString());
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
    PMManager::PMSelectableVec::const_iterator it = _y2pm.packageManager().begin();

    while ( it != _y2pm.packageManager().end() )
    {
        PMSelectablePtr selectable = *it;

        if ( selectable->to_modify() )
        {
            if ( selectable->by_auto() && byAuto ||
                 selectable->by_appl() && byApp  ||
                 selectable->by_user() && byUser   )
            {
				pgk2list (packages, selectable->theObject(), names_only);
            }
        }

        ++it;
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

    for (PMManager::PMSelectableVec::const_iterator it = _y2pm.packageManager().begin();
	 it != _y2pm.packageManager().end();
	 ++it)
    {
	PMObjectPtr package;
	if (which == "installed")
	{
	    package = (*it)->installedObj();
	}
	else if (which == "selected")
	{
	    if (!((*it)->to_install()))
	    {
		continue;
	    }
	    package = (*it)->candidateObj();
	}
	else if (which == "available")
	{
	    for (PMSelectable::PMObjectList::const_iterator ait = (*it)->av_begin();
		 ait != (*it)->av_end(); ++ait)
	    {
		pgk2list (packages, (*ait), names_only);
	    }
	    continue;
	}
	else
	{
	    return YCPError ("Wrong parameter for Pkg::GetPackages");
	}

	if (package)
	    pgk2list (packages, package, names_only);
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
 * @param boolean delete_unmaintained
 * @return list<integer>
 *
 */

YCPValue
PkgModuleFunctions::PkgUpdateAll (const YCPBoolean& del)
{
    PMUpdateStats stats;
    stats.delete_unmaintained = del->value();
    _y2pm.packageManager().doUpdate (stats);

    YCPList ret;
    ret->add (YCPInteger ((long long)stats.totalToInstall()));
    ret->add (YCPInteger ((long long)_y2pm.packageManager().updateSize()));

#if 0
    YCPMap data;
    // formerly 2nd list arg
    data->add( YCPSymbol("ProblemListSze"),
	       YCPInteger( _y2pm.packageManager().updateSize() ) );

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
    data->add( YCPSymbol("deleteDropped"),YCPInteger( stats.delete_unmaintained ) );

    // Total mumber of installed packages processed
    data->add( YCPSymbol("SumProcessed"), YCPInteger( stats.chk_installed_total ) );
#endif

    return ret;
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
    PMSelectablePtr selectable = getPackageSelectable (p->value ());

    if (!selectable)
    {
	return YCPBoolean (false);
    }
    return YCPBoolean (selectable->user_set_install());
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
    PMSelectablePtr selectable = getPackageSelectable (p->value ());

    if (!selectable)
    {
	return YCPBoolean (false);
    }
    return YCPBoolean (selectable->set_source_install(true));
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
    PMSelectablePtr selectable = getPackageSelectable (p->value ());
    if (!selectable)
    {
	return YCPBoolean (false);
    }
    return YCPBoolean (selectable->user_set_delete());
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
    PMSelectablePtr selectable = getPackageSelectable (p->value ());
    if (!selectable)
    {
	return YCPBoolean (false);
    }
    return YCPBoolean (selectable->user_set_taboo());
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
    PMSelectablePtr selectable = getPackageSelectable (p->value ());
    if (!selectable)
    {
	return YCPBoolean (false);
    }
    selectable->setNothingSelected();
    return YCPBoolean (true);
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
    _y2pm.selectionManager().setNothingSelected();
    _y2pm.packageManager().setNothingSelected();

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
    bool filter_conflicts_with_installed = false;

    if (! filter.isNull ())
    {
	filter_conflicts_with_installed = filter->value();

    }

    PkgDep::ResultList good;
    PkgDep::ErrorResultList bad;

    if (!_y2pm.packageManager().solveInstall(good, bad, filter_conflicts_with_installed))
    {
	_solve_errors = bad.size();
	y2error ("Solve: %zd packages failed (see /var/log/YaST2/badlist)", bad.size());

	std::ofstream out ("/var/log/YaST2/badlist");
	out << bad.size() << " packages failed" << std::endl;
	for( PkgDep::ErrorResultList::const_iterator p = bad.begin();
	     p != bad.end(); ++p )
	{
	    out << *p << std::endl;
	}

	return YCPBoolean (false);
    }
    return YCPBoolean (true);
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
    return YCPInteger (_solve_errors);
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

    std::list<std::string> errors;
    std::list<std::string> remaining;
    std::list<std::string> srcremaining;
    int count = _y2pm.commitPackages (medianr, errors, remaining, srcremaining );

    YCPList ret;
    ret->add (YCPInteger (count));

    YCPList errlist;
    for (std::list<std::string>::const_iterator it = errors.begin(); it != errors.end(); ++it)
    {
	errlist->add (YCPString (*it));
    }
    ret->add (errlist);
    YCPList remlist;
    for (std::list<std::string>::const_iterator it = remaining.begin(); it != remaining.end(); ++it)
    {
	remlist->add (YCPString (*it));
    }
    ret->add (remlist);
    YCPList srclist;
    for (std::list<std::string>::const_iterator it = srcremaining.begin(); it != srcremaining.end(); ++it)
    {
	srclist->add (YCPString (*it));
    }
    ret->add (srclist);
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
    return YCPString (_y2pm.instTarget().getBackupPath().asString());
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
    Pathname path (p->value());
    _y2pm.instTarget().setBackupPath (path);
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
    _y2pm.instTarget ().createPackageBackups (flag->value ());
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
  PMSelectablePtr selectable = getPackageSelectable( package->value() );

  if ( ! ( selectable && selectable->candidateObj() ) ) {
    // unknown or no candidate package
    return YCPString("");
  }

  if (! PMPackagePtr(selectable->candidateObj())->hasLicenseToConfirm ()) {
    // license was confirmed before
    return YCPString ("");
  }

  string text( join( PMPackagePtr(selectable->candidateObj())->licenseToConfirm() ) );
  return YCPString( text );
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
    PMSelectablePtr selectable = getPackageSelectable( packages->value(i)->asString()->value() );

    if ( ! ( selectable && selectable->candidateObj() ) ) {
      // unknown or no candidate package
      continue;
    }

    string text( join( PMPackagePtr(selectable->candidateObj())->licenseToConfirm() ) );
    if ( text.length() && PMPackagePtr(selectable->candidateObj())->hasLicenseToConfirm ()) {
      ret->add( packages->value(i), YCPString( text ) );
    }
  }

  return ret;
}

YCPBoolean PkgModuleFunctions::PkgMarkLicenseConfirmed (const YCPString & package)
{
  PMSelectablePtr selectable = getPackageSelectable( package->value() );

  if ( ! ( selectable && selectable->candidateObj() ) ) {
    // unknown or no candidate package
    return YCPBoolean(false);
  }

  PMPackagePtr(selectable->candidateObj())->markLicenseConfirmed ();
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
  return YCPBoolean( RpmHeader::readPackage( filename->value(), RpmHeader::VERIFY ) );
}
