proc getNextRandom(x : int) : int {
   var poly = 0;
   inlinecxx("{} = 0x7;", poly);
   var hirandbit = 0;
   inlinecxx("{} = 0x1ULL << (64-1);", hirandbit);
   inlinecxx("{} = ({} << 1) ^ ( ({} & {}) ? {} : 0);", x, x, x, hirandbit, poly);
   return x;
}

proc computeProblemSize(numArrays : int, physicalMemoryBytes : int, memRatio : int, returnLog2 : bool) : int {
    var totalMem = physicalMemoryBytes;
    var memoryTarget = totalMem / memRatio;
    var numBytesPerType = 0;
    inlinecxx("{} = sizeof(std::int64_t);", numBytesPerType);
    var bytesPerIndex = numArrays * numBytesPerType;
    var numIndices = memoryTarget / bytesPerIndex;

    var lgProblemSize = 0;
    inlinecxx("{} = std::log2(numIndices);", lgProblemSize);

      if (returnLog2 == false) {
      //numIndices = 2**lgProblemSize;
      inlinecxx("{} = std::pow(2,{});", numIndices, lgProblemSize);
      inlinecxx("if({} * {} <= {})", numIndices, bytesPerIndex, memoryTarget);
        inlinecxx("{} *= 2;", numIndices);
      inlinecxx("if({} * {} <= {})", numIndices, bytesPerIndex, memoryTarget);
        inlinecxx("{} += 1;", lgProblemSize);
    }

   var retval : int = 0;
   inlinecxx("{} = {} ? {} : {};", retval, returnLog2, lgProblemSize, numIndices);
   return retval;
}


proc computeM2Values(m2 : [] int, count : int) :bool {
   var nextval = 0;
   inlinecxx("{} = 0x1;", nextval);
   var count_range : int;
   count_range = count - 1;
   for i in 0..count_range {
      m2[i] = nextval;
      nextval = getNextRandom(nextval);
      nextval = getNextRandom(nextval);
   }

   return true;
}

proc getNthRandom(N : int, m2 : [] int, m2count : int) {
   var ran = 0;
   inlinecxx("{} = 0x2;", ran);
   var i = 0;
   inlinecxx("{} = std::ceil(std::log2(static_cast<double>(N)));", i);
   var val = 0;
   var J = 0;
   var I = i-1;
   for j in 0..I {
      J = i - j - 1;
      for k in 0..<m2count {
         // if ((ran >> k) & 1) then val ^= m2[k];
         inlinecxx("if ((({} >> {}) & 1) != 0) {} ^= {} [ {} ];", ran, k, val, m2, k);
      }
      ran = val;
      // if ((N >> J) & 1) then ran = getNextRandom(ran);
      inlinecxx("if ((({} >> {}) & 1) != 0) {} = getNextRandom({});", N, J, ran, ran);
   }

   return ran;
}

proc RAStream(vals : [] int, numvals : int, m2 : [] int, m2count : int) {

   var val = getNthRandom(2, m2, m2count);
   var NUMvals = numvals - 1;
   for i in 0..NUMvals {
      val = getNextRandom(val);
      vals[i] = val;
   }
}

param randWidth = 64;
config var physicalMemory = 134217728;
config var memRatio = 4;
var memoryTarget_ = physicalMemory / memRatio;
var bytesPerType = 4;
inlinecxx("{} = sizeof(decltype({}));",bytesPerType, memoryTarget_);
param numTables = 1;
var m2range : int;
m2range = randWidth - 1;
var m2 : [0..m2range] int;
computeM2Values(m2, randWidth);

// Compute problem sizes
var n = 0;
n = computeProblemSize(numTables, physicalMemory, memRatio, true);
var N_U = 0;
inlinecxx("{} = static_cast<int>(std::pow(2, (n + 2)));", N_U);
var N_U_idx : int;
N_U_idx= N_U - 1;
var SZ = 0; // Size of array
inlinecxx("{} = static_cast<int>(std::pow(2, (n)));", SZ);


var randval : [0..N_U_idx] int;

var indexMask : int;
indexMask = SZ - 1;
var T : [0..indexMask] int;

for i in 0..indexMask {
  T[i] = i;
}

inlinecxx("hpx::chrono::high_resolution_timer gups;");

RAStream(randval, N_U, m2, randWidth);
forall r in 0..N_U_idx {
   inlinecxx("{} [ {} [ {} ] & {} ] ^= {} [ {} ];", T, randval, r, indexMask, randval, r);
}

// Measure time and compute performance
inlinecxx("auto elapsed_sec = gups.elapsed();");
inlinecxx("auto t0 = elapsed_sec;");
inlinecxx("auto nupdates = {};", N_U);
inlinecxx("auto gups_val = ((double)nupdates) / t0;");
inlinecxx("std::cout << hpx::resource::get_num_threads() << \",\" << t0 << \",\" << gups_val << \",\" << physicalMemory << \",\" << memRatio << \",\" << n << std::endl;");
