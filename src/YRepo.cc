
/////////////////////////////////////////////////////////////////////////////
//
// Class: YRepo
//

#include <YRepo.h>

#define y2log_component "Pkg"
#include <ycp/y2log.h>

IMPL_PTR_TYPE(YRepo);

YRepo::YRepo(zypp::RepoInfo & repo)
    : _repo(repo), _deleted(false)
{}

YRepo::~YRepo()
{
    if (_maccess)
    {
        try { _maccess->release(); }
        catch (const zypp::media::MediaException & ex)
	{
	    y2error("Error in ~Yrepo(): %s", ex.asString().c_str());
	}
    }
}

zypp::MediaSetAccess_Ptr & YRepo::mediaAccess()
{
    if (!_maccess)
    {
        y2milestone("Creating new MediaSetAccess for url %s",
            (*_repo.baseUrlsBegin()).asString().c_str());
        _maccess = new zypp::MediaSetAccess(*_repo.baseUrlsBegin()); // FIXME handle multiple baseUrls
    }

    return _maccess;
}

const YRepo YRepo::NOREPO;

