param
(
    [ Parameter( Mandatory = $true ) ][ string ]$sFileName,
    [ Parameter( Mandatory = $true ) ][ string ]$sObjName
)

$Bytes = [ System.IO.File ]::ReadAllBytes( $sFileName )

Write-Output "static unsigned char $sObjName[] = {"

$sLine = "    "
for ( $i = 0; $i -lt $Bytes.Length; $i++ )
{
    $Byte = $Bytes[ $i ]

    if ( ( $i % 20 ) -ne 0 )
    {
        $sLine += " "
    }
    $sLine += ( "0x{0:x2}," -f $Byte )

    if ( ( $i % 20 ) -eq 19 )
    {
        Write-Output $sLine
        $sLine = "    "
    }
}

$sLine += " 0x00"
Write-Output $sLine
Write-Output "};"