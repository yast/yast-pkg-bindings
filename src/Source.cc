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

   File:	PkgModuleFunctionsSource.cc

   Author:	Klaus Kaempf <kkaempf@suse.de>
   Maintainer:  Klaus Kaempf <kkaempf@suse.de>

   Summary:     Access to Installation Sources
   Namespace:   Pkg
   Purpose:	Access to InstSrc
		Handles source related Pkg::function (list_of_arguments) calls
		from WFMInterpreter.
/-*/
//#include <unistd.h>
//#include <sys/statvfs.h>

#include <iostream>
#include <ycp/y2log.h>

#include <ycpTools.h>
#include <PkgModule.h>
#include <PkgModuleFunctions.h>

#include <zypp/SourceManager.h>
#include <zypp/SourceFactory.h>
#include <zypp/Source.h>
#include <zypp/Product.h>

#include <stdio.h> // snprintf

/****************************************************************************************
 * @builtin SourceSetRamCache
 * @short Allow/prevent InstSrces from caching package metadata on ramdisk
 * @description
 * In InstSys: Allow/prevent InstSrces from caching package metadata on ramdisk.
 * If no cache is used the media cannot be unmounted, i.e. no CD change possible.
 *
 * @param boolean allow  If true, allow caching
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceSetRamCache (const YCPBoolean& a)
{
    y2warning( "Pkg::SourceSetRamCache is obsolete and does nothing");
    return YCPBoolean( true );
}

/****************************************************************************************
 * @builtin SourceStartManager
 *
 * @short Make sure the InstSrcManager is up and knows all available InstSrces.
 * @description
 * Make sure the InstSrcManager is up and knows all available InstSrces.
 * Depending on the value of autoEnable, InstSources may be enabled on the
 * fly. It's safe to call this multiple times, and once the InstSources are
 * actually enabled, it's even cheap (enabling an InstSrc is expensive).
 *
 * @param boolean autoEnable If true, all InstSrces are enabled according to their default.
 * If false, InstSrces will be created in disabled state, and remain unchanged if
 * the InstSrcManager is already up.
 *
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceStartManager (const YCPBoolean& enable)
{
    bool success = true;

    try {
	// we always use the configured caches
	if( !zypp::SourceManager::sourceManager()->restore(_target_root, true) )
	{
	    y2error( "Unable to restore all sources" );
	    
	    success = false;
	}
	
	if( enable->value() )
	{
	    // go over all sources and get resolvables
	    std::list<zypp::SourceManager::SourceId> ids = zypp::SourceManager::sourceManager()->enabledSources();
	    for( std::list<zypp::SourceManager::SourceId>::iterator it = ids.begin(); it != ids.end(); ++it)
	    {
		zypp::Source_Ref src = zypp::SourceManager::sourceManager()->findSource(*it);
		if( src.enabled() )
			zypp_ptr->addResolvables (src.resolvables());
	    }
	}
    }
    catch (const zypp::FailedSourcesRestoreException& excpt)
    {
	// FIXME: assuming the sources are already initialized
	y2error ("Error in SourceStartManager: %s", excpt.asString().c_str());
	_last_error.setLastError(excpt.asUserString());
	success = false;
    }
    catch (const zypp::Exception& excpt)
    {
	// FIXME: assuming the sources are already initialized
	y2error ("Error in SourceStartManager: %s", excpt.asString().c_str());
	_last_error.setLastError(excpt.asUserString());
	success = false;
    }

    return YCPBoolean( success );
}

/****************************************************************************************
 * @builtin SourceStartCache
 *
 * @short Make sure the InstSrcManager is up, and return the list of SrcIds.
 * @description
 * Make sure the InstSrcManager is up, and return the list of SrcIds.
 * In fact nothing more than:
 *
 * <code>
 *   SourceStartManager( enabled_only );
 *   return SourceGetCurrent( enabled_only );
 * </code>
 *
 * @param boolean enabled_only If true, make sure all InstSrces are enabled according to
 * their default, and return the Ids of enabled InstSrces only. If false, return
 * the Ids of all known InstSrces.
 *
 * @return list<integer> list of SrcIds
 **/
YCPValue
PkgModuleFunctions::SourceStartCache (const YCPBoolean& enabled)
{
    SourceStartManager(enabled);
    
    return SourceGetCurrent(enabled);
}

/****************************************************************************************
 * @builtin SourceGetCurrent
 *
 * @short Return the list of all InstSrc Ids.
 *
 * @param boolean enabled_only If true, or omitted, return the Ids of all enabled InstSrces.
 * If false, return the Ids of all known InstSrces.
 *
 * @return list<integer> list of SrcIds (integer)
 **/
YCPValue
PkgModuleFunctions::SourceGetCurrent (const YCPBoolean& enabled)
{
    std::list<zypp::SourceManager::SourceId> ids = (enabled->value()) ?
	zypp::SourceManager::sourceManager()->enabledSources()
	: zypp::SourceManager::sourceManager()->allSources();
    
    YCPList res;
    
    for( std::list<zypp::SourceManager::SourceId>::const_iterator it = ids.begin(); it != ids.end() ; ++it )
    {
	res->add( YCPInteger( *it ) );
    }

    return res;
}

/****************************************************************************************
 * @builtin SourceReleaseAll
 *
 * @short Release all medias hold by all sources
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceReleaseAll ()
{
    try
    {
	y2milestone( "Relesing all sources") ;
	zypp::SourceManager::sourceManager()->releaseAllSources ();
    }
    catch (zypp::Exception & excpt)
    {
	_last_error.setLastError(excpt.asUserString());
	y2error("Pkg::SourceReleaseAll has failed: %s", excpt.msg().c_str() );
	return YCPBoolean(false);
    }

    y2milestone( "All sources released");

    return YCPBoolean(true);
}

/****************************************************************************************
 * @builtin SourceFinishAll
 *
 * @short Disable all InstSrces.
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceFinishAll ()
{
    try
    {
	// look if there are any enabled sources
	std::list<zypp::SourceManager::SourceId> enabled_sources = zypp::SourceManager::sourceManager()->enabledSources();
	if (enabled_sources.empty()) {
	    y2milestone( "No enabled sources." );
	    return YCPBoolean( true );
	}
	y2milestone( "Storing the source setup in %s", _target_root.asString().c_str()) ;
	zypp::SourceManager::sourceManager()->store( _target_root, true );
	y2milestone( "Disabling all sources") ;
	zypp::SourceManager::sourceManager()->disableAllSources ();
    }
    catch (zypp::Exception & excpt)
    {
	y2error("Pkg::SourceFinishAll has failed: %s", excpt.msg().c_str() );
	_last_error.setLastError(excpt.asUserString());
	return YCPBoolean(false);
    }

    y2milestone( "All sources finished");

    return YCPBoolean(true);
}

/////////////////////////////////////////////////////////////////////////////////////////
// Query individual sources
/////////////////////////////////////////////////////////////////////////////////////////

/****************************************************************************************
 * @builtin SourceGeneralData
 *
 * @short Get general data about the source
 * @description
 * Return general data about the source as a map:
 *
 * <code>
 * $[
 * "enabled"	: YCPBoolean,
 * "autorefresh": YCPBoolean,
 * "product_dir": YCPString,
 * "type"	: YCPString,
 * "url"	: YCPString
 * ];
 *
 * </code>
 * @param integer SrcId Specifies the InstSrc to query.
 * @return map
 **/
YCPValue
PkgModuleFunctions::SourceGeneralData (const YCPInteger& id)
{
    YCPMap data;
    zypp::Source_Ref src;

    try
    {
	src = zypp::SourceManager::sourceManager()->findSource(id->value());
    }
    catch (const zypp::Exception& excpt)
    {
	y2error("Cannot find source '%lld': %s"
		,id->value(), excpt.msg().c_str() );
	_last_error.setLastError(excpt.asUserString());
	return YCPVoid ();
    }

    data->add( YCPString("enabled"),		YCPBoolean(src.enabled()));
    data->add( YCPString("autorefresh"),	YCPBoolean(src.autorefresh()));
    data->add( YCPString("type"),		YCPString(src.type()));
    data->add( YCPString("product_dir"),	YCPString(src.path().asString()));

#warning SourceGeneralData returns URL without password
    // if password is required then use this parameter:
    // asString(url::ViewOptions() + url::ViewOptions::WITH_PASSWORD);
    data->add( YCPString("url"),		YCPString(src.url().asString()));
    return data;
}

/****************************************************************************************
 * @builtin SourceMediaData
 * @short Return media data about the source
 * @description
 * Return media data about the source as a map:
 *
 * <code>
 * $["media_count": YCPInteger,
 * "media_id"	  : YCPString,
 * "media_vendor" : YCPString,
 * "url"	  : YCPString,
 * ];
 * </code>
 *
 * @param integer SrcId Specifies the InstSrc to query.
 * @return map
 **/
YCPValue
PkgModuleFunctions::SourceMediaData (const YCPInteger& id)
{
    YCPMap data;
    zypp::Source_Ref src;

    try
    {
	src = zypp::SourceManager::sourceManager()->findSource(id->value());
    }
    catch (const zypp::Exception& excpt)
    {
	y2error ("Source ID %lld not found: %s", id->asInteger()->value(), excpt.msg().c_str());
	_last_error.setLastError(excpt.asUserString());
        return YCPVoid();
    }

  data->add( YCPString("media_count"),	YCPInteger(src.numberOfMedia()));
  data->add( YCPString("media_id"),	YCPString(src.unique_id()));
  data->add( YCPString("media_vendor"),	YCPString(src.vendor()));

#warning SourceMediaData returns URL without password
  data->add( YCPString("url"),		YCPString(src.url().asString()));
  return data;
}

/****************************************************************************************
 * @builtin SourceProductData
 * @short Return Product data about the source
 * @param integer SrcId Specifies the InstSrc to query.
 * @description
 * Product data about the source as a map:
 *
 * <code>
 * $[
 * "productname"	: YCPString,
 * "productversion"	: YCPString,
 * "baseproductname"	: YCPString,
 * "baseproductversion"	: YCPString,
 * "vendor"		: YCPString,
 * "defaultbase"	: YCPString,
 * "architectures"	: YCPList(YCPString),
 * "requires"		: YCPString,
 * "linguas"		: YCPList(YCPString),
 * "label"		: YCPString,
 * "labelmap"		: YCPMap(YCPString lang,YCPString label),
 * "language"		: YCPString,
 * "timezone"		: YCPString,
 * "descrdir"		: YCPString,
 * "datadir"		: YCPString
 * ];
 * </code>
 *
 * @return map
 **/
YCPValue
PkgModuleFunctions::SourceProductData (const YCPInteger& id)
{
  zypp::Source_Ref src;

  try {
	src = zypp::SourceManager::sourceManager()->findSource(id->asInteger()->value());
  }
  catch (const zypp::Exception& excpt)
  {
	y2error ("Source ID not found: %lld", id->asInteger()->value());
	_last_error.setLastError(excpt.asUserString());
	return YCPVoid();
  }

#warning product category handling???

  zypp::ResStore products (src.resolvables(zypp::ResTraits<zypp::Product>::kind));
  
  if( products.empty() )
  {
	y2error ("Product for source '%lld' not found", id->asInteger()->value());
	return YCPVoid();
  }
  
  zypp::Product::constPtr product = boost::dynamic_pointer_cast<const zypp::Product>( *(products.begin()) );
  
  y2debug ("Found");

  YCPMap data;

  data->add( YCPString("label"),		YCPString( product->displayName() ) );
  data->add( YCPString("vendor"),		YCPString( product->vendor() ) );
  data->add( YCPString("productname"),		YCPString( product->name() ) );
  data->add( YCPString("productversion"),	YCPString( product->edition().version() ) );
  data->add( YCPString("relnotesurl"), 		YCPString( product->releaseNotesUrl().asString()));
  
#warning SourceProductData not finished 
/*  
  data->add( YCPString("datadir"),		YCPString( descr->content_datadir().asString() ) );
*/
  return data;
}

/****************************************************************************************
 * @builtin SourceProduct
 * @short Get Product info
 * @param integer SrcId Specifies the InstSrc to query.
 *
 * <code>
 * $[
 *   "baseproduct":"",
 *   "baseversion":"", 
 *   "defaultbase":"i386", 
 *   "distproduct":"SuSE-Linux-DVD", 
 *   "distversion":"10.0", 
 *   "flags":"update", 
 *   "name":"SUSE LINUX", 
 *   "product":"SUSE LINUX 10.0", 
 *   "relnotesurl":"http://www.suse.com/relnotes/i386/SUSE-LINUX/10.0/release-notes.rpm",
 *   "requires":"suse-release-10.0",
 *   "vendor":"SUSE LINUX Products GmbH, Nuernberg, Germany", 
 *   "version":"10.0"
 * ] 
 * </code>
 * @return map Product info as a map
 **/
YCPValue
PkgModuleFunctions::SourceProduct (const YCPInteger& id)
{
    /* TODO FIXME */
  return YCPMap();
}

/****************************************************************************************
 * @builtin SourceProvideFile
 *
 * @short Make a file available at the local filesystem
 * @description
 * Let an InstSrc provide some file (make it available at the local filesystem).
 *
 * @param integer SrcId	Specifies the InstSrc .
 * @param integer medianr Number of the media the file is located on ('1' for the 1st media).
 * @param string file Filename relative to the media root.
 *
 * @return string local path as string
 **/
YCPValue
PkgModuleFunctions::SourceProvideFile (const YCPInteger& id, const YCPInteger& mid, const YCPString& f)
{
    zypp::Source_Ref src;

    try {
	src = zypp::SourceManager::sourceManager()->findSource(id->asInteger()->value());
    }
    catch (const zypp::Exception& excpt)
    {
	_last_error.setLastError(excpt.asUserString());
	y2error ("Source ID not found: %lld", id->asInteger()->value());
	return YCPVoid();
    }

    zypp::filesystem::Pathname path;
  
    try {
    	path = src.provideFile(f->value(), mid->asInteger()->value());
    }
    catch (const zypp::Exception& excpt)
    {
	_last_error.setLastError(excpt.asUserString());
	y2milestone ("File not found: %s", f->value_cstr());
	return YCPVoid();
    }

    return YCPString(path.asString());
}

/****************************************************************************************
 * @builtin SourceProvideDir
 * @short make a directory available at the local filesystem
 * @description
 * Let an InstSrc provide some directory (make it available at the local filesystem) and
 * all the files within it (non recursive).
 *
 * @param integer SrcId	Specifies the InstSrc .
 * @param integer medianr Number of the media the file is located on ('1' for the 1st media).
 * @param string dir Directoryname relative to the media root.
 * @return string local path as string
 */
YCPValue
PkgModuleFunctions::SourceProvideDir (const YCPInteger& id, const YCPInteger& mid, const YCPString& d)
{
    zypp::Source_Ref src;

    try
    {
	src = zypp::SourceManager::sourceManager()->findSource(id->value());
    }
    catch (const zypp::Exception& excpt)
    {
	_last_error.setLastError(excpt.asUserString());
        y2error ("Source ID %lld not found: %s", id->asInteger()->value(), excpt.msg().c_str());
	return YCPVoid();
    }

    zypp::filesystem::Pathname path;

    try
    {
	path = src.provideDir(d->value(), mid->asInteger()->value());
    }
    catch (const zypp::Exception& excpt)
    {
	_last_error.setLastError(excpt.asUserString());
	y2milestone ("Directory not found: %s", d->value_cstr());
	return YCPVoid();
    }

    return YCPString(path.asString());
}

/****************************************************************************************
 * @builtin SourceChangeUrl
 * @short Change Source URL
 * @description
 * Change url of an InstSrc. Used primarely when re-starting during installation
 * and a cd-device changed from hdX to srX since ide-scsi was activated.
 * @param integer SrcId Specifies the InstSrc.
 * @param string url The new url to use.
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceChangeUrl (const YCPInteger& id, const YCPString& u)
{

#warning SourceChangeUrl is not implemented
    /* TODO FIXME
  YCPList args;
  args->add (id);
  args->add (u);

  //-------------------------------------------------------------------------------------//
  YcpArgLoad decl(__FUNCTION__);

  InstSrcManager::ISrcId & source_id( decl.arg<YT_INTEGER, InstSrcManager::ISrcId>() );
  Url &                    url      ( decl.arg<YT_STRING, Url>() );

  if ( ! decl.load( args ) ) {
    return pkgError_bad_args;
  }
  //-------------------------------------------------------------------------------------//

  if ( ! source_id )
    return pkgError( InstSrcError::E_bad_id );

  PMError err = _y2pm.instSrcManager().rewriteUrl( source_id, url );

  if ( err )
    return pkgError( err );
*/
  return YCPBoolean( true );
}

/****************************************************************************************
 * @builtin SourceInstallOrder
 *
 * @short Explicitly set an install order.
 * @param map order_map A map of 'int order : int source_id'. source_ids are expected to
 * denote known and enabled sources.
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceInstallOrder (const YCPMap& ord)
{
    /* TODO FIXME
  YCPList args;
  args->add (ord);

  //-------------------------------------------------------------------------------------//
  YcpArgLoad decl(__FUNCTION__);

  YCPMap & order_map( decl.arg<YT_MAP, YCPMap>() );

  if ( ! decl.load( args ) ) {
    return pkgError_bad_args;
  }
  //-------------------------------------------------------------------------------------//

  InstSrcManager::InstOrder order;
  order.reserve( order_map->size() );
  bool error = false;

  for ( YCPMapIterator it = order_map->begin(); it != order_map->end(); ++it ) {

    if ( it.value()->isInteger() ) {
      InstSrc::UniqueID uId( it.value()->asInteger()->value() );
      InstSrcManager::ISrcId source_id( _y2pm.instSrcManager().getSourceByID( uId ) );
      if ( source_id ) {
	if ( source_id->enabled() ) {
	  order.push_back( uId );  // finaly ;)

	} else {
	  y2error ("order map entry '%s:%s': source not enabled",
		    it.key()->toString().c_str(),
		    it.value()->toString().c_str() );
	  error = true;
	}
      } else {
	y2error ("order map entry '%s:%s': bad source id",
		  it.key()->toString().c_str(),
		  it.value()->toString().c_str() );
	error = true;
      }
    } else {
      y2error ("order map entry '%s:%s': integer value expected",
		it.key()->toString().c_str(),
		it.value()->toString().c_str() );
      error = true;
    }
  }
  if ( error ) {
    return pkgError( Error::E_bad_args );
  }

  // store new instorder
  _y2pm.instSrcManager().setInstOrder( order );
*/

#warning SourceInstallOrder is not implemented
  return YCPBoolean( true );
}

// return integer greater than the greatest source ID
zypp::SourceManager::SourceId max_src_id()
{
    // all sources
    std::list<zypp::SourceManager::SourceId> srcs = zypp::SourceManager::sourceManager()->allSources();

    // the greater ID
    std::list<zypp::SourceManager::SourceId>::const_iterator maxit = max_element(srcs.begin(), srcs.end());

    return (maxit == srcs.end()) ?  zypp::SourceManager::SourceId(0) : (*maxit) + 1;
}

// helper function - convert int to std::string using snprintf
std::string id_to_string(zypp::SourceManager::SourceId input)
{
#define BUFF_SIZE 16
    char buffer[BUFF_SIZE];
    snprintf(buffer, BUFF_SIZE, "%lu", input);
#undef BUFF_SIZE

    return std::string(buffer);
}


/****************************************************************************************
 * @builtin SourceCacheCopyTo
 *
 * @short Copy cache data of all installation sources to the target
 * @description
 * Copy cache data of all installation sources to the target located below 'dir'.
 * To be called at end of initial installation.
 *
 * @param string dir Root directory of target.
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceCacheCopyTo (const YCPString& dir)
{
  y2warning( "Pkg::SourceCacheCopyTo is obsolete now, it does nothing" );

  return YCPBoolean( true );
}

/****************************************************************************************
 * @builtin SourceScan
 * @short Scan a Source Media
 * @description
 * Load all InstSrces found at media_url, i.e. all sources mentioned in /media.1/products.
 * If no /media.1/products is available, InstSrc is expected to be located directly
 * below media_url (product_dir: /).
 *
 * If a product_dir is provided, only the InstSrc located at media_url/product_dir is loaded.
 *
 * In contrary to @ref SourceCreate, InstSrces are loaded into the InstSrcManager,
 * but not enabled (packages and selections are not provided to the PackageManager),
 * and the SrcIds of <b>all</b> InstSrces found are returned.
 *
 * @param string url The media to scan.
 * @optarg string product_dir Restrict scan to a certain InstSrc located in media_url/product_dir.
 *
 * @return list<integer> list of SrcIds (integer).
 **/
YCPValue
PkgModuleFunctions::SourceScan (const YCPString& media, const YCPString& pd)
{
  zypp::SourceFactory factory;
  
  
  zypp::Url url;
  
  try {
    url = zypp::Url(media->value ());
  }
  catch(const zypp::Exception & expt )
  {
    y2error ("Invalid URL: %s", expt.asString().c_str());
    _last_error.setLastError(expt.asUserString());
    return YCPBoolean (false);
  }
  
  zypp::Pathname pn(pd->value ());
  
  YCPList ids;
  unsigned int id;
  
  if ( pd->value().empty() ) {
    // scan all sources
    
    zypp::SourceFactory::ProductSet products;
        
    try
    {
	factory.listProducts( url, products );
    }
    catch ( const zypp::Exception& excpt)
    {
	y2error("Scanning products for '%s' has failed"
		, url.asString().c_str());
	_last_error.setLastError(excpt.asUserString());
	return ids;
    }
    
    for( zypp::SourceFactory::ProductSet::const_iterator it = products.begin();
	it != products.end() ; ++it )
    {
	try
	{
	    // create the source, use URL + ID as the alias 
	    // alias must be unique, add max source number
	    std::string alias = url.asString()+pn.asString()+"-"+id_to_string(max_src_id());
	    id = zypp::SourceManager::sourceManager()->addSource(url, pn, alias);
	    ids->add( YCPInteger(id) );
	    y2milestone("Added source %d: %s (alias %s)", id, (url.asString()+pn.asString()).c_str(), alias.c_str() );  
	}
	catch ( const zypp::Exception& excpt)
	{
	    y2error("SourceCreate for '%s' product '%s' has failed"
		, url.asString().c_str(), pn.asString().c_str());
	    _last_error.setLastError(excpt.asUserString());
	}
    }
  } else {
    y2debug("Creating source...");

    try
    {
	// create the source, use URL + ID as the alias 
	id = zypp::SourceManager::sourceManager()->addSource(url, pn, url.asString()+pn.asString()+"-"+id_to_string(max_src_id()));
	ids->add( YCPInteger(id) );
    }
    catch ( const zypp::Exception& excpt)
    {
	y2error("SourceCreate for '%s' product '%s' has failed"
	    , url.asString().c_str(), pn.asString().c_str());
	_last_error.setLastError(excpt.asUserString());
    }
  }

  y2milestone("Found sources: %s", ids->toString().c_str() );  
  return ids;
}

/****************************************************************************************
 * @builtin SourceCreate
 *
 * @short Create a Source
 * @description
 * Load and enable all InstSrces found at media_url, i.e. all sources mentioned in /media.1/products.
 * If no /media.1/products is available, InstSrc is expected to be located directly below
 * media_url (product_dir: /).
 *
 * If a product_dir is provided, only the InstSrc located at media_url/product_dir is loaded
 * and enabled.
 *
 * @param string url The media to scan.
 * @optarg string product_dir Restrict scan to a certain InstSrc located in media_url/product_dir.
 *
 * @return integer The source_id of the first InstSrc found on the media.
 **/
YCPValue
PkgModuleFunctions::SourceCreate (const YCPString& media, const YCPString& pd)
{
  y2debug("Creating source...");

  zypp::SourceFactory factory;
  zypp::Pathname pn(pd->value ());

  zypp::Url url;
  
  try {
    url = zypp::Url(media->value ());
  }
  catch(const zypp::Exception & expt )
  {
    y2error ("Invalid URL: %s", expt.asString().c_str());
    _last_error.setLastError(expt.asUserString());
    return YCPBoolean (false);
  }
  
  
  YCPList ids;
  int ret = -1;
  
  if ( pd->value().empty() ) {
    // scan all sources
    
    zypp::SourceFactory::ProductSet products;

    try {        
	factory.listProducts( url, products );
    }
    catch ( const zypp::Exception& excpt)
    {
	_last_error.setLastError(excpt.asUserString());
	y2error( "Cannot read the product list from the media" );
	return YCPInteger(ret);
    }
    
    for( zypp::SourceFactory::ProductSet::const_iterator it = products.begin();
	it != products.end() ; ++it )
    {
	try
	{
	    // create the source, use URL + ID as the alias 
	    std::string alias = url.asString()+pn.asString()+"-"+id_to_string(max_src_id());
	    unsigned id = zypp::SourceManager::sourceManager()->addSource(url, pn, alias);

	    zypp::Source_Ref src = zypp::SourceManager::sourceManager()->findSource(id);

	    src.enable(); 
    
    	    zypp_ptr->addResolvables (src.resolvables());

	    // return the id of the first product
	    if ( ret == -1 ) 
		ret = id;

	    y2milestone("Added source %d: %s (alias %s)", id, (url.asString()+pn.asString()).c_str(), alias.c_str() );  
	}
	catch ( const zypp::Exception& excpt)
	{
	    y2error("SourceCreate for '%s' product '%s' has failed"
		, url.asString().c_str(), pn.asString().c_str());
	    _last_error.setLastError(excpt.asUserString());
	}
    }
  } else {
    y2debug("Creating source...");

    try
    {
	std::string alias = url.asString()+pn.asString()+"-"+id_to_string(max_src_id());

	// create the source, use URL + ID as the alias 
	ret = zypp::SourceManager::sourceManager()->addSource(url, pn, alias);

	zypp::Source_Ref src = zypp::SourceManager::sourceManager()->findSource(ret);

	src.enable(); 
    
    	zypp_ptr->addResolvables (src.resolvables());
	y2milestone("Added source %d: %s (alias %s)", ret, (url.asString()+pn.asString()).c_str(), alias.c_str() );  
    }
    catch ( const zypp::Exception& excpt)
    {
	y2error("SourceCreate for '%s' product '%s' has failed"
	    , url.asString().c_str(), pn.asString().c_str());
	_last_error.setLastError(excpt.asUserString());
    }
  }

  return YCPInteger(ret);
}


/****************************************************************************************
 * @builtin SourceSetEnabled
 *
 * @short Set the default activation state of an InsrSrc.
 * @param integer SrcId Specifies the InstSrc.
 * @param boolean enabled Default activation state of source.
 *
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceSetEnabled (const YCPInteger& id, const YCPBoolean& e)
{
    zypp::Source_Ref src;
    bool enabled = e->value();

    try {
	src = zypp::SourceManager::sourceManager()->findSource(id->asInteger()->value());
    }
    catch(const zypp::Exception& excpt)
    {
	_last_error.setLastError(excpt.asUserString());
	y2error ("Source ID not found: %lld", id->asInteger()->value());
	return YCPBoolean(false);
    }

    if (enabled)
    {
	src.enable();
    }
    else
    {
	src.disable();
    }

    return YCPBoolean(src.enabled() == enabled);
}

/****************************************************************************************
 * @builtin SourceSetAutorefresh
 *
 * @short Set whether this source should automaticaly refresh it's
 * meta data when it gets enabled. (default true, if not CD/DVD)
 * @param integer SrcId Specifies the InstSrc.
 * @param boolean enabled Whether autorefresh should be turned on or off.
 *
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceSetAutorefresh (const YCPInteger& id, const YCPBoolean& e)
{
    zypp::Source_Ref src;

    try
    {
	src = zypp::SourceManager::sourceManager()->findSource(id->value());
    }
    catch (const zypp::Exception& excpt)
    {
	y2error ("Source ID %lld not found: %s", id->asInteger()->value(), excpt.msg().c_str());
	_last_error.setLastError(excpt.asUserString());
	return YCPVoid();
    }

    src.setAutorefresh(e->value());

    return YCPBoolean( true );
}

/****************************************************************************************
 * @builtin SourceFinish
 * @short Disable an Installation Source
 * @param integer SrcId Specifies the InstSrc.
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceFinish (const YCPInteger& id)
{
    return SourceSetEnabled(id, false);
}

/****************************************************************************************
 * @builtin SourceRefreshNow
 * @short Attempt to immediately refresh a Source
 * @description
 * The InsrSrc will be encouraged to check and refresh all metadata
 * cached on disk.
 *
 * @param integer SrcId Specifies the InstSrc.
 *
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceRefreshNow (const YCPInteger& id)
{
    zypp::Source_Ref src;

    try {
	src = zypp::SourceManager::sourceManager()->findSource(id->asInteger()->value());
    } catch( const zypp::Exception & expt ) {
	y2error ("Source ID not found: %lld", id->asInteger()->value());
	return YCPBoolean(false);
    }

    try {
	src.refresh();
    } catch ( const zypp::Exception & expt ) {
	y2error ("Error while refreshin the source: %s", expt.asString().c_str());
	return YCPBoolean(false);
    }
    return YCPBoolean( true );
}

/****************************************************************************************
 * @builtin SourceDelete
 * @short Delete a Source
 * @description
 * Delete an InsrSrc. The InsrSrc together with all metadata cached on disk
 * is removed. The SrcId passed becomes invalid (other SrcIds stay valid).
 *
 * @param integer SrcId Specifies the InstSrc.
 *
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceDelete (const YCPInteger& id)
{
    try
    {
	zypp_ptr->removeResolvables(
	    zypp::SourceManager::sourceManager()->
		findSource(id->asInteger()->value()).resolvables());

	zypp::SourceManager::sourceManager()->removeSource(id->asInteger()->value());
    }
    catch (const zypp::Exception& excpt)
    {
	y2error("Pkg::SourceDelete: Cannot remove source %lld", id->value());
	_last_error.setLastError(excpt.asUserString());
	return YCPBoolean(false);
    }

    return YCPBoolean(true);
}

/****************************************************************************************
 * @builtin SourceEditGet
 *
 * @short Get state of Sources
 * @description
 * Return a list of states for all known InstSources sorted according to the
 * source priority (highest first). A source state is a map:
 * $[
 * "SrcId"	: YCPInteger,
 * "enabled"	: YCPBoolean
 * "autorefresh": YCPBoolean
 * ];
 *
 * @return list<map> list of source states (map)
 **/
YCPValue
PkgModuleFunctions::SourceEditGet ()
{
    YCPList ret;
    std::list<zypp::SourceManager::SourceId> ids = zypp::SourceManager::sourceManager()->allSources();
	    
    for( std::list<zypp::SourceManager::SourceId>::iterator it = ids.begin(); it != ids.end(); ++it)
    {
	YCPMap src_map;
	zypp::Source_Ref src;

	try
	{
	    src = zypp::SourceManager::sourceManager()->findSource(*it);
	}
	catch (const zypp::Exception& excpt)
	{
	    // this should never happen
	    y2internal("Source ID %lu not found: %s", *it, excpt.msg().c_str());
	}

	src_map->add(YCPString("SrcId"), YCPInteger(*it));
	src_map->add(YCPString("enabled"), YCPBoolean(src.enabled()));
	src_map->add(YCPString("autorefresh"), YCPBoolean(src.autorefresh()));
	
	ret->add(src_map);
    }

    return ret;
}

/****************************************************************************************
 * @builtin SourceEditSet
 *
 * @short Rearange known InstSrces rank and default state
 * @description
 * Rearange known InstSrces rank and default state according to source_states
 * (highest priority first). Known InstSrces not mentioned in source_states
 * are deleted.
 *
 * @param list source_states List of source states. Same format as returned by
 * @see SourceEditGet.
 *
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceEditSet (const YCPList& states)
{
  bool error = false;
 
  for (int index = 0; index < states->size(); index++ )
  {
    if( ! states->value(index)->isMap() )
    {
	ycperror( "Pkg::SourceEditSet, entry not a map at index %d", index);
	error = true;
	continue;
    }
    
    YCPMap descr = states->value(index)->asMap();
    
    if (descr->value( YCPString("SrcId") ).isNull() || !descr->value(YCPString("SrcId"))->isInteger())
    {
	ycperror( "Pkg::SourceEditSet, SrcId not defined for a source description at index %d", index);
	error = true;
	continue;
    }
    int id = descr->value( YCPString("SrcId") )->asInteger()->value();
    
    zypp::Source_Ref src;
    try {
	src = zypp::SourceManager::sourceManager()->findSource(id);
    }
    catch (const zypp::Exception& excpt)
    {
	ycperror( "Pkg::SourceEditSet, source %d not found", index);
	_last_error.setLastError(excpt.asUserString());
	error = true;
	continue;
    }
    
    // now, we have the source
    if( ! descr->value( YCPString("enabled")).isNull() && descr->value(YCPString("enabled"))->isBoolean ())
    {
	bool enable = descr->value(YCPString("enabled"))->asBoolean ()->value();
	
	if( enable )
	    src.enable();
	else
	    src.disable();
    }

    if( !descr->value(YCPString("autorefresh")).isNull() && descr->value(YCPString("autorefresh"))->isBoolean ())
    {
	src.setAutorefresh( descr->value(YCPString("autorefresh"))->asBoolean ()->value() );
    }

#warning SourceEditSet ordering not implemented yet
  }
  
  return YCPBoolean( true );
}

/////////////////////////////////////////////////////////////////////////////////////////
//
// DEPRECATED
//
/////////////////////////////////////////////////////////////////////////////////////////

/****************************************************************************************
 * Pkg::SourceRaisePriority (integer SrcId) -> bool
 *
 * Raise priority of source.
 *
 * @param SrcId Specifies the InstSrc.
 *
 * @return bool
 **/
YCPValue
PkgModuleFunctions::SourceRaisePriority (const YCPInteger& id)
{
    zypp::Source_Ref src;

    try {
	src = zypp::SourceManager::sourceManager()->findSource(id->asInteger()->value());
    } catch(...) {
	y2error ("Source ID not found: %lld", id->asInteger()->value());
	return YCPBoolean(false);
    }

    // raise priority by one
    src.setPriority(src.priority() + 1);

    return YCPBoolean( true );
}

/****************************************************************************************
 * Pkg::SourceLowerPriority (integer SrcId) -> void
 *
 * Lower priority of source.
 *
 * @param SrcId Specifies the InstSrc.
 *
 * @return bool
 */
YCPValue
PkgModuleFunctions::SourceLowerPriority (const YCPInteger& id)
{
    zypp::Source_Ref src;

    try {
	src = zypp::SourceManager::sourceManager()->findSource(id->asInteger()->value());
    } catch(...) {
	y2error ("Source ID not found: %lld", id->asInteger()->value());
	return YCPBoolean(false);
    }

    // lower priority by one
    src.setPriority(src.priority() - 1);

    return YCPBoolean( true );
}

/****************************************************************************************
 * Pkg::SourceSaveRanks () -> boolean
 *
 * Save ranks to disk. Return true on success, false on error.
 **/
YCPValue
PkgModuleFunctions::SourceSaveRanks ()
{
    y2internal( "SourceSaveRanks not implemented yet, please, report" );

  return YCPBoolean( true );
}

/****************************************************************************************
 * @builtin SourceMoveDownloadArea
 *
 * @short Move download area of CURL-based sources to specified directory
 * @param path specifies the path to move the download area to
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceMoveDownloadArea (const YCPString & path)
{
    try
    {
	y2milestone( "Moving download area of all sources to %s", path->value().c_str()) ;
	zypp::SourceManager::sourceManager()->reattachSources (path->value());
    }
    catch (zypp::Exception & excpt)
    {
	_last_error.setLastError(excpt.asUserString());
	y2error("Pkg::SourceMoveDownloadArea has failed: %s", excpt.msg().c_str() );
	return YCPBoolean(false);
    }

    y2milestone( "Download areas moved");

    return YCPBoolean(true);
}

