#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main(int argc, char** argv) {
    int myrank;
    int size, count, source;
    MPI_Status status;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

    if (myrank > 0) {
        char str[100];

        sprintf(str, "Hello from process %d, number %.0f", myrank, pow(5, myrank));

        MPI_Send(str, (int) strlen(str) + 1, MPI_CHAR, 0, 42, MPI_COMM_WORLD);

    } else {
        char** result = (char**)malloc(sizeof(char*) * (size - 1));

        for (int i = 1; i < size; ++i) {

            MPI_Probe(MPI_ANY_SOURCE, 42, MPI_COMM_WORLD, &status);

            MPI_Get_count(&status, MPI_CHAR, &count);

            source = status.MPI_SOURCE;

            result[source - 1] = (char*)malloc(sizeof(char) * count);

            MPI_Recv(result[source - 1], count, MPI_CHAR, status.MPI_SOURCE, 42, MPI_COMM_WORLD, &status);

            printf("Received %d elements from %d process\n", count, source);
        }

        for (int i = 0; i < size - 1; ++i) {
            printf("%s\n", result[i]);
        }
    }

    MPI_Finalize();
    return 0;
}