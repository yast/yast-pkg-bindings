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

#include "PkgFunctions.h"
#include "log.h"
#include "Callbacks.h"
#include "Callbacks.YCP.h" // PkgFunctions::CallbackHandler::YCPCallbacks
#include "GPGMap.h"

#include "zypp/ZYppCallbacks.h"
#include "zypp/Package.h"
#include "zypp/Patch.h"
#include "zypp/KeyRing.h"
#include "zypp/PublicKey.h"
#include "zypp/Digest.h"
#include "zypp/base/String.h"
#include "zypp/sat/FileConflicts.h"

#include <ctime>

// FIXME: do this nicer, source create use this to avoid user feedback
// on probing of source type

ZyppRecipients::MediaChangeSensitivity _silent_probing = ZyppRecipients::MEDIA_CHANGE_FULL;

// remember redirected URLs
// FIXME huh?

typedef std::map<unsigned, zypp::Url> MediaMap;
typedef std::map<zypp::Url, MediaMap> RedirectMap;

RedirectMap redirect_map;

// default timeout for callbacks, evaluate the callbacks after 3 seconds
// even if the progress percent has not been changed
static const time_t callback_timeout = 3;

///////////////////////////////////////////////////////////////////
namespace ZyppRecipients {
///////////////////////////////////////////////////////////////////

  typedef PkgFunctions::CallbackHandler::YCPCallbacks YCPCallbacks;

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
		callback.addStr(pth.asString());
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
		callback.evaluate();
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
	PkgFunctions &_pkg_ref;
	int last_reported;
	time_t last_reported_time;

	InstallPkgReceive(RecipientCtl & construct_r, PkgFunctions &pk) : Recipient(construct_r), _last(NULL), _pkg_ref(pk)
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
	  last_reported_time = time(NULL);

#warning install non-package
	  zypp::Package::constPtr res =
	    zypp::asKind<zypp::Package>(resolvable);

	  // if we have started this resolvable already, don't do it again
	  if( _last == resolvable )
	    return;

	  // convert the repo ID
	  PkgFunctions::RepoId source_id = _pkg_ref.logFindAlias(res->repoInfo().alias());
	  int media_nr = res->mediaNr();

	  if( source_id != _pkg_ref.LastReportedRepo() || media_nr != _pkg_ref.LastReportedMedium())
	  {
	    CB callback( ycpcb( YCPCallbacks::CB_SourceChange ) );
	    if (callback._set) {
	        callback.addInt( source_id );
	        callback.addInt( media_nr );
	        callback.evaluate();
	    }

	    _pkg_ref.SetReportedSource(source_id, media_nr);
          }

	  CB callback( ycpcb( YCPCallbacks::CB_StartPackage ) );
	  if (callback._set) {
	    callback.addStr(res->name());
	    callback.addStr(res->location().filename());
	    callback.addStr(res->summary());
	    callback.addInt(res->installSize());
	    callback.addBool(false);	// is_delete = false (package installation)
	    callback.evaluate();
	  }

	  _last = resolvable;
	}

	virtual bool progress(int value, zypp::Resolvable::constPtr resolvable)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ProgressPackage) );
	    // call the callback function only if the difference since the last call is at least 5%
	    // or if 100% is reached or at least 3 seconds have elapsed
	    time_t current_time = time(NULL);
	    if (callback._set && (value - last_reported >= 5 || last_reported - value >= 5 || value == 100 || current_time - last_reported_time >= callback_timeout))
	    {
		callback.addInt( value );
		bool res = callback.evaluateBool();

		if( !res )
		    y2milestone( "Package installation callback returned abort" );

		last_reported = value;
		last_reported_time = current_time;
		return res;
	    }

	    // return default value from the parent class
	    return zypp::target::rpm::InstallResolvableReport::progress(value, resolvable);
	}

        virtual Action problem(
          zypp::Resolvable::constPtr resolvable
          , zypp::target::rpm::InstallResolvableReport::Error error
          , const std::string &description
          // note: the RpmLevel argument is not used anymore, ignore it
          , zypp::target::rpm::InstallResolvableReport::RpmLevel level
        )
	{
	    _last = zypp::Resolvable::constPtr();

            // for historical reasons the DonePackage callback is used for error reporting
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

        // note: the RpmLevel argument is not used anymore, ignore it
	virtual void finish(zypp::Resolvable::constPtr resolvable, Error error, const std::string &reason, zypp::target::rpm::InstallResolvableReport::RpmLevel /*level*/)
	{
            // errors are handled in the problem() callback above, here just log the message
            if (error != zypp::target::rpm::InstallResolvableReport::NO_ERROR)
            {
                y2milestone("Error in finish callback: %s", reason.c_str());
            }

            CB callback( ycpcb( YCPCallbacks::CB_DonePackage) );
            if (callback._set) {
                // report no error, errors were already reported in problem() callback above,
                // just report real "done" status
                callback.addInt(NO_ERROR);
                callback.addStr(std::string(""));

                // return value ignored
                callback.evaluateStr();
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
	    callback.addStr(std::string());
	    callback.addInt(-1);
	    callback.addBool(true);	// is_delete = true
	    callback.evaluate();
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

        virtual Action problem(
          zypp::Resolvable::constPtr resolvable
          , zypp::target::rpm::RemoveResolvableReport::Error error
          , const std::string &description
        )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_DonePackage) );
	    if (callback._set) {
		callback.addInt( error );
		callback.addStr( description );

                std::string ret = callback.evaluateStr();

                // "R" =  retry
                if (ret == "R") return zypp::target::rpm::RemoveResolvableReport::RETRY;

                // "C" = cancel
                if (ret == "C") return zypp::target::rpm::RemoveResolvableReport::ABORT;

                // otherwise ignore
                return zypp::target::rpm::RemoveResolvableReport::IGNORE;
	    }

	    return zypp::target::rpm::RemoveResolvableReport::problem
		(resolvable, error, description);
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


    struct ProgressReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::ProgressReport>
    {
	ProgressReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

	virtual void start(const zypp::ProgressData &task)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ProgressStart ) );
	    y2debug("ProgressStart: id:%d, %s", task.numericId(), task.name().c_str());

	    if (callback._set)
	    {
		callback.addInt( task.numericId() );
		callback.addStr( task.name() );
		callback.addBool( task.reportPercent() );
		callback.addBool( task.reportAlive() );
		callback.addInt( task.min() );
		callback.addInt( task.max() );
		callback.addInt( task.val() );
		callback.addInt( task.reportValue() );
		callback.evaluate();
	    }
	}

	virtual bool progress(const zypp::ProgressData &task)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ProgressProgress ) );
	    y2debug("ProgressProgress: id:%d, %s: %lld%%", task.numericId(), task.name().c_str(), task.reportValue());

	    if (callback._set)
	    {
		callback.addInt( task.numericId() );
		callback.addInt( task.val() );
		callback.addInt( task.reportValue() );
		return callback.evaluateBool();
	    }

	    return zypp::ProgressReport::progress(task);
	}

	virtual void finish( const zypp::ProgressData &task )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ProgressDone ) );
	    y2debug("ProgressFinish: id:%d, %s", task.numericId(), task.name().c_str());

	    if (callback._set)
	    {
		callback.addInt( task.numericId() );
		callback.evaluate();
	    }
	}
    };



    ///////////////////////////////////////////////////////////////////
    // DownloadResolvableCallback
    ///////////////////////////////////////////////////////////////////
    struct DownloadResolvableReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::repo::DownloadResolvableReport>
    {
	PkgFunctions &_pkg_ref;

	DownloadResolvableReceive( RecipientCtl & construct_r, PkgFunctions &pk ) : Recipient( construct_r ), _pkg_ref(pk) {}
	int last_reported;
	time_t last_reported_time;

	int last_reported_delta_download;
	time_t last_reported_delta_download_time;

	int last_reported_delta_apply;
	time_t last_reported_delta_apply_time;

	int last_reported_patch_download;
	time_t last_reported_patch_download_time;

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
	  last_reported_time = time(NULL);

	  if ( zypp::isKind<zypp::Package> (resolvable_ptr) )
	  {
	    zypp::Package::constPtr pkg =
		zypp::asKind<zypp::Package>(resolvable_ptr);

	    size = pkg->downloadSize();

	    // convert the repo ID
	    PkgFunctions::RepoId source_id = _pkg_ref.logFindAlias(pkg->repoInfo().alias());
	    int media_nr = pkg->mediaNr();

	    if( source_id != _pkg_ref.LastReportedRepo() || media_nr != _pkg_ref.LastReportedMedium())
	    {
	      CB callback( ycpcb( YCPCallbacks::CB_SourceChange ) );
	      if (callback._set) {
	        callback.addInt( source_id );
	        callback.addInt( media_nr );
	        callback.evaluate();
	      }
	      _pkg_ref.SetReportedSource(source_id, media_nr);
            }
	  }

	  CB callback( ycpcb( YCPCallbacks::CB_StartProvide ) );
	  if (callback._set) {
	    std::string scheme = zypp::str::toLower(url.getScheme());

	    bool remote = scheme != "cd" && scheme != "dvd" && scheme != "nfs" && scheme != "dir" && scheme != "file";

	    callback.addStr(resolvable_ptr->name());
	    callback.addInt( size );
	    callback.addBool(remote);
	    callback.evaluate();
	  }
	}

	virtual void finish(zypp::Resolvable::constPtr resolvable, zypp::repo::DownloadResolvableReport::Error error, const std::string &reason)
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
	    time_t current_time = time(NULL);
	    if (callback._set && (value - last_reported >= 5 || last_reported - value >= 5 || value == 100 || current_time - last_reported_time >= callback_timeout))
	    {
		last_reported = value;
		last_reported_time = current_time;
		callback.addInt( value );
		return callback.evaluateBool(); // return value ignored by RpmDb
	    }

	    return zypp::repo::DownloadResolvableReport::progress(value, resolvable_ptr);
	}

	virtual Action problem(zypp::Resolvable::constPtr resolvable_ptr, zypp::repo::DownloadResolvableReport::Error error, const std::string &description)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_DoneProvide) );
	    if (callback._set) {
		callback.addInt( error );
		callback.addStr( description );
		callback.addStr( resolvable_ptr->name() );
                std::string ret = callback.evaluateStr();

                // "R" =  retry
                if (ret == "R") return zypp::repo::DownloadResolvableReport::RETRY;

                // "C" = cancel
                if (ret == "C") return zypp::repo::DownloadResolvableReport::ABORT;

                // "I" = ignore
                if (ret == "I") return zypp::repo::DownloadResolvableReport::IGNORE;

                // otherwise return the default value from the parent class
	    }

            // return the default value from the parent class
	    return zypp::repo::DownloadResolvableReport::problem(resolvable_ptr, error, description);
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
	    last_reported_delta_download_time = time(NULL);

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
	    time_t current_time = time(NULL);
	    if (callback._set && (value - last_reported_delta_download >= 5 || last_reported_delta_download - value >= 5 || value == 100 || current_time - last_reported_delta_download_time >= callback_timeout))
	    {
		last_reported_delta_download = value;
		last_reported_delta_download_time = current_time;
		callback.addInt( value );

		return callback.evaluateBool();
	    }

	    return zypp::repo::DownloadResolvableReport::progressDeltaDownload(value);
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
	    last_reported_delta_apply_time = time(NULL);

	    CB callback( ycpcb( YCPCallbacks::CB_StartDeltaApply) );
	    if (callback._set) {
		callback.addStr( filename.asString() );

		callback.evaluate();
	    }
	}

	virtual void progressDeltaApply( int value )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ProgressDeltaApply ) );
	    time_t current_time = time(NULL);
	    if (callback._set && (value - last_reported_delta_apply >= 5 || last_reported_delta_apply - value >= 5 || value == 100 || current_time - last_reported_delta_apply_time >= callback_timeout))
	    {
		last_reported_delta_apply = value;
		last_reported_delta_apply_time = current_time;
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
    };

    ///////////////////////////////////////////////////////////////////
    // DownloadProgressReceive
    ///////////////////////////////////////////////////////////////////
    struct DownloadProgressReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::media::DownloadProgressReport>
    {
	int last_reported;
	time_t last_reported_time;

	DownloadProgressReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

        virtual void start( const zypp::Url &file, zypp::Pathname localfile )
	{
	    last_reported = 0;
	    last_reported_time = time(NULL);
	    CB callback( ycpcb( YCPCallbacks::CB_StartDownload ) );

	    if ( callback._set )
	    {
		callback.addStr( file.asString() );
		callback.addStr( localfile.asString() );
		callback.evaluate();
	    }
	}

        virtual bool progress(int value, const zypp::Url &file, double bps_avg, double bps_current)
        {
	    CB callback( ycpcb( YCPCallbacks::CB_ProgressDownload ) );
	    // call the callback function only if the difference since the last call is at least 5%
	    // or if 100% is reached or if at least 3 seconds have elapsed
	    time_t current_time = time(NULL);
	    if (callback._set && (value - last_reported >= 5 || last_reported - value >= 5 || value == 100 || current_time - last_reported_time >= callback_timeout))
	    {
		last_reported = value;
		last_reported_time = current_time;
		// report changed values
		callback.addInt( value );
		callback.addInt( (long long) bps_avg  );
		callback.addInt( (long long) bps_current  );
		return callback.evaluateBool( true ); // default == continue
	    }

	    return zypp::media::DownloadProgressReport::progress(value, file, bps_avg, bps_current);
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

	    zypp::media::DownloadProgressReport::Error err = error;

	    // ignore errors for optional files
	    if ( _silent_probing == MEDIA_CHANGE_DISABLE ||
		_silent_probing == MEDIA_CHANGE_OPTIONALFILE)
	    {
		err = zypp::media::DownloadProgressReport::NO_ERROR;
	    }

	    if ( callback._set ) {
		callback.addInt( err );
		callback.addStr( reason );
		callback.evaluate();
	    }
	}
    };


    ///////////////////////////////////////////////////////////////////
    // ScriptExecCallbacks
    ///////////////////////////////////////////////////////////////////
    struct ScriptExecReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::target::PatchScriptReport>
    {
	ScriptExecReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

	virtual void start( const zypp::Package::constPtr &pkg, const zypp::Pathname &path_r)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ScriptStart) );
	    if ( callback._set )
	    {
		callback.addStr(pkg->name());
		callback.addStr(pkg->edition().asString());
		callback.addStr(pkg->arch().asString());
		callback.addStr(path_r);

		callback.evaluate();
	    }
	}

	virtual bool progress( zypp::target::PatchScriptReport::Notify ping, const std::string &out = std::string() )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ScriptProgress) );

	    if ( callback._set )
	    {
		callback.addBool(ping == zypp::target::PatchScriptReport::PING);
		callback.addStr(out);

		// false = abort the script
		return callback.evaluateBool();
	    }
	    else
	    {
		// return the default implementation
		return zypp::target::PatchScriptReport::progress(ping, out);
	    }
	}

	virtual zypp::target::PatchScriptReport::Action problem( const std::string &description )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ScriptProblem) );

	    if ( callback._set )
	    {
		callback.addStr(description);

		std::string ret = callback.evaluateStr();

		// "A" =  abort
		if (ret == "A") return zypp::target::PatchScriptReport::ABORT;

		// "I" = ignore
		if (ret == "I") return zypp::target::PatchScriptReport::IGNORE;

		// "R" = retry
		if (ret == "R") return zypp::target::PatchScriptReport::RETRY;

		y2error("Unknown return value: %s", ret.c_str());
	    }

	    // return the defaulf when the callback is not registered
	    // or the returned value is unknown
	    return zypp::target::PatchScriptReport::problem(description);
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

    struct MessageReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::target::PatchMessageReport>
    {
	MessageReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

        virtual bool show(zypp::Patch::constPtr &p)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_Message) );

	    if ( callback._set )
	    {
		callback.addStr(p->name());
		callback.addStr(p->edition().asString());
		callback.addStr(p->arch().asString());
		callback.addStr(p->message(zypp::ZConfig::instance().textLocale()));

		return callback.evaluateBool();
	    }

	    // return the default
	    return zypp::target::PatchMessageReport::show(p);
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
		    auth_data.setUsername(ycp_val->asString()->value());
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

	std::string MediaChangeErrorAsString(zypp::media::MediaChangeReport::Error error)
	{
	    // convert enum to a string
	    std::string error_str;

	    switch(error)
	    {
		// no error
		case zypp::media::MediaChangeReport::NO_ERROR	: error_str = "NO_ERROR"; break;
		case zypp::media::MediaChangeReport::NOT_FOUND	: error_str = "NOT_FOUND"; break;
		case zypp::media::MediaChangeReport::IO		: error_str = "IO"; break;
		case zypp::media::MediaChangeReport::INVALID	: error_str = "INVALID"; break;
		case zypp::media::MediaChangeReport::WRONG	: error_str = "WRONG"; break;
		case zypp::media::MediaChangeReport::IO_SOFT	: error_str = "IO_SOFT"; break;
	    }

	    return error_str;
	}

	virtual Action requestMedia(zypp::Url &url,
                                    unsigned int mediumNr,
                                    const std::string & label,
                                    zypp::media::MediaChangeReport::Error error,
                                    const std::string &description,
                                    const std::vector<std::string> & devices,
                                    unsigned int &dev_current)
	{
	    if ( _silent_probing == MEDIA_CHANGE_DISABLE )
		return zypp::media::MediaChangeReport::ABORT;

	    if ( _silent_probing == MEDIA_CHANGE_OPTIONALFILE
	      && error == zypp::media::MediaChangeReport::NOT_FOUND )
	        return zypp::media::MediaChangeReport::ABORT;

	    CB callback( ycpcb( YCPCallbacks::CB_MediaChange ) );
	    if ( callback._set )
	    {
		// error code
		callback.addStr( MediaChangeErrorAsString(error) );

		// error message
		callback.addStr( description );

		// search URL in the redirection map
		RedirectMap::const_iterator source_it = redirect_map.find(url);
		bool found = false;
		zypp::Url report_url;

		if (source_it != redirect_map.end())
		{
		    // search medium in the redirection map
		    MediaMap::const_iterator media_it = (*source_it).second.find(mediumNr);

		    if (media_it != (*source_it).second.end())
		    {
			// found medium in the source map
			found = true;
			// report the redirected URL
			report_url = (*media_it).second;

			y2milestone("Using redirected URL %s, original URL: %s", report_url.asString().c_str(), url.asString().c_str());
		    }
		}

		if (!found)
		{
		    // the source has not been redirected
		    // use URL of the source
		    report_url = url;
		}

		// current URL
		callback.addStr( report_url.asString() );

		// repo alias (see bnc#330094)
		callback.addStr( label );

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

		// add device names
		YCPList device_names;
	        for_(iter, devices.begin(), devices.end())
		{
		    device_names->add(YCPString(*iter));
		}
		callback.addList(device_names);

		// add current device index
		callback.addInt(dev_current);

		std::string ret = callback.evaluateStr();

		// "" =  retry
		if (ret == "") return zypp::media::MediaChangeReport::RETRY;

		// "I" = ignore wrong media ID
		if (ret == "I") return zypp::media::MediaChangeReport::IGNORE_ID;

		// "C" = cancel
		if (ret == "C") return zypp::media::MediaChangeReport::ABORT;

		// "E" = eject media
		if (ret == "E") return zypp::media::MediaChangeReport::EJECT;

		// "E" + numbure = eject the required device
		if (ret.size() > 1 && ret[0] == 'E')
		{
		    // change the device
		    dev_current = zypp::str::strtonum<unsigned int>(ret.c_str() + 1);
		    y2milestone("Ejecting device %d", dev_current);
		    return zypp::media::MediaChangeReport::EJECT;
		}

		// "S" = skip (ignore) this media
		if (ret == "S") return zypp::media::MediaChangeReport::IGNORE;

		// otherwise change media URL
		// try/catch to catch invalid URLs
		try {
		    // set the new URL
		    url = zypp::Url(ret);

		    // remember the redirection
		    MediaMap source_redir = redirect_map[url];
		    source_redir[mediumNr] = url;
		    redirect_map[url] = source_redir;

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
	    return zypp::media::MediaChangeReport::requestMedia(url, mediumNr, label, error, description, devices, dev_current);
	}
    };

    struct SourceCreateReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::repo::RepoCreateReport>
    {
	SourceCreateReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

	virtual void reportbegin()
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceCreateInit ) );
	    y2debug("Repo Create begin");

	    if (callback._set)
	    {
		callback.evaluate();
	    }
	}

	virtual void reportend()
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceCreateDestroy ) );
	    y2debug("Repo Create destroy");

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

	    return zypp::repo::RepoCreateReport::progress(value);
	}

	std::string CreateSrcErrorAsString(zypp::repo::RepoCreateReport::Error error)
	{
	    // convert enum to string
	    std::string error_str;

	    switch(error)
	    {
		// no error
		case zypp::repo::RepoCreateReport::NO_ERROR	: error_str = "NO_ERROR"; break;
		// the requested Url was not found
		case zypp::repo::RepoCreateReport::NOT_FOUND	: error_str = "NOT_FOUND"; break;
		// IO error
		case zypp::repo::RepoCreateReport::IO		: error_str = "IO"; break;
		// the source is invalid
		case zypp::repo::RepoCreateReport::INVALID	: error_str = "INVALID"; break;
		// rejected
		case zypp::repo::RepoCreateReport::REJECTED	: error_str = "REJECTED"; break;
		// unknown error
		case zypp::repo::RepoCreateReport::UNKNOWN	: error_str = "UNKNOWN"; break;
	    }

	    return error_str;
	}

	virtual Action problem( const zypp::Url &url, zypp::repo::RepoCreateReport::Error error, const std::string &description )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceCreateError ) );

	    if ( callback._set )
	    {
		callback.addStr(url);
		callback.addSymbol(CreateSrcErrorAsString(error));
		callback.addStr(description);

		std::string result = callback.evaluateSymbol();

		// check the returned symbol
		if ( result == "ABORT" ) return zypp::repo::RepoCreateReport::ABORT;
		if ( result == "RETRY" ) return zypp::repo::RepoCreateReport::RETRY;

		// still here?
		y2error("Unexpected symbol '%s' returned from callback.", result.c_str());
		// return default
	    }

	    // return the default implementation
	    return zypp::repo::RepoCreateReport::problem(url, error, description);
	}

	virtual void finish( const zypp::Url &url, zypp::repo::RepoCreateReport::Error error, const std::string &reason )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceCreateEnd ) );

	    if (callback._set)
	    {
		callback.addStr(url);
		callback.addSymbol(CreateSrcErrorAsString(error));
		callback.addStr(reason);

		callback.evaluate();
	    }
	}
    };

    ///////////////////////////////////////////////////////////////////
    // ProbeSourceReceive
    ///////////////////////////////////////////////////////////////////
    struct ProbeSourceReceive : public Recipient, public zypp::callback::ReceiveReport<zypp::repo::ProbeRepoReport>
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

	std::string ProbeSrcErrorAsString(zypp::repo::ProbeRepoReport::Error error)
	{
	    // convert enum to string
	    std::string error_str;

	    switch(error)
	    {
		// no error
		case zypp::repo::ProbeRepoReport::NO_ERROR	: error_str = "NO_ERROR"; break;
		// the requested Url was not found
		case zypp::repo::ProbeRepoReport::NOT_FOUND	: error_str = "NOT_FOUND"; break;
		// IO error
		case zypp::repo::ProbeRepoReport::IO	: error_str = "IO"; break;
		// the source is invalid
		case zypp::repo::ProbeRepoReport::INVALID	: error_str = "INVALID"; break;
		// unknow error
		case zypp::repo::ProbeRepoReport::UNKNOWN	: error_str = "UNKNOWN"; break;
	    }

	    return error_str;
	}

	virtual void finish(const zypp::Url &url, zypp::repo::ProbeRepoReport::Error error, const std::string &reason )
	{
	    _silent_probing = MEDIA_CHANGE_FULL;

	    CB callback( ycpcb( YCPCallbacks::CB_SourceProbeEnd ) );

	    if (callback._set)
	    {
		callback.addStr(url);
		callback.addSymbol(ProbeSrcErrorAsString(error));
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

	    return zypp::repo::ProbeRepoReport::progress(url, value);
	    return true;
	}

	virtual zypp::repo::ProbeRepoReport::Action problem( const zypp::Url &url, zypp::repo::ProbeRepoReport::Error error, const std::string &description )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceProbeError ) );

	    if ( callback._set )
	    {
		callback.addStr(url);
		callback.addSymbol(ProbeSrcErrorAsString(error));
		callback.addStr(description);

		std::string result = callback.evaluateSymbol();

		// check the returned symbol
		if ( result == "ABORT" ) return zypp::repo::ProbeRepoReport::ABORT;
		if ( result == "RETRY" ) return zypp::repo::ProbeRepoReport::RETRY;

		// still here?
		y2error("Unexpected symbol '%s' returned from callback.", result.c_str());
		// return default
	    }

	    // return the default value
	    return zypp::repo::ProbeRepoReport::problem(url, error, description);
	}
    };


    struct RepoReport : public Recipient, public zypp::callback::ReceiveReport<zypp::repo::RepoReport>
    {
	const PkgFunctions &_pkg_ref;
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

	RepoReport( RecipientCtl & construct_r, const PkgFunctions &pk ) : Recipient( construct_r ), _pkg_ref(pk) {}

        virtual void start(const zypp::ProgressData &task, const zypp::RepoInfo repo)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceReportStart ) );

	    if (callback._set)
	    {
		callback.addInt(_pkg_ref.logFindAlias(repo.alias()));

		std::string url;
		if (repo.baseUrlsBegin() != repo.baseUrlsEnd())
		{
		    url = repo.baseUrlsBegin()->asString();
		}

		callback.addStr(url);
		callback.addStr(task.name());

		callback.evaluate();
	    }
	}

        virtual bool progress(const zypp::ProgressData &task)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceReportProgress ) );

	    if (callback._set)
	    {
		callback.addInt(task.reportValue());

		return callback.evaluateBool();
	    }

	    return zypp::repo::RepoReport::progress(task);
	}

	std::string SrcReportErrorAsString(zypp::repo::RepoReport::Error error)
	{
	    // convert enum to string
	    std::string error_str;

	    switch(error)
	    {
		// no error
		case zypp::repo::RepoReport::NO_ERROR	: error_str = "NO_ERROR"; break;
		// the requested Url was not found
		case zypp::repo::RepoReport::NOT_FOUND	: error_str = "NOT_FOUND"; break;
		// IO error
		case zypp::repo::RepoReport::IO		: error_str = "IO"; break;
		// the source is invalid
		case zypp::repo::RepoReport::INVALID	: error_str = "INVALID"; break;
	    }

	    return error_str;
	}

	virtual zypp::repo::RepoReport::Action problem(zypp::Repository source,
	    zypp::repo::RepoReport::Error error, const std::string &description)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceReportError ) );

	    // the file is optional, ignore the error
	    if (_silent_probing == ZyppRecipients::MEDIA_CHANGE_OPTIONALFILE)
	    {
		y2milestone("The file is optional, ignoring the error");
		return zypp::repo::RepoReport::IGNORE;
	    }

	    if ( callback._set )
	    {
		// search Yast source ID
		callback.addInt(_pkg_ref.logFindAlias(source.info().alias()));

		std::string url;
		if (source.info().baseUrlsBegin() != source.info().baseUrlsEnd())
		{
		    url = source.info().baseUrlsBegin()->asString();
		}

		callback.addStr(url);
		callback.addSymbol(SrcReportErrorAsString(error));
		callback.addStr(description);

		std::string result = callback.evaluateSymbol();

		// check the returned symbol
		if ( result == "ABORT" ) return zypp::repo::RepoReport::ABORT;
		if ( result == "RETRY" ) return zypp::repo::RepoReport::RETRY;
		if ( result == "IGNORE" ) return zypp::repo::RepoReport::IGNORE;

		// still here?
		y2error("Unexpected symbol '%s' returned from callback.", result.c_str());
		// return default
	    }

	    // return the default value
	    return zypp::repo::RepoReport::problem(source, error, description);
	}

	virtual void finish(zypp::Repository source, const std::string &task,
	    zypp::repo::RepoReport::Error error, const std::string &reason)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_SourceReportEnd ) );

	    if (callback._set)
	    {
		// search Yast source ID
		callback.addInt(_pkg_ref.logFindAlias(source.info().alias()));

		std::string url;
		if (source.info().baseUrlsBegin() != source.info().baseUrlsEnd())
		{
		    url = source.info().baseUrlsBegin()->asString();
		}
		callback.addStr(url);

		callback.addStr(task);
		callback.addSymbol(SrcReportErrorAsString(error));
		callback.addStr(reason);

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
	const PkgFunctions &_pkg_ref;
	KeyRingReceive( RecipientCtl & construct_r, const PkgFunctions &pk) : Recipient( construct_r ), _pkg_ref(pk) {}

	virtual zypp::KeyRingReport::KeyTrust askUserToAcceptKey( const zypp::PublicKey &key, const zypp::KeyContext &context)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_ImportGpgKey) );

	    if (callback._set)
	    {
		GPGMap gpgmap(key);

		callback.addMap(gpgmap.getMap());
		PkgFunctions::RepoId srcid = context.empty() ? _pkg_ref.current_repo_id() : _pkg_ref.logFindAlias(context.repoInfo().alias());
		callback.addInt(srcid);

		bool res = callback.evaluateBool();
		y2milestone("Callback ImportGpgKey value: %s", res ? "true" : "false");

		return res ? KEY_TRUST_AND_IMPORT : KEY_DONT_TRUST;
	    }

	    y2milestone("Callback ImportGpgKey not registered, using default value: %s", zypp::KeyRingReport::askUserToAcceptKey(key, context) ? "true" : "false");

	    return zypp::KeyRingReport::askUserToAcceptKey(key, context);
	}

	virtual bool askUserToAcceptUnsignedFile(const std::string &file, const zypp::KeyContext &context)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_AcceptUnsignedFile) );

	    if (callback._set)
	    {
		callback.addStr(file);
		long long srcid = _pkg_ref.logFindAlias(context.repoInfo().alias());
		callback.addInt(srcid);

		return callback.evaluateBool();
	    }

	    return zypp::KeyRingReport::askUserToAcceptUnsignedFile(file);
	}

	virtual bool askUserToAcceptUnknownKey(const std::string &file, const std::string &id, const zypp::KeyContext &context)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_AcceptUnknownGpgKey) );

	    if (callback._set)
	    {
		callback.addStr(file);
		callback.addStr(id);
		long long srcid = _pkg_ref.logFindAlias(context.repoInfo().alias());
		callback.addInt(srcid);

		bool res = callback.evaluateBool();
		y2milestone("Callback AcceptUnknownGpgKey value: %s", res ? "true" : "false");

		return res;
	    }

	    y2milestone("Callback AcceptUnknownGpgKey is not registered, using default value: %s", zypp::KeyRingReport::askUserToAcceptUnknownKey(file,id) ? "true" : "false");

	    return zypp::KeyRingReport::askUserToAcceptUnknownKey(file,id);
	}

	virtual bool askUserToAcceptVerificationFailed(const std::string &file, const zypp::PublicKey &key, const zypp::KeyContext &context)
	{
	    CB callback( ycpcb( YCPCallbacks::CB_AcceptVerificationFailed) );

	    if (callback._set)
	    {
		GPGMap gpgmap(key);

		callback.addStr(file);
		callback.addMap(gpgmap.getMap());
		long long srcid = _pkg_ref.logFindAlias(context.repoInfo().alias());
		callback.addInt(srcid);

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
		GPGMap gpgmap(key);

		callback.addMap(gpgmap.getMap());
		callback.evaluate();
	    }
	}

        virtual void trustedKeyRemoved( const zypp::PublicKey &key )
	{
	    CB callback( ycpcb( YCPCallbacks::CB_TrustedKeyRemoved) );

	    if (callback._set)
	    {
		GPGMap gpgmap(key);

		callback.addMap(gpgmap.getMap());
		callback.evaluate();
	    }
	}
    };

    ///////////////////////////////////////////////////////////////////
    // File conflict callbacks
    ///////////////////////////////////////////////////////////////////
    struct FileConflictReceive : public Recipient,
            public zypp::callback::ReceiveReport<zypp::target::FindFileConflictstReport>
    {
        FileConflictReceive( RecipientCtl & construct_r ) : Recipient( construct_r ) {}

        virtual void reportbegin()
        {
            CB callback( ycpcb( YCPCallbacks::CB_FileConflictStart) );

            if (callback._set)
            {
                callback.evaluate();
            }
        }

        virtual bool start( const zypp::ProgressData & progress_r )
        {
            return report_progress(progress_r);
        }

        virtual bool progress( const zypp::ProgressData & progress_r,
            const zypp::sat::Queue & noFilelist_r )
        {
            return report_progress(progress_r);
        }

        virtual bool result( const zypp::ProgressData & progress_r,
            const zypp::sat::Queue & noFilelist_r,
            const zypp::sat::FileConflicts & conflicts_r )
        {
            CB callback( ycpcb( YCPCallbacks::CB_FileConflictReport) );

            if (!callback._set)
            {
                // continue
                return true;
            }

            YCPList excluded_packages;
            for_(iter, noFilelist_r.begin(), noFilelist_r.end())
            {
                // convert solvable ID to a Package
                zypp::Package::Ptr pkg(zypp::make<zypp::Package>(zypp::sat::Solvable(*iter)));
                if (pkg) {
                    excluded_packages->add(YCPString(pkg->name() + "-" +
                    pkg->edition().asString() + "-" + pkg->arch().asString()));
                }
            }

            YCPList conflicts;
            for_(iter, conflicts_r.begin(), conflicts_r.end())
            {
                conflicts->add(YCPString(iter->asUserString()));
            }

            callback.addList(excluded_packages);
            callback.addList(conflicts);

            return callback.evaluateBool();
        }

        virtual void reportend()
        {
            CB callback( ycpcb( YCPCallbacks::CB_FileConflictFinish) );

            if (callback._set)
            {
                callback.evaluate();
            }
        }

    private:

        bool report_progress(const zypp::ProgressData & progress_r)
        {
            CB callback( ycpcb( YCPCallbacks::CB_FileConflictProgress) );

            // continue
            if (!callback._set)
            {
                return true;
            }

            callback.addInt(progress_r.reportValue());
            return callback.evaluateBool();
        }
    };

///////////////////////////////////////////////////////////////////
}; // namespace ZyppRecipients
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
//
//	CLASS NAME : PkgFunctions::CallbackHandler::ZyppReceive
/**
 * @short Manages the Y2PMCallbacks we receive.
 *
 **/
class PkgFunctions::CallbackHandler::ZyppReceive : public ZyppRecipients::RecipientCtl {

  private:

    // RRM DB callbacks
    ZyppRecipients::ConvertDbReceive  _convertDbReceive;
    ZyppRecipients::RebuildDbReceive  _rebuildDbReceive;

    // package callbacks
    ZyppRecipients::InstallPkgReceive _installPkgReceive;
    ZyppRecipients::RemovePkgReceive  _removePkgReceive;
    ZyppRecipients::DownloadResolvableReceive _providePkgReceive;
    ZyppRecipients::FileConflictReceive _fileConflictReceive;

    // media callback
    ZyppRecipients::MediaChangeReceive   _mediaChangeReceive;
    ZyppRecipients::DownloadProgressReceive _downloadProgressReceive;

    // script/messages
    ZyppRecipients::ScriptExecReceive	_scriptExecReceive;
    ZyppRecipients::MessageReceive	_messageReceive;

    // source manager callback
    ZyppRecipients::SourceCreateReceive _sourceCreateReceive;
    ZyppRecipients::RepoReport	_sourceReport;
    ZyppRecipients::ProbeSourceReceive _probeSourceReceive;

    ZyppRecipients::ProgressReceive _progressReceive;

    // digest callback
    ZyppRecipients::DigestReceive _digestReceive;

    // key ring callback
    ZyppRecipients::KeyRingReceive _keyRingReceive;

    // key ring signal callback
    ZyppRecipients::KeyRingSignal _keyRingSignal;

    // authentication callback
    ZyppRecipients::AuthReceive _authReceive;

  public:

    ZyppReceive( const YCPCallbacks & ycpcb_r, PkgFunctions &pkg)
      : RecipientCtl( ycpcb_r )
      , _convertDbReceive( *this )
      , _rebuildDbReceive( *this )
      , _installPkgReceive( *this, pkg )
      , _removePkgReceive( *this )
      , _providePkgReceive( *this, pkg )
      , _fileConflictReceive( *this )
      , _mediaChangeReceive( *this )
      , _downloadProgressReceive( *this )
      , _scriptExecReceive( *this )
      , _messageReceive( *this )
      , _sourceCreateReceive( *this )
      , _sourceReport( *this, pkg)
      , _probeSourceReceive( *this )
      , _progressReceive( *this )
      , _digestReceive( *this )
      , _keyRingReceive( *this, pkg )
      , _keyRingSignal( *this )
      , _authReceive( *this )
    {
	// connect the receivers
	_convertDbReceive.connect();
	_rebuildDbReceive.connect();
	_installPkgReceive.connect();
	_removePkgReceive.connect();
	_providePkgReceive.connect();
        _fileConflictReceive.connect();
	_mediaChangeReceive.connect();
	_downloadProgressReceive.connect();
	_scriptExecReceive.connect();
	_messageReceive.connect();
	_sourceCreateReceive.connect();
	_sourceReport.connect();
	_probeSourceReceive.connect();
	_progressReceive.connect();
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
	_installPkgReceive.disconnect();
	_removePkgReceive.disconnect();
	_providePkgReceive.disconnect();
        _fileConflictReceive.disconnect();
	_mediaChangeReceive.disconnect();
	_downloadProgressReceive.disconnect();
	_scriptExecReceive.disconnect();
	_messageReceive.disconnect();
	_sourceCreateReceive.disconnect();
	_sourceReport.disconnect();
	_probeSourceReceive.disconnect();
	_progressReceive.disconnect();
	_digestReceive.disconnect();
	_keyRingReceive.disconnect();
	_keyRingSignal.disconnect();
	_authReceive.disconnect();
    }
  public:

};

///////////////////////////////////////////////////////////////////
//
//	CLASS NAME : PkgFunctions::CallbackHandler
//
///////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : PkgFunctions::CallbackHandler::CallbackHandler
//	METHOD TYPE : Constructor
//
PkgFunctions::CallbackHandler::CallbackHandler(PkgFunctions &pk)
    : _ycpCallbacks( *new YCPCallbacks() )
    , _zyppReceive( *new ZyppReceive(_ycpCallbacks, pk) )
{
}

///////////////////////////////////////////////////////////////////
//
//
//	METHOD NAME : PkgFunctions::CallbackHandler::~CallbackHandler
//	METHOD TYPE : Destructor
//
PkgFunctions::CallbackHandler::~CallbackHandler()
{
  y2debug("Deleting callback handler");
  delete &_zyppReceive;
  delete &_ycpCallbacks;
}

