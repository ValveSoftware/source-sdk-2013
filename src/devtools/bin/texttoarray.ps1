param
(
    [ Parameter( Mandatory=$true ) ][ string ]$sFileName,
    [ Parameter( Mandatory=$true ) ][ string ]$sObjName
)

$Bytes = [ System.IO.File ]::ReadAllBytes( $sFileName )

Write-Host "static unsigned char $sObjName[] = {"
Write-Host "    " -NoNewline

for ( $i = 0; $i -lt $Bytes.Length; $i++ )
{
    $Byte = $Bytes[ $i ]
    Write-Host ( "0x{0:x2}," -f $Byte ) -NoNewline

    if ( ( $i % 20 ) -eq 19 )
    {
        Write-Host ""
        Write-Host "    " -NoNewline
    }
}

Write-Host "0x00"
Write-Host "};"