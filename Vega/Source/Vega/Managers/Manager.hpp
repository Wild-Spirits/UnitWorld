#pragma once

namespace Vega
{

    class Manager
    {
    public:
        virtual ~Manager() = default;

        virtual void Destroy() = 0;

    protected:
    };

}    // namespace Vega
