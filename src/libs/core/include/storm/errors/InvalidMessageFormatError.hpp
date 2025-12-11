#include <stdexcept>

namespace storm
{

class InvalidMessageFormatError : public std::runtime_error
{
public:
    explicit InvalidMessageFormatError(const std::string &msg)
        : runtime_error(msg)
    {
    }
};

}


