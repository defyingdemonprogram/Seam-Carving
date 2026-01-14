# Seam Carving

**Seam carving** is a *content-aware image resizing* technique. Rather than uniformly scaling an image, it changes the image’s dimensions by **removing or inserting paths of pixels that contribute least to the visual content**.

### Core Concept

A **seam** is a continuous path of connected pixels:

* **Vertical seam**: one pixel per row, connected from top to bottom
* **Horizontal seam**: one pixel per column, connected from left to right

Seams are chosen so they pass through pixels with **low visual importance**, allowing the image to be resized while preserving key features.

## Quick Start

Build and run the program:

```bash
cc -o nob nob.c   # one-time build
./nob images/tower.jpg output.jpg
```

### How It Works

1. **Energy Map Calculation**

   * Each pixel is assigned an *energy* value, commonly based on image gradients.
   * High energy indicates edges, textures, or important details.
   * Low energy corresponds to flat or less noticeable regions like skies or walls.

2. **Seam Identification**

   * Dynamic programming is used to find a seam with the **lowest total energy**.

3. **Seam Removal or Insertion**

   * Removing a seam reduces the image size.
   * Inserting a seam increases the image size.

4. **Iteration**

   * Since each seam alters the image, the process is repeated until the target dimensions are reached.

### Why Seam Carving Works Better Than Simple Resizing

**Traditional resizing**:

* Scales all pixels equally, often distorting important objects.

**Seam carving**:

* Preserves visually important content.
* Removes mostly background or low-detail pixels.

### Intuitive Example

Consider shrinking an image of a person standing against the sky:

* **Standard resizing** makes the person appear thinner.
* **Seam carving** removes pixels from the sky first, maintaining the person’s proportions.

### Limitations

Seam carving may produce distortions when images contain:

* Dense textures throughout
* Repeating patterns
* Multiple important objects close together

Excessive seam removal can also introduce visible artifacts.

### References

* [stb_image.h – GitHub](https://github.com/nothings/stb/blob/master/stb_image.h)
* [stb_image_write.h – nothing.com]()
