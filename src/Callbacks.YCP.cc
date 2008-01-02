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

#include "Callbacks.YCP.h"

#include <y2/Y2ComponentBroker.h>
#include <y2/Y2Component.h>

    /**
     * Returns the enum name without the leading "CB_"
     * (e.g. "StartProvide" for CB_StartProvide). Should
     * be in sync with @ref CBid.
     **/
    string PkgModuleFunctions::CallbackHandler::YCPCallbacks::cbName( CBid id_r ) {
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
	ENUM_OUT( StartPatchDownload );
	ENUM_OUT( ProgressPatchDownload );
	ENUM_OUT( ProblemPatchDownload );
	ENUM_OUT( FinishDeltaDownload );
	ENUM_OUT( FinishDeltaApply );
	ENUM_OUT( FinishPatchDownload );
	ENUM_OUT( MediaChange );
	ENUM_OUT( SourceChange );
	ENUM_OUT( ResolvableReport );
	ENUM_OUT( ImportGpgKey );
	ENUM_OUT( AcceptNonTrustedGpgKey );
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
#undef ENUM_OUT
	// no default! let compiler warn missing values
      }
      return stringutil::form( "CBid(%d)", id_r );
    }

    void PkgModuleFunctions::CallbackHandler::YCPCallbacks::popCallback( CBid id_r ) {
       _cbdata_t::iterator tmp1 = _cbdata.find(id_r);
       if (tmp1 != _cbdata.end() && !tmp1->second.empty())
           tmp1->second.pop();
    }

    /**
     * Set a YCPCallbacks data from string "module::symbol"
     **/
    void PkgModuleFunctions::CallbackHandler::YCPCallbacks::setCallback( CBid id_r, const string & name_r ) {
      y2debug ("Registering callback %s", name_r.c_str ());
      string::size_type colonpos = name_r.find("::");
      if ( colonpos != string::npos ) {

        string module = name_r.substr ( 0, colonpos );
	string symbol = name_r.substr ( colonpos + 2 );

        Y2Component *c = Y2ComponentBroker::getNamespaceComponent (module.c_str());
        if (c == NULL)
        {
          y2error ("No component can provide namespace %s for a callback of %s (callback id %d)",
                 module.c_str (), symbol.c_str (), id_r);
          return;
        }

        Y2Namespace *ns = c->import (module.c_str ());
        if (ns == NULL)
        {
          y2error ("Component %p could not provide namespace %s for a callback of %s",
                 c, module.c_str (), symbol.c_str ());
	  return;
        }

	// ensure it is an initialized namespace
	ns->initialize ();

        _cbdata[id_r].push (CBdata (module, symbol, ns));
      } else {
	y2error ("Callback must be a part of a namespace");
      }
    }
    /**
     * Set a YCPCallbacks data according to args_r.
     **/
    bool PkgModuleFunctions::CallbackHandler::YCPCallbacks::setCallback( CBid id_r, const YCPString & args ) {
      string name = args->value();
      setCallback( id_r, name );
      return true;
    }
    /**
     * Set the YCPCallback according to args_r.
     * @return YCPVoid on success, otherwise YCPError.
     **/
    YCPValue PkgModuleFunctions::CallbackHandler::YCPCallbacks::setYCPCallback( CBid id_r, const YCPString & args_r ) {
       if (!args_r->value().empty ())
       {
    	   if ( ! setCallback( id_r, args_r ) ) {
		return YCPError( string("Bad args to Pkg::Callback") + cbName( id_r ) );
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
    bool PkgModuleFunctions::CallbackHandler::YCPCallbacks::isSet( CBid id_r ) const {
       const _cbdata_t::const_iterator tmp1 = _cbdata.find(id_r);
       return tmp1 != _cbdata.end() && !tmp1->second.empty();
    }

    /**
     * @return The YCPCallback term, ready to append any arguments.
     **/
    Y2Function* PkgModuleFunctions::CallbackHandler::YCPCallbacks::createCallback( CBid id_r ) const {
       const _cbdata_t::const_iterator tmp1 = _cbdata.find(id_r);
       if (tmp1 == _cbdata.end() || tmp1->second.empty())
           return NULL;
       const CBdata& tmp2 = tmp1->second.top();

      string module = tmp2.module;
      string name = tmp2.symbol;
      Y2Namespace *ns = tmp2.component;
      if (ns == NULL)
      {
          y2error ("No namespace %s for a callback of %s", module.c_str (), name.c_str ());
	  return NULL;
      }

      Y2Function* func = ns->createFunctionCall (name, Type::Unspec); // FIXME: here we can setup the type check
      if (func == NULL)
      {
          y2error ("Cannot find function %s in module %s as a callback", name.c_str (), module.c_str());
	  return NULL;
      }

      return func;
    }

