foreach $_ (sort <> )
{
	next if( defined( $prevline ) && $_ eq $prevline );
	$prevline = $_;
	print;
}
