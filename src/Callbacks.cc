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

#include <y2util/Y2SLog.h>
#include <y2util/stringutil.h>

#include "PkgModuleFunctions.h"
#include "Callbacks.h"
#include "Callbacks.YCP.h" // PkgModuleFunctions::CallbackHandler::YCPCallbacks

#include <zypp/ZYppCallbacks.h>

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

	virtual void finish(Pathname path, Error error, std::string reason)
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

	virtual void finish(zypp::Pathname path, Error error, std::string reason)
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
	    return zypp::target::rpm::InstallResolvableReport::progress(value, resolvable);
	}

	virtual void finish(zypp::Resolvable::constPtr resolvable, Error error, std::string reason)
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

	virtual void start(zypp::Resolvable::Ptr resolvable)
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
				  
	virtual bool progress(int value, zypp::Resolvable::Ptr resolvable)
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

	virtual void finish(zypp::Resolvable::Ptr resolvable, Error error, std::string reason)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_DonePackage) );
	    if (callback._set) {
		callback.addInt( error );
		callback.addStr( reason );
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
  {
    ///////////////////////////////////////////////////////////////////
    // Redirecting MediaCallbacks::DownloadProgressCallback
    // to trigger CB_ProgressProvide during CommitProvide.
    struct RedirectDownloadProgress : public Recipient, public ReportReceive<MediaCallbacks::DownloadProgressCallback>
    {
      RedirectDownloadProgress( RecipientCtl & construct_r )
	: Recipient( construct_r )
	, ReportReceive<MediaCallbacks::DownloadProgressCallback>( MediaCallbacks::downloadProgressReport )
      {}

      ProgressCounter _pc;

      virtual void reportbegin() {
	_pc.reset();
      }
      virtual void reportend() {
      }
      virtual void start( const Url & url_r, const Pathname & localpath_r ) {
      }
      virtual bool progress( const ProgressData & prg ) {
	CB callback( ycpcb( YCPCallbacks::CB_ProgressProvide ) );
	if ( callback._set ) {
	  _pc = prg;
	  if ( _pc.updateIfNewPercent() ) {
	    // report changed values
	    callback.addInt( _pc.percent() );
	    return callback.evaluateBool( true ); // default == continue
	  }
	}
        return DownloadProgressCallback::progress( prg );
      }
      virtual void stop( PMError error ) {
      }
    };
    ///////////////////////////////////////////////////////////////////

    CommitProvideReceive( RecipientCtl & construct_r )
      : Recipient( construct_r )
      , _redirect( 0 )
    {}

    RedirectDownloadProgress * _redirect;
    string _name;
    FSize  _size;
    bool   _isRemote;

    virtual void reportbegin() {
      // redirect MediaCallbacks::downloadProgressReport
      _redirect = new RedirectDownloadProgress( _control );
    }
    virtual void reportend()   {
      // restrore MediaCallbacks::downloadProgressReport
      delete _redirect;
      _redirect = 0;
    }
    virtual void start( constPMPackagePtr pkg, bool sourcepkg ) {
      // remember values to send on attempt
      _isRemote = pkg->isRemote();
      if ( sourcepkg ) {
	_name = pkg->nameEd() + ".src";
	_size = pkg->sourcesize(); // download size
      } else {
	_name = pkg->nameEdArch();
	_size = pkg->archivesize(); // download size
      }
    }
    virtual CBSuggest attempt( unsigned cnt ) {
      if ( _isRemote ) {
	CB callback( ycpcb( YCPCallbacks::CB_StartProvide ) );
	if ( callback._set ) {
	  callback.addStr( _name );
	  callback.addInt( _size );
	  callback.addBool( _isRemote );
	  callback.evaluate(); // CB_StartProvide is void
	}
      }
      return CommitProvideCallback::attempt( cnt ); // return default implementation
    }

    virtual CBSuggest result( PMError error, const Pathname & localpath ) {
      if ( error || _isRemote ) {
	CB callback( ycpcb( YCPCallbacks::CB_DoneProvide ) );
	if ( callback._set ) {
	  callback.addInt( error );
	  callback.addStr( error.errstr() );
	  if ( error ) {
	    // localpath is empty, send the name instead
	    callback.addStr( _name );
	  } else {
	    callback.addStr( localpath );
	  }
	  return CBSuggest( callback.evaluateStr() );
	}
      }
      return CommitProvideCallback::result( error, localpath ); // return default implementation
    }
    virtual void stop( PMError error, const Pathname & localpath ) {
    }
  };

  ///////////////////////////////////////////////////////////////////
  // Y2PMCallbacks::CommitInstallCallback
  ///////////////////////////////////////////////////////////////////
  struct CommitInstallReceive : public Recipient, public Y2PMCallbacks::CommitInstallCallback
  {
    ///////////////////////////////////////////////////////////////////
    // Redirecting RpmDbCallbacks::InstallPkgCallback
    // to triggr progress only
    struct RedirectInstallPkg : public Recipient, public ReportReceive<RpmDbCallbacks::InstallPkgCallback>
    {
      RedirectInstallPkg( RecipientCtl & construct_r )
        : Recipient( construct_r )
        , ReportReceive<RpmDbCallbacks::InstallPkgCallback>( RpmDbCallbacks::installPkgReport )
      {}

      ProgressCounter _pc;

      virtual void reportbegin() {
	_pc.reset();
      }
      virtual void reportend()   {
      }
      virtual void start( const Pathname & filename ) {
      }
      virtual void progress( const ProgressData & prg ) {
	CB callback( ycpcb( YCPCallbacks::CB_ProgressPackage ) );
	if ( callback._set ) {
	  _pc = prg;
	  if ( _pc.updateIfNewPercent( 5 ) ) {
	    // report changed values
	    callback.addInt( _pc.percent() );
	    callback.evaluate();
	  }
	}
      }
      virtual void stop( PMError error ) {
      }
    };
    ///////////////////////////////////////////////////////////////////

    CommitInstallReceive( RecipientCtl & construct_r )
      : Recipient( construct_r )
      , _redirect( 0 )
    {}

    RedirectInstallPkg * _redirect;
    string   _name;
    FSize    _size;
    string   _summary;

    virtual void reportbegin() {
      // redirect RpmDbCallbacks::installPkgReport
      _redirect = new RedirectInstallPkg( _control );
    }
    virtual void reportend()   {
      // restore RpmDbCallbacks::installPkgReport
      delete _redirect;
      _redirect = 0;
    }
    virtual void start( constPMPackagePtr pkg, bool sourcepkg, const Pathname & path ) {
      // remember values to send on attempt
      if ( sourcepkg ) {
	_name = pkg->nameEd() + ".src";
	_size = pkg->sourcesize(); // don't have the contentsize
      } else {
	_name = pkg->nameEdArch();
	_size = pkg->size(); // content size
      }
      _summary = pkg->summary();
    }
    virtual CBSuggest attempt( unsigned cnt ) {
      CB callback( ycpcb( YCPCallbacks::CB_StartPackage ) );
      if ( callback._set ) {
	callback.addStr( _name );
	callback.addStr( _summary );
	callback.addInt( _size );
	callback.addBool( *is_delete* false );
	bool res = callback.evaluateBool(); // CB_StartPackage is "true" to continue, "false" to cancel
	return CBSuggest( res ? CBSuggest::PROCEED : CBSuggest::CANCEL );
      }
      return CommitInstallCallback::attempt( cnt ); // return default implementation
    }
    virtual CBSuggest result( PMError error ) {
      CB callback( ycpcb( YCPCallbacks::CB_DonePackage ) );
      if ( callback._set ) {
	callback.addInt( error );
	callback.addStr( error.errstr() );
	return CBSuggest( callback.evaluateStr() );
      }
      return CommitInstallCallback::result( error ); // return default implementation
    }
    virtual void stop( PMError error ) {
    }
  };

  ///////////////////////////////////////////////////////////////////
  // Y2PMCallbacks::CommitRemoveCallback
  ///////////////////////////////////////////////////////////////////
  struct CommitRemoveReceive : public Recipient, public Y2PMCallbacks::CommitRemoveCallback
  {
    ///////////////////////////////////////////////////////////////////
    // Redirecting RpmDbCallbacks::RemovePkgCallback
    // to triggr progress only
    struct RedirectRemovePkg : public Recipient, public ReportReceive<RpmDbCallbacks::RemovePkgCallback>
    {
      RedirectRemovePkg( RecipientCtl & construct_r )
        : Recipient( construct_r )
        , ReportReceive<RpmDbCallbacks::RemovePkgCallback>( RpmDbCallbacks::removePkgReport )
      {}

      ProgressCounter _pc;

      virtual void reportbegin() {
	_pc.reset();
      }
      virtual void reportend()   {
      }
      virtual void start( const std::string & label ) {
      }
      virtual void progress( const ProgressData & prg ) {
	CB callback( ycpcb( YCPCallbacks::CB_ProgressPackage ) );
	if ( callback._set ) {
	  _pc = prg;
	  if ( _pc.updateIfNewPercent( 5 ) ) {
	    // report changed values
	    callback.addInt( _pc.percent() );
	    callback.evaluate();
	  }
	}
      }
      virtual void stop( PMError error ) {
      }
    };
    ///////////////////////////////////////////////////////////////////

    CommitRemoveReceive( RecipientCtl & construct_r )
      : Recipient( construct_r )
      , _redirect( 0 )
    {}

    RedirectRemovePkg * _redirect;
    string   _name;
    FSize    _size;
    string   _summary;

    virtual void reportbegin() {
      // redirect RpmDbCallbacks::removePkgReport
      _redirect = new RedirectRemovePkg( _control );
    }
    virtual void reportend()   {
      // restore RpmDbCallbacks::removePkgReport
      delete _redirect;
      _redirect = 0;
    }
    virtual void start( constPMPackagePtr pkg ) {
      // remember values to send on attempt
      _name = pkg->nameEdArch();
      _size = pkg->size(); // content size
      _summary = pkg->summary();
    }
    virtual CBSuggest attempt( unsigned cnt ) {
      CB callback( ycpcb( YCPCallbacks::CB_StartPackage ) );
      if ( callback._set ) {
	callback.addStr( _name );
	callback.addStr( _summary );
	callback.addInt( _size );
	callback.addBool( *is_delete* true );
	bool res = callback.evaluateBool(); // CB_StartPackage is "true" to continue, "false" to cancel
	return CBSuggest( res ? CBSuggest::PROCEED : CBSuggest::CANCEL );
      }
      return CommitRemoveCallback::attempt( cnt ); // return default implementation
    }
    virtual CBSuggest result( PMError error ) {
      CB callback( ycpcb( YCPCallbacks::CB_DonePackage ) );
      if ( callback._set ) {
	callback.addInt( error );
	callback.addStr( error.errstr() );
	return CBSuggest( callback.evaluateStr() );
      }
      return CommitRemoveCallback::result( error ); // return default implementation
    }
    virtual void stop( PMError error ) {
    }
  };
*/

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
		arg->add( YCPString("url"),		YCPString( url.asString() ) );
/* TODO
		arg->add( YCPString("product_dir"),	YCPString( descr_r->product_dir().asString() ) );
		arg->add( YCPString("label"),		YCPString( descr_r->content_label() ) );
*/
		callback.addMap( arg );
		startArgs = arg; //remember
		callback.evaluate();
	    }
	}

	virtual Action problem(zypp::Source_Ref source, Error error, std::string description)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ErrorSourceRefresh ) );
	    if ( callback._set )
	    {
		YCPMap arg = startArgs;
// TODO		arg->add( YCPString( "error" ),	YCPSymbol( asString( error_r ) ) );
		arg->add( YCPString( "detail" ),	YCPString( description ) );
		callback.addMap( arg );
		string result = callback.evaluateSymbol();

		if ( result == "RETRY" ) return RETRY;
		if ( result == "SKIP_REFRESH" ) return IGNORE;
		if ( result == "DISABLE_SOURCE" )
		{
		    source.disable();
		    return IGNORE;
		}

		// still here?
		INT << "Unexpected Symbol '" << result << "' returned from callback." << endl;
		// return default
	    }

	    // return default implementation
	    return zypp::source::RefreshSourceReport::problem(source, error, description);
	}

	virtual void finish(zypp::Source_Ref source, Error error, std::string reason)
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

	virtual Action requestMedia(zypp::Source_Ref source, unsigned mediumNr, Error error, std::string description)
	{
	    if ( _silent_probing) 
		return ABORT;
		
	    CB callback( ycpcb( YCPCallbacks::CB_MediaChange ) );
	    if ( callback._set )
	    {
		callback.addStr( description );
		callback.addStr( source.url().asString() );	// current URL
		// TODO FIXME
		callback.addStr( string("descr->label()") );
		callback.addInt( 0 );
		callback.addStr( string() );
		callback.addInt( mediumNr );
		// TODO FIXME
		callback.addStr( string("descr->media_label( expectedMedianr )()") );
		callback.addBool( false /* TODO FIXME descr->media_doublesided()*/ );

		std::string ret = callback.evaluateStr();

		// "" =  retry
		if (ret == "") return RETRY;

		// "I" = ignore wrong media ID
		if (ret == "I") return IGNORE_ID;

		// "C" = cancel
		if (ret == "C") return ABORT;

		// "E" = eject media
		if (ret == "E") return EJECT;
		
		// "S" = skip (ignore) this media
		if (ret == "S") return IGNORE;

		// otherwise change media URL
		return CHANGE_URL;
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
		// TODO FIXME
		callback.addStr( std::string("TODO: localpath_r") );
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

	virtual Action problem( zypp::Url url, Error error, std::string description )
	{
	    return ABORT;
	}

	virtual void finish( zypp::Url url, Error error, std::string reason)
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
	    y2internal ("Probing for source creation started");
	    _silent_probing = true;
	}

        virtual void endProbe(zypp::Url url) {
	    _silent_probing = false;
	}

        virtual bool progressData(int value, zypp::Url url)
        { return zypp::source::CreateSourceReport::progressData(value, url); }

        virtual Action problem(
          zypp::Url url
          , Error error
          , std::string description
        ) { return zypp::source::CreateSourceReport::problem(url, error, description); }

        virtual void finishData(
          zypp::Url url
          , Error error
          , std::string reason
        ) {}
    };

/*
  ///////////////////////////////////////////////////////////////////
  // InstYou::Callbacks
  ///////////////////////////////////////////////////////////////////
  //
  // YOU is still special. Does not Trigger via Report.
  //
  struct YouReceive : public Recipient, public InstYou::Callbacks
  {
    YouReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

    virtual bool progress( int percent )
    {
      D__ << "you progress: " << percent << endl;
      CB callback( ycpcb( YCPCallbacks::CB_YouProgress ) );
      if ( callback._set ) {
	callback.addInt( percent );
	return callback.evaluateBool();
      }
      return false;
    }

    virtual bool patchProgress( int percent, const string & pkg )
    {
      D__ << "you patch progress: " << percent << endl;
      CB callback( ycpcb( YCPCallbacks::CB_YouPatchProgress ) );
      if ( callback._set ) {
	callback.addInt( percent );
	callback.addStr( pkg );
	return callback.evaluateBool();
      }
      return false;
    }

    virtual PMError showError( const string &type, const string &text,
                               const string &details )
    {
      D__ << "you error: " << text << endl;
      CB callback( ycpcb( YCPCallbacks::CB_YouError ) );
      if ( callback._set ) {
	callback.addStr( type );
	callback.addStr( text );
	callback.addStr( details );
        string result = callback.evaluateStr();
        INT << "callback result: " << result << endl;
        DBG << "callback result: " << result << endl;
        if ( result == "" ) return PMError();
        if ( result == "abort" ) return YouError::E_user_abort;
        if ( result == "skip" ) return YouError::E_user_skip;
        if ( result == "skipall" ) return YouError::E_user_skip_all;
        if ( result == "retry" ) return YouError::E_user_retry;
        return PMError::E_error;
      }
      return YouError::E_callback_missing;
    }

    virtual PMError showMessage( const string &type,
                                 const list<PMYouPatchPtr> &patches )
    {
      D__ << "you showmessage: " << type << endl;
      CB callback( ycpcb( YCPCallbacks::CB_YouMessage ) );
      if ( callback._set ) {
	callback.addStr( type );

        YCPList patchesArg;
        list<PMYouPatchPtr>::const_iterator it;
        for( it = patches.begin(); it != patches.end(); ++it ) {
          YCPMap patch; ///  = PkgModuleFunctions::YouPatch( *it );
          patchesArg.add( patch );
        }

        callback._func->appendParameter( patchesArg );

        string result = callback.evaluateStr();
        if ( result == "" ) return PMError();
        if ( result == "abort" ) return YouError::E_user_abort;
        if ( result == "skip" ) return YouError::E_user_skip;
        return PMError::E_error;
      }
      return YouError::E_callback_missing;
    }

    virtual void log( const string &text )
    {
      D__ << "you log: " << text << endl;
      CB callback( ycpcb( YCPCallbacks::CB_YouLog ) );
      if ( callback._set ) {
	callback.addStr( text );
        bool success = callback.evaluate();
        if ( !success ) ERR << "Error evaluating YouLog callback." << endl;
      }
    }

    virtual bool executeYcpScript( const string & script ) {
      D__ << "you execute YCP script" << endl;
      CB callback( ycpcb( YCPCallbacks::CB_YouExecuteYcpScript ) );
      if ( callback._set ) {
	callback.addStr( script );
	return callback.evaluateBool();
      }
      return false;
    }
  };
  ///////////////////////////////////////////////////////////////////
*/
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

    // media callback
    ZyppRecipients::DownloadProgressReceive _downloadProgressReceive;
    ZyppRecipients::MediaChangeReceive   _mediaChangeReceive;

    // source manager callback
    ZyppRecipients::SourceRefreshReceive _sourceRefreshReceive;
    ZyppRecipients::CreateSourceReceive _createSourceReceive;

/*
    // InstTargetCallbacks
    ZyppRecipients::ScriptExecReceive _scriptExecReceive;

    // Y2PMCallbacks
    ZyppRecipients::CommitReceive        _commitReceive;
    ZyppRecipients::CommitProvideReceive _commitProvideReceive;
    ZyppRecipients::CommitInstallReceive _commitInstallReceive;
    ZyppRecipients::CommitRemoveReceive  _commitRemoveReceive;

    // YouCallbacks
    ZyppRecipients::YouReceive _youReceive;
*/

  public:

    ZyppReceive( const YCPCallbacks & ycpcb_r )
      : RecipientCtl( ycpcb_r )
      , _convertDbReceive( *this )
      , _rebuildDbReceive( *this )
      , _installPkgReceive( *this )
      , _removePkgReceive( *this )
      , _downloadProgressReceive( *this )
      , _mediaChangeReceive( *this )
      , _sourceRefreshReceive( *this )
      , _createSourceReceive( *this )
/*      , _scriptExecReceive( *this )
      , _commitReceive( *this )
      , _commitProvideReceive( *this )
      , _commitInstallReceive( *this )
      , _commitRemoveReceive( *this )
      , _youReceive( *this )*/
    {
	// connect the receivers
	_convertDbReceive.connect();
	_rebuildDbReceive.connect();
	_installPkgReceive.connect();
	_removePkgReceive.connect();
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
