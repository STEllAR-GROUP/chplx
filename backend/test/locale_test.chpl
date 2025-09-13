writeln("This program is running on ", numLocales, " locales");

var thisLocale = here.id;
writeln("This locale ID is: ", thisLocale);


var thisLocale2 = here.id;
writeln("This locale ID is: ", thisLocale2);

var thisLocale3 = here.id;

var x: int = here.id;

var b: int;
b = here.id;
writeln("This locale ID is: ", b);

writeln("This locale ID is: ", x);

var MyLocaleArray: [0..numLocales] locale =
      for i in 0..numLocales do Locales[(i)%numLocales];

forall loc in Locales {
   writeln("This locale ID is: ", here.id);
}

for loc in Locales {
   writeln("This locale ID is: ", here.id);
}

coforall loc in Locales {
   writeln("This locale ID is: ", here.id);
}

coforall loc in Locales {
   writeln("This locale ID is: ", loc.id);
}

forall loc in Locales {
   writeln("This locale ID is: ", loc.id);
}

for loc in Locales {
   writeln("This locale ID is: ", loc.id);
}


for loc in Locales do
  on loc do
    writeln("hello locale ", loc.id);

for loc in Locales do
  on loc {
    writeln("hello locale ", loc.id);
  }

coforall loc in Locales { 
   on loc {
      writeln("hello locale ", loc.id); 
   } 
}

on b {
   writeln("This locale ID is: ", here.id);
}

on b do
   writeln("This locale ID is: ", here.id, " b is ", b);


var x1: int = Locales[1 % numLocales];
var z1: int = 4;
var loc1 : int = Locales[0];
on x1 {
    var y1 = 3;
    writeln("From locale ", here.id, ", x is: ", x1.id, " and y is: ", y1);
    
    on loc1 {
      writeln("From locale 0, x is: ", x1.id, " and y is: ", y1);
    }
}
writeln();

