#include <mpi.h>
#include <cstdio>
#include <cstdlib>

int main(int argc, char** argv) {

    int N1, N2;

    if (argc < 3) {
        fprintf(stderr, "Parameters N1 and N2 should be specified\n");
        return 1;
    }

    N1 = atoi(argv[1]);
    N2 = atoi(argv[2]);

    int myrank;
    int size;

    int prev_proc, next_proc;
    int tag = 42;

    MPI_Status status;
    MPI_Request recv_request = MPI_REQUEST_NULL, send_request = MPI_REQUEST_NULL;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

    int P = size, B = 10;

    if (N1 % P) {
        fprintf(stderr, "N1 = %d is not divided by number of processes = %d. Finishing...\n", N1, P);
        MPI_Finalize();
        return 1;
    }

    if (N2 % B) {
        fprintf(stderr, "N2 = %d is not divided by block size = %d. Finishing...\n", N2, B);
        MPI_Finalize();
        return 1;
    }

    int rows_per_proc = N1 / P, blocks_per_proc = N2 / B;

    int* buffer = new int[B];
    int* matrix_part = new int[rows_per_proc * N2];

    if (myrank == 0) {
        prev_proc = MPI_PROC_NULL;
        for (int i = 0; i < B; ++i)
            buffer[i] = 0;
    }
    else {
        prev_proc = myrank - 1;
    }

    if (myrank == size - 1) {
        next_proc = MPI_PROC_NULL;
    }
    else {
        next_proc = myrank + 1;
    }

    for (int i = 0; i < blocks_per_proc; ++i) {
        if (i == 0)
            // first receive should be blocking
            MPI_Recv(buffer, B, MPI_INT, prev_proc, tag, MPI_COMM_WORLD, &status);
        else
            MPI_Wait(&recv_request, &status);

        for (int k = 0; k < B; ++k) {
            matrix_part[i * B + k] = buffer[k] + 1;
        }

        if (i > 0)
            MPI_Irecv(buffer, B, MPI_INT, prev_proc, tag, MPI_COMM_WORLD, &recv_request);

        for (int j = 1; j < rows_per_proc; ++j) {
            for (int k = 0; k < B; ++k) {
                matrix_part[j * N2 + i * B + k] = matrix_part[(j - 1) * N2 + i * B + k] + 1;
            }
        }

        // actually we don't need to wait
        // MPI_Wait(&send_request, &status);
        MPI_Isend(matrix_part + (rows_per_proc - 1) * N2 + i * B, B, MPI_INT, next_proc, tag, MPI_COMM_WORLD, &send_request);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    for (int i = 0; i < size; ++i) {
        if (myrank == i) {
            printf("Process #%d matrix part:\n", myrank);
            for (int j = 0; j < rows_per_proc; ++j) {
                for (int k = 0; k < N2; ++k) {
                    printf("%4d ", matrix_part[j * N2 + k]);
                }
                printf("\n");
            }
            printf("\n");
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}