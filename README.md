topor - simple scripting language
===
Dynamic typed, simple scripting engine that should only depend on freestanding
libraries (namely, c41 - a general, common purpose library).
The aim is for this engine to be compilable for all platforms (user & kernel 
mode).


Licence: ISC
* Internet Systems Consortium licence
* http://en.wikipedia.org/wiki/ISC_license
* http://opensource.org/licenses/ISC
* https://www.isc.org/wordpress/software/software-support-policy/isc-license/
* this is the preffered licence of OpenBSD

Syntax

The syntax for the scripting language is C-like with obvious changes to account
for loose-typed vars. Something like this:

    module "com.example.project.mod"; # name of this module
    stdio = import("topor.app.stdio"); # stdio is a global of this module of type module

    CONSTIBLE = 1 # just some global; all caps is a convention for constants
    fn set_data (o, v)
    {
      o.data = v;
    }
    fn sum (a, b)
    {
      return a.class.new(a.data + b.data);
    }
    fn astr (a)
    {
      return a.data.to_str();
    }
    A = class.new('Alpha', object);
    A.define_method('_init', set_data); # choose the constructor
    a = A.new(5);
    A.define_method('+', sum); # define the + operator
    b = A.new(4);
    c = a + b;
    A.define_method('to_str", astr);
    stdio.out.print(" 5 + 4 =#{c} ".strip()); # ruby-ish style
    stdio.out.print("again: " + c);
    


  
