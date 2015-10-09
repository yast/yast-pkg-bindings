/*---------------------------------------------------------------------\
|                                                                      |
|                      __   __    ____ _____ ____                      |
|                      \ \ / /_ _/ ___|_   _|___ \                     |
|                       \ V / _` \___ \ | |   __) |                    |
|                        | | (_| |___) || |  / __/                     |
|                        |_|\__,_|____/ |_| |_____|                    |
|                                                                      |
|                               core system                            |
|                                                        (C) SuSE GmbH |
\----------------------------------------------------------------------/

  File:       PkgModuleCallbacks.YCP.h

  Author:     Michael Andres <ma@suse.de>
  Maintainer: Michael Andres <ma@suse.de>

  Purpose: Implementation of PkgModuleFunctions::CallbackHandler::YCPCallbacks
           (not intended to be distributed)

/-*/
#ifndef PkgModuleCallbacksYCP_h
#define PkgModuleCallbacksYCP_h

#include <stack>

#include <y2util/stringutil.h>
//#include <y2util/Date.h>
//#include <y2util/FSize.h>

#include <y2/Y2Namespace.h>
#include <y2/Y2Function.h>

#include "ycpTools.h"
#include "Callbacks.h"

//#include <ycp/y2log.h>

using namespace std;

///////////////////////////////////////////////////////////////////
//
//	CLASS NAME : PkgModuleFunctions::CallbackHandler::YCPCallbacks
/**
 * @short Stores YCPCallback related data and communicates with Y2ComponentBroker
 *
 * For each YCPCallback it's data, identified by a <CODE>@ref CBid</CODE>,
 * are stored in maps.
 *
 * To invoke a YCPCallback:
 * <PRE>
 *   YCPTerm callback = createCallback( CB_PatchProgress ); // create callback Term
 *   callback->add( YCPInteger ( percent ) );               // add arguments
 *   callback->add( YCPString ( pkg ) );
 *   bool result = evaluateBool( callback );                // evaluate
 * </PRE>
 **/
class PkgFunctions::CallbackHandler::YCPCallbacks
{
  public:

    /**
     * Unique id for each YCPCallback we may have to trigger.
     * On changes here, adapt @ref cbName.
     **/
    enum CBid {
      CB_StartRebuildDb, CB_ProgressRebuildDb, CB_NotifyRebuildDb, CB_StopRebuildDb,
      CB_StartConvertDb, CB_ProgressConvertDb, CB_NotifyConvertDb, CB_StopConvertDb,
      CB_StartScanDb, CB_ProgressScanDb, CB_ErrorScanDb, CB_DoneScanDb,
      CB_StartProvide, CB_ProgressProvide, CB_DoneProvide,
      CB_StartPackage, CB_ProgressPackage, CB_DonePackage,

      CB_SourceCreateStart, CB_SourceCreateProgress, CB_SourceCreateError, CB_SourceCreateEnd,
      CB_SourceCreateInit, CB_SourceCreateDestroy,

      CB_ProgressStart, CB_ProgressProgress, CB_ProgressDone,

      CB_StartSourceRefresh, CB_ErrorSourceRefresh, CB_DoneSourceRefresh, CB_ProgressSourceRefresh,
      CB_StartDeltaDownload, CB_ProgressDeltaDownload, CB_ProblemDeltaDownload,
      CB_StartDeltaApply, CB_ProgressDeltaApply, CB_ProblemDeltaApply,
      CB_StartPatchDownload, CB_ProgressPatchDownload, CB_ProblemPatchDownload,
      CB_FinishDeltaDownload, CB_FinishDeltaApply, CB_FinishPatchDownload, CB_PkgGpgCheck,
      CB_StartDownload, CB_ProgressDownload, CB_DoneDownload, CB_InitDownload, CB_DestDownload,

      CB_SourceProbeStart, CB_SourceProbeFailed, CB_SourceProbeSucceeded, CB_SourceProbeEnd, CB_SourceProbeProgress, CB_SourceProbeError, 
      CB_SourceReportStart, CB_SourceReportProgress, CB_SourceReportError, CB_SourceReportEnd, CB_SourceReportInit, CB_SourceReportDestroy,

      CB_ScriptStart, CB_ScriptProgress, CB_ScriptProblem, CB_ScriptFinish,
      CB_Message,

      CB_Authentication,

      CB_MediaChange,
      CB_SourceChange,
      CB_ResolvableReport,
      CB_ImportGpgKey,
      CB_AcceptUnknownGpgKey,
      CB_AcceptUnsignedFile,
      CB_AcceptFileWithoutChecksum,
      CB_AcceptVerificationFailed,
      CB_AcceptWrongDigest, CB_AcceptUnknownDigest,
      CB_TrustedKeyAdded,
      CB_TrustedKeyRemoved,

      CB_ProcessStart,
      CB_ProcessNextStage,
      CB_ProcessProgress,
      CB_ProcessFinished,
    };

    /**
     * Returns the enum name without the leading "CB_"
     * (e.g. "StartProvide" for CB_StartProvide). Should
     * be in sync with @ref CBid.
     **/
    static string cbName( CBid id_r );
  private:

    typedef map <CBid, stack<YCPReference> > _cbdata_t;
    _cbdata_t _cbdata;

  public:

    /**
     * Constructor.
     **/
    YCPCallbacks( )
    {}


    void popCallback( CBid id_r );

    /**
     * Set a YCPCallback data
     **/
    void setCallback( CBid id_r, const YCPReference &func_r );

    /**
     * Set the YCPCallback according to args_r.
     * @return YCPVoid on success, otherwise YCPError.
     **/
    YCPValue setYCPCallback( CBid id_r, const YCPValue &func);

    /**
     * @return Whether the YCPCallback is set. If not, there's
     * no need to create and evaluate it.
     **/
    bool isSet( CBid id_r ) const;

  public:

    /**
     * @return The YCPCallback term, ready to append any arguments.
     **/
    Y2Function* createCallback( CBid id_r ) const;

  public:

    /**
     * @short Convenience base class for YCPCallback sender
     *
     * A functional interface for sending YCPCallbacks with well known arguments
     * and return values is desirable. Esp. for YCPcallbacks triggered from
     * multiple recipients. Currently each recipient has to implememt correct
     * number and type of arguments, as well as the returned type. That's bad
     * if something changes. As soon as YCPCallbacks provides them (as const methods),
     * ZyppRecipients::Recipient should no longer inherit Send, but provide
     * an easy access to RecipientCtl::_ycpcb.
     **/
    class Send {
      public:
	/**
	 * @short Convenience class for YCPCallback sending
	 **/
	struct CB {
	  const Send & _send;
	  CBid _id;
	  bool     _set;
	  Y2Function* _func;
	  YCPValue _result;
	  CB( const Send & send_r, CBid func )
	    : _send( send_r )
	    , _id( func )
	    , _set( _send.ycpcb().isSet( func ) )
	    , _func( _send.ycpcb().createCallback( func ) )
	    , _result( YCPVoid() )
	  {}

	  ~CB ()
	  {
	    if (_func) delete _func;
	  }

	  CB & addStr( const string & arg ) { if (_func != NULL) _func->appendParameter( YCPString( arg ) ); return *this; }
	  CB & addStr( const zypp::Pathname & arg ) { return addStr( arg.asString() ); }
	  CB & addStr( const zypp::Url & arg ) { return addStr( arg.asString() ); }

	  CB & addInt( long long arg ) { if (_func != NULL) _func->appendParameter( YCPInteger( arg ) ); return *this; }

	  CB & addBool( bool arg ) { if (_func != NULL) _func->appendParameter( YCPBoolean( arg ) ); return *this; }

	  CB & addMap( YCPMap arg ) { if (_func != NULL) _func->appendParameter( arg ); return *this; }
	  CB & addList( YCPList arg ) { if (_func != NULL) _func->appendParameter( arg ); return *this; }

	  CB & addSymbol( const string &arg ) { if (_func != NULL) _func->appendParameter( YCPSymbol(arg) ); return *this; }

	  bool isStr() const { return _result->isString(); }
	  bool isInt() const { return _result->isInteger(); }
	  bool isBool() const { return _result->isBoolean(); }

	  bool expecting( YCPValueType exp_r ) const;

	  bool evaluate();

	  bool evaluate( YCPValueType exp_r ) {
	    return evaluate() && expecting( exp_r );
	  }

	  string evaluateStr( const string & def_r = "" ) {
	    return evaluate( YT_STRING ) ? _result->asString()->value() : def_r;
	  }

	  string evaluateSymbol( const string & def_r = "" ) {
	    return evaluate( YT_SYMBOL ) ? _result->asSymbol()->symbol() : def_r;
	  }

	  long long evaluateInt( const long long & def_r = 0 ) {
	    return evaluate( YT_INTEGER ) ? _result->asInteger()->value() : def_r;
	  }

	  bool evaluateBool( const bool & def_r = false ) {
	    return evaluate( YT_BOOLEAN ) ? _result->asBoolean()->value() : def_r;
	  }

	  YCPMap evaluateMap( const YCPMap def_r = YCPMap())
	  {
	      return evaluate( YT_MAP ) ? _result->asMap() : def_r;
	  }
	};
      private:
	const YCPCallbacks & _ycpcb;
      public:
	Send( const YCPCallbacks & ycpcb_r ) : _ycpcb( ycpcb_r ) {}
	virtual ~Send() {}
	const YCPCallbacks & ycpcb() const { return _ycpcb; }
	CB ycpcb( CBid func ) const { return CB( *this, func ); }
    };
};

///////////////////////////////////////////////////////////////////

#endif // PkgModuleCallbacksYCP_h
