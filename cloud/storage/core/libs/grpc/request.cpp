#include "request.h"

namespace NCloud::NStorage::NGrpc {

////////////////////////////////////////////////////////////////////////////////

void* TRequestHandlerBase::AcquireCompletionTag()
{
    ++RefCount;
    return this;
}

void TRequestHandlerBase::ReleaseCompletionTag()
{
    if (--RefCount == 0) {
        delete this;
    }
}

}   // namespace NCloud::NStorage::NGrpc
