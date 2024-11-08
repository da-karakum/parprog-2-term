#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <omp.h>

/* Print usage message. */
void usage (const char *argv0);

/* Parse int from nthreads_str and return. Return 0 if empty string. 
   exit(1) if fails. */
int get_nthreads (const char *nthreads_str);

int main (int argc, char *argv[]) {

    int nthreads, tid;

    if (argc != 2)
        usage (argv[0]);
    
    nthreads = get_nthreads (argv[1]);

    if (nthreads != 0)
        omp_set_num_threads (nthreads);

    #pragma omp parallel private(nthreads, tid)
    {
        tid = omp_get_thread_num();
        nthreads = omp_get_num_threads();
        printf("Hello, World! From tid=%d, nthreads=%d\n",
               tid, nthreads);
    } // omp parallel
}

void usage (const char *argv0) {
    printf ("Usage: %s <num_threads>\n", argv0);
    printf ("num_threads: the number of threads. "
            "0 or empty string defaults this value.\n");
    exit (1);
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