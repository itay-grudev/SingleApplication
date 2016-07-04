Changelog
=========

__v2.4__
--------

*   Stability improvements
*   Support for secondary instances.
*   The library now recovers safely after the primary process has crashed
    and the shared memory had not been deleted.

__v2.3__
--------

*   Improved pimpl design and inheritance safety.

    _Vladislav Pyatnichenko_

__v2.2__
--------

*   The `QAPPLICATION_CLASS` macro can now be defined in the file including the
Single Application header or with a `DEFINES+=` statement in the project file.

__v2.1__
--------

*   A race condition can no longer occur when starting two processes nearly
    simultaneously.

    Fix issue [#3](https://github.com/itay-grudev/SingleApplication/issues/3)

__v2.0__
--------

*   SingleApplication is now being passed a reference to `argc` instead of a
    copy.

    Fix issue [#1](https://github.com/itay-grudev/SingleApplication/issues/1)

*   Improved documentation.
