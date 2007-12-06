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
          ycp2error ("No component can provide namespace %s for a callback of %s (callback id %d)",
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
	ycp2error ("Callback must be a part of a namespace");
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
          ycp2error ("Cannot find function %s in module %s as a callback", name.c_str (), module.c_str());
	  return NULL;
      }

      return func;
    }

