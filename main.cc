#include <drogon/drogon.h>
#include <controllers/PageController.h>
#include <controllers/ResourceController.h>
int main() {
    drogon::app()
        .loadConfigFile("config.json")
        .run();
    return 0;
}
