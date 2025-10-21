# Installation

This document provides instructions on how to install `vis` on your system using CMake or Docker.

## Dependencies

Before you can build `vis`, you need to ensure that the following dependencies are installed on your system:

*   A C99 compliant compiler (e.g., GCC or Clang)
*   A POSIX.1-2008 compatible environment
*   [CMake](https://cmake.org/)
*   [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/)
*   [libtermkey](http://www.leonerd.org.uk/code/libtermkey/)
*   [curses](https://en.wikipedia.org/wiki/Curses_(programming_library)) (ncurses is recommended)
*   [Lua](http://www.lua.org/) >= 5.2
*   [LPeg](http://www.inf.puc-rio.br/~roberto/lpeg/) >= 0.12 (runtime dependency for syntax highlighting)
*   [TRE](http://laurikari.net/tre/) (optional, for more memory efficient regex search)

## Using CMake

1.  Create a build directory:

    ```bash
    mkdir build && cd build
    ```

2.  Run CMake:

    ```bash
    cmake ..
    ```

    You can pass options to CMake to configure the build. For example, to disable Lua support:

    ```bash
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr -DENABLE_LUA=OFF
    ```

3.  Build the project:

    ```bash
    make
    ```

4.  Install:

    ```bash
    sudo make install
    ```

## Docker

If you have Docker installed, you can use it to build and run `vis`.

### Building the Docker image

You can build the Docker image using `docker-compose`:

```bash
docker-compose build
```

### Running the build

To compile the project inside a Docker container, run:

```bash
docker-compose run --rm build
```

This will create a `vis` executable in the `build` directory.

# Uninstalling

To uninstall `vis`, run the following command from the build directory:

```bash
sudo make uninstall
```