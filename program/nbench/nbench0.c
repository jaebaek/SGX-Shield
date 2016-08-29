
/*
** nbench0.c
*/

/*******************************************
**             BYTEmark (tm)              **
** BYTE MAGAZINE'S NATIVE MODE BENCHMARKS **
**           FOR CPU/FPU                  **
**             ver 2.0                    **
**       Rick Grehan, BYTE Magazine       **
********************************************
** NOTE: These benchmarks do NOT check for the presence
** of an FPU.  You have to find that out manually.
**
** REVISION HISTORY FOR BENCHMARKS
**  9/94 -- First beta. --RG
**  12/94 -- Bug discovered in some of the integer routines
**    (IDEA, Huffman,...).  Routines were not accurately counting
**    the number of loops.  Fixed. --RG (Thanks to Steve A.)
**  12/94 -- Added routines to calculate and display index
**    values. Indexes based on DELL XPS 90 (90 MHz Pentium).
**  1/95 -- Added Mac time manager routines for more accurate
**    timing on Macintosh (said to be good to 20 usecs) -- RG
**  1/95 -- Re-did all the #defines so they made more
**    sense.  See NMGLOBAL.H -- RG
**  3/95 -- Fixed memory leak in LU decomposition.  Did not
**    invalidate previous results, just made it easier to run.--RG
**  3/95 -- Added TOOLHELP.DLL timing routine to Windows timer. --RG
**  10/95 -- Added memory array & alignment; moved memory
**      allocation out of LU Decomposition -- RG
**
** DISCLAIMER
** The source, executable, and documentation files that comprise
** the BYTEmark benchmarks are made available on an "as is" basis.
** This means that we at BYTE Magazine have made every reasonable
** effort to verify that the there are no errors in the source and
** executable code.  We cannot, however, guarantee that the programs
** are error-free.  Consequently, McGraw-HIll and BYTE Magazine make
** no claims in regard to the fitness of the source code, executable
** code, and documentation of the BYTEmark.
**  Furthermore, BYTE Magazine, McGraw-Hill, and all employees
** of McGraw-Hill cannot be held responsible for any damages resulting
** from the use of this code or the results obtained from using
** this code.
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "nmglobal.h"
#include "nbench0.h"
#include "../enclave.h"

/*************
**** main ****
*************/
void enclave_main()
{
int i;                  /* Index */
double bmean;           /* Benchmark mean */
double bstdev;          /* Benchmark stdev */
double lx_memindex;     /* Linux memory index (mainly integer operations)*/
double lx_intindex;     /* Linux integer index */
double lx_fpindex;      /* Linux floating-point index */
double intindex;        /* Integer index */
double fpindex;         /* Floating-point index */
ulong bnumrun;          /* # of runs */

/*
** Set global parameters to default.
*/
global_min_ticks=MINIMUM_TICKS;
global_min_seconds=MINIMUM_SECONDS;
global_allstats=0;
global_custrun=0;
global_align=8;
lx_memindex=(double)1.0;        /* set for geometric mean computations */
lx_intindex=(double)1.0;
lx_fpindex=(double)1.0;
intindex=(double)1.0;
fpindex=(double)1.0;
mem_array_ents=0;               /* Nothing in mem array */

/*
** We presume all tests will be run unless told
** otherwise
*/
for(i=0;i<NUMTESTS;i++)
        tests_to_do[i]=1;

/*
** Initialize test data structures to default
** values.
*/
set_request_secs();     /* Set all request_secs fields */
global_numsortstruct.adjust=0;
global_numsortstruct.arraysize=NUMARRAYSIZE;

global_strsortstruct.adjust=0;
global_strsortstruct.arraysize=STRINGARRAYSIZE;

global_bitopstruct.adjust=0;
global_bitopstruct.bitfieldarraysize=BITFARRAYSIZE;

global_emfloatstruct.adjust=0;
global_emfloatstruct.arraysize=EMFARRAYSIZE;

global_fourierstruct.adjust=0;

global_assignstruct.adjust=0;

global_ideastruct.adjust=0;
global_ideastruct.arraysize=IDEAARRAYSIZE;

global_huffstruct.adjust=0;
global_huffstruct.arraysize=HUFFARRAYSIZE;

global_nnetstruct.adjust=0;

global_lustruct.adjust=0;

#ifdef LINUX
output_string("\nBYTEmark* Native Mode Benchmark ver. 2 (10/95)\n");
output_string("Index-split by Andrew D. Balsa (11/97)\n");
output_string("Linux/Unix* port by Uwe F. Mayer (12/96,11/97)\n");
#endif

/*
** Execute the tests.
*/
#ifdef LINUX
output_string("\nTEST                : Iterations/sec.  : Old Index   : New Index\n");
output_string("                    :                  : Pentium 90* : AMD K6/233*\n");
output_string("--------------------:------------------:-------------:------------\n");
#endif

for(i=0;i<NUMTESTS;i++)
{
        if(tests_to_do[i])
        {       sprintf(buffer,"%s    :",ftestnames[i]);
                                output_string(buffer);
                if (0!=bench_with_confidence(i,
                        &bmean,
                        &bstdev,
                        &bnumrun)){
		  output_string("\n** WARNING: The current test result is NOT 95 % statistically certain.\n");
		  output_string("** WARNING: The variation among the individual results is too large.\n");
		  output_string("                    :");
		}
		/*
		** Gather integer or FP indexes
		*/
		if((i==4)||(i==8)||(i==9)){
		  /* FP index */
		  fpindex=fpindex*(bmean/bindex[i]);
		  /* Linux FP index */
		  lx_fpindex=lx_fpindex*(bmean/lx_bindex[i]);
		}
		else{
		  /* Integer index */
		  intindex=intindex*(bmean/bindex[i]);
		  if((i==0)||(i==3)||(i==6)||(i==7))
		    /* Linux integer index */
		    lx_intindex=lx_intindex*(bmean/lx_bindex[i]);
		  else
		    /* Linux memory index */
		    lx_memindex=lx_memindex*(bmean/lx_bindex[i]);
		}
        }
}
/* printf("...done...\n"); */

/*
** Output the total indexes
*/
if(global_custrun==0)
{
        output_string("==========================ORIGINAL BYTEMARK RESULTS==========================\n");
        sprintf(buffer,"INTEGER INDEX       : %.3f\n",
                       pow(intindex,(double).142857));
        output_string(buffer);
        sprintf(buffer,"FLOATING-POINT INDEX: %.3f\n",
                        pow(fpindex,(double).33333));
        output_string(buffer);
        output_string("Baseline (MSDOS*)   : Pentium* 90, 256 KB L2-cache, Watcom* compiler 10.0\n");
#ifdef LINUX
        output_string("==============================LINUX DATA BELOW===============================\n");
        sprintf(buffer,"MEMORY INDEX        : %.3f\n",
                       pow(lx_memindex,(double).3333333333));
        output_string(buffer);
        sprintf(buffer,"INTEGER INDEX       : %.3f\n",
                       pow(lx_intindex,(double).25));
        output_string(buffer);
        sprintf(buffer,"FLOATING-POINT INDEX: %.3f\n",
                        pow(lx_fpindex,(double).3333333333));
        output_string(buffer);
        output_string("Baseline (LINUX)    : AMD K6/233*, 512 KB L2-cache, gcc 2.7.2.3, libc-5.4.38\n");
#endif
output_string("* Trademarks are property of their respective holder.\n");
}
enclave_exit();
}

/*********************
** set_request_secs **
**********************
** Set everyone's "request_secs" entry to whatever
** value is in global_min_secs.  This is done
** at the beginning, and possibly later if the
** user redefines global_min_secs in the command file.
*/
static void set_request_secs(void)
{

global_numsortstruct.request_secs=global_min_seconds;
global_strsortstruct.request_secs=global_min_seconds;
global_bitopstruct.request_secs=global_min_seconds;
global_emfloatstruct.request_secs=global_min_seconds;
global_fourierstruct.request_secs=global_min_seconds;
global_assignstruct.request_secs=global_min_seconds;
global_ideastruct.request_secs=global_min_seconds;
global_huffstruct.request_secs=global_min_seconds;
global_nnetstruct.request_secs=global_min_seconds;
global_lustruct.request_secs=global_min_seconds;

return;
}


/**************************
** bench_with_confidence **
***************************
** Given a benchmark id that indicates a function, this routine
** repeatedly calls that benchmark, seeking to collect and replace
** scores to get 5 that meet the confidence criteria.
**
** The above is mathematically questionable, as the statistical theory
** depends on independent observations, and if we exchange data points
** depending on what we already have then this certainly violates
** independence of the observations. Hence I changed this so that at
** most 30 observations are done, but none are deleted as we go
** along. We simply do more runs and hope to get a big enough sample
** size so that things stabilize. Uwe F. Mayer
**
** Return 0 if ok, -1 if failure.  Returns mean
** and std. deviation of results if successful.
*/
static int bench_with_confidence(int fid,       /* Function id */
        double *mean,                   /* Mean of scores */
        double *stdev,                  /* Standard deviation */
        ulong *numtries)                /* # of attempts */
{
double myscores[30];            /* Need at least 5 scores, use at most 30 */
double c_half_interval;         /* Confidence half interval */
int i;                          /* Index */
/* double newscore; */          /* For improving confidence interval */

/*
** Get first 5 scores.  Then begin confidence testing.
*/
for (i=0;i<5;i++)
{       (*funcpointer[fid])();
        myscores[i]=getscore(fid);
#ifdef DEBUG
	printf("score # %d = %g\n", i, myscores[i]);
#endif
}
*numtries=5;            /* Show 5 attempts */

/*
** The system allows a maximum of 30 tries before it gives
** up.  Since we've done 5 already, we'll allow 25 more.
*/

/*
** Enter loop to test for confidence criteria.
*/
while(1)
{
        /*
        ** Calculate confidence. Should always return 0.
        */
        if (0!=calc_confidence(myscores,
		*numtries,
                &c_half_interval,
                mean,
                stdev)) return(-1);

        /*
        ** Is the length of the half interval 5% or less of mean?
        ** If so, we can go home.  Otherwise, we have to continue.
        */
        if(c_half_interval/ (*mean) <= (double)0.05)
                break;

#ifdef OLDCODE
#undef OLDCODE
#endif
#ifdef OLDCODE
/* this code is no longer valid, we now do not replace but add new scores */
/* Uwe F. Mayer */
	      /*
	      ** Go get a new score and see if it
	      ** improves existing scores.
	      */
	      do {
		      if(*numtries==10)
			      return(-1);
		      (*funcpointer[fid])();
		      *numtries+=1;
		      newscore=getscore(fid);
	      } while(seek_confidence(myscores,&newscore,
		      &c_half_interval,mean,stdev)==0);
#endif
	/* We now simply add a new test run and hope that the runs
           finally stabilize, Uwe F. Mayer */
	if(*numtries==30) return(-1);
	(*funcpointer[fid])();
	myscores[*numtries]=getscore(fid);
#ifdef DEBUG
	printf("score # %ld = %g\n", *numtries, myscores[*numtries]);
#endif
	*numtries+=1;
}

return(0);
}

#ifdef OLDCODE
/* this procecdure is no longer needed, Uwe F. Mayer */
  /********************
  ** seek_confidence **
  *********************
  ** Pass this routine an array of 5 scores PLUS a new score.
  ** This routine tries the new score in place of each of
  ** the other five scores to determine if the new score,
  ** when replacing one of the others, improves the confidence
  ** half-interval.
  ** Return 0 if failure.  Original 5 scores unchanged.
  ** Return -1 if success.  Also returns new half-interval,
  ** mean, and standard deviation of the sample.
  */
  static int seek_confidence( double scores[5],
  		double *newscore,
  		double *c_half_interval,
  		double *smean,
  		double *sdev)
  {
  double sdev_to_beat;    /* Original sdev to be beaten */
  double temp;            /* For doing a swap */
  int is_beaten;          /* Indicates original was beaten */
  int i;                  /* Index */

  /*
  ** First calculate original standard deviation
  */
  calc_confidence(scores,c_half_interval,smean,sdev);
  sdev_to_beat=*sdev;
  is_beaten=-1;

  /*
  ** Try to beat original score.  We'll come out of this
  ** loop with a flag.
  */
  for(i=0;i<5;i++)
  {
  	temp=scores[i];
  	scores[i]=*newscore;
  	calc_confidence(scores,c_half_interval,smean,sdev);
  	scores[i]=temp;
  	if(sdev_to_beat>*sdev)
  	{       is_beaten=i;
  		sdev_to_beat=*sdev;
  	}
  }

  if(is_beaten!=-1)
  {       scores[is_beaten]=*newscore;
  	return(-1);
  }
  return(0);
  }
#endif

/********************
** calc_confidence **
*********************
** Given a set of numtries scores, calculate the confidence
** half-interval.  We'll also return the sample mean and sample
** standard deviation.
** NOTE: This routines presumes a confidence of 95% and
** a confidence coefficient of .95
** returns 0 if there is an error, otherwise -1
*/
static int calc_confidence(double scores[], /* Array of scores */
		int num_scores,             /* number of scores in array */
                double *c_half_interval,    /* Confidence half-int */
                double *smean,              /* Standard mean */
                double *sdev)               /* Sample stand dev */
{
/* Here is a list of the student-t distribution up to 29 degrees of
   freedom. The value at 0 is bogus, as there is no value for zero
   degrees of freedom. */
double student_t[30]={0.0 , 12.706 , 4.303 , 3.182 , 2.776 , 2.571 ,
                             2.447 , 2.365 , 2.306 , 2.262 , 2.228 ,
                             2.201 , 2.179 , 2.160 , 2.145 , 2.131 ,
                             2.120 , 2.110 , 2.101 , 2.093 , 2.086 ,
                             2.080 , 2.074 , 2.069 , 2.064 , 2.060 ,
		             2.056 , 2.052 , 2.048 , 2.045 };
int i;          /* Index */
if ((num_scores<2) || (num_scores>30)) {
  output_string("Internal error: calc_confidence called with an illegal number of scores\n");
  return(-1);
}
/*
** First calculate mean.
*/
*smean=(double)0.0;
for(i=0;i<num_scores;i++){
  *smean+=scores[i];
}
*smean/=(double)num_scores;

/* Get standard deviation */
*sdev=(double)0.0;
for(i=0;i<num_scores;i++) {
  *sdev+=(scores[i]-(*smean))*(scores[i]-(*smean));
}
*sdev/=(double)(num_scores-1);
*sdev=sqrt(*sdev);

/* Now calculate the length of the confidence half-interval.  For a
** confidence level of 95% our confidence coefficient gives us a
** multiplying factor of the upper .025 quartile of a t distribution
** with num_scores-1 degrees of freedom, and dividing by sqrt(number of
** observations). See any introduction to statistics.
*/
*c_half_interval=student_t[num_scores-1] * (*sdev) / sqrt((double)num_scores);
return(0);
}

/*************
** getscore **
**************
** Return the score for a particular benchmark.
*/
static double getscore(int fid)
{

/*
** Fid tells us the function.  This is really a matter of
** doing the proper coercion.
*/
switch(fid)
{
        case TF_NUMSORT:
                return(global_numsortstruct.sortspersec);
        case TF_SSORT:
                return(global_strsortstruct.sortspersec);
        case TF_BITOP:
                return(global_bitopstruct.bitopspersec);
        case TF_FPEMU:
                return(global_emfloatstruct.emflops);
        case TF_FFPU:
                return(global_fourierstruct.fflops);
        case TF_ASSIGN:
                return(global_assignstruct.iterspersec);
        case TF_IDEA:
                return(global_ideastruct.iterspersec);
        case TF_HUFF:
                return(global_huffstruct.iterspersec);
        case TF_NNET:
                return(global_nnetstruct.iterspersec);
        case TF_LU:
                return(global_lustruct.iterspersec);
}
return((double)0.0);
}

/******************
** output_string **
*******************
** Displays a string on the screen.  Also, if the flag
** write_to_file is set, outputs the string to the output file.
** Note, this routine presumes that you've included a carriage
** return at the end of the buffer.
*/
static void output_string(char *buffer)
{
    puts(buffer);
    return;
}
