$infilename = shift;
$outfilename1 = shift;
$outfilename2 = shift;
open INPUT, $infilename || die;
@input = <INPUT>;
close INPUT;

open MERGEDMINE, ">$outfilename1" || die;
open MERGEDTHEIRS, ">$outfilename2" || die;

for( $i = 0; $i < scalar( @input ); $i++ )
{
	$line = $input[$i];

	if( $line =~ m/^(.*)<<<<<<</ )
	{
		$first = 1;
		$second = 0;
		print MERGEDMINE $1;
		print MERGEDTHEIRS $1;
		next;
	}
	# Make sure that we are in a split block so that comments with ======= don't mess us up.
	if( $line =~ m/^(.*)=======$/ && $first == 1 )
	{
		$first = 0;
		$second = 1;
		print MERGEDMINE $1;
		next;
	}
	if( $line =~ m/^(.*)>>>>>>>/ )
	{
		$first = $second = 0;
		print MERGEDTHEIRS $1;
		next;
	}

	if( $first )
	{
		print MERGEDMINE $line;
	}
	elsif( $second )
	{
		print MERGEDTHEIRS $line;
	}
	else
	{
		print MERGEDMINE $line;
		print MERGEDTHEIRS $line;
	}
}

close MERGEDMINE;
close MERGEDTHEIRS;
