sub BackToForwardSlash
{
	my( $path ) = shift;
	$path =~ s,\\,/,g;
	return $path;
}

sub RemoveFileName
{
	my( $in ) = shift;
	$in = &BackToForwardSlash( $in );
	$in =~ s,/[^/]*$,,;
	return $in;
}

sub RemovePath
{
	my( $in ) = shift;
	$in = &BackToForwardSlash( $in );
	$in =~ s,^(.*)/([^/]*)$,$2,;
	return $in;
}

sub MakeDirHier
{
	my( $in ) = shift;
#	print "MakeDirHier( $in )\n";
	$in = &BackToForwardSlash( $in );
	my( @path );
	while( $in =~ m,/, ) # while $in still has a slash
	{
		my( $end ) = &RemovePath( $in );
		push @path, $end;
#		print $in . "\n";
		$in = &RemoveFileName( $in );
	}
	my( $i );
	my( $numelems ) = scalar( @path );
	my( $curpath );
	for( $i = $numelems - 1; $i >= 0; $i-- )
	{
		$curpath .= "/" . $path[$i];
		my( $dir ) = $in . $curpath;
		if( !stat $dir )
		{
#			print "mkdir $dir\n";
			mkdir $dir, 0777;
		}
	}
}

sub FileExists
{
	my $filename = shift;
	my @statresult = stat $filename;
	my $iswritable = @statresult != 0;
	return $iswritable;
}

sub MakeFileWritable
{
	my $filename = shift;
	if ( &FileExists( $filename ) )
	{
		chmod 0666, $filename || die;
	}
}

sub MakeFileReadOnly
{
	my $filename = shift;
	chmod 0444, $filename || die;
}

# Run a command and get stdout and stderr to an array
sub RunCommand
{
	my $cmd = shift;
#	print STDERR "command: $cmd\n";
	system "$cmd > cmdout.txt 2>&1" || die;
	local( *FILE );
	open FILE, "<cmdout.txt" || die;
	my @output = <FILE>;
#	print STDERR "command output: @output\n";
	close FILE;
	unlink "cmdout.txt" || die;
	return @output;
}

sub PerforceEditOrAdd
{
	return;
	my $filename = shift;
	my $changelistarg = shift;

	# Is the file on the client?
	my $cmd = "p4 fstat \"$filename\"";
	my @p4output = &RunCommand( $cmd );
	my $p4output = join "", @p4output;
	if( $p4output =~ m/no such file/ )
	{
		# not on client. . add
		my $cmd = "p4 add $changelistarg $filename";
		my @p4output = &RunCommand( $cmd );
		my $p4output = join "", @p4output;
		if( $p4output =~ m/opened for add/ )
		{
			print $p4output;
			return;
		}
		print "ERROR: $p4output";
		return;
	}

	# The file is known to be on the client at this point.

	# Is it open for edit?
	if( $p4output =~ m/action edit/ )
	{
		# Is is open for edit, let's see if it's still different.
		# check for opened files that are not different from the revision in the depot.
		my $cmd = "p4 diff -sr \"$filename\"";
		my @p4output = &RunCommand( $cmd );
		my $outputstring = join "", @p4output;
		# check for empty string
		if( !( $outputstring =~ m/^\s*$/ ) )
		{
			my $cmd = "p4 revert \"$filename\"";
			my @p4output = &RunCommand( $cmd );
			my $outputstring = join "", @p4output;
			print $outputstring;
			return;
		}
	}

	# check for unopened files that are different from the revision in the depot.
	my $cmd = "p4 diff -se \"$filename\"";
	my @p4output = &RunCommand( $cmd );
	my $outputstring = join "", @p4output;
	# check for empty string
	if( $outputstring =~ m/^\s*$/ )
	{
		&MakeFileReadOnly( $filename );
		return;
	}

	# We need to edit the file since it is known to be different here.
	my $cmd = "p4 edit $changelistarg \"$filename\"";
	my @p4output = &RunCommand( $cmd );

	my $line;
	foreach $line ( @p4output )
	{
		if( $line =~ m/not on client/ )
		{
			#print "notonclient...";
			print "ERROR: @p4output\n";
			return;
		}
		if( $line =~ m/currently opened for edit/ )
		{
			return;
		}
		if( $line =~ m/opened for edit/ )
		{
			print $line;
		}
	}
}

sub FileIsWritable
{
	local( $filename ) = shift;
	local( @statresult ) = stat $filename;
	local( $mode, $iswritable );
	$mode = oct( $statresult[2] );
	$iswritable = ( $mode & 2 ) != 0;
	return $iswritable;
}

sub TouchFile
{
	my $filename = shift;
	if( !&FileExists( $filename ) )
	{
		if( !open FILE, ">$filename" )
		{
			die;
		}
		close FILE;
	}
	my $now = time;
	local( *FILE );
	utime $now, $now, $filename;
}

sub FileExistsInPerforce
{
	my $filename = shift;
	my @output = &RunCommand( "p4 fstat $filename" );
	my $line;
	foreach $line (@output)
	{
		if( $line =~ m/no such file/ )
		{
			return 0;
		}
	}
	return 1;
}

sub PerforceWriteFile
{
	my $filename = shift;
	my $filecontents = shift;
#	my $changelistname = shift;
	
	# Get the changelist number for the Shader Auto Checkout changelist. Will create the changelist if it doesn't exist.
#	my $changelistnumber = `valve_p4_create_changelist.cmd . \"$changelistname\"`;
	# Get rid of the newline
#	$changelistnumber =~ s/\n//g;

#	my $changelistarg = "";
#	if( $changelistnumber != 0 )
#	{
#		$changelistarg = "-c $changelistnumber"
#	}

	# Make the target vcs writable if it exists
	MakeFileWritable( $filename );

	# Write the file.
	local( *FP );
	open FP, ">$filename";
	print FP $filecontents;
	close FP;

	# Do whatever needs to happen with perforce for this file.
#	&PerforceEditOrAdd( $filename, $changelistarg );
}

sub WriteFile
{
	my $filename = shift;
	my $filecontents = shift;
	
	# Make the target vcs writable if it exists
	MakeFileWritable( $filename );

	# Write the file.
	local( *FP );
	open FP, ">$filename";
	print FP $filecontents;
	close FP;
}

sub PrintCleanPerforceOutput
{
	my $line;
	while( $line = shift )
	{
		if( $line =~ m/currently opened/i )
		{
			next;
		}
		if( $line =~ m/already opened for edit/i )
		{
			next;
		}
		if( $line =~ m/also opened/i )
		{
			next;
		}
		if( $line =~ m/add of existing file/i )
		{
			next;
		}
		print $line;
	}
}

# HACK!!!!  Need to pass something in to do this rather than hard coding.
sub NormalizePerforceFilename
{
	my $line = shift;

	# remove newlines.
	$line =~ s/\n//;
	# downcase.
	$line =~ tr/[A-Z]/[a-z]/;
	# backslash to forwardslash
	$line =~ s,\\,/,g;

	# for inc files HACK!
	$line =~ s/^.*(fxctmp9.*)/$1/i;
	$line =~ s/^.*(vshtmp9.*)/$1/i;

	# for vcs files. HACK!
	$line =~ s,^.*game/hl2/shaders/,,i;

	return $line;
}

sub MakeSureFileExists
{
	local( $filename ) = shift;
	local( $testexists ) = shift;
	local( $testwrite ) = shift;

	local( @statresult ) = stat $filename;
	if( !@statresult && $testexists )
	{
		die "$filename doesn't exist!\n";
	}
	local( $mode, $iswritable );
	$mode = oct( $statresult[2] );
	$iswritable = ( $mode & 2 ) != 0;
	if( !$iswritable && $testwrite )
	{
		die "$filename isn't writable!\n";
	}
}

sub LoadShaderListFile_GetShaderType
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

sub LoadShaderListFile_GetShaderSrc
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

sub LoadShaderListFile_GetShaderBase
{
	my $shadername = shift;
	if ( $shadername =~ m/-----(.*)$/i )
	{
		return $1;
	}
	else
	{
		my $shadertype = &LoadShaderListFile_GetShaderType( $shadername );
		$shadername =~ s/\.$shadertype//i;
		return $shadername;
	}
}

sub LoadShaderListFile
{
	my $inputbase = shift;

	my @srcfiles;
	&MakeSureFileExists( "$inputbase.txt", 1, 0 );

	open SHADERLISTFILE, "<$inputbase.txt" || die;
	my $line;
	while( $line = <SHADERLISTFILE> )
	{
		$line =~ s/\/\/.*$//;	# remove comments "//..."
		$line =~ s/^\s*//;		# trim leading whitespace
		$line =~ s/\s*$//;		# trim trailing whitespace
		next if( $line =~ m/^\s*$/ );
		if( $line =~ m/\.fxc/ || $line =~ m/\.vsh/ || $line =~ m/\.psh/ )
		{
			my $shaderbase = &LoadShaderListFile_GetShaderBase( $line );
			
			if( $ENV{"DIRECTX_FORCE_MODEL"} =~ m/^30$/i )	# forcing all shaders to be ver. 30
			{
				my $targetbase = $shaderbase;
				$targetbase =~ s/_ps2x/_ps30/i;
				$targetbase =~ s/_ps20b/_ps30/i;
				$targetbase =~ s/_ps20/_ps30/i;
				$targetbase =~ s/_vs20/_vs30/i;
				$targetbase =~ s/_vsxx/_vs30/i;
				push @srcfiles, ( $line . "-----" . $targetbase );
			}
			else
			{
   				if( $shaderbase =~ m/_ps2x/i )
				{
					my $targetbase = $shaderbase;
					$targetbase =~ s/_ps2x/_ps20/i;
					push @srcfiles, ( $line . "-----" . $targetbase );
					
					$targetbase = $shaderbase;
					$targetbase =~ s/_ps2x/_ps20b/i;
					push @srcfiles, ( $line . "-----" . $targetbase );
				}
				elsif( $shaderbase =~ m/_vsxx/i )
				{
					my $targetbase = $shaderbase;
					$targetbase =~ s/_vsxx/_vs11/i;
					push @srcfiles, ( $line . "-----" . $targetbase );
					
					$targetbase = $shaderbase;
					$targetbase =~ s/_vsxx/_vs20/i;
					push @srcfiles, ( $line . "-----" . $targetbase );
				}
				else
				{
					push @srcfiles, ( $line . "-----" . $shaderbase );
				}
			}
		}
	}
	close SHADERLISTFILE;
	return @srcfiles;
}

sub ReadInputFileWithIncludes
{
	local( $filename ) = shift;
#	print STDERR "ReadInputFileWithIncludes: $filename\n";

	local( *INPUT );
	local( $output );

#	print STDERR "before open\n";
	open INPUT, "<$filename" || die;
#	print STDERR "after open\n";

	local( $line );
	while( $line = <INPUT> )
	{
#		print STDERR $line;
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

sub GetCRCFromSourceFile
{
	my $filename = shift;
	my $data = &ReadInputFileWithIncludes( $filename );
#	print STDERR $data;
	$crc = crc32( $data );
#	print STDERR "GetCRCFromSourceFile: $crc\n";
	return $crc;
}

sub GetCRCFromVCSFile
{
	my $filename = shift;
#	print STDERR "GetCRCFromVCSFile $filename\n";
	local( *FP );
	open FP, "<$filename" || die "GetCRCFromVCSFile: can't open file $filename\n";
	binmode( FP );

	# unpack arguments
	my $sInt = "i";
	my $uInt = "I";
	if( $filename =~ m/\.360\./ )
	{
		# Change arguments to "big endian long"
		$sInt = "N";
		$uInt = "N";
	}

	my $header;
	read FP, $header, 7 * 4 || die "updateshaders.pl:GetCRCFromVCSFile: can't read header for $filename\n";
	my $version,$numCombos,$numDynamicCombos,$flags,$centroidMask,$refSize,$crc;
	($version,$numCombos,$numDynamicCombos,$flags,$centroidMask,$refSize,$crc) = unpack "$sInt$sInt$sInt$uInt$uInt$uInt$uInt", $header;
	unless( $version == 4 || $version == 5 || $version == 6 )
	{
		print STDERR "ERROR: GetCRCFromVCSFile: $filename is version $version\n";
		return 0;
	}
#	print STDERR "version: $version\n";
#	print STDERR "numCombos: $numCombos\n";
#	print STDERR "numDynamicCombos: $numDynamicCombos\n";
#	print STDERR "flags: $flags\n";
#	print STDERR "centroidMask: $centroidMask\n";
#	print STDERR "refSize: $refSize\n";
#	print STDERR "GetCRCFromVCSFile: $crc\n";
	close( FP );
	return $crc;
}

sub CheckCRCAgainstTarget
{
	my $srcFileName = shift;
	my $vcsFileName = shift;
	my $warn = shift;
	
	# Make sure both files exist.
#	print STDERR "$srcFileName doesn't exist\n" if( !( -e $srcFileName ) );
#	print STDERR "$vcsFileName doesn't exist\n" if( !( -e $vcsFileName ) );
	if( !( -e $srcFileName ) )
	{
		if( $warn )
		{
			print "$srcFileName missing\n";
		}
		return 0;
	}
	if( !( -e $vcsFileName ) )
	{
		if( $warn )
		{
			print "$vcsFileName missing\n";
		}
		return 0;
	}
#	print STDERR "CheckCRCAgainstTarget( $srcFileName, $vcsFileName );\n";
#	print STDERR "vcsFileName: $vcsFileName\n";
#	print STDERR "vcsFileName: $srcFileName\n";
	my $vcsCRC = &GetCRCFromVCSFile( $vcsFileName );
	my $srcCRC = &GetCRCFromSourceFile( $srcFileName );
	if( $warn && ( $vcsCRC != $srcCRC ) )
	{
		print "$vcsFileName checksum ($vcsCRC) != $srcFileName checksum: ($srcCRC)\n";
	}
	
#	return 0; # use this to skip crc checking.
#	if( $vcsCRC == $srcCRC )
#	{
#		print STDERR "CRC passed for $srcFileName $vcsFileName $vcsCRC\n";
#	}
	return $vcsCRC == $srcCRC;
}

1;
