BEGIN {use File::Basename; push @INC, dirname($0); }
require "valve_perl_helpers.pl";
use Cwd;
use String::CRC32;

sub ReadInputFileWithIncludes
{
	local( $filename ) = shift;

	local( *INPUT );
	local( $output );

	open INPUT, "<$filename" || die;

	local( $line );
	local( $linenum ) = 1;
	while( $line = <INPUT> )
	{
		if( $line =~ m/\#include\s+\"(.*)\"/i )
		{
			$output.= ReadInputFileWithIncludes( $1 );
		}
		else
		{
			$output .= $line;
		}
	}

	close INPUT;
	return $output;
}

sub PatchCRC
{
	my $filename = shift;
	my $crc = shift;
#	print STDERR "PatchCRC( $filename, $crc )\n";
	local( *FP );
	open FP, "+<$filename" || die;
	binmode( FP );
	seek FP, 6 * 4, 0;
	my $uInt = "I";
	if( $filename =~ m/360/ )
	{
		$uInt = "N";
	}
	print FP pack $uInt, $crc;
	close FP;
}

my $txtfilename = shift;
my $arg = shift;

my $is360 = 0;
my $platformextension = "";
if( $arg =~ m/-x360/i )
{
	$is360 = 1;
	$platformextension = ".360";
}

# Get the changelist number for the Shader Auto Checkout changelist. Will create the changelist if it doesn't exist.
my $changelistnumber = `valve_p4_create_changelist.cmd ..\\..\\..\\game\\hl2\\shaders \"Shader Auto Checkout VCS\"`;
# Get rid of the newline
$changelistnumber =~ s/\n//g;

my $changelistarg = "";
if( $changelistnumber != 0 )
{
	$changelistarg = "-c $changelistnumber"
}

open TXTFILE, "<$txtfilename";

my $src;
my $dst;
while( $src = <TXTFILE> )
{
	# get rid of comments
	$src =~ s,//.*,,g;

	# skip blank lines
	if( $src =~ m/^\s*$/ )
	{
		next;
	}

	# Get rid of newlines.
	$src =~ s/\n//g;
	
	# Save off the shader source filename.
	my $shadersrcfilename = $src;
	$shadersrcfilename =~ s/-----.*$//;
	# use only target basename.
	$src =~ s/^.*-----//;

	# where the binary vcs file is
	my $spath = "";

	if ( $shadersrcfilename =~ m@\.fxc@i )
	{
		$spath = "shaders\\fxc\\";
	}
	if ( $shadersrcfilename =~ m@\.vsh@i )
	{
		$spath = "shaders\\vsh\\";
	}
	if ( $shadersrcfilename =~ m@\.psh@i )
	{
		$spath = "shaders\\psh\\";
	}
	
	# make the source have path and extension
	$src = $spath . $src . $platformextension . ".vcs";

	# build the dest filename.
	$dst = $src;

	$dst =~ s/shaders\\/..\\..\\..\\game\\hl2\\shaders\\/i;

	# Does the dst exist?
	my $dstexists = -e $dst;
	my $srcexists = -e $src;
	# What are the time stamps for the src and dst?
	my $srcmodtime = ( stat $src )[9];
	my $dstmodtime = ( stat $dst )[9];

	# Write $dst to a file so that we can do perforce stuff to it later.
	local( *VCSLIST );
	open VCSLIST, ">>vcslist.txt" || die;
	print VCSLIST $dst . "\n";
	close VCSLIST;

	# Open for edit or add if different than what is in perforce already.
	if( !$dstexists || ( $srcmodtime != $dstmodtime ) )
	{
		if ( $srcexists && $shadersrcfilename =~ m@\.fxc@i )
		{
			# Get the CRC for the source file.
			my $srccode = ReadInputFileWithIncludes( $shadersrcfilename );
			my $crc = crc32( $srccode );

			# Patch the source VCS file with the CRC32 of the source code used to build that file.
			PatchCRC( $src, $crc );
		}

		# Make the target vcs writable if it exists
		if( $dstexists )
		{
			MakeFileWritable( $dst );
		}

		my $dir = $dst;
		$dir =~ s,([^/\\]*$),,;  # rip the filename off the end
		my $filename = $1;

		# create the target directory if it doesn't exist
		if( !$dstexists )
		{
			&MakeDirHier( $dir, 0777 );
		}

		# copy the file to its targets. . . we want to see STDERR here if there is an error.
		my $cmd = "copy $src $dst > nul";
#		print STDERR "$cmd\n";
		system $cmd;
		
		MakeFileReadOnly( $dst );
	}
}

close TXTFILE;
