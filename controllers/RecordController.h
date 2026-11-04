#pragma once
#include <drogon/HttpController.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpTypes.h>

namespace SlimyTube {
    using drogon::HttpMethod::Get,
        drogon::HttpMethod::Post,
        drogon::HttpMethod::Delete,
        drogon::HttpRequestPtr,
        drogon::HttpResponsePtr;

    class RecordController : public drogon::HttpController<RecordController> {
        public:
    METHOD_LIST_BEGIN
        // unless specified otherwise
        // 400 - headers missing
        // 401 - authentication not provided
        // 403 - video not owned by user
        // 404 - video/user not found
        // 200/201/202 / 303 - success (/successful redirect) - user facing endpoints using 303 and api endpoints using 2XX

        // headers: name, pass
        // errors: 400(name/pass headers missing) 409(name exists) 500(ssl failed)
        // success: 303
        ADD_METHOD_TO(RecordController::userRegister, "/api/user/register", Post, "SlimyTube::RateLimiter");
        ADD_METHOD_TO(RecordController::userRegister, "/register", Post, "SlimyTube::RateLimiter");

        // errors: 429 303(wrong password)
        // success: 303(reset /account)
        ADD_METHOD_TO(RecordController::userLogin, "/api/user/login", Post, "SlimyTube::LaxApiAuthenticator", "SlimyTube::RateLimiter");
        ADD_METHOD_TO(RecordController::userLogin, "/login", Post, "SlimyTube::LaxAuthenticator", "SlimyTube::RateLimiter");

        // headers: Cookie
        // errors: 401 400(logged in via non session)
        // success: 303(go to login.csp)
        ADD_METHOD_TO(RecordController::userLogout, "/api/user/logout", Post, "SlimyTube::StrictApiAuthenticator");
        ADD_METHOD_TO(RecordController::userLogout, "/logout", Post, "SlimyTube::StrictAuthenticator");

        // headers: Cookie(for session login method)
        // errors: 401
        // success: 200
        ADD_METHOD_TO(RecordController::userDelete, "/api/user", Delete, "SlimyTube::StrictApiAuthenticator");
        ADD_METHOD_TO(RecordController::userDelete, "/user/delete", Post, "SlimyTube::StrictAuthenticator");

        // headers: Cookie(for session login method)
        // errors: 401 400(new_name empty/missing) 409(name exists)
        // success: 200
        ADD_METHOD_TO(RecordController::userChangeName, "/api/user/name", Post, "SlimyTube::StrictApiAuthenticator");
        ADD_METHOD_TO(RecordController::userChangeName, "/user/name", Post, "SlimyTube::StrictAuthenticator");

        // headers: Cookie(for session login method)
        // errors: 401 400(new_pass empty/missing) 500(ssl failed)
        // success: 200/303(reset /account)
        ADD_METHOD_TO(RecordController::userChangePass, "/api/user/password", Post, "SlimyTube::StrictApiAuthenticator");
        ADD_METHOD_TO(RecordController::userChangePass, "/user/password", Post, "SlimyTube::StrictAuthenticator");

        // headers: Cookie(for session login method)
        // errors: 400(!title || !desc) 401
        // success: 200/303(go to /upload)
        ADD_METHOD_TO(RecordController::videoCreate, "/api/video", Post, "SlimyTube::StrictApiAuthenticator");
        ADD_METHOD_TO(RecordController::videoCreate, "/video/create", Post, "SlimyTube::StrictAuthenticator");

        // headers: Cookie(for session login method)
        // errors: 404 403 401
        // success: 200/303
        ADD_METHOD_TO(RecordController::videoDelete, "/api/video/{id}", Delete, "SlimyTube::StrictApiAuthenticator");
        ADD_METHOD_TO(RecordController::videoDelete, "/video/delete/{id}", Post, "SlimyTube::StrictAuthenticator");

        // headers: Cookie(for session login method)
        // errors 404 403 401
        // success 200/303
        ADD_METHOD_TO(RecordController::videoChangeTitle, "/api/video/title/{id}", Post, "SlimyTube::StrictAuthenticator");
        ADD_METHOD_TO(RecordController::videoChangeTitle, "/video/title/{id}", Post, "SlimyTube::StrictAuthenticator");

        // headers: Cookie(for session login method)
        // errors 404 403 401
        // success 200/303
        ADD_METHOD_TO(RecordController::videoChangeDescription, "/api/video/description/{id}", Post, "SlimyTube::StrictApiAuthenticator");   
        ADD_METHOD_TO(RecordController::videoChangeDescription, "/video/description/{id}", Post, "SlimyTube::StrictAuthenticator");
    METHOD_LIST_END

        void userRegister(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const;
        void userDelete(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const;
        void userLogin(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const;
        void userLogout(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const;
        void userChangeName(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const;
        void userChangePass(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const;

        void videoChangeTitle(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, const std::string& id) const;
        void videoChangeDescription(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, const std::string& id) const;

        void videoCreate(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond) const;
        void videoDelete(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&respond, const std::string& id) const;
    };
}
