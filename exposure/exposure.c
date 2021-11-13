/*   
	 run as 

	 printf 'id;dob;gender;policy_issue_date;policy_status_code;policy_status_date\n' > linhas.txt
	 printf '1234;1982-11-17;1;2010-01-01;1;\n' >> linhas.txt 
	 printf '5678;1977-06-23;2;2012-03-04;3;2015-09-17\n' >> linhas.txt 
	 tail -n+2 linhas.txt | ./exposure --start=2010-01-01 --end=2021-12-31 --type=3

*/

#include <stdio.h>       // stdin, stdout, stderr, fprintf, ...
#include <string.h>      // strings parsing: strtok_r
#include <stdlib.h>      // malloc, calloc, realloc ...
#include <stdbool.h>     // boolean data type
#include <locale.h>      // setlocale()
#include <getopt.h>      // command line arguments: getopt_long()
#include <glib.h>        // date calculations: g_date...
#include <time.h>        // time annotations: time()

// 
// Options for the number of days in a year
//  (1) without adjustment for leap years:   365      days/year    
//  (2) exact adjustment for leap years:     365.2425 days/year --> 365.2425 = ( 291 * 366 + 909 * 365 ) / 1200  )
//  (3) adjusted for leap years:             365.25   days/year --> 365.25   = ( 3 * 365 + 366 ) / 4
//
const float DAYS_IN_YEAR = 365.25;
//
// Field delimiter in stdin stream
//
const char *DELIM = ";";


// Relevant data structures for the experience study
//  common parameters (study level)
typedef struct
{
  char *start;
  char *end;
  char *type
} exp_study;
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
//  the experience study parameters (initialized to NULL pointer)
exp_study *study = NULL;
//  and the policy details (initialized to NULL pointer)
policy *policy = NULL;

//  prototypes of the functions
void study_parameters(
					  int argc            // amount of command-line arguments
					  ,char **argv        // array of command-line arguments
					  ,bool *ok           // flag for validity of study based on the parameters given by the user
					  ,exp_study **study  // pointer to pointer to struct containing pointers to parameters
					  );

// main 
int main(int argc, char **argv){
  
  // Improve guessing power from glib's g_date_set_parse() function
  setlocale( LC_ALL, "");
  
  // Step 1: Check study parameters for valid inputs.
  //
  //   Do's:
  //     1.1 Both study start and study end dates must be valid
  //     1.2 Study start date must be older than study end date
  //     1.3 Study type must be a valid integer between 1 and 7
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
  
  ;
  while ( (read = getline( &line, &len, stdin )) != 1  ) {

	// Step 3: Tokenize line, storing the inputs in the struct 'policy'
	tokenize( line, &policy);
	  
	// if not exposed to risk, skip line but document reason in a log file
	// if exposed, proceed to calculations and print results to stdout (long-format)
	// 
  } // while
	  
  /* frees memory allocated for reading the lines of stdin */
  free(line);

  return EXIT_SUCCESS;
} // main





// declarations of functions
