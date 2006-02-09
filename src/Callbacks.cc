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

   File:	PkgModuleCallbacks.cc

   Author:	Klaus Kaempf <kkaempf@suse.de>
   Maintainer:  Klaus Kaempf <kkaempf@suse.de>

   Purpose:	Implement callbacks from Y2PM to UI/WFM.

/-*/

#include <iostream>

#include <y2util/stringutil.h>

#include "PkgModuleFunctions.h"
#include "Callbacks.h"
#include "Callbacks.YCP.h" // PkgModuleFunctions::CallbackHandler::YCPCallbacks

#include <zypp/ZYppCallbacks.h>
#include <zypp/Package.h>

// FIXME: do this nicer, source create use this to avoid user feedback
// on probing of source type
static bool _silent_probing;

///////////////////////////////////////////////////////////////////
namespace ZyppRecipients {
///////////////////////////////////////////////////////////////////

  typedef PkgModuleFunctions::CallbackHandler::YCPCallbacks YCPCallbacks;

  ///////////////////////////////////////////////////////////////////
  // Data excange. Shared between Recipients, inherited by ZyppReceive.
  ///////////////////////////////////////////////////////////////////
  struct RecipientCtl {
    const YCPCallbacks & _ycpcb;
    public:
      RecipientCtl( const YCPCallbacks & ycpcb_r )
	: _ycpcb( ycpcb_r )
      {}
      virtual ~RecipientCtl() {}
  };

  ///////////////////////////////////////////////////////////////////
  // Base class common to Recipients. Provides RecipientCtl and inherits
  // YCPCallbacks::Send(see comment in PkgModuleCallbacks.YCP.h).
  ///////////////////////////////////////////////////////////////////
  struct Recipient : public YCPCallbacks::Send {
    RecipientCtl & _control; // shared beween Recipients.
    public:
      Recipient( RecipientCtl & control_r )
        : Send( control_r._ycpcb )
        , _control( control_r )
      {}
      virtual ~Recipient() {}
  };


    ///////////////////////////////////////////////////////////////////
    // ConvertDbCallback
    ///////////////////////////////////////////////////////////////////
    struct ConvertDbReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::target::rpm::ConvertDBReport>
    {
	ConvertDbReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

	virtual void reportbegin()
	{
	}

	virtual void reportend()
	{
	}

	virtual void start(zypp::Pathname pname) {
	    CB callback(ycpcb(YCPCallbacks::CB_StartConvertDb));
	    if (callback._set) {
		callback.addStr(pname.asString());
		callback.evaluate();
	    }
	}

	virtual bool progress(int value, zypp::Pathname pth)
	{		
	    CB callback( ycpcb( YCPCallbacks::CB_ProgressConvertDb ) );
	    if (callback._set) {
		callback.addInt( value );
		// TODO adapt callback typeinfo to zypp
		callback.addInt( 100 /* TODO ?? _pc.max()*/ );
		callback.addInt( 0 /*failed*/ );
		callback.addInt( 0 /* ignored */ );
		callback.addInt( 1 /*alreadyInV4*/ );
		callback.evaluate();
	    }

	    // return default value from the parent class
	    return zypp::target::rpm::ConvertDBReport::progress(value, pth);
	}

	virtual void finish(zypp::Pathname path, zypp::target::rpm::ConvertDBReport::Error error, std::string reason)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_StopConvertDb ) );
	    if (callback._set) {
		callback.addInt( error );
		callback.addStr( reason );
		callback.evaluateStr(); // return value ignored by RpmDb
	    }
	}
    };



    ///////////////////////////////////////////////////////////////////
    // RebuildDbCallback
    ///////////////////////////////////////////////////////////////////
    struct RebuildDbReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::target::rpm::RebuildDBReport>
    {
	RebuildDbReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

        virtual void reportbegin()
	{
	}

	virtual void reportend()
	{
	}

	virtual void start(zypp::Pathname path)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_StartRebuildDb ) );
	    if ( callback._set ) {
		callback.evaluate();
	    }
	}

	virtual bool progress(int value, zypp::Pathname pth)
	{		
	    CB callback( ycpcb( YCPCallbacks::CB_ProgressRebuildDb ) );
	    if ( callback._set ) {
		// report changed values
		callback.addInt( value );
		callback.evaluate();
	    }

	    // return default value from the parent class
	    return zypp::target::rpm::RebuildDBReport::progress(value, pth);
	}

	virtual void finish(zypp::Pathname path, zypp::target::rpm::RebuildDBReport::Error error, std::string reason)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_StopRebuildDb ) );
	    if (callback._set) {
		callback.addInt( error );
		callback.addStr( reason );
		callback.evaluateStr(); // return value ignored by RpmDb
	    }
	}
    };

    ///////////////////////////////////////////////////////////////////
    // InstallPkgCallback
    ///////////////////////////////////////////////////////////////////
    struct InstallPkgReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::target::rpm::InstallResolvableReport>
    {
	InstallPkgReceive(RecipientCtl & construct_r) : Recipient(construct_r) 
	{
	}

	virtual void reportbegin()
	{
	  //_pc.reset();
	}

	virtual void reportend()
	{
	}
	
	virtual void start(zypp::Resolvable::constPtr resolvable)
	{
#warning install non-package
	  zypp::Package::constPtr res = 
	    zypp::asKind<zypp::Package>(resolvable);
	  CB callback( ycpcb( YCPCallbacks::CB_StartPackage ) );
	  if (callback._set) {
	    callback.addStr(res->plainRpm());
	    callback.addStr(res->summary());
	    callback.addInt(res->size());	// which size it is?
	    callback.addBool(false);	// is_delete = false (package installation)
	    callback.evaluateBool();
	  }
	}
				  
	virtual bool progress(int value, zypp::Resolvable::constPtr resolvable)
	{		
	    CB callback( ycpcb( YCPCallbacks::CB_ProgressPackage) );
	    if (callback._set) {
		// TODO ?? if ( _pc.updateIfNewPercent( 5 ) ) {
		// report changed values
		callback.addInt( value );
		callback.evaluate();
		//}
	    }

	    // return default value from the parent class
	    return zypp::target::rpm::InstallResolvableReport::progress(value, resolvable);
	}

        virtual Action problem(
          zypp::Resolvable::constPtr resolvable
          , zypp::target::rpm::InstallResolvableReport::Error error
          , std::string description
          , zypp::target::rpm::InstallResolvableReport::RpmLevel level
        ) 
	{
	    if (level != zypp::target::rpm::InstallResolvableReport::RPM_NODEPS_FORCE)
	    {
		y2milestone( "Retrying installation problem with too low severity (%d)", level);
		return zypp::target::rpm::InstallResolvableReport::ABORT;
	    }
 
	    CB callback( ycpcb( YCPCallbacks::CB_DonePackage) );
	    if (callback._set) {
		callback.addInt( error );
		callback.addStr( description );

                std::string ret = callback.evaluateStr();

                // "R" =  retry
                if (ret == "R") return zypp::target::rpm::InstallResolvableReport::RETRY;

                // "C" = cancel
                if (ret == "C") return zypp::target::rpm::InstallResolvableReport::ABORT;

                // otherwise ignore
                return zypp::target::rpm::InstallResolvableReport::IGNORE;
	    }

	    return zypp::target::rpm::InstallResolvableReport::problem
		(resolvable, error, description, level);
	}

	virtual void finish(zypp::Resolvable::constPtr resolvable, Error error, std::string reason, zypp::target::rpm::InstallResolvableReport::RpmLevel level)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_DonePackage) );
	    if (callback._set) {
		callback.addInt( level == zypp::target::rpm::InstallResolvableReport::RPM_NODEPS_FORCE ? error : NO_ERROR);
		callback.addStr( reason );
		callback.evaluateStr(); // return value ignored by RpmDb
	    }
	}
    };


    ///////////////////////////////////////////////////////////////////
    // RemovePkgCallback
    ///////////////////////////////////////////////////////////////////
    struct RemovePkgReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::target::rpm::RemoveResolvableReport>
    {
	RemovePkgReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

	virtual void reportbegin()
	{
	}

	virtual void reportend()
	{
	}

	virtual void start(zypp::Resolvable::constPtr resolvable)
	{
	  CB callback( ycpcb( YCPCallbacks::CB_StartPackage ) );
	  if (callback._set) {
	    callback.addStr(resolvable->name());
	    callback.addStr(std::string());
	    callback.addInt(-1);
	    callback.addBool(false);	// is_delete = false (package installation)
	    callback.evaluateBool();
	  }
	}
				  
	virtual bool progress(int value, zypp::Resolvable::constPtr resolvable)
	{		
	    CB callback( ycpcb( YCPCallbacks::CB_ProgressPackage) );
	    if (callback._set) {
		// TODO ?? if ( _pc.updateIfNewPercent( 5 ) ) {
		// report changed values
		callback.addInt( value );
		callback.evaluate();
		//}
	    }

	    // return default value from the parent class
	    return zypp::target::rpm::RemoveResolvableReport::progress(value, resolvable);
	}

	virtual void finish(zypp::Resolvable::constPtr resolvable, zypp::target::rpm::RemoveResolvableReport::Error error, std::string reason)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_DonePackage) );
	    if (callback._set) {
		callback.addInt( error );
		callback.addStr( reason );
		callback.evaluateStr(); // return value ignored by RpmDb
	    }
	}
    };

    ///////////////////////////////////////////////////////////////////
    // DownloadResolvableCallback
    ///////////////////////////////////////////////////////////////////
    struct DownloadResolvableReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::source::DownloadResolvableReport>
    {
	DownloadResolvableReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

	virtual void reportbegin()
	{
	}

	virtual void reportend()
	{
	}

	virtual void start( zypp::Resolvable::constPtr resolvable_ptr, zypp::Url url)
	{
	  CB callback( ycpcb( YCPCallbacks::CB_StartProvide ) );
	  if (callback._set) {
	    callback.addStr(resolvable_ptr->name());
	    callback.addInt(-1 /* FIXME: size */);
	    callback.addBool(true);	// is_remote = tru always
	    callback.evaluateBool();
	  }
	}
				  
	virtual void finish(zypp::Resolvable::constPtr resolvable, zypp::source::DownloadResolvableReport::Error error, std::string reason)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_DoneProvide) );
	    if (callback._set) {
		callback.addInt( error );
		callback.addStr( reason );
		callback.addStr( std::string() ); // FIXME: on error name, for OK, local path
		callback.evaluateStr(); // return value ignored by RpmDb
	    }
	}
    };

/*
  ///////////////////////////////////////////////////////////////////
  // InstTargetCallbacks::ScriptExecCallback
  ///////////////////////////////////////////////////////////////////
  //
#warning InstTargetCallbacks::ScriptExecCallback is actually YOU specific
  // Actually a YouScriptProgress and the behaviour of percentage
  // report is strange. Space for improvement (e.g. error reporting).
  // Maybe provide a common YCP callback for ScriptExec and let
  // YOU redirect it to YouScriptProgress, if this kind of report
  // is desired.
  //
  struct ScriptExecReceive : public Recipient, public InstTargetCallbacks::ScriptExecCallback
  {
    ScriptExecReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

    virtual void reportbegin() {
    }
    virtual void reportend()   {
    }
    virtual void start( const Pathname & pkpath ) {
      CB callback( ycpcb( YCPCallbacks::CB_YouScriptProgress ) );
      if ( callback._set ) {
	callback.addInt( 0 );
	callback.evaluate();
      }
    }
    **
     * Execution time is unpredictable. ProgressData range will be set to
     * [0:0]. Aprox. every half second progress is reported with incrementing
     * counter value. If <CODE>false</CODE> is returned, execution is canceled.
     **/
/*  virtual bool progress( const ProgressData & prg ) {
      CB callback( ycpcb( YCPCallbacks::CB_YouScriptProgress ) );
      if ( callback._set ) {
	callback.addInt( -1 );
	return callback.evaluateBool();
      }
      return ScriptExecCallback::progress( prg ); // return default implementation
    }
    virtual void stop( PMError error ) {
      CB callback( ycpcb( YCPCallbacks::CB_YouScriptProgress ) );
      if ( callback._set ) {
	callback.addInt( 100 );
	callback.evaluate();
      }
    }
  };

  ///////////////////////////////////////////////////////////////////
  // Y2PMCallbacks::CommitCallback
  ///////////////////////////////////////////////////////////////////
  struct CommitReceive : public Recipient, public Y2PMCallbacks::CommitCallback
  {
    CommitReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

    virtual void reportbegin() {
    }
    virtual void reportend()   {
    }
    virtual void advanceToMedia( constInstSrcPtr srcptr, unsigned mediaNr ) {
      CB callback( ycpcb( YCPCallbacks::CB_SourceChange ) );
      if ( callback._set ) {
	// Translate the (internal) InstSrcPtr to an (external) instOrder vector index
	// FIXME callback.addInt( Y2PM::instSrcManager().instOrderIndex( srcptr ) );
	callback.addInt( mediaNr );
	callback.evaluate();
      }
    }
  };

  ///////////////////////////////////////////////////////////////////
  // Y2PMCallbacks::CommitProvideCallback
  ///////////////////////////////////////////////////////////////////
  struct CommitProvideReceive : public Recipient, public Y2PMCallbacks::CommitProvideCallback
  {*/
    ///////////////////////////////////////////////////////////////////
    // SourceRefreshCallback
    ///////////////////////////////////////////////////////////////////
    struct SourceRefreshReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::source::RefreshSourceReport>
    {
	YCPMap startArgs;

	SourceRefreshReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

	virtual void reportbegin() {
	  startArgs = YCPMap();
	}
	virtual void reportend()   {
	  startArgs = YCPMap();
	}

	virtual void start(zypp::Source_Ref source, zypp::Url url)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_StartSourceRefresh ) );
	    if ( callback._set )
	    {
		YCPMap arg;
		arg->add( YCPString("url"), YCPString( url.asString() ) );

		// search product
		zypp::ResStore store = source.resolvables();
		for (zypp::ResStore::const_iterator it = store.begin();
		   it != store.end();
		   it++)
		{
		    if (zypp::isKind<zypp::Product>(*it))
		    {
			arg->add( YCPString("label"), YCPString( (*it)->name() ) );
		    }
		}
		
/* TODO
		arg->add( YCPString("product_dir"),	YCPString( descr_r->product_dir().asString() ) );
*/
		callback.addMap( arg );
		startArgs = arg; //remember
		callback.evaluate();
	    }
	}

	virtual Action problem(zypp::Source_Ref source, zypp::source::RefreshSourceReport::Error error, std::string description)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ErrorSourceRefresh ) );
	    if ( callback._set )
	    {
		YCPMap arg = startArgs;
// TODO		arg->add( YCPString( "error" ),	YCPSymbol( asString( error_r ) ) );
		arg->add( YCPString( "detail" ),	YCPString( description ) );
		callback.addMap( arg );
		std::string result = callback.evaluateSymbol();

		if ( result == "RETRY" ) return zypp::source::RefreshSourceReport::RETRY;
		if ( result == "SKIP_REFRESH" ) return zypp::source::RefreshSourceReport::IGNORE;
		if ( result == "DISABLE_SOURCE" )
		{
		    source.disable();
		    return zypp::source::RefreshSourceReport::IGNORE;
		}

		// still here?
		y2error("Unexpected Symbol '%s' returned from callback.", result.c_str());
		// return default
	    }

	    // return default implementation
	    return zypp::source::RefreshSourceReport::problem(source, error, description);
	}

	virtual void finish(zypp::Source_Ref source, zypp::source::RefreshSourceReport::Error error, std::string reason)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_DoneSourceRefresh ) );
	    if ( callback._set )
	    {
		YCPMap arg = startArgs;
/* TODO
		arg->add( YCPString( "result" ),	YCPSymbol( asString( result_r ) ) );
		arg->add( YCPString( "cause" ),	YCPSymbol( asString( cause_r ) ) );
*/
		arg->add( YCPString( "detail" ),	YCPString( reason ) );
		callback.addMap( arg ),
		callback.evaluate();
	    }
	}
    };

    ///////////////////////////////////////////////////////////////////
    // MediaChangeCallback
    ///////////////////////////////////////////////////////////////////
    struct MediaChangeReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::media::MediaChangeReport>
    {
	MediaChangeReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

	virtual Action requestMedia(zypp::Source_Ref source, unsigned mediumNr, zypp::media::MediaChangeReport::Error error, std::string description)
	{
	    if ( _silent_probing) 
		return zypp::media::MediaChangeReport::ABORT;
		
	    CB callback( ycpcb( YCPCallbacks::CB_MediaChange ) );
	    if ( callback._set )
	    {
		// error message
		callback.addStr( description );
		// current URL
		callback.addStr( source.url().asString() );

		std::string product_name;

		// get name of the product
		for (zypp::ResStore::iterator it = source.resolvables().begin(); it != source.resolvables().end(); it++)
		{
		    // is it a product object?
		    if (zypp::isKind<zypp::Product>(*it))
		    {
			product_name = (*it)->name();
			break;
		    }	
		}
		
		// current product name
		callback.addStr( product_name );

		// current medium, -1 means enable [Ignore]
		callback.addInt( 0 );

		// current label, not used now
		callback.addStr( std::string() );

		// requested medium
		callback.addInt( mediumNr );

		// requested product, "" = use the current product
		callback.addStr( std::string() );

#warning Double sided media are not supported in MediaChangeCallback
		callback.addBool( false );

		std::string ret = callback.evaluateStr();

		// "" =  retry
		if (ret == "") return zypp::media::MediaChangeReport::RETRY;

		// "I" = ignore wrong media ID
		if (ret == "I") return zypp::media::MediaChangeReport::IGNORE_ID;

		// "C" = cancel
		if (ret == "C") return zypp::media::MediaChangeReport::ABORT;

		// "E" = eject media
		if (ret == "E") return zypp::media::MediaChangeReport::EJECT;
		
		// "S" = skip (ignore) this media
		if (ret == "S") return zypp::media::MediaChangeReport::IGNORE;

		// otherwise change media URL
		// FIXME: validate url
		source.redirect( mediumNr, Url(ret) );
		return zypp::media::MediaChangeReport::CHANGE_URL;
	    }
	   
	    // return default value from the parent class
	    return zypp::media::MediaChangeReport::requestMedia(source, mediumNr, error, description);
	}
    };


    ///////////////////////////////////////////////////////////////////
    // DownloadProgressCallback
    ///////////////////////////////////////////////////////////////////
    struct DownloadProgressReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::source::DownloadFileReport>
    {
	DownloadProgressReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

	virtual void reportbegin()
	{
	}

        virtual void reportend()
	{
	}

	virtual void start( zypp::Source_Ref source, zypp::Url url)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_StartDownload ) );

	    if ( callback._set )
	    {
		callback.addStr( url.asString() );
		callback.addStr( source.path() );
		callback.evaluate();
	    }
	}

	virtual bool progress(int value, zypp::Url url)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ProgressDownload ) );
	    if ( callback._set )
	    {
		// TODO is any check needed?? if ( _pc.updateIfNewPercent() ) {
		// report changed values
		callback.addInt( value );
		callback.addInt( 100 /*TODO FIXME _pc.max()*/ );
		return callback.evaluateBool( true ); // default == continue
	    }

	    return zypp::source::DownloadFileReport::progress(value, url);
	}

	virtual Action problem( zypp::Url url, zypp::source::DownloadFileReport::Error error, std::string description )
	{
	    return zypp::source::DownloadFileReport::ABORT;
	}

	virtual void finish( zypp::Url url, zypp::source::DownloadFileReport::Error error, std::string reason)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_DoneDownload ) );

	    if ( callback._set ) {
		callback.addInt( error );
		callback.addStr( reason );
		callback.evaluate();
	    }
	}
    };

    ///////////////////////////////////////////////////////////////////
    // CreateSourceReport
    ///////////////////////////////////////////////////////////////////
    struct CreateSourceReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::source::CreateSourceReport>
    {
	CreateSourceReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

	virtual void reportbegin()
	{
	}

        virtual void reportend()
	{
	}

        virtual void startData(
          zypp::Url source_url
        ) {}

        virtual void startProbe(zypp::Url url) {
	    _silent_probing = true;
	}

        virtual void endProbe(zypp::Url url) {
	    _silent_probing = false;
	}

        virtual bool progressData(int value, zypp::Url url)
        { return zypp::source::CreateSourceReport::progressData(value, url); }

        virtual Action problem(
          zypp::Url url
          , zypp::source::CreateSourceReport::Error error
          , std::string description
        ) { return zypp::source::CreateSourceReport::problem(url, error, description); }

        virtual void finishData(
          zypp::Url url
          , zypp::source::CreateSourceReport::Error error
          , std::string reason
        ) {}
    };

///////////////////////////////////////////////////////////////////
}; // namespace ZyppRecipients
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
//
//	CLASS NAME : PkgModuleFunctions::CallbackHandler::ZyppReceive
/**
 * @short Manages the Y2PMCallbacks we receive.
 *
 **/
class PkgModuleFunctions::CallbackHandler::ZyppReceive : public ZyppRecipients::RecipientCtl {

  private:

    // RRM DB callbacks
    ZyppRecipients::ConvertDbReceive  _convertDbReceive;
    ZyppRecipients::RebuildDbReceive  _rebuildDbReceive;

    // package callbacks
    ZyppRecipients::InstallPkgReceive _installPkgReceive;
    ZyppRecipients::RemovePkgReceive  _removePkgReceive;
    ZyppRecipients::DownloadResolvableReceive _providePkgReceive;

    // media callback
    ZyppRecipients::DownloadProgressReceive _downloadProgressReceive;
    ZyppRecipients::MediaChangeReceive   _mediaChangeReceive;

    // source manager callback
    ZyppRecipients::SourceRefreshReceive _sourceRefreshReceive;
    ZyppRecipients::CreateSourceReceive _createSourceReceive;

  public:

    ZyppReceive( const YCPCallbacks & ycpcb_r )
      : RecipientCtl( ycpcb_r )
      , _convertDbReceive( *this )
      , _rebuildDbReceive( *this )
      , _installPkgReceive( *this )
      , _removePkgReceive( *this )
      , _providePkgReceive( *this )
      , _downloadProgressReceive( *this )
      , _mediaChangeReceive( *this )
      , _sourceRefreshReceive( *this )
      , _createSourceReceive( *this )
    {
	// connect the receivers
	_convertDbReceive.connect();
	_rebuildDbReceive.connect();
	_installPkgReceive.connect();
	_removePkgReceive.connect();
	_providePkgReceive.connect();
	_mediaChangeReceive.connect();
	_downloadProgressReceive.connect();
	_sourceRefreshReceive.connect();
	_createSourceReceive.connect();
    }

    virtual ~ZyppReceive()
    {
	// disconnect the receivers
	_convertDbReceive.disconnect();
	_rebuildDbReceive.disconnect();
	_installPkgReceive.disconnect();
	_removePkgReceive.disconnect();
	_mediaChangeReceive.disconnect();
	_downloadProgressReceive.disconnect();
	_sourceRefreshReceive.disconnect();
    }
  public:

};

///////////////////////////////////////////////////////////////////
//
//	CLASS NAME : PkgModuleFunctions::CallbackHandler
//
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : PkgModuleFunctions::CallbackHandler::CallbackHandler
//	METHOD TYPE : Constructor
//
PkgModuleFunctions::CallbackHandler::CallbackHandler(  )
    : _ycpCallbacks( *new YCPCallbacks() )
    , _zyppReceive( *new ZyppReceive( _ycpCallbacks ) )
{
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : PkgModuleFunctions::CallbackHandler::~CallbackHandler
//	METHOD TYPE : Destructor
//
PkgModuleFunctions::CallbackHandler::~CallbackHandler()
{
  delete &_zyppReceive;
  delete &_ycpCallbacks;
}

///////////////////////////////////////////////////////////////////
//
//	CLASS NAME : PkgModuleFunctions
//
//      Set YCPCallbacks.  _ycpCallbacks
//
///////////////////////////////////////////////////////////////////

#define SET_YCP_CB(E,A) _callbackHandler._ycpCallbacks.setYCPCallback( CallbackHandler::YCPCallbacks::E, A );

YCPValue PkgModuleFunctions::CallbackStartProvide( const YCPString& args ) {
  return SET_YCP_CB( CB_StartProvide, args );
}
YCPValue PkgModuleFunctions::CallbackProgressProvide( const YCPString& args ) {
  return SET_YCP_CB( CB_ProgressProvide, args );
}
YCPValue PkgModuleFunctions::CallbackDoneProvide( const YCPString& args ) {
  return SET_YCP_CB( CB_DoneProvide, args );
}

YCPValue PkgModuleFunctions::CallbackStartPackage( const YCPString& args ) {
  return SET_YCP_CB( CB_StartPackage, args );
}
YCPValue PkgModuleFunctions::CallbackProgressPackage( const YCPString& args ) {
  return SET_YCP_CB( CB_ProgressPackage, args );
}
YCPValue PkgModuleFunctions::CallbackDonePackage( const YCPString& args ) {
  return SET_YCP_CB( CB_DonePackage, args );
}

YCPValue PkgModuleFunctions::CallbackStartDownload( const YCPString& args ) {
  return SET_YCP_CB( CB_StartDownload, args );
}
YCPValue PkgModuleFunctions::CallbackProgressDownload( const YCPString& args ) {
  return SET_YCP_CB( CB_ProgressDownload, args );
}
YCPValue PkgModuleFunctions::CallbackDoneDownload( const YCPString& args ) {
  return SET_YCP_CB( CB_DoneDownload, args );
}

YCPValue PkgModuleFunctions::CallbackStartSourceRefresh( const YCPString& args ) {
  return SET_YCP_CB( CB_StartSourceRefresh, args );
}
YCPValue PkgModuleFunctions::CallbackErrorSourceRefresh( const YCPString& args ) {
  return SET_YCP_CB( CB_ErrorSourceRefresh, args );
}
YCPValue PkgModuleFunctions::CallbackDoneSourceRefresh( const YCPString& args ) {
  return SET_YCP_CB( CB_DoneSourceRefresh, args );
}

YCPValue PkgModuleFunctions::CallbackMediaChange( const YCPString& args ) {
  // FIXME: Allow omission of 'src' argument in 'src, name'. Since we can
  // handle one callback function at most, passing a src argument
  // implies a per-source callback which isn't implemented anyway.
  return SET_YCP_CB( CB_MediaChange, args );
}

YCPValue PkgModuleFunctions::CallbackSourceChange( const YCPString& args ) {
  return SET_YCP_CB( CB_SourceChange, args );
}

YCPValue PkgModuleFunctions::CallbackYouProgress( const YCPString& args ) {
  return SET_YCP_CB( CB_YouProgress, args );
}

YCPValue PkgModuleFunctions::CallbackYouPatchProgress( const YCPString& args ) {
  return SET_YCP_CB( CB_YouPatchProgress, args );
}

YCPValue PkgModuleFunctions::CallbackYouError( const YCPString& args ) {
  return SET_YCP_CB( CB_YouError, args );
}

YCPValue PkgModuleFunctions::CallbackYouMessage( const YCPString& args ) {
  return SET_YCP_CB( CB_YouMessage, args );
}

YCPValue PkgModuleFunctions::CallbackYouLog( const YCPString& args ) {
  return SET_YCP_CB( CB_YouLog, args );
}

YCPValue PkgModuleFunctions::CallbackYouExecuteYcpScript( const YCPString& args ) {
  return SET_YCP_CB( CB_YouExecuteYcpScript, args );
}
YCPValue PkgModuleFunctions::CallbackYouScriptProgress( const YCPString& args ) {
  return SET_YCP_CB( CB_YouScriptProgress, args );
}

YCPValue PkgModuleFunctions::CallbackStartRebuildDb( const YCPString& args ) {
  return SET_YCP_CB( CB_StartRebuildDb, args );
}
YCPValue PkgModuleFunctions::CallbackProgressRebuildDb( const YCPString& args ) {
  return SET_YCP_CB( CB_ProgressRebuildDb, args );
}
YCPValue PkgModuleFunctions::CallbackNotifyRebuildDb( const YCPString& args ) {
  return SET_YCP_CB( CB_NotifyRebuildDb, args );
}
YCPValue PkgModuleFunctions::CallbackStopRebuildDb( const YCPString& args ) {
  return SET_YCP_CB( CB_StopRebuildDb, args );
}

YCPValue PkgModuleFunctions::CallbackStartConvertDb( const YCPString& args ) {
  return SET_YCP_CB( CB_StartConvertDb, args );
}
YCPValue PkgModuleFunctions::CallbackProgressConvertDb( const YCPString& args ) {
  return SET_YCP_CB( CB_ProgressConvertDb, args );
}
YCPValue PkgModuleFunctions::CallbackNotifyConvertDb( const YCPString& args ) {
  return SET_YCP_CB( CB_NotifyConvertDb, args );
}
YCPValue PkgModuleFunctions::CallbackStopConvertDb( const YCPString& args ) {
  return SET_YCP_CB( CB_StopConvertDb, args );
}

#undef SET_YCP_CB
