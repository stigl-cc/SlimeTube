#pragma once

#include <drogon/HttpMiddleware.h>
#include <drogon/HttpRequest.h>
#include <drogon/drogon_callbacks.h>

using drogon::HttpMiddleware,
    drogon::HttpRequestPtr,
    drogon::MiddlewareCallback,
    drogon::MiddlewareNextCallback;

namespace SlimyTube {
    class Authenticator : public HttpMiddleware<Authenticator> {
        public:
        void invoke(const HttpRequestPtr &req, MiddlewareNextCallback &&nextCb, MiddlewareCallback &&mcb) override;
    };
}
