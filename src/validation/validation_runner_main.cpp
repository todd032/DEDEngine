#include <cstdlib>
#include <iostream>

#include "validation/scenario_runner.hpp"

int main()
{
    const auto results = cotrx::validation::RunAllScenarios();

    auto allPassed = true;
    for (const auto& result : results)
    {
        std::cout << result.scenarioName << ": "
                  << (result.success ? "PASS" : "FAIL")
                  << " (json=" << result.jsonPath << ")\n";
        if (!result.success)
        {
            allPassed = false;
            for (const auto& message : result.assertionMessages)
            {
                std::cout << "  - " << message << "\n";
            }
        }
    }

    return allPassed ? EXIT_SUCCESS : EXIT_FAILURE;
}
