# SingleApplication

[![CI](https://github.com/itay-grudev/SingleApplication/workflows/CI:%20Build%20Test/badge.svg?branch=master)](https://github.com/itay-grudev/SingleApplication/actions)

This is a replacement of the QtSingleApplication for `Qt5` and `Qt6`.

Keeps the Primary Instance of your Application and kills each subsequent
instances. It can (if enabled) spawn secondary (non-related to the primary)
instances and can send data to the primary instance from secondary instances.

# [Documentation](https://itay-grudev.github.io/SingleApplication/)

You can find the full usage reference and examples [here](https://itay-grudev.github.io/SingleApplication/classSingleApplication.html).

## Usage

The `SingleApplication` class inherits from whatever `Q[Core|Gui]Application`
class you specify via the `QAPPLICATION_CLASS` macro (`QCoreApplication` is the
default). Further usage is similar to the use of the `Q[Core|Gui]Application`
classes.

You can use the library as if you use any other `QCoreApplication` derived
class:

```cpp
#include <QApplication>
#include <SingleApplication.h>

int main( int argc, char* argv[] )
{
    SingleApplication app( argc, argv );

    return app.exec();
}
```

To include the library files I would recommend that you add it as a git
submodule to your project. Here is how:

```bash
git submodule add https://github.com/itay-grudev/SingleApplication.git singleapplication
```

**Qmake:**

Then include the `singleapplication.pri` file in your `.pro` project file.

```qmake
include(singleapplication/singleapplication.pri)
DEFINES += QAPPLICATION_CLASS=QApplication
```

**CMake:**

Then include the subdirectory in your `CMakeLists.txt` project file.

```cmake
set(QAPPLICATION_CLASS QApplication CACHE STRING "Inheritance class for SingleApplication")
add_subdirectory(src/third-party/singleapplication)
target_link_libraries(${PROJECT_NAME} SingleApplication::SingleApplication)
```

Directly including this repository as a Git submodule, or even just a shallow copy of the
source code into new projects might not be ideal when using CMake.
Another option is using CMake's `FetchContent` module, available since version `3.11`.
```cmake

# Define the minumun CMake version, as an example 3.24
cmake_minimum_required(VERSION 3.24)

# Include the module
include(FetchContent)

# If using Qt6, override DEFAULT_MAJOR_VERSION
set(QT_DEFAULT_MAJOR_VERSION 6 CACHE STRING "Qt version to use, defaults to 6")

# Set QAPPLICATION_CLASS
set(QAPPLICATION_CLASS QApplication CACHE STRING "Inheritance class for SingleApplication")


# Declare how is the source going to be obtained
FetchContent_Declare(
  SingleApplication
  GIT_REPOSITORY https://github.com/itay-grudev/SingleApplication
  GIT_TAG        master
  #GIT_TAG        e22a6bc235281152b0041ce39d4827b961b66ea6
)

# Fetch the repository and make it available to the build
FetchContent_MakeAvailable(SingleApplication)

# Then simply use find_package as usual
find_package(SingleApplication)

# Finally add it to the target_link_libraries() section
target_link_libraries(ClientePOS PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Sql

    SingleApplication::SingleApplication
)

```


The library sets up a `QLocalServer` and a `QSharedMemory` block. The first
instance of your Application is your Primary Instance. It would check if the
shared memory block exists and if not it will start a `QLocalServer` and listen
for connections. Each subsequent instance of your application would check if the
shared memory block exists and if it does, it will connect to the QLocalServer
to notify the primary instance that a new instance had been started, after which
it would terminate with status code `0`. In the Primary Instance
`SingleApplication` would emit the `instanceStarted()` signal upon detecting
that a new instance had been started.

The library uses `stdlib` to terminate the program with the `exit()` function.

Also don't forget to specify which `QCoreApplication` class your app is using if it
is not `QCoreApplication` as in examples above.

## Freestanding mode

Traditionally, the functionality of this library is implemented as part of the Qt
application class. The base class is defined by the macro `QAPPLICATION_CLASS`.

In freestanding mode, `SingleApplication` is not derived from a Qt application
class. Instead, an instance of a Qt application class is created as normal,
followed by a separate instance of the `SingleApplication` class.

```cpp
#include <QApplication>
#include <SingleApplication.h>

int main( int argc, char* argv[] )
{
    // The normal application class with a type of your choice
    QApplication app( argc, argv );

    // Separate single application object (argc and argv are discarded)
    SingleApplication single( argc, argv /*, options ...*/ );

    // Do your stuff

    return app.exec();
}
```

_Note:_ With the discarded arguments and the class name that sounds like a Qt
application class without being one, this looks like a workaround – it is a
workaround. For 4.x, the single instance functionality could be moved to
something like a `SingleManager` class, which would then be used to implement
`SingleApplication`. This can't be done in 3.x, because moving
`SingleApplication::Mode` to `SingleManager::Mode` would be a breaking change.

To enable the freestanding mode set `QAPPLICATION_CLASS` to
`FreeStandingSingleApplication`. This is a fake base class with no additional
functionality.

The standalone mode allows us to use a precompiled version of this library,
because we don't need the `QAPPLICATION_CLASS` macro to define our Qt application
class at build time. Furthermore, we can use `std::optional<SingleApplication>`
to decide at runtime whether we want single application functionality or not.

Use the standard CMake workflow to create a precompiled static library version,
including CMake config files.

```bash
cmake -DQAPPLICATION_CLASS=FreeStandingSingleApplication -DSINGLEAPPLICATION_INSTALL=ON SingleApplicationDir
cmake --build .
cmake --install
```

This can be used via:

```cmake
find_package(SingleApplication REQUIRED)
target_link_libraries(YourTarget SingleApplication::SingleApplication)
```

_Note:_ The `QAPPLICATION_CLASS` macro is eliminated during CMake install.

## Instance started signal

The `SingleApplication` class implements a `instanceStarted()` signal. You can
bind to that signal to raise your application's window when a new instance had
been started, for example.

```cpp
// window is a QWindow instance
QObject::connect(
    &app,
    &SingleApplication::instanceStarted,
    &window,
    &QWindow::raise
);
```

Using `SingleApplication::instance()` is a neat way to get the
`SingleApplication` instance for binding to it's signals anywhere in your
program.

_Note:_ On Windows the ability to bring the application windows to the
foreground is restricted. See [Windows specific implementations](Windows.md)
for a workaround and an example implementation.


## Secondary Instances

If you want to be able to launch additional Secondary Instances (not related to
your Primary Instance) you have to enable that with the third parameter of the
`SingleApplication` constructor. The default is `false` meaning no Secondary
Instances. Here is an example of how you would start a Secondary Instance send
a message with the command line arguments to the primary instance and then shut
down.

```cpp
int main(int argc, char *argv[])
{
    SingleApplication app( argc, argv, true );

    if( app.isSecondary() ) {
        app.sendMessage(  app.arguments().join(' ')).toUtf8() );
        app.exit( 0 );
    }

    return app.exec();
}
```

_Note:_ A secondary instance won't cause the emission of the
`instanceStarted()` signal by default. See `SingleApplication::Mode` for more
details.*

You can check whether your instance is a primary or secondary with the following
methods:

```cpp
app.isPrimary();
// or
app.isSecondary();
```

_Note:_ If your Primary Instance is terminated a newly launched instance
will replace the Primary one even if the Secondary flag has been set.*

## Examples

There are five examples provided in this repository:

* Basic example that prevents a secondary instance from starting [`examples/basic`](examples/basic)
* An example of a graphical application raising it's parent window [`examples/calculator`](examples/calculator)
* A console application sending the primary instance it's command line parameters [`examples/sending_arguments`](examples/sending_arguments)
* A variant of `sending_arguments` where `SingleApplication`is used in freestanding mode [`examples/separate_object`](examples/separate_object)
* A graphical application with Windows specific additions raising it's parent window [`examples/windows_raise_widget`](examples/windows_raise_widget)

## Versioning

Each major version introduces either very significant changes or is not
backwards compatible with the previous version. Minor versions only add
additional features, bug fixes or performance improvements and are backwards
compatible with the previous release. See [CHANGELOG.md](CHANGELOG.md) for
more details.

## Implementation

The library is implemented with a `QSharedMemory` block which is thread safe and
guarantees a race condition will not occur. It also uses a `QLocalSocket` to
notify the main process that a new instance had been spawned and thus invoke the
`instanceStarted()` signal and for messaging the primary instance.

Additionally the library can recover from being forcefully killed on *nix
systems and will reset the memory block given that there are no other
instances running.

## License

This library and it's supporting documentation are released under
`The MIT License (MIT)` with the exception of the Qt calculator examples which
is distributed under the BSD license.
