## Table of Contents
- [nvpfilesystem.hpp](#nvpfilesystemhpp)
- [nvpsystem.hpp](#nvpsystemhpp)
- [nvpwindow.hpp](#nvpwindowhpp)

## nvpfilesystem.hpp
### class nvp::FileSystemMonitor


Monitors files and/or directories for changes

This cross-platform wrapper does not create any threads, but is designed for
it. checkEvents() will block until either an event is generated, or cancel()
is called. See ModifiedFilesMonitor for an example.
### class nvp::FSMRunner


Adds a thread to nvp::FileSystemMonitor that repeatedly calls
nvp::FileSystemMonitor::checkEvents().
### class nvp::FSMCallbacks


Utility class to get per-path callbacks.

Make sure PathSetCallback objects returned by add() do not outlive the FSMCallbacks.
Be careful not to destroy a PathCallback during a callback.

Example:
```cpp
FSMCallbacks callbacks;

auto callbackFile1 = callbacks.add(std::vector<std::string>{"file1.txt"}, nvp::FileSystemMonitor::FSM_MODIFY,
[this](nvp::FileSystemMonitor::EventData ev) {
// Do something with file1.txt
});

auto callbackFile2 = callbacks.add(std::vector<std::string>{"file2.txt"}, nvp::FileSystemMonitor::FSM_MODIFY,
[this](nvp::FileSystemMonitor::EventData ev) {
// Do something with file2.txt
});

// When callbackFile1 goes out of scope, file1.txt stops being monitored
callbackFile1.reset()
```
### class nvp::ModifiedFilesMonitor


Monitors files and/or directories for changes.

Starts a thread at creation. Be careful to keep it in scope while events are needed.

This cross-platform wrapper does not create any threads, but is designed to be
used with one. checkEvents() will block until either an event is generated, or
cancel() is called.

Example:
```cpp
std::vector<std::string> dirs = {"shaders_bin"};
nvp::FileSystemMonitor::Callback callback = [](nvp::FileSystemMonitor::EventData ev){
g_reloadShaders = true;
};
auto fileMonitor = std::make_unique<nvp::ModifiedFilesMonitor>(dirs, callback);
```

## nvpsystem.hpp
### class NVPSystem

>  NVPSystem is a utility class to handle some basic system
functionality that all projects likely make use of.
It does not require any window to be opened.
Typical usage is calling init right after main and deinit
in the end, or use the NVPSystem object for that.
init
- calls glfwInit and registers the error callback for it
- sets up and log filename based on projectName via nvprintSetLogFileName

## nvpwindow.hpp
### class NVPWindow

>  base class for a window, to catch events
Using and deriving of NVPWindow base-class is optional.
However one must always make use of the NVPSystem
That takes care of glfwInit/terminate as well.
