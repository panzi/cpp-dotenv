# C++ Implementation of `dotenv` NPM Package

This is an attempt to replicate the exact parsing behavior of the
[dotenv](https://www.npmjs.com/package/dotenv) npm package in C++. It only
implements the parser of the dotenv package, not of the dotenv-expand package,
which would add variable substitution.

In particular this is (or attempts to be) compatible with this version of
[main.js](https://github.com/motdotla/dotenv/blob/8ab33066f90a20445d3c41e4fafba6c929c5e1a5/lib/main.js).

This was written so it maybe could be included in NodeJS as a better compatible
version of the dotenv parser. See [this issue](https://github.com/nodejs/node/issues/53461)
for context.

Compile and run a test that compares the output of the JavaScript dotenv
implementation with this one:

```bash
make test
```

Only compile debug build:

```bash
make
```

Produces the file: `target/debug/dotenv`

Release build:

```bash
make BUILD_TYPE=release
```

Produces the file: `target/release/dotenv`

This includes a simple starter program. Usage:

```plain
usage: ./build/debug/dotenv [--file=PATH] [--replace] [--overwrite] [--] [command args...]

Positional arguments:
    command    The command to run.
               If no command is provided the constructed environment will be printed.
    args...    Arguments to the command.

Options:
    -f, --file=PATH    Use this file instead of ".env".
    -r, --replace      Construct an entirely new environment.
    -o, --overwrite    Overwrite already defined environment variables.
    -0, --print0       If the environment is printed use NUL bytes instead of new lines.
```
