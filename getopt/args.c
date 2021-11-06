#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

/* https://azrael.digipen.edu/~mmead/www/mg/getopt/index.html */

/*   run as 
	 ./args --start=2010-01-01 --end=2021-12-31 --type=dac 
	 or as
	 ./args --start 2010-01-01 --end 2021-12-31 --type dac 
*/

int main(int argc, char **argv) {
  
  int c;
  int LEN = 100;
  char s[LEN], e[LEN], t[LEN];

  while (1) 
	{
      int option_index = 0;
      static struct option long_options[] = 
		{
          {"start", required_argument, NULL, 's' },
          {"end",   required_argument, NULL, 'e' },
          {"type",  required_argument, NULL, 't' },
          {NULL,    0,                 NULL,  0 }
		};

      c = getopt_long(argc, argv, "-:s:e:t:", long_options, &option_index);
      if (c == -1)
		break;

      switch (c) 
		{

		case 0:
		  printf("long option %s", long_options[option_index].name);
		  if (optarg)
			printf(" with arg %s", optarg);
		  printf("\n");
		  break;

		case 1:
		  printf("regular argument '%s'\n", optarg);
		  break;

		case 's':
		  printf("Start: '%s'\n", optarg);
		  strncpy(s, optarg, sizeof(s));
		  break;

		case 'e':
		  printf("  End: '%s'\n", optarg);
		  strncpy(e, optarg, sizeof(e));
		  break;

		case 't':
		  printf(" Type: '%s'\n", optarg);
		  strncpy(t, optarg, sizeof(t));
		  break;

		case '?':
		  printf("Unknown option %c\n", optopt);
		  break;

		case ':':
		  printf("Missing option for %c\n", optopt);
		  break;

		default:
		  printf("?? getopt returned character code 0%o ??\n", c);
	  
		}
	}

  printf("\n");
  printf("Variáveis lidas via linha de comando:\n\n");
  printf("\ts = %s \t length = %lu\n", s, strlen(s) );
  printf("\te = %s \t length = %lu\n", e, strlen(e) );
  printf("\tt = %s \t length = %lu\n", t, strlen(t) );
  
  return EXIT_SUCCESS;
}

