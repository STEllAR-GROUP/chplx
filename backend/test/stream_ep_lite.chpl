
const numVectors = 3;


config const m = 100000;
config const alpha = 3.0;

//
// Configuration constants to set the number of trials to run and the
// amount of error to permit in the verification
//
config const numTrials = 10;
config const epsilon = 1e-6;

config const printParams = true;
config const printStats = true;

config const seed = 1234;

//
// The program entry point
//
proc chapel_main() {
  printConfiguration();   // print the problem size, number of trials, etc.    

  // var minTimes: [1..numLocales] real;
  // var validAnswers: [1..numLocales] bool;

  //
  // *** Fragment control so that we have a single task running on
  // *** every locale.
  //
  coforall loc in Locales do on loc {

    var execTime: [1..numTrials] real;

    //
    // *** A, B, and C are the three local vectors
    //
    var A: [1..m] real;
    var B: [1..m] real;
    var C: [1..m] real;


    // Randomly initialize the vectors A and B

    const Acoef = 1664525;
    const Ccoef = 1013904223;
    const M     = 1 << 31;      // 2^31
    const D     = M - 1;        // for scaling to [0,1]

    var s : int;
    s = seed;

    for idx in 0..m {
        // advance state, fill A
        s = (Acoef*s + Ccoef) % M;
        A[idx] = s / D ;

        // advance state, fill B
        s = (Acoef*s + Ccoef) % M;
        B[idx] = s / D ;
    }

    for trial in 1..numTrials {                        // loop over the trials

      //
      // *** The main loop looks identical to stream.chpl.  However,
      // *** in this version we are iterating over arrays that are
      // *** not distributed.
      //
      forall (a, b, c) in zip(A, B, C) do
        a = b + alpha * c;

    }

    var _here_id = here.id;

    // minTimes[_here_id] = 0.0;
    // validAnswers[_here_id] = true;

    if (printStats) {
      writeln("Execution done on locale ", here.id);
      // writeln("Min Time: ", minTimes[_here_id]);
      // writeln("Valid: ", validAnswers[_here_id]);
    }

 }

}

//
// Print the problem size and number of trials
//
proc printConfiguration() {
  if (printParams) {
    //
    // *** Here we multiply m by the number of locales so that we can
    // *** print out the global problem size.
    //
    writeln("Number of trials = ", numTrials);
    writeln("m = ", m);
    writeln("seed = ", seed);
  }
}

chapel_main();