# notifys

A notify server with an HTTP interface.

# Compile

```bash
cc -Wall -Wextra -Wpedantic main.c notify.c server.c -o notifys
```

# Usage

Watch some folder:
```bash
$ ./notifys localhost 9000 /tmp/test/
watching /tmp/test/
```

Call the long polling HTTP interface
```bash
$ curl -w"\n" http://localhost:9000
```

When a file is created or deleted in the watched folder, for example:
```bash
$ touch /tmp/test/foo
```

The HTTP client receives an answer:
```bash
$ curl -w"\n" http://localhost:9000
{"event": "IN_CREATE","file":"/tmp/test/toto"}
```
