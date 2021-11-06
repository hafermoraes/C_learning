#include <stdio.h>
#include <gsl/gsl_rng.h>

/* https://www.gnu.org/software/gsl/doc/html/rng.html */

/*   run as 
	 ./a.out 10000 rnd_u.csv
*/

gsl_rng * r;  /* global generator */

int main(int argc, char **argv) {
  const gsl_rng_type * T;
  gsl_rng * r;

  if( argc > 1) {

	FILE *fp; /* file container for random numbers */
	double i, n = atoi( argv[1] );
	char *fname = argv[2]; 

	fp = fopen(fname,"w");
	if( fp == NULL) {
	  printf("file can't be opened\n");
	  exit(1);
	}

	fprintf( fp, "u\n");
	
	gsl_rng_env_setup();

	T = gsl_rng_default;
	r = gsl_rng_alloc (T);

	for (i = 0; i < n; i++)
	  {
		double u = gsl_rng_uniform (r);
		/* printf ("%.5f\t%s\n", u, fname); */
		fprintf( fp, "%.6f\n", u);
	  }

	fclose(fp);
	
	gsl_rng_free (r);
  }

  return EXIT_SUCCESS;
}
