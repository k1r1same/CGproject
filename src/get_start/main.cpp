#include <cstdlib>
#include <iostream>

#include "scene.h"

Options getOptions(int argc, char* argv[]) {
    Options options;
    options.windowTitle = "必死无疑";
    options.windowWidth = 1920;
    options.windowHeight = 1080;
    options.windowResizable = false;
    options.vSync = true;
    options.msaa = true;
    options.glVersion = {3, 3};
    options.backgroundColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    options.assetRootDir = "../../media/";

    return options;
}

int main(int argc, char* argv[]) {
    Options options = getOptions(argc, argv);

    try {
        Scene app(options);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        exit(EXIT_FAILURE);
    } catch (...) {
        std::cerr << "Unknown exception" << std::endl;
        exit(EXIT_FAILURE);
    }

    return 0;
}