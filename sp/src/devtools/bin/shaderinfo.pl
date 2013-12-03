#! perl

my $fname=shift || die "format is shaderinfo blah.vcs";

open(SHADER, $fname) || die "can't open $fname";
binmode SHADER;

read(SHADER,$header,20);
($ver,$ntotal,$ndynamic,$flags,$centroidmask)=unpack("LLLLL",$header);

#print "Version $ver total combos=$ntotal, num dynamic combos=$ndynamic,\n flags=$flags, centroid mask=$centroidmask\n";

read(SHADER,$refsize,4);
$refsize=unpack("L",$refsize);
#print "Size of reference shader for diffing=$refsize\n";

seek(SHADER,$refsize,1);

$nskipped_combos=0;
for(1..$ntotal)
  {
	read(SHADER,$combodata,8);
	($ofs,$combosize)=unpack("LL",$combodata);
	if ( $ofs == 0xffffffff)
	  {
		$nskipped_combos++;
	  }
	else
	  {
	  }
  }
#print "$nskipped_combos skipped, for an actual total of ",$ntotal-$nskipped_combos,"\n";
#print "Real to skipped ratio = ",($ntotal-$nskipped_combos)/$ntotal,"\n";
# csv output - name, real combos, virtual combos, dynamic combos
my $real_combos=$ntotal-$nskipped_combos;
print "$fname,$real_combos,$ntotal,$ndynamic\n";
