SingleApplication
=================

This is a replacement of the QSingleApplication for `Qt5`.

Keeps the Primary Instance of your Application and kills each subsequent
instances. It can (if enabled) spawn a certain number of secondary instances
(with the `--secondary` command line argument).

Usage
-----

The `SingleApplication` class inherits from whatever `Q[Core|Gui]Application`
class you specify. Further usage is similar to the use of the
`Q[Core|Gui]Application` classes.

The library uses your `Organization Name` and `Application Name` to set up a
`QLocalServer` and a `QSharedMemory` block. The first instance of your
Application is your Primary Instance. It would check if the shared memory block
exists and if not it will start a `QLocalServer` and then listen for connections
on it. Each subsequent instance of your application would check if the shared
memory block exists and if it does, it will connect to the QLocalServer to
notify it that a new instance had been started, after which it would terminate
 with status code `0`. The Primary Instance, `SingleApplication` would emit the
 `showUp()` signal upon detecting that a new instance had been started.

The library uses `stdlib` to terminate the program with the `exit()` function.

Here is an example usage of the library:

```cpp
// In your Project.pro
DEFINES += QAPPLICATION_CLASS=QGuiApplication # QApplication is the default

// main.cpp
#include "singleapplication.h"

int main( int argc, char* argv[] )
{
    QApplication::setApplicationName("{Your App Name}");
    QApplication::setOrganizationName("{Your Organization Name}");

    SingleApplication app( argc, argv );

    return app.exec();
}
```

The `Show Up` signal
------------------------

The SingleApplication class implements a `showUp()` signal. You can bind to that
signal to raise your application's window when a new instance had been started.

Note that since `SingleApplication` extends the `QApplication` class you can do
the following:

```cpp
// window is a QWindow instance
QObject::connect( &app, &SingleApplication::showUp, window, &QWindow::raise );
```

Using `QApplication::instance()` is a neat way to get the `SingleApplication`
instance anywhere in your program.

Secondary Instances
-------------------

If you want to be able to launch additional Secondary Instances (not related to
your Primary Instance) you have to enable that with the third parameter of the
`SingleApplication` constructor. The default is `0` meaning no Secondary
Instances. Here is an example allowing spawning up to `2` Secondary Instances.

```cpp
SingleApplication app( argc, argv, 2 );
```

After which just call your program with the `--secondary` argument to launch a
secondary instance.

__*Note:*__ If your Primary Instance is terminated upon launch of a new one it
will replace it as Primary even if the `--secondary` argument has been set.

*P.S. If you think this behavior could be improved create an issue and explain
why.*

Implementation
--------------

The library is implemented with a QSharedMemory block which is thread safe and
guarantees a race condition will not occur. It also uses a QLocalSocket to
notify the main process that a new instance had been spawned and thus invoke the
`showUp()` signal.

To handle an issue on `*nix` systems, where the operating system owns the shared
memory block and if the program crashes the memory remains untouched, the
library binds to the following signals and closes the program with error code =
`128 + signum` where signum is the number representation of the signal listed
below. Handling the signal is required in order to safely delete the
`QSharedMemory` block. Each of these signals are potentially lethal and will
results in process termination.

*   `SIGHUP` - `1`, Hangup.
*   `SIGINT` - `2`, Terminal interrupt signal
*   `SIGQUIT` - `3`, Terminal quit signal.
*   `SIGILL` - `4`, Illegal instruction.
*   `SIGABRT` - `6`, Process abort signal.
*   `SIGBUS` - `7`, Access to an undefined portion of a memory object.
*   `SIGFPE` - `8`, Erroneous arithmetic operation (such as division by zero).
*   `SIGSEGV` - `11`, Invalid memory reference.
*   `SIGSYS` - `12`, Bad system call.
*   `SIGPIPE` - `13`, Write on a pipe with no one to read it.
*   `SIGALRM` - `14`, Alarm clock.
*   `SIGTERM` - `15`, Termination signal.
*   `SIGXCPU` - `24`, CPU time limit exceeded.
*   `SIGXFSZ` - `25`, File size limit exceeded.


License
-------
This library and it's supporting documentation are released under `The MIT License (MIT)`.
