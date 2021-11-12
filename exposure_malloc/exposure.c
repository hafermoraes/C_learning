/*   
	 run as 

	 printf 'id;dob;gender;policy_issue_date;policy_status_code;policy_status_date\n' > linhas.txt
	 printf '1234;1982-11-17;F;2010-01-01;1;\n' >> linhas.txt 
	 printf '5678;1977-06-23;M;2012-03-04;3;2015-09-17\n' >> linhas.txt 
	 printf '91011;1977-06-23;M;2012-03-04;3;\n' >> linhas.txt 
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

void tokenize_and_validate_stdin(
					char *line
					,int *in, FILE *f
					,char *start, char *end, char *type
					,char **id
					,char **dob
					,char **sex
					,char **pid
					,char **psc
					,char **psd
					);


// main 
int main(int argc, char **argv){

  // file for out-of-study policies
  FILE *f_out = fopen("out_of_study","w");
  if( f_out == NULL) {
	fprintf(stderr, "file 'out_of_study' can't be opened. Exiting...\n");
	exit(EXIT_FAILURE);
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

  // necessary to ensure correct date parsing at g_date_set_parse()
  setlocale( LC_ALL, "");

  // Checks study parameters for valid inputs
  int study_params_ok = 0;
  read_study_parameters(
						argc, argv
						,&study_params_ok, &study_start, &study_end, &study_type
						);
  if ( study_params_ok != 0 ) {
	fprintf(stderr, "Inconsistent study parameters. Exiting...\n");
	exit(EXIT_FAILURE);
  }

  // Reads input data from policyholders via 'stdin', one line at the time
  char *line = NULL;
  size_t len = 0;
  ssize_t read = 0;

  while ( (read = getline( &line, &len, stdin )) != -1 ) {
	int in_study = 0;

	// tokenize line into the policyholder variables
	tokenize_and_validate_stdin(
				   line
				   ,&in_study, f_out
				   ,study_start, study_end, study_type
				   ,&policy_id, &policy_dob, &policy_gender, &policy_issue_date, &policy_status_code, &policy_status_date
				   );


	// if policy does not belong to study, save it to disk with the reasons why
	if ( in_study != 0) {
	  //fprintf( f_out, "%s;%s\n", policy_id, reason_out);
	  continue;
	}
	  
	// if not exposed to risk, skip line but document reason in a log file
	// if exposed, proceed to calculations and print results to stdout (long-format)
	// 

  } // while
	
	// frees memory allocated for reading the lines of stdin
  free(line);
 
  // freeing memory from study variables
  free(study_start);         
  free(study_end);           
  free(study_type);
	
  // frees memory from policy variables
  free(policy_id);           
  free(policy_dob);          
  free(policy_gender);       
  free(policy_issue_date);   
  free(policy_status_code);  
  free(policy_status_date);  

  fclose(f_out);

  return EXIT_SUCCESS;
} // main


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
		} // switch case
	} // while

  // study start date after study end date
  if( (*ptr_start != NULL) && (*ptr_end != NULL) && (strcmp(*ptr_start, *ptr_end) >= 0) ){
	fprintf( stderr, "Study start date must be before study end date.\n");
	*study_params_ok = 1; // setting flag on due to the error
  }
  
  g_date_free(date);
}

void tokenize_and_validate_stdin(char *line, int *in, FILE *f_out, char *start, char *end, char *type, char **ptr_id, char **ptr_dob, char **ptr_sex, char **ptr_pid, char **ptr_psc, char **ptr_psd){
  GDate *date = g_date_new ();

  char *token = NULL;
  char *rest = NULL;

  rest=line;

  // ID
  //   parsing 
  token = strtok_r(rest, DELIM, &rest);
  *ptr_id = (char *) realloc( *ptr_id, strlen(token) );
  if( *ptr_id == NULL){
	fprintf(stderr, "Could not reallocate memory for *ptr_id. Exiting...\n");
	exit(EXIT_FAILURE);
  }
  strncpy( *ptr_id, token, strlen(token)) ;

  // Date of Birth 
  //   parsing
  token = strtok_r(rest, DELIM, &rest);
  *ptr_dob = (char *) realloc( *ptr_dob, strlen(token) );
  if( *ptr_dob == NULL){
	fprintf(stderr, "Could not reallocate memory for *ptr_dob. Exiting...\n");
	exit(EXIT_FAILURE);
  }
  strncpy( *ptr_dob, token, strlen(token)) ;
  //   single validation
  g_date_set_parse( date, *ptr_dob );
  if( !g_date_valid(date) ){
	fprintf( f_out, "%s; Invalid date of birth.\n", *ptr_id);
	(*in)++;
  }

  // Gender
  //   parsing
  token = strtok_r(rest, DELIM, &rest); 
  *ptr_sex = (char *) realloc( *ptr_sex, strlen(token) );
  if( *ptr_sex == NULL){
	fprintf(stderr, "Could not reallocate memory for *ptr_sex.\n");
	exit(EXIT_FAILURE);
  }
  strncpy( *ptr_sex, token, strlen(token)) ;

  // Policy Issue Date
  //   parsing
  token = strtok_r(rest, DELIM, &rest);
  *ptr_pid = (char *) realloc( *ptr_pid, strlen(token) );
  if( *ptr_pid == NULL){
	fprintf(stderr, "Could not reallocate memory for *ptr_pid. Exiting...\n");
	exit(EXIT_FAILURE);
  }
  strncpy( *ptr_pid, token, strlen(token)) ;
  //   single validation
  g_date_set_parse( date, *ptr_pid );
  if( !g_date_valid(date) ){
	fprintf( f_out, "%s; Invalid policy issue date.\n", *ptr_id);
	(*in)++;
  }

  // Policy Status Code
  //   parsing
  token = strtok_r(rest, DELIM, &rest);
  *ptr_psc = (char *) realloc( *ptr_psc, strlen(token) );
  if( *ptr_psc == NULL){
	fprintf(stderr, "Could not reallocate memory for *ptr_psc. Exiting...\n");
	exit(EXIT_FAILURE);
  }
  strncpy( *ptr_psc, token, strlen(token)) ;

  // Policy Status Date
  //   parsing
  token = strtok_r(rest, DELIM, &rest);
  *ptr_psd = (char *) realloc( *ptr_psd, strlen(token) );
  if( *ptr_psd == NULL){
	fprintf(stderr, "Could not reallocate memory for *ptr_psd. Exiting...\n");
	exit(EXIT_FAILURE);
  }
  strncpy( *ptr_psd, token, strlen(token) -1 ) ; //ignoring the '\n' at the end of line
  //   single validation
  g_date_set_parse( date, *ptr_psd );
  if( !g_date_valid(date) ){
	if ( atoi(*ptr_psc) != 1) {
	  fprintf( f_out, "%s; Invalid policy status date.\n", *ptr_id);
	  (*in)++;
	}
  }

  // Compound validations
  //   Date of birth is greater than Policy Issue Date
  if ( strcmp( *ptr_dob, *ptr_pid ) > 0 ){
	fprintf( f_out, "%s; Date of birth is greater than policy issue date.\n", *ptr_id);
	(*in)++;
  }
  //   Date of birth is greater than Study End Date
  if ( strcmp( *ptr_dob, end ) > 0 ){
	fprintf( f_out, "%s; Date of birth is greater than study end date.\n", *ptr_id);
	(*in)++;
  }
  //   Policy Issue Date is greater then Study End Date
  if ( strcmp( *ptr_pid, end ) > 0 ){
	fprintf( f_out, "%s; Policy issue date is greater than study end date.\n", *ptr_id);
	(*in)++;
  }
  //   Policy Issue Date is greater then Policy Status Date
  if ( atoi(*ptr_psc) != 1 && strcmp( *ptr_psd, "") != 0 && strcmp( *ptr_pid, *ptr_psd  ) > 0 ){
	fprintf( f_out, "%s; Policy issue date is greater than policy status date (for terminated policies).\n", *ptr_id);
	(*in)++;
  }
  //   Policy Status Date is smaller then Study Start Date
  if ( atoi(*ptr_psc) != 1 && strcmp( *ptr_psd, "") != 0 && strcmp( *ptr_psd, start  ) < 0 ){
	fprintf( f_out, "%s; Policy status date is smaller than study start date (for terminated policies).\n", *ptr_id);
	(*in)++;
  }
  //   Policy Status Date not available
  if ( atoi(*ptr_psc) != 1 && strcmp( *ptr_psd, "") == 0 ){
	fprintf( f_out, "%s; Policy status date is unavailable (for terminated policies).\n", *ptr_id);
	(*in)++;
  }
  
  // frees memory of the pointers
  token = NULL;
  rest = NULL;
  g_date_free(date);
}


/* void belongs_to_study(int *in, char **ptr_reason, char *start, char *end, char *issue_date, char *status_code, char *status_date){ */
	  
/*   GDate *e = g_date_new (); */
/*   GDate *s = g_date_new (); */
/*   GDate *pid = g_date_new (); */
/*   GDate *psd = g_date_new (); */

/*   g_date_set_parse (e, end); */
/*   g_date_set_parse (s, start); */
/*   g_date_set_parse (pid, issue_date); */
/*   g_date_set_parse (psd, status_date); */

/*   // not exposed to risk if... */
/*   //   any of the dates are invalid */
/*   if( !g_date_valid(pid) ){ */
/* 	char pid_invalid[] = "Invalid policy issue date, "; */

/* 	*ptr_reason = (char *) realloc( *ptr_reason, strlen(pid_invalid) ); */

/* 	strncat( *ptr_reason, pid_invalid, strlen(pid_invalid) ); */

/* 	(*in)++; */
/*   } */
	  
/*   if( strlen(status_date) != 0 && !g_date_valid(psd) ){ */
/* 	char psd_invalid[] = "Invalid policy status date, "; */

/* 	*ptr_reason = (char *) realloc( *ptr_reason, strlen(psd_invalid) ); */

/* 	strncat( *ptr_reason, psd_invalid, strlen(psd_invalid) ); */

/* 	(*in)++; */
/*   } */
/*   //   policy issue date is greater then study end date */
/*   if( g_date_compare( pid, e) > 0 ) { */
/* 	char pid_gt_end[] = "Policy issued after end of study, "; */

/* 	*ptr_reason = (char *) realloc( *ptr_reason, strlen(pid_gt_end) ); */

/* 	strncat( *ptr_reason, pid_gt_end, strlen(pid_gt_end) ); */

/* 	(*in)++; */
/*   } */
/*   //   policy issue date is greater then policy status date */
/*   if( g_date_valid(pid) && g_date_valid(psd) && g_date_compare( pid, psd) > 0) { */
/* 	char pid_gt_psd[] = "Policy issued after status changes, "; */

/* 	*ptr_reason = (char *) realloc( *ptr_reason, strlen(pid_gt_psd) ); */

/* 	strncat( *ptr_reason, pid_gt_psd, strlen(pid_gt_psd) ); */

/* 	(*in)++; */
/*   } */
/*   //   policy status date is smaller then study start date */
/*   if( g_date_valid(psd) && g_date_compare( psd, s) < 0) { */
/* 	char psd_lt_start[] = "Policy status date before start of study, "; */

/* 	*ptr_reason = (char *) realloc( *ptr_reason, strlen(psd_lt_start) ); */

/* 	strncat( *ptr_reason, psd_lt_start, strlen( psd_lt_start) ); */

/* 	(*in)++; */
/*   } */

/*   g_date_free(e);    e = NULL; */
/*   g_date_free(s);    s = NULL; */
/*   g_date_free(pid);  pid = NULL; */
/*   g_date_free(psd);  psd = NULL; */
/* } */
