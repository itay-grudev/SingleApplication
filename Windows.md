# Windows Specifics

## Setting the foreground window

In the `instanceStarted()` example in the `README` we demonstrated how an
application can bring it's primary instance window whenever a second copy
of the application is started.

On Windows the ability to bring the application windows to the foreground is
restricted, see [AllowSetForegroundWindow()](https://msdn.microsoft.com/en-us/library/windows/desktop/ms632668.aspx) for more details.

The background process (the primary instance) can bring its windows to the
foreground if it is allowed by the current foreground process (the secondary
instance). To bypass this `SingleApplication` must be initialized with the
`allowSecondary` parameter set to `true` .

If the widget is minimized to Windows task bar, `QWidget::raise()` or
`QWidget::show()` can not bring it to the front, you have to use Windows API
`ShowWindow()` .

You can find [demo code](examples/windows_raise_widget/main.cpp) in the examples directory.
