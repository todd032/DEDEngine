#include <cstdlib>
#include <iostream>

#include "viewer/main.hpp"

int main()
{
    cotrx::viewer::ViewerApp app;
    if (!app.Initialize())
    {
        std::cerr << "viewer initialization failed\n";
        return EXIT_FAILURE;
    }

    app.Run();
    app.Shutdown();
    return EXIT_SUCCESS;
}
