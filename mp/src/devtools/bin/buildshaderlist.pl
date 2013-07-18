use File::DosGlob;
@ARGV = map {
              my @g = File::DosGlob::glob($_) if /[*?]/;
              @g ? @g : $_;
            } @ARGV;

open FILE, ">__tmpshaderlist.txt";

foreach $arg (@ARGV)
{
	if( $arg =~ m/\.fxc$/i || $arg =~ m/\.vsh$/i || $arg =~ m/\.psh$/i )
	{	
		print $arg . "\n";
		print FILE $arg . "\n";
	}
}

close FILE;

system "buildshaders.bat __tmpshaderlist";

unlink "__tmpshaderlist.txt";