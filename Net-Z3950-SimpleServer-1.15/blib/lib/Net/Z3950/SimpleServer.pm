## This file is part of simpleserver
## Copyright (C) 2000-2011 Index Data.
## All rights reserved.
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
##     * Redistributions of source code must retain the above copyright
##       notice, this list of conditions and the following disclaimer.
##     * Redistributions in binary form must reproduce the above copyright
##       notice, this list of conditions and the following disclaimer in the
##       documentation and/or other materials provided with the distribution.
##     * Neither the name of Index Data nor the names of its contributors
##       may be used to endorse or promote products derived from this
##       software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
## EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
## WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
## DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
## DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
## (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
## LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
## ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
## (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
## THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package Net::Z3950::SimpleServer;

use strict;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
use Carp;

require Exporter;
require DynaLoader;
require AutoLoader;

@ISA = qw(Exporter AutoLoader DynaLoader);
@EXPORT = qw( );
$VERSION = '1.15';

bootstrap Net::Z3950::SimpleServer $VERSION;

# Preloaded methods go here.

my $count = 0;

sub new {
	my $class = shift;
	my %args = @_;
	my $self = \%args;

	if ($count) {
		carp "SimpleServer.pm: WARNING: Multithreaded server unsupported";
	}
	$count = 1;

	croak "SimpleServer.pm: ERROR: Unspecified search handler" unless defined($self->{SEARCH});
	croak "SimpleServer.pm: ERROR: Unspecified fetch handler" unless defined($self->{FETCH});

	bless $self, $class;
	return $self;
}


sub launch_server {
	my $self = shift;
	my @args = @_;

	### This modal internal interface, in which we set a bunch of
	#   globals and then call start_server(), is asking for
	#   trouble.  Instead, we should just pass the $self object
	#   as a parameter into start_server().
	if (defined($self->{GHANDLE})) {
		set_ghandle($self->{GHANDLE});
	}
	if (defined($self->{INIT})) {
		set_init_handler($self->{INIT});
	}
	set_search_handler($self->{SEARCH});
	set_fetch_handler($self->{FETCH});
	if (defined($self->{CLOSE})) {
		set_close_handler($self->{CLOSE});
	}
	if (defined($self->{PRESENT})) {
		set_present_handler($self->{PRESENT});
	}
	if (defined($self->{SCAN})) {
		set_scan_handler($self->{SCAN});
	}
	if (defined($self->{SORT})) {
		set_sort_handler($self->{SORT});
	}
	if (defined($self->{EXPLAIN})) {
		set_explain_handler($self->{EXPLAIN});
	}
	if (defined($self->{DELETE})) {
		set_delete_handler($self->{DELETE});
	}

	start_server(@args);
}


# Register packages that we will use in translated RPNs
package Net::Z3950::RPN::Node;
package Net::Z3950::APDU::Query;
our @ISA = qw(Net::Z3950::RPN::Node);
package Net::Z3950::APDU::OID;
package Net::Z3950::RPN::And;
our @ISA = qw(Net::Z3950::RPN::Node);
package Net::Z3950::RPN::Or;
our @ISA = qw(Net::Z3950::RPN::Node);
package Net::Z3950::RPN::AndNot;
our @ISA = qw(Net::Z3950::RPN::Node);
package Net::Z3950::RPN::Term;
our @ISA = qw(Net::Z3950::RPN::Node);
package Net::Z3950::RPN::RSID;
our @ISA = qw(Net::Z3950::RPN::Node);
package Net::Z3950::RPN::Attributes;
package Net::Z3950::RPN::Attribute;
package Net::Z3950::FacetList;
package Net::Z3950::FacetField;
package Net::Z3950::FacetTerms;
package Net::Z3950::FacetTerm;


# Utility method for re-rendering Type-1 query back down to PQF
package Net::Z3950::RPN::Node;

sub toPQF {
    my $this = shift();
    my $class = ref $this;

    if ($class eq "Net::Z3950::APDU::Query") {
	my $res = "";
	my $set = $this->{attributeSet};
	$res .= "\@attrset $set " if defined $set;
	return $res . $this->{query}->toPQF();
    } elsif ($class eq "Net::Z3950::RPN::Or") {
	return '@or ' . $this->[0]->toPQF() . ' ' . $this->[1]->toPQF();
    } elsif ($class eq "Net::Z3950::RPN::And") {
	return '@and ' . $this->[0]->toPQF() . ' ' . $this->[1]->toPQF();
    } elsif ($class eq "Net::Z3950::RPN::AndNot") {
	return '@not ' . $this->[0]->toPQF() . ' ' . $this->[1]->toPQF();
    } elsif ($class eq "Net::Z3950::RPN::RSID") {
	return '@set ' . $this->{id};
    } elsif ($class ne "Net::Z3950::RPN::Term") {
	die "unknown PQF node-type '$class'";
    }

    my $res = "";
    foreach my $attr (@{ $this->{attributes} }) {
	$res .= "\@attr ";
	my $set = $attr->{attributeSet};
	$res .= "$set " if defined $set;
	$res .= $attr->{attributeType} . "=" . $attr->{attributeValue} . " ";
    }

    return $res . $this->{term};
}


# Must revert to original package for Autoloader's benefit
package Net::Z3950::SimpleServer;


# Autoload methods go after =cut, and are processed by the autosplit program.

1;
__END__
# Below is the stub of documentation for your module. You better edit it!

=head1 NAME

Net::Z3950::SimpleServer - Simple Perl API for building Z39.50 servers. 

=head1 SYNOPSIS

  use Net::Z3950::SimpleServer;

  sub my_search_handler {
	my $args = shift;

	my $set_id = $args->{SETNAME};
	my @database_list = @{ $args->{DATABASES} };
	my $query = $args->{QUERY};

	## Perform the query on the specified set of databases
	## and return the number of hits:

	$args->{HITS} = $hits;
  }

  sub my_fetch_handler {        # Get a record for the user
	my $args = shift;

	my $set_id = $args->{SETNAME};

	my $record = fetch_a_record($args->{OFFSET});

	$args->{RECORD} = $record;
	if (number_of_hits() == $args->{OFFSET}) {	## Last record in set?
		$args->{LAST} = 1;
	} else {
		$args->{LAST} = 0;
	}
  }

  ## Register custom event handlers:
  my $z = new Net::Z3950::SimpleServer(GHANDLE = $someObject,
				       INIT   =>  \&my_init_handler,
				       CLOSE  =>  \&my_close_handler,
				       SEARCH =>  \&my_search_handler,
				       FETCH  =>  \&my_fetch_handler);

  ## Launch server:
  $z->launch_server("ztest.pl", @ARGV);

=head1 DESCRIPTION

The SimpleServer module is a tool for constructing Z39.50 "Information
Retrieval" servers in Perl. The module is easy to use, but it
does help to have an understanding of the Z39.50 query
structure and the construction of structured retrieval records.

Z39.50 is a network protocol for searching remote databases and
retrieving the results in the form of structured "records". It is widely
used in libraries around the world, as well as in the US Federal Government.
In addition, it is generally useful whenever you wish to integrate a number
of different database systems around a shared, abstract data model.

The model of the module is simple: It implements a "generic" Z39.50
server, which invokes callback functions supplied by you to search
for content in your database. You can use any tools available in
Perl to supply the content, including modules like DBI and
WWW::Search.

The server will take care of managing the network connections for
you, and it will spawn a new process (or thread, in some
environments) whenever a new connection is received.

The programmer can specify subroutines to take care of the following type
of events:

  - Initialize request
  - Search request
  - Present request
  - Fetching of records
  - Scan request (browsing) 
  - Closing down connection

Note that only the Search and Fetch handler functions are required.
The module can supply default responses to the other on its own.

After the launching of the server, all control is given away from
the Perl script to the server. The server calls the registered
subroutines to field incoming requests from Z39.50 clients.

A reference to an anonymous hash is passed to each handler. Some of
the entries of these hashes are to be considered input and others
output parameters.

The Perl programmer specifies the event handlers for the server by
means of the SimpleServer object constructor

  my $z = new Net::Z3950::SimpleServer(
			INIT	=>	\&my_init_handler,
			CLOSE	=>	\&my_close_handler,
			SEARCH	=>	\&my_search_handler,
			PRESENT	=>	\&my_present_handler,
			SCAN	=>	\&my_scan_handler,
			FETCH	=>	\&my_fetch_handler,
  			EXPLAIN =>	\&my_explain_handler,
  			DELETE  =>	\&my_delete_handler,
  			SORT    =>	\&my_sort_handler);

In addition, the arguments to the constructor may include GHANDLE, a
global handle which is made available to each invocation of every
callback function.  This is typically a reference to either a hash or
an object.

If you want your SimpleServer to start a thread (threaded mode) to
handle each incoming Z39.50 request instead of forking a process
(forking mode), you need to register the handlers by symbol rather
than by code reference. Thus, in threaded mode, you will need to
register your handlers this way:

  my $z = new Net::Z3950::SimpleServer(
  			INIT	=>	"my_package::my_init_handler",
			CLOSE	=>	"my_package::my_close_handler",
			....
			....          );

where my_package is the Perl package in which your handler is
located.

After the custom event handlers are declared, the server is launched
by means of the method

  $z->launch_server("MyServer.pl", @ARGV);

Notice, the first argument should be the name of your server
script (for logging purposes), while the rest of the arguments
are documented in the YAZ toolkit manual: The section on
application invocation: <http://indexdata.com/yaz/doc/server.invocation.tkl>

In particular, you need to use the -T switch to start your SimpleServer
in threaded mode.

=head2 Init handler

The init handler is called whenever a Z39.50 client is attempting
to logon to the server. The exchange of parameters between the
server and the handler is carried out via an anonymous hash reached
by a reference, i.e.

  $args = shift;

The argument hash passed to the init handler has the form

  $args = {
				    ## Response parameters:

	     PEER_NAME =>  "",      ## Name or IP address of connecting client
	     IMP_ID    =>  "",      ## Z39.50 Implementation ID
	     IMP_NAME  =>  "",      ## Z39.50 Implementation name
	     IMP_VER   =>  "",      ## Z39.50 Implementation version
	     ERR_CODE  =>  0,       ## Error code, cnf. Z39.50 manual
	     ERR_STR   =>  "",      ## Error string (additional info.)
	     USER      =>  "xxx"    ## If Z39.50 authentication is used,
	     			    ## this member contains user name
	     PASS      =>  "yyy"    ## Under same conditions, this member
	     			    ## contains the password in clear text
	     GHANDLE   =>  $obj     ## Global handler specified at creation
	     HANDLE    =>  undef    ## Handler of Perl data structure
	  };

The HANDLE member can be used to store any scalar value which will then
be provided as input to all subsequent calls (ie. for searching, record
retrieval, etc.). A common use of the handle is to store a reference to
a hash which may then be used to store session-specific parameters.
If you have any session-specific information (such as a list of
result sets or a handle to a back-end search engine of some sort),
it is always best to store them in a private session structure -
rather than leaving them in global variables in your script.

The Implementation ID, name and version are only really used by Z39.50
client developers to see what kind of server they're dealing with.
Filling these in is optional.

The ERR_CODE should be left at 0 (the default value) if you wish to
accept the connection. Any other value is interpreted as a failure
and the client will be shown the door, with the code and the
associated additional information, ERR_STR returned.

=head2 Search handler

Similarly, the search handler is called with a reference to an anony-
mous hash. The structure is the following:

  $args = {
	  			    ## Request parameters:

	     GHANDLE   =>  $obj     ## Global handler specified at creation
	     HANDLE    =>  ref,     ## Your session reference.
	     SETNAME   =>  "id",    ## ID of the result set
	     REPL_SET  =>  0,       ## Replace set if already existing?
	     DATABASES =>  ["xxx"], ## Reference to a list of data-
				    ## bases to search
	     QUERY     =>  "query", ## The query expression
	     RPN       =>  $obj,    ## Reference to a Net::Z3950::APDU::Query

				    ## Response parameters:

	     ERR_CODE  =>  0,       ## Error code (0=Successful search)
	     ERR_STR   =>  "",      ## Error string
	     HITS      =>  0        ## Number of matches
	  };

Note that a search which finds 0 hits is considered successful in
Z39.50 terms - you should only set the ERR_CODE to a non-zero value
if there was a problem processing the request. The Z39.50 standard
provides a comprehensive list of standard diagnostic codes, and you
should use these whenever possible.

The QUERY is a tree-structure of terms combined by operators, the
terms being qualified by lists of attributes. The query is presented
to the search function in the Prefix Query Format (PQF) which is
used in many applications based on the YAZ toolkit. The full grammar
is described in the YAZ manual.

The following are all examples of valid queries in the PQF. 

	dylan

	"bob dylan"

	@or "dylan" "zimmerman"

	@set Result-1

	@or @and bob dylan @set Result-1

	@and @attr 1=1 "bob dylan" @attr 1=4 "slow train coming"

	@attrset @attr 4=1 @attr 1=4 "self portrait"

You will need to write a recursive function or something similar to
parse incoming query expressions, and this is usually where a lot of
the work in writing a database-backend happens. Fortunately, you don't
need to support anymore functionality than you want to. For instance,
it is perfectly legal to not accept boolean operators, but you SHOULD
try to return good error codes if you run into something you can't or
won't support.

A more convenient alternative to the QUERY member may be the RPN
member, which is a reference to a Net::Z3950::APDU::Query object
representing the RPN query tree.  The structure of that object is
supposed to be self-documenting, but here's a brief summary of what
you get:

=over 4

=item *

C<Net::Z3950::APDU::Query> is a hash with two fields:

Z<>

=over 4

=item C<attributeSet>

Optional.  If present, it is a reference to a
C<Net::Z3950::APDU::OID>.  This is a string of dot-separated integers
representing the OID of the query's top-level attribute set.

=item C<query>

Mandatory: a reference to the RPN tree itself.

=back

=item *

Each node of the tree is an object of one of the following types:

Z<>

=over 4

=item C<Net::Z3950::RPN::And>

=item C<Net::Z3950::RPN::Or>

=item C<Net::Z3950::RPN::AndNot>

These three classes are all arrays of two elements, each of which is a
node of one of the above types.

=item C<Net::Z3950::RPN::Term>

See below for details.

=item C<Net::Z3950::RPN::RSID>

A reference to a result-set ID indicating a previous search.  The ID
of the result-set is in the C<id> element.

=back

=back

=over 4

=item *

C<Net::Z3950::RPN::Term> is a hash with two fields:

Z<>

=over 4

=item C<term>

A string containing the search term itself.

=item C<attributes>

A reference to a C<Net::Z3950::RPN::Attributes> object.

=back

=item *

C<Net::Z3950::RPN::Attributes> is an array of references to
C<Net::Z3950::RPN::Attribute> objects.  (Note the plural/singular
distinction.)

=item *

C<Net::Z3950::RPN::Attribute> is a hash with three elements:

Z<>

=over 4

=item C<attributeSet>

Optional.  If present, it is dot-separated OID string, as above.

=item C<attributeType>

An integer indicating the type of the attribute - for example, under
the BIB-1 attribute set, type 1 indicates a ``use'' attribute, type 2
a ``relation'' attribute, etc.

=item C<attributeValue>

An integer or string indicating the value of the attribute - for example, under
BIB-1, if the attribute type is 1, then value 4 indicates a title
search and 7 indicates an ISBN search; but if the attribute type is
2, then value 4 indicates a ``greater than or equal'' search, and 102
indicates a relevance match.

=back

=back

All of these classes except C<Attributes> and C<Attribute> are
subclasses of the abstract class C<Net::Z3950::RPN::Node>.  That class
has a single method, C<toPQF()>, which may be used to turn an RPN
tree, or part of one, back into a textual prefix query.

Note that, apart to C<toPQF()>, none of these classes have any methods at
all: the blessing into classes is largely just a documentation thing
so that, for example, if you do

	{ use Data::Dumper; print Dumper($args->{RPN}) }

you get something fairly human-readable.  But of course, the type
distinction between the three different kinds of boolean node is
important.

By adding your own methods to these classes (building what I call
``augmented classes''), you can easily build code that walks the tree
of the incoming RPN.  Take a look at C<samples/render-search.pl> for a
sample implementation of such an augmented classes technique.


=head2 Present handler

The presence of a present handler in a SimpleServer front-end is optional.
Each time a client wishes to retrieve records, the present service is
called. The present service allows the origin to request a certain number
of records retrieved from a given result set.
When the present handler is called, the front-end server should prepare a
result set for fetching. In practice, this means to get access to the
data from the backend database and store the data in a temporary fashion
for fast and efficient fetching. The present handler does *not* fetch
anything. This task is taken care of by the fetch handler, which will be
called the correct number of times by the YAZ library. More about this
below.
If no present handler is implemented in the front-end, the YAZ toolkit
will take care of a minimum of preparations itself. This default present
handler is sufficient in many situations, where only a small amount of
records are expected to be retrieved. If on the other hand, large result
sets are likely to occur, the implementation of a reasonable present
handler can gain performance significantly.

The information exchanged between client and present handle is:

  $args = {
				    ## Client/server request:

	     GHANDLE   =>  $obj     ## Global handler specified at creation
	     HANDLE    =>  ref,     ## Reference to datastructure
	     SETNAME   =>  "id",    ## Result set ID
	     START     =>  xxx,     ## Start position
	     COMP      =>  "",	    ## Desired record composition
	     NUMBER    =>  yyy,	    ## Number of requested records


				    ## Response parameters:

	     HITS      =>  zzz,	    ## Number of returned records
	     ERR_CODE  =>  0,	    ## Error code
	     ERR_STR   =>  ""	    ## Error message
          };


=head2 Fetch handler

The fetch handler is asked to retrieve a SINGLE record from a given
result set (the front-end server will automatically call the fetch
handler as many times as required).

The parameters exchanged between the server and the fetch handler are

  $args = {
				    ## Client/server request:

	     GHANDLE   =>  $obj     ## Global handler specified at creation
	     HANDLE    =>  ref	    ## Reference to data structure
	     SETNAME   =>  "id"     ## ID of the requested result set
	     OFFSET    =>  nnn      ## Record offset number
	     REQ_FORM  =>  "n.m.k.l"## Client requested format OID
	     COMP      =>  "xyz"    ## Formatting instructions
	     SCHEMA    =>  "abc"    ## Requested schema, if any

				    ## Handler response:

	     RECORD    =>  ""       ## Record string
	     BASENAME  =>  ""       ## Origin of returned record
	     LAST      =>  0        ## Last record in set?
	     ERR_CODE  =>  0        ## Error code
	     ERR_STR   =>  ""       ## Error string
	     SUR_FLAG  =>  0        ## Surrogate diagnostic flag
	     REP_FORM  =>  "n.m.k.l"## Provided format OID
	     SCHEMA    =>  "abc"    ## Provided schema, if any
	  };

The REP_FORM value has by default the REQ_FORM value but can be set to
something different if the handler desires. The BASENAME value should
contain the name of the database from where the returned record originates.
The ERR_CODE and ERR_STR works the same way they do in the search
handler. If there is an error condition, the SUR_FLAG is used to
indicate whether the error condition pertains to the record currently
being retrieved, or whether it pertains to the operation as a whole
(eg. the client has specified a result set which does not exist.)

If you need to return USMARC records, you might want to have a look at
the MARC module on CPAN, if you don't already have a way of generating
these.

NOTE: The record offset is 1-indexed - 1 is the offset of the first
record in the set.

=head2 Scan handler

A full featured Z39.50 server should support scan (or in some literature
browse). The client specifies a starting term of the scan, and the server
should return an ordered list of specified length consisting of terms
actually occurring in the data base. Each of these terms should be close
to or equal to the term originally specified. The quality of scan compared
to simple search is a guarantee of hits. It is simply like browsing through
an index of a book, you always find something! The parameters exchanged are

  $args = {
						## Client request

		GHANDLE		=> $obj,	## Global handler specified at creation
		HANDLE		=> $ref,	## Reference to data structure
		DATABASES	=> ["xxx"],	## Reference to a list of data-
						## bases to search
		TERM		=> 'start',	## The start term
		RPN		=>  $obj,       ## Reference to a Net::Z3950::RPN::Term

		NUMBER		=> xx,		## Number of requested terms
		POS		=> yy,		## Position of starting point
						## within returned list
		STEP		=> 0,		## Step size

						## Server response

		ERR_CODE	=> 0,		## Error code
		ERR_STR		=> '',		## Diagnostic message
		NUMBER		=> zz,		## Number of returned terms
		STATUS		=> $status,	## ScanSuccess/ScanFailure
		ENTRIES		=> $entries	## Referenced list of terms
	};

where the term list is returned by reference in the scalar $entries, which
should point at a data structure of this kind,

  my $entries = [
			{	TERM		=> 'energy',
				OCCURRENCE	=> 5		},

			{	TERM		=> 'energy density',
				OCCURRENCE	=> 6,		},

			{	TERM		=> 'energy flow',
				OCCURRENCE	=> 3		},

				...

				...
	];

The $status flag is only meaningful after a successful scan, and
should be assigned one of two values:

  Net::Z3950::SimpleServer::ScanSuccess  Full success (default)
  Net::Z3950::SimpleServer::ScanPartial  Fewer terms returned than requested

The STEP member contains the requested number of entries in the term-list
between two adjacent entries in the response.

A better alternative to the TERM member is the the RPN
member, which is a reference to a Net::Z3950::RPN::Term object
representing the scan clause.  The structure of that object is the
same as for Term objects included as part of the RPN tree passed to
search handlers.  This is more useful than the simple TERM because it
includes attributes (e.g. access points associated with the term),
which are discarded by the TERM element.

=head2 Close handler

The argument hash received by the close handler has two elements only:

  $args = {
				    ## Server provides:

	     GHANDLE   =>  $obj     ## Global handler specified at creation
	     HANDLE    =>  ref      ## Reference to data structure
	  };

What ever data structure the HANDLE value points at goes out of scope
after this call. If you need to close down a connection to your server
or something similar, this is the place to do it.

=head2 Delete handler

The argument hash received by the delete handler has the following elements:

  $args = {
				    ## Client request:
	     GHANDLE   =>  $obj,    ## Global handler specified at creation
	     HANDLE    =>  ref,     ## Reference to data structure
	     SETNAME   =>  "id",    ## Result set ID

				    ## Server response:
	     STATUS    => 0         ## Deletion status
	  };

The SETNAME element of the argument hash may or may not be defined.
If it is, then SETNAME is the name of a result set to be deleted; if
not, then all result-sets associated with the current session should
be deleted.  In either case, the callback function should report on
success or failure by setting the STATUS element either to zero, on
success, or to an integer from 1 to 10, to indicate one of the ten
possible failure codes described in section 3.2.4.1.4 of the Z39.50
standard -- see 
http://www.loc.gov/z3950/agency/markup/05.html#Delete-list-statuses1

=head2 Sort handler

The argument hash received by the sort handler has the following elements:

	$args = {
					## Client request:
		GHANDLE => $obj,	## Global handler specified at creation
		HANDLE => ref,		## Reference to data structure
		INPUT => [ a, b ... ],	## Names of result-sets to sort
		OUTPUT => "name",	## Name of result-set to sort into
		SEQUENCE		## Sort specification: see below

					## Server response:
		STATUS => 0,		## Success, Partial or Failure
		ERR_CODE => 0,		## Error code
		ERR_STR => '',		## Diagnostic message

	};

The SEQUENCE element is a reference to an array, each element of which
is a hash representing a sort key.  Each hash contains the following
elements:

=over 4

=item RELATION

0 for an ascending sort, 1 for descending, 3 for ascending by
frequency, or 4 for descending by frequency.

=item CASE

0 for a case-sensitive sort, 1 for case-insensitive

=item MISSING

How to respond if one or more records in the set to be sorted are
missing the fields indicated in the sort specification.  1 to abort
the sort, 2 to use a "null value", 3 if a value is provided to use in
place of the missing data (although in the latter case, the actual
value to use is currently not made available, so this is useless).

=back

And one or other of the following:

=over 4

=item SORTFIELD

A string indicating the field to be sorted, which the server may
interpret as it sees fit (presumably by an out-of-band agreement with
the client).

=item ELEMENTSPEC_TYPE and ELEMENTSPEC_VALUE

I have no idea what this is.

=item ATTRSET and SORT_ATTR

ATTRSET is the attribute set from which the attributes are taken, and
SORT_ATTR is a reference to an array containing the attributes
themselves.  Each attribute is represented by (are you following this
carefully?) yet another hash, this one containing the elements
ATTR_TYPE and ATTR_VALUE: for example, type=1 and value=4 in the BIB-1
attribute set would indicate access-point 4 which is title, so that a
sort of title is requested.

=back

Precisely why all of the above is so is not clear, but goes some way
to explain why, in the Z39.50 world, the developers of the standard
are not so much worshiped as blamed.

The backend function should set STATUS to 0 on success, 1 for "partial
success" (don't ask) or 2 on failure, in which case ERR_CODE and
ERR_STR should be set.

=head2 Support for SRU and SRW

Since release 1.0, SimpleServer includes support for serving the SRU
and SRW protocols as well as Z39.50.  These ``web-friendly'' protocols
enable similar functionality to that of Z39.50, but by means of rich
URLs in the case of SRU, and a SOAP-based web-service in the case of
SRW.  These protocols are described at
http://www.loc.gov/sru

In order to serve these protocols from a SimpleServer-based
application, it is necessary to launch the application with a YAZ
Generic Frontend Server (GFS) configuration file, which can be
specified using the command-line argument C<-f> I<filename>.  A
minimal configuration file looks like this:

  <yazgfs>
    <server>
      <cql2rpn>pqf.properties</cql2rpn>
    </server>
  </yazgfs>

This file specifies only that C<pqf.properties> should be used to
translate the CQL queries of SRU and SRW into corresponding Z39.50
Type-1 queries.  For more information about YAZ GFS configuration,
including how to specify an Explain record, see the I<Virtual Hosts>
section of the YAZ manual at
http://indexdata.com/yaz/doc/server.vhosts.tkl

The mapping of CQL queries into Z39.50 Type-1 queries is specified by
a file that indicates which BIB-1 attributes should be generated for
each CQL index, relation, modifiers, etc.  A typical section of this
file looks like this:

  index.dc.title                        = 1=4
  index.dc.subject                      = 1=21
  index.dc.creator                      = 1=1003
  relation.<                            = 2=1
  relation.le                           = 2=2

This file specifies the BIB-1 access points (type=1) for the Dublin
Core indexes C<title>, C<subject> and C<creator>, and the BIB-1
relations (type=2) corresponding to the CQL relations C<E<lt>> and
C<E<lt>=>.  For more information about the format of this file, see
the I<CQL> section of the YAZ manual at
http://indexdata.com/yaz/doc/tools.tkl#tools.cql

The YAZ distribution include a sample CQL-to-PQF mapping configuration
file called C<pqf.properties>; this is sufficient for many
applications, and a good base to work from for most others.

If a SimpleServer-based application is run without this SRU-specific
configuration, it can still serve SRU; however, CQL queries will not
be translated, but passed straight through to the search-handler
function, as the C<CQL> member of the parameters hash.  It is then the
responsibility of the back-end application to parse and handle the CQL
query, which is most easily done using Ed Summers' fine C<CQL::Parser>
module, available from CPAN at
http://search.cpan.org/~esummers/CQL-Parser/

=head1 AUTHORS

Anders Sønderberg (sondberg@indexdata.dk),
Sebastian Hammer (quinn@indexdata.dk),
Mike Taylor (indexdata.com).

=head1 COPYRIGHT AND LICENCE

Copyright (C) 2000-2011 by Index Data.

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.8.4 or,
at your option, any later version of Perl 5 you may have available.

=head1 SEE ALSO

Any Perl module which is useful for accessing the data source of your
choice.

=cut
