/*   
	 run as 

	 printf 'id;dob;gender;policy_issue_date;policy_status_code;policy_status_date\n' > linhas.txt
	 printf '1234;1982-11-17;1;2010-01-01;1;\n' >> linhas.txt 
	 printf '5678;1977-06-23;2;2012-03-04;3;2015-09-17\n' >> linhas.txt 
	 tail -n+2 linhas.txt | ./exposure --start=2010-01-01 --end=2021-12-31 --type=3

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
   (2) exact adjustment for leap years:     365.2425 days/year --> 365.2425 = ( 291 * 366 + 909 * 365 ) / 1200  )
   (3) adjusted for leap years:             365.25   days/year --> 365.25   = ( 3 * 365 + 366 ) / 4
*/
const float DAYS_IN_YEAR = 365.25;
const char *DELIM = ";";

// Relevant data structures for experience study
// common parameters (in the study level)
struct exp_study
{
  char start[10];
  int type;
  char end[10];
};
// parameters in the policy level
struct policy
{
  int id;                 // any identification possible
  char dob[10];           // date of birth (YYYY-MM-DD)
  int gender;             // 1 (female) or 2 (male)
  char issue_date[10];    // YYYY-MM-DD
  int status_code;        // 1 Inforce, 2 Lapsed, 3 Death, 4 Accidental Death, 5 TPD Disease, 6 TPD Accident
  char status_date[10];   // date corresponding to the status code (YYYY-MM-DD)
};


// Struct containing the experience study parameters
struct exp_study study; 


//  prototypes of the functions
void read_study_parameters(int argc, char **argv, int *study_params_ok);
void tokenize_policy_data(char *line, struct policy *policy_data);

// main 
int main(int argc, char **argv){

  setlocale( LC_ALL, "");
  
  /* Checks study parameters for valid values. */
  int study_params_ok = 0;
  read_study_parameters(argc, argv, &study_params_ok);

  if (study_params_ok == 0) {

	// data structure to store policyholder data from stdin
	struct policy policy = { // initialize to NUL for code cleaness
	  .id = 0,
	  .dob = "",
	  .gender = 0,
	  .issue_date = "",
	  .status_code = 0,
	  .status_date = ""
	}; 

	/* Reads input data from policyholders via 'stdin', one line at the time */
	char *line = NULL;
	size_t len = 0;
	ssize_t read = 0;

	while (1) {
	  read = getline(&line, &len, stdin);
	  if (read == -1)
		break;

	  // tokenize line into policyholders data structures
	  tokenize_policy_data( line, &policy);

	  // check if the policyholder belongs to the study (i.e., if it is exposed to risk)

	  int belongs_to_study = 0;
	  char reason_out[1024] = "";  
	  
	  GDate *e = g_date_new();
	  GDate *s = g_date_new();
	  GDate *pid = g_date_new();
	  GDate *psd = g_date_new();

	  g_date_set_parse(e, study.end);
	  g_date_set_parse(s, study.start);
	  g_date_set_parse(pid, policy.issue_date);
	  g_date_set_parse(psd, policy.status_date);

	  // not exposed to risk if...
	  //   any of the dates are invalid
	  if( !g_date_valid(pid) ){
		strcat( reason_out, "Invalid policy issue date; ");
		belongs_to_study++;
	  }
	  if( strlen(policy.status_date) > 5 && !g_date_valid(psd) ){
		strcat( reason_out, "Invalid policy status date; ");
		belongs_to_study++;
	  }
	  //   policy issue date is greater then study end date
	  if( g_date_compare( pid, e) > 0 ) {
		strcat( reason_out, "Policy issued after end of study; ");
		belongs_to_study++;
	  }
	  //   policy issue date is greater then policy status date
	  if( strlen(policy.status_date) > 5 && g_date_compare( pid, psd) > 0) {
		strcat( reason_out, "Policy issued after status changes; ");
		belongs_to_study++;
	  }
	  //   policy status date is smaller then study start date
	  if( strlen(policy.status_date) > 5 && g_date_compare( psd, e) > 0) {
		strcat( reason_out, "Policy issued after end of study; ");
		belongs_to_study++;
	  }

	  g_date_free(e);
	  g_date_free(s);
	  g_date_free(pid);
	  g_date_free(psd);

	  
	  // if not exposed to risk, skip line but document reason in a log file
	  // if exposed, proceed to calculations and print results to stdout (long-format)
	  // 
 
	}
	
	  
	/* frees memory allocated for reading the lines of stdin */
	free(line);

}

  
  return EXIT_SUCCESS;
}



// declarations of functions
void tokenize_policy_data(char *line, struct policy *policy_data){
  char *token;
  char *rest;

  rest=line;

  /* parsing the policyholder's ID */
  token = strtok_r(rest, DELIM, &rest);
  /* strncpy( policy_data->id, token, 255); */
  policy_data->id = atoi( token);

  /* parsing the date of birth of the policyholder */
  token = strtok_r(rest, DELIM, &rest);
  strncpy( policy_data->dob, token, 11);

  /* parsing the gender of policyholder  */
  token = strtok_r(rest, DELIM, &rest);
  /* strncpy( policy_data->gender, token, 6); */
  policy_data->gender = atoi(token);

  /* parsing the issue date of the policy */
  token = strtok_r(rest, DELIM, &rest);
  strncpy( policy_data->issue_date, token, 11);

  /* parsing the status code of the policy */
  token = strtok_r(rest, DELIM, &rest);
  /* strncpy( policy_data->status_code, token, 6); */
  policy_data->status_code = atoi(token);

  /* parsing the date regarding the status date */
  token = strtok_r(rest, DELIM, &rest);
  strncpy( policy_data->status_date, token, 11);
  
}



void read_study_parameters(int argc, char **argv, int *study_params_ok){ 
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
		/* case 0: */
		/*   printf("long option %s", long_options[option_index].name); */
		/*   if (optarg) */
		/* 	printf(" with arg %s", optarg); */
		/*   printf("\n"); */
		/*   break; */

		case 1:
		  fprintf( stderr, "Missing study parameters: start, end and type.\n");
		  *study_params_ok = 1; // setting flag on due to the error
		  break;

		case 's':
		  // Read in the study start date
		  g_date_set_parse (date, optarg);
		  if( g_date_valid(date) ){
			strncpy( study.start, optarg, 10);
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
			strncpy(study.end, optarg, 10);
		  }
		  else {
			fprintf( stderr, "Study end date must be a valid date.\n");
			*study_params_ok = 1; // setting flag on due to the error
		  }
		  break;

		case 't':
		  // Read in the study type
		  if ( !( study.type = atoi(optarg) ) ){
			fprintf( stderr, "Study type must be a integer.\n");
			*study_params_ok = 1; // setting flag on due to the error
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
