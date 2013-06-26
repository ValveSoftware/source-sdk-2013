use String::CRC32;
BEGIN {use File::Basename; push @INC, dirname($0); }
require "valve_perl_helpers.pl";

sub BuildDefineOptions
{
	local( $output );
	local( $combo ) = shift;
	local( $i );
	for( $i = 0; $i < scalar( @dynamicDefineNames ); $i++ )
	{
		local( $val ) = ( $combo % ( $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1 ) ) + $dynamicDefineMin[$i];
		$output .= "/D$dynamicDefineNames[$i]=$val ";		
		$combo = $combo / ( $dynamicDefineMax[$i] - $dynamicDefineMin[$i] + 1 );
	}
	for( $i = 0; $i < scalar( @staticDefineNames ); $i++ )
	{
		local( $val ) = ( $combo % ( $staticDefineMax[$i] - $staticDefineMin[$i] + 1 ) ) + $staticDefineMin[$i];
		$output .= "/D$staticDefineNames[$i]=$val ";
		$combo = $combo / ( $staticDefineMax[$i] - $staticDefineMin[$i] + 1 );
	}
	return $output;
}

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

$g_dx9 = 1;

while( 1 )
{
	$psh_filename = shift;

	if( $psh_filename =~ m/-source/ )
	{
		$g_SourceDir = shift;
	}
	elsif( $psh_filename =~ m/-x360/ )
	{
		$g_x360 = 1;
	}
	else
	{
		last;
	}
}

$psh_filename =~ s/-----.*$//;


# Get the shader binary version number from a header file.
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



local( @staticDefineNames );
local( @staticDefineMin );
local( @staticDefineMax );
local( @dynamicDefineNames );
local( @dynamicDefineMin );
local( @dynamicDefineMax );

# Parse the combos.
open PSH, "<$psh_filename";
while( <PSH> )
{
	last if( !m,^;, );
	s,^;\s*,,;
	if( m/\s*STATIC\s*\:\s*\"(.*)\"\s+\"(\d+)\.\.(\d+)\"/ )
	{
		local( $name, $min, $max );
		$name = $1;
		$min = $2;
		$max = $3;
#		print "\"STATIC: $name\" \"$min..$max\"\n";
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
#		print "\"DYNAMIC: $name\" \"$min..$max\"\n";
		if (/\[(.*)\]/)
		{
			$platforms=$1;
			next if ( ($g_x360) && (!($platforms=~/XBOX/i)) );
			next if ( (!$g_x360) && (!($platforms=~/PC/i)) );
		}
		push @dynamicDefineNames, $name;
		push @dynamicDefineMin, $min;
		push @dynamicDefineMax, $max;
	}
}
close PSH;

$numCombos = &CalcNumCombos();
$numDynamicCombos = &CalcNumDynamicCombos();
print "$psh_filename\n";
#print "$numCombos combos\n";
#print "$numDynamicCombos dynamic combos\n";

if( $g_x360 )
{
	$pshtmp = "pshtmp9_360";
}
elsif( $g_dx9 )
{
	$pshtmp = "pshtmp9";
}
else
{
	$pshtmp = "pshtmp8";
}
$basename = $psh_filename;
$basename =~ s/\.psh$//i;

for( $shaderCombo = 0; $shaderCombo < $numCombos; $shaderCombo++ )
{
	my $tempFilename = "shader$shaderCombo.o";
	unlink $tempFilename;
	
	if( $g_x360 )
	{
		$cmd = "$g_SourceDir\\x360xdk\\bin\\win32\\psa /D_X360=1 /Foshader$shaderCombo.o /nologo " . &BuildDefineOptions( $shaderCombo ) . "$psh_filename > NIL";
	}
	else
	{
		$cmd = "$g_SourceDir\\dx9sdk\\utilities\\psa /Foshader$shaderCombo.o /nologo " . &BuildDefineOptions( $shaderCombo ) . "$psh_filename > NIL";
	}

	if( !stat $pshtmp )
	{
		mkdir $pshtmp, 0777 || die $!;
	}

#	print $cmd . "\n";
	system $cmd || die $!;

	# Make sure a file got generated because sometimes the die above won't happen on compile errors.
	my $filesize = (stat $tempFilename)[7];
	if ( !$filesize )
	{
		die "Error compiling shader$shaderCombo.o";
	}

	push @outputHeader, @hdr;
}

$basename =~ s/\.fxc//gi;
push @outputHeader, "static PrecompiledShaderByteCode_t " . $basename . "_pixel_shaders[" . $numCombos . "] = \n";
push @outputHeader, "{\n";
local( $j );
for( $j = 0; $j < $numCombos; $j++ )
{
	local( $thing ) = "pixelShader_" . $basename . "_" . $j;
	push @outputHeader, "\t{ " . "$thing, sizeof( $thing ) },\n";
}
push @outputHeader, "};\n";

push @outputHeader, "struct $basename" . "PixelShader_t : public PrecompiledShader_t\n";
push @outputHeader, "{\n";
push @outputHeader, "\t$basename" . "PixelShader_t()\n";
push @outputHeader, "\t{\n";
push @outputHeader, "\t\tm_nFlags = 0;\n";
push @outputHeader, "\t\tm_pByteCode = " . $basename . "_pixel_shaders;\n";
push @outputHeader, "\t\tm_nShaderCount = $numCombos;\n";
#push @outputHeader, "\t\tm_nDynamicCombos = m_nShaderCount;\n";
push @outputHeader, "\t\t// NOTE!!!  psh_prep.pl shaders are always static combos!\n";
push @outputHeader, "\t\tm_nDynamicCombos = 1;\n";
push @outputHeader, "\t\tm_pName = \"$basename\";\n";
if( $basename =~ /vs\d\d/ ) # hack
{
	push @outputHeader, "\t\tGetShaderDLL()->InsertPrecompiledShader( PRECOMPILED_VERTEX_SHADER, this );\n";
}
else
{
	push @outputHeader, "\t\tGetShaderDLL()->InsertPrecompiledShader( PRECOMPILED_PIXEL_SHADER, this );\n";
}
push @outputHeader, "\t}\n";
push @outputHeader, "\tvirtual const PrecompiledShaderByteCode_t &GetByteCode( int shaderID )\n";
push @outputHeader, "\t{\n";
push @outputHeader, "\t\treturn m_pByteCode[shaderID];\n";
push @outputHeader, "\t}\n";
push @outputHeader, "};\n";

push @outputHeader, "static $basename" . "PixelShader_t $basename" . "_PixelShaderInstance;\n";


&MakeDirHier( "shaders/psh" );

my $vcsName = "";
if( $g_x360 )
{
	$vcsName = $basename . ".360.vcs";
}
else
{
	$vcsName = $basename . ".vcs";
}

open COMPILEDSHADER, ">shaders/psh/$vcsName" || die;
binmode( COMPILEDSHADER );

#
# Write out the part of the header that we know. . we'll write the rest after writing the object code.
#

#print $numCombos . "\n";

# Pack arguments
my $sInt = "i";
my $uInt = "I";
if ( $g_x360 )
{
	# Change arguments to "big endian long"
	$sInt = "N";
	$uInt = "N";
}

open PSH, "<$psh_filename";
my $crc = crc32( *PSH );
close PSH;
#print STDERR "crc for $psh_filename: $crc\n";

# version
print COMPILEDSHADER pack $sInt, 4;
# totalCombos
print COMPILEDSHADER pack $sInt, $numCombos;
# dynamic combos
print COMPILEDSHADER pack $sInt, $numDynamicCombos;
# flags
print COMPILEDSHADER pack $uInt, 0x0; # nothing here for now.
# centroid mask
print COMPILEDSHADER pack $uInt, 0;
# reference size for diffs
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
	open SHADERBYTECODE, "<$filename";
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


