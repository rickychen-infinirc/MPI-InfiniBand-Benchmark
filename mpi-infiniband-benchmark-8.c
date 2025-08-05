#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE (1024 * 1024) 

int main(int argc, char** argv) {
    int rank, size;
    char* buffer;
    MPI_Status status;
    double start_time, end_time, elapsed_time = 0.0;
    int desired_seconds;
    long long total_bytes = 0;
    int iterations = 0;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 2) {
        if (rank == 0) {
            fprintf(stderr, "Usage: %s <desired_seconds>\n", argv[0]);
            fflush(stderr);
        }
        MPI_Finalize();
        return 1;
    }

    desired_seconds = atoi(argv[1]);

    if (size % 2 != 0) {
        if (rank == 0) {
            fprintf(stderr, "This program requires an even number of MPI tasks.\n");
            fflush(stderr);
        }
        MPI_Finalize();
        return 1;
    }

    buffer = (char*)malloc(BUFFER_SIZE);
    if (buffer == NULL) {
        fprintf(stderr, "Rank %d: Unable to allocate buffer\n", rank);
        fflush(stderr);
        MPI_Finalize();
        return 1;
    }

    memset(buffer, 'A' + rank, BUFFER_SIZE);
    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0) {
        printf("Starting benchmark for %d seconds with %d MPI ranks...\n", desired_seconds, size);
        fflush(stdout);
    }

    start_time = MPI_Wtime();

    int peer = (rank % 2 == 0) ? rank + 1 : rank - 1;

    while (elapsed_time < desired_seconds) {
        if (rank % 2 == 0) {
            MPI_Send(buffer, BUFFER_SIZE, MPI_CHAR, peer, 0, MPI_COMM_WORLD);
            MPI_Recv(buffer, BUFFER_SIZE, MPI_CHAR, peer, 0, MPI_COMM_WORLD, &status);
        } else {
            MPI_Recv(buffer, BUFFER_SIZE, MPI_CHAR, peer, 0, MPI_COMM_WORLD, &status);
            MPI_Send(buffer, BUFFER_SIZE, MPI_CHAR, peer, 0, MPI_COMM_WORLD);
        }
        total_bytes += 2 * BUFFER_SIZE;
        iterations++;
        end_time = MPI_Wtime();
        elapsed_time = end_time - start_time;
    }

    long long total_bytes_all = 0;
    MPI_Reduce(&total_bytes, &total_bytes_all, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    int total_iterations = 0;
    MPI_Reduce(&iterations, &total_iterations, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        double bandwidth_MBps = (total_bytes_all / (1024.0 * 1024.0)) / elapsed_time;
        double bandwidth_Gbps = bandwidth_MBps * 8 / 1000;

        printf("\n=========== BENCHMARK RESULTS ===========\n");
        printf("Total MPI pairs: %d\n", size / 2);
        printf("Average total bandwidth: %.2f MB/s (%.2f Gbps)\n", bandwidth_MBps, bandwidth_Gbps);
        printf("Total data transferred: %.2f MB\n", total_bytes_all / (1024.0 * 1024.0));
        printf("Total iterations: %d\n", total_iterations);
        printf("Total time: %.2f seconds\n", elapsed_time);
        printf("Buffer size: %d bytes\n", BUFFER_SIZE);
        printf("========================================\n");
    }

    free(buffer);
    MPI_Finalize();
    return 0;
}
