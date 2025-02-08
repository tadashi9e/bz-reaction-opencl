# Belousov–Zhabotinsky Reaction Simulator

A Belousov–Zhabotinsky reaction, or BZ reaction, is one of a class of reactions that serve as a classical example of non-equilibrium thermodynamics, resulting in the establishment of a nonlinear chemical oscillator.

## Files

- bz.cpp
- bz_kernel.cl
- Makefile

## Requirements

- OpenCL
- OpenGL
- FreeGLUT

## Usage

```
Usage: ./bz [--device N] [-w width] [-h height] [-i interval_millis] [-d diffusion_rate] [-P]
 -d, --device    : Select compute device.
 -w, --width     : Field width.
 -h, --height    : Field height.
 -i, --interval  : Step interval in milli seconds.
 -d, --diffusion : Diffusion rate (0.0 ~ 1.0).
 -a              : Parameter a.
 -b              : Parameter b.
 -c              : Parameter c.
 -P, --pause     : Pause at start. Will be released by 'p' key.
```