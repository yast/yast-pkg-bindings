/*
   File:	PkgProgress.h

   Author:	Ladislav Slezák <lslezak@novell.com>
/-*/

#ifndef PkgProgress_h
#define PkgProgress_h

#include <string>
#include <list>

#include <PkgModuleFunctions.h>
#include <Callbacks.YCP.h>

#include <zypp/ProgressData.h>
#include <boost/bind.hpp>

class PkgProgress
{
    public:

	PkgProgress(PkgModuleFunctions::CallbackHandler &handler_ref)
	    : callback_handler(handler_ref),
	    progress_handler(boost::bind(&PkgProgress::_receiver, this, _1)),
	    running(false)
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
	const PkgModuleFunctions::CallbackHandler &callback_handler;
	zypp::ProgressData::ReceiverFnc progress_handler;
	bool running;

    protected:
	bool _receiver(const zypp::ProgressData &progress);
};

#endif

