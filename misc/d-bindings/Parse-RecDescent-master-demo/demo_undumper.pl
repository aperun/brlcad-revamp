#!/usr/bin/perl -w
use strict;
$|++;

# $::RD_TRACE = 80;
$::RD_HINT = 1;

use Parse::RecDescent;
use Data::Dumper;

## define grammar

my $parser = Parse::RecDescent->new(q{

{ my %TABLE; }

file: { %TABLE = (); } assignment(s?) /\z/ { \%TABLE }

assignment: scalar_assignment | <error>

scalar_assignment: scalar_lvalue '=' scalar_rvalue ';'
  { ${$item{scalar_lvalue}} = ${$item{scalar_rvalue}}; 1; }

## lvalues, indicated as reference to value, so we can assign to them

scalar_lvalue: deref_head deref_chain(?) {
  my $return = $item{deref_head};
  if ($item{deref_chain}) {
    for (@{$item{deref_chain}[0]}) {
      if ($_->[0] eq "hash") {
        $return = \$$return->{${$_->[1]}};
      } elsif ($_->[0] eq "array") {
        $return = \$$return->[${$_->[1]}];
      } else { die "what is $_->[0]?" }
    }
  }
  $return;
}
deref_head: simple_scalar_lvalue | simple_scalar_rvalue
deref_chain: "->" hash_or_array_subscript deref_chain_more(s?)
  { [$item[2], @{$item[3]}] }
deref_chain_more: "->" hash_or_array_subscript | hash_or_array_subscript
hash_or_array_subscript: hash_subscript | array_subscript
hash_subscript: "{" scalar_constant "}"
  { ["hash", $item{scalar_constant}] }
array_subscript: "[" scalar_constant "]"
  { ["array", $item{scalar_constant}] }

simple_scalar_lvalue: '$' ident
  { \ $TABLE{'$' . $item{ident}} }
ident: /[^\W\d]\w*/

simple_scalar_lvalue: '$' '{' scalar_rvalue '}'
  { \ ${${$item{scalar_rvalue}}} }

## rvalues, indicated as reference to value, because "undef" is legal

scalar_rvalue: simple_scalar_rvalue | scalar_lvalue

simple_scalar_rvalue: scalar_constant
scalar_constant: 'undef'
  { \ undef }
scalar_constant: /-?[1-9]\d*|0/
  { \ $item[1] }
scalar_constant: <perl_quotelike>
  { \ $item[1][2] }

simple_scalar_rvalue: "\x5C" scalar_rvalue
  { \ $item{scalar_rvalue} }
simple_scalar_rvalue: '[' scalar_rvalue(s? /,/) ']'
  { \ [map $$_, @{$item[2]}] }
simple_scalar_rvalue: '{' hashpair(s? /,/) '}'
  { \ {map @$_, @{$item[2]}} }
hashpair: scalar_constant '=>' scalar_rvalue
  { [${$item{scalar_constant}}, ${$item{scalar_rvalue}}] }
simple_scalar_rvalue: 'bless' '(' scalar_rvalue ',' scalar_constant ')'
  { \ bless( ${$item{scalar_rvalue}}, ${$item{scalar_constant}} ) }
simple_scalar_rvalue: 'do' '{' "\x5C" '(' 'my' '$o' '=' scalar_rvalue ')' '}'
  { \ do { \ (my $o = ${$item{scalar_rvalue}})} }

}) or die "compile";

## following tests from t/dumper.t in 5.6.1 distribution

if (0) {
  my @c = ('c');
  my $c = \@c;
  my $b = {};
  my $a = [1, $b, $c];
  $b->{a} = $a;
  $b->{b} = $a->[1];
  $b->{c} = $a->[2];

  test([$a, $b, $c], [qw(a b c)]);
}

if (0) {
  my $foo = { "abc\000\'\efg" => "mno\000",
              "reftest" => \\1,
            };

  test([$foo], [qw($foo)]);
}

if (0) {
  my $foo = 5;
  my @foo = (-10,\$foo);
  my %foo = (a=>1,b=>\$foo,c=>\@foo);
  $foo{d} = \%foo;
  $foo[2] = \%foo;

  test([\%foo],[qw($foo)]);
}

if (0) {
  my @dogs = ( 'Fido', 'Wags' );
  my %kennel = (
            First => \$dogs[0],
            Second =>  \$dogs[1],
           );
  $dogs[2] = \%kennel;
  my $mutts = \%kennel;
  test([\@dogs, \%kennel, $mutts], [qw($dogs $kennel $mutts)]);
}

if (0) {
  my $a = [];
  $a->[1] = \$a->[0];
  test([$a], [qw($a)]);
}

if (0) {
  my $a = \\\\\'foo';
  my $b = $$$a;
  test([$a, $b], [qw($a $b)]);
}

if (1) {
  my $b;
  my $a = [{ a => \$b }, { b => undef }];
  $b = [{ c => \$b }, { d => \$a }];
  timetest([$a, $b], [qw($a $b)]);
}

if (0) {
  my $a = [[[[\\\\\'foo']]]];
  my $b = $a->[0][0];
  my $c = $${$b->[0][0]};
  test([$a, $b, $c], [qw($a $b $c)]);
}

if (0) {
  my $f = "pearl";
  my $e = [        $f ];
  my $d = { 'e' => $e };
  my $c = [        $d ];
  my $b = { 'c' => $c };
  my $a = { 'b' => $b };
  test([$a, $b, $c, $d, $e, $f], [qw($a $b $c $d $e $f)]);
}

if (0) {
  my $a;
  $a = \$a;
  my $b = [$a];
  test([$b], [qw($b)]);
}

## end of tests from t/dumper.t, now some of my own

if (0) {
  my $x = bless {fred => 'flintstone'}, 'x';
  my $y = bless \$x, 'y';
  timetest([$x, $y], [qw($x $y)]);
}

sub test {
  my $input = Data::Dumper->new(@_)->Purity(1)->Dumpxs;
  print "=" x 60, "\ninput:\n$input\n==>\noutput:\n";
  my $symbol = $parser->file($input) or die "execute";
  print Data::Dumper->new([values %$symbol], [keys %$symbol])->Purity(1)->Dumpxs;
}

sub timetest {
  require Benchmark;

  my $input = Data::Dumper->new(@_)->Purity(1)->Dumpxs;
  print "=" x 60, "\ninput:\n$input\n==>\noutput:\n";
  Benchmark::timethese(0, {
                           PRD => sub {
                             package Dummy;
                             no strict;
                             my $symbol = $parser->file($input)
                               or die "execute";
                           },
                           EVAL => sub {
                             package Dummy;
                             no strict;
                             eval $input;
                           },
                          });
}

=head1 Linux Magazine Column 29 (Oct 2001)

[suggested title: Safe Undumping]

Recently on the Perl Monestary at C<http://www.perlmonks.org/>, the
user known as C<ton> asked about parsing a Perl-style double-quoted
string, as part of a project to construct a safe C<Data::Dumper>
parser that would take the output and interpret it rather than handing
the result directly to C<eval> as is typically done.  A few postings
later, the work in progress for their C<Undumper> was posted, and I commented
that there was probably a simpler way to do some of the things,
and that it didn't handle blessed references.

Well, me and my big mouth.  Or was it my continuing curiousity to
tinker with Damian Conway's excellent C<Parse::RecDescent> module? I'm
not sure, but I found myself over the next dozen hours or so staring
at C<Data::Dumper> source code, output, test cases, and
C<Parse::RecDescent> traces and documentation.  I also pounded my head
on the desk for better than a day trying to figure out how to break a
left-recursion loop, and came up with a nice obvious (now!) solution.

The point of this is to be able to take the output of C<Data::Dumper>
and reconstruct the original data, but not open ourselves to the
possibility of being fed dangerous constructs, like backticks or
symbol-table-manipulating code.  Sure, you could also do this with the
right use of the C<Safe> module, but I was already committed to
finishing this version before I thought of that.  Maybe another column
someday.

But anyway, that brings us to the program in [listing one, below].
Most of this program is input to C<Parse::RecDescent>, so I'll start
by setting that aside and describing the Perl support structure first.

Lines 1 through 3 start nearly every program I write, turning on
warnings, compiler restrictions, and disabling the buffering on
C<STDOUT>.

Lines 5 and 6 control the behaviour of C<Parse::RecDescent>. When
tracing is enabled (although commented out here), I can see how the
parse is progressing. The number 80 trims the "to-be-parsed" string
dumping to its first 80 characters, which is usually plenty for me to
see what's going on. I left hints enabled, because the error messages
that result from a hint were often helpful in debugging the grammar.

Lines 8 and 9 pull in the C<Parse::RecDescent> (found in the CPAN) and
C<Data::Dumper> (part of the core Perl distribution) modules.

Lines 13 to 81 form the input grammar for C<Parse::RecDescent>,
included as a single-quoted string using C<q{}> for delimiters. Damian
Conway seems to prefer this quoting style for sending stuff to
C<Parse::RecDescent>, so I've copied it at least here. In other
places, I might have used a "here-document" instead. The result of
feeding this grammar to C<Parse::RecDescent> is a parser object that
we can then call on to parse the output of C<Data::Dumper> to "undump"
it. If the grammar is bad, we die in line 81. At that point
C<Parse::RecDescent> has already printed its own diagnostics, expanded
by the enabled "hint" flag earlier.

During development, I probably changed this grammar over 50 times,
constantly tweaking it to recognize each new thing that I encountered
in the input source. The hints were often useful, although the "left
recursion" problem (that I'll describe later) had me stumped for the
better part of two days.

Once we have a parser object (in $parser), the action to "undump" a
string containing Perl code is simple.  Skipping down quickly to the
C<test> subroutine starting at line 177, we can see the use of this
C<$parser> to take a given C<$input> and transform it to a hashref
response (saved as C<$symbol> here). The keys of the hashref contain
the top-level scalar variable names, which default to C<$VAR1>,
C<$VAR2> and so on, unless overridden during the dumping. The values
of this hashref are then the corresponding values of those variables
from the original dump, as best can be reconstructed.

The C<test> subroutine here uses C<Data::Dumper> to convert an
existing collection of values (passed in as arguments) into a string,
shows the string as the input to the undumper, then parses the string
with the constructed parser object. The output of this undumping
process is then dumped once again and shown. The comparison process
has to be a manual one, because it is sensitive to the order in which
the items are presented to C<Data::Dumper>.

Above the definition of this testing routine is a series of tests that
I used to develop and verify the undumper.  I started with the
included tests found in the Perl 5.6.1 distribution to verify Perl on
initial installation.  I actually didn't include the entire test, but
took the data structure for each applicable test and copied that into
this program.  I skipped over the tests that included symbol-table
references (using globs) or coderefs, because I chose to ignore that
for this program.  Each test provides an arrayref containing the
references to be dumped, and most tests also provide a label list to
keep the variables from simply being named C<$VAR1>, C<$VAR2> and so
on.

I enclosed each test in a conditional that I could switch between "if
(0)" and "if (1)" to either run the test or skip it.  When I was
focussing on a new feature, I enabled only one test until it worked,
but once I got a good run, I reenabled all previous tests to ensure
that I didn't break something else in the process.

The "Extreme Programming" technique relies on this strategy of
creating tests first and driving the development from those tests, and
it worked nicely for the development of this program.  Other than a
few snippets of code that I used for scaffolding to test some initial
grammar ideas, all of the code that I used to develop and test this
code is thereby archived along with the routines, for later
maintenance to take place easily.

Any one of the tests can be switched to invoke C<timetest> rather than
C<test>, as I've done on the final test (starting in line 171).  This
final test, by the way, exercises blessed references (objects), which
none of the distribution tests even approach.  (Maybe a patch to the
distribution's C<t/dumper.t> is in order?)  Using C<timetest>, we pop
down to the routine beginning in line 184, which runs a quick
benchmark comparing a straight C<eval> with our safe undumping parser.
I'm using the C<timethese> routine from C<Benchmark> (a standard Perl
module), giving it the default "0 times", which attempts to run each
routine for slighly over 3 CPU seconds to compute an interations per
second for each type.

The bad news is that my code clocks in at 1/100th of the speed of
C<eval>.  Ahh, the price to pay for safety.  The good news is that the
speed difference is probably negligible for typical applications, and
pennies to pay for the dollars saved in having a safe evaluation of
what should be good C<Data::Dumper> output.

Well, what little Perl code there is (outside of the grammar), that's
it.  So let's pick apart the grammar now, starting back up at line 13.

Line 15 declares a variable local to the parser for the "symbol table"
we'll be creating.  The keys will be the full variable names as given
in the dump. The values will be the constructed value for that
key. Yes, this is the hash that is returned upon a successful parse.

Line 17 defines the top-level rule for the parse.  A "file" is zero or
more "assignments".  Howver, we also have to clear out the possibly
leftover values from the previous run, and this is handled by the
action block immediately preceding the assignment. We also have to
verify that we've parsed the entire input string, which I'll do by
matching C</\z/>, which matches only at the end of the string. If all
went well, we'll return the hashref.

Line 19 defines an assignment.  Currently, I'm handling only a scalar
assignment, although C<Data::Dumper> can also generate array and hash
assignments under some circumstances.  Gotta leave something for
"release 2.0".

Lines 21 and 22 define a scalar assignment, and the action to take for
that.  A scalar assignment consists of an "lvalue" (like a variable)
and an "rvalue" (like an expression).  Both lvalues and rvalues are
references, so we dereference each of them in the action, and perform
the requested assignment to keep our "virtual Perl symbol table" up to
date.  The action returns a constant 1 to let the parent rule know
that this rule succeeded.  Without that, an C<undef> value being
assigned would have caused that rule to fail (one of my debugging
attempts revealed such, with much head-banging-on-desk until I
realized what I was doing).

Lvalues are defined starting in line 24.  The most complex lvalue is a
dereferenced scalar reference, handled in lines 26 to 47.  This is
because C<Data::Dumper> can generate a reference path to a specific
entry in the data structure to patch it up so that it points to a
proper place for complex data interelationships.  For example, if an
element of C<@a> points to C<@b>, but an element of C<@b> points to
C<@a>, the "chicken-and-egg" tie is broken by first generating C<@a>
without the C<@b> reference, then C<@b> with the C<@a> reference, then
patching up C<@a> finally to point to C<@b>.

The definition starting in line 26 uses a C<deref_head> to be a simple
scalar, followed by an optional C<deref_chain> which is one or more
array-element or hash element dereferences.  Let's look at the syntax
before we study the actions taken.  Line 39 shows us that
C<deref_head> is either a C<simple_scalar_lvalue> or
C<simple_scalar_rvalue>.

Further down in lines 49 and 53, we see that a C<simple_scalar_lvalue>
can be either a scalar variable or a scalar reference dereference.
And that would form the head of the chain.  If it's a scalar variable,
we take a reference to a hash element in our "symbol table", and
return that.  The key is the name of the variable, including the
dollar sign.  This permits eventual expansion to include arrays and
hashes in our "symbol table", again for version 2.0.  For the
dereference of the scalar reference, we emulate the same steps in our
virtual world, again returning a reference for all lvalues.

Once we have the chain head, we then look for a dereference chain
(line 40), starting with the mandatory arrow then a hash or array
subscript expression.  This may be followed with additional
arrow-optional hash or array subscript expressions (line 42).  Each
subscript is returned as a two-element arrayref, with either the
keyword "hash" or "array" followed by the scalar constant selecting
the particular element (lines 45 and 47).  The action in line 41 rolls
up the deref chain as a reference to an array of those two-element
arrayrefs.

Back in line 26, we thus have a reference to a starting point for the
C<deref_head>, and possible one or more dereferences to apply to it.
The action in lines 27 to 37 walk the dereference chain to get to the
final target lvalue, and return it (as a reference to a scalar
somewhere in our virtual Perl symbol space).

On the other side of the equation, we've got rvalues of various
shapes, defined in line 58 to be either a C<simple_scalar_rvalue> or a
C<scalar_lvalue>.  The loop back to C<scalar_value> is necessary
because any variable reference is also itself a source of values.

Lines 60 to 66 define the only scalar constants that C<Data::Dumper>
seems to emit: C<undef>, simple signed integers, and quoted strings.
These values are returned as references to those values so that
C<undef> can be a valid return value: again, something I figured out
the hard way after my rule for C<undef> kept breaking.

Lines 68 to 75 handle the reference values: scalar references, array
references, and hash references.  Again, the syntax is very narrow:
just the types of things I was able to see C<Data::Dumper> generate,
both by trying some sample data, and by lightly examining the source
code to C<Data::Dumper>.  I may have missed a form or two, in which
case this grammar will make it easy to add additional forms.

The grammar is nicely recursive: an array reference in turn contains
one or more C<scalar_rvalue> items, which brings us right back to the
same place in the hierarchy, but at a nested level.

Lines 76 to 79 handle blessed references. Again, the syntax is very
specific to what I was able to determine about C<Data::Dumper>'s
behavior.

And that's pretty much it.  The grammar looks for a series of
assignments, each of which is an lvalue being given an rvalue.  Some
of the lvalues ultimately lead to symbol table entries, which then
populate our symbol table hash with keys and values when assigned.
Rvalues can be constants, or pointers into existing data in the symbol
table.  OK, it I<is> somewhat full of smoke and mirrors, and I was
rather pleased when the whole thing worked.  And on some rainy Oregon
afternoon, I hope to extend it to handle arrays and hashes as well, just
to make it extremely flexible and universal. I even have some ideas
about how to get it to handle globs. But that's for another day: until
next time, enjoy!
