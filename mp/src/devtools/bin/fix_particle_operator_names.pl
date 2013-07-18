#!perl
use File::Find;

&BuildRemapTable;

find(\&convert, "." );


sub convert
  {
	return unless (/\.pcf$/i);
	return if (/^tmp\.pcf$/i);
	return if (/^tmp2\.pcf$/i);
	return if (/360\.pcf$/i);
	print STDERR "process ", $File::Find::name," ($_) dir=",`cd`," \n";
	my $fname=$_;
	print `p4 edit $fname`;
	print `dmxconvert -i $_ -o tmp.pcf -oe keyvalues2`;
	open(TMP, "tmp.pcf" ) || return;
	open(OUT, ">tmp2.pcf" ) || die;
	while(<TMP>)
	  {
		s/[\n\r]//g;
		if ( (/^(\s*\"functionName\"\s*\"string\"\s*\")(.*)\"(.*)$/) &&
			 length($map{$2}) )
		  {
			$_=$1.$map{$2}.'"'.$3;
		  }
		if ( (/^(\s*\"name\"\s*\"string\"\s*\")(.*)\"(.*)$/) &&
			 length($map{$2}) )
		  {
			$_=$1.$map{$2}.'"'.$3;
		  }
		print OUT "$_\n";
	  }
	close OUT;
	close TMP;
	print `dmxconvert -i tmp2.pcf -o $fname -ie keyvalues2 -oe binary`;
	unlink "tmp.pcf";
	unlink "tmp2.pcf";
}












sub BuildRemapTable
{
  $map{"alpha_fade"}= "Alpha Fade and Decay";
  $map{"alpha_fade_in_random"}= "Alpha Fade In Random";
  $map{"alpha_fade_out_random"}= "Alpha Fade Out Random";
  $map{"basic_movement"}= "Movement Basic";
  $map{"color_fade"}= "Color Fade";
  $map{"controlpoint_light"}= "Color Light From Control Point";
  $map{"Dampen Movement Relative to Control Point"}= "Movement Dampen Relative to Control Point";
  $map{"Distance Between Control Points Scale"}= "Remap Distance Between Two Control Points to Scalar";
  $map{"Distance to Control Points Scale"}= "Remap Distance to Control Point to Scalar";
  $map{"lifespan_decay"}= "Lifespan Decay";
  $map{"lock to bone"}=	"Movement Lock to Bone";
  $map{"postion_lock_to_controlpoint"}= "Movement Lock to Control Point";
  $map{"maintain position along path"}= "Movement Maintain Position Along Path";
  $map{"Match Particle Velocities"}= "Movement Match Particle Velocities";
  $map{"Max Velocity"}= "Movement Max Velocity";
  $map{"noise"}= "Noise Scalar";
  $map{"vector noise"}= "Noise Vector";
  $map{"oscillate_scalar"}= "Oscillate Scalar";
  $map{"oscillate_vector"}= "Oscillate Vector";
  $map{"Orient Rotation to 2D Direction"}= "Rotation Orient to 2D Direction";
  $map{"radius_scale"}= "Radius Scale";
  $map{"Random Cull"}= "Cull Random";
  $map{"remap_scalar"}= "Remap Scalar";
  $map{"rotation_movement"}= "Rotation Basic";
  $map{"rotation_spin"}= "Rotation Spin Roll";
  $map{"rotation_spin yaw"}= "Rotation Spin Yaw";
  $map{"alpha_random"}= "Alpha Random";
  $map{"color_random"}= "Color Random";
  $map{"create from parent particles"}= "Position From Parent Particles";
  $map{"Create In Hierarchy"}= "Position In CP Hierarchy";
  $map{"random position along path"}= "Position Along Path Random";
  $map{"random position on model"}= "Position on Model Random";
  $map{"sequential position along path"}= "Position Along Path Sequential";
  $map{"position_offset_random"}= "Position Modify Offset Random";
  $map{"position_warp_random"}= "Position Modify Warp Random";
  $map{"position_within_box"}= "Position Within Box Random";
  $map{"position_within_sphere"}= "Position Within Sphere Random";
  $map{"Inherit Velocity"}= "Velocity Inherit from Control Point";
  $map{"Initial Repulsion Velocity"}= "Velocity Repulse from World";
  $map{"Initial Velocity Noise"}= "Velocity Noise";
  $map{"Initial Scalar Noise"}= "Remap Noise to Scalar";
  $map{"Lifespan from distance to world"}= "Lifetime from Time to Impact";
  $map{"Pre-Age Noise"}= "Lifetime Pre-Age Noise";
  $map{"lifetime_random"}= "Lifetime Random";
  $map{"radius_random"}= "Radius Random";
  $map{"random yaw"}= "Rotation Yaw Random";
  $map{"Randomly Flip Yaw"}= "Rotation Yaw Flip Random";
  $map{"rotation_random"}= "Rotation Random";
  $map{"rotation_speed_random"}= "Rotation Speed Random";
  $map{"sequence_random"}= "Sequence Random";
  $map{"second_sequence_random"}= "Sequence Two Random";
  $map{"trail_length_random"}= "Trail Length Random";
  $map{"velocity_random"}= "Velocity Random";
}

