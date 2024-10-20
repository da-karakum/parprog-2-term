all: bin/hello-world bin/sum bin/seq-access

bin/%: %.c
	gcc -fopenmp $< -o $@