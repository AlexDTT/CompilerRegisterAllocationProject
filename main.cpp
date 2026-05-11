#include "ui/RegisterAllocApp.h"

/**
 * @brief Program entry point.
 * @param argc Number of command-line arguments.
 * @param argv Command-line argument vector.
 * @return Process exit code returned by RegisterAllocApp.
 * @complexity O(App), where App is the selected interactive or batch workflow.
 */
int main(int argc, char *argv[])
{
  RegisterAllocApp app;
  return app.run(argc, argv);
}
