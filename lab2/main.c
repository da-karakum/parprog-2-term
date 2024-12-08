#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

void usage (const char *argv0);
void triagonal_matrix_algo (double const *a, double const *b, double const *c, 
                            double const *d, size_t N, double *y);

double f (double y) {
    return exp (-y);
}

// Нулевое приближение метода линеаризации
double y0 (double x, double b) {
    return (b-1) * x + 1;
}

double d2y (double *y, size_t m, double h) {
    return (y[m-1] - 2 * y[m] + y[m+1]) / (h*h);
}

double d2y_left (double *y, double h) {
    return (2*y[0]-5*y[1]+4*y[2]-y[3]) / (h*h);
}

double d2y_right (double *y, double h, size_t M) {
    return (2*y[M-1]-5*y[M-2]+4*y[M-3]-y[M-4]) / (h*h);
}

double *newton_linearization (size_t N, double b, double h);

int main (int argc, char *argv[]) {
    if (argc != 2)
        usage (argv[0]);
    size_t M = get_npoints (argv[1]);

    double * const y = newton_linearization (M, 1, 1.0 / (M-1));

    return EXIT_SUCCESS;
}

void usage (const char *argv0) {
    printf ("Usage: %s <number of points>\n");
    exit (EXIT_FAILURE);
}

int get_npoints (const char *npoints_str) {
    char *end;
    unsigned long res = strtoul (npoints_str, &end, 10);
    errno = 0;
    if (*end != '\0' || errno == ERANGE || res > __INT32_MAX__) {
        printf ("Incorrect npoints: (npoints - 3) has to be divisible by 4\n");
        exit (1);
    }
    return res;
}

void triagonal_matrix_algo (double const *a, double const *b, double const *c,
                            double const *d, size_t N, double *y) {
    
    double * const buffer = (double *) malloc (5 * (N-1) * sizeof (*buffer));

    double *alpha = buffer - 1;
    double *beta = alpha + N - 1;
    double *gamma = beta + N - 1;
    double *mu = gamma + N - 1;
    double *nu = mu + N - 1;

    alpha[1] = c[0]/b[0];
    beta[1]  = d[0]/b[0];
    gamma[1] = a[0]/b[0];

    for (int k = 1; k < N-1; ++k) {
        alpha[k+1] = c[k] / (b[k] - alpha[k]*a[k]);
        beta[k+1]  = (a[k]*beta[k] - d[k]) / (b[k] - alpha[k]*a[k]);
        gamma[k+1] = a[k]*gamma[k] / (b[k] - alpha[k]*a[k]);
    }

    mu[N-1] = -c[N-1] / (a[N-1]*(alpha[N-1]+gamma[N-1])-b[N-1]);
    nu[N-1] = (d[N-1]-a[N-1]*beta[N-1]) / (a[N-1]*(alpha[N-1]+gamma[N-1])-b[N-1]);

    for (int n = N-1; n > 0; --n) {
        mu[n-1] =           alpha[n]*mu[n] + gamma[n]*mu[N-1];
        nu[n-1] = beta[n] + alpha[n]*nu[n] + gamma[n]*nu[N-1];
    }

    y[0] = nu[0] / (1 - mu[0]);

    for (int n = 2; n < N; ++n)
        y[n-1] = alpha[n] * (mu[n]*y[0]+nu[n]) + beta[n] + gamma[n] * (mu[N-1] * y[0] + nu[N-1]);

    free (buffer);
}

void triagonal_matrix_algo_parallel (double const *a, double const *b, 
                        double const *c, double const *d, size_t N, double *y) {
    if (N & (N+1))
        return;

    double * const buffer = (double *) malloc (2 * N * sizeof (*buffer));
    double *red_a = buffer, *red_b = buffer + N / 2, *red_c = buffer + N, 
           *red_d = buffer + N + N / 2; // Нет! N нечётно, духотааа
    
    red_a[0] = a[0]; red_b[0] = b[0]; red_c[0] = c[0]; red_d[0] = d[0];
    for (;;) {

    }
    
}

double *newton_linearization (size_t M, double b, double h) {
    const double EPS = M * 1e-4;
    double * const buffer = (double *) malloc (8 * M * sizeof (*buffer));
    double *u = buffer, *v = buffer + M, *a = buffer + 2*M, *b = buffer + 3*M,
           *c = buffer + 4*M, *d = buffer + 5*M, *d2u = buffer + 6*M, 
           *fu = buffer + 7*M;

    // Нулевое приближение
    double x = 0;
    for (int i = 0; i < M; i++) {
        u[i] = y0 (x, b);
        v[i] = 0;
        x += h;
    }

    for (;;) {
        // вычислить в v поправку
        /// вычислить d2u
        d2u[0] = d2y_left (u, h);
        d2u[M-1] = d2y_right (u, h, M);
        for (int m = 1; m < M-1; ++m)
            d2u[m] = d2y (u, m, h);

        /// вычислить fu
        for (int m = 0; m < M; ++m)
            fu[m] = f(u[m]);
        
        /// подготовить a,b,c,d
        c[0] = d[0] = a[M-1] = d[M-1] = 0;
        b[0] = -1; b[M-1] = -1;

        for (int m = 1; m < M-1; ++m) {
            a[m] = 1 / (h*h) + 1/12 * fu[m-1];
            b[m] = 2 / (h*h) - 5/6  * fu[m];
            c[m] = 1 / (h*h) + 1/12 * fu[m+1];
            d[m] = 1/12 * (fu[m+1] + fu[m-1] + d2u[m-1] + d2u[m+1]) + 
                   5/6 * (fu[m] + d2u[m]);
        }

        /// параллельная прогонка
        triagonal_matrix_algo_parallel (a, b, c, d, M, v);

        // вычислить норму поправки
        double discrepancy = 0;
        for (int m = 0; m < M; ++m) {
            discrepancy += v[m];
            u[m] += v[m];
        }
        
        if (discrepancy < EPS)
            return u;

    }


}