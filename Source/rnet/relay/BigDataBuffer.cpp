#include <rnet/relay/BigDataBuffer.h>

bool rnet::BigDataBuffer::Reserve(size_t size)
{
    Free();
    if (size > myMaxSize)
    {
        RNetAbnormal(false); //"Requested more than max size, should be configurable;
        return false;
    }
    myTotalSize = size;
    myUsedSize = 0;
    myBuffer = (char*)(malloc(size));
    return true;
}

void rnet::BigDataBuffer::Free()
{
    if (myBuffer)
    {
        free(myBuffer);
    }
}

rnet::BigDataBuffer::~BigDataBuffer()
{
    Free();
}