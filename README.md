[![Build Status](https://travis-ci.org/swojtasiak/jsrdbg.svg?branch=master)](https://travis-ci.org/swojtasiak/jsrdbg)

JavaScript Remote Debugger
==========================

The following chapters describe some of the most important features of the project.

## What it is for?

This is an implementation of a high level debugging protocol for SpiderMonkey
engine which is available as a shared library. The library can be used to
integrate debugging facilities into an existing application leveraging
SpiderMonkey engine. There are several integration possibilities including
exposition of the high level debugger API locally directly to the application
and even exposing it to remote clients using full duplex TCP/IP communication.

The project consists of two main parts. The debugger engine itself in a form
of shared library and the standalone console client application used to connect
to the debugger remotely. It's a simple console based implementation whose
idea is similar to GNU debugger.

Architecture, integration and remote client are described in the following
chapters.

## Install

### Linux

#### Dependencies

Besides a working C++ compiler (supporting C++11) you need a few additional
dependencies to compile jsrdbg.

On Fedora:
```sh
sudo dnf install \
    autoconf \
    autoconf-archive \
    automake \
    gettext-devel \
    libtool \
    mozjs24-devel \
    readline-devel
```

On Ubuntu:
```sh
sudo apt-get install \
    autoconf \
    autoconf-archive \
    build-essential \
    gettext \
    libmozjs-24-dev \
    libreadline-dev \
    libtool \
    pkg-config
```

Note: If you haven't installed mozjs-24 with your distribution's package
manager you probably have to tell pkg-config where to find the `mozjs-24.pc`
file. You can do this by setting the `PKG_CONFIG_PATH` environment variable
appropriately, e.g.

```sh
$ PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
```

On Linux, you can build jsrdbg with the usual `./configure && make && make
install` like so

```sh
$ autoreconf -i
$ ./configure
$ make
$ sudo make install
```

### Windows

On Windows, jsrdbg provides Visual Studio project files in the `win/` directory
to create a Windows DLL. We have tested building jsrdbg with Visual Studio 2012
but it should work with any later version as well.

It is important that you checkout the win-iconv submodule that is referenced by
the jsrdbg Git repository when compiling on Windows. You can do this by either
cloning the repository as follows

```
git clone --recursive https://github.com/swojtasiak/jsrdbg.git
```

or by checking out the submodule after the fact like this:

```
git submodule init
git submodule update
```

Additionally, you need to set two environment variables to let Visual Studio
know where to find the SpiderMonkey header files at compile-time and the
SpiderMonkey import library at link-time. Make sure to set both environment
variables to the appropriate paths before opening Visual Studio.

```
set MOZJS_INCLUDE_DIR=C:\mozjs-24\include\js
set MOZJS_LIB_PATH=C:\mozjs-24\lib\mozjs-24.lib
```

Then build the solution with Visual Studio Solution as usual.

Note: On Windows, jsrdbg logs to a file by default. (On Linux, jsrdbg logs via
the syslog(3) facility, which is of course unavailable on Windows.) You can
specify the log file by setting the `JSRDBG_LOG_FILE_PATH` environment variable
before starting your program, e.g.

```
set JSRDBG_LOG_FILE_PATH=C:\Users\test\Desktop\jsrdbg.log
```

If the environment variable is not set, jsrdbg does not write any logs.

## How to integrate

There are plenty of ways how an application can be integrated with a
debugger, but the easier one is probably to integrate it with a remote debugger
which is based on TCP/IP connections. It's probably the best choice, due to the
fact that integration is really simple and the debugger is fully functional
without any additional work, which is needed in case of the local debugger
implementation. In order to integrate with the local debugger you have to
inherit from an abstract debugger implementation and provide some methods
that are exposed by the class.

As has been said, the integration process is really simple and can be
completed in a few lines of code. The working example is available in the
"example" directory in the project root directory. The example itself is
really simple, the character encoding is broken and there is almost no error
handling, but it shows how the process of integration looks like.

Having working SpiderMonkey engine embedded in your code, everything you have
to do is to create a new instance of the JSRemoteDebugger class which needs
JSRemoteDebuggerCfg instance when initialized. Notice that the debugger itself
is not connected to any specific JSRuntime and JSContext instance. It's designed
to make integration of various JSContexts as simple as possible. Everything you
have to care about is to make sure that you call debuggers methods using the
same threads that are to run JS runtimes. So it's entirely possible to use
only one debugger instance bounded to one specific TCP/IP port to handle all
JSContexts within a process.

Initializing a remote debugger instance you can provide dedicated configuration
for the remote debugger itself as well as directly for a debugger engine. That
is, two of the available implementations (local and remote) use the same high
level debugger engine under the hood, so they share the same set of
configuration options.

So being familiar with that, we can try to prepare fully configured remote
debugger instance for our own application. Let's say we need to bind it only
to the localhost and to the default TCP/IP port 8089. We also would like the
debugger to suspend as soon as a debuggee started in order to debug the script
from the beginning. Nothing easier than that:

```cpp
JSRemoteDebuggerCfg cfg;
cfg.setTcpHost("localhost");
cfg.setTcpPort(JSR_DEFAULT_TCP_PORT);
cfg.setScriptLoader(&loader);

JSRemoteDebugger dbg( cfg );
```

In the first line the new configuration object is created. It's the mentioned
dedicated configuration for the remote debugger implementation. In the second
and third line two configuration options are set. The first one is the host
we would like to listen on and the next one is the TCP/IP port. That's
basically all we have to do here. There is one more important option around:
'scriptLoader', which can be used to provide scripts source code for the
debugging engine, but let's leave it for now. The last one can be used to
choose between available protocols supported by the debugger, but currently
only TCP/IP is supported which is a default protocol, so for now this option is
useless.

In the last line an instance of remote debugger is being created for our
configuration options prepared earlier.

Having the debugger created you have to install it. In order to understand the
installation process you have to be aware of one important fact. The core of
the debugger engine is implemented in JavaScript language and it runs in the
same virtual machine which is debugged by the debugger engine. Of course
there is plenty of C++ code which handles whole communication etc., but the
debugger itself is a pure JavaScript code which uses some provided native
interfaces to exchange communicates with the C++ part. Therefore the
installation procedure of the debugger engine is neither more nor less
about creating dedicated compartment inside existing SpiderMonkey engine
and running a bootstrapping code which registers the debugger. In order to
do it you have to call a dedicated method: 'install' exposed by debugger
instance. The method gets three arguments: JSContext which we would like to
debug. A name of the context which is just a kind of identifier used on the
client level just in order to communicate with concrete context instance.
The last one holds debugger engine configuration parameters JSDbgEngineOptions.

When the installation is completed you have your debugger engine up and running,
but we need a way to do some interaction with it. Protocol mentioned before is
responsible for exposing communication interface. In our case it's TCP/IP
protocol, so the next thing we have to do is to start the TCP/IP server
handling the communication logic. Of course this logic is also hidden under a
layer of abstraction, therefore everything that has to be done is to call
another parameter-free method: 'start' on debugger instance. Remember that
the protocol exposed by the debugger is fully asynchronous one and the whole
communication is handled by a full duplex TCP/IP server. It's why there is a
new thread started in the background after calling 'start' method.

OK, that's pretty much everything you have to do in order to connect to the
debugger remotely. So let's look at the source code:

```cpp
JSDbgEngineOptions dbgOptions;
dbgOptions.suspended();

if( dbg.install( cx, 'example-js-context', dbgOptions ) != JSR_ERROR_NO_ERROR ) {
    cout << "Cannot install debugger." << endl;
    return false;
}

if( dbg.start() != JSR_ERROR_NO_ERROR ) {
    dbg.uninstall();
    cout << "Cannot start debugger." << endl;
    return false;
}
```

The lines first and second are responsible for providing options for the
debugger engine. Currently only three options are available. The first one
called 'suspended' can be used to start debugger in the suspended mode
mentioned before, 'continueWhenNoConnections' which should be set if we
would like to make the debugging application continue when all remote
client have disconnected and the last one 'sourceDisplacement' used to
synchronize source code position if JS engine being debugged messes up with
line numbers.

The rest of the code explains itself. Only one thing that might be really
interesting here is the model of error handling. Every method exposed by the
debugging engine returns one of the unified error codes. All the codes are
available in the following header file: jsrdbg/jsdbg_common.h. Bear in mind
that it's up to you to uninstall every context correctly before deleting the
debugger. Debugger cannot do it automatically just because every 'uninstall'
invocation has to be done on dedicated thread, the same which runs the JS
engine being unregistered.

Debugger is up and running now, but you have to add every compartment you
would like to debug into the debugger, by providing its JS global object.
It can be done using 'addDebuggee' method like as in the following code:

```cpp
if( dbg.addDebuggee( cx, global ) != JSR_ERROR_NO_ERROR ) {
    dbg.uninstall( cx );
    cout << "Cannot add debuggee." << endl;
    return false;
}
```

That's all. If you have done all the steps correctly there is much a chance
that everything works and you should be able to use jrdb client in order to
make a remote connection to the debugger.

Of curse it's a very young project, so there is always a risk of a nasty bug
or something, so do not hesitate to report everything or provide a patch if you
find something.

As was said before, there is also a local debugger available, but its
integration is quite complex. There is 'check' subproject which is used to run
all unit tests. Take a look at it in order to get a real example of its usage.

## Remote client

Inside 'jrdb' directory, there is a simple remote debugger client available. It
was designed and implemented having gdb in mind, so it's quite similar to the
gdb, but only a small subset of all gdb commands is implemented for now.

Anyway the client is not the most convenient one as it doesn't use ncurses, but
it's definitely better than nothing :-). I will probably prepare a client with
a real GUI in the future, but I don't have time for it now.

Ok, so let's start with a simple example.

First of all go to the example directory and build the provided example. It's
a very simple application which runs an embedded JS script and exposing remote
debugger instance. The application binds to all interfaces and uses the
default TCP/IP port 8089. In order to build everything go to the example
directory and run:

```sh
$ autoreconf -i
$ ./configure
$ make
```

Bear in mind that the project uses pkg-config to find libjsrdbg shared
library. So you have to install jsrdbg and make sure that `PKG_CONFIG_PATH` is set
correctly. For instance, if you are installing the project in /usr/local, then
there is always a risk that you will have to set the following environment
variable:

```sh
$ export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
```

It depends if your system handles this location by default.

Now after building the example, you have the following application in the "src"
directory: jsrdbg_example.

Just run it.

It should print the following messages into the standard output:

```
$ src/jsrdbg_example
Use jrdb command in order to connect to the debugger.
Application is suspended.
```

OK, it means that application has been started in the suspended mode described
before and is waiting for new connections. It uses the default TCP/IP port, so
everything that has to be done to connect it is to run jrdb.

First of all type --help parameter in order to get all available arguments:

```sh
$ jrdb --help
```

Then start the client:

```
$ jrdb
JavaScript Remote Debugger Client connected to a remote debugger.
Waiting for a list of JavaScript contexts being debugged.
Type "help context" for more information.
Available JSContext instances:
0) example-JS (PAUSED)
There is only one JSContext managed by the debugger, so it has been chosen as
the current one. You can change it using the "context" command.
jrdb>
```

As you can see the client informed us that debugger we are connected to
provides one paused JSContext which can be debugged. As there is only one
context, it has been chosen as the current one by default. Type 'help
context' in order to get more comprehensive explanation.

The jsrd> prompt means that the client is waiting for commands. So try to type
the most important one:

```
jrdb> help
List of available commands.

set - Sets environment variables
env - Prints environment variables
pc - Prints information about current frame
step - Steps to a next instruction
next - Steps to a next instructions going through subroutines
finish - Finishes the execution of the current function body
continue - Continues execution of the program being debugged
stop - Terminates a debuggee.
break,pause - Pauses the program being debugged or sets breakpoints
delete - Deletes breakpoints
info - List information about program being debugged
list - Prints source code of the script being debugged
backtrace - Prints backtrace of stack frames
print - Evaluates expressions
version - Shows application version
source - Loads script source code
animation - Enables/disables stepping animation
jrdb>
```

As you can see it's not the most sophisticated one. It just prints some
information about supported commands, so it's definitely one of the most
helpful ones :-). In order to get more comprehensive description of every
command you can type its name as an argument:

```
jrdb> help list
Lists number of source code lines starting from the
location where debuggee is paused. When run first time
it requests for the whole source code of the script being
debugged and caches it. It's why first call for a particular
script can be a bit slower.
Usage:
list [n]
l [n]
[n] - Number of source lines to print.
jrdb>
```

OK, so instead of describing every command available, let's do some exercises
using the example application started a moment ago. First of all type 'pc' and
hit enter. You should see something like that:

```
jrdb> pc
Script: example.js
Line: 0
jrdb>
```

It means that we are paused inside the "example.js" script at line 0. Ok, so it
would be definitelly helpful if we have an opportunity to see a piece of source
code at this location. Nothing easier than that:

```
jrdb> l
 0* debugger;
 1  print('Hello. ');
 2  (function(fn) {
 3      print('Yes, ');
 4      fn('this is dog.');
 5  })(function(msg) {
 6      print(msg);
 7  });
 8  print('Woof, woof.');
 9  debugger;
jrdb>
```

The 'list' command can be used to show a number of source lines just after the
point where debugger is currently paused. The asterisk after the line number
tells us where we are paused. In this particular case we are paused on
'debugger' statement which is quite interesting one, because it can be used
directly in the code as a breakpoint in order to make the debugger pause the
application. OK, so let's step through the next few instructions.

```
jrdb> s
1 print('Hello. ');
jrdb> s
2 (function(fn) {
jrdb> s
5 })(function(msg) {
jrdb> s
2 (function(fn) {
jrdb> s
3     print('Yes, ');
jrdb> l
 3*     print('Yes, ');
 4      fn('this is dog.');
 5  })(function(msg) {
 6      print(msg);
 7  });
 8  print('Woof, woof.');
 9  debugger;
10
jrdb>
```

As you can see we have gone through a few source code lines using 'step'
instruction and after doing it we are paused at the third line. There is
one argument to the function we are in. It's 'fn'. Let's see what it is.
It can be done using print function, which is used to evaluate code:

```
jrdb> p fn
{
  "___jsrdbg_function_desc___": {
    "parameterNames": [
      "msg"
    ]
  },
  "prototype": {
    "___jsrdbg_collapsed___": true
  },
  "length": 1,
  "name": "",
  "arguments": null,
  "caller": null
}
jrdb>
```

So it's an anonymous function with one argument 'msg'.

Hmm, 'print' evaluates code. So let's try to do something strange:

```
jrdb> p fn = function() { debugger; };
{
  "___jsrdbg_function_desc___": {
    "displayName": "fn",
    "parameterNames": []
  },
  "prototype": {
    "___jsrdbg_collapsed___": true
  },
  "length": 0,
  "name": "",
  "arguments": null,
  "caller": null
}
jrdb> c
debugger eval code
1
jrdb>

```

What has just happened? It's a bit tricky but in fact very simple. The mentioned
print method is used to evaluate the code as it is. So by evaluating the
following instruction: 'fn = function() { debugger; };' we replaced the
original function passed to the method using the 'fn' argument by a new one,
which does only one thing pauses when executed. It's why the debugger paused
in the evaluated code after we continued execution using 'continue' command.
So as you can see you have really pretty much control over the debugged code.
But remember that with great power comes great responsibility, it's not really
hard to break the whole debugged application using such evaluation. OK, let's
go out from the evaluated code. Remember that you can display all local
variables using 'info locals' command, but head over to the 'help' description,
because it can be a quite tricky instruction and you probably will be
interested how to control the depth of displayed variables.

That's should be enough for now. With such a knowledge you should be able to
play with the debugger on your own using 'help' command.


```
jrdb> s
1
jrdb> s
example.js
4     fn('this is dog.');
jrdb>
```

Some interesting commands:

*set debug=true* - Enables debug mode. All sent and received packets will be
displayed in the console as well as log messages from the client.

*animation* - If enabled, every time when client receives information that
debugger has paused, it displays bigger source context and highlights the line
where debugger is paused. For instance:

```
jrdb> s
 0  debugger;
 1> print('Hello. ');
 2  (function(fn) {
 3      print('Yes, ');
 4      fn('this is dog.');
 5  })(function(msg) {
 6      print(msg);
 7  });
 8  print('Woof, woof.');
 9  debugger;
10
jrdb> s
 0  debugger;
 1  print('Hello. ');
 2> (function(fn) {
 3      print('Yes, ');
 4      fn('this is dog.');
 5  })(function(msg) {
 6      print(msg);
 7  });
 8  print('Woof, woof.');
 9  debugger;
10
jrdb>

```

*source* - Returns list of available script files.

```
jrdb> source
/home/tas/workspace_gnome/gjs/examples/gtk.js
resource:///org/gnome/gjs/modules/overrides/GLib.js
resource:///org/gnome/gjs/modules/lang.js
```

## Protocol

The whole protocol is based on full duplex TCP/IP communication. Almost every
command and response is encoded using JSON. So after implementing transport
layer everything you have to do to communicate with a debugger is to send and
receive JSON encoded objects. For instance the following packets show how a few
step commands look like on the protocol level:

```
Sending command: {"id":1,"type":"command","name":"step"}
Got packet: {"type":"info","subtype":"paused","url":"test_script.js","line":5,
"source":"Utils.sum = function(x, y) {"}
Sending command: {"id":2,"type":"command","name":"step"}
Got packet: {"type":"info","subtype":"paused","url":"test_script.js","line":12,
"source":"Manager.prototype = {"}
Sending command: {"id":3,"type":"command","name":"step"}
Got packet: {"type":"info","subtype":"paused","url":"test_script.js","line":13,
"source":"    calculate: function( x ) {"}
Sending command: {"id":4,"type":"command","name":"step"}
Got packet: {"type":"info","subtype":"paused","url":"test_script.js","line":20,
"source":"    show: function(x) {"}

"source":"manager.calculate(5);"}
```

This chapter describes all available commands and responses, so after reading it
you should be able to implement a client application for the debugger.

There are also two special commands, which are not encoded using JSON. These
are: get_available_contexts and exit.

### Request

Request is normalized and consists of following parts:

*[context id]/(JSON PACKET)\n*

* *context id*  - Numerical identifier of the JSContext we would like to send packet to. It's optional parameter in case of non-existence the packet is sent to all the contexts.
* *JSON PACKET* - Command itself, mandatory.
* *\n*          - Commands are separated by new-line characters.

JSON packet consists of the following properties:

* *type* - Currently it has to be set to 'command'.
* *name* - Command name. For instance 'step'.
* *id*   - Optional packet ID. It's just sent back with a response and can be used to pair them.

The rest of properties are optional.

Examples:
```js
{"type":"command","name":"step"}
{"type":"command","name":"set_breakpoint","breakpoint":{"url":"test.js",
"line":22}}
```

### Response

Response if normalized and consists of the following parts:

* *type* - Response type: 'error, 'info'. Info for plain responses, error for errors.
* *subtype* - Name of the response.

The rest of the properties are optional.

Examples:

```js
{"type":"info","subtype":"pc","script":"example.js","line":2,"source":null,
"id":"95D892FEC352D9AF"}
```

### Supported commands

Standard parameters described above have been omitted intentionally.

    ----

    Name: pc
    Description: Gets current program counter.
    Request:
        name             - 'pc'
        source (boolean) - True if current source line (code, not a number)
                           should be returned in response.
    Response:
        subtype  - 'pc'
        script   - Script's URL.
        line     - Line number.
        source   - A line of source code pointed by PC.

    Req: {"type":"command","name":"pc","source":false,"id":"CE2DA6CB1B6AFC7A"}
    Res: {"type":"info","subtype":"pc","script":"example.js","line":2,"source":null,
         "id":"CE2DA6CB1B6AFC7A"}

    ----

    Name: step
    Description: Steps program until it reaches a different source line.
    Request:
        name  - 'step'
    Response: None.

    Req: {"type":"command","name":"step"}

    ----

    Name: step_out
    Description: Finishes the execution of the current function body.
    Request:
        name - 'step_out'
    Response: None.

    Req: {"type":"command", "name": "step_out"}

    ----

    Name: next
    Description: Steps program proceeding through subroutines.
    Request:
        name  - 'next'
    Response: None.

    Req: {"type":"command","name":"next"}

    ----

    Name: continue
    Description: Continues execution.
    Request:
        name - 'continue'
    Response: None.

    Req: {"type":"command","name":"continue"}

    ----

    Name: stop
    Description: Terminates a debuggee.
    Request:
        name - 'stop'
    Response: None.

    Req: {"type":"command","name":"stop"}

    ----

    Name: source_code
    Description: Gets source code for given URL.
    Request:
        name  - 'get_source'
        url   - Script's URL.

    Response:
        subtype       - 'source_code'
        script        - Script's URL.
        source        - Source code.
        displacement  - Source code displacement, see debugger engine options.

    Example:
    Req: {"type":"command","name":"get_source","url":"example.js","id":
         "CCE548813F631269"}
    Res: {"type":"info","subtype":"source_code","script":"example.js","source":
         ["debugger;","..."],"displacement":0,"id":"DFF3E74BE6BF7DD7"}

    ----

    Name: delete_all_breakpoints
    Description: Deletes all registered breakpoints.
    Request:
        name - 'delete_all_breakpoints'

    Response:
        subtype - 'all_breakpoints_deleted'

    Example:
    Req: {"type":"command","name":"delete_all_breakpoints",
         "id":"6DE62B9327DDEFEA"}
    Res: {"type":"info","subtype":"all_breakpoints_deleted","id":"6DE62B9327DDEFEA"
         }

    ----

    Name: pause
    Description: Pauses debuggee as soon as possible.
    Request:
        name - 'pause'

    Response: None

    Example:
    Req: {"type":"command","name":"pause","id":"F37C8BF4BCBCEAE4"}

    ----

    Name: set_breakpoint
    Description: Registers a new breakpoint.
    Request:
        name - 'set_breakpoint'

    Response:
        subtype - 'breakpoint_set'
        bid - Numeric breakpoint identifier.
        url - Script file where breakpoint has been set.
        line - Line at which breakpoint has been set.
        pending - False if breakpoint has been already registered, true if it's a
                  pending breakpoint which waits for the script for being loaded.

    Example:
    Req: {"type":"command","name":"set_breakpoint","breakpoint":{
         "url":"/home/tas/workspace_gnome/gjs/examples/gtk.js","line":2,
         "pending":true}
    Res: {"type":"info","subtype":"breakpoint_set","bid":0,
         "url":"/home/tas/workspace_gnome/gjs/examples/gtk.js","line":2,
         "pending":false,"id":"46287654F429A626"}

    ----

    Name: delete_breakpoint
    Description: Deletes given breakpoint.
    Request:
        name - 'delete_breakpoint'
        ids - Array of numeric breakpoints identifiers to be deleted.

    Response:
        subtype - 'breakpoint_deleted'
        ids - Array of numeric breakpoints identifiers to be deleted.

    Example:
    Req: {"type":"command","name":"delete_breakpoint","ids":[1],
         "id":"9C59281AB28B5678"}
    Res: {"type":"info","subtype":"breakpoint_deleted","ids":[],
         "id":"9C59281AB28B5678"}

    ----

    Name: get_stacktrace
    Description: Gets current JSContext stacktrace.
    Request:
        name - 'get_stacktrace'

    Response:
        subtype - 'stacktrace'
        stacktrace - Array of stacktrace elements.
            url - Script's URL.
            line - Line number.
            rDepth - Stacktrace depth.

    Example:
    Req: {"type":"command","name":"get_stacktrace","id":"857B3B96591A5163"}
    Res: {"type":"info","subtype":"stacktrace","stacktrace":
         [{"url":"/home/tas/workspace_gnome/gjs/examples/gtk.js","line":96,
         "rDepth":0}],"id":"857B3B96591A5163"}

    ----

    Name: get_variables
    Description: Get list of variables from given frame.
    Request:
        name - 'get_stacktrace'
        query - Query.
            depth - Frame depth.
            options - Retrieving options.
                show-hierarchy - Evaluates hierarchy of variables.
                evaluation-depth - How depth to evaluate.

    Response:
        subtype - 'variables'
        variables - Array of stack elements.
            stackElement - Describes variables for given frame.
                url - Script's URL.
                line - Source line.
                rDepth - Frame depth.
                variables - Array of variables.
                    name - Name of a variable.
                    value - Value of the variable.

    Response:
        subtype - 'stacktrace'
        stacktrace - Array of stacktrace elements.
            url - Script's URL.
            line - Line number.
            rDepth - Stacktrace depth.

    Example:
    Req: {"type":"command","name":"get_variables","query":{"depth":0,"options":{
         "show-hierarchy":true,"evaluation-depth":1}},"id":"63DFE5D533FD5EB4"}
    Res: {"type":"info","subtype":"variables","variables":[{
        "stackElement":{"url":"/home/tas/workspace_gnome/gjs/examples/gtk.js",
        "line":96,"rDepth":0},"variables":[{"name":"name","value":1}}]}],
        "id":"63DFE5D533FD5EB4"}

    ----

    Name: evaluate
    Description: Evaluates variable.
    Request:
        name - 'evaluate'
        path - Code to evaluate.
        options - Retrieving options.
            show-hierarchy - Evaluates hierarchy of variables.
            evaluation-depth - How depth to evaluate.

    Response:
        subtype - 'evaluated'
        result - Evaluation result.

    Example:
    Req: {"type":"command","name":"evaluate","path":"toString","options":
         {"show-hierarchy":true,"evaluation-depth":1},"id":"5791287CABE43BF6"}
    Res: {"type":"info","subtype":"evaluated","result":{
         "___jsrdbg_function_desc___":{"displayName":"toString","name":"toString",
         "parameterNames":[]},"length":0,"name":"toString","arguments":null,
         "caller":null},"id":"5791287CABE43BF6"}

    ----

    Name: get_all_source_urls
    Description: Get list of source scripts handled by a debugger.
    Request:
        name - 'get_all_source_urls'

    Response:
        subtype - 'all_source_urls'
        urls - Array of script URLs.

    Example:
    Req: {"type":"command","name":"get_all_source_urls","id":"AD9CD9D1C76D8EC4"}
    Res: {"type":"info","subtype":"all_source_urls","urls":
         ["/home/tas/workspace_gnome/gjs/examples/gtk.js","debugger eval code",
         "resource:///org/gnome/gjs/modules/overrides/GLib.js",
         "resource:///org/gnome/gjs/modules/lang.js"],"id":"AD9CD9D1C76D8EC4"}

    ----

    Name: get_breakpoints
    Description: Gets list of all registered breakpoints.
    Request:
        name - 'get_breakpoints'

    Response:
        subtype - 'breakpoints_list'
        breakpoints - Array of available breakpoints.
            bid - Numeric breakpoint identifier.
            url - Script file where breakpoint has been set.
            line - Line at which breakpoint has been set.
            pending (boolean) - False if breakpoint has been already registered,
                      true if it's a pending breakpoint which waits for the script
                      for being loaded.

    Example:
    Req: {"type":"command","name":"get_breakpoints","id":"3E5D8A1A168E11F5"}
    Res: {"type":"info","subtype":"breakpoints_list","breakpoints":[{"bid":0,
         "url":"/home/tas/workspace_gnome/gjs/examples/gtk.js","line":2,
         "pending":false}],"id":"3E5D8A1A168E11F5"}

    ----

    Name: get_available_contexts
    Description: Gets list of all handled JSContexts.
    Request: Plain string: get_available_contexts\n
    Response:
        subtype  - 'contexts_list'
        contexts - Array of contexts.
            contextId - Numerical context ID.
            contextName - Context name, the same which has to be passed to the
                          install method.
            paused (boolean ) - True if context is already paused.

    Res: {"type":"info","subtype":"contexts_list","contexts":[{"contextId":0,
         "contextName":"example-JS","paused":true}]}

    ----

    Name: server_version
    Description: Gets the version of the remote debugger.
    Request: Plain string: server_version\n
    Response:
        subtype  - 'server_version'
        version - String representing the git tag, commit etc.

    Res: {"type":"info","subtype":"server_version","version":"0.0.7-16-gd6de3b3"}

    ----

    Name: exit
    Description: Closes debugger.
    Request: Plain string: exit\n

    Common packets:

    Packed sent every time when debugger switches to PAUSED state.

    {"type":"info","subtype":"paused",
    "url":"/home/tas/workspace_gnome/gjs/examples/gtk.js",
    "line":2,"source":"const Gtk = imports.gi.Gtk;"}

    source - Source code line.
    line - Line number.
    url - Script's URL.

    Packed sent every time when error occurred.
    {"type":"error","message":"Error message.","code":1,"id":"65855F75466DA4A6"}

    message - Textual error message.
    code - One of the supported error codes:

    ERROR_CODE_UNKNOWN_COMMAND       = 1
    ERROR_CODE_NO_COMMAND_NAME       = 2
    ERROR_CODE_NOT_A_COMMAND_PACKAGE = 3
    ERROR_CODE_NOT_PAUSED            = 4
    ERROR_CODE_BAD_ARGS              = 5
    ERROR_CODE_SCRIPT_NOT_FOUND      = 6
    ERROR_CODE_CANNOT_SET_BREAKPOINT = 8
    ERROR_CODE_IS_PAUSED             = 9
    ERROR_CODE_UNEXPECTED_EXC        = 10
    ERROR_CODE_EVALUATION_FAILED     = 11
    ERROR_CODE_PC_NOT_AVAILABLE      = 12
    ERROR_CODE_NO_ACTIVE_FRAME       = 13
