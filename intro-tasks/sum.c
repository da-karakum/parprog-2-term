#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <omp.h>

void usage (const char *argv0);

int getN (const char *N_str, unsigned long *result);

int get_nthreads (const char *nthreads_str);

void calculateSection (unsigned long *from, unsigned long *to, unsigned long N,
                       int tid, int nthreads);

double sum_worker (unsigned long from, unsigned long to);

int main (int argc, char *argv[]) {

    int nthreads, tid;

    if (argc != 3) {
        usage (argv[0]);
        return EXIT_FAILURE;
    }

    nthreads = get_nthreads (argv[1]);

    if (nthreads != 0)
        omp_set_num_threads (nthreads);

    unsigned long N;
    getN (argv[2], &N);
    
    double result = 0;

    #pragma omp parallel shared(result) private(nthreads, tid)
    {
        tid = omp_get_thread_num();
        nthreads = omp_get_num_threads();

        unsigned long from, to;
        calculateSection (&from, &to, N, tid, nthreads);

        double result_local = sum_worker (from, to);

        #pragma omp atomic
        result += result_local;

    } // #pragma omp

    printf ("Result = %.9f\n", result);
}

double sum_worker (unsigned long from, unsigned long to) {
    double result = 0;
    for (unsigned long n = from; n < to; n++) {
        result += 1.0 / n;
    }
    return result;
}

void calculateSection (unsigned long *from, unsigned long *to, unsigned long N,
                       int tid, int nthreads) {
    *from = tid      * N / nthreads + 1;
    *to  = (tid + 1) * N / nthreads + 1;
}

void usage (const char *argv0) {
    printf ("Usage: %s <num_threads> <N>\n", argv0);
}

int getN (const char *N_str, unsigned long *result) {
    errno = 0;
    char *end = NULL;
    *result = strtoul (N_str, &end, 10);
    if (N_str[0] == '\0' || *end != '\0' || errno == ERANGE) {
        printf ("Incorrect value of N: has to be from 0 to UINT_MAX\n");
        exit (EXIT_FAILURE);
    }

    return 0;

}

int get_nthreads (const char *nthreads_str) {
    char *end;
    unsigned long res = strtoul (nthreads_str, &end, 10);
    errno = 0;
    if (*end != '\0' || errno == ERANGE || res > __INT32_MAX__) {
        printf ("Incorrect nthreads\n");
        exit (1);
    }
    return res;
}