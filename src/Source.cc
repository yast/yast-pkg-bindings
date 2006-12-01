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

#include <Callbacks.h>
#include <Callbacks.YCP.h>

#include <zypp/SourceManager.h>
#include <zypp/SourceFactory.h>
#include <zypp/Source.h>
#include <zypp/Product.h>
#include <zypp/target/store/PersistentStorage.h>
#include <zypp/media/MediaManager.h>
#include <zypp/Pathname.h>

#include <stdio.h> // snprintf


/*
  Textdomain "pkg-bindings"
*/

void PkgModuleFunctions::CallSourceReportStart(const std::string &text)
{
    // get the YCP callback handler
    Y2Function* ycp_handler = _callbackHandler._ycpCallbacks.createCallback(CallbackHandler::YCPCallbacks::CB_SourceReportStart);

    // is the callback registered?
    if (ycp_handler != NULL)
    {
	// add parameters
	ycp_handler->appendParameter( YCPInteger(0LL) );
	ycp_handler->appendParameter( YCPString("") );
	ycp_handler->appendParameter( YCPString(text) );
	// evaluate the callback function
	ycp_handler->evaluateCall();
    }
}

void PkgModuleFunctions::CallSourceReportEnd(const std::string &text)
{
    // get the YCP callback handler for end event
    Y2Function* ycp_handler = _callbackHandler._ycpCallbacks.createCallback(CallbackHandler::YCPCallbacks::CB_SourceReportEnd);

    // is the callback registered?
    if (ycp_handler != NULL)
    {
	// add parameters
	ycp_handler->appendParameter( YCPInteger(0LL) );
	ycp_handler->appendParameter( YCPString("") );
	ycp_handler->appendParameter( YCPString(text) );
	ycp_handler->appendParameter( YCPString("NO_ERROR") );
	ycp_handler->appendParameter( YCPString("") );
	// evaluate the callback function
	ycp_handler->evaluateCall();
    }
}

/**
 * Logging helper:
 * call zypp::SourceManager::sourceManager()->findSource
 * and in case of exception, log error and setLastError AND RETHROW
 */
zypp::Source_Ref PkgModuleFunctions::logFindSource (zypp::SourceManager::SourceId id)
{
    zypp::Source_Ref src;

    try
    {
	src = zypp::SourceManager::sourceManager()->findSource(id);
    }
    catch (const zypp::Exception& excpt)
    {
	y2error("Cannot find source %ld: %s",id, excpt.msg().c_str() );
	_last_error.setLastError(excpt.asUserString());
	throw;
    }
    return src;
}

/****************************************************************************************
 * @builtin SourceSetRamCache
 * @short Obsoleted function, do not use
 * @param boolean
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceSetRamCache (const YCPBoolean& a)
{
    y2warning( "Pkg::SourceSetRamCache is obsolete and does nothing");
    return YCPBoolean( true );
}


/****************************************************************************************
 * @builtin SourceRestore
 *
 * @short Restore the sources from the persistent store
 * @description
 * Make sure the Source Manager is up and knows all available installation sources.
 * It's safe to call this multiple times, and once the installation sources are
 * actually enabled, it's even cheap
 *
 * @return boolean True on success
 **/
YCPValue
PkgModuleFunctions::SourceRestore()
{
    CallSourceReportStart(_("Downloading files..."));

    bool success = true;

    try {
	// we always use the configured caches
	if( !zypp::SourceManager::sourceManager()->restore(_target_root, true) )
	{
	    y2error( "Unable to restore all sources" );

	    success = false;
	}
    }
    catch (const zypp::FailedSourcesRestoreException& excpt)
    {
	// FIXME: assuming the sources are already initialized
	y2error ("Error in SourceRestore: %s", excpt.asString().c_str());
	_last_error.setLastError(excpt.asUserString());
	_broken_sources = excpt.aliases();
	success = false;
    }
    catch (const zypp::SourcesAlreadyRestoredException& excpt)
    {
	y2milestone ("At least one source already registered, SourceManager already started.");
	success = true;
    }
    catch (const zypp::Exception& excpt)
    {
	// FIXME: assuming the sources are already initialized
	y2error ("Error in SourceRestore: %s", excpt.asString().c_str());
	_last_error.setLastError(excpt.asUserString());
	success = false;
    }

    CallSourceReportEnd(_("Downloading files..."));

    return YCPBoolean(success);
}


/****************************************************************************************
 * @builtin SourceLoad
 *
 * @short Load resolvables from the installation sources
 * @description
 * Refresh the pool - Add/remove resolvables from the enabled/disabled sources.
 * @return boolean True on success
 **/
YCPValue
PkgModuleFunctions::SourceLoad()
{
    bool success = true;

    CallSourceReportStart(_("Parsing files..."));

    std::list<zypp::SourceManager::SourceId> ids;

    // get all enabled sources
    try
    {
	ids = zypp::SourceManager::sourceManager()->enabledSources();
    }
    catch (const zypp::Exception& excpt)
    {
	y2error ("Error in SourceLoad: %s", excpt.asString().c_str());
	_last_error.setLastError(excpt.asUserString());
	success = false;
    }
   
    // load resolvables the enabled sources 
    for( std::list<zypp::SourceManager::SourceId>::iterator it = ids.begin(); it != ids.end(); ++it)
    {
	try
	{
	    zypp::Source_Ref src = logFindSource(*it);
	    try
	    {
		if( src.enabled() )
		{
		    zypp_ptr()->addResolvables (src.resolvables());
		}
		else
		{
		    // remove the resolvables if they have been added
		    if (src.resStoreInitialized ())
		    {
		        zypp_ptr()->removeResolvables(src.resolvables());
		    }
		}
			
	    }
	    catch (const zypp::Exception& excpt)
	    {
		std::string url = src.url().asString();
		y2error ("Error for %s: %s", url.c_str(), excpt.asString().c_str());
		_last_error.setLastError(url + ": " + excpt.asUserString());
		success = false;

		// disable the source
		y2error("Disabling source %s", url.c_str());
		src.disable();

		// remember the broken source for SourceCleanupBroken
		_broken_sources.insert(src.alias());
	    }
	}
	catch (const zypp::Exception& excpt)
	{
	    success = false;
	}
    }

    CallSourceReportEnd(_("Parsing files..."));

    return YCPBoolean(success);
}


/****************************************************************************************
 * @builtin SourceStartManager
 *
 * @short Start the source manager - restore the sources and load the resolvables
 * @description
 * Calls SourceRestore(), if argument enable is true SourceLoad() is called.
 * @param boolean enable If true the resolvables are loaded from the enabled sources
 *
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceStartManager (const YCPBoolean& enable)
{
    YCPValue success = SourceRestore();

    if( enable->value() )
    {
	if (!success->asBoolean()->value())
	{
	    y2warning("SourceStartManager: Some sources have not been restored, loading only the active sources...");
	}

	// enable all sources and load the resolvables
	success = YCPBoolean(SourceLoad()->asBoolean()->value() && success->asBoolean()->value());
    }

    return success;
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
    try
    {
	SourceStartManager(enabled);

	return SourceGetCurrent(enabled);
    }
    catch (const zypp::Exception& excpt)
    {
	y2error ("Error in SourceStartCache: %s", excpt.asString().c_str());
	_last_error.setLastError(excpt.asUserString());
    }
    // catch an exception from boost (e.g. a file cannot be read by non-root user)
    catch (const std::exception& err)
    {
	y2error ("Error in SourceStartCache: %s", err.what());
	_last_error.setLastError(err.what());
    }
    catch (...)
    {
	y2error("Unknown error in SourceStartCache");
    }

    return YCPList();
}

/****************************************************************************************
 * @builtin SourceCleanupBroken
 *
 * @short Clean up all sources that were not properly restored on the last
 * call of SourceStartManager or SourceStartCache.
 *
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceCleanupBroken ()
{
    bool success = true;

    zypp::storage::PersistentStorage store;
    store.init( _target_root );
    
    for( std::set<std::string>::const_iterator it = _broken_sources.begin() ;
	it != _broken_sources.end() ; ++it )
    {
	try {
	    store.deleteSource( *it );
	} catch( const zypp::Exception& excpt )
	{
	    success = false;
	}
    }
    
    _broken_sources.clear();
    
    return YCPBoolean(success);
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

/******************************************************************************
 * @builtin SourceSaveAll
 *
 * @short Save all InstSrces.
 * @return boolean
 **/
YCPValue
PkgModuleFunctions::SourceSaveAll ()
{
    try
    {
	y2milestone( "Storing the source setup in %s", _target_root.asString().c_str()) ;
	zypp::SourceManager::sourceManager()->store( _target_root, true );
    }
    catch (zypp::Exception & excpt)
    {
	y2error("Pkg::SourceSaveAll has failed: %s", excpt.msg().c_str() );
	_last_error.setLastError(excpt.asUserString());
	return YCPBoolean(false);
    }

    y2milestone( "All sources saved");

    return YCPBoolean(true);
}

/****************************************************************************************
 * @builtin SourceFinishAll
 *
 * @short Save and then disable all InstSrces.
 * @description
 * If there are no enabled sources, do nothing
 * (idempotence hack, broken design: #155459, #176013, use SourceSaveAll).
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
 * "url"	: YCPString (without password, but see SourceURL),
 * "alias"	: YCPString,
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
	src = logFindSource(id->value());
    }
    catch (const zypp::Exception& excpt)
    {
	return YCPVoid ();
    }

    data->add( YCPString("enabled"),		YCPBoolean(src.enabled()));
    data->add( YCPString("autorefresh"),	YCPBoolean(src.autorefresh()));
    data->add( YCPString("type"),		YCPString(src.type()));
    data->add( YCPString("product_dir"),	YCPString(src.path().asString()));
    data->add( YCPString("url"),		YCPString(src.url().asString()));
    data->add( YCPString("alias"),		YCPString(src.alias()));
    return data;
}

/******************************************************************************
 * @builtin SourceURL
 *
 * @short Get full source URL, including password
 * @param integer SrcId Specifies the InstSrc to query.
 * @return string or nil on failure
 **/
YCPValue
PkgModuleFunctions::SourceURL (const YCPInteger& id)
{
    zypp::Source_Ref src;

    try
    {
	src = logFindSource(id->value());
    }
    catch (const zypp::Exception& excpt)
    {
	return YCPVoid ();
    }

    // #186842
    return YCPString(src.url().asCompleteString());
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
	src = logFindSource(id->value());
    }
    catch (const zypp::Exception& excpt)
    {
	return YCPVoid ();
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
 * "label"		: YCPString,
 * "vendor"		: YCPString,
 * "productname"	: YCPString,
 * "productversion"	: YCPString,
 * "relnotesurl"	: YCPString,
 * ];
 * </code>
 *
 * @return map
 **/
YCPValue
PkgModuleFunctions::SourceProductData (const YCPInteger& src_id)
{
    YCPMap ret;

    try
    {
	zypp::Source_Ref src = logFindSource(src_id->value());

	// find a product for the given source
	zypp::ResPool::byKind_iterator it = zypp_ptr()->pool().byKindBegin(zypp::ResTraits<zypp::Product>::kind);

	for( ; it != zypp_ptr()->pool().byKindEnd(zypp::ResTraits<zypp::Product>::kind) ; ++it) 
	{
	    zypp::Product::constPtr product = boost::dynamic_pointer_cast<const zypp::Product>( it->resolvable() );

	    if( product->source() == src )
	    {
		ret->add( YCPString("label"),		YCPString( product->summary() ) );
		ret->add( YCPString("vendor"),		YCPString( product->vendor() ) );
		ret->add( YCPString("productname"),	YCPString( product->name() ) );
		ret->add( YCPString("productversion"),	YCPString( product->edition().version() ) );
		ret->add( YCPString("relnotesurl"), 	YCPString( product->releaseNotesUrl().asString()));

		#warning SourceProductData not finished
		/*
		  data->add( YCPString("datadir"),		YCPString( descr->content_datadir().asString() ) );
		TODO (?): "baseproductname", "baseproductversion", "defaultbase", "architectures",
		"requires", "linguas", "labelmap", "language", "timezone", "descrdir", "datadir"
		*/

		break;
	    }
	}

	if( it == zypp_ptr()->pool().byKindEnd(zypp::ResTraits<zypp::Product>::kind) )
	{
	    y2error ("Product for source '%lld' not found", src_id->value());
	}
    }
    catch (...)
    {
    }

    return ret;
}

/****************************************************************************************
 * @builtin SourceProduct
 * @short Obsoleted function, do not use, see SourceProductData builtin
 * @deprecated
 * @param integer
 * @return map empty map
 **/
YCPValue
PkgModuleFunctions::SourceProduct (const YCPInteger& id)
{
    /* TODO FIXME */
  y2error("Pkg::SourceProduct() is obsoleted, use Pkg::SourceProductData() instead!");
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
    CallSourceReportStart(_("Downloading file..."));

    zypp::Source_Ref src;
    bool found = true;

    try {
	src = logFindSource(id->value());
    }
    catch (const zypp::Exception& excpt)
    {
	found = false;
    }

    zypp::filesystem::Pathname path;

    if (found)
    {
	try {
	    path = src.provideFile(f->value(), mid->asInteger()->value());
	}
	catch (const zypp::Exception& excpt)
	{
	    _last_error.setLastError(excpt.asUserString());
	    y2milestone ("File not found: %s", f->value_cstr());
	    found = false;
	}
    }

    CallSourceReportEnd(_("Downloading file..."));

    if (found)
    {
	return YCPString(path.asString());
    }
    else
    {
	return YCPVoid();
    }

}

/****************************************************************************************
 * @builtin SourceProvideOptionalFile
 *
 * @short Make an optional file available at the local filesystem
 * @description
 * Let an InstSrc provide some file (make it available at the local filesystem).
 * If the file doesn't exist don't ask user for another medium and return nil
 *
 * @param integer SrcId	Specifies the InstSrc .
 * @param integer medianr Number of the media the file is located on ('1' for the 1st media).
 * @param string file Filename relative to the media root.
 *
 * @return string local path as string
 **/
YCPValue
PkgModuleFunctions::SourceProvideOptionalFile (const YCPInteger& id, const YCPInteger& mid, const YCPString& f)
{
    CallSourceReportStart(_("Downloading files..."));

    YCPValue ret;

    extern ZyppRecipients::MediaChangeSensitivity _silent_probing;

    // remember the current value 
    ZyppRecipients::MediaChangeSensitivity _silent_probing_old = _silent_probing;

    // disable media change callback
    _silent_probing = ZyppRecipients::MEDIA_CHANGE_OPTIONALFILE;

    zypp::Source_Ref src;
    bool found = true;

    try {
	src = logFindSource(id->value());
    }
    catch (const zypp::Exception& excpt)
    {
	found = false;
    }

    zypp::filesystem::Pathname path;

    if (found)
    {
	try {
	    path = src.provideFile(f->value(), mid->asInteger()->value());
	}
	catch (const zypp::Exception& excpt)
	{
	    found = false;
	    // the file is optional, don't set error flag here
	}
    }

 
    // set the original probing value
    _silent_probing = _silent_probing_old;


    CallSourceReportEnd(_("Downloading files..."));

    if (found)
    {
	return YCPString(path.asString());
    }
    else
    {
	return YCPVoid();
    }
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
	src = logFindSource(id->value());
    }
    catch (const zypp::Exception& excpt)
    {
	return YCPVoid ();
    }

    zypp::filesystem::Pathname path;

    try
    {
	path = src.provideDirTree(d->value(), mid->asInteger()->value());
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
    zypp::Source_Ref src;
    try {
	src = logFindSource(id->value());
    }
    catch (const zypp::Exception& excpt)
    {
	return YCPBoolean(false);
    }

    try {
	zypp::Pathname pth = src.path();
	zypp::Url url = zypp::Url(u->value ());
	zypp::media::MediaManager media_mgr;
	zypp::media::MediaAccessId media_id = media_mgr.open(url);
	src.changeMedia(media_id, pth);
    }
    catch (const zypp::Exception & excpt)
    {
	_last_error.setLastError(excpt.asUserString());
        y2error ("Cannot change media for source %lld: %s", id->asInteger()->value(), excpt.msg().c_str());
	return YCPBoolean(false);
    }
    return YCPBoolean(true);
}

/****************************************************************************************
 * @builtin SourceInstallOrder
 *
 * @short not implemented, do not use (Explicitly set an install order.)
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


static std::string timestamp ()
{
    time_t t = time(NULL);
    struct tm * tmp = localtime(&t);

    if (tmp == NULL) {
	return "";
    }

    char outstr[50];
    if (strftime(outstr, sizeof(outstr), "%Y%m%d-%H%M%S", tmp) == 0) {
	return "";
    }
    return outstr;
}

/**
 * Take the ?alias=foo part of old_url, if any, and return it,
 * putting the rest to new_url.
 * \throws Exception on malformed URLs I guess
 */
static std::string removeAlias (const zypp::Url & old_url,
				zypp::Url & new_url)
{
  std::string alias;
  new_url = old_url;
  zypp::url::ParamMap query = new_url.getQueryStringMap ();
  zypp::url::ParamMap::iterator alias_it = query.find ("alias");
  if (alias_it != query.end ())
  {
    alias = alias_it->second;
    query.erase (alias_it);
    new_url.setQueryStringMap (query);
  }
  return alias;
}


/** Create a Source and immediately put it into the SourceManager.
 * \return the SourceId
 * \throws Exception if Source creation fails
*/
zypp::SourceManager::SourceId
createManagedSource( const zypp::Url & url_r,
		     const zypp::Pathname & path_r,
		     const bool base_source,
		     const std::string& type )
{
  y2milestone ("Original URL: %s", url_r.asString().c_str());

  // #158850#c17, if the URL contains an alias, we use that
  zypp::Url new_url;
  string alias = removeAlias (url_r, new_url);

  zypp::Source_Ref newsrc = 
  (type.empty()) ?
    // autoprobe source type
    zypp::SourceFactory().createFrom(new_url, path_r, alias, zypp::filesystem::Pathname(), base_source) :
    // use required source type, autorefresh = true
    zypp::SourceFactory().createFrom(type, new_url, path_r, alias, zypp::filesystem::Pathname(), base_source, true);
  
  if (alias.empty ())
  {
    // use product name+edition as the alias
    // (URL is not enough for different sources in the same DVD drive)
    // alias must be unique, add timestamp
    zypp::ResStore products = newsrc.resolvables (zypp::Product::TraitsType::kind);
    zypp::ResStore::iterator
      b = products.begin (),
      e = products.end ();  
    if (b != e)
    {
      zypp::ResObject::Ptr p = *b;
      alias = p->name () + '-' + p->edition ().asString () + '-';
    }
    alias += timestamp ();
  
    newsrc.setAlias( alias );
  }

  zypp::SourceManager::SourceId id = zypp::SourceManager::sourceManager()->addSource( newsrc );
  y2milestone("Added source %lu: %s %s (alias %s)", id, new_url.asString().c_str(), path_r.asString().c_str(), alias.c_str() );
  return id;
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
    return YCPList();
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
    
    if( products.empty() )
    {
	// no products found, use the base URL instead
	zypp::SourceFactory::ProductEntry entry ;
	products.insert( entry );
    }
    
    for( zypp::SourceFactory::ProductSet::const_iterator it = products.begin();
	it != products.end() ; ++it )
    {
	try
	{
	    id = createManagedSource(url, it->_dir, false, "");
	    ids->add( YCPInteger(id) );
	}
	catch ( const zypp::Exception& excpt)
	{
	    y2error("SourceScan for '%s' product '%s' has failed"
		, url.asString().c_str(), pn.asString().c_str());
	    _last_error.setLastError(excpt.asUserString());
	}
    }
  } else {
    y2debug("Creating source...");

    try
    {
	id = createManagedSource(url, pn, false, "");
	ids->add( YCPInteger(id) );
    }
    catch ( const zypp::Exception& excpt)
    {
	y2error("SourceScan for '%s' product '%s' has failed"
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
  // not base product, autoprobe source type
  return SourceCreateEx (media, pd, false, YCPString(""));
}

YCPValue
PkgModuleFunctions::SourceCreateBase (const YCPString& media, const YCPString& pd)
{
  // base product, autoprobe source type
  return SourceCreateEx (media, pd, true, YCPString(""));
}

/**
 * @builtin SourceCreateType
 * @short Create source of required type
 * @description
 * Create a source without autoprobing the source type. This builtin should be used only for "Plaindir" sources, because Plaindir sources are not automatically probed in SourceCreate() builtin.
 * @param media URL of the source
 * @param pd product directory (if empty the products will be searched)
 * @param type type of the source ("YaST", "YUM" or "Plaindir")
*/

YCPValue
PkgModuleFunctions::SourceCreateType (const YCPString& media, const YCPString& pd, const YCPString& type)
{
  // not base product, autoprobe source type
  return SourceCreateEx (media, pd, false, type);
}

YCPValue
PkgModuleFunctions::SourceCreateEx (const YCPString& media, const YCPString& pd, bool base, const YCPString& source_type)
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
    return YCPInteger (-1);
  }


  YCPList ids;
  int ret = -1;

  const std::string type = source_type->value();

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

    if( products.empty() )
    {
	// no products found, use the base URL instead
	zypp::SourceFactory::ProductEntry entry ;
	products.insert( entry );
    }

    for( zypp::SourceFactory::ProductSet::const_iterator it = products.begin();
	it != products.end() ; ++it )
    {
	try
	{
	    unsigned id = createManagedSource(url, it->_dir, base, type);

	    zypp::Source_Ref src = zypp::SourceManager::sourceManager()->findSource(id);

	    src.enable();

	    CallSourceReportStart(_("Parsing files..."));
    	    zypp_ptr()->addResolvables (src.resolvables());
	    CallSourceReportEnd(_("Parsing files..."));

	    // return the id of the first product
	    if ( ret == -1 )
		ret = id;

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
	ret = createManagedSource(url, pn, base, type);

	zypp::Source_Ref src = zypp::SourceManager::sourceManager()->findSource(ret);

	src.enable();

	CallSourceReportStart(_("Parsing files..."));
    	zypp_ptr()->addResolvables (src.resolvables());
	CallSourceReportEnd(_("Parsing files..."));
    }
    catch ( const zypp::Exception& excpt)
    {
	y2error("SourceCreate for '%s' product '%s' has failed"
	    , url.asString().c_str(), pn.asString().c_str());
	_last_error.setLastError(excpt.asUserString());
    }
  }

  PkgFreshen();
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
	src = logFindSource(id->value());
    }
    catch (const zypp::Exception& excpt)
    {
	return YCPBoolean(false);
    }

    if (enabled)
    {
	src.enable();
	PkgFreshen();
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
	src = logFindSource(id->value());
    }
    catch (const zypp::Exception& excpt)
    {
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
	src = logFindSource(id->value());
    }
    catch (const zypp::Exception& excpt)
    {
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
    zypp::SourceManager::SourceId src_id = id->value();
    try
    {
	zypp::Source_Ref src = zypp::SourceManager::sourceManager()->
	    findSource(src_id);
	// If the source cache is corrupt for any reason, inst_source
	// would first try to access the resolvables only now,
	// preventing the deletion of the broken source.  But we do
	// not need to remove resolvables that never got to the pool
	// in the first place. #174840.
	if (src.resStoreInitialized ())
	{
	    zypp_ptr()->removeResolvables(src.resolvables());
	}

	zypp::SourceManager::sourceManager()->removeSource(src_id);
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

  PkgFreshen();
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
	src = logFindSource(id->value());
    }
    catch (const zypp::Exception& excpt)
    {
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
	src = logFindSource(id->value());
    }
    catch (const zypp::Exception& excpt)
    {
	return YCPBoolean(false);
    }

    // lower priority by one
    src.setPriority(src.priority() - 1);

    return YCPBoolean( true );
}

/****************************************************************************************
 * Pkg::SourceSaveRanks () -> boolean
 *
 * @short Obsoleted function, do not use
 * @return boolean true
 **/
YCPValue
PkgModuleFunctions::SourceSaveRanks ()
{
  y2error( "SourceSaveRanks not implemented" );

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

