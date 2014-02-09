/work in progress.../


NanoVG
==========

NanoVG is small antialiased vector graphics rendering library for OpenGL. It has lean API modeled after HTML5 canvas API. It is aimed to be a practical and fun toolset for building scalable user interfaces and visualizations. 

## Screenshot

![screenshot of some text rendered witht the sample program](/example/screenshot-01.png?raw=true)

Usage
=====

The NanoVG API is modeled loosely on HTML5 canvas API. If you know canvas, you're up to speed with NanoVG in no time.

## Creating drawing context

The drawing context is created using platform specific constructor function. If you're using the default OpenGL 2.0 back-end the context is created as follows:
```C
struct NVGcontext* vg = nvgCreateGL2(512, 512);
```
The two values passed to the constructor define the size of the texture atlas used for text rendering, 512x512 is a good starting point. If you're rendering retina sized text or plan to use a lot of different fonts, 1024x1024 is better choice.

Currently there are two different back-ends for NanoVG: [nanovg_gl2.h](/src/nanovg_gl2.h) for OpenGL 2.0 and [nanovg_gl3.h](/src/nanovg_gl3.h) for OpenGL 3.2 core profile.

## Drawing shapes with NanoVG

Drawing a simple shape using NanoVG consists of four steps: 1) begin a new shape, 2) define the path to draw, 3) set fill or stroke, 4) and finally fill or stroke the path.

```C
nvgBeginPath(vg);
nvgRect(vg, 100,100, 120,30);
nvgFillColor(vg, nvgRGBA(255,192,0,255));
nvgFill(vg);
```

Calling nvgBeginPath() will clear any existing paths and start drawing from blank slate. There are number of number of functions to define the path to draw, such as rectangle, rounded rectangle and ellipse, or you can use the common moveTo, lineTo, bezierTo and arcTo API to compose the paths step by step.

## Understanding Composite Paths

Because of the way the rendering backend is build in NanoVG, drawing a composite path, that is path consisting from multiple paths defining holes and fills, is a bit more involved. NanoVG uses even-odd filling rule and by default the paths are wound in counter clockwise order. Keep that in mind when drawing using the low level draw API. In order to wind one of the predefined shapes as a hole, you should call nvgPathWinding(vg, NVG_HOLE), or nvgPathWinding(vg, NVG_CV) _after_ defining the path.

``` C
nvgBeginPath(vg);
nvgRect(vg, 100,100, 120,30);
nvgCircle(vg, 120,120, 5);
nvgPathWinding(vg, NVG_HOLE);	// Mark circle as a hole.
nvgFillColor(vg, nvgRGBA(255,192,0,255));
nvgFill(vg);
```

## API Reference

See the header file [nanovg.h](/src/nanovg.h) for API reference.


# License
The library is licensed under [zlib license](LICENSE.txt)

## Links
Uses [stb_truetype](http://nothings.org) for font rendering.
Uses [stb_image](http://nothings.org) for image loading.
