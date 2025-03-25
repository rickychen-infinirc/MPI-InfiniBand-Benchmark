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
    
    if (size != 2) {
        if (rank == 0) {
            fprintf(stderr, "This program requires exactly 2 MPI tasks.\n");
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
        printf("Starting benchmark for %d seconds...\n", desired_seconds);
        fflush(stdout);
    }
    
    start_time = MPI_Wtime();
    
    while (elapsed_time < desired_seconds) {
        if (rank == 0) {
            MPI_Send(buffer, BUFFER_SIZE, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(buffer, BUFFER_SIZE, MPI_CHAR, 1, 0, MPI_COMM_WORLD, &status);
        } else if (rank == 1) {
            MPI_Recv(buffer, BUFFER_SIZE, MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
            MPI_Send(buffer, BUFFER_SIZE, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        }
        
        total_bytes += 2 * BUFFER_SIZE; 
        iterations++;
        
        end_time = MPI_Wtime();
        elapsed_time = end_time - start_time;
    }
    

    long long remote_total_bytes = 0;
    int remote_iterations = 0;
    
    if (rank == 1) {
        MPI_Send(&total_bytes, 1, MPI_LONG_LONG, 0, 1, MPI_COMM_WORLD);
        MPI_Send(&iterations, 1, MPI_INT, 0, 2, MPI_COMM_WORLD);
    } else if (rank == 0) {
        MPI_Recv(&remote_total_bytes, 1, MPI_LONG_LONG, 1, 1, MPI_COMM_WORLD, &status);
        MPI_Recv(&remote_iterations, 1, MPI_INT, 1, 2, MPI_COMM_WORLD, &status);
        

        if (remote_total_bytes < total_bytes) total_bytes = remote_total_bytes;
        if (remote_iterations < iterations) iterations = remote_iterations;
        
        double bandwidth_MBps = (total_bytes / (1024.0 * 1024.0)) / elapsed_time; 
        double bandwidth_Gbps = bandwidth_MBps * 8 / 1000; 
        
        printf("\n=========== BENCHMARK RESULTS ===========\n");
        printf("Average bandwidth: %.2f MB/s (%.2f Gbps)\n", bandwidth_MBps, bandwidth_Gbps);
        printf("Total data transferred: %.2f MB\n", total_bytes / (1024.0 * 1024.0));
        printf("Number of iterations: %d\n", iterations);
        printf("Total time: %.2f seconds\n", elapsed_time);
        printf("Buffer size: %d bytes\n", BUFFER_SIZE);
        printf("========================================\n");
        fflush(stdout);
    }
    

    MPI_Barrier(MPI_COMM_WORLD);
    
    free(buffer);
    MPI_Finalize();
    return 0;
}
