<h1>Description</h1>
This is a program for plotting HDF output from the Parkes Radio Telescope which has first been converted to text. The plot generated is a plot of mean intensity of each polarisation as a function of the spectral dump (time block).

<h2>Build</h2>
```
gcc -o optimisedplotting optimisedplotting.c -lcpgplot -lpgplot -lX11 -lm
```
