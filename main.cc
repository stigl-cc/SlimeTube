#include <drogon/HttpResponse.h>
#include <drogon/drogon.h>
#include <controllers/PageController.h>
#include <controllers/ResourceController.h>

using drogon::app;

int main() {
    app().loadConfigFile("config.json");

    app().setSSLConfigCommands({
            { "Options", "Compression" }
        });

    app().setFileTypes({
            "html",
            "js",
            "css",
        });

    app().setStaticFileHeaders({
            { "Access-Control-Allow-Origin", "*" },
        });

    HttpViewData data;
    data.insert("title", "SlimyTube");
    const HttpResponsePtr res404 = HttpResponse::newHttpViewResponse("error404", data);
    app().setCustom404Page(res404, true);

    app()
        .setFloatPrecisionInJson(0, "significant")
        .setUnicodeEscapingInJson(true)
        .setJsonParserStackLimit(1000);

    app()
        .setServerHeaderField("SlimyTube")
        .enableServerHeader(true)
        .enableDateHeader(true);

    app().enableReusePort(true);

    app()
        .setUploadPath("../data/uploads")
        .setDocumentRoot("./static/")
        .setImplicitPageEnable(false)
        .enableSession(604'800, Cookie::SameSite::kLax, "SessionId", 2'628'000); // timeout = week, max_age = month

    app()
        .setGzipStatic(true)
        .setBrStatic(true)
        .enableGzip(true)
        .enableBrotli(false)
        .setStaticFilesCacheTime(60)
        .enableSendfile(true);

    app()
        .setClientMaxBodySize(10'485'760)
        .setClientMaxMemoryBodySize(10'485'760)
        .setClientMaxWebSocketMessageSize(131'072);

    app()
        .setMaxConnectionNum(10'000)
        .setMaxConnectionNumPerIP(10);

    app().run();
    return 0;
}
