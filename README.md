# 🧬 SCT-Center Color Segmentation for Melanoma Diagnosis

This project implements an interactive image segmentation application in **C++ using OpenCV**, based on the **SCT-Center color space transform**. The system is tailored for medical image preprocessing and was developed to assist in identifying skin cancer regions (melanoma) by extracting the region of interest (ROI) from dermatoscopic images.

---

## 🧠 Overview

The application allows the user to load a color image and interactively segment the region of interest using a combination of:

- Median filtering
- SCT-Center color quantization
- Cluster-based pixel selection
- Morphological operations (erosion & dilation)
- Final ROI computation (area and percentage)

> 📌 This tool was developed for a medical imaging project focused on skin cancer (melanoma) analysis using color space transformations and human-in-the-loop refinement.

---

## 🖼️ Interface Example

<p align="center">
  <img src="./Screenshot 2025-06-05 083727.png" alt="SCT Segmentation UI" width="700"/>
</p>

---

## 📌 Algorithm Steps

### 🔹 Step 1: Preprocessing
- Apply **3x3 median filter** independently on each RGB channel to remove acquisition noise.
- (Optional) Avoid RGB quantization to 256 colors to preserve segmentation fidelity.

### 🔹 Step 2: SCT-Center Color Transform
- Transform the image from RGB to **SCT (Spherical Coordinate Transform)**:
  - \( L = \sqrt{R^2 + G^2 + B^2} \)
  - \( A = \text{atan2}(\sqrt{G^2 + R^2}, B) \)
  - \( B = \text{atan2}(G, R) \)
- Quantize angles A and B to a grid (e.g. 15x15 = 225 color classes)
- Remap segmented SCT colors back to RGB

### 🔹 Step 3: Interactive Clustering
- User clicks on a region of interest in the image
- A **24³ cube** is generated in the RGB space around that color
- All pixels inside the cube are marked (in blue)
- This step can be repeated until the ROI is fully segmented

### 🔹 Step 4: Morphological Filtering
- Apply **erosion + dilation** using 3x3 cross-shaped structuring elements
- Eliminates noise and enhances object boundaries
- Outputs a binarized image (ROI in black, background in white)

### 🔹 Step 5: ROI Area Calculation
- Calculate:
  - Number of pixels marked as ROI
  - Percentage of image occupied by ROI
- Display results in the console

---

## 💡 Key Benefits

- ✅ Precise control via interactive clustering
- ✅ Robust to illumination variance due to SCT transformation
- ✅ Flexible and adjustable parameters for experimentation
- ✅ Easy-to-use GUI with click-based segmentation
- ✅ Suitable for medical image preprocessing pipelines

---

## 🛠️ Technologies Used

- **Language**: C++  
- **Library**: OpenCV  
- **Concepts**: Color space transforms, clustering, morphological operations, interactive GUI

---

## 🏁 How to Run

1. Clone the repository:
```bash
git clone https://github.com/yourusername/sct-color-segmentation.git
```

2. Build the project:
```bash
g++ main.cpp -o sct-segment `pkg-config --cflags --libs opencv4`
```

3. Run the application:
```bash
./sct-segment
```

> Use mouse clicks to mark ROI points and keyboard keys to switch between processing steps.

---

## 📚 References

- Umbaugh Scot E., *Computer Vision and Image Processing*, Prentice Hall, NJ, 1998.
- [SCT-Center segmentation article](https://biblioteca.utcluj.ro/files/carti-online-cu-coperta/625-8.pdf#page=44)

---

## 📄 License

This project is part of a medical imaging lab assignment and is intended for educational and research use only.

© 2025 Francesco Maxim
