## Seam Carving
Seam carving, also known as liquid rescaling, is a content-aware image resizing technique developed by Shai Avidan and Ariel Shamir. Unlike traditional resizing methods that uniformly scale an image, seam carving intelligently adjusts image dimensions by identifying and removing or inserting seams—paths of least importance—to preserve essential content.

**How Seam Carving Works:**

1. **Energy Calculation:** The algorithm assigns an energy value to each pixel, typically based on measures like gradient magnitude, to determine its importance.
2. **Seam Identification:** A seam is a connected path of pixels extending from top to bottom (vertical seam) or left to right (horizontal seam), with one pixel in each row or column. The algorithm identifies seams with the lowest cumulative energy, indicating areas of least significance.
3. **Seam Removal or Insertion:** To reduce image size, the algorithm removes low-energy seams. Conversely, to enlarge the image, it inserts seams by duplicating existing low-energy paths, thereby preserving the image's critical features.

**Applications of Seam Carving:**

- **Image Retargeting:** Adjusting images to fit various display sizes and aspect ratios without distorting important content.
- **Object Removal:** Eliminating unwanted objects from photographs by removing seams that pass through them.
- **Video Processing:** Extending the technique to video retargeting by considering both spatial and temporal dimensions to maintain visual coherence.

 
## Sobel operator

The **Sobel operator** is a discrete differentiation operator used in image processing to detect edges in images. It is a type of **edge detection filter** that highlights regions of an image where there is a rapid intensity change, making it useful for detecting boundaries or transitions between different regions in an image.

The Sobel operator applies two convolution kernels (one for detecting edges in the horizontal direction and one for detecting edges in the vertical direction) to the image. The results of these convolutions are then combined to find the overall gradient magnitude, which indicates the strength of an edge at each pixel.

The two Sobel kernels are:

1. **Horizontal edge detection kernel (Gx)**:
   $$
   G_x = \begin{pmatrix}
   1 & 0 & -1 \\
   2 & 0 & -2 \\
   1 & 0 & -1
   \end{pmatrix}
   $$
   This kernel detects edges in the **horizontal direction**, i.e., it highlights vertical edges in the image.

2. **Vertical edge detection kernel (Gy)**:
   $$
   G_y = \begin{pmatrix}
   1 & 2 & 1 \\
   0 & 0 & 0 \\
   -1 & -2 & -1
   \end{pmatrix}
   $$
   This kernel detects edges in the **vertical direction**, i.e., it highlights horizontal edges in the image.

### Convolution:
- The kernels are convolved with the image (i.e., they are slid over the image to compute the weighted sum of the pixels around each position).
- The **horizontal gradient (Gx)** and **vertical gradient (Gy)** are computed at each pixel.
  
For each pixel in the image, the resulting gradients $G_x$ and $G_y$ are calculated by performing a dot product between the kernel and the neighborhood of the pixel (a 3x3 region around the pixel).

### Gradient Magnitude:
The gradient magnitude at each pixel is calculated using the following formula:
$$
\text{Magnitude} = \sqrt{G_x^2 + G_y^2}
$$
This gives the strength of the edge at each point. Larger values indicate a stronger edge.

### Direction of the edge:
The direction of the edge can also be computed by:
$$
\text{Direction} = \text{atan2}(G_y, G_x)
$$
This gives the angle of the edge at each pixel.


### References:
- [Seam Carving - Wikipedia](https://en.m.wikipedia.org/wiki/Seam_carving)
- [Seam carving for content-aware image resizing - paper](https://dl.acm.org/doi/10.1145/1275808.1276390)
- [Sobel Operator - Wikipedia](https://en.wikipedia.org/wiki/Sobel_operator)
