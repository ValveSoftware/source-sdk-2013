use String::CRC32;
BEGIN {use File::Basename; push @INC, dirname($0); }
require "valve_perl_helpers.pl";

sub WriteHelperVar
{
	local( $name ) = shift;
	local( $min ) = shift;
	local( $max ) = shift;
	local( $varname ) = "m_n" . $name;
	local( $boolname ) = "m_b" . $name;
	push @outputHeader, "private:\n";
	push @outputHeader, "\tint $varname;\n";
	push @outputHeader, "#ifdef _DEBUG\n";
	push @outputHeader, "\tbool $boolname;\n";
	push @outputHeader, "#endif\n";
	push @outputHeader, "public:\n";
	# int version of set function
	push @outputHeader, "\tvoid Set" . $name . "( int i )\n";
	push @outputHeader, "\t{\n";
	if ( $min != $max )
	{
		push @outputHeader, "\t\tAssert( i >= $min && i <= $max );\n";
		push @outputHeader, "\t\t$varname = i;\n";
		push @outputHeader, "#ifdef _DEBUG\n";
		push @outputHeader, "\t\t$boolname = true;\n";
		push @outputHeader, "#endif\n";
	}
	push @outputHeader, "\t}\n";
	# bool version of set function
	push @outputHeader, "\tvoid Set" . $name . "( bool i )\n";
	push @outputHeader, "\t{\n";
	if ( $min != $max )
	{
#		push @outputHeader, "\t\tAssert( i >= $min && i <= $max );\n";
		push @outputHeader, "\t\t$varname = i ? 1 : 0;\n";
		push @outputHeader, "#ifdef _DEBUG\n";
		push @outputHeader, "\t\t$boolname = true;\n";
		push @outputHeader, "#endif\n";
	}
	push @outputHeader, "\t}\n";
}

sub WriteStaticBoolExpression
{
	local( $prefix ) = shift;
	local( $operator ) = shift;
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		if( $i )
		{
			push @outputHeader, " $operator ";
		}
		local( $name ) = @staticDefineNames[$i];
		local( $boolname ) = "m_b" . $name;
		push @outputHeader, "$prefix$boolname";
	}
	push @outputHeader, ";\n";
}

sub WriteDynamicBoolExpression
{
	local( $prefix ) = shift;
	local( $operator ) = shift;
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		if( $i )
		{
			push @outputHeader, " $operator ";
		}
		local( $name ) = @dynamicDefineNames[$i];
		local( $boolname ) = "m_b" . $name;
		push @outputHeader, "$prefix$boolname";
	}
	push @outputHeader, ";\n";
}

sub WriteDynamicHelperClasses
{
	local( $basename ) = $fxc_filename;
	$basename =~ s/\.fxc//i;
	$basename =~ tr/A-Z/a-z/;
	local( $classname ) = $basename . "_Dynamic_Index";
	push @outputHeader, "class $classname\n";
	push @outputHeader, "{\n";
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		$name = $dynamicDefineNames[$i];
		$min = $dynamicDefineMin[$i];
		$max = $dynamicDefineMax[$i];
		&WriteHelperVar( $name, $min, $max );
	}
	push @outputHeader, "public:\n";
	push @outputHeader, "\t$classname()\n";
	push @outputHeader, "\t{\n";
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		$min = $dynamicDefineMin[$i];
		$max = $dynamicDefineMax[$i];

		local( $name ) = @dynamicDefineNames[$i];
		local( $boolname ) = "m_b" . $name;
		local( $varname ) = "m_n" . $name;
		push @outputHeader, "#ifdef _DEBUG\n";
		if ( $min != $max )
		{
			push @outputHeader, "\t\t$boolname = false;\n";
		}
		else
		{
			push @outputHeader, "\t\t$boolname = true;\n";
		}
		push @outputHeader, "#endif // _DEBUG\n";
		push @outputHeader, "\t\t$varname = 0;\n";
	}
	push @outputHeader, "\t}\n";
	push @outputHeader, "\tint GetIndex()\n";
	push @outputHeader, "\t{\n";
	push @outputHeader, "\t\t// Asserts to make sure that we aren't using any skipped combinations.\n";
	foreach $skip (@perlskipcodeindividual)
	{
		$skip =~ s/\$/m_n/g;
#		push @outputHeader, "\t\tAssert( !( $skip ) );\n";
	}
	push @outputHeader, "\t\t// Asserts to make sure that we are setting all of the combination vars.\n";

	push @outputHeader, "#ifdef _DEBUG\n";
	if( scalar( @dynamicDefineNames ) > 0 )
	{
		push @outputHeader, "\t\tbool bAllDynamicVarsDefined = ";
		WriteDynamicBoolExpression( "", "&&" );
	}
	if( scalar( @dynamicDefineNames ) > 0 )
	{
		push @outputHeader, "\t\tAssert( bAllDynamicVarsDefined );\n";
	}
	push @outputHeader, "#endif // _DEBUG\n";

	if( $spewCombos && scalar( @dynamicDefineNames ) )
	{
		push @outputHeader, &CreateCCodeToSpewDynamicCombo();
	}
	push @outputHeader, "\t\treturn ";
	local( $scale ) = 1;
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		local( $name ) = @dynamicDefineNames[$i];
		local( $varname ) = "m_n" . $name;
		push @outputHeader, "( $scale * $varname ) + ";
		$scale *= $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1;
	}
	push @outputHeader, "0;\n";
	push @outputHeader, "\t}\n";
	push @outputHeader, "};\n";
}

sub WriteStaticHelperClasses
{
	local( $basename ) = $fxc_filename;
	$basename =~ s/\.fxc//i;
	$basename =~ tr/A-Z/a-z/;
	local( $classname ) = $basename . "_Static_Index";
	push @outputHeader, "class $classname\n";
	push @outputHeader, "{\n";
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		$name = $staticDefineNames[$i];
		$min = $staticDefineMin[$i];
		$max = $staticDefineMax[$i];
		&WriteHelperVar( $name, $min, $max );
	}
	push @outputHeader, "public:\n";
	push @outputHeader, "\t$classname()\n";
	push @outputHeader, "\t{\n";
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		$min = $staticDefineMin[$i];
		$max = $staticDefineMax[$i];

		local( $name ) = @staticDefineNames[$i];
		local( $boolname ) = "m_b" . $name;
		local( $varname ) = "m_n" . $name;
		
		push @outputHeader, "#ifdef _DEBUG\n";
		if ( $min != $max )
		{
			push @outputHeader, "\t\t$boolname = false;\n";
		}
		else
		{
			push @outputHeader, "\t\t$boolname = true;\n";
		}
		push @outputHeader, "#endif // _DEBUG\n";
		push @outputHeader, "\t\t$varname = 0;\n";
	}
	push @outputHeader, "\t}\n";
	push @outputHeader, "\tint GetIndex()\n";
	push @outputHeader, "\t{\n";
	push @outputHeader, "\t\t// Asserts to make sure that we aren't using any skipped combinations.\n";
	foreach $skip (@perlskipcodeindividual)
	{
		$skip =~ s/\$/m_n/g;
#		push @outputHeader, "\t\tAssert( !( $skip ) );\n";
	}
	push @outputHeader, "\t\t// Asserts to make sure that we are setting all of the combination vars.\n";

	push @outputHeader, "#ifdef _DEBUG\n";
	if( scalar( @staticDefineNames ) > 0 )
	{
		push @outputHeader, "\t\tbool bAllStaticVarsDefined = ";
		WriteStaticBoolExpression( "", "&&" );

	}
	if( scalar( @staticDefineNames ) > 0 )
	{
		push @outputHeader, "\t\tAssert( bAllStaticVarsDefined );\n";
	}
	push @outputHeader, "#endif // _DEBUG\n";

	if( $spewCombos && scalar( @staticDefineNames ) )
	{
		push @outputHeader, &CreateCCodeToSpewStaticCombo();
	}
	push @outputHeader, "\t\treturn ";
	local( $scale ) = 1;
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		$scale *= $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1;
	}
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		local( $name ) = @staticDefineNames[$i];
		local( $varname ) = "m_n" . $name;
		push @outputHeader, "( $scale * $varname ) + ";
		$scale *= $staticDefineMax[$i] - $staticDefineMin[$i] + 1;
	}
	push @outputHeader, "0;\n";
	push @outputHeader, "\t}\n";
	push @outputHeader, "};\n";
}

sub CreateFuncToSetPerlVars
{
	local( $out ) = "";

	$out .= "sub SetPerlVarsFunc\n";
	$out .= "{\n";
	$out .= "	local( \$combo ) = shift;\n";
	$out .= "	local( \$i );\n";
	local( $i );
	for( $i = 0; $i < scalar( @dynamicDefineNames ); \$i++ )
	{
		$out .= "	\$$dynamicDefineNames[$i] = \$combo % ";
		$out .= ( $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1 ) + $dynamicDefineMin[$i];
		$out .= ";\n";
		$out .= "	\$combo = \$combo / " . ( $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1 ) . ";\n";
	}
	for( $i = 0; $i < scalar( @staticDefineNames ); \$i++ )
	{
		$out .= "	\$$staticDefineNames[$i] = \$combo % ";
		$out .= ( $staticDefineMax[$i] - $staticDefineMin[$i] + 1 ) + $staticDefineMin[$i];
		$out .= ";\n";
		$out .= "	\$combo = \$combo / " . ( $staticDefineMax[$i] - $staticDefineMin[$i] + 1 ) . ";\n";
	}
	$out .= "}\n";

#	print $out;
	eval $out;
}

# These sections can be interchanged to enable profiling.
#$ShowTimers = 1;
#use Time::HiRes;
#sub SampleTime()
#{
#	return Time::HiRes::time();
#}

$ShowTimers = 0;
sub SampleTime() { return 0; }

$total_start_time = SampleTime();

# NOTE: These must match the same values in macros.vsh!
$vPos				= "v0";
$vBoneWeights			= "v1";
$vBoneIndices			= "v2";
$vNormal			= "v3";
if( $g_x360 )
{
	$vPosFlex		= "v4";
	$vNormalFlex		= "v13";
}
$vColor				= "v5";
$vSpecular			= "v6";
$vTexCoord0			= "v7";
$vTexCoord1			= "v8";
$vTexCoord2			= "v9";
$vTexCoord3			= "v10";
$vTangentS			= "v11";
$vTangentT			= "v12";
$vUserData			= "v14";

sub ReadInputFileWithLineInfo
{
	local( $base_filename ) = shift;

	local( *INPUT );
	local( @output );

	# Look in the stdshaders directory, followed by the current directory.
	# (This is for the SDK, since some of its files are under stdshaders).
	local( $filename ) = $base_filename;
	if ( !-e $filename )
	{
		$filename = "$g_SourceDir\\materialsystem\\stdshaders\\$base_filename";
		if ( !-e $filename )
		{
			die "\nvsh_prep.pl ERROR: missing include file: $filename.\n\n";
		}
	}

	open INPUT, "<$filename" || die;

	local( $line );
	local( $linenum ) = 1;
	while( $line = <INPUT> )
	{
		$line =~ s/\n//g;
		local( $postfix ) = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
		$postfix .= "; LINEINFO($filename)($linenum)\n";
		if( $line =~ m/\#include\s+\"(.*)\"/i )
		{
			push @output, &ReadInputFileWithLineInfo( $1 );
		}
		else
		{
			push @output, $line . $postfix;
		}
		$linenum++;
	}

	close INPUT;
	return @output;
}

sub ReadInputFileWithoutLineInfo
{
	local( $base_filename ) = shift;

	local( *INPUT );
	local( @output );

	# Look in the stdshaders directory, followed by the current directory.
	# (This is for the SDK, since some of its files are under stdshaders).
	local( $filename ) = $base_filename;
	if ( !-e $filename )
	{
		$filename = "$g_SourceDir\\materialsystem\\stdshaders\\$base_filename";
		if ( !-e $filename )
		{
			die "\nERROR: missing include file: $filename.\n\n";
		}
	}

	open INPUT, "<$filename" || die;

	local( $line );
	while( $line = <INPUT> )
	{
		if( $line =~ m/\#include\s+\"(.*)\"/i )
		{
			push @output, &ReadInputFileWithoutLineInfo( $1 );
		}
		else
		{
			push @output, $line;
		}
	}

	close INPUT;
	return @output;
}

sub IsPerl
{
	local( $line ) = shift;
	if( $line =~ m/^\s*sub.*\,/ )
	{
		return 0;
	}
	if( $line =~ m/^\#include/	||
		$line =~ m/^\#define/	||
		$line =~ m/^\#undef/	||
		$line =~ m/^\#ifdef/	||
		$line =~ m/^\#ifndef/	||
		$line =~ m/^\#else/		||
		$line =~ m/^\#endif/	||
		$line =~ m/^\#error/
		)
	{
		return 0;
	}
	if( $line =~ m/^\s*if\s*\(/		||
		$line =~ m/^\s*else/		||
		$line =~ m/^\s*elsif/		||
		$line =~ m/^\s*for\s*\(/	||
		$line =~ m/^\s*\{/			||
		$line =~ m/^sub\s*/		||
		$line =~ m/^\s*\}/			||
		$line =~ m/^\s*\&/			||
		$line =~ m/^\s*\#/			||
		$line =~ m/^\s*\$/			||
		$line =~ m/^\s*print/			||
		$line =~ m/^\s*return/			||
		$line =~ m/^\s*exit/			||
		$line =~ m/^\s*die/			||
		$line =~ m/^\s*eval/			||
		$line =~ m/^\s*local/		 ||
		$line =~ m/^\s*my\s+/		 ||
		$line =~ m/^\s*@/		 ||
		$line =~ m/^\s*alloc\s+/		||
		$line =~ m/^\s*free\s+/		
		)
	{
		return 1;
	}
	return 0;
}

# translate the output into something that takes us back to the source line
# that we care about in msdev
sub TranslateErrorMessages
{
	local( $origline );
	while( $origline = shift )
	{
		if( $origline =~ m/(.*)\((\d+)\)\s*:\s*(.*)$/i )
		{
			local( $filename ) = $1;
			local( $linenum ) = $2;
			local( $error ) = $3;
			local( *FILE );
			open FILE, "<$filename" || die;
			local( $i );
			local( $line );
			for( $i = 1; $i < $linenum; $i++ )
			{
				$line = <FILE>;
			}
			if( $line =~ m/LINEINFO\((.*)\)\((.*)\)/ )
			{
				print "$1\($2\) : $error\n";
				my $num = $linenum - 1;
				print "$filename\($num\) : original error location\n";
			}
			close FILE;
		}
		else
		{
			$origline =~ s/successful compile\!.*//gi;
			if( !( $origline =~ m/^\s*$/ ) )
			{
#				print "WTF: $origline\n";
			}
		}
	}
}


sub CountInstructions
{
	local( $line );
	local( $count ) = 0;
	while( $line = shift )
	{
		# get rid of comments
		$line =~ s/;.*//gi;
		$line =~ s/\/\/.*//gi;
		# skip the vs1.1 statement
		$line =~ s/^\s*vs.*//gi;
		# if there's any text left, it's an instruction
		if( $line =~ /\S/gi )
		{
			$count++;
		}
	}
	return $count;
}


%compiled = ();

sub UsesRegister
{
	my $registerName = shift;
	my $str = shift;

	# Cache a compiled RE for each register name. This makes UsesRegister about 2.5x faster.
	if ( !$compiled{$registerName} )
	{
		$compiled{$registerName} = qr/\b$registerName\b/;
	}

	$ret = 0;
	if( $str =~ /$compiled{$registerName}/gi )
	{
		$ret = 1;
	}

	return $ret;
}

sub PadString
{
	local( $str ) = shift;
	local( $desiredLen ) = shift;
	local( $len ) = length $str;
	while( $len < $desiredLen )
	{
		$str .= " ";
		$len++;
	}
	return $str;
}

sub FixupAllocateFree
{
	local( $line ) = shift;
	$line =~ s/\&AllocateRegister\s*\(\s*\\(\S+)\s*\)/&AllocateRegister( \\$1, \"\\$1\" )/g;
	$line =~ s/\&FreeRegister\s*\(\s*\\(\S+)\s*\)/&FreeRegister( \\$1, \"\\$1\" )/g;
	$line =~ s/alloc\s+(\S+)\s*/local( $1 ); &AllocateRegister( \\$1, \"\\$1\" );/g;
	$line =~ s/free\s+(\S+)\s*/&FreeRegister( \\$1, \"\\$1\" );/g;
	return $line;
}

sub TranslateDXKeywords
{
	local( $line ) = shift;
	$line =~ s/\bENDIF\b/endif/g;
	$line =~ s/\bIF\b/if/g;
	$line =~ s/\bELSE\b/else/g;

	return $line;
}

# This is used to make the generated pl files all pretty.
sub GetLeadingWhiteSpace
{
	local( $str ) = shift;
	if( $str =~ m/^;\S/ || $str =~ m/^; \S/ )
	{
		return "";
	}
	$str =~ s/^;/ /g; # count a leading ";" as whitespace as far as this is concerned.
	$str =~ m/^(\s*)/;
	return $1;
}

$g_dx9 = 1;
$g_SourceDir = "..\\..";

while( 1 )
{
	$filename = shift;

	if ( $filename =~ m/-source/i )
	{
		$g_SourceDir = shift;
	}
	elsif( $filename =~ m/-x360/i )
	{
		$g_x360 = 1;
	}
	else
	{
		last;
	}
}

$filename =~ s/-----.*$//;


#
# Get the shader binary version number from a header file.
#
open FILE, "<$g_SourceDir\\public\\materialsystem\\shader_vcs_version.h" || die;
while( $line = <FILE> )
{
	if( $line =~ m/^\#define\s+SHADER_VCS_VERSION_NUMBER\s+(\d+)\s*$/ )
	{
		$shaderVersion = $1;
		last;
	}
}
if( !defined $shaderVersion )
{
	die "couldn't get shader version from shader_vcs_version.h";
}
close FILE;


if( $g_x360 )
{
	$vshtmp = "vshtmp9_360_tmp";
}
else
{
	$vshtmp = "vshtmp9_tmp";
}

if( !stat $vshtmp )
{
	mkdir $vshtmp, 0777 || die $!;
}

# suck in all files, including $include files.
@input = &ReadInputFileWithLineInfo( $filename );

sub CalcNumCombos
{
	local( $i, $numCombos );
	$numCombos = 1;
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		$numCombos *= $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1;
	}
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		$numCombos *= $staticDefineMax[$i] - $staticDefineMin[$i] + 1;
	}
	return $numCombos;
}

sub CalcNumDynamicCombos
{
	local( $i, $numCombos );
	$numCombos = 1;
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		$numCombos *= $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1;
	}
	return $numCombos;
}

# READ THE TOP OF THE FILE TO FIND SHADER COMBOS
foreach $_ ( @input )
{
	next if( m/^\s*$/ );
#	last if( !m,^//, );
	s,^//\s*,,;
	if( m/\s*STATIC\s*\:\s*\"(.*)\"\s+\"(\d+)\.\.(\d+)\"/ )
	{
		local( $name, $min, $max );
		$name = $1;
		$min = $2;
		$max = $3;
#		print "\"$name\" \"$min..$max\"\n";
		if (/\[(.*)\]/)
		{
			$platforms=$1;
			next if ( ($g_x360) && (!($platforms=~/XBOX/i)) );
			next if ( (!$g_x360) && (!($platforms=~/PC/i)) );
		}
		push @staticDefineNames, $name;
		push @staticDefineMin, $min;
		push @staticDefineMax, $max;
	}
	elsif( m/\s*DYNAMIC\s*\:\s*\"(.*)\"\s+\"(\d+)\.\.(\d+)\"/ )
	{
		local( $name, $min, $max );
		$name = $1;
		$min = $2;
		$max = $3;
		if (/\[(.*)\]/)
		{
			$platforms=$1;
			next if ( ($g_x360) && (!($platforms=~/XBOX/i)) );
			next if ( (!$g_x360) && (!($platforms=~/PC/i)) );
		}
#		print "\"$name\" \"$min..$max\"\n";
		push @dynamicDefineNames, $name;
		push @dynamicDefineMin, $min;
		push @dynamicDefineMax, $max;
	}
}

# READ THE WHOLE FILE AND FIND SKIP STATEMENTS
foreach $_ ( @input )
{
	if( m/^\s*\#\s*SKIP\s*\:\s*(.*\S+)\s*\; LINEINFO.*$/ )
	{
		$perlskipcode .= "(" . $1 . ")||";
		push @perlskipcodeindividual, $1;
	}	
}
if( defined $perlskipcode )
{
	$perlskipcode .= "0";
	$perlskipcode =~ s/\n//g;
}
else
{
	$perlskipcode = "0";
}

#print $perlskipcode . "\n";


# Translate the input into a perl program that'll unroll everything and
# substitute variables.
while( $inputLine = shift @input )
{
	$inputLine =~ s/\n//g;
	# leave out lines that are only whitespace.
	if( $inputLine =~ m/^\s*; LINEINFO.*$/ )
	{
		next;
	}
	local( $inputLineNoLineNum ) = $inputLine;
	$inputLineNoLineNum =~ s/; LINEINFO.*//gi;
	if( &IsPerl( $inputLineNoLineNum ) )
	{
		$inputLineNoLineNum = &FixupAllocateFree( $inputLineNoLineNum );
		push @outputProgram, $inputLineNoLineNum . "\n";
	}
	else
	{
		# make asm lines that have quotes in them not barf.
		$inputLine =~ s/\"/\\\"/g;
		$inputLine = &TranslateDXKeywords( $inputLine );
		push @outputProgram, &GetLeadingWhiteSpace( $inputLine ) . "push \@output, \"" . 
			$inputLine . "\\n\";\n";
	}
}

$outputProgram = join "", @outputProgram;

$filename_base = $filename;
$filename_base =~ s/\.vsh//i;

open DEBUGOUT, ">$vshtmp" . "/$filename_base.pl" || die;
print DEBUGOUT $outputProgram;
close DEBUGOUT;

# Make a function called OutputProgram()
$bigProg = "sub OutputProgram { " . $outputProgram . "}";
eval( $bigProg );


#print $outputProgram;

#push @finalheader, "// hack to force dependency checking\n";
#push @finalheader, "\#ifdef NEVER\n";
#push @finalheader, "\#include \"" . $filename_base . "\.vsh\"\n";
#push @finalheader, "\#include \"..\\..\\devtools\\bin\\vsh_prep.pl\"\n";
#push @finalheader, "\#endif\n";

%g_TimingBlocks = ();
$main_start_time = SampleTime();

$numCombos = &CalcNumCombos();
$numDynamicCombos = &CalcNumDynamicCombos();
#print "$numCombos total combos\n";
#print "$numDynamicCombos dynamic combos\n";
#print $numCombos / $numDynamicCombos . " static combos\n";

# Write out the C++ helper class for picking shader combos
$fxc_filename = $filename_base;
&WriteStaticHelperClasses();
&WriteDynamicHelperClasses();

# Create a subroutine out of $perlskipcode
$perlskipfunc = "sub SkipCombo { return $perlskipcode; }\n";
#print $perlskipfunc;

eval $perlskipfunc;
&CreateFuncToSetPerlVars();

my $incfilename = "$vshtmp/$filename_base" . ".inc";

# Write the inc file that has indexing helpers, etc.
&WriteFile( $incfilename, join( "", @outputHeader ) );


# Run the output program for all the combinations of bones and lights.
print "$filename_base.vsh\n";
for( $i = 0; $i < $numCombos; $i++ )
{
#	print "combo $i\n";
	&SetPerlVarsFunc( $i );
	local( $compileFailed );
	$ret = &SkipCombo;
	if( !defined $ret )
	{
		die "$@\n";
	}
	if( $ret )
	{
		# skip this combo!
		$compileFailed = 1;
		$numSkipped++;
		next;
	}

	$start = SampleTime();

	$g_usesPos				= 0;
	$g_usesPosFlex			= 0;
	$g_usesBoneWeights		= 0;
	$g_usesBoneIndices		= 0;
	$g_usesNormal			= 0;
	$g_usesNormalFlex		= 0;
	$g_usesColor			= 0;
	$g_usesSpecular			= 0;
	$g_usesTexCoord0		= 0;
	$g_usesTexCoord1		= 0;
	$g_usesTexCoord2		= 0;
	$g_usesTexCoord3		= 0;
	$g_usesTangentS			= 0;
	$g_usesTangentT			= 0;
	$g_usesUserData			= 0;

	undef @output;

	$g_TimingBlocks{"inner1"} += SampleTime() - $start;
			
	$eval_start_time = SampleTime();
	&OutputProgram();
	$eval_total_time += (SampleTime() - $eval_start_time);

	$start = SampleTime();
			
	# Strip out comments once so we don't have to do it in all the UsesRegister calls.
	@stripped = @output;
	map 
	{ 
		$_ =~ s/;.*//gi;
		$_ =~ s/\/\/.*//gi;
	} @stripped;
	my $strippedStr = join( "", @stripped );

	$g_TimingBlocks{"inner2"} += SampleTime() - $start;
			
	$start = SampleTime();

	# Have to make another pass through after we know which v registers are used. . yuck.
	$g_usesPos				= &UsesRegister( $vPos, $strippedStr );
	if( $g_x360 )
	{
		$g_usesPosFlex		= &UsesRegister( $vPosFlex, $strippedStr );
		$g_usesNormalFlex	= &UsesRegister( $vNormalFlex, $strippedStr );
	}
	$g_usesBoneWeights		= &UsesRegister( $vBoneWeights, $strippedStr );
	$g_usesBoneIndices		= &UsesRegister( $vBoneIndices, $strippedStr );
	$g_usesNormal			= &UsesRegister( $vNormal, $strippedStr );
	$g_usesColor			= &UsesRegister( $vColor, $strippedStr );
	$g_usesSpecular			= &UsesRegister( $vSpecular, $strippedStr );
	$g_usesTexCoord0		= &UsesRegister( $vTexCoord0, $strippedStr );
	$g_usesTexCoord1		= &UsesRegister( $vTexCoord1, $strippedStr );
	$g_usesTexCoord2		= &UsesRegister( $vTexCoord2, $strippedStr );
	$g_usesTexCoord3		= &UsesRegister( $vTexCoord3, $strippedStr );
	$g_usesTangentS			= &UsesRegister( $vTangentS, $strippedStr );
	$g_usesTangentT			= &UsesRegister( $vTangentT, $strippedStr );
	$g_usesUserData			= &UsesRegister( $vUserData, $strippedStr );
	undef @output;

	$g_TimingBlocks{"inner2"} += SampleTime() - $start;

	$eval_start_time = SampleTime();
	# Running OutputProgram generates $outfilename
	&OutputProgram();
	$eval_total_time += (SampleTime() - $eval_start_time);
		
	$start = SampleTime();

	&CheckUnfreedRegisters();

	for( $j = 0; $j < scalar( @output ); $j++ )
	{
		# remove whitespace from the beginning of each line.
		$output[$j] =~ s/^\s+//;
		# remove LINEINFO from empty lines.
		$output[$j] =~ s/^; LINEINFO.*//;
	}

	$g_TimingBlocks{"inner3"} += SampleTime() - $start;
	$start = SampleTime();


	$outfilename_base = $filename_base . "_" . $i;
			
	# $outfilename is the name of the file generated from executing the perl code
	# for this shader.  This file is generated once per combo.
	# We will assemble this shader with vsa.exe.
	$outfilename = "$vshtmp\\" . $outfilename_base . ".tmp";

#	$outhdrfilename = "$vshtmp\\" . $outfilename_base . ".h";
#	unlink $outhdrfilename;

	open OUTPUT, ">$outfilename" || die;
	print OUTPUT @output;
	close OUTPUT;
	
	$g_TimingBlocks{"inner4"} += SampleTime() - $start;
	$start = SampleTime();

	local( $instructionCount ) = &CountInstructions( @output );
	$g_TimingBlocks{"inner5"} += SampleTime() - $start;

	local( $debug );

	$debug = 1;
#	for( $debug = 1; $debug >= 0; $debug-- )
	{
		# assemble the vertex shader
		unlink "shader$i.o";
		if( $g_x360 )
		{
			$vsa = "..\\..\\x360xdk\\bin\\win32\\vsa";
		}
		else
		{
			$vsa = "..\\..\\dx9sdk\\utilities\\vsa";
		}
		$vsadebug = "$vsa /nologo /Foshader$i.o $outfilename";
		$vsanodebug = "$vsa /nologo /Foshader$i.o $outfilename";

		$vsa_start_time = SampleTime();

		if( $debug )
		{
#			print $vsadebug . "\n";
			@vsaoutput = `$vsadebug 2>&1`;
#			print @vsaoutput;
		}
		else
		{
			@vsaoutput = `$vsanodebug 2>&1`;
		}

		$vsa_total_time += SampleTime() - $vsa_start_time;
		
		$start = SampleTime();

		&TranslateErrorMessages( @vsaoutput );

		$g_TimingBlocks{"inner6"} += SampleTime() - $start;
		
		push @finalheader, @hdr;
	}
}


$main_total_time = SampleTime() - $main_start_time;

# stick info about the shaders at the end of the inc file.
push @finalheader, "static PrecompiledShaderByteCode_t $filename_base" . "_vertex_shaders[] = {\n";
for( $i = 0; $i < $numCombos; $i++ )
{
	$outfilename_base = $filename_base . "_" . $i;
	push @finalheader, "{ $outfilename_base, sizeof( $outfilename_base ) },\n";
}
push @finalheader, "};\n";


push @finalheader, "struct $filename_base" . "_VertexShader_t : public PrecompiledShader_t\n";
push @finalheader, "{\n";
push @finalheader, "\t$filename_base" . "_VertexShader_t()\n";
push @finalheader, "\t{\n";
push @finalheader, "\t\tm_nFlags = 0;\n";

$flags = 0;
#push @finalheader, "\t\tppVertexShaders = $filename_base" . "_vertex_shaders;\n";
push @finalheader, "\t\tm_pByteCode = $filename_base" . "_vertex_shaders;\n";
push @finalheader, "\t\tm_pName = \"$filename_base\";\n";
push @finalheader, "\t\tm_nShaderCount = " . ( $maxNumBones + 1 ) * $totalFogCombos * $totalLightCombos . ";\n";
push @finalheader, "\t\tm_nDynamicCombos = m_nShaderCount;\n";
push @finalheader, "\t\tGetShaderDLL()->InsertPrecompiledShader( PRECOMPILED_VERTEX_SHADER, this );\n";
push @finalheader, "\t}\n";
push @finalheader, "\tvirtual const PrecompiledShaderByteCode_t &GetByteCode( int shaderID )\n";
push @finalheader, "\t{\n";
push @finalheader, "\t\treturn m_pByteCode[shaderID];\n";
push @finalheader, "\t}\n";
push @finalheader, "};\n";
push @finalheader, "static $filename_base" . "_VertexShader_t $filename_base" . "_VertexShaderInstance;\n";

# Write the final header file with the compiled vertex shader programs.
$finalheadername = "$vshtmp\\" . $filename_base . ".inc";
#print "writing $finalheadername\n";
#open FINALHEADER, ">$finalheadername" || die;
#print FINALHEADER @finalheader;
#close FINALHEADER;

&MakeDirHier( "shaders/vsh" );

my $vcsName = "";
if( $g_x360 )
{
	$vcsName = $filename_base . ".360.vcs";
}
else
{
	$vcsName = $filename_base . ".vcs";
}
open COMPILEDSHADER, ">shaders/vsh/$vcsName" || die;
binmode( COMPILEDSHADER );

#
# Write out the part of the header that we know. . we'll write the rest after writing the object code.
#

# Pack arguments
my $sInt = "i";
my $uInt = "I";
if ( $g_x360 )
{
	# Change arguments to "big endian long"
	$sInt = "N";
	$uInt = "N";
}

my $undecoratedinput = join "", &ReadInputFileWithoutLineInfo( $filename );
#print STDERR "undecoratedinput: $undecoratedinput\n";
my $crc = crc32( $undecoratedinput );
#print STDERR "crc for $filename: $crc\n";

# version
print COMPILEDSHADER pack $sInt, 4;
# totalCombos
print COMPILEDSHADER pack $sInt, $numCombos;
# dynamic combos
print COMPILEDSHADER pack $sInt, $numDynamicCombos;
# flags
print COMPILEDSHADER pack $uInt, $flags;
# centroid mask
print COMPILEDSHADER pack $uInt, 0;
# reference size
print COMPILEDSHADER pack $uInt, 0;
# crc32 of the source code
print COMPILEDSHADER pack $uInt, $crc;

my $beginningOfDir = tell COMPILEDSHADER;

# Write out a blank directionary. . we'll fill it in later.
for( $i = 0; $i < $numCombos; $i++ )
{
	# offset from beginning of file.
	print COMPILEDSHADER pack $sInt, 0;
	# size
	print COMPILEDSHADER pack $sInt, 0;
}

my $startByteCode = tell COMPILEDSHADER;
my @byteCodeStart;
my @byteCodeSize;

# Write out the shader object code.
for( $shaderCombo = 0; $shaderCombo < $numCombos; $shaderCombo++ )
{
	my $filename = "shader$shaderCombo\.o";
	my $filesize = (stat $filename)[7];
	$byteCodeStart[$shaderCombo] = tell COMPILEDSHADER;
	$byteCodeSize[$shaderCombo] = $filesize;
	open SHADERBYTECODE, "<$filename" || die;
	binmode SHADERBYTECODE;
	my $bin;
	my $numread = read SHADERBYTECODE, $bin, $filesize;
#	print "filename: $filename numread: $numread filesize: $filesize\n";
	close SHADERBYTECODE;
	unlink $filename;

	print COMPILEDSHADER $bin;
}

# Seek back to the directory and write it out.
seek COMPILEDSHADER, $beginningOfDir, 0;
for( $i = 0; $i < $numCombos; $i++ )
{
	# offset from beginning of file.
	print COMPILEDSHADER pack $sInt, $byteCodeStart[$i];
	# size
	print COMPILEDSHADER pack $sInt, $byteCodeSize[$i];
}

close COMPILEDSHADER;

$total_time = SampleTime() - $total_start_time;

if ( $ShowTimers )
{
	print "\n\n";
	print sprintf( "Main loop time   : %0.4f sec, (%0.2f%%)\n", $main_total_time, 100*$main_total_time / $total_time );
	print sprintf( "Inner1 time      : %0.4f sec, (%0.2f%%)\n", $inner1_total_time, 100*$inner1_total_time / $total_time );
	print sprintf( "VSA time         : %0.4f sec, (%0.2f%%)\n", $vsa_total_time, 100*$vsa_total_time / $total_time );
	print sprintf( "eval() time      : %0.4f sec, (%0.2f%%)\n", $eval_total_time, 100*$eval_total_time / $total_time );
	print sprintf( "UsesRegister time: %0.4f sec, (%0.2f%%)\n", $usesr_total_time, 100*$usesr_total_time / $total_time );

	foreach $key ( keys %g_TimingBlocks )
	{
		print sprintf( "$key time: %0.4f sec, (%0.2f%%)\n", $g_TimingBlocks{$key}, 100*$g_TimingBlocks{$key} / $total_time );
	}

	print sprintf( "Total time : %0.4f sec\n", $total_time );
}

