// Original author: ??

#include <vector>
#include <array>

template<class T = DWORD, class S = DWORD> struct Pointer
{
private:
    std::vector<S> params;
    S variable;
    bool MoreThanOne;
public:
    template<class... Args>
    Pointer(Args... args)
    {
        std::array<S, sizeof...(args)> list = { args... };
        for (auto i : list)
            params.push_back(i);
        if (params.size() > 1)
            MoreThanOne = true;
        else
            MoreThanOne = false;
    }
    T ResolvePointer()
    {
        variable = params[0];
        if (!MoreThanOne)
            return (T)variable;
        try
        {
            auto it = params.begin();
            ++it;
            for (; it != params.end(); ++it)
            {
                if (*reinterpret_cast<S*>(variable) == NULL)
                    return static_cast<T>(NULL);
                variable = *reinterpret_cast<S*>(variable)+*it;
            }
        }
        catch (...)
        {
            return static_cast<T>(NULL);
        }
        return (T)variable;
    }
    T operator()()
    {
        return ResolvePointer();
    }
};