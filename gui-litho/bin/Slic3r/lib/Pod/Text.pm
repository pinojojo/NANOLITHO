# Convert POD data to formatted text.
#
# This module converts POD to formatted text.  It replaces the old Pod::Text
# module that came with versions of Perl prior to 5.6.0 and attempts to match
# its output except for some specific circumstances where other decisions
# seemed to produce better output.  It uses Pod::Parser and is designed to be
# very easy to subclass.
#
# Perl core hackers, please note that this module is also separately
# maintained outside of the Perl core as part of the podlators.  Please send
# me any patches at the address above in addition to sending them to the
# standard Perl mailing lists.
#
# Copyright 1999, 2000, 2001, 2002, 2004, 2006, 2008, 2009, 2012, 2013, 2014,
#     2015, 2016 Russ Allbery <rra@cpan.org>
#
# This program is free software; you may redistribute it and/or modify it
# under the same terms as Perl itself.

##############################################################################
# Modules and declarations
##############################################################################

package Pod::Text;

use 5.006;
use strict;
use warnings;

use vars qw(@ISA @EXPORT %ESCAPES $VERSION);

use Carp qw(carp croak);
use Encode qw(encode);
use Exporter ();
use Pod::Simple ();

@ISA = qw(Pod::Simple Exporter);

# We have to export pod2text for backward compatibility.
@EXPORT = qw(pod2text);

$VERSION = '4.09';

# Ensure that $Pod::Simple::nbsp and $Pod::Simple::shy are available.  Code
# taken from Pod::Simple 3.32, but was only added in 3.30.
my ($NBSP, $SHY);
if ($Pod::Simple::VERSION ge 3.30) {
    $NBSP = $Pod::Simple::nbsp;
    $SHY  = $Pod::Simple::shy;
} else {
    if ($] ge 5.007_003) {
        $NBSP = chr utf8::unicode_to_native(0xA0);
        $SHY  = chr utf8::unicode_to_native(0xAD);
    } elsif (Pod::Simple::ASCII) {
        $NBSP = "\xA0";
        $SHY  = "\xAD";
    } else {
        $NBSP = "\x41";
        $SHY  = "\xCA";
    }
}

##############################################################################
# Initialization
##############################################################################

# This function handles code blocks.  It's registered as a callback to
# Pod::Simple and therefore doesn't work as a regular method call, but all it
# does is call output_code with the line.
sub handle_code {
    my ($line, $number, $parser) = @_;
    $parser->output_code ($line . "\n");
}

# Initialize the object and set various Pod::Simple options that we need.
# Here, we also process any additional options passed to the constructor or
# set up defaults if none were given.  Note that all internal object keys are
# in all-caps, reserving all lower-case object keys for Pod::Simple and user
# arguments.
sub new {
    my $class = shift;
    my $self = $class->SUPER::new;

    # Tell Pod::Simple to handle S<> by automatically inserting &nbsp;.
    $self->nbsp_for_S (1);

    # Tell Pod::Simple to keep whitespace whenever possible.
    if ($self->can ('preserve_whitespace')) {
        $self->preserve_whitespace (1);
    } else {
        $self->fullstop_space_harden (1);
    }

    # The =for and =begin targets that we accept.
    $self->accept_targets (qw/text TEXT/);

    # Ensure that contiguous blocks of code are merged together.  Otherwise,
    # some of the guesswork heuristics don't work right.
    $self->merge_text (1);

    # Pod::Simple doesn't do anything useful with our arguments, but we want
    # to put them in our object as hash keys and values.  This could cause
    # problems if we ever clash with Pod::Simple's own internal class
    # variables.
    my %opts = @_;
    my @opts = map { ("opt_$_", $opts{$_}) } keys %opts;
    %$self = (%$self, @opts);

    # Send errors to stderr if requested.
    if ($$self{opt_stderr} and not $$self{opt_errors}) {
        $$self{opt_errors} = 'stderr';
    }
    delete $$self{opt_stderr};

    # Validate the errors parameter and act on it.
    if (not defined $$self{opt_errors}) {
        $$self{opt_errors} = 'pod';
    }
    if ($$self{opt_errors} eq 'stderr' || $$self{opt_errors} eq 'die') {
        $self->no_errata_section (1);
        $self->complain_stderr (1);
        if ($$self{opt_errors} eq 'die') {
            $$self{complain_die} = 1;
        }
    } elsif ($$self{opt_errors} eq 'pod') {
        $self->no_errata_section (0);
        $self->complain_stderr (0);
    } elsif ($$self{opt_errors} eq 'none') {
        $self->no_whining (1);
    } else {
        croak (qq(Invalid errors setting: "$$self{errors}"));
    }
    delete $$self{errors};

    # Initialize various things from our parameters.
    $$self{opt_alt}      = 0  unless defined $$self{opt_alt};
    $$self{opt_indent}   = 4  unless defined $$self{opt_indent};
    $$self{opt_margin}   = 0  unless defined $$self{opt_margin};
    $$self{opt_loose}    = 0  unless defined $$self{opt_loose};
    $$self{opt_sentence} = 0  unless defined $$self{opt_sentence};
    $$self{opt_width}    = 76 unless defined $$self{opt_width};

    # Figure out what quotes we'll be using for C<> text.
    $$self{opt_quotes} ||= '"';
    if ($$self{opt_quotes} eq 'none') {
        $$self{LQUOTE} = $$self{RQUOTE} = '';
    } elsif (length ($$self{opt_quotes}) == 1) {
        $$self{LQUOTE} = $$self{RQUOTE} = $$self{opt_quotes};
    } elsif (length ($$self{opt_quotes}) % 2 == 0) {
        my $length = length ($$self{opt_quotes}) / 2;
        $$self{LQUOTE} = substr ($$self{opt_quotes}, 0, $length);
        $$self{RQUOTE} = substr ($$self{opt_quotes}, $length);
    } else {
        croak qq(Invalid quote specification "$$self{opt_quotes}");
    }

    # If requested, do something with the non-POD text.
    $self->code_handler (\&handle_code) if $$self{opt_code};

    # Return the created object.
    return $self;
}

##############################################################################
# Core parsing
##############################################################################

# This is the glue that connects the code below with Pod::Simple itself.  The
# goal is to convert the event stream coming from the POD parser into method
# calls to handlers once the complete content of a tag has been seen.  Each
# paragraph or POD command will have textual content associated with it, and
# as soon as all of a paragraph or POD command has been seen, that content
# will be passed in to the corresponding method for handling that type of
# object.  The exceptions are handlers for lists, which have opening tag
# handlers and closing tag handlers that will be called right away.
#
# The internal hash key PENDING is used to store the contents of a tag until
# all of it has been seen.  It holds a stack of open tags, each one
# represented by a tuple of the attributes hash for the tag and the contents
# of the tag.

# Add a block of text to the contents of the current node, formatting it
# according to the current formatting instructions as we do.
sub _handle_text {
    my ($self, $text) = @_;
    my $tag = $$self{PENDING}[-1];
    $$tag[1] .= $text;
}

# Given an element name, get the corresponding method name.
sub method_for_element {
    my ($self, $element) = @_;
    $element =~ tr/-/_/;
    $element =~ tr/A-Z/a-z/;
    $element =~ tr/_a-z0-9//cd;
    return $element;
}

# Handle the start of a new element.  If cmd_element is defined, assume that
# we need to collect the entire tree for this element before passing it to the
# element method, and create a new tree into which we'll collect blocks of
# text and nested elements.  Otherwise, if start_element is defined, call it.
sub _handle_element_start {
    my ($self, $element, $attrs) = @_;
    my $method = $self->method_for_element ($element);

    # If we have a command handler, we need to accumulate the contents of the
    # tag before calling it.
    if ($self->can ("cmd_$method")) {
        push (@{ $$self{PENDING} }, [ $attrs, '' ]);
    } elsif ($self->can ("start_$method")) {
        my $method = 'start_' . $method;
        $self->$method ($attrs, '');
    }
}

# Handle the end of an element.  If we had a cmd_ method for this element,
# this is where we pass along the text that we've accumulated.  Otherwise, if
# we have an end_ method for the element, call that.
sub _handle_element_end {
    my ($self, $element) = @_;
    my $method = $self->method_for_element ($element);

    # If we have a command handler, pull off the pending text and pass it to
    # the handler along with the saved attribute hash.
    if ($self->can ("cmd_$method")) {
        my $tag = pop @{ $$self{PENDING} };
        my $method = 'cmd_' . $method;
        my $text = $self->$method (@$tag);
        if (defined $text) {
            if (@{ $$self{PENDING} } > 1) {
                $$self{PENDING}[-1][1] .= $text;
            } else {
                $self->output ($text);
            }
        }
    } elsif ($self->can ("end_$method")) {
        my $method = 'end_' . $method;
        $self->$method ();
    }
}

##############################################################################
# Output formatting
##############################################################################

# Wrap a line, indenting by the current left margin.  We can't use Text::Wrap
# because it plays games with tabs.  We can't use formline, even though we'd
# really like to, because it screws up non-printing characters.  So we have to
# do the wrapping ourselves.
sub wrap {
    my $self = shift;
    local $_ = shift;
    my $output = '';
    my $spaces = ' ' x $$self{MARGIN};
    my $width = $$self{opt_width} - $$self{MARGIN};
    while (length > $width) {
        if (s/^([^\n]{0,$width})\s+// || s/^([^\n]{$width})//) {
            $output .= $spaces . $1 . "\n";
        } else {
            last;
        }
    }
    $output .= $spaces . $_;
    $output =~ s/\s+$/\n\n/;
    return $output;
}

# Reformat a paragraph of text for the current margin.  Takes the text to
# reformat and returns the formatted text.
sub reformat {
    my $self = shift;
    local $_ = shift;

    # If we're trying to preserve two spaces after sentences, do some munging
    # to support that.  Otherwise, smash all repeated whitespace.
    if ($$self{opt_sentence}) {
        s/ +$//mg;
        s/\.\n/. \n/g;
        s/\n/ /g;
        s/   +/  /g;
    } else {
        s/\s+/ /g;
    }
    return $self->wrap ($_);
}

# Output text to the output device.  Replace non-breaking spaces with spaces
# and soft hyphens with nothing, and then try to fix the output encoding if
# necessary to match the input encoding unless UTF-8 output is forced.  This
# preserves the traditional pass-through behavior of Pod::Text.
sub output {
    my ($self, @text) = @_;
    my $text = join ('', @text);
    if ($NBSP) {
        $text =~ s/$NBSP/ /g;
    }
    if ($SHY) {
        $text =~ s/$SHY//g;
    }
    unless ($$self{opt_utf8}) {
        my $encoding = $$self{encoding} || '';
        if ($encoding && $encoding ne $$self{ENCODING}) {
            $$self{ENCODING} = $encoding;
            eval { binmode ($$self{output_fh}, ":encoding($encoding)") };
        }
    }
    if ($$self{ENCODE}) {
        print { $$self{output_fh} } encode ('UTF-8', $text);
    } else {
        print { $$self{output_fh} } $text;
    }
}

# Output a block of code (something that isn't part of the POD text).  Called
# by preprocess_paragraph only if we were given the code option.  Exists here
# only so that it can be overridden by subclasses.
sub output_code { $_[0]->output ($_[1]) }

##############################################################################
# Document initialization
##############################################################################

# Set up various things that have to be initialized on a per-document basis.
sub start_document {
    my ($self, $attrs) = @_;
    if ($$attrs{contentless} && !$$self{ALWAYS_EMIT_SOMETHING}) {
        $$self{CONTENTLESS} = 1;
    } else {
        delete $$self{CONTENTLESS};
    }
    my $margin = $$self{opt_indent} + $$self{opt_margin};

    # Initialize a few per-document variables.
    $$self{INDENTS} = [];       # Stack of indentations.
    $$self{MARGIN}  = $margin;  # Default left margin.
    $$self{PENDING} = [[]];     # Pending output.

    # We have to redo encoding handling for each document.
    $$self{ENCODING} = '';

    # When UTF-8 output is set, check whether our output file handle already
    # has a PerlIO encoding layer set.  If it does not, we'll need to encode
    # our output before printing it (handled in the output() sub).  Wrap the
    # check in an eval to handle versions of Perl without PerlIO.
    $$self{ENCODE} = 0;
    if ($$self{opt_utf8}) {
        $$self{ENCODE} = 1;
        eval {
            my @options = (output => 1, details => 1);
            my $flag = (PerlIO::get_layers ($$self{output_fh}, @options))[-1];
            if ($flag & PerlIO::F_UTF8 ()) {
                $$self{ENCODE} = 0;
                $$self{ENCODING} = 'UTF-8';
            }
        };
    }

    return '';
}

# Handle the end of the document.  The only thing we do is handle dying on POD
# errors, since Pod::Parser currently doesn't.
sub end_document {
    my ($self) = @_;
    if ($$self{complain_die} && $self->errors_seen) {
        croak ("POD document had syntax errors");
    }
}

##############################################################################
# Text blocks
##############################################################################

# Intended for subclasses to override, this method returns text with any
# non-printing formatting codes stripped out so that length() correctly
# returns the length of the text.  For basic Pod::Text, it does nothing.
sub strip_format {
    my ($self, $string) = @_;
    return $string;
}

# This method is called whenever an =item command is complete (in other words,
# we've seen its associated paragraph or know for certain that it doesn't have
# one).  It gets the paragraph associated with the item as an argument.  If
# that argument is empty, just output the item tag; if it contains a newline,
# output the item tag followed by the newline.  Otherwise, see if there's
# enough room for us to output the item tag in the margin of the text or if we
# have to put it on a separate line.
sub item {
    my ($self, $text) = @_;
    my $tag = $$self{ITEM};
    unless (defined $tag) {
        carp "Item called without tag";
        return;
    }
    undef $$self{ITEM};

    # Calculate the indentation and margin.  $fits is set to true if the tag
    # will fit into the margin of the paragraph given our indentation level.
    my $indent = $$self{INDENTS}[-1];
    $indent = $$self{opt_indent} unless defined $indent;
    my $margin = ' ' x $$self{opt_margin};
    my $tag_length = length ($self->strip_format ($tag));
    my $fits = ($$self{MARGIN} - $indent >= $tag_length + 1);

    # If the tag doesn't fit, or if we have no associated text, print out the
    # tag separately.  Otherwise, put the tag in the margin of the paragraph.
    if (!$text || $text =~ /^\s+$/ || !$fits) {
        my $realindent = $$self{MARGIN};
        $$self{MARGIN} = $indent;
        my $output = $self->reformat ($tag);
        $output =~ s/^$margin /$margin:/ if ($$self{opt_alt} && $indent > 0);
        $output =~ s/\n*$/\n/;

        # If the text is just whitespace, we have an empty item paragraph;
        # this can result from =over/=item/=back without any intermixed
        # paragraphs.  Insert some whitespace to keep the =item from merging
        # into the next paragraph.
        $output .= "\n" if $text && $text =~ /^\s*$/;

        $self->output ($output);
        $$self{MARGIN} = $realindent;
        $self->output ($self->reformat ($text)) if ($text && $text =~ /\S/);
    } else {
        my $space = ' ' x $indent;
        $space =~ s/^$margin /$margin:/ if $$self{opt_alt};
        $text = $self->reformat ($text);
        $text =~ s/^$margin /$margin:/ if ($$self{opt_alt} && $indent > 0);
        my $tagspace = ' ' x $tag_length;
        $text =~ s/^($space)$tagspace/$1$tag/ or warn "Bizarre space in item";
        $self->output ($text);
    }
}

# Handle a basic block of text.  The only tricky thing here is that if there
# is a pending item tag, we need to format this as an item paragraph.
sub cmd_para {
    my ($self, $attrs, $text) = @_;
    $text =~ s/\s+$/\n/;
    if (defined $$self{ITEM}) {
        $self->item ($text . "\n");
    } else {
        $self->output ($self->reformat ($text . "\n"));
    }
    return '';
}

# Handle a verbatim paragraph.  Just print it out, but indent it according to
# our margin.
sub cmd_verbatim {
    my ($self, $attrs, $text) = @_;
    $self->item if defined $$self{ITEM};
    return if $text =~ /^\s*$/;
    $text =~ s/^(\n*)([ \t]*\S+)/$1 . (' ' x $$self{MARGIN}) . $2/gme;
    $text =~ s/\s*$/\n\n/;
    $self->output ($text);
    return '';
}

# Handle literal text (produced by =for and similar constructs).  Just output
# it with the minimum of changes.
sub cmd_data {
    my ($self, $attrs, $text) = @_;
    $text =~ s/^\n+//;
    $text =~ s/\n{0,2}$/\n/;
    $self->output ($text);
    return '';
}

##############################################################################
# Headings
##############################################################################

# The common code for handling all headers.  Takes the header text, the
# indentation, and the surrounding marker for the alt formatting method.
sub heading {
    my ($self, $text, $indent, $marker) = @_;
    $self->item ("\n\n") if defined $$self{ITEM};
    $text =~ s/\s+$//;
    if ($$self{opt_alt}) {
        my $closemark = reverse (split (//, $marker));
        my $margin = ' ' x $$self{opt_margin};
        $self->output ("\n" . "$margin$marker $text $closemark" . "\n\n");
    } else {
        $text .= "\n" if $$self{opt_loose};
        my $margin = ' ' x ($$self{opt_margin} + $indent);
        $self->output ($margin . $text . "\n");
    }
    return '';
}

# First level heading.
sub cmd_head1 {
    my ($self, $attrs, $text) = @_;
    $self->heading ($text, 0, '====');
}

# Second level heading.
sub cmd_head2 {
    my ($self, $attrs, $text) = @_;
    $self->heading ($text, $$self{opt_indent} / 2, '==  ');
}

# Third level heading.
sub cmd_head3 {
    my ($self, $attrs, $text) = @_;
    $self->heading ($text, $$self{opt_indent} * 2 / 3 + 0.5, '=   ');
}

# Fourth level heading.
sub cmd_head4 {
    my ($self, $attrs, $text) = @_;
    $self->heading ($text, $$self{opt_indent} * 3 / 4 + 0.5, '-   ');
}

##############################################################################
# List handling
##############################################################################

# Handle the beginning of an =over block.  Takes the type of the block as the
# first argument, and then the attr hash.  This is called by the handlers for
# the four different types of lists (bullet, number, text, and block).
sub over_common_start {
    my ($self, $attrs) = @_;
    $self->item ("\n\n") if defined $$self{ITEM};

    # Find the indentation level.
    my $indent = $$attrs{indent};
    unless (defined ($indent) && $indent =~ /^\s*[-+]?\d{1,4}\s*$/) {
        $indent = $$self{opt_indent};
    }

    # Add this to our stack of indents and increase our current margin.
    push (@{ $$self{INDENTS} }, $$self{MARGIN});
    $$self{MARGIN} += ($indent + 0);
    return '';
}

# End an =over block.  Takes no options other than the class pointer.  Output
# any pending items and then pop one level of indentation.
sub over_common_end {
    my ($self) = @_;
    $self->item ("\n\n") if defined $$self{ITEM};
    $$self{MARGIN} = pop @{ $$self{INDENTS} };
    return '';
}

# Dispatch the start and end calls as appropriate.
sub start_over_bullet { $_[0]->over_common_start ($_[1]) }
sub start_over_number { $_[0]->over_common_start ($_[1]) }
sub start_over_text   { $_[0]->over_common_start ($_[1]) }
sub start_over_block  { $_[0]->over_common_start ($_[1]) }
sub end_over_bullet { $_[0]->over_common_end }
sub end_over_number { $_[0]->over_common_end }
sub end_over_text   { $_[0]->over_common_end }
sub end_over_block  { $_[0]->over_common_end }

# The common handler for all item commands.  Takes the type of the item, the
# attributes, and then the text of the item.
sub item_common {
    my ($self, $type, $attrs, $text) = @_;
    $self->item if defined $$self{ITEM};

    # Clean up the text.  We want to end up with two variables, one ($text)
    # which contains any body text after taking out the item portion, and
    # another ($item) which contains the actual item text.  Note the use of
    # the internal Pod::Simple attribute here; that's a potential land mine.
    $text =~ s/\s+$//;
    my ($item, $index);
    if ($type eq 'bullet') {
        $item = '*';
    } elsif ($type eq 'number') {
        $item = $$attrs{'~orig_content'};
    } else {
        $item = $text;
        $item =~ s/\s*\n\s*/ /g;
        $text = '';
    }
    $$self{ITEM} = $item;

    # If body text for this item was included, go ahead and output that now.
    if ($text) {
        $text =~ s/\s*$/\n/;
        $self->item ($text);
    }
    return '';
}

# Dispatch the item commands to the appropriate place.
sub cmd_item_bullet { my $self = shift; $self->item_common ('bullet', @_) }
sub cmd_item_number { my $self = shift; $self->item_common ('number', @_) }
sub cmd_item_text   { my $self = shift; $self->item_common ('text',   @_) }
sub cmd_item_block  { my $self = shift; $self->item_common ('block',  @_) }

##############################################################################
# Formatting codes
##############################################################################

# The simple ones.
sub cmd_b { return $_[0]{alt} ? "``$_[2]''" : $_[2] }
sub cmd_f { return $_[0]{alt} ? "\"$_[2]\"" : $_[2] }
sub cmd_i { return '*' . $_[2] . '*' }
sub cmd_x { return '' }

# Apply a whole bunch of messy heuristics to not quote things that don't
# benefit from being quoted.  These originally come from Barrie Slaymaker and
# largely duplicate code in Pod::Man.
sub cmd_c {
    my ($self, $attrs, $text) = @_;

    # A regex that matches the portion of a variable reference that's the
    # array or hash index, separated out just because we want to use it in
    # several places in the following regex.
    my $index = '(?: \[.*\] | \{.*\} )?';

    # Check for things that we don't want to quote, and if we find any of
    # them, return the string with just a font change and no quoting.
    $text =~ m{
      ^\s*
      (?:
         ( [\'\`\"] ) .* \1                             # already quoted
       | \` .* \'                                       # `quoted'
       | \$+ [\#^]? \S $index                           # special ($^Foo, $")
       | [\$\@%&*]+ \#? [:\'\w]+ $index                 # plain var or func
       | [\$\@%&*]* [:\'\w]+ (?: -> )? \(\s*[^\s,]\s*\) # 0/1-arg func call
       | [+-]? ( \d[\d.]* | \.\d+ ) (?: [eE][+-]?\d+ )? # a number
       | 0x [a-fA-F\d]+                                 # a hex constant
      )
      \s*\z
     }xo && return $text;

    # If we didn't return, go ahead and quote the text.
    return $$self{opt_alt}
        ? "``$text''"
        : "$$self{LQUOTE}$text$$self{RQUOTE}";
}

# Links reduce to the text that we're given, wrapped in angle brackets if it's
# a URL.
sub cmd_l {
    my ($self, $attrs, $text) = @_;
    if ($$attrs{type} eq 'url') {
        if (not defined($$attrs{to}) or $$attrs{to} eq $text) {
            return "<$text>";
        } elsif ($$self{opt_nourls}) {
            return $text;
        } else {
            return "$text <$$attrs{to}>";
        }
    } else {
        return $text;
    }
}

##############################################################################
# Backwards compatibility
##############################################################################

# The old Pod::Text module did everything in a pod2text() function.  This
# tries to provide the same interface for legacy applications.
sub pod2text {
    my @args;

    # This is really ugly; I hate doing option parsing in the middle of a
    # module.  But the old Pod::Text module supported passing flags to its
    # entry function, so handle -a and -<number>.
    while ($_[0] =~ /^-/) {
        my $flag = shift;
        if    ($flag eq '-a')       { push (@args, alt => 1)    }
        elsif ($flag =~ /^-(\d+)$/) { push (@args, width => $1) }
        else {
            unshift (@_, $flag);
            last;
        }
    }

    # Now that we know what arguments we're using, create the parser.
    my $parser = Pod::Text->new (@args);

    # If two arguments were given, the second argument is going to be a file
    # handle.  That means we want to call parse_from_filehandle(), which means
    # we need to turn the first argument into a file handle.  Magic open will
    # handle the <&STDIN case automagically.
    if (defined $_[1]) {
        my @fhs = @_;
        local *IN;
        unless (open (IN, $fhs[0])) {
            croak ("Can't open $fhs[0] for reading: $!\n");
            return;
        }
        $fhs[0] = \*IN;
        $parser->output_fh ($fhs[1]);
        my $retval = $parser->parse_file ($fhs[0]);
        my $fh = $parser->output_fh ();
        close $fh;
        return $retval;
    } else {
        $parser->output_fh (\*STDOUT);
        return $parser->parse_file (@_);
    }
}

# Reset the underlying Pod::Simple object between calls to parse_from_file so
# that the same object can be reused to convert multiple pages.
sub parse_from_file {
    my $self = shift;
    $self->reinit;

    # Fake the old cutting option to Pod::Parser.  This fiddings with internal
    # Pod::Simple state and is quite ugly; we need a better approach.
    if (ref ($_[0]) eq 'HASH') {
        my $opts = shift @_;
        if (defined ($$opts{-cutting}) && !$$opts{-cutting}) {
            $$self{in_pod} = 1;
            $$self{last_was_blank} = 1;
        }
    }

    # Do the work.
    my $retval = $self->Pod::Simple::parse_from_file (@_);

    # Flush output, since Pod::Simple doesn't do this.  Ideally we should also
    # close the file descriptor if we had to open one, but we can't easily
    # figure this out.
    my $fh = $self->output_fh ();
    my $oldfh = select $fh;
    my $oldflush = $|;
    $| = 1;
    print $fh '';
    $| = $oldflush;
    select $oldfh;
    return $retval;
}

# Pod::Simple failed to provide this backward compatibility function, so
# implement it ourselves.  File handles are one of the inputs that
# parse_from_file supports.
sub parse_from_filehandle {
    my $self = shift;
    $self->parse_from_file (@_);
}

# Pod::Simple's parse_file doesn't set output_fh.  Wrap the call and do so
# ourself unless it was already set by the caller, since our documentation has
# always said that this should work.
sub parse_file {
    my ($self, $in) = @_;
    unless (defined $$self{output_fh}) {
        $self->output_fh (\*STDOUT);
    }
    return $self->SUPER::parse_file ($in);
}

# Do the same for parse_lines, just to be polite.  Pod::Simple's man page
# implies that the caller is responsible for setting this, but I don't see any
# reason not to set a default.
sub parse_lines {
    my ($self, @lines) = @_;
    unless (defined $$self{output_fh}) {
        $self->output_fh (\*STDOUT);
    }
    return $self->SUPER::parse_lines (@lines);
}

# Likewise for parse_string_document.
sub parse_string_document {
    my ($self, $doc) = @_;
    unless (defined $$self{output_fh}) {
        $self->output_fh (\*STDOUT);
    }
    return $self->SUPER::parse_string_document ($doc);
}

##############################################################################
# Module return value and documentation
##############################################################################

1;
__END__

#line 1032
