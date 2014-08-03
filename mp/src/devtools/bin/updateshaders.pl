use String::CRC32;
BEGIN {use File::Basename; push @INC, dirname($0); }
require "valve_perl_helpers.pl";

$dynamic_compile = defined $ENV{"dynamic_shaders"} && $ENV{"dynamic_shaders"} != 0;

$depnum = 0;
$baseSourceDir = ".";

my %dep;

sub GetAsmShaderDependencies_R
{
	local( $shadername ) = shift;
	local( *SHADER );
	
	open SHADER, "<$shadername";
	while( <SHADER> )
	{
		if( m/^\s*\#\s*include\s+\"(.*)\"/ )
		{
			# make sure it isn't in there already.
			if( !defined( $dep{$1} ) )
			{
				$dep{$1} = 1;
				GetAsmShaderDependencies_R( $1 );
			}
		}
	}
	close SHADER;
}

sub GetAsmShaderDependencies
{
	local( $shadername ) = shift;
	undef %dep;
	GetAsmShaderDependencies_R( $shadername );
#	local( $i );
#	foreach $i ( keys( %dep ) )
#	{
#		print "$shadername depends on $i\n";
#	}
	return keys( %dep );
}

sub GetShaderType
{
	my $shadername = shift;
	my $shadertype;
	if( $shadername =~ m/\.vsh/i )
	{
		$shadertype = "vsh";
	}
	elsif( $shadername =~ m/\.psh/i )
	{
		$shadertype = "psh";
	}
	elsif( $shadername =~ m/\.fxc/i )
	{
		$shadertype = "fxc";
	}
	else
	{
		die;
	}
	return $shadertype;
}

sub GetShaderSrc
{
	my $shadername = shift;
	if ( $shadername =~ m/^(.*)-----/i )
	{
		return $1;
	}
	else
	{
		return $shadername;
	}
}

sub GetShaderBase
{
	my $shadername = shift;
	if ( $shadername =~ m/-----(.*)$/i )
	{
		return $1;
	}
	else
	{
		my $shadertype = &GetShaderType( $shadername );
		$shadername =~ s/\.$shadertype//i;
		return $shadername;
	}
}

sub DoAsmShader
{
	my $argstring = shift;
	my $shadername = &GetShaderSrc( $argstring );
	my $shaderbase = &GetShaderBase( $argstring );
	my $shadertype = &GetShaderType( $argstring );
	my $incfile = "";
	if( $shadertype eq "fxc" || $shadertype eq "vsh" )
	{
		$incfile = $shadertype . "tmp9" . $g_tmpfolder . "\\$shaderbase.inc ";
	}

	my $vcsfile = $shaderbase . $g_vcsext;
	my $bWillCompileVcs = 1;
	if( ( $shadertype eq "fxc") && $dynamic_compile )
	{
		$bWillCompileVcs = 0;
	}
	if( $shadercrcpass{$argstring} )
	{
		$bWillCompileVcs = 0;
	}

	if( $bWillCompileVcs )
	{
		&output_makefile_line( $incfile . "shaders\\$shadertype\\$vcsfile: $shadername @dep\n") ;
	}
	else
	{
		# psh files don't need a rule at this point since they don't have inc files and we aren't compiling a vcs.
		if( $shadertype eq "fxc" || $shadertype eq "vsh" )
		{
			&output_makefile_line( $incfile . ":  $shadername @dep\n") ;
		}
	}
	

	my $x360switch = "";
	my $moreswitches = "";
	if( !$bWillCompileVcs && $shadertype eq "fxc" )
	{
		$moreswitches .= "-novcs ";
	}
	if( $g_x360 )
	{
		$x360switch = "-x360";
		
		if( $bWillCompileVcs && ( $shaderbase =~ m/_ps20$/i ) )
		{
			$moreswitches .= "-novcs ";
			$bWillCompileVcs = 0;
		}
	}

	# if we are psh and we are compiling the vcs, we don't need this rule.
	if( !( $shadertype eq "psh" && !$bWillCompileVcs ) )
	{
		&output_makefile_line( "\tperl $g_SourceDir\\devtools\\bin\\" . $shadertype . "_prep.pl $moreswitches $x360switch -source \"$g_SourceDir\" $argstring\n") ;
	}

	if( $bWillCompileVcs )
	{
		&output_makefile_line( "\techo $shadername>> filestocopy.txt\n") ;
		my $dep;
		foreach $dep( @dep )
		{
			&output_makefile_line( "\techo $dep>> filestocopy.txt\n") ;
		}
	}
	&output_makefile_line( "\n") ;
}

if( scalar( @ARGV ) == 0 )
{
	die "Usage updateshaders.pl shaderprojectbasename\n\tie: updateshaders.pl stdshaders_dx6\n";
}

$g_x360			= 0;
$g_tmpfolder	= "_tmp";
$g_vcsext		= ".vcs";

while( 1 )
{
	$inputbase = shift;

	if( $inputbase =~ m/-source/ )
	{
		$g_SourceDir = shift;
	}
	elsif( $inputbase =~ m/-x360/ )
	{
		$g_x360 = 1;
		$g_tmpfolder = "_360_tmp";
		$g_vcsext = ".360.vcs";
	}
	elsif( $inputbase =~ m/-execute/ )
	{
		$g_execute = 1;
	}
	elsif( $inputbase =~ m/-nv3x/ )
	{
		$nv3x = 1;
	}
	else
	{
		last;
	}
}

my @srcfiles = &LoadShaderListFile( $inputbase );

open MAKEFILE, ">makefile\.$inputbase";
open COPYFILE, ">makefile\.$inputbase\.copy";
open INCLIST, ">inclist.txt";
open VCSLIST, ">vcslist.txt";

# make a default dependency that depends on all of the shaders.
&output_makefile_line( "default: ") ;
foreach $shader ( @srcfiles )
{
	my $shadertype = &GetShaderType( $shader );
	my $shaderbase = &GetShaderBase( $shader );
	my $shadersrc = &GetShaderSrc( $shader );
	if( $shadertype eq "fxc" || $shadertype eq "vsh" )
	{
		# We only generate inc files for fxc and vsh files.
		my $incFileName = "$shadertype" . "tmp9" . $g_tmpfolder . "\\" . $shaderbase . "\.inc";
		&output_makefile_line( " $incFileName" );
		&output_inclist_line( "$incFileName\n" );  
	}

	my $vcsfile = $shaderbase . $g_vcsext;

	my $compilevcs = 1;
	if( $shadertype eq "fxc" && $dynamic_compile )
	{
		$compilevcs = 0;
	}
	if( $g_x360 && ( $shaderbase =~ m/_ps20$/i ) )
	{
		$compilevcs = 0;
	}
	if( $compilevcs )
	{
		my $vcsFileName = "..\\..\\..\\game\\hl2\\shaders\\$shadertype\\$shaderbase" . $g_vcsext;
		# We want to check for perforce operations even if the crc matches in the event that a file has been manually reverted and needs to be checked out again.
		&output_vcslist_line( "$vcsFileName\n" );  
		$shadercrcpass{$shader} = &CheckCRCAgainstTarget( $shadersrc, $vcsFileName, 0 );
		if( $shadercrcpass{$shader} )
		{
			$compilevcs = 0;
		}
	}
	if( $compilevcs )
	{
		&output_makefile_line( " shaders\\$shadertype\\$vcsfile" );
		# emit a list of vcs files to copy to the target since we want to build them.
		&output_copyfile_line( GetShaderSrc($shader) . "-----" . GetShaderBase($shader) . "\n" );
	}
}
&output_makefile_line( "\n\n") ;

# Insert all of our vertex shaders and depencencies
$lastshader = "";
foreach $shader ( @srcfiles )
{
	my $currentshader = &GetShaderSrc( $shader );
	if ( $lastshader ne $currentshader )
	{
		$lastshader = $currentshader;
		@dep = &GetAsmShaderDependencies( $lastshader );
	}
	&DoAsmShader( $shader );
}
close VCSLIST;
close INCLIST;
close COPYFILE;
close MAKEFILE;

# nuke the copyfile if it is zero length
if( ( stat "makefile\.$inputbase\.copy" )[7] == 0 )
{
	unlink "makefile\.$inputbase\.copy";
}

sub output_makefile_line
{
	local ($_)=@_;
	print MAKEFILE $_;
}

sub output_copyfile_line
{
	local ($_)=@_;
	print COPYFILE $_;
}

sub output_vcslist_line
{
	local ($_)=@_;
	print VCSLIST $_;
}

sub output_inclist_line
{
	local ($_)=@_;
	print INCLIST $_;
}

