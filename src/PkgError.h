
#include <string>

class PkgError
{
    private:
      std::string _last_error;
      std::string _last_error_details;

    public:
      PkgError() : _last_error(), _last_error_details() {}

      void setLastError(const std::string &error = "", const std::string &details = "");
      const std::string& lastError() {return _last_error;}
      const std::string& lastErrorDetails() {return _last_error_details;}
};

