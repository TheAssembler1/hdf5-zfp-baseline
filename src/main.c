#include <stdio.h>
#include <mpi.h>

#include "hdf5.h"

int main(int argc, char** argv) {
	MPI_Init(&argc, &argv);

	H5open();

	H5close();

	MPI_Finalize();
	
	return 0;
}
