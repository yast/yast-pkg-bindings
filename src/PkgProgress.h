/*
   File:	PkgProgress.h

   Author:	Ladislav Slezák <lslezak@novell.com>
/-*/

#ifndef PkgProgress_h
#define PkgProgress_h

#include <string>
#include <list>

//class PkgFunctions::CallbackHandler;
#include <PkgModuleFunctions.h>
#include <Callbacks.YCP.h>


#include <zypp/ProgressData.h>
#include <boost/bind.hpp>

class PkgProgress
{
    public:

	PkgProgress(PkgModuleFunctions::CallbackHandler &handler_ref)
	{}

	void Start( const std::string &process, const std::list<std::string> &stages, const std::string &help);

	void NextStage();

	void Done();

        const zypp::ProgressData::ReceiverFnc & Receiver()
        {
            return progress_handler;
        }


	~PkgProgress();

 private:
        zypp::ProgressData::ReceiverFnc progress_handler;

};

#endif

