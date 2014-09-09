/*

PICCANTE
The hottest HDR imaging library!
http://piccantelib.net

Copyright (C) 2014
Visual Computing Laboratory - ISTI CNR
http://vcg.isti.cnr.it
First author: Francesco Banterle

PICCANTE is free software; you can redistribute it and/or modify
under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 3.0 of
the License, or (at your option) any later version.

PICCANTE is distributed in the hope that it will be useful, but
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License
( http://www.gnu.org/licenses/lgpl-3.0.html ) for more details.

*/

/*! \mainpage Welcome to the PICCANTE documentation.
 *
 * \section intro_sec Introduction
 *
 * <a href="http://piccantelib.net">PICCANTE</a> is a C++11 image processing library aimed
 * to provide structures and functionalities for enabling
 * both High Dynamic Range (HDR) and standard imaging.
 *
 * \subsection Usage
 *
 * To use <a href="http://piccantelib.net">PICCANTE</a> simply set the wanted options and include \c "piccante.hpp"
 *
 * The options are set with a  \c #define and are:
 *
 * \li \c PIC_DEBUG used for debugging; it mostly enables some printf messages;
 * i.e. for warning when a computation succeeds or fails.
 * \li \c PIC_DISABLE_OPENGL disables the OpenGL support.
 * \li \c PIC_DISABLE_QT disables the QT support. Note that JPEG and PNG files
 * are read using QT, therefore it is required for reading such files.
 * \li \c PIC_ENABLE_OPEN_EXR enables the support for the OpenEXR library. This may be useful to have
 * in the case .exr images are used. Note that you need to manually install OpenEXR on your developing maching in order
 * to enable this flag.
 *
 * \section descr_sec Modules
 *
 * <a href="http://piccantelib.net">PICCANTE</a> is a modular library with different modules. The main classes are
 * pic::Image, pic::ImageRAW and pic::Histogram.
 * \li \c pic::Image is the base class for managing an image. Pixels are stored
 * in an array of interleaved channels float values; i.e. pic::Image::data.
 * Pixels are stored as float values, because the library is meant mostly for accurate and HDR
 * imaging processing. This class provides standard functions for extracting
 * image statistics (e.g. maximum value, minimum value, mean value, etc.), image operators
 * (e.g. add, sub, mul, div, etc.) and memory management functions (e.g. allocation, cloning, etc.).
 * Note that this class supports multi-channels (e.g. alpha channel) and temporal/volumetric images.
 * \li \c pic::ImageRAW inherits from pic::Image, and it provides an I/O interface for reading different
 * file formats (ppm, pgm, pbm, bmp, tga, hdr, pfm, etc.) natively in Piccante and through other optional
 * external libraries (e.g. OpenEXR and QT).
 * \li \c pic::Histogram is a class for creating, managing, and processing LDR/HDR image histograms.
 *
 * \subsection filters_module Filtering
 *
 * The main class of this module is pic::Filter. This is a base class for managing
 * a filter; it provides mechanisms for multi-threading, memory allocation, and so on.
 * Typically, a filter in <a href="http://piccantelib.net">PICCANTE</a> can have multiple pic::ImageRAW images as input, imgIn,
 * and a single output, imgOut.
 * Many image filters are implemented in <a href="http://piccantelib.net">PICCANTE</a> susch as: linear filters
 * (e.g. Gaussian, gradient based, DCT, etc.), non-linear filters
 * (e.g. bilateral, anisotropic diffusion, guided, median, etc.), and image transforms
 * (e.g. warping).
 *
 * \subsection algo_module Algorithms
 *
 * This module contains high-level imaging functionalities such as
 * Laplacian/Gaussian pyramids, a simple Poisson solver, SuperPixels (SLIC),
 * a simple gradient based demosacing method, a color to gray algorithm, etc.
 *
 * \subsection colors_module Colors
 *
 * This module provides classes and methods for editing, processing
 * and converting colors.
 * For example, the class pic::Color3 provides a basic type for three color
 * components representations. This can be useful for some applications
 * such as a 3D renderer.
 *
 * \subsection io_module Input and Output
 *
 * This module provides functions for reading and writing images natively in different
 * file formats such as:
 * \li \c BMP: Windows bitmap file; 24-bit color images are only supported.
 * \li \c HDR: Greg Ward's RGBE format.
 * \li \c PGM: Portable Gray Map images; greyscale images.
 * \li \c PPM: Portable Pixel Map images; color images.
 * \li \c PFM: Portable Float Map images; HDR color images.
 * \li \c TGA: targa file; 24-bit color images are only supported.
 * \li \c TMP: a dump of the pic::Image data.
 * \li \c VOL: a volumetric format for rendering; 32-bit per voxel.
 *
 * The module provides an interface for OpenEXR, but it requires
 * the linking with the OpenEXR library (please have a look at <a href="http://www.openexr.com">the official website</a>).
 * \li \c EXR: ILM's OpenEXR format; HDR color images at 16-bit per component.
 *
 * \subsection metrics_module Metrics
 *
 * This module provides classic objective metrics for measuring differences in images.
 * Several metrics are provided such as: PSNR, mPSNR (for HDR images), RMSE,
 * logRMSE (for HDR images), maximum error, relative error, etc.
 *
 * \subsection ps_module Point Samplers
 *
 * This module provides structures and functions for generating points' set in n-dimensions using
 * different distributions such as: random, stratified random, regular, Poisson-disk, etc.
 * Points generated with such distributions may be useful for filtering algorithms.
 *
 * \subsection is_module Image Samplers
 *
 * This module provides methods for sampling 2D and 3D images using different filters
 * such as: nearest neighbors filter, bilinear filter, bi-cubic filter, Gaussian filter,
 * etc.
 *
 * \subsection tm_module Tone Mapping
 *
 * This module provides tone mapping operators (TMOs) for reducing the dynamic
 * range in HDR images. Several TMOs are present such as: Ward Histogram Adjustment,
 * Reinhard Photographic Tone Reproduction Operator, Lischinski Improved Photographic Tone
 * Reproduction Operator, Drago TMO, Banterle Hybrid TMO, etc.
 *
 * \subsection fm_module Features and Matching
 *
 * This module provides classes and functions for extracting 2D features from 2D images,
 * and matching the extracted features. This may be useful for aligning images for different
 * tasks such as: HDR exposures stack alignment, generation of panoramas, etc.
 * Different features can be extracted and matched:
 * \li \c Corners (Key-point): SUSAN, Harris' method, and FAST.
 * \li \c Edges: Canny's method, Ward's MTB.
 * \li \c Key-point descriptors: BRIEF, ORB, and LUCID.
 *
 * \subsection utils_module Utilities
 *
 * This module provides different utilies for manipulating strings,
 * arrays, indexed arrays, math functions, 2D arrays, vectors, etc.
 *
 */

#ifndef PIC_PICCANTE_HPP
#define PIC_PICCANTE_HPP

#ifdef _MSC_VER
//we are using windows
#define PIC_WIN32
#elif __APPLE__
//we are using mac os x
#define PIC_MAC_OS_X
#else
// we assume that we are using a UNIX system
#define PIC_UNIX
#endif

//Mac OS X
#ifdef PIC_MAC_OS_X
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#pragma clang diagnostic ignored "-Wswitch"
#pragma clang diagnostic ignored "-Woverloaded-virtual"
#pragma clang diagnostic ignored "-Wdelete-non-virtual-dtor"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wformat"
#endif

//Win32
#ifdef PIC_WIN32

#pragma warning(disable:4100)
#pragma warning(disable:4146)

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <windows.h>
#include <winuser.h>
#include <vfw.h>
#include <tchar.h>
#include <direct.h>
#include <mmsystem.h>
#define strcasecmp stricmp
#pragma comment( lib, "Winmm" )
#pragma comment( lib, "vfw32" )
#endif

// Qt stuff
#include "qt.hpp"

// base stuff
#include "base.hpp"
#include "image.hpp"
#include "image_raw.hpp"
#include "image_raw_vec.hpp"
#include "histogram.hpp"

// sub dirs
#include "algorithms.hpp"
#include "colors.hpp"
#include "features_matching.hpp"
#include "filtering.hpp"
#include "gl.hpp"
#include "image_samplers.hpp"
#include "io.hpp"
#include "metrics.hpp"
#include "point_samplers.hpp"
#include "tone_mapping.hpp"
#include "util.hpp"

#ifdef PIC_MAC_OS_X
#pragma clang diagnostic pop
#endif

#endif /* PIC_PICCANTE_HPP */

