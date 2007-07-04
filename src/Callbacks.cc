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
#include <zypp/PublicKey.h>
#include <zypp/Digest.h>

// FIXME: do this nicer, source create use this to avoid user feedback
// on probing of source type

ZyppRecipients::MediaChangeSensitivity _silent_probing = ZyppRecipients::MEDIA_CHANGE_FULL;

// remember redirected URLs
// FIXME huh?
std::map<zypp::Repository::NumericId, std::map<unsigned, std::string> > redirect_map;

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
	    y2milestone("Convert DB Init Callback");
	}

	virtual void reportend()
	{
	    y2milestone("Convert DB Destroy Callback");
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

	virtual void finish(zypp::Pathname path, zypp::target::rpm::ConvertDBReport::Error error, const std::string &reason)
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

	virtual void finish(zypp::Pathname path, zypp::target::rpm::RebuildDBReport::Error error, const std::string &reason)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_StopRebuildDb ) );
	    if (callback._set) {
		callback.addInt( error );
		callback.addStr( reason );
		callback.evaluateStr(); // return value ignored by RpmDb
	    }
	}
    };

    struct ScanDbReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::target::rpm::ScanDBReport>
    {
	ScanDbReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

        virtual void start()
	{
	    CB callback( ycpcb( YCPCallbacks::CB_StartScanDb ) );
	    if ( callback._set ) {
		callback.evaluate();
	    }
	}

        virtual bool progress(int value)
        {
	    CB callback( ycpcb( YCPCallbacks::CB_ProgressScanDb ) );
	    if ( callback._set ) {
		// report changed values
		callback.addInt( value );
		return callback.evaluateBool();
	    }

	    // return default value from the parent class
	    return zypp::target::rpm::ScanDBReport::progress(value);
	}

        virtual Action problem(zypp::target::rpm::ScanDBReport::Error error, const std::string &description)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ErrorScanDb ) );
	    if ( callback._set ) {
		callback.addInt( error );
		callback.addStr( description );

		std::string ret = callback.evaluateStr();

                // "C" = cancel (abort)
                if (ret == "C") return zypp::target::rpm::ScanDBReport::ABORT;

                // "R" =  retry
                if (ret == "R") return zypp::target::rpm::ScanDBReport::RETRY;

                // "I" = ignore
                if (ret == "I") return zypp::target::rpm::ScanDBReport::IGNORE;

		y2warning("Unknown callback result '%s', using default value", ret.c_str());
	    }

	    return zypp::target::rpm::ScanDBReport::problem(error, description);
	}

        virtual void finish(zypp::target::rpm::ScanDBReport::Error error, const std::string &reason)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_DoneScanDb ) );
	    if ( callback._set ) {
		callback.addInt( error );
		callback.addStr( reason );

		callback.evaluate();
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
          , const std::string &description
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

	virtual void finish(zypp::Resolvable::constPtr resolvable, Error error, const std::string &reason, zypp::target::rpm::InstallResolvableReport::RpmLevel level)
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

		bool res = callback.evaluateBool();

		if( !res )
		{
		    y2milestone( "Package remove callback returned abort" );
		}

		return res;
	    }

	    // return default value from the parent class
	    return zypp::target::rpm::RemoveResolvableReport::progress(value, resolvable);
	}

	virtual void finish(zypp::Resolvable::constPtr resolvable, zypp::target::rpm::RemoveResolvableReport::Error error, const std::string &reason)
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
	int last_reported_delta_download;
	int last_reported_delta_apply;
	int last_reported_patch_download;

	virtual void reportbegin()
	{
	}

	virtual void reportend()
	{
	}

	virtual void start( zypp::Resolvable::constPtr resolvable_ptr, const zypp::Url &url)
	{
	  unsigned size = 0;
	  last_reported = 0;

	  if ( zypp::isKind<zypp::Package> (resolvable_ptr) )
	  {
	    zypp::Package::constPtr pkg =
		zypp::asKind<zypp::Package>(resolvable_ptr);

	    size = pkg->archivesize();

// FIXME NID
	    int source_id = 0;//pkg->source().numericId();
// FIXME
	    int media_nr = 0;//pkg->sourceMediaNr();

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

	virtual void finish(zypp::Resolvable::constPtr resolvable, zypp::source::DownloadResolvableReport::Error error, const std::string &reason)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_DoneProvide) );
	    if (callback._set) {
		callback.addInt( error );
		callback.addStr( reason );
		callback.addStr( resolvable->name() );
		callback.evaluateStr(); // return value is ignored
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

	virtual Action problem(zypp::Resolvable::constPtr resolvable_ptr, zypp::source::DownloadResolvableReport::Error error, const std::string &description)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_DoneProvide) );
	    if (callback._set) {
		callback.addInt( error );
		callback.addStr( description );
		callback.addStr( resolvable_ptr->name() );
                std::string ret = callback.evaluateStr();

                // "R" =  retry
                if (ret == "R") return zypp::source::DownloadResolvableReport::RETRY;

                // "C" = cancel
                if (ret == "C") return zypp::source::DownloadResolvableReport::ABORT;

                // "I" = ignore
                if (ret == "I") return zypp::source::DownloadResolvableReport::IGNORE;

                // otherwise return the default value from the parent class
	    }

            // return the default value from the parent class
	    return zypp::source::DownloadResolvableReport::problem(resolvable_ptr, error, description);
	}

	// Download delta rpm:
	// - path below url reported on start()
	// - expected download size (0 if unknown)
	// - download is interruptable
	// - problems are just informative
	virtual void startDeltaDownload( const zypp::Pathname & filename, const zypp::ByteCount & downloadsize )
	{
	    // reset the counter
	    last_reported_delta_download = 0;

	    CB callback( ycpcb( YCPCallbacks::CB_StartDeltaDownload) );
	    if (callback._set) {
		callback.addStr( filename.asString() );
		callback.addInt( downloadsize );

		callback.evaluate();
	    }
	}

	virtual bool progressDeltaDownload( int value )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ProgressDeltaDownload) );
	    if (callback._set && (value - last_reported_delta_download >= 5 || last_reported_delta_download - value >= 5 || value == 100))
	    {
		last_reported_delta_download = value;
		callback.addInt( value );

		return callback.evaluateBool();
	    }

	    return zypp::source::DownloadResolvableReport::progressDeltaDownload(value);
	}

	virtual void problemDeltaDownload( const std::string &description )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ProblemDeltaDownload ) );
	    if (callback._set) {
		callback.addStr( description );

		callback.evaluate();
	    }
	}

	virtual void finishDeltaDownload()
	{
	    CB callback( ycpcb( YCPCallbacks::CB_FinishDeltaDownload ) );

	    if (callback._set)
	    {
		callback.evaluate();
	    }
	}
	

	// Apply delta rpm:
	// - local path of downloaded delta
	// - apply is not interruptable
	// - problems are just informative
	virtual void startDeltaApply( const zypp::Pathname & filename )
	{
	    // reset the counter
	    last_reported_delta_apply = 0;

	    CB callback( ycpcb( YCPCallbacks::CB_StartDeltaApply) );
	    if (callback._set) {
		callback.addStr( filename.asString() );

		callback.evaluate();
	    }
	}

	virtual void progressDeltaApply( int value )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ProgressDeltaApply ) );
	    if (callback._set && (value - last_reported_delta_apply >= 5 || last_reported_delta_apply - value >= 5 || value == 100))
	    {
		last_reported_delta_apply = value;
		callback.addInt( value );

		callback.evaluate();
	    }
	}

	virtual void problemDeltaApply( const std::string &description )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ProblemDeltaApply ) );

	    if (callback._set) {
		callback.addStr( description );

		callback.evaluate();
	    }
	}

	virtual void finishDeltaApply()
	{
	    CB callback( ycpcb( YCPCallbacks::CB_FinishDeltaApply ) );

	    if (callback._set)
	    {
		callback.evaluate();
	    }
	}
	

	// Download patch rpm:
	// - path below url reported on start()
	// - expected download size (0 if unknown)
	// - download is interruptable
	virtual void startPatchDownload( const zypp::Pathname & filename, const zypp::ByteCount & downloadsize )
	{
	    // reset the counter
	    last_reported_patch_download = 0;

	    CB callback( ycpcb( YCPCallbacks::CB_StartPatchDownload ) );
	    if (callback._set) {
		callback.addStr( filename.asString() );
		callback.addInt( downloadsize );

		callback.evaluate();
	    }
	}

	virtual bool progressPatchDownload( int value )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ProgressPatchDownload) );
	    if (callback._set && (value - last_reported_patch_download >= 5 || last_reported_patch_download - value >= 5 || value == 100))
	    {
		last_reported_patch_download = value;
		callback.addInt( value );

		return callback.evaluateBool();
	    }

	    return zypp::source::DownloadResolvableReport::progressPatchDownload(value);
	}

	virtual void problemPatchDownload( const std::string &description )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ProblemPatchDownload ) );

	    if (callback._set) {
		callback.addStr( description );

		callback.evaluate();
	    }
	}

	virtual void finishPatchDownload()
	{
	    CB callback( ycpcb( YCPCallbacks::CB_FinishPatchDownload ) );

	    if (callback._set)
	    {
		callback.evaluate();
	    }
	}

    };

    int DownloadResolvableReceive::last_source_id = -1;
    int DownloadResolvableReceive::last_source_media = -1;


    ///////////////////////////////////////////////////////////////////
    // DownloadProgressReceive
    ///////////////////////////////////////////////////////////////////
    struct DownloadProgressReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::media::DownloadProgressReport>
    {
	int last_reported;

	DownloadProgressReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

        virtual void start( const zypp::Url &file, zypp::Pathname localfile )
	{
	    last_reported = 0;
	    CB callback( ycpcb( YCPCallbacks::CB_StartDownload ) );

	    if ( callback._set )
	    {
		callback.addStr( file.asString() );
		callback.addStr( localfile.asString() );
		callback.evaluate();
	    }
	}

        virtual bool progress(int value, const zypp::Url &file)
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

	    return zypp::media::DownloadProgressReport::progress(value, file);
	}

        virtual Action problem( const zypp::Url &file, zypp::media::DownloadProgressReport::Error error, const std::string &description)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_DoneProvide) );

	    if (callback._set) {
		callback.addInt( error );
		callback.addStr( description );
		callback.addStr( file.asString() ); // FIXME: on error name, for OK, local path
                std::string ret = callback.evaluateStr();

		y2milestone("DoneProvide result: %s", ret.c_str());

                // "R" =  retry
                if (ret == "R") return zypp::media::DownloadProgressReport::RETRY;

                // "C" = cancel
                if (ret == "C") return zypp::media::DownloadProgressReport::ABORT;

                // "I" = cancel
                if (ret == "I") return zypp::media::DownloadProgressReport::IGNORE;

                // otherwise return the default value from the parent class
	    }
	    return zypp::media::DownloadProgressReport::problem(file, error, description);
	}

        virtual void finish( const zypp::Url &file, zypp::media::DownloadProgressReport::Error error, const std::string &reason) 
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
    // ScriptExecCallbacks
    ///////////////////////////////////////////////////////////////////
    struct ScriptExecReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::target::ScriptResolvableReport>
    {
	ScriptExecReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

	virtual void start( const zypp::Resolvable::constPtr &script_r, const zypp::Pathname &path_r, zypp::target::ScriptResolvableReport::Task task)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ScriptStart) );
	    if ( callback._set )
	    {
		callback.addStr(script_r->name());
		callback.addStr(script_r->edition().asString());
		callback.addStr(script_r->arch().asString());
		callback.addStr(path_r);
		callback.addBool(task == zypp::target::ScriptResolvableReport::DO);
		callback.evaluate();
	    }
	}
       
	virtual bool progress( zypp::target::ScriptResolvableReport::Notify ping, const std::string &out = std::string() )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ScriptProgress) );

	    if ( callback._set )
	    {
		callback.addBool(ping == zypp::target::ScriptResolvableReport::PING);
		callback.addStr(out);

		// false = abort the script
		return callback.evaluateBool();
	    }
	    else
	    {
		// return the default implementation
		return zypp::target::ScriptResolvableReport::progress(ping, out);
	    }
	}

	virtual void problem( const std::string &description )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ScriptProblem) );

	    if ( callback._set )
	    {
		callback.addStr(description);
		callback.evaluate();
	    }
	}

	virtual void finish()
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ScriptFinish) );

	    if ( callback._set )
	    {
		callback.evaluate();
	    }
	}
    };

    struct MessageReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::target::MessageResolvableReport>
    {
	MessageReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

        virtual void show(zypp::Message::constPtr message)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_Message) );

	    if ( callback._set )
	    {
		zypp::Patch::constPtr p = message->patch();

		callback.addStr(p->name());
		callback.addStr(p->edition().asString());
		callback.addStr(p->arch().asString());
		callback.addStr(message->text().asString());

		callback.evaluate();
	    }
	}
    };

    struct AuthReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::media::AuthenticationReport>
    {
	AuthReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

	virtual bool prompt(const zypp::Url& url, const std::string& msg, zypp::media::AuthData& auth_data)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_Authentication ) );

	    if (callback._set)
	    {
		callback.addStr(url.asString());
		callback.addStr(msg);
		callback.addStr(auth_data.username());
		callback.addStr(auth_data.password());

		YCPMap cbk(callback.evaluateMap());

		YCPValue ycp_val = cbk->value(YCPString("username"));
		if (!ycp_val.isNull() && ycp_val->isString())
		{
		    // set the entered username
		    auth_data.setUserName(ycp_val->asString()->value());
		}
		else
		{
		    y2error("Invalid/missing value 'username'");
		}

		ycp_val = cbk->value(YCPString("password"));
		if (!ycp_val.isNull() && ycp_val->isString())
		{
		    // set the entered password
		    auth_data.setPassword(ycp_val->asString()->value());
		}
		else
		{
		    y2error("Invalid/missing value 'password'");
		}

		// authentication confirmed?
		bool ret = false;

		ycp_val = cbk->value(YCPString("continue"));
		if (!ycp_val.isNull() && ycp_val->isBoolean())
		{
		    // continue?
		    ret = ycp_val->asBoolean()->value();
		    y2milestone("Use the authentication data: %s", ret ? "true" : "false");
		}
		else
		{
		    y2error("Invalid/missing value 'continue'");
		}

		return ret;
	    }

	    // return the default value from the parent class
	    return zypp::media::AuthenticationReport::prompt(url, msg, auth_data);
	}
    };

    ///////////////////////////////////////////////////////////////////
    // MediaChangeCallback
    ///////////////////////////////////////////////////////////////////
    struct MediaChangeReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::media::MediaChangeReport>
    {
	MediaChangeReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

	virtual Action requestMedia(zypp::Repository source, unsigned mediumNr, zypp::media::MediaChangeReport::Error error, const std::string &description)
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

		// search URL in the redirection map
		std::map<zypp::Repository::NumericId, std::map<unsigned, std::string> >::const_iterator source_it = redirect_map.find(source.numericId());
		bool found = false;
		std::string report_url;

		if (source_it != redirect_map.end())
		{
		    // search medium in the redirection map
		    std::map<unsigned, std::string>::const_iterator media_it = (*source_it).second.find(mediumNr);

		    if (media_it != (*source_it).second.end())
		    {
			// found medium in the source map
			found = true;
			// report the redirected URL
			report_url = (*media_it).second;
			y2milestone("Using redirected URL %s, original URL: %s", report_url.c_str(), source.info().baseUrlsBegin()->asString().c_str());
		    }
		}

		if (!found)
		{
		    // the source has not been redirected
		    // use URL of the source 
		    report_url = source.info().baseUrlsBegin()->asString();
		}

		// current URL
		callback.addStr( report_url );

		// current product name (use the alias, see #214886)
		callback.addStr( source.info().alias() );

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
#warning FIXME: redirection is missing!
		    //source.redirect( mediumNr, ret_url );

		    // remember the redirection		    
		    std::map<unsigned, std::string> source_redir = redirect_map[source.numericId()];
		    source_redir[mediumNr] = ret;
		    redirect_map[source.numericId()] = source_redir;

		    y2milestone("Source redirected to %s", ret.c_str());

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

#warning FIXME SourceCreateReport has been removed
/*
    struct SourceCreateReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::source::SourceCreateReport>
    {
	SourceCreateReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

	virtual void reportbegin()
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceCreateInit ) );
	    y2debug("Source Create begin");

	    if (callback._set)
	    {
		callback.evaluate();
	    }
	}

	virtual void reportend()
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceCreateDestroy ) );
	    y2debug("Source Create destroy");

	    if (callback._set)
	    {
		callback.evaluate();
	    }
	}

	virtual void start( const zypp::Url &url )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceCreateStart ) );

	    if (callback._set)
	    {
		callback.addStr(url);

		callback.evaluate();
	    }
	}

	virtual bool progress( int value )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceCreateProgress ) );

	    if (callback._set)
	    {
		callback.addInt(value);

		return callback.evaluateBool();
	    }

	    return zypp::source::SourceCreateReport::progress(value);
	}

	std::string CreateSrcErrorAsString(zypp::source::SourceCreateReport::Error error)
	{
	    // convert enum to string
	    std::string error_str;

	    switch(error)
	    {
		// no error
		case zypp::source::SourceCreateReport::NO_ERROR	: error_str = "NO_ERROR"; break;
		// the requested Url was not found
		case zypp::source::SourceCreateReport::NOT_FOUND	: error_str = "NOT_FOUND"; break;
		// IO error
		case zypp::source::SourceCreateReport::IO		: error_str = "IO"; break;
		// the source is invalid
		case zypp::source::SourceCreateReport::INVALID	: error_str = "INVALID"; break;
		// rejected
		case zypp::source::SourceCreateReport::REJECTED	: error_str = "REJECTED"; break;
		// unknown error
		case zypp::source::SourceCreateReport::UNKNOWN	: error_str = "UNKNOWN"; break;
	    }

	    return error_str;
	}

	virtual Action problem( const zypp::Url &url, zypp::source::SourceCreateReport::Error error, const std::string &description )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceCreateError ) );

	    if ( callback._set )
	    {
		callback.addStr(url);
		callback.addStr(CreateSrcErrorAsString(error));
		callback.addStr(description);

		std::string result = callback.evaluateSymbol();

		// check the returned symbol
		if ( result == "ABORT" ) return zypp::source::SourceCreateReport::ABORT;
		if ( result == "RETRY" ) return zypp::source::SourceCreateReport::RETRY;

		// still here?
		y2error("Unexpected symbol '%s' returned from callback.", result.c_str());
		// return default
	    }

	    // return the default implementation
	    return zypp::source::SourceCreateReport::problem(url, error, description);
	}

	virtual void finish( const zypp::Url &url, zypp::source::SourceCreateReport::Error error, const std::string &reason )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceCreateEnd ) );

	    if (callback._set)
	    {
		callback.addStr(url);
		callback.addStr(CreateSrcErrorAsString(error));
		callback.addStr(reason);

		callback.evaluate();
	    }
	}
    };
*/

# warning FIXME: ProbeSourceReport has been removeed
    ///////////////////////////////////////////////////////////////////
    // ProbeSourceReport
    ///////////////////////////////////////////////////////////////////
/*    struct ProbeSourceReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::source::ProbeSourceReport>
    {
	ProbeSourceReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

	virtual void start(const zypp::Url &url)
	{
	    _silent_probing = MEDIA_CHANGE_DISABLE;
	    
	    CB callback( ycpcb( YCPCallbacks::CB_SourceProbeStart ) );

	    if (callback._set)
	    {
		callback.addStr(url);

		callback.evaluate();
	    }
	}

	virtual void failedProbe( const zypp::Url &url, const std::string &type )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceProbeFailed ) );

	    if (callback._set)
	    {
		callback.addStr(url);
		callback.addStr(type);

		callback.evaluate();
	    }
	}

	virtual void successProbe( const zypp::Url &url, const std::string &type )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceProbeSucceeded ) );

	    if (callback._set)
	    {
		callback.addStr(url);
		callback.addStr(type);

		callback.evaluate();
	    }
	}

	std::string ProbeSrcErrorAsString(zypp::source::ProbeSourceReport::Error error)
	{
	    // convert enum to string
	    std::string error_str;

	    switch(error)
	    {
		// no error
		case zypp::source::ProbeSourceReport::NO_ERROR	: error_str = "NO_ERROR"; break;
		// the requested Url was not found
		case zypp::source::ProbeSourceReport::NOT_FOUND	: error_str = "NOT_FOUND"; break;
		// IO error
		case zypp::source::ProbeSourceReport::IO	: error_str = "IO"; break;
		// the source is invalid
		case zypp::source::ProbeSourceReport::INVALID	: error_str = "INVALID"; break;
		// unknow error
		case zypp::source::ProbeSourceReport::UNKNOWN	: error_str = "UNKNOWN"; break;
	    }

	    return error_str;
	}

	virtual void finish(const zypp::Url &url, zypp::source::ProbeSourceReport::Error error, const std::string &reason )
	{
	    _silent_probing = MEDIA_CHANGE_FULL;

	    CB callback( ycpcb( YCPCallbacks::CB_SourceProbeEnd ) );

	    if (callback._set)
	    {
		callback.addStr(url);
		callback.addStr(ProbeSrcErrorAsString(error));
		callback.addStr(reason);

		callback.evaluate();
	    }
	}

	virtual bool progress(const zypp::Url &url, int value)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceProbeProgress ) );

	    if (callback._set)
	    {
		callback.addStr(url);
		callback.addInt(value);

		return callback.evaluateBool();
	    }

	    return zypp::source::ProbeSourceReport::progress(url, value);
	    return true;
	}

	virtual zypp::source::ProbeSourceReport::Action problem( const zypp::Url &url, zypp::source::ProbeSourceReport::Error error, const std::string &description )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceProbeError ) );

	    if ( callback._set )
	    {
		callback.addStr(url);
		callback.addStr(ProbeSrcErrorAsString(error));
		callback.addStr(description);

		std::string result = callback.evaluateSymbol();

		// check the returned symbol
		if ( result == "ABORT" ) return zypp::source::ProbeSourceReport::ABORT;
		if ( result == "RETRY" ) return zypp::source::ProbeSourceReport::RETRY;

		// still here?
		y2error("Unexpected symbol '%s' returned from callback.", result.c_str());
		// return default
	    }

	    // return the default value
	    return zypp::source::ProbeSourceReport::problem(url, error, description);
	}
    };
    */

# warning FIXME: SourceReport has been removed
/*
    struct SourceReport : public Recipient, public zypp::callback::ReceiveReport<zypp::source::SourceReport>
    {
	virtual void reportbegin()
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceReportInit ) );
	    y2debug("Source Report begin");

	    if (callback._set)
	    {
		callback.evaluate();
	    }
	}

	virtual void reportend()
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceReportDestroy ) );
	    y2debug("Source Report end");

	    if (callback._set)
	    {
		callback.evaluate();
	    }
	}

	SourceReport( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

	virtual void start( zypp::Source_Ref source, const std::string &task )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceReportStart ) );

	    if (callback._set)
	    {
		callback.addInt(source.numericId());
		callback.addStr(source.url());
		callback.addStr(task);

		callback.evaluate();
	    }
	}

	virtual bool progress( int value )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceReportProgress ) );

	    if (callback._set)
	    {
		callback.addInt(value);

		return callback.evaluateBool();
	    }

	    return zypp::source::SourceReport::progress(value);
	}

	std::string SrcReportErrorAsString(zypp::source::SourceReport::Error error)
	{
	    // convert enum to string
	    std::string error_str;

	    switch(error)
	    {
		// no error
		case zypp::source::SourceReport::NO_ERROR	: error_str = "NO_ERROR"; break;
		// the requested Url was not found
		case zypp::source::SourceReport::NOT_FOUND	: error_str = "NOT_FOUND"; break;
		// IO error
		case zypp::source::SourceReport::IO		: error_str = "IO"; break;
		// the source is invalid
		case zypp::source::SourceReport::INVALID	: error_str = "INVALID"; break;
	    }

	    return error_str;
	}

	virtual zypp::source::SourceReport::Action problem( zypp::Source_Ref source, zypp::source::SourceReport::Error error, const std::string &description )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceReportError ) );
   
	    // the file is optional, ignore the error 
	    if (_silent_probing == ZyppRecipients::MEDIA_CHANGE_OPTIONALFILE)
	    {
		y2milestone("The file is optional, ignoring the error");
		return zypp::source::SourceReport::IGNORE;
	    }

	    if ( callback._set )
	    {
		callback.addInt(source.numericId());
		callback.addStr(source.url());
		callback.addStr(SrcReportErrorAsString(error));
		callback.addStr(description);

		std::string result = callback.evaluateSymbol();

		// check the returned symbol
		if ( result == "ABORT" ) return zypp::source::SourceReport::ABORT;
		if ( result == "RETRY" ) return zypp::source::SourceReport::RETRY;
		if ( result == "IGNORE" ) return zypp::source::SourceReport::IGNORE;

		// still here?
		y2error("Unexpected symbol '%s' returned from callback.", result.c_str());
		// return default
	    }

	    // return the default value
	    return zypp::source::SourceReport::problem(source, error, description);
	}

	virtual void finish( zypp::Source_Ref source, const std::string &task, zypp::source::SourceReport::Error error, const std::string &reason )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceReportEnd ) );

	    if (callback._set)
	    {
		callback.addInt(source.numericId());
		callback.addStr(source.url());
		callback.addStr(task);
		callback.addStr(SrcReportErrorAsString(error));
		callback.addStr(reason);

		callback.evaluate();
	    }
	}
    };
    */


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

	virtual bool askUserToImportKey( const zypp::PublicKey &key)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ImportGpgKey) );

	    if (callback._set)
	    {
		callback.addStr(key.id());
		callback.addStr(key.name());
                callback.addStr(key.fingerprint());

		bool res = callback.evaluateBool();
		y2milestone("Callback ImportGpgKey value: %s", res ? "true" : "false");

		return res;
	    }

	    y2milestone("Callback ImportGpgKey not registered, using default value: %s", zypp::KeyRingReport::askUserToImportKey(key) ? "true" : "false");

	    return zypp::KeyRingReport::askUserToImportKey(key);
	}

	virtual bool askUserToTrustKey(const zypp::PublicKey& key)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_AcceptNonTrustedGpgKey) );

	    if (callback._set)
	    {
		callback.addStr(key.path().asString());
		callback.addStr(key.id());

		callback.addStr(key.name());
                callback.addStr(key.fingerprint());

		bool res = callback.evaluateBool();
		y2milestone("Callback AcceptNonTrustedGpgKey value: %s", res ? "true" : "false");

		return res;
	    }

	    y2milestone("Callback AcceptNonTrustedGpgKey not registered, using default value: %s", zypp::KeyRingReport::askUserToTrustKey(key) ? "true" : "false");

	    return zypp::KeyRingReport::askUserToTrustKey(key);
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

	virtual bool askUserToAcceptUnknownKey(const std::string &file, const std::string &id)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_AcceptUnknownGpgKey) );

	    if (callback._set)
	    {
		callback.addStr(file);
		callback.addStr(id);

		bool res = callback.evaluateBool();
		y2milestone("Callback AcceptUnknownGpgKey value: %s", res ? "true" : "false");

		return res;
	    }

	    y2milestone("Callback AcceptUnknownGpgKey is not registered, using default value: %s", zypp::KeyRingReport::askUserToAcceptUnknownKey(file,id) ? "true" : "false");

	    return zypp::KeyRingReport::askUserToAcceptUnknownKey(file,id);
	}

	virtual bool askUserToAcceptVerificationFailed(const std::string &file, const zypp::PublicKey &key) 
	{
	    CB callback( ycpcb( YCPCallbacks::CB_AcceptVerificationFailed) );

	    if (callback._set)
	    {
		callback.addStr(file);
		callback.addStr(key.id());
		callback.addStr(key.name());
                callback.addStr(key.fingerprint());

		return callback.evaluateBool();
	    }

	    return zypp::KeyRingReport::askUserToAcceptVerificationFailed(file, key);
	}
    };

    ///////////////////////////////////////////////////////////////////
    // KeyRingSignals handler
    ///////////////////////////////////////////////////////////////////
    struct KeyRingSignal : public Recipient, public zypp::callback::ReceiveReport<zypp::KeyRingSignals>
    {
	KeyRingSignal ( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

	virtual void trustedKeyAdded( const zypp::PublicKey &key )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_TrustedKeyAdded) );

	    if (callback._set)
	    {
		callback.addStr(key.id());
		callback.addStr(key.name());
                callback.addStr(key.fingerprint());
	    }
	}

        virtual void trustedKeyRemoved( const zypp::PublicKey &key )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_TrustedKeyRemoved) );

	    if (callback._set)
	    {
		callback.addStr(key.id());
		callback.addStr(key.name());
                callback.addStr(key.fingerprint());
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
    ZyppRecipients::ScanDbReceive  _scanDbReceive;

    // package callbacks
    ZyppRecipients::InstallPkgReceive _installPkgReceive;
    ZyppRecipients::RemovePkgReceive  _removePkgReceive;
    ZyppRecipients::DownloadResolvableReceive _providePkgReceive;

    // media callback
    ZyppRecipients::MediaChangeReceive   _mediaChangeReceive;
    ZyppRecipients::DownloadProgressReceive _downloadProgressReceive;

    // script/messages
    ZyppRecipients::ScriptExecReceive	_scriptExecReceive;
    ZyppRecipients::MessageReceive	_messageReceive;

    // source manager callback
#warning FIXME: source callbacks are missing
/*
    ZyppRecipients::SourceCreateReceive _sourceCreateReceive;
    ZyppRecipients::SourceReport	_sourceReport;
    ZyppRecipients::ProbeSourceReceive _probeSourceReceive;
*/

    // resolvable report
    ZyppRecipients::ResolvableReport _resolvableReport;

    // digest callback
    ZyppRecipients::DigestReceive _digestReceive;

    // key ring callback
    ZyppRecipients::KeyRingReceive _keyRingReceive;

    // key ring signal callback
    ZyppRecipients::KeyRingSignal _keyRingSignal;

    // authentication callback
    ZyppRecipients::AuthReceive _authReceive;

  public:

    ZyppReceive( const YCPCallbacks & ycpcb_r )
      : RecipientCtl( ycpcb_r )
      , _convertDbReceive( *this )
      , _rebuildDbReceive( *this )
      , _scanDbReceive( *this )
      , _installPkgReceive( *this )
      , _removePkgReceive( *this )
      , _providePkgReceive( *this )
      , _mediaChangeReceive( *this )
      , _downloadProgressReceive( *this )
      , _scriptExecReceive( *this )
      , _messageReceive( *this )
#warning FIXME: source callbacks are missing
//      , _sourceCreateReceive( *this )
//      , _sourceReport( *this )
//      , _probeSourceReceive( *this )
      , _resolvableReport( *this )
      , _digestReceive( *this )
      , _keyRingReceive( *this )
      , _keyRingSignal( *this )
      , _authReceive( *this )
    {
	// connect the receivers
	_convertDbReceive.connect();
	_rebuildDbReceive.connect();
	_scanDbReceive.connect();
	_installPkgReceive.connect();
	_removePkgReceive.connect();
	_providePkgReceive.connect();
	_mediaChangeReceive.connect();
	_downloadProgressReceive.connect();
	_scriptExecReceive.connect();
	_messageReceive.connect();
#warning FIXME: source callbacks are missing
//	_sourceCreateReceive.connect();
//	_sourceReport.connect();
//	_probeSourceReceive.connect();
	_resolvableReport.connect();
 	_digestReceive.connect();
	_keyRingReceive.connect();
	_keyRingSignal.connect();
	_authReceive.connect();
    }

    virtual ~ZyppReceive()
    {
	// disconnect the receivers
	_convertDbReceive.disconnect();
	_rebuildDbReceive.disconnect();
	_scanDbReceive.disconnect();
	_installPkgReceive.disconnect();
	_removePkgReceive.disconnect();
	_providePkgReceive.disconnect();
	_mediaChangeReceive.disconnect();
	_downloadProgressReceive.disconnect();
	_scriptExecReceive.disconnect();
	_messageReceive.disconnect();
#warning FIXME: source callbacks are missing
//	_sourceCreateReceive.disconnect();
//	_sourceReport.disconnect();
//	_probeSourceReceive.disconnect();
	_resolvableReport.disconnect();
	_digestReceive.disconnect();
	_keyRingReceive.disconnect();
	_keyRingSignal.disconnect();
	_authReceive.disconnect();
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
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(string filename, string keyid)</code>. The callback function should ask user whether the unknown key can be accepted, returned true value means to accept the key. 
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackAcceptUnknownGpgKey( const YCPString& args ) {
  return SET_YCP_CB( CB_AcceptUnknownGpgKey, args );
}

/**
 * @builtin CallbackAcceptNonTrustedGpgKey
 * @short Register callback function
 * @param string args Name of the callback handler function. Required callback prototype is <code>boolean(string filename, string keyid, string keyname, string fingerprint)</code>. The callback function should ask user whether the unknown key can be accepted, returned true value means to accept the file.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackAcceptNonTrustedGpgKey( const YCPString& args ) {
  return SET_YCP_CB( CB_AcceptNonTrustedGpgKey, args );
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
  y2warning("Pkg::CallbackYouProgress is obsoleted, do not use it (empty implementation)!");
  return YCPVoid();
}

YCPValue PkgModuleFunctions::CallbackYouPatchProgress( const YCPString& args ) {
  y2warning("Pkg::CallbackYouPatchProgress is obsoleted, do not use it (empty implementation)!");
  return YCPVoid();
}

YCPValue PkgModuleFunctions::CallbackYouError( const YCPString& args ) {
  y2warning("Pkg::CallbackYouError is obsoleted, do not use it (empty implementation)!");
  return YCPVoid();
}

YCPValue PkgModuleFunctions::CallbackYouMessage( const YCPString& args ) {
  y2warning("Pkg::CallbackYouMessage is obsoleted, do not use it (empty implementation)!");
  return YCPVoid();
}

YCPValue PkgModuleFunctions::CallbackYouLog( const YCPString& args ) {
  y2warning("Pkg::CallbackYouLog is obsoleted, do not use it (empty implementation)!");
  return YCPVoid();
}

YCPValue PkgModuleFunctions::CallbackYouExecuteYcpScript( const YCPString& args ) {
  y2warning("Pkg::CallbackYouExecuteYcpScript is obsoleted, do not use it (empty implementation)!");
  return YCPVoid();
}
YCPValue PkgModuleFunctions::CallbackYouScriptProgress( const YCPString& args ) {
  y2warning("Pkg::CallbackYouScriptProgress is obsoleted, do not use it (empty implementation)!");
  return YCPVoid();
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



/**
 * @builtin CallbackStartScanDb
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void()</code>. The callback function is evaluated when the RPM DB reading has been started.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackStartScanDb( const YCPString& args ) {
  return SET_YCP_CB( CB_StartScanDb, args );
}
/**
 * @builtin CallbackProgressScanDb
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>boolean(integer percent)</code>. The callback function is evaluated during RPM DB reading.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackProgressScanDb( const YCPString& args ) {
  return SET_YCP_CB( CB_ProgressScanDb, args );
}
/**
 * @builtin CallbackErrorScanDb
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void(integer error_code, string description)</code>. The callback function is evaluated when an error occurrs during RPM DB reading. error_code 0 means no error.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackErrorScanDb( const YCPString& args ) {
  return SET_YCP_CB( CB_ErrorScanDb, args );
}
/**
 * @builtin CallbackDoneScanDb
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void(integer error_code, string description)</code>. The callback function is evaluated when RPM DB reading is finished. error_code 0 means no error.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackDoneScanDb( const YCPString& args ) {
  return SET_YCP_CB( CB_DoneScanDb, args );
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


/**
 * @builtin CallbackStartDeltaDownload
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void(string filename, integer download_size)</code>. If the download size is unknown download_size is 0. The callback function is evaluated when a delta RPM download has been started.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackStartDeltaDownload( const YCPString& func ) {
  return SET_YCP_CB( CB_StartDeltaDownload, func );
}

/**
 * @builtin CallbackProgressDeltaDownload
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>boolean (integer value)</code>. The callback function is evaluated when more than 5% of the size has been downloaded since the last evaluation. If the handler returns false the download is aborted.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackProgressDeltaDownload( const YCPString& func ) {
  return SET_YCP_CB( CB_ProgressDeltaDownload, func );
}

/**
 * @builtin CallbackProblemDeltaDownload
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void(string description)</code>. The callback function should inform user that a problem has occurred during delta file download. This is not fatal, it still may be possible to download the full RPM instead.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackProblemDeltaDownload( const YCPString& func ) {
  return SET_YCP_CB( CB_ProblemDeltaDownload, func );
}

/**
 * @builtin CallbackStartDeltaApply
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void(string filename)</code>. The callback function should inform user that a delta application has been started.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackStartDeltaApply( const YCPString& func ) {
  return SET_YCP_CB( CB_StartDeltaApply, func );
}

/**
 * @builtin CallbackProgressDeltaApply
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void(integer value)</code>. The callback function is evaluated when more than 5% of the delta size has been applied since the last evaluation.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackProgressDeltaApply( const YCPString& func ) {
  return SET_YCP_CB( CB_ProgressDeltaApply, func );
}

/**
 * @builtin CallbackProblemDeltaApply
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void(string description)</code>. The callback function should inform user that a problem has occurred during delta file application. This is not fatal, it still may be possible to use the full RPM instead.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackProblemDeltaApply( const YCPString& func ) {
  return SET_YCP_CB( CB_ProblemDeltaApply, func );
}

/**
 * @builtin CallbackStartPatchDownload
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void(string filename, integer download_size)</code>. If the download size is unknown download_size is 0. The callback function is evaluated when a patch download has been started.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackStartPatchDownload( const YCPString& func ) {
  return SET_YCP_CB( CB_StartPatchDownload, func );
}

/**
 * @builtin CallbackProgressPatchDownload
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>boolean(integer value)</code>. The callback function is evaluated when more than 5% of the patch size has been downloaded since the last evaluation. If the handler returns false the download is aborted.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackProgressPatchDownload( const YCPString& func ) {
  return SET_YCP_CB( CB_ProgressPatchDownload, func );
}

/**
 * @builtin CallbackProblemPatchDownload
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void(string description)</code>. The callback function should inform user that a problem has occurred during download of the patch.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackProblemPatchDownload( const YCPString& func ) {
  return SET_YCP_CB( CB_ProblemPatchDownload, func );
}


/**
 * @builtin CallbackFinishDeltaDownload
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void()</code>. The callback function is evaluated when the delta download has been finished.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackFinishDeltaDownload( const YCPString& func)
{
    return SET_YCP_CB( CB_FinishDeltaDownload, func );
}

/**
 * @builtin CallbackFinishDeltaApply
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void()</code>. The callback function is evaluated when the delta download has been applied.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackFinishDeltaApply( const YCPString& func)
{
    return SET_YCP_CB( CB_FinishDeltaApply, func );
}

/**
 * @builtin CallbackFinishPatchDownload
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void()</code>. The callback function is evaluated when the patch download has been finished.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackFinishPatchDownload( const YCPString& func)
{
    return SET_YCP_CB( CB_FinishPatchDownload, func );
}


/**
 * @builtin CallbackSourceCreateStart
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void(string url)</code>. The callback is evaluated when a source creation has been started.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackSourceCreateStart( const YCPString& func)
{
    return SET_YCP_CB( CB_SourceCreateStart, func );
}


/**
 * @builtin CallbackSourceProgressData
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>boolean(integer value)</code>. The callback function is evaluated when more than 5% of the data has been processed since the last evaluation. If the handler returns false the download is aborted.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackSourceCreateProgress( const YCPString& func)
{
    return SET_YCP_CB( CB_SourceCreateProgress, func );
}

/**
 * @builtin CallbackSourceCreateError
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>string(string url, string err_code, string description)</code>. err_code is "NO_ERROR", "NOT_FOUND" (the URL was not found), "IO" (I/O error) or "INVALID" (the source is not valid). The callback function must return "ABORT" or "RETRY". The callback function is evaluated when an error occurrs during creation of the source.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackSourceCreateError( const YCPString& func)
{
    return SET_YCP_CB( CB_SourceCreateError, func );
}

/**
 * @builtin CallbackSourceCreateEnd
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void(string url, string err_code, string description)</code>. err_code is "NO_ERROR", "NOT_FOUND" (the URL was not found), "IO" (I/O error) or "INVALID" (the source is not valid). The callback function is evaluated when creation of the source has been finished.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackSourceCreateEnd( const YCPString& func)
{
    return SET_YCP_CB( CB_SourceCreateEnd, func );
}




/**
 * @builtin CallbackSourceProbeStart
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void(string url)</code>. The callback function is evaluated when source probing has been started.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackSourceProbeStart( const YCPString& func)
{
    return SET_YCP_CB( CB_SourceProbeStart, func );
}

/**
 * @builtin CallbackSourceProbeFailed
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void(string url, string type)</code>. The callback function is evaluated when the probed source has different type.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackSourceProbeFailed( const YCPString& func)
{
    return SET_YCP_CB( CB_SourceProbeFailed, func );
}

/**
 * @builtin CallbackSourceProbeSucceeded
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void(string url, string type)</code>. The callback function is evaluated when the probed source has type <code>type</code>.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackSourceProbeSucceeded( const YCPString& func)
{
    return SET_YCP_CB( CB_SourceProbeSucceeded, func );
}

/**
 * @builtin CallbackSourceProbeEnd
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void(string url, string error, string reason)</code>. The callback function is evaluated when source probing has been finished.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackSourceProbeEnd( const YCPString& func)
{
    return SET_YCP_CB( CB_SourceProbeEnd, func );
}

/**
 * @builtin CallbackSourceProbeProgress
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>boolean(integer value)</code>. If the handler returns false the refresh is aborted.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackSourceProbeProgress( const YCPString& func)
{
    return SET_YCP_CB( CB_SourceProbeProgress, func );
}

/**
 * @builtin CallbackSourceProbeError
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>string(string url, string error, string reason)</code>. The callback function is evaluated when an error occurrs. The callback function must return string "ABORT" or "RETRY".
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackSourceProbeError( const YCPString& func)
{
    return SET_YCP_CB( CB_SourceProbeError, func );
}


YCPValue PkgModuleFunctions::CallbackSourceReportInit( const YCPString& func)
{
    return SET_YCP_CB( CB_SourceReportInit, func );
}

YCPValue PkgModuleFunctions::CallbackSourceReportDestroy( const YCPString& func)
{
    return SET_YCP_CB( CB_SourceReportDestroy, func );
}

YCPValue PkgModuleFunctions::CallbackSourceCreateInit( const YCPString& func)
{
    return SET_YCP_CB( CB_SourceCreateInit, func );
}

YCPValue PkgModuleFunctions::CallbackSourceCreateDestroy( const YCPString& func)
{
    return SET_YCP_CB( CB_SourceCreateDestroy, func );
}

/**
 * @builtin CallbackSourceProbeStart
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void(integer source_id, string url, string task)</code>.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackSourceReportStart( const YCPString& func)
{
    return SET_YCP_CB( CB_SourceReportStart, func );
}

/**
 * @builtin CallbackSourceReportProgress
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>boolean(integer value)</code>. If the handler returns false the task is aborted.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackSourceReportProgress( const YCPString& func)
{
    return SET_YCP_CB( CB_SourceReportProgress, func );
}

/**
 * @builtin CallbackSourceReportError
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>string(integer numeric_id, string url, string error, string reason)</code>. Parameter error is "NO_ERROR", "NOT_FOUND", "IO" or "INVALID". The callback function is evaluated when an error occurrs. The callback function must return string "ABORT", "IGNORE" or "RETRY".
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackSourceReportError( const YCPString& func)
{
    return SET_YCP_CB( CB_SourceReportError, func );
}

/**
 * @builtin CallbackSourceReportEnd
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void(integer numeric_id, string url, string task, string error, string reason)</code>. Parameter error is "NO_ERROR", "NOT_FOUND", "IO" or "INVALID". The callback function is evaluated when an error occurrs. The callback function must return string "ABORT", "IGNORE" or "RETRY".
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackSourceReportEnd( const YCPString& func)
{
    return SET_YCP_CB( CB_SourceReportEnd, func );
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


/**
 * @builtin CallbackScriptStart
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void(string patch_name, string patch_version, string patch_arch, string script_path, boolean installation)</code>. Parameter 'installation' is true when the script is called during installation of a patch, false means patch removal. The callback function is evaluated when a script (which is part of a patch) has been started.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackScriptStart( const YCPString& args ) {
    return SET_YCP_CB( CB_ScriptStart, args );
}
/**
 * @builtin CallbackScriptProgress
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void(boolean ping, string output)</code>. If parameter 'ping' is true than there is no output available, but the script is still running (This functionality enables aborting the script). If it is false, 'output' contains (part of) the script output. The callback function is evaluated when a script is running.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackScriptProgress( const YCPString& args ) {
    return SET_YCP_CB( CB_ScriptProgress, args );
}
/**
 * @builtin CallbackScriptProblem
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void(string description)</code>. The callback function is evaluated when an error occurrs.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackScriptProblem( const YCPString& args ) {
    return SET_YCP_CB( CB_ScriptProblem, args );
}
/**
 * @builtin CallbackScriptFinish
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void()</code>. The callback function is evaluated when the script has been finished.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackScriptFinish( const YCPString& args ) {
    return SET_YCP_CB( CB_ScriptFinish, args );
}
/**
 * @builtin CallbackMessage
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>void(string patch_name, string patch_version, string patch_arch, string message)</code>. The callback function is evaluated when a message which is part of a patch should be displayed.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackMessage( const YCPString& args ) {
    return SET_YCP_CB( CB_Message, args );
}

/**
 * @builtin CallbackAuthentication
 * @short Register callback function
 * @param string func Name of the callback handler function. Required callback prototype is <code>map(string url, string message, string username, string password)</code>. The returned map must contain these items: $[ "username" : string, "password" : string, "continue" : boolean ]. If <code>"continue"</code> value is false or is missing the authentification (and the download process) is canceled. The callback function is evaluated when user authentication is required to download the requested file.
 * @return void
 */
YCPValue PkgModuleFunctions::CallbackAuthentication( const YCPString& func ) {
    return SET_YCP_CB( CB_Authentication, func );
}

#undef SET_YCP_CB
