#include <stdio.h>
#include <glib.h>  /*  g_date... */
#include <time.h>  /* time()  */

// https://github.com/sailfishos-mirror/glib/blob/92e059280f5be23cf77bbe4ec016b5cc0f9af959/glib/tests/date.c

/*   run as 
	 ./dt '1982-11-17' dt.txt
*/

int main(int argc, char **argv) {

  GDate *d, *h;
  d = g_date_new ();
  h = g_date_new ();

  time_t now;
  now = time(NULL);
  g_date_set_time_t (h, now);

  if( argc > 1) {

	FILE *fp;
	char *fname = argv[2];

	g_date_set_parse (d, argv[1]);
	
	fp = fopen(fname,"w");
	if( fp == NULL) {
	  printf("file can't be opened\n");
	  exit(1);
	}

	fprintf( fp, "dt.c\n");

	fprintf( fp, "argv[1] = %s\n", argv[1]);
	fprintf( fp, "d->day = %d\n", d->day);
	fprintf( fp, "d->month = %d\n", d->month);
	fprintf( fp, "d->year = %d\n\n", d->year);

	fprintf( fp, "h->day = %d\n", h->day);
	fprintf( fp, "h->month = %d\n", h->month);
	fprintf( fp, "h->year = %d\n\n", h->year);

	fprintf( fp, "days between d and h = %d\n", g_date_days_between(d,h));
	fprintf( fp, "years between d and h = %f\n", g_date_days_between(d,h) / 365.25);
	
	fclose(fp);
	
  }

  g_date_free(d);

  return EXIT_SUCCESS;
}
