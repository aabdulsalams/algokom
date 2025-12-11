#include <stdio.h>
#include <omp.h>
#include <time.h>

#define N 20

int fib_parallel(int n) {
    if (n < 2) return n;

    int x, y;

    #pragma omp task shared(x)
    {
        x = fib_parallel(n - 1);
    }

    y = fib_parallel(n - 2);

    #pragma omp taskwait
    return x + y;
}

int fib_sequential(int n) {
    if (n < 2) return n;
    return fib_sequential(n - 1) + fib_sequential(n - 2);
}

void main() {
    double seq_wall_start, seq_wall_end, seq_wall_time;
    double par_wall_start, par_wall_end, par_wall_time;
    clock_t seq_cpu_start, seq_cpu_end;
    clock_t par_cpu_start, par_cpu_end;
    double seq_cpu_time, par_cpu_time;
    int result;

    // Sequential execution
    printf("=== Sequential (Tanpa Multi-threading) ===\n");
    seq_wall_start = omp_get_wtime();
    seq_cpu_start = clock();
    result = fib_sequential(N);
    seq_cpu_end = clock();
    seq_wall_end = omp_get_wtime();
    seq_wall_time = seq_wall_end - seq_wall_start;
    seq_cpu_time = ((double)(seq_cpu_end - seq_cpu_start)) / CLOCKS_PER_SEC;
    printf("Bilangan fibonacci %d: %d\n", N, result);
    printf("Wall Time: %.6f seconds\n", seq_wall_time);
    printf("CPU Time: %.6f seconds\n\n", seq_cpu_time);

    // Parallel execution
    printf("=== Parallel (Dengan OpenMP) ===\n");
    par_wall_start = omp_get_wtime();
    par_cpu_start = clock();
    #pragma omp parallel
    {
        #pragma omp single
        {
            result = fib_parallel(N);
        }
    }
    par_cpu_end = clock();
    par_wall_end = omp_get_wtime();
    par_wall_time = par_wall_end - par_wall_start;
    par_cpu_time = ((double)(par_cpu_end - par_cpu_start)) / CLOCKS_PER_SEC;
    printf("Bilangan fibonacci %d: %d\n", N, result);
    printf("Wall Time: %.6f seconds\n", par_wall_time);
    printf("CPU Time: %.6f seconds\n\n", par_cpu_time);

    // Speedup calculation
    printf("=== Perbandingan ===\n");
    printf("Speedup (Wall Time): %.2fx\n", seq_wall_time / par_wall_time);
    printf("Speedup (CPU Time): %.2fx\n", seq_cpu_time / par_cpu_time);
}