# Langton's Ant 
A Langton's Ant implementation using C and SDL2.

# Compiling
You must have SDL2 library installed. If you don't have it in your default include path, you must specify it with the `-I` flag.

```
gcc -Wall -Wextra -O3 ant.c lodepng.c -o ant `sdl2-config --cflags --libs`
```
