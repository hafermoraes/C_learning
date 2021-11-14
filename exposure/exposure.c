/*   
  create test stdin for simulating
    printf 'id;date_of_birth;policy_issue_date;policy_status_code;policy_status_date\n' > stdin.txt
	printf '1234;1982-11-17;2010-01-01;1;\n' >> stdin.txt 
	printf '5678;1977-06-23;2012-03-04;3;2015-09-17\n' >> stdin.txt 
	printf '91011;1977-06-23;2012-33-04;3;2015-09-17\n' >> stdin.txt 
	printf '121314;1977-06-23;2012-03-04;3;\n' >> stdin.txt 

  run as 
    tail +2 stdin.txt | ./exposure --start=2000-01-01 --end=2008-12-31 --type=3

  check memory leaks with valgrind as
    tail +2 stdin.txt | valgrind ./exposure --start=2000-01-01 --end=2008-12-31 --type=3

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
typedef struct study_str
{
  char *start;         // start date of experience study (must be a valid date YYYY-MM-DD)
  char *end;           // end date of experience study (must be a valid date YYYY-MM-DD)
  char *type;          // type of experience study ( 2 Lapse, 3 Mortality, 4 Accidental Death, 5 TPD Disease, 6 TPD Accident
} study_str;
//
//  policy level parameters
typedef struct policy_str
{
  char *id;            // any identification possible, must be unique to each policy
  char *date_of_birth; // date of birth of policyholder (must be a valid date YYYY-MM-DD)
  char *issue_date;    // day at which policyholder turned into client (must be a valid date YYYY-MM-DD)
  char *status_code;   // 1 Inforce, 2 Lapsed, 3 Death, 4 Accidental Death, 5 TPD Disease, 6 TPD Accident
  char *status_date;   // date detailing the status code (must be a valid date YYYY-MM-DD except for status code 1)
} policy_str;

// Structs containing
//
//  - the experience study parameters (initialized to NULL pointer)
study_str *study = NULL;
//
//  - and the policy parameters (initialized to NULL pointer)
policy_str *policy = NULL;

// --------------------------------------------------------------------------------------------------------------------------
//  prototypes of the functions
void study_parameters(
					  int argc            // amount of command-line arguments
					  ,char **argv        // array of command-line arguments
					  ,bool *ok           // flag for validity of study based on the parameters given by the user
					  ,study_str **study  // pointer to pointer to struct containing pointers to parameters
					  );
void tokenize(
			  char *line             // pointer to single, one at a time, line read from stdin
			  ,policy_str **policy   // pointer to pointer to policy struct where inputs will be copied
			  );
void validate(
			  study_str *study    // pointer to struct containing pointers to parameters
			  ,policy_str *policy // pointer to policy struct with parsed inputs to be validated
			  ,bool *exposed      // pointer to boolean flag controlling if policy is exposed to study
			  ,FILE *f_out        // pointer to LOG file ~f_out~, where the problems will be listed if policy is not exposed to study
			  );


// --------------------------------------------------------------------------------------------------------------------------
// main 
int main(int argc, char **argv){
  
  // Improve guessing power from glib's g_date_set_parse() function
  setlocale( LC_ALL, "");

  // File connections
  //
  //  ~exposures.csv~ file with the exposures for each policyholder at each policy year in the experience study
  FILE *f_exp = fopen("exposures.csv", "w");
  if( f_exp == NULL ){
	fprintf( stderr, "Could not open file 'exposures.csv'. Aborting...\n");
	exit( EXIT_FAILURE );
  }
  //
  //  ~out_of_study.csv~ file with the LOG of policies not exposed to study due to inconsistencies in their inputs
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
  
  read = getline(&line, &len, stdin);
  while ( read >= 0  ) {
	// Step 3: Tokenize line and store policy inputs into the struct ~policy~
	tokenize( line, &policy);

	// Step 4: Validate policy inputs and flag its exposure to study
	//
	//  Do's:
	//    4.1  Date of birth must be a valid date
	//    4.2  Policy issue date must be a valid date
	//    4.3  Policy status code must be a valid integer between 1 and 6
	//    4.4  Policy status date must be a valid date (when policy status code is not 1)
	//    4.5  Date of birth must be older than study end date
	//    4.6  Policy issue date must be older than policy status date (when policy status code is not 1)
	//    4.7  Policy issue date must be older than study end date
	//    4.8  Policy status date must be newer than study start date
	//    4.9  Date of birth must be earlier than policy issue date
	//
	//  Result:  If any from 4.1-4.8 fails...
	//    R1. Flag inconsistencies into log file ~out_of_study.csv~ ( FILE *f_out )
	//    R2. Flag ~exposed_policy~to false
	//
	bool exposed_policy = true;
	validate( study, policy, &exposed_policy, f_out );

	// Step 5: Calculate exposure by policy year for policies exposed to study
	// 
	//  Export results into file ~exposures.csv~ ( FILE *f_exp ) and into well-formated stdout
	/* if ( exposed_policy == true){ */
	/*   calculate_exposure( study, policy ); */
	/* } */

	// Step 6: Free memory of ~policy~ struct and its pointers to ~id~, ~date_of_birth~, ~issue_date~, ~status_code~ and ~status_date~
	free( policy->id );
	free( policy->date_of_birth );
	free( policy->issue_date );
	free( policy->status_code );
	free( policy->status_date );
	free( policy );
	
	// reads in next line from stdin
	read = getline(&line, &len, stdin);
  } // while

  // Step 7: Free memory of allocated structs and pointers
  //   7.1 ~line~, used to read lines of stdin 
  free(line);
  //   7.2 ~study~ struct and its pointers to ~start~, ~end~ and ~type~
  free( study->start );
  free( study->end   );
  free( study->type  );
  free( study );

  // Step 8: Close file connections to ~exposures.csv~ and ~out_of_study.csv~.
  fclose(f_exp);
  fclose(f_out);

  return EXIT_SUCCESS;
} // main




// --------------------------------------------------------------------------------------------------------------------------
// declarations of functions
void study_parameters(
					  int argc            // amount of command-line arguments
					  ,char **argv        // array of command-line arguments
					  ,bool *ok           // flag for validity of study based on the parameters given by the user
					  ,study_str **study  // pointer to pointer to struct containing pointers to parameters
					  ){

  // Read the command line arguments --start=YYYY-MM-DD --end=YYYY-MM-DD --type=int
  // and return ~false~ to the variable ~valid_study~ in main() in case of any errors

  // Set study pointer to NULL before allocating memory to it
  *study = NULL;
  *study = (study_str *) malloc( sizeof(study_str) );
  if (*study == NULL){
	fprintf( stderr, "Could not allocate memory for ~study_str~ pointer from within ~study_parameters()~ function. Aborting...\n");
	exit( EXIT_FAILURE );
  }
  // By succeeding memory allocation, initialize the struct pointers all to NULL
  (*study)->start = NULL;
  (*study)->end = NULL;
  (*study)->type = NULL;

  int study_type = 0;
  
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
			if ( (*study)->start == NULL ){
			  fprintf( stderr, "Could not allocate memory for ~(*study)->start~ pointer from within ~study_parameters()~ function. Aborting...\n");
			  exit( EXIT_FAILURE );
			}
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
			if ( (*study)->end == NULL ){
			  fprintf( stderr, "Could not allocate memory for ~(*study)->end~ pointer from within ~study_parameters()~ function. Aborting...\n");
			  exit( EXIT_FAILURE );
			}
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
			if ( (*study)->type == NULL ){
			  fprintf( stderr, "Could not allocate memory for ~(*study)->type~ pointer from within ~study_parameters()~ function. Aborting...\n");
			  exit( EXIT_FAILURE );
			}
			strncpy( (*study)->type, optarg, strlen(optarg));
			study_type = atoi((*study)->type);
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

  // study type non numeric or outside interval [1,6]
  if( study_type == 0 || !(study_type >= 1 && study_type <= 6)){
	fprintf( stderr, "Study type must be a number between 1 and 6 .\n");
	*ok = false; // setting flag on due to the error
  }

  // frees memory of the temporary variable for correct parsing of study start and study end dates
  g_date_free(date);

}

void tokenize(
			  char *line             // single, one at a time, line read from stdin
			  ,policy_str **policy   // pointer to pointer to policy struct where inputs will be copied
			  ){
  // read one line at a time from stdin
  // and tokenize its content into the policy struct elements (pointers to id, DOB, issue date, status code and status date)

  // Set ~policy~ pointer to NULL before allocating memory to it
  *policy = NULL;
  *policy = (policy_str *) malloc( sizeof(policy_str) );
  if (*policy == NULL){
	fprintf( stderr, "Could not allocate memory for ~policy_str~ pointer from within ~tokenize()~ function. Aborting...\n");
	exit( EXIT_FAILURE );
  }
  // By succeeding memory allocation, initialize the struct pointers all to NULL
  (*policy)->id = NULL;
  (*policy)->date_of_birth = NULL;
  (*policy)->issue_date = NULL;
  (*policy)->status_code = NULL;
  (*policy)->status_date = NULL;

  // pointers needed to tokenize the read line
  char *token = NULL;
  char *rest = line;
  int token_len = 0;

  // parsing the policyholder's ID
  //token = strtok_r(rest, DELIM, &rest);
  token = strsep( &rest, DELIM);
  token_len  = (strlen(token)==0) ? 1 : strlen(token);
  (*policy)->id = (char *) realloc( (*policy)->id, token_len + 1);
  if( (*policy)->id == NULL){
	fprintf( stderr, "Could not allocate memory for ~(*policy)->id~ pointer from within ~tokenize()~ function. Aborting...\n");
	exit( EXIT_FAILURE );
  }
  strcpy( (*policy)->id, token ) ;

  // parsing the date of birth of the policyholder
  //token = strtok_r(rest, DELIM, &rest);
  token = strsep( &rest, DELIM);
  token_len = (strlen(token)==0) ? 1 : strlen(token);
  (*policy)->date_of_birth = (char *) realloc( (*policy)->date_of_birth, token_len + 1 );
  if( (*policy)->date_of_birth == NULL){
	fprintf( stderr, "Could not allocate memory for ~(*policy)->date_of_birth~ pointer from within ~tokenize()~ function. Aborting...\n");
	exit( EXIT_FAILURE );
  }
  strcpy( (*policy)->date_of_birth, token ) ;

  // parsing the issue date of the policy
  //token = strtok_r(rest, DELIM, &rest);
  token = strsep( &rest, DELIM);
  token_len = (strlen(token)==0) ? 1 : strlen(token);
  (*policy)->issue_date = (char *) realloc( (*policy)->issue_date, token_len + 1 );
  if( (*policy)->issue_date == NULL){
	fprintf( stderr, "Could not allocate memory for ~(*policy)->issue_date~ pointer from within ~tokenize()~ function. Aborting...\n");
	exit( EXIT_FAILURE );
  }
  strcpy( (*policy)->issue_date, token ) ;

  // parsing the status code of the policy
  //token = strtok_r(rest, DELIM, &rest);
  token = strsep( &rest, DELIM);
  token_len = (strlen(token)==0) ? 1 : strlen(token);
  (*policy)->status_code = (char *) realloc( (*policy)->status_code, token_len + 1);
  if( (*policy)->status_code == NULL){
	fprintf( stderr, "Could not allocate memory for ~(*policy)->status_code~ pointer from within ~tokenize()~ function. Aborting...\n");
	exit( EXIT_FAILURE );
  }
  strcpy( (*policy)->status_code, token ) ;

  // parsing the date regarding the status date
  //token = strtok_r(rest, DELIM, &rest);
  token = strsep( &rest, DELIM);
  token_len = (strlen(token)==0) ? 1 : strlen(token);
  (*policy)->status_date = (char *) realloc( (*policy)->status_date, token_len + 1);
  if( (*policy)->status_date == NULL){
	fprintf( stderr, "Could not allocate memory for ~(*policy)->status_date~ pointer from within ~tokenize()~ function. Aborting...\n");
	exit( EXIT_FAILURE );
  }
  memset( (*policy)->status_date, 0, sizeof((*policy)->status_date) );
  strcpy( (*policy)->status_date, token ) ; 

  // Free memory from pointers used for tokenize the read line from stdin
  token = NULL;
  rest = NULL;
}

void validate(
			  study_str *study    // pointer to struct containing pointers to study parameters
			  ,policy_str *policy // pointer to policy struct with parsed inputs to be validated
			  ,bool *exposed      // pointer to boolean flag controlling if policy is exposed to study
			  ,FILE *f_out        // pointer to LOG file ~f_out~, where the problems will be listed if policy is not exposed to study
			  ){
  //  Validations done in this function
  //
  //   Individual ones
  //    - I1. Date of birth must be a valid date
  //    - I2. Policy issue date must be a valid date
  //    - I3. Policy status code must be a valid integer between 1 and 6
  //    - I4. Policy status date must be a valid date (when policy status code is valid and not equal to 1)
  //
  //   Compound ones
  //    - C1. Date of birth must be older then study end date
  //    - C2. Policy issue date must be older than policy status date (when policy status code is valid and not equal to 1)
  //    - C3. Policy issue date must be older than study end date
  //    - C4. Policy status date must be sooner than study start date
  //    - C5. Date of birth must be earlier than policy issue date
  //
  //  Result:  If any of the above fails, then
  //    - Flag inconsistencies into log file ~out_of_study.csv~ ( FILE *f_out )
  //    - Flag ~exposed_policy~to false

  // Declaration of variables used to validate exposure of policy to study
  GDate   *s = g_date_new(); // study start date
  GDate   *e = g_date_new(); // study end date
  GDate *dob = g_date_new(); // policyholder's date of birth
  GDate *pid = g_date_new(); // policy issue date
  int    psc = 0;            // policy status code (PSC)
  bool psc_valid;            // PSC numeric and between 1 and 6
  GDate *psd = g_date_new(); // policy status date
  
  //  both study start and study end dates are valid as a result of function ~study_parameters()~
  g_date_set_parse( s, study->start );
  g_date_set_parse( e, study->end );
  
  // Individual validations
  //
  //  I1. Policyholder's Date of Birth must be a valid date
  g_date_set_parse( dob, policy->date_of_birth );
  if( !g_date_valid(dob) ){
	fprintf( f_out, "%s;Invalid date of birth;%s\n", policy->id, policy->date_of_birth );
	*exposed = false;
  }
  //  I2. Policy issue date must be a valid date
  g_date_set_parse( pid, policy->issue_date );
  if( !g_date_valid(pid) ){
	fprintf( f_out, "%s;Invalid policy issue date;%s\n", policy->id, policy->issue_date );
	*exposed = false;
  }
  //  I3. Policy status code must be a valid integer between 1 and 6
  psc_valid = true;
  if(
	 (psc = atoi(policy->status_code)) == 0 || // if policy status code is not a number OR
	 !(atoi(policy->status_code) >= 1 && atoi(policy->status_code) <=6) // is not 1,2,3,4,5 nor 6
	 ){
	fprintf( f_out, "%s;Invalid policy status code (must be a number between 1 and 6);%s\n", policy->id, policy->status_code );
	*exposed = false;
	psc_valid = false;
  }
  //  I4. Policy status date must be a valid date (when policy status code is valid and not equal to 1)
  g_date_set_parse( psd, policy->status_date );
  if( psc_valid == true && psc != 1 && !g_date_valid(psd) ){
	fprintf( f_out, "%s;Invalid policy status date;%s\n", policy->id, policy->status_date );
	*exposed = false;
  }

  // Compound validations
  //
  //  C1. Date of birth must be older then study end date
  if ( g_date_valid(dob) && g_date_compare(dob, e) >= 0 ){
	fprintf( f_out, "%s;Date of birth (DOB) after study end date (EOS);DOB %s >= EOS %s\n", policy->id, policy->date_of_birth, study->end );
	*exposed = false;
  }
  //  C2. Policy issue date must be older than policy status date (when policy status code is valid and not equal to 1)
  if ( psc_valid == true && psc != 1 && g_date_valid(pid) && g_date_valid(psd) && g_date_compare(pid, psd) >= 0 ){
	fprintf( f_out, "%s;Policy issue date (PID) after Policy status date (PSD);PID %s >= PSD %s\n", policy->id, policy->issue_date, policy->status_date );
	*exposed = false;
  }
  //  C3. Policy issue date must be older than study end date
  if ( g_date_valid(pid) && g_date_compare(pid, e) >= 0 ){
	fprintf( f_out, "%s;Policy issue date (PID) after study end date (EOS);PID %s >= EOS %s\n", policy->id, policy->issue_date, study->end );
	*exposed = false;
  }
  //  C4. Policy status date must be sooner than study start date
  if ( psc_valid == true && psc != 1 && g_date_valid(psd) && g_date_compare(psd, s) < 0 ){
	fprintf( f_out, "%s;Policy status date (PSD) before Study start date (SOS);PSD %s < SOS %s\n", policy->id, policy->status_date, study->start );
	*exposed = false;
  }
  //  C5. Date of birth must be earlier than policy issue date
  if ( g_date_valid(dob) && g_date_valid(pid) && g_date_compare(dob, pid) >= 0 ){
	fprintf( f_out, "%s;Date of birth (DOB) after Policy issue date (PID);DOB %s > PID %s\n", policy->id, policy->date_of_birth, policy->issue_date );
	*exposed = false;
  }
  
  // frees memory from GDate pointers use to validate dates
  g_date_free(s);
  g_date_free(e);
  g_date_free(pid);
  g_date_free(dob);
  g_date_free(psd);
}
