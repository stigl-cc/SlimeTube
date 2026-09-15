#include "Settings.hh"
#include <drogon/Cookie.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpViewData.h>
#include <drogon/drogon.h>
#include <controllers/PageController.h>
#include <controllers/ResourceController.h>

using
    drogon::HttpViewData,
    drogon::HttpResponse,
    drogon::Cookie,
    drogon::app,
    SlimyTube::Settings;

int main() {
    Settings& settings = Settings::getInstance();
    app().loadConfigFile("config.json");

    Json::Value custom_user_config = app().getCustomConfig();
    settings.title = custom_user_config.get("title", "SlimyTube").as<std::string>();
    settings.zeroRangeLimit = custom_user_config.get("zero_range_limit", 1024 * 1024).as<uint64_t>();
    settings.thumbnails_dir = custom_user_config.get("thumbnails_dir", "data/thumbnails").as<std::string>();
    settings.videos_dir = custom_user_config.get("videos_dir", "data/videos").as<std::string>();
    settings.salt_len = custom_user_config.get("salt_len", 16).as<size_t>();

    app().setSSLConfigCommands({
            { "Options", "Compression" }
        });

    app().setFileTypes({
            "html",
            "js",
            "css",
            "ico",
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
