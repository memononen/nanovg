Font Stash
==========

Font stash is light-weight online font texture atlas builder written in C. It uses [stb_truetype](http://nothings.org) to render fonts on demand to a texture atlas.

The code is split in two parts, the font atlas and glyph quad generator [fontstash.h](/src/fontstash.h), and an example OpenGL backend ([glstash.h](/glstash.h).

## Screenshot

![screenshot of some text rendered witht the sample program](/screenshots/screen-01.png?raw=true)

## Example
``` C
// Create stash for 512x512 texture, our coordinate system has zero at top-left.
struct FONSparams params;
memset(&params, 0, sizeof(params));
params.width = 512;
params.height = 512;
params.flags = FONS_ZERO_TOPLEFT;
glstInit(&params);
struct FONScontext* fs = fonsCreate(&params);

// Add font to stash.
int fontNormal = fonsAddFont(fs, "DroidSerif-Regular.ttf");

// Render some text
float dx = 10, dy = 10;
unsigned int white = glstRGBA(255,255,255,255);
unsigned int brown = glstRGBA(192,128,0,128);

struct fontstash_style styleBig = { FONT_NORMAL, 124.0f, white };

fonsSetFont(fs, fontNormal);
fonsSetSize(fs, 124.0f);
fonsSetColor(fs, white);
fonsDrawText(fs, dx,dy,"The big ", &dx);

fonsSetSize(fs, 24.0f);
fonsSetColor(fs, brown);
fonsDrawText(fs, dx,dy,"brown fox", &dx);
```

## Using Font Stash in your project

In order to use fontstash in your own project, just copy fontstash.h, stb_truetype.h, and potentially glstash.h to your project.
In one C/C++ define FONTSTASH_IMPLEMENTATION before including the library to expand the font stash implementation in that file.

``` C
#include <stdio.h>					// malloc, free, fopen, fclose, ftell, fseek, fread
#include <string.h>					// memset
#define FONTSTASH_IMPLEMENTATION	// Expands implementation
#include "fontstash.h"
```

``` C
#include <GLFW/glfw3.h>				// Or any other GL header of your choice.
#define GLSTASH_IMPLEMENTATION		// Expands implementation
#include "glstash.h"
```

## Creating new rendering backend

The default rendering backend uses OpenGL to render the glyphs. If you want to render the text using some other API, or want tighter integration with your code base you can write your own rendering backend. Take a look at the [glstash.h](/src/glstash.h) for reference implementation.

The rendering interface FontStash assumes access to is defined in the FONSparams structure. The call to `glstInit()` fills in variables.

```C
struct FONSparams {
	...
	void* userPtr;
	int (*renderCreate)(void* uptr, int width, int height);
	void (*renderUpdate)(void* uptr, int* rect, const unsigned char* data);
	void (*renderDraw)(void* uptr, const float* verts, const float* tcoords, const unsigned int* colors, int nverts);
	void (*renderDelete)(void* uptr);
};
```

- **renderCreate** is called to create renderer for specific API, this is where you should create a texture of given size.
	- return 1 of success, or 0 on failure.
- **renderUpdate** is called to update texture data
	- _rect_ describes the region of the texture that has changed
	- _data_ pointer to full texture data
- **renderDraw** is called when the font triangles should be drawn
	- _verts_ pointer to vertex position data, 2 floats per vertex
	- _tcoords_ pointer to texture coordinate data, 2 floats per vertex
	- _colors_ pointer to color data, 1 uint per vertex (or 4 bytes)
	- _nverts_ is the number of vertices to draw
- **renderDelete** is called when the renderer should be deleted
- **userPtr** is passed to all calls as first parameter

FontStash uses this API as follows:

```
fonsDrawText() {
	foreach (glyph in input string) {
		if (internal buffer full) {
			updateTexture()
			render()
		}
		add glyph to interal draw buffer
	}
	updateTexture()
	render()
}
```

The size of the internal buffer is defined using `FONS_VERTEX_COUNT` define. The default value is 1024, you can override it when you include fontstash.h and specify the implementation:

``` C
#define FONS_VERTEX_COUNT 2048
#define FONTSTASH_IMPLEMENTATION	// Expands implementation
#include "fontstash.h"
```

## Compiling

In order to compile the demo project, your will need to install [GLFW](http://www.glfw.org/) to compile.

FontStash example project uses [premake4](http://industriousone.com/premake) to build platform specific projects, now is good time to install it if you don't have it already. To build the example, navigate into the root folder in your favorite terminal, then:

- *OS X*: `premake4 xcode4`
- *Windows*: `premake4 vs2010`
- *Linux*: `premake4 gmake`

See premake4 documentation for full list of supported build file types. The projects will be created in `build` folder. An example of building and running the example on OS X:

```bash
$ premake4 gmake
$ cd build/
$ make
$ ./example
```

# License
The library is licensed under [zlib license](LICENSE.txt)

## Links
Uses [stb_truetype](http://nothings.org) for font rendering.
