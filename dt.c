#include <stdio.h>
#include <glib.h>

// https://github.com/sailfishos-mirror/glib/blob/92e059280f5be23cf77bbe4ec016b5cc0f9af959/glib/tests/date.c

/*   run as 
	 ./dt '1982-11-17' dt.txt
*/

int main(int argc, char **argv) {

  GDate *d, *h;

  d = g_date_new ();
  h = g_date_new ();

  g_date_set_dmy (h, 4, 11, 2021);
  
  if( argc > 1) {

	FILE *fp;
	char *fname = argv[2];

	g_date_set_parse (d, argv[1]);
	
	fp = fopen(fname,"w");
	if( fp == NULL) {
	  printf("file can't be opened\n");
	  exit(1);
	}

	fprintf( fp, "u\n");
	fprintf( fp, "argv[1] = %s\n", argv[1]);
	fprintf( fp, "d->day = %d\n", d->day);
	fprintf( fp, "d->month = %d\n", d->month);
	fprintf( fp, "d->year = %d\n", d->year);

	fprintf( fp, "days between d and today = %d\n", g_date_days_between(d,h));
	fprintf( fp, "years between d and today = %f\n", g_date_days_between(d,h) / 365.25);
	
	fclose(fp);
	
  }

  g_date_free(d);

  return EXIT_SUCCESS;
}
