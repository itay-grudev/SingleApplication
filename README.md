SingleApplication
=================

This is a replacement of the QSingleApplication for Qt5.

Usage
-----

The library uses your ```Organization Name``` and ```Application Name``` to set up a Local Server on which the first instance of your application would listen on and each subsequent instance would connect and then exit.

I also used ```stdlib``` to terminate the program with it's ```exit()``` function.

Here is an example usage of the library:
```
#include "singleapplication.h"

int main(int argc, char *argv[])
{
    QApplication::setApplicationName("{Your App Name}");
    QApplication::setOrganizationName("{Your Organization Name}");
    
    SingleApplication app(argc, argv);

    return app.exec();
}
```

The ```Show Up``` signal
------------------------
The SingleApplication class implements a ```showUp()``` signal. You can bind to that signal to raise your application's window, for example. This way when someone tries to run your application twice he would end up with the primary instance's Main Window and the new instance would be terminated.

License
-------
This library and it's supporting documentation are released under ```The MIT License (MIT)```.
