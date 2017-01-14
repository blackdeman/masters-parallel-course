#include <mpi.h>
#include <cstdio>
#include <cstdlib>
#include <utility>

void printLocals(int rank, int size, int stripeSize, int N, int * localRowStripe, int * localColStripe, int * localResult) {
    for (int i = 0; i < size; ++i) {
        if (rank == i) {
            printf("I am process %d\n", rank);
            printf("--- Row ---\n");
            for (int j = 0; j < stripeSize; ++j) {
                for (int k = 0; k < N; ++k) {
                    printf("%d ", localRowStripe[j * N + k]);
                }
                printf("\n");
            }
            printf("--- Col ---\n");
            for (int j = 0; j < stripeSize; ++j) {
                for (int k = 0; k < N; ++k) {
                    printf("%d ", localColStripe[j * N + k]);
                }
                printf("\n");
            }
            printf(" --- Local result ---\n");
            for (int j = 0; j < stripeSize; ++j) {
                for (int k = 0; k < N; ++k) {
                    printf("%4d ", localResult[j * N + k]);
                }
                printf("\n");
            }
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
}

void printMatrix(char * name, int * matrix, int N) {
    printf("Matrix %s\n", name);
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            printf("%4d ", matrix[i * N + j]);
        }
        printf("\n");
    }
}

int main(int argc, char** argv) {

    int N;
    int *A, *B, *C;

    if (argc < 2) {
        fprintf(stderr, "Parameter N should be specified\n");
        MPI_Finalize();
        return 1;
    }

    N = atoi(argv[1]);

    int rank;
    int size;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        // init matrices A and B in zero process
        A = new int[N * N];
        B = new int[N * N];

        for (int i = 0; i < N * N; ++i) {
            A[i] = i;
            B[i] = i;
        }
        printMatrix((char *) "A", A, N);
        printMatrix((char *) "B", B, N);

        // correct result
        printf("Correct result\n");
        for (int i = 0; i < N; ++i) {
            for (int j = 0; j < N; ++j) {
                int result = 0;
                for (int k = 0; k < N; ++k) {
                    result += A[i * N + k] * B[k * N + j];
                }
                printf("%4d ", result);
            }
            printf("\n");
        }

//        printMatrix("A", A, N);
//        printMatrix("B", B, N);

        // init var for result
        C = new int[N * N];
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Status status;
    MPI_Request recvRequest = MPI_REQUEST_NULL, sendRequest = MPI_REQUEST_NULL;

    int prevProc = (rank - 1 + size) % size;
    int nextProc = (rank + 1) % size;

    int stripeSize = N / size;

    int * localRowStripe = new int[stripeSize * N];
    int * localColStripeCompute = new int[stripeSize * N];
    int * localColStripeComm = new int[stripeSize * N];

    int * localResult = new int[stripeSize * N];

    MPI_Datatype _colType, colType;

    if (rank == 0) {
        MPI_Type_vector(N, 1, N, MPI_INT, &_colType);
        MPI_Type_create_resized(_colType, 0, sizeof(int), &colType);
        MPI_Type_commit(&colType);
    }

    // send row stripes of matrix A to other processes
    MPI_Scatter(A, stripeSize * N, MPI_INT, localRowStripe, stripeSize * N, MPI_INT, 0, MPI_COMM_WORLD);

    // send col stripes of matrix B to other processes
    MPI_Scatter(B, stripeSize, colType, localColStripeComm, stripeSize * N, MPI_INT, 0, MPI_COMM_WORLD);

    for (int iter = 0; iter < size; ++iter) {
//        if (rank == 0)
//            printf("===== Iteration %d ========\n", iter);

        MPI_Wait(&sendRequest, &status);
        MPI_Wait(&recvRequest, &status);

        std::swap(localColStripeComm, localColStripeCompute);

        if (iter < size - 1) {
            MPI_Isend(localColStripeCompute, stripeSize * N, MPI_INT, nextProc, 42, MPI_COMM_WORLD, &sendRequest);
            MPI_Irecv(localColStripeComm, stripeSize * N, MPI_INT, prevProc, 42, MPI_COMM_WORLD, &recvRequest);
        }

        // computation (localColStripeCompute is transposed!)
        for (int i = 0; i < stripeSize; ++i) {
            for (int j = 0; j < stripeSize; ++j) {
                int resultIndex = stripeSize * ((rank + iter) % size) + j * N + i;
                localResult[resultIndex] = 0;
                for (int k = 0; k < N; ++k) {
                    localResult[resultIndex] += localRowStripe[i * N + k] * localColStripeCompute[j * N + k];
                }
            }
        }
//        printLocals(rank, size, stripeSize, N, localRowStripe, localColStripeCompute, localResult);
    }

    MPI_Gather(localResult, stripeSize * N, MPI_INT, C, stripeSize * N, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        printMatrix((char *) "C", C, N);
    }

    if (rank == 0) {
        MPI_Type_free(&colType);
    }

    delete [] localRowStripe;
    delete [] localColStripeComm;
    delete [] localColStripeCompute;

    delete [] localResult;

    if (rank == 0) {
        delete [] A;
        delete [] B;
        delete [] C;
    }

    MPI_Finalize();

    return 0;
}