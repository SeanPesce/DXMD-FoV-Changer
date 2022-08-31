/*
    CONTRIBUTORS:
        Sean Pesce

*/
#ifdef _WIN32

#include "sp/io/powershell_ostream.h"

__SP_NAMESPACE
namespace io {

    const std::unordered_set<std::string>ps_ostream::COLORS =
    {
        "black",
        "white",
        "gray",
        "darkgray",
        "red",
        "darkred",
        "blue",
        "darkblue",
        "green",
        "darkgreen",
        "yellow",
        "darkyellow",
        "cyan",
        "darkcyan",
        "magenta",
        "darkmagenta"
    };

} // namespace io
__SP_NAMESPACE_CLOSE // namespace sp


#endif // _WIN32
