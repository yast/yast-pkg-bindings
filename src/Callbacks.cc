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
   Summary:     Register Package Manager callbacks
   Namespace:   Pkg

   Purpose:	Implement callbacks from ZYpp to UI/WFM.

/-*/

#include <iostream>

#include <y2util/stringutil.h>

#include "PkgModuleFunctions.h"
#include "Callbacks.h"
#include "Callbacks.YCP.h" // PkgModuleFunctions::CallbackHandler::YCPCallbacks

#include <zypp/ZYppCallbacks.h>
#include <zypp/Package.h>
#include <zypp/Product.h>
#include <zypp/KeyRing.h>
#include <zypp/Digest.h>
#include <zypp/SourceManager.h>

// FIXME: do this nicer, source create use this to avoid user feedback
// on probing of source type

ZyppRecipients::MediaChangeSensitivity _silent_probing = ZyppRecipients::MEDIA_CHANGE_FULL;

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
		callback.addInt( 100 );
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
	zypp::Resolvable::constPtr _last;
	int last_reported;

	InstallPkgReceive(RecipientCtl & construct_r) : Recipient(construct_r)
	{
	}

	virtual void reportbegin()
	{
	}

	virtual void reportend()
	{
	}

	virtual void start(zypp::Resolvable::constPtr resolvable)
	{
	  // initialize the counter
	  last_reported = 0;

#warning install non-package
	  zypp::Package::constPtr res =
	    zypp::asKind<zypp::Package>(resolvable);

	  // if we have started this resolvable already, don't do it again
	  if( _last == resolvable )
	    return;

	  CB callback( ycpcb( YCPCallbacks::CB_StartPackage ) );
	  if (callback._set) {
	    callback.addStr(res->location());
	    callback.addStr(res->summary());
	    callback.addInt(res->size());
	    callback.addBool(false);	// is_delete = false (package installation)
	    callback.evaluateBool();
	  }

	  _last = resolvable;
	}

	virtual bool progress(int value, zypp::Resolvable::constPtr resolvable)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ProgressPackage) );
	    // call the callback function only if the difference since the last call is at least 5%
	    // or if 100% is reached
	    if (callback._set && (value - last_reported >= 5 || last_reported - value >= 5 || value == 100))
	    {
		callback.addInt( value );
		bool res = callback.evaluateBool();

		if( !res )
		    y2milestone( "Package installation callback returned abort" );

		last_reported = value;
		return res;
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

	    _last = zypp::Resolvable::constPtr();

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
	    if (error != zypp::target::rpm::InstallResolvableReport::NO_ERROR && level != zypp::target::rpm::InstallResolvableReport::RPM_NODEPS_FORCE)
	    {
		y2milestone( "Skipping finish due to retrying installation problem with too low severity (%d)", level);
		return;
	    }

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
	    callback.addBool(true);	// is_delete = true
	    callback.evaluateBool();
	  }
	}

	virtual bool progress(int value, zypp::Resolvable::constPtr resolvable)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ProgressPackage) );
	    if (callback._set) {
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
	static int last_source_id;
	static int last_source_media;

	DownloadResolvableReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}
	int last_reported;

	virtual void reportbegin()
	{
	}

	virtual void reportend()
	{
	}

	virtual void start( zypp::Resolvable::constPtr resolvable_ptr, zypp::Url url)
	{
	  unsigned size = 0;
	  last_reported = 0;

	  if ( zypp::isKind<zypp::Package> (resolvable_ptr) )
	  {
	    zypp::Package::constPtr pkg =
		zypp::asKind<zypp::Package>(resolvable_ptr);

	    size = pkg->archivesize();

	    int source_id = pkg->source().numericId();
	    int media_nr = pkg->sourceMediaNr();

	    if( source_id != last_source_id || media_nr != last_source_media )
	    {
	      CB callback( ycpcb( YCPCallbacks::CB_SourceChange ) );
	      if (callback._set) {
	        callback.addInt( source_id );
	        callback.addInt( media_nr );
	        callback.evaluate();
	      }
	      last_source_id = source_id;
	      last_source_media = media_nr;
            }
	  }

	  CB callback( ycpcb( YCPCallbacks::CB_StartProvide ) );
	  if (callback._set) {
	    bool remote = url.getScheme() != "cd" && url.getScheme() != "dvd"
		&& url.getScheme() != "nfs";
	    callback.addStr(resolvable_ptr->name());
	    callback.addInt( size );
	    callback.addBool(remote);
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

        virtual bool progress(int value, zypp::Resolvable::constPtr resolvable_ptr)
        {
	    CB callback( ycpcb( YCPCallbacks::CB_ProgressProvide) );
	    if (callback._set && (value - last_reported >= 5 || last_reported - value >= 5 || value == 100))
	    {
		last_reported = value;
		callback.addInt( value );
		return callback.evaluateBool(); // return value ignored by RpmDb
	    }

	    return zypp::source::DownloadResolvableReport::progress(value, resolvable_ptr);
	}

	virtual Action problem(zypp::Resolvable::constPtr resolvable_ptr, zypp::source::DownloadResolvableReport::Error error, std::string description)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_DoneProvide) );
	    if (callback._set) {
		callback.addInt( error );
		callback.addStr( description );
		callback.addStr( std::string() ); // FIXME: on error name, for OK, local path
                std::string ret = callback.evaluateStr();

                // "R" =  retry
                if (ret == "R") return zypp::source::DownloadResolvableReport::RETRY;

                // "C" = cancel
                if (ret == "C") return zypp::source::DownloadResolvableReport::ABORT;

                // otherwise return the default value from the parent class
	    }

            // return the default value from the parent class
	    return zypp::source::DownloadResolvableReport::problem(resolvable_ptr, error, description);
	}

    };

    int DownloadResolvableReceive::last_source_id = -1;
    int DownloadResolvableReceive::last_source_media = -1;

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
			arg->add( YCPString("product_dir"), YCPString( source.path().asString() ) );
		    }
		}


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

	virtual bool progress(int value, zypp::Source_Ref source)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ProgressSourceRefresh) );

	    if (callback._set)
	    {
		callback.addInt( value );
		callback.addInt( source.numericId() );
		return callback.evaluateBool();
	    }

	    // return default value from the parent class
	    return zypp::source::RefreshSourceReport::progress(value, source);
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
	    if ( _silent_probing == MEDIA_CHANGE_DISABLE )
		return zypp::media::MediaChangeReport::ABORT;

	    if ( _silent_probing == MEDIA_CHANGE_OPTIONALFILE
	      && error == zypp::media::MediaChangeReport::NOT_FOUND )
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
			product_name = (*it)->summary();
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
		// try/catch to catch invalid URLs
		try {
		    zypp::Url ret_url (ret);
		    source.redirect( mediumNr, ret_url );
		    return zypp::media::MediaChangeReport::CHANGE_URL;
		}
		catch ( ... )
		{
		    // invalid URL, try again
		    return zypp::media::MediaChangeReport::RETRY;
		}
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
	int last_reported;

	virtual void reportbegin()
	{
	}

        virtual void reportend()
	{
	}

	virtual void start( zypp::Source_Ref source, zypp::Url url)
	{
	    last_reported = 0;
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
	    // call the callback function only if the difference since the last call is at least 5%
	    // or if 100% is reached
	    if (callback._set && (value - last_reported >= 5 || last_reported - value >= 5 || value == 100))
	    {
		last_reported = value;
		// report changed values
		callback.addInt( value );
		callback.addInt( 100  );
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
        ) {
	    _silent_probing = MEDIA_CHANGE_DISABLE;
	}

        virtual void startProbe(zypp::Url url) {
	    _silent_probing = MEDIA_CHANGE_DISABLE;
	}

        virtual void endProbe(zypp::Url url) {
	    _silent_probing = MEDIA_CHANGE_FULL;
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
        ) {
	    _silent_probing = MEDIA_CHANGE_FULL;
	}
    };

    struct ResolvableReport : public Recipient, public zypp::callback::ReceiveReport<zypp::target::MessageResolvableReport>
    {
	ResolvableReport( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

	virtual void show(zypp::Message::constPtr message)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ResolvableReport) );

	    if (callback._set)
	    {
		zypp::Patch::constPtr patch = message->patch();

		// patch name
		callback.addStr(patch ? patch->name() : message->name());
		// patch summary
		callback.addStr(patch ? patch->summary() : message->summary());
		// message itself
		callback.addStr(message->text().asString());

		callback.evaluate();
	    }
	}
    };

    ///////////////////////////////////////////////////////////////////
    // DigestReport handler
    ///////////////////////////////////////////////////////////////////
    struct DigestReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::DigestReport>
    {
	DigestReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

	virtual bool askUserToAcceptNoDigest( const zypp::Pathname &file )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_AcceptFileWithoutChecksum) );

	    if (callback._set)
	    {
		callback.addStr(file.asString());

		return callback.evaluateBool();
	    }

	    return zypp::DigestReport::askUserToAcceptNoDigest(file);
	}

	virtual bool askUserToAccepUnknownDigest( const zypp::Pathname &file, const std::string &name )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_AcceptUnknownDigest) );

	    if (callback._set)
	    {
		callback.addStr(file.asString());
		callback.addStr(name);

		return callback.evaluateBool();
	    }

	    return zypp::DigestReport::askUserToAccepUnknownDigest(file, name);
	}

	virtual bool askUserToAcceptWrongDigest( const zypp::Pathname &file, const std::string &requested, const std::string &found )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_AcceptWrongDigest) );

	    if (callback._set)
	    {
		callback.addStr(file.asString());
		callback.addStr(requested);
		callback.addStr(found);

		return callback.evaluateBool();
	    }

	    return zypp::DigestReport::askUserToAcceptWrongDigest(file, requested, found);
	}
		
    };


    ///////////////////////////////////////////////////////////////////
    // KeyRingReport handler
    ///////////////////////////////////////////////////////////////////
    struct KeyRingReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::KeyRingReport>
    {
	KeyRingReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

	virtual bool askUserToTrustKey( const std::string &keyid
            , const std::string &keyname, const std::string &fingerprint )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ImportGpgKey) );

	    if (callback._set)
	    {
		callback.addStr(keyid);
		callback.addStr(keyname);
                callback.addStr(fingerprint);

		return callback.evaluateBool();
	    }

	    return zypp::KeyRingReport::askUserToTrustKey(keyid, keyname, fingerprint);
	}

	virtual bool askUserToAcceptUnknownKey(const std::string &file, const std::string &keyid, const std::string &keyname, const std::string &fingerprint)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_AcceptUnknownGpgKey) );

	    if (callback._set)
	    {
		callback.addStr(file);
		callback.addStr(keyid);
		callback.addStr(keyname);
                callback.addStr(fingerprint);

		return callback.evaluateBool();
	    }

	    return zypp::KeyRingReport::askUserToAcceptUnknownKey(file, keyid, keyname, fingerprint );
	}

	virtual bool askUserToAcceptUnsignedFile(const std::string &file)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_AcceptUnsignedFile) );

	    if (callback._set)
	    {
		callback.addStr(file);

		return callback.evaluateBool();
	    }

	    return zypp::KeyRingReport::askUserToAcceptUnsignedFile(file);
	}

	virtual bool askUserToAcceptVerificationFailed(const std::string &file,
	    const std::string &keyid, const std::string &keyname, const std::string &fingerprint)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_AcceptVerificationFailed) );

	    if (callback._set)
	    {
		callback.addStr(file);
		callback.addStr(keyid);
		callback.addStr(keyname);
                callback.addStr(fingerprint);

		return callback.evaluateBool();
	    }

	    return zypp::KeyRingReport::askUserToAcceptVerificationFailed(file, keyid, keyname, fingerprint);
	}
    };

    ///////////////////////////////////////////////////////////////////
    // KeyRingSignals handler
    ///////////////////////////////////////////////////////////////////
    struct KeyRingSignal : public Recipient, public zypp::callback::ReceiveReport<zypp::KeyRingSignals>
    {
	KeyRingSignal ( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

        virtual void trustedKeyAdded( const zypp::KeyRing &keyring, const std::string &keyid, const std::string &keyname, const std::string &fingerprint )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_TrustedKeyAdded) );

	    if (callback._set)
	    {
		callback.addStr(keyid);
		callback.addStr(keyname);
                callback.addStr(fingerprint);
	    }
	}

        virtual void trustedKeyRemoved( const zypp::KeyRing &keyring, const std::string &keyid, const std::string &keyname, const std::string &fingerprint )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_TrustedKeyRemoved) );

	    if (callback._set)
	    {
		callback.addStr(keyid);
		callback.addStr(keyname);
                callback.addStr(fingerprint);
	    }
	}
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

    // resolvable report
    ZyppRecipients::ResolvableReport _resolvableReport;

    // digest callback
    ZyppRecipients::DigestReceive _digestReceive;

    // key ring callback
    ZyppRecipients::KeyRingReceive _keyRingReceive;

    // key ring signal callback
    ZyppRecipients::KeyRingSignal _keyRingSignal;

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
      , _resolvableReport( *this )
      , _digestReceive( *this )
      , _keyRingReceive( *this )
      , _keyRingSignal( *this )
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
	_resolvableReport.connect();
 	_digestReceive.connect();
	_keyRingReceive.connect();
	_keyRingSignal.connect();
    }

    virtual ~ZyppReceive()
    {
	// disconnect the receivers
	_convertDbReceive.disconnect();
	_rebuildDbReceive.disconnect();
	_installPkgReceive.disconnect();
	_removePkgReceive.disconnect();
	_providePkgReceive.disconnect();
	_mediaChangeReceive.disconnect();
	_downloadProgressReceive.disconnect();
	_sourceRefreshReceive.disconnect();
	_createSourceReceive.disconnect();
	_resolvableReport.disconnect();
	_digestReceive.disconnect();
	_keyRingReceive.disconnect();
	_keyRingSignal.disconnect();
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
/**
 * @builtin CallbackProgressSourceRefresh
 * @short Register a callback function
 * @param string args Name of the callback handler function. Required callback
 *  prototype is <code>boolean(integer value, integer sourceId)</code>.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackProgressSourceRefresh( const YCPString& args ) {
  return SET_YCP_CB( CB_ProgressSourceRefresh, args );
}
YCPValue PkgModuleFunctions::CallbackErrorSourceRefresh( const YCPString& args ) {
  return SET_YCP_CB( CB_ErrorSourceRefresh, args );
}
YCPValue PkgModuleFunctions::CallbackDoneSourceRefresh( const YCPString& args ) {
  return SET_YCP_CB( CB_DoneSourceRefresh, args );
}

YCPValue PkgModuleFunctions::CallbackResolvableReport( const YCPString& args ) {
  return SET_YCP_CB( CB_ResolvableReport, args );
}

/**
 * @builtin CallbackImportGpgKey
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(string keyid, string keyname, string keydetails)</code>. The callback function should ask user whether the key is trusted, returned true value means the key is trusted.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackImportGpgKey( const YCPString& args ) {
  return SET_YCP_CB( CB_ImportGpgKey, args );
}

/**
 * @builtin CallbackAcceptUnknownGpgKey
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(string filename, string keyid, string keyname)</code>. The callback function should ask user whether the unknown key can be accepted, returned true value means to accept the file.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackAcceptUnknownGpgKey( const YCPString& args ) {
  return SET_YCP_CB( CB_AcceptUnknownGpgKey, args );
}

/**
 * @builtin CallbackAcceptUnsignedFile
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(string filename)</code>. The callback function should ask user whether the unsigned file can be accepted, returned true value means to accept the file.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackAcceptUnsignedFile( const YCPString& args ) {
  return SET_YCP_CB( CB_AcceptUnsignedFile, args );
}

/**
 * @builtin CallbackAcceptFileWithoutChecksum
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(string filename)</code>. The callback function should ask user whether the unsigned file can be accepted, returned true value means to accept the file.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackAcceptFileWithoutChecksum( const YCPString& args ) {
  return SET_YCP_CB( CB_AcceptFileWithoutChecksum, args );
}

/**
 * @builtin CallbackAcceptVerificationFailed
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(string filename, string keyid, string keyname)</code>. The callback function should ask user whether the unsigned file can be accepted, returned true value means to accept the file.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackAcceptVerificationFailed( const YCPString& args ) {
  return SET_YCP_CB( CB_AcceptVerificationFailed, args );
}

/**
 * @builtin CallbackAcceptWrongDigest
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(string filename, string requested_digest, string found_digest)</code>. The callback function should ask user whether the wrong digest can be accepted, returned true value means to accept the file.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackAcceptWrongDigest( const YCPString& func)
{
  return SET_YCP_CB( CB_AcceptWrongDigest, func );
}

/**
 * @builtin CallbackAcceptUnknownDigest
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(string filename, string name)</code>. The callback function should ask user whether the uknown digest can be accepted, returned true value means to accept the digest.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackAcceptUnknownDigest( const YCPString& func)
{
  return SET_YCP_CB( CB_AcceptUnknownDigest, func );
}

/**
 * @builtin CallbackTrustedKeyAdded
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void(string keyid, string keyname)</code>. The callback function should inform user that a trusted key has been added.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackTrustedKeyAdded( const YCPString& args ) {
  return SET_YCP_CB( CB_TrustedKeyAdded, args );
}

/**
 * @builtin CallbackTrustedKeyRemoved
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>void(string keyid, string keyname)</code>. The callback function should inform user that a trusted key has been removed.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackTrustedKeyRemoved( const YCPString& args ) {
  return SET_YCP_CB( CB_TrustedKeyRemoved, args );
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
