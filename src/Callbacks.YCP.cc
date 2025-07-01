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

  Purpose: Implementation of PkgFunctions::CallbackHandler::YCPCallbacks
           (not intended to be distributed)

/-*/

#include "Callbacks.YCP.h"
#include "log.h"

#include <y2/Y2ComponentBroker.h>
#include <y2/Y2Component.h>

    /**
     * Returns the enum name without the leading "CB_"
     * (e.g. "StartProvide" for CB_StartProvide). Should
     * be in sync with @ref CBid.
     **/
    string PkgFunctions::CallbackHandler::YCPCallbacks::cbName( CBid id_r ) {
      switch ( id_r ) {
#define ENUM_OUT(N) case CB_##N: return #N
	ENUM_OUT( StartRebuildDb );
	ENUM_OUT( ProgressRebuildDb );
	ENUM_OUT( NotifyRebuildDb );
	ENUM_OUT( StopRebuildDb );
	ENUM_OUT( StartConvertDb );
	ENUM_OUT( ProgressConvertDb );
	ENUM_OUT( NotifyConvertDb );
	ENUM_OUT( StopConvertDb );
	ENUM_OUT( StartScanDb );
	ENUM_OUT( ProgressScanDb );
	ENUM_OUT( ErrorScanDb );
	ENUM_OUT( DoneScanDb );

	ENUM_OUT( StartProvide );
	ENUM_OUT( ProgressProvide );
	ENUM_OUT( DoneProvide );
	ENUM_OUT( StartPackage );
	ENUM_OUT( ProgressPackage );
	ENUM_OUT( DonePackage );
	ENUM_OUT( StartDownload );
	ENUM_OUT( ProgressDownload );
	ENUM_OUT( DoneDownload );
	ENUM_OUT( InitDownload );
	ENUM_OUT( DestDownload );
	ENUM_OUT( StartInstallResolvableSA );
	ENUM_OUT( ProgressInstallResolvableSA );
	ENUM_OUT( FinishInstallResolvableSA );

	ENUM_OUT( ScriptStart );
	ENUM_OUT( ScriptProgress );
	ENUM_OUT( ScriptProblem );
	ENUM_OUT( ScriptFinish );
	ENUM_OUT( Message );

	ENUM_OUT( Authentication );

	ENUM_OUT( SourceCreateStart );
	ENUM_OUT( SourceCreateProgress );
	ENUM_OUT( SourceCreateError );
	ENUM_OUT( SourceCreateEnd );
	ENUM_OUT( SourceCreateInit );
	ENUM_OUT( SourceCreateDestroy );

	ENUM_OUT( SourceProbeStart );
	ENUM_OUT( SourceProbeFailed );
	ENUM_OUT( SourceProbeSucceeded );
	ENUM_OUT( SourceProbeEnd );
	ENUM_OUT( SourceProbeProgress );
	ENUM_OUT( SourceProbeError );

	ENUM_OUT( SourceReportStart );
	ENUM_OUT( SourceReportProgress );
	ENUM_OUT( SourceReportError );
	ENUM_OUT( SourceReportEnd );
	ENUM_OUT( SourceReportInit );
	ENUM_OUT( SourceReportDestroy );
      
	ENUM_OUT( ProgressStart );
	ENUM_OUT( ProgressProgress );
	ENUM_OUT( ProgressDone );

	ENUM_OUT( StartSourceRefresh );
	ENUM_OUT( ErrorSourceRefresh );
	ENUM_OUT( DoneSourceRefresh );
	ENUM_OUT( ProgressSourceRefresh );
	ENUM_OUT( StartDeltaDownload );
	ENUM_OUT( ProgressDeltaDownload );
	ENUM_OUT( ProblemDeltaDownload );
	ENUM_OUT( StartDeltaApply );
	ENUM_OUT( ProgressDeltaApply );
	ENUM_OUT( ProblemDeltaApply );
	ENUM_OUT( FinishDeltaDownload );
	ENUM_OUT( FinishDeltaApply );
	ENUM_OUT( PkgGpgCheck );
	ENUM_OUT( MediaChange );
	ENUM_OUT( SourceChange );
	ENUM_OUT( ResolvableReport );
	ENUM_OUT( ImportGpgKey );
	ENUM_OUT( AcceptUnknownGpgKey );
	ENUM_OUT( AcceptUnsignedFile );
	ENUM_OUT( AcceptVerificationFailed );
	ENUM_OUT( AcceptFileWithoutChecksum );
	ENUM_OUT( TrustedKeyAdded );
	ENUM_OUT( TrustedKeyRemoved );
	ENUM_OUT( AcceptWrongDigest );
	ENUM_OUT( AcceptUnknownDigest );

        ENUM_OUT( ProcessStart );
        ENUM_OUT( ProcessNextStage );
        ENUM_OUT( ProcessProgress );
        ENUM_OUT( ProcessFinished );
        ENUM_OUT( FileConflictStart );
        ENUM_OUT( FileConflictProgress );
        ENUM_OUT( FileConflictReport );
        ENUM_OUT( FileConflictFinish );
#undef ENUM_OUT
	// no default! let compiler warn missing values
      }
      return stringutil::form( "CBid(%d)", id_r );
    }

    void PkgFunctions::CallbackHandler::YCPCallbacks::popCallback( CBid id_r ) {
       _cbdata_t::iterator tmp1 = _cbdata.find(id_r);
       if (tmp1 != _cbdata.end() && !tmp1->second.empty())
       {
	   y2debug("Unregistering callback, restoring the previous one");
           tmp1->second.pop();
       }
    }

    /**
     * Set a YCPCallbacks data from string "module::symbol"
     **/
    void PkgFunctions::CallbackHandler::YCPCallbacks::setCallback( CBid id_r, const YCPReference &func_r ) {
	y2debug ("Registering callback %s", cbName(id_r).c_str());

        _cbdata[id_r].push(func_r);
    }

    /**
     * Set the YCPCallback according to args_r.
     * @return YCPVoid on success, otherwise YCPError.
     **/
    YCPValue PkgFunctions::CallbackHandler::YCPCallbacks::setYCPCallback( CBid id_r, const YCPValue &func ) {
       if (!func->isVoid())
       {
	    if (func->isReference())
	    {
		setCallback(id_r, func->asReference());
	    }
	    else
	    {
		y2internal("Parameter 'func' is not a reference!");
	    }
       }
       else
       {
           popCallback( id_r );
       }
       return YCPVoid();
    }

    /**
     * @return Whether the YCPCallback is set. If not, there's
     * no need to create and evaluate it.
     **/
    bool PkgFunctions::CallbackHandler::YCPCallbacks::isSet( CBid id_r ) const {
       const _cbdata_t::const_iterator tmp1 = _cbdata.find(id_r);
       return tmp1 != _cbdata.end() && !tmp1->second.empty();
    }

    /**
     * @return The YCPCallback term, ready to append any arguments.
     **/
    Y2Function* PkgFunctions::CallbackHandler::YCPCallbacks::createCallback( CBid id_r ) const {
	const _cbdata_t::const_iterator tmp1 = _cbdata.find(id_r);

	if (tmp1 == _cbdata.end())
	{
	    y2debug ("Callback %s not found", cbName(id_r).c_str());
	    return NULL;
	}
	if (tmp1->second.empty())
	{
	    y2debug ("Callback %s is empty", cbName(id_r).c_str());
	    return NULL;
	}

	const YCPReference func(tmp1->second.top());

	if (func.isNull() || ! func->isReference())
	{
	    y2debug ("Callback %s is not a func reference", cbName(id_r).c_str());
	    return NULL;
	}

	SymbolEntryPtr ptr_sentry = func->entry();

	Y2Namespace* ns = const_cast<Y2Namespace*> (ptr_sentry->nameSpace ());

	Y2Function* functioncall = ns->createFunctionCall (
	    ptr_sentry->name (),
	    ptr_sentry->type ()
	);

	if (!functioncall)
	{
//	    TODO: y2internal ("Cannot get function call object for %s", m_sentry->toString().c_str());
	    return NULL;
	}

	return functioncall;
    }


bool PkgFunctions::CallbackHandler::YCPCallbacks::Send::CB::expecting( YCPValueType exp_r ) const
{
    if ( _result->valuetype() == exp_r )
      return true;
    y2internal ("Wrong return type %s: Expected %s", Type::vt2type(_result->valuetype())->toString().c_str(), Type::vt2type(exp_r)->toString().c_str());
    return false;
}

bool PkgFunctions::CallbackHandler::YCPCallbacks::Send::CB::evaluate()
{
    if ( _set && _func ) {
      y2debug ("Evaluating callback (registered funciton: %s)", _func->name().c_str());
      _result = _func->evaluateCall ();

      delete _func;
      _func = _send.ycpcb().createCallback( _id );
      return true;
    }

    return false;
}

