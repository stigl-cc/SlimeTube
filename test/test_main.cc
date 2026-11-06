#define DROGON_TEST_MAIN
#include <drogon/drogon_test.h>
#include <drogon/drogon.h>
#include <drogon/HttpClient.h>
#include <drogon/HttpRequest.h>
#include <drogon/HttpTypes.h>
#include <drogon/utils/coroutine.h>
#include <string>

DROGON_TEST(LoginTest) {
    using namespace drogon;

    std::string cookie;
    HttpClientPtr client = HttpClient::newHttpClient("http://localhost:8080");

    // Delete potentially existing test account
    std::function<Task<void>()> req_delete = [&TEST_CTX, &client]() -> Task<void> {
        HttpRequestPtr req = HttpRequest::newHttpRequest();
        req->setMethod(HttpMethod::Delete);
        req->setPath("/api/user?name=test&pass=testing");

        HttpResponsePtr resp = co_await client->sendRequestCoro(req);
        CO_REQUIRE(resp != nullptr);
        CHECK((resp->getStatusCode() == HttpStatusCode::k200OK || resp->getStatusCode() == HttpStatusCode::k401Unauthorized));
    };

    // Create test account
    std::function<Task<void>()> req_register = [&TEST_CTX, &client]() -> Task<void> {
        HttpRequestPtr req = HttpRequest::newHttpRequest();
        req->setMethod(HttpMethod::Post);
        req->setPath("/api/user/register?name=test&pass=testing");

        HttpResponsePtr resp = co_await client->sendRequestCoro(req);
        CO_REQUIRE(resp != nullptr);
        CHECK(resp->getStatusCode() == k201Created);
        CHECK(resp->contentType() == CT_NONE);
    };

    // Try bad login into the account
    std::function<Task<void>()> req_wrong_login = [&TEST_CTX, &client]() -> Task<void> {
        HttpRequestPtr req = HttpRequest::newHttpRequest();
        req->setMethod(HttpMethod::Post);
        req->setPath("/api/user/login?name=test&pass=wrongpassword");

        HttpResponsePtr resp = co_await client->sendRequestCoro(req);
        CO_REQUIRE(resp != nullptr);
        CHECK(resp->getStatusCode() == k401Unauthorized);
    };

    // Log into said account
    std::function<Task<void>()> req_login = [&TEST_CTX, &client, &cookie]() -> Task<void> {
        HttpRequestPtr req = HttpRequest::newHttpRequest();
        req->setMethod(HttpMethod::Post);
        req->setPath("/api/user/login?name=test&pass=testing");

        HttpResponsePtr resp = co_await client->sendRequestCoro(req);
        CO_REQUIRE(resp != nullptr);
        CHECK(resp->getStatusCode() == k200OK);
        CHECK(resp->contentType() == CT_NONE);
        CHECK(resp->cookies().contains("SessionId"));

        // Check if not empty and also save cookie
        CHECK(!(cookie = resp->cookies().at("SessionId")).empty());
    };

    sync_wait(req_delete());
    sync_wait(req_register());
    sync_wait(req_login());
    sync_wait(req_wrong_login());
}

int main(int argc, char** argv)  {
    using drogon::app;
    std::promise<void> p1;
    std::future<void> f1 = p1.get_future();

    std::thread thr([&]() {
        app().getLoop()->queueInLoop([&p1]() { p1.set_value(); });
        app().run();
    });

    f1.get();
    int status = drogon::test::run(argc, argv);

    app().getLoop()->queueInLoop([]() { app().quit(); });
    thr.join();
    return status;
}
