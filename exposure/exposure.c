/*   
	 run as 

	 printf 'id;date_of_birth;policy_issue_date;policy_status_code;policy_status_date\n' > stdin.txt
	 printf '1234;1982-11-17;2010-01-01;1;\n' >> stdin.txt 
	 printf '5678;1977-06-23;2012-03-04;3;2015-09-17\n' >> stdin.txt 
	 tail +2 stdin.txt | ./exposure --start=2010-01-01 --end=2021-12-31 --type=3

*/

// --------------------------------------------------------------------------------------------------------------------------
// C libraries to get the job done
//
#include <stdio.h>       // stdin, stdout, stderr, fprintf, FILE, ...
#include <string.h>      // strings parsing: strtok_r, strlen, strcmp, strcat, strncpy
#include <stdlib.h>      // malloc, calloc, realloc, memset ...
#include <stdbool.h>     // bool (data type)
#include <locale.h>      // setlocale()
#include <getopt.h>      // command line arguments: getopt_long()
#include <glib.h>        // date calculations: g_date...
#include <time.h>        // time annotations: time()

// Options for the number of days in a year
//
//  (1) no adjustment for leap years:        365      days/year    
//  (2) accurate adjustment for leap years:  365.2425 days/year --> 365.2425 = ( 291 * 366 + 909 * 365 ) / 1200  )
//  (3) fair adjustment for leap years:      365.25   days/year --> 365.25   = ( 3 * 365 + 366 ) / 4
//
const float DAYS_IN_YEAR = 365.25;
//
// Field delimiter in stdin stream
//
const char *DELIM = ";";

// Relevant data structures for the experience study
//
//  common parameters (study level)
typedef struct
{
  char *start;         // start date of experience study (must be a valid date YYYY-MM-DD)
  char *end;           // end date of experience study (must be a valid date YYYY-MM-DD)
  char *type           // type of experience study ( 2 Lapse, 3 Mortality, 4 Accidental Death, 5 TPD Disease, 6 TPD Accident
} exp_study;
//
//  policy level parameters
typedef struct
{
  char *id;            // any identification possible, must be unique to each policy
  char *date_of_birth; // date of birth of policyholder (must be a valid date YYYY-MM-DD)
  char *issue_date;    // day at which policyholder turned into client (must be a valid date YYYY-MM-DD)
  char *status_code;   // 1 Inforce, 2 Lapsed, 3 Death, 4 Accidental Death, 5 TPD Disease, 6 TPD Accident
  char *status_date;   // date detailing the status code (must be a valid date YYYY-MM-DD except for status code 1)
} policy;

// Structs containing
//
//  - the experience study parameters (initialized to NULL pointer)
exp_study *study = NULL;
//
//  - and the policy parameters (initialized to NULL pointer)
policy *policy = NULL;

// --------------------------------------------------------------------------------------------------------------------------
//  prototypes of the functions
void study_parameters(
					  int argc            // amount of command-line arguments
					  ,char **argv        // array of command-line arguments
					  ,bool *ok           // flag for validity of study based on the parameters given by the user
					  ,exp_study **study  // pointer to pointer to struct containing pointers to parameters
					  );

// --------------------------------------------------------------------------------------------------------------------------
// main 
int main(int argc, char **argv){
  
  // Improve guessing power from glib's g_date_set_parse() function
  setlocale( LC_ALL, "");

  // File connections
  //
  //  ~exposures.csv~ file contains the exposures for each policyholder at each policy year in the experience study
  FILE *f_exp = fopen("exposures.csv", "w");
  if( f_exp == NULL ){
	fprintf( stderr, "Could not open file 'exposures.csv'. Aborting...\n");
	exit( EXIT_FAILURE );
  }
  //
  //  ~out_of_study.csv~ file is the LOG of policies which are not exposed to study due to inconsistencies in its inputs
  FILE *f_out = fopen("out_of_study.csv", "w");
  if( f_out == NULL ){
	fprintf( stderr, "Could not open file 'out_of_study.csv'. Aborting...\n");
	exit( EXIT_FAILURE );
  }

  
  // Step 1: Check study parameters for valid inputs
  //
  //   Do's:
  //     1.1 Both study start and study end dates must be valid
  //     1.2 Study start date must be older than study end date
  //     1.3 Study type must be a valid integer between 1 and 6
  //
  bool valid_study = true; 
  study_parameters(argc, argv, &valid_study, &study);

  // if any condition 1.1, 1.2 or 1.3 above fails, print error and abort execution.
  if ( valid_study == false ) {
	fprintf( stderr, "Inconsistent study parameters. Exiting...\n" );
	exit( EXIT_FAILURE );
  }

  // Step 2: Read each line of stdin
  char *line = NULL;
  size_t len = 0;
  ssize_t read = 0;
  
  while ( (read = getline( &line, &len, stdin )) != 1 ) {

	// Step 3: Tokenize line and store policy inputs into the struct ~policy~
	tokenize( line, &policy);

	// Step 4: Validate policy inputs and flag its exposure to study
	//
	//  Do's:
	//    4.1  Date of birth must be a valid date
	//    4.2  Policy issue date must be a valid date
	//    4.3  Policy status code must be a valid integer between 1 and 6
	//    4.4  Policy status date must be a valid date (when policy status code is not 1)
	//    4.5  Date of birth must be older than policy issue date
	//    4.6  Policy issue date must be older than policy status date (when policy status code is not 1)
	//    4.7  Policy issue date must be older than study end date
	//    4.8  Policy status date must be newer than study start date
	//
	//  Result:  If any from 4.1-4.8 fails...
	//    R1. Flag inconsistencies into log file ~out_of_study.csv~ ( FILE *f_out )
	//    R2. Flag ~exposed_policy~to false
	//
	bool exposed_policy = true;
	validate( policy, &exposed_policy, f_out );
	if ( exposed_policy == false)
	  continue; // go to next line
	
	// Step 5: Calculate exposure by policy year for policies exposed to study
	// 
	//  Export results into file ~exposures.csv~ ( FILE *f_exp ) and into well-formated stdout
	
	  
  } // while

  // Step 6: Free memory of allocated structs and pointers used until now
  free(line);
  free(policy);
  free(study);
  free(f_exp);
  free(f_out);

  return EXIT_SUCCESS;
} // main




// --------------------------------------------------------------------------------------------------------------------------
// declarations of functions
void study_parameters(
					  int argc            // amount of command-line arguments
					  ,char **argv        // array of command-line arguments
					  ,bool *ok           // flag for validity of study based on the parameters given by the user
					  ,exp_study **study  // pointer to pointer to struct containing pointers to parameters
					  ){

  // Read the command line arguments --start=YYYY-MM-DD --end=YYYY-MM-DD --type=int
  // and return ~false~ to the variable ~valid_study~ in main() in case of any errors

  // Set study pointer to NULL before any step
  *study = NULL;
  
  // temporary variable for correct parsing of study start and study end dates
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
		  fprintf( stderr, "Missing mandatory study parameters: start, end and type.\n");
		  *ok = false; // setting flag on due to the error
		  break;

		case 's':
		  // Read in the study start date
		  g_date_set_parse (date, optarg);
		  if ( g_date_valid(date) ){
			(*study)->start = (char *) realloc( (*study)->start, strlen(optarg) );
			strncpy( (*study)->start, optarg, strlen(optarg));
		  }
		  else {
			fprintf( stderr, "Study start date must be a valid date.\n");
			*ok = false; // setting flag on due to the error
		  }
		  break;

		case 'e':
		  // Read in the study end date
		  g_date_set_parse (date, optarg);
		  if( g_date_valid(date) ){
			(*study)->end = (char *) realloc( (*study)->end, strlen(optarg) );
			strncpy( (*study)->end, optarg, strlen(optarg));
		  }
		  else {
			fprintf( stderr, "Study end date must be a valid date.\n");
			*ok = false; // setting flag on due to the error
		  }
		  break;

		case 't':
		  // Read in the study type
		  if ( atoi(optarg) == 0 ){
			fprintf( stderr, "Study type must be a integer.\n");
			*ok = false; // setting flag on due to the error
		  } else {
			(*study)->type = (char *) realloc( (*study)->type, strlen(optarg) );
			strncpy( (*study)->type, optarg, strlen(optarg));
		  }
		  break;

		case '?':
		  printf("Unknown option %c\n", optopt);
		  *ok = false; // setting flag on due to the error
		  break;

		case ':':
		  printf("Missing option for %c\n", optopt);
		  *ok = false; // setting flag on due to the error
		  break;
		} // switch case
	} // while

  // study start date after study end date
  if( ((*study)->start != NULL) && ((*study)->end != NULL) && ( strcmp( (*study)->start, (*study)->end ) >= 0) ){
	fprintf( stderr, "Study start date must be before study end date.\n");
	*ok = false; // setting flag on due to the error
  }
  
  g_date_free(date);

}
