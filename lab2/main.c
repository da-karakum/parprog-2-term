#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

void usage (const char *argv0);
void triagonal_matrix_algo (double const *a, double const *b, double const *c, 
                            double const *d, size_t N, double *y);
int get_npoints (const char *npoints_str);
void triagonal_matrix_algo_parallel (double const *a, double const *b, 
                        double const *c, double *d, size_t N, double *y);
void triagonal_matrix_algo_test (void);
void triagonal_matrix_test_create (void);
double f (double y) {
    return exp (-y);
}

// Нулевое приближение метода линеаризации
double y_0 (double x, double b) {
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
    triagonal_matrix_algo_test();
    return 0;
    if (argc != 2)
        usage (argv[0]);
    size_t M = get_npoints (argv[1]);


    double * const y = newton_linearization (M, 1, 1.0 / (M-1));
    
    for (size_t i = 0; i < M; ++i) {
        printf ("%f, ", y[i]);
    }
    printf ("\n");


    return EXIT_SUCCESS;
}

void usage (const char *argv0) {
    printf ("Usage: %s <number of points>\n", argv0);
    exit (EXIT_FAILURE);
}

int get_npoints (const char *npoints_str) {
    char *end;
    unsigned long res = strtoul (npoints_str, &end, 10);
    errno = 0;
    if (*end != '\0' || errno == ERANGE || res > __INT32_MAX__) {
        printf ("Incorrect npoints\n");
        exit (1);
    }
    return res;
}

void triagonal_matrix_algo (double const *a, double const *b, double const *c,
                            double const *d, size_t N, double *y) {
    
    for (int i = 0; i < N; ++i) {
        printf ("%f y_%d - %f y_%d + %f y_%d = %f\n", a[i], i-1, b[i], i, c[i], i+1, d[i]);
    }
    double * const buffer = (double *) malloc (2 * N * sizeof (*buffer));

    double *alpha = buffer;
    double *beta =  buffer + N;

    alpha[0] = c[0]/b[0];
    beta[0]  = -d[0]/b[0];

    for (int k = 1; k < N-1; ++k) {
        alpha[k] = c[k] / (b[k] - alpha[k-1]*a[k]);
        beta[k]  = (a[k]*beta[k-1] - d[k]) / (b[k] - alpha[k-1]*a[k]);
    }

    beta[N-1] = (a[N-1]*beta[N-2] - d[N-1]) / (b[N-1] - alpha[N-2]*a[N-1]);

    y[N-1] = beta[N-1];
    for (int n = N-2; n > -1; --n)
        y[n] = alpha[n] * y[n+1] + beta[n];

    free (buffer);

    printf ("y = (");
    for (size_t i = 0; i < N; ++i)
        printf ("%f, ", y[i]);
    printf (")\n\n");
}

void triagonal_matrix_algo_parallel (double const *a, double const *b, 
                        double const *c, double *d, size_t N, double *y) {
    if ((N-2) & (N-1)) // N-1 должно быть степенью двойки, наверное...
    {
        printf ("Error: N-1 has to be a power of 2\n");
        return;
    }

    double * const buffer = (double *) malloc (4 * N * sizeof (*buffer));
    double *red_a = buffer, *red_b = buffer + N, *red_c = buffer + 2 * N, 
           *red_d = buffer + 3 * N;

    red_a[0]   = a[0];   red_b[0] =   b[0];   red_c[0] =   c[0];   red_d[0] =   d[0];
    red_a[N-1] = a[N-1]; red_b[N-1] = b[N-1]; red_c[N-1] = c[N-1]; red_d[N-1] = d[N-1]; 
    for (size_t k = 2; k < N-2; k += 2) {
        // reduction
        red_a[k] = a[k]*a[k-1]/b[k-1];
        red_b[k] = b[k] - a[k]*c[k-1]/b[k-1] - c[k]*a[k+1]/b[k+1];
        red_c[k] = c[k]*c[k+1]/b[k+1];
        red_d[k] = d[k] + a[k]/b[k-1]*d[k-1] + c[k]/b[k+1]*d[k+1];
    }

    size_t N_ = N / 2 + 1;
    size_t step;
    for (step = 2; step < (N-1) / 4 /*?*/; step *= 2) {
        int count = 0;
        for (size_t k = step * 2; k < N - step * 2; k += step * 2) {
            red_a[k] = red_a[k]*red_a[k-step]/red_b[k-step];
            red_b[k] = red_b[k] - red_a[k]*red_c[k-step]/red_b[k-step] - red_c[k]*red_a[k+step]/red_b[k+step];
            red_c[k] = red_c[k]*red_c[k+step]/red_b[k+step];
            red_d[k] = red_d[k] + red_a[k]/red_b[k-step]*red_d[k-step] + red_c[k]/red_b[k+step]*red_d[k+step];
            count++;
        }
        printf ("line %d: %u ?= 3\n", __LINE__, count);
    }

    printf ("step = %lu\n", step);
    red_a[1] = red_a[step];
    red_a[2] = red_a[2*step];
    red_a[3] = red_a[3*step];
    red_a[4] = red_a[4*step];
    red_b[1] = red_b[step];
    red_b[2] = red_b[2*step];
    red_b[3] = red_b[3*step];
    red_b[4] = red_b[4*step];
    red_c[1] = red_c[step];
    red_c[2] = red_c[2*step];
    red_c[3] = red_c[3*step];
    red_c[4] = red_c[4*step];
    red_d[1] = red_d[step];
    red_d[2] = red_d[2*step];
    red_d[3] = red_d[3*step];
    red_d[4] = red_d[4*step];

    double red_y[5];
    triagonal_matrix_algo (red_a, red_b, red_c, red_d, 5, red_y);
    y[0] = red_y[0]; y[step] = red_y[1]; y[2*step] = red_y[2]; y[3*step] = red_y[3]; y[4*step] = red_y[4];
    // splitting initial matrix
    d[1] -= y[0]*a[1];

    d[step-1] -= y[step]*c[step-1]; 
    d[step+1] -= y[step]*a[step+1];

    d[2*step-1] -= y[2*step]*c[2*step-1]; 
    d[2*step+1] -= y[2*step]*a[2*step+1];

    d[3*step-1] -= y[3*step]*c[3*step-1]; 
    d[3*step+1] -= y[3*step]*a[3*step+1];

    d[4*step-1] -= y[4*step]*c[4*step-1]; 

    triagonal_matrix_algo (a+1, b+1, c+1, d+1, step-1, y+1);
    triagonal_matrix_algo (a+1+step, b+1+step, c+1+step, d+1+step, step-1, y+1+step);
    triagonal_matrix_algo (a+1+2*step, b+1+2*step, c+1+2*step, d+1+2*step, step-1, y+1+2*step);
    triagonal_matrix_algo (a+1+3*step, b+1+3*step, c+1+3*step, d+1+3*step, step-1, y+1+3*step);

    free (buffer);

}

double *newton_linearization (size_t M, double B, double h) {
    const double EPS = M * 1e-4;
    double * const buffer = (double *) malloc (8 * M * sizeof (*buffer));
    double *u = buffer, *v = buffer + M, *a = buffer + 2*M, *b = buffer + 3*M,
           *c = buffer + 4*M, *d = buffer + 5*M, *d2u = buffer + 6*M, 
           *fu = buffer + 7*M;

    // Нулевое приближение
    double x = 0;
    for (int i = 0; i < M; i++) {
        u[i] = y_0 (x, B);
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
        
        if (discrepancy < EPS) {
            free (buffer);
            return u;
        }

    }


}


double randfrom(double min, double max) 
{
    double range = (max - min); 
    double div = RAND_MAX / range;
    return min + (rand() / div);
}

void generate_example (double *a, double *b, double *c, double *d, double *y, size_t N) {
    srand (time (0));

    for (size_t i = 0; i < N; ++i) {
        y[i] = randfrom (-1, 1);
    }
    b[0] = randfrom (-1, 1);
    c[0] = 0;
    d[0] = - b[0]*y[0] + c[0]*y[1];
    for (size_t i = 1; i < N-1; ++i) {
        a[i] = randfrom (-1, 1);
        c[i] = randfrom (-1, 1);
        b[i] = randfrom (fabs(a[i]) + fabs(c[i]), fabs(a[i]) + fabs(c[i]) + 1);
        if (rand() & 1) b[i] = -b[i];
        d[i] = a[i]*y[i-1] - b[i]*y[i] + c[i]*y[i+1];
    }
    a[N-1] = 0;
    b[N-1] = randfrom (-1, 1);
    d[N-1] = a[N-1]*y[N-2] - b[N-1]*y[N-1];
}


void triagonal_matrix_algo_test (void) {
    double a[17] = {NAN, -0.0453352430, -0.7892442484, 0.7613985416, -0.4008339901, -0.2048139131, 0.0447129156, 0.0075377566, -0.5105699773, 0.8045634999, -0.7333023454, 0.8465147409, 0.2814253835, 0.9495592313, 0.5131449781, 0.5477298189, 0.0000000000};
    double b[17] = {0.1730744658, 0.9644167106, 1.6646984609, -2.5944433881, -1.1835535961, 1.9178716088, 1.7146259475, 1.1357885600, 2.0419443241, 1.2254827997, 2.6249246935, -2.4179914228, 2.1174016814, -1.8143112323, 2.0428713467, -1.4540635308, -0.7011552321};
    double c[17] = {0.0000000000, 0.8983738645, -0.0250869743, 0.8875109245, 0.5251204788, 0.8040174245, 0.7320282281, 0.5046897118, 0.9061896354, 0.2917846298, 0.9590038173, 0.7563049056, -0.8544581932, 0.7811167509, -0.9569598930, -0.6073448665, NAN};
    double d[17] = {-0.1097383212, -0.0182897828, 0.1981177729, 0.8947096681, -0.1336220874, 1.5195307202, -0.3616963012, 0.1901080125, -1.2776631141, -0.1957215230, -0.5911889247, 1.2320293984, -1.3360506507, 0.8256077827, -0.0787112916, 0.2230831752, -0.2390466396};
    double y_exp[17] = {0.6340526359, -0.0870491020, -0.0818105513, 0.2700656090, 0.2888191702, -0.6992749170, 0.2954729429, 0.2406954911, 0.9139476865, 0.7851105778, 0.1065478898, 0.2755095294, 0.6289201042, 0.0958613777, 0.0697572003, -0.0152596883, -0.3409325477};
    double y[17];
    triagonal_matrix_algo_parallel (a,b,c,d,17,y);
    for (int i = 0; i < 17; ++i) {
        printf ("%f == %f\n", y[i], y_exp[i]);
    }
    printf ("\n");
}

void triagonal_matrix_test_create (void) {
    double * const a = (double *) malloc (17 * sizeof(double));
    double * const b = (double *) malloc (17 * sizeof(double));
    double * const c = (double *) malloc (17 * sizeof(double));
    double * const d = (double *) malloc (17 * sizeof(double));
    double * const y = (double *) malloc (17 * sizeof(double));

    generate_example (a,b,c,d,y,17);

    printf ("double a[17] = {NAN, ");
    for (int i = 1; i < 16; ++i)
        printf ("%.10f, ", a[i]);
    printf ("%.10f};\n", a[16]);

    printf ("double b[17] = {");
    for (int i = 0; i < 16; ++i)
        printf ("%.10f, ", b[i]);
    printf ("%.10f};\n", b[16]);

    printf ("double c[17] = {");
    for (int i = 0; i < 16; ++i)
        printf ("%.10f, ", c[i]);
    printf ("NAN};\n");

    printf ("double d[17] = {");
    for (int i = 0; i < 16; ++i)
        printf ("%.10f, ", d[i]);
    printf ("%.10f};\n", d[16]);

    printf ("double y_exp[17] = {");
    for (int i = 0; i < 16; ++i)
        printf ("%.10f, ", y[i]);
    printf ("%.10f};\n", y[16]);

    free (a); free (b); free (c); free (d); free (y);
}