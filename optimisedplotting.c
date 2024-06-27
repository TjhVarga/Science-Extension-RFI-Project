#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "cpgplot.h" // Include PGPLOT header, requires running on Linux

#define MAX_LINE_LENGTH 256
#define INITIAL_TIME_BLOCKS 1400 // Initial size for dynamic allocation

void redrawPlot(float *x, float *y1, float *y2, int n_points, float xmin, float xmax, float ymin, float ymax, const char *filename) {
    cpgbbuf(); // Begin buffered output
    cpgeras(); // Erase the plot
    cpgenv(xmin, xmax, ymin, ymax, 0, 1); // Set new plot environment
    char title[256];
    sprintf(title, "Mean Intensity Line Graph for Polarisaion 1 and Polarisation 2 - %s", filename);
    cpglab("Time Block", "Mean Intensity", title);
    cpgsci(1); // Set color index to black for P1
    cpgline(n_points, x, y1); // Draw line for P1
    cpgsci(6); // Set color index to magenta for P2
    cpgline(n_points, x, y2); // Draw line for P2
    cpgsci(1); // Reset color index to black for any further text or borders
    cpgebuf(); // End buffered output
}

int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    printf("Program start.\n");

    char filename[256];
    strcpy(filename, argv[1]);

    FILE *inputFile = fopen(filename, "r");
    if (inputFile == NULL) {
        printf("Error: Unable to open input file.\n", filename);
        return 1;
    }
    printf("File loaded successfully.\n", filename);

    double *meanIntensityP1 = calloc(INITIAL_TIME_BLOCKS, sizeof(double));
    double *meanIntensityP2 = calloc(INITIAL_TIME_BLOCKS, sizeof(double));
    int *countP1 = calloc(INITIAL_TIME_BLOCKS, sizeof(int));
    int *countP2 = calloc(INITIAL_TIME_BLOCKS, sizeof(int));
    int allocatedTimeBlocks = INITIAL_TIME_BLOCKS;

    if (!meanIntensityP1 || !meanIntensityP2 || !countP1 || !countP2) {
        printf("Error: Memory allocation failed.\n");
        fclose(inputFile);
        free(meanIntensityP1);
        free(meanIntensityP2);
        free(countP1);
        free(countP2);
        return 1;
    }

    char line[MAX_LINE_LENGTH];
    int previousTimeBlock = -1;
    while (fgets(line, sizeof(line), inputFile)) {
        int timeBlock;
        double intensity1, intensity2;
        if (sscanf(line, "%*s %*s %*s %d %*s %*s %lf %lf", &timeBlock, &intensity1, &intensity2) != 3) {
            printf("Error: Line format incorrect or incomplete.\n");
            continue;
        }

        if (timeBlock >= allocatedTimeBlocks) {
            int newBlocks = timeBlock + 100;
            meanIntensityP1 = realloc(meanIntensityP1, newBlocks * sizeof(double));
            meanIntensityP2 = realloc(meanIntensityP2, newBlocks * sizeof(double));
            countP1 = realloc(countP1, newBlocks * sizeof(int));
            countP2 = realloc(countP2, newBlocks * sizeof(int));
            for (int i = allocatedTimeBlocks; i < newBlocks; i++) {
                meanIntensityP1[i] = 0;
                meanIntensityP2[i] = 0;
                countP1[i] = 0;
                countP2[i] = 0;
            }
            allocatedTimeBlocks = newBlocks;
        }

        if (timeBlock != previousTimeBlock && previousTimeBlock != -1) {
            meanIntensityP1[previousTimeBlock] /= countP1[previousTimeBlock];
            meanIntensityP2[previousTimeBlock] /= countP2[previousTimeBlock];
            printf("Time Block: %d  Mean Intensity P1: %.3f  Mean Intensity P2: %.3f\n", previousTimeBlock, meanIntensityP1[previousTimeBlock], meanIntensityP2[previousTimeBlock]);
        }

        meanIntensityP1[timeBlock] += intensity1;
        meanIntensityP2[timeBlock] += intensity2;
        countP1[timeBlock]++;
        countP2[timeBlock]++;
        previousTimeBlock = timeBlock;
    }

    if (previousTimeBlock != -1) { // Handle last block
        meanIntensityP1[previousTimeBlock] /= countP1[previousTimeBlock];
        meanIntensityP2[previousTimeBlock] /= countP2[previousTimeBlock];
        printf("Time Block: %d  Mean Intensity P1: %.3f  Mean Intensity P2: %.3f\n", previousTimeBlock, meanIntensityP1[previousTimeBlock], meanIntensityP2[previousTimeBlock]);
    }


    // Prepare x and y coordinates for line graph
    float *x_coords = malloc((previousTimeBlock + 1) * sizeof(float));
    float *y_coordsP1 = malloc((previousTimeBlock + 1) * sizeof(float));
    float *y_coordsP2 = malloc((previousTimeBlock + 1) * sizeof(float));
    int n_points = 0; // Number of valid data points

    for (int i = 0; i <= previousTimeBlock; i++) {
        if (countP1[i] > 0) { // Only consider blocks with data
            x_coords[n_points] = (float)i;
            y_coordsP1[n_points] = (float)meanIntensityP1[i];
            y_coordsP2[n_points] = (float)meanIntensityP2[i];
            n_points++;
        }
    }

    // Initial plot bounds
    double ymin = 1e30, ymax = -1e30;
    for (int i = 0; i < n_points; i++) {
        ymin = fmin(ymin, fmin(y_coordsP1[i], y_coordsP2[i]));
        ymax = fmax(ymax, fmax(y_coordsP1[i], y_coordsP2[i]));
    }
    if (ymin == ymax) {
        ymin -= 1; // Create a reasonable range if all values are the same
        ymax += 1;
    }

    if (cpgopen("/XWINDOW") > 0) {
        redrawPlot(x_coords, y_coordsP1, y_coordsP2, n_points, 0, (float)previousTimeBlock, (float)ymin, (float)ymax, filename);
        cpgask(0);
        float x1, x2, y1, y2;
        char ch;
        do {
            cpgband(0, 0, 0, 0, &x1, &y1, &ch); // Wait for user input
            if (ch == 'z') { // Zoom mode
                cpgband(2, 0, x1, y1, &x2, &y2, &ch); // Let user draw a rectangle
                redrawPlot(x_coords, y_coordsP1, y_coordsP2, n_points, fmin(x1, x2), fmax(x1, x2), fmin(y1, y2), fmax(y1, y2), filename);
            } else if (ch == 'u') { // Unzoom mode
                redrawPlot(x_coords, y_coordsP1, y_coordsP2, n_points, 0, (float)previousTimeBlock, (float)ymin, (float)ymax, filename);
            }
        } while (ch != 'q'); // Quit on 'q'

        cpgclos();
    } else {
        printf("Error: PGPLOT device could not be opened or plot range invalid.\n");
    }

    fclose(inputFile);
    free(x_coords);
    free(y_coordsP1);
    free(y_coordsP2);
    free(meanIntensityP1);
    free(meanIntensityP2);
    free(countP1);
    free(countP2);

    printf("Program completed successfully.\n");

    return 0;
}
