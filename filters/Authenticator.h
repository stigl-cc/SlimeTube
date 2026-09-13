#pragma once

#include <drogon/HttpMiddleware.h>
#include <drogon/HttpRequest.h>
#include <drogon/drogon_callbacks.h>

using drogon::HttpMiddleware,
    drogon::HttpRequestPtr,
    drogon::MiddlewareCallback,
    drogon::MiddlewareNextCallback;

namespace SlimyTube {
    class Authenticator {
        protected:
        virtual ~Authenticator() = default;
        int passwordCheck(uint32_t& userId, const std::string& name, const std::string& pass);
    };

    class StrictAuthenticator : public HttpMiddleware<StrictAuthenticator>, public Authenticator {
        public:
        ~StrictAuthenticator() override = default;
        virtual void invoke(const HttpRequestPtr &req, MiddlewareNextCallback &&nextCb, MiddlewareCallback &&mcb) override;
    };

    class LaxAuthenticator : public HttpMiddleware<LaxAuthenticator>, public Authenticator {
        public:
        ~LaxAuthenticator() override = default;
        virtual void invoke(const HttpRequestPtr &req, MiddlewareNextCallback &&nextCb, MiddlewareCallback &&mcb) override;
    };
}
