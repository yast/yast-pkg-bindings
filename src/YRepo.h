
#ifndef YRepo_h
#define YRepo_h

#include <zypp/RepoInfo.h>
#include <zypp/MediaSetAccess.h>
#include <zypp/base/ReferenceCounted.h>

DEFINE_PTR_TYPE(YRepo);
class YRepo : public zypp::base::ReferenceCounted
{
private:
    zypp::RepoInfo _repo;
    zypp::MediaSetAccess_Ptr _maccess;
    bool _deleted;

    YRepo() {}

public:
    YRepo(zypp::RepoInfo & repo);
    ~YRepo();

    const zypp::RepoInfo & repoInfo() const { return _repo; }
    zypp::RepoInfo & repoInfo() { return _repo; }
    zypp::MediaSetAccess_Ptr & mediaAccess();

    bool isDeleted() {return _deleted;}
    void setDeleted() {_deleted = true;}

public:
    static const YRepo NOREPO;
};

#endif
