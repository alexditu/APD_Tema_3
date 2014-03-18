// Ditu Alexandru Mihai 333CA
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"
#include <math.h>

#define NUM_COLORS 256

struct Point {
	double x, y;
	int c;
};

int main (int argc, char** argv) {

    int rank, size;
    MPI_Status status;
    int type;
    double xMin, xMax, yMin, yMax, res, maxSteps, jx, jy;
    FILE *fin;
    int i, j, k;
    int *mc;
    int *vc;

    // init
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // procesul master este cel cu rankul 0
    if (rank == 0) {

	    // Citire date din fisier
	    fin = fopen (argv[1], "r");
	    fscanf(fin, "%d", &type);
	    fscanf(fin, "%lf %lf %lf %lf", &xMin, &xMax, &yMin, &yMax);
	    fscanf(fin, "%lf", &res);
	    fscanf(fin, "%lf", &maxSteps);
	    if (type == 1) {
	    	fscanf(fin, "%lf %lf", &jx, &jy);
	    }
	    fclose(fin);

	    // trimit celorlalti datele de intrare
	    for (i = 1; i < size; i++) {
	    	MPI_Send(&type, 1, MPI_INT, i, 1, MPI_COMM_WORLD);
	    	MPI_Send(&xMin, 1, MPI_DOUBLE, i, 1, MPI_COMM_WORLD);
	    	MPI_Send(&xMax, 1, MPI_DOUBLE, i, 1, MPI_COMM_WORLD);
	    	MPI_Send(&yMin, 1, MPI_DOUBLE, i, 1, MPI_COMM_WORLD);
	    	MPI_Send(&yMax, 1, MPI_DOUBLE, i, 1, MPI_COMM_WORLD);
	    	MPI_Send(&res, 1, MPI_DOUBLE, i, 1, MPI_COMM_WORLD);
	    	MPI_Send(&maxSteps, 1, MPI_DOUBLE, i, 1, MPI_COMM_WORLD);

	    	// daca tipul e Julia tb sa trimit si jx, jy;
	    	if (type == 1) {
	    		MPI_Send(&jx, 1, MPI_DOUBLE, i, 1, MPI_COMM_WORLD);
		    	MPI_Send(&jy, 1, MPI_DOUBLE, i, 1, MPI_COMM_WORLD);
	    	}	    	
	    }
	    
	}

	// toate procesele primesc datele, mai putin Master (rank == 0)
	if (rank != 0) {
		MPI_Recv(&type, 1, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
		MPI_Recv(&xMin, 1, MPI_DOUBLE, 0,1,MPI_COMM_WORLD, &status);
		MPI_Recv(&xMax, 1, MPI_DOUBLE, 0,1,MPI_COMM_WORLD, &status);
		MPI_Recv(&yMin, 1, MPI_DOUBLE, 0,1,MPI_COMM_WORLD, &status);
		MPI_Recv(&yMax, 1, MPI_DOUBLE, 0,1,MPI_COMM_WORLD, &status);
		MPI_Recv(&res, 1, MPI_DOUBLE, 0,1,MPI_COMM_WORLD, &status);
		MPI_Recv(&maxSteps, 1, MPI_DOUBLE, 0,1,MPI_COMM_WORLD, &status);

		if (type == 1) {
			MPI_Recv(&jx, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, &status);
			MPI_Recv(&jy, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, &status);

		}
	}

	// de acum incep calculele
	int width, height, stripSize;
	
	width = floor((xMax - xMin) / res);
	height = floor((yMax - yMin) / res);
	stripSize = (width * height) / size;

	// daca e ultimul proces at calculeaza si restul
	if (rank == size - 1) {
		stripSize += (width * height) % size;
	}

	// aloc vectorul cu valorile ce urmeaza a fi calculate
	vc = (int*) malloc(stripSize * sizeof(int));

	int start, end, step, color, vpos;
	double cx, cy, zx, zy, zx2, zy2;

	start = rank * stripSize;
	end = (rank + 1) * stripSize;
	vpos = 0;

	for (k = start; k < end; k++) {
		i = k % width;
		j = k / width;
		cx = xMin + i * res;
		cy = yMin + j * res;

		zx = zy = 0;
		step = 0;

		while ((zx * zx + zy * zy < 4) && (step < maxSteps)) {
			zx2 = zx * zx - zy * zy + cx;
			zy2 = 2 * zx * zy + cy;

			zx = zx2;
			zy = zy2;
			step ++;
		}

		color = step % NUM_COLORS;
		vc[vpos] = color;
		vpos ++;
	}

	// trimit rezultatele la Master (rank == 0)
	if (rank != 0) {
		MPI_Send(vc, stripSize, MPI_INT, 0, 1, MPI_COMM_WORLD);
	} else {

		mc = (int*) malloc(width * height * sizeof(int));

		// adaug la matrice si ce a calculat Masterul:
		for (k = 0; k < stripSize; k++) {
			mc[k] = vc[k];
		}

		// masterul primeste rezultatele de la ceilalti
		int recvSize = stripSize;
		for (i = 1; i < size; i++) {
			start = i * stripSize;
			if (i == size - 1) {
				recvSize += ((width * height) % size);
			}
			MPI_Recv(mc + start, recvSize, MPI_INT, i, 1, MPI_COMM_WORLD, &status);

		}

		FILE *fp;
		fp = fopen(argv[2], "w");
		fprintf(fp, "%s", "P2\n");
		fprintf(fp, "%d %d\n", width, height);
		fprintf(fp, "%d\n", 255);

		i = width * (height - 1);
		while (i >= 0) {
			for(j = i; j < i + width; j++) {
				fprintf(fp, "%d ", mc[j]);
			}
			fprintf(fp, "\n");
			i -= width;
		}		
		fclose(fp);

	}



	



	MPI_Finalize();	
	return 0;
}