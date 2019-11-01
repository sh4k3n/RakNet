#pragma once

#include <rnet/RNet.h>
#include <stdint.h>
#include <stdlib.h>

namespace rnet
{

    class BigDataBuffer
    {
    public:
        BigDataBuffer() : myTotalSize(0) { }

        bool Reserve(size_t size);

        void Free();

        ~BigDataBuffer();

        char* myBuffer = nullptr;
        uint32_t myUsedSize = 0;
        uint32_t myTotalSize;

        void SetMaxSize(uint32_t s) { myMaxSize = s; }

    private:
        uint32_t myMaxSize = 256 * 1024 * 1024;
    };
}