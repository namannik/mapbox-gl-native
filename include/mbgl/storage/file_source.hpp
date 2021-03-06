#pragma once

#include <mbgl/storage/response.hpp>
#include <mbgl/storage/resource.hpp>

#include <mbgl/util/noncopyable.hpp>
#include <mbgl/util/async_request.hpp>

#include <functional>
#include <memory>

namespace mbgl {

class FileSource : private util::noncopyable {
public:
    virtual ~FileSource() = default;

    using Callback = std::function<void (Response)>;

    // Request a resource. The callback will be called asynchronously, in the same
    // thread as the request was made. This thread must have an active RunLoop. The
    // request may be cancelled before completion by releasing the returned AsyncRequest.
    // If the request is cancelled before the callback is executed, the callback will
    // not be executed.
    virtual std::unique_ptr<AsyncRequest> request(const Resource&, Callback) = 0;
};

} // namespace mbgl
