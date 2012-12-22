#include "singleapplication.h"

int main(int argc, char *argv[])
{
  SingleApplication app(argc, argv);

  // Is another instance of the program is already running
  if(!app.shouldContinue())return 0;

  return app.exec();
}
