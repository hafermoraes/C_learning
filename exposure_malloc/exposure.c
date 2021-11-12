/*   
	 run as 

	 printf 'id;dob;gender;policy_issue_date;policy_status_code;policy_status_date\n' > linhas.txt
	 printf '1234;1982-11-17;F;2010-01-01;1;\n' >> linhas.txt 
	 printf '5678;1977-06-23;M;2012-03-04;3;2015-09-17\n' >> linhas.txt 
	 tail +2 linhas.txt | ./exposure --start=2010-01-01 --end=2021-12-31 --type=3

*/

#include <stdio.h>       // stdin, stdout, stderr, fprintf, ...
#include <string.h>      // strings parsing: strtok_r
#include <stdlib.h>
#include <locale.h>
#include <getopt.h>      // command line arguments: getopt_long()
#include <glib.h>        // date calculations: g_date...
#include <time.h>        // time annotations: time()

/* 
   Options for the number of days in a year
   (1) without adjustment for leap years:   365      days/year    
   (2) good adjustment for leap years:      365.2425 days/year --> 365.2425 = ( 291 * 366 + 909 * 365 ) / 1200  )
   (3) accatble adjustment for leap years:  365.25   days/year --> 365.25   = ( 3 * 365 + 366 ) / 4
*/
const float DAYS_IN_YEAR = 365.25;
const char *DELIM = ";"; // delimiter in the stdin which is passed on to ./exposure


//  prototypes of the functions
void read_study_parameters(
						   int argc
						   ,char **argv
						   ,int *study_params_ok
						   ,char **start
						   ,char **end
						   ,char **type
						   );
void tokenize_stdin(
					char *line
					,char **id
					,char **dob
					,char **sex
					,char **pid
					,char **psc
					,char **psd
					);

void belongs_to_study(
					  int *in, char *reason
					  ,char *start, char *end
					  ,char *issue_date, char *status_code, char *status_date
					  );


// main 
int main(int argc, char **argv){

  /* file container for out-of-study policies */
  FILE *f_out = fopen("out_of_study","w");
  if( f_out == NULL) {
	printf("file can't be opened\n");
	exit(1);
  }

  // study variables
  char *study_start        = NULL;
  char *study_end          = NULL;
  char *study_type         = NULL;
  // policyholder variables
  char *policy_id          = NULL;
  char *policy_dob         = NULL;
  char *policy_gender      = NULL;
  char *policy_issue_date  = NULL;
  char *policy_status_code = NULL; 
  char *policy_status_date = NULL;

  //
  setlocale( LC_ALL, "");
  
  /* Checks study parameters for valid values. */
  int study_params_ok = 0;
  read_study_parameters(argc, argv, &study_params_ok, &study_start, &study_end, &study_type);

  if (study_params_ok == 0) {

	/* Reads input data from policyholders via 'stdin', one line at the time */
	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;

	/* reads first line of stdin*/
	read = getline(&line, &len, stdin);
	while ( read >= 0  ) {
	  // tokenize line into policyholders data structures
	  tokenize_stdin(
					 line
					 ,&policy_id, &policy_dob, &policy_gender, &policy_issue_date, &policy_status_code, &policy_status_date
					 );

	  // check if the policyholder belongs to the study (i.e., if it is exposed to risk)
	  int in_study = 0;
	  char *reason_out = NULL;

	  belongs_to_study(
					   &in_study, reason_out
					   ,study_start, study_end
					   ,policy_issue_date, policy_status_code, policy_status_date
					   );

	  // if policy does not belong to study, save it to disk with the reasons why
	  if ( in_study != 0) {

		fprintf( f_out, "%s;%s\n", policy_id, reason_out);

	  }
	  
	  // if not exposed to risk, skip line but document reason in a log file
	  // if exposed, proceed to calculations and print results to stdout (long-format)
	  // 

	  /* reads in next line from stdin*/
	  read = getline(&line, &len, stdin);
	}
	
	/* frees memory allocated for reading the lines of stdin */
	free(line);
}
 

  // freeing memory ...
  // from study variables
  free(study_start);         study_start = NULL;
  free(study_end);           study_end = NULL;
  free(study_type);          study_type = NULL;
  // and policy variables
  free(policy_id);           policy_id = NULL;
  free(policy_dob);          policy_dob = NULL;
  free(policy_gender);       policy_gender = NULL;
  free(policy_issue_date);   policy_issue_date = NULL;
  free(policy_status_code);  policy_status_code = NULL;
  free(policy_status_date);  policy_status_date = NULL;

  fclose(f_out);

  return EXIT_SUCCESS;
}


// declarations of functions
void read_study_parameters(int argc, char **argv, int *study_params_ok, char **ptr_start, char **ptr_end, char **ptr_type){ 
  // reads in the command line arguments --start=YYYY-MM-DD --end=YYYY-MM-DD --type=int
  // and returns 1 to the variable study_params_ok in main() in case of any errors

  // temporary variable for correct parsing of start and end dates
  GDate *date = g_date_new();

  // parsing of command line arguments
  int c;
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
		case 1:
		  fprintf( stderr, "Missing study parameters: start, end and type.\n");
		  *study_params_ok = 1; // setting flag on due to the error
		  break;

		case 's':
		  // Read in the study start date
		  g_date_set_parse (date, optarg);
		  if( g_date_valid(date) ){
			*ptr_start = (char *) realloc( *ptr_start, strlen(optarg) );
			strncpy( *ptr_start, optarg, strlen(optarg));
		  }
		  else {
			fprintf( stderr, "Study start date must be a valid date.\n");
			*study_params_ok = 1; // setting flag on due to the error
		  }
		  break;

		case 'e':
		  // Read in the study end date
		  g_date_set_parse (date, optarg);
		  if( g_date_valid(date) ){
			*ptr_end = (char *) realloc( *ptr_end, strlen(optarg) );
			strncpy( *ptr_end, optarg, strlen(optarg));
		  }
		  else {
			fprintf( stderr, "Study end date must be a valid date.\n");
			*study_params_ok = 1; // setting flag on due to the error
		  }
		  break;

		case 't':
		  // Read in the study type
		  if ( atoi(optarg) == 0 ){
			fprintf( stderr, "Study type must be a integer.\n");
			*study_params_ok = 1; // setting flag on due to the error
		  } else {
			*ptr_type = (char *) realloc( *ptr_type, strlen(optarg) );
			strncpy( *ptr_type, optarg, strlen(optarg));
		  }
		  break;

		case '?':
		  printf("Unknown option %c\n", optopt);
		  *study_params_ok = 1; // setting flag on due to the error
		  break;

		case ':':
		  printf("Missing option for %c\n", optopt);
		  *study_params_ok = 1; // setting flag on due to the error
		  break;

		/* default: */
		/*   printf("?? getopt returned character code 0%o ??\n", c); */
		}
	}
  g_date_free(date);
}

void tokenize_stdin(char *line, char **ptr_id, char **ptr_dob, char **ptr_sex, char **ptr_pid, char **ptr_psc, char **ptr_psd){
  char *token;
  char *rest;

  rest=line;

  /* parsing the policyholder's ID */
  token = strtok_r(rest, DELIM, &rest);
  *ptr_id = (char *) realloc( *ptr_id, strlen(token) );
  strncpy( *ptr_id, token, strlen(token)) ;

  /* parsing the date of birth of the policyholder */
  token = strtok_r(rest, DELIM, &rest);
  *ptr_dob = (char *) realloc( *ptr_dob, strlen(token) );
  strncpy( *ptr_dob, token, strlen(token)) ;

  /* parsing the gender of policyholder  */
  token = strtok_r(rest, DELIM, &rest); 
  *ptr_sex = (char *) realloc( *ptr_sex, strlen(token) );
  strncpy( *ptr_sex, token, strlen(token)) ;

  /* parsing the issue date of the policy */
  token = strtok_r(rest, DELIM, &rest);
  *ptr_pid = (char *) realloc( *ptr_pid, strlen(token) );
  strncpy( *ptr_pid, token, strlen(token)) ;

  /* parsing the status code of the policy */
  token = strtok_r(rest, DELIM, &rest);
  *ptr_psc = (char *) realloc( *ptr_psc, strlen(token) );
  strncpy( *ptr_psc, token, strlen(token)) ;

  /* parsing the date regarding the status date */
  token = strtok_r(rest, DELIM, &rest);
  *ptr_psd = (char *) realloc( *ptr_psd, strlen(token) );
  strncpy( *ptr_psd, token, strlen(token)) ;
}

void belongs_to_study(int *in, char *reason, char *start, char *end, char *issue_date, char *status_code, char *status_date){

  GDate *e = g_date_new ();
  GDate *s = g_date_new ();
  GDate *pid = g_date_new ();
  GDate *psd = g_date_new ();

  g_date_set_parse (e, end);
  g_date_set_parse (s, start);
  g_date_set_parse (pid, issue_date);
  g_date_set_parse (psd, status_date);

  // not exposed to risk if...
  //   any of the dates are invalid
  if( !g_date_valid(pid) ){
	strcat( reason, "Invalid policy issue date, ");
	(*in)++;
  }
  if( strlen(status_date) > 5 && !g_date_valid(psd) ){
	strcat( reason, "Invalid policy status date, ");
	(*in)++;
  }
  //   policy issue date is greater then study end date
  if( g_date_compare( pid, e) > 0 ) {
	strcat( reason, "Policy issued after end of study, ");
	(*in)++;
  }
  //   policy issue date is greater then policy status date
  if( !strncmp(status_date,"",10)  && g_date_compare( pid, psd) > 0) {
	strcat( reason, "Policy issued after status changes, ");
	(*in)++;
  }
  //   policy status date is smaller then study start date
  if( !strncmp(status_date,"",10) && g_date_compare( psd, e) > 0) {
	strcat( reason, "Policy issued after end of study, ");
	(*in)++;
  }

  g_date_free(e);
  g_date_free(s);
  g_date_free(pid);
  g_date_free(psd);
}
