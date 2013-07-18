for($ix=-2;$ix<=2;$ix++)
{
	for($iy=-2;$iy<=2;$iy++)
	{
		print "vRefractColor += tex2D( RefractSampler, vRefractTexCoord + $ix * ddx1 + $iy * ddy1 );\n";
		$sumweights+=1;
	}
}
print "float sumweights = $sumweights;\n";
