param (
    [Parameter(Mandatory=$true)][string]$SRC,
    [Parameter(Mandatory=$true)][string]$DEST,
    [Parameter(Mandatory=$false)][switch]$RELEASE
 )

if(!(Test-Path -Path $DEST)) {
    New-Item -Path $DEST -ItemType Directory | out-null
}

Write-Output "Copying resources to output dir..."
robocopy $SRC $DEST /mt /z /s /XX | out-null

robocopy $SRC"..\..\internal\SaneEngine\include\resources\" $DEST"..\resources\" EngineData.blob  | out-null

$glslangValidator=$SRC+"../../engine/glslangValidator.exe"

if($LastExitCode -gt 0 -and $LastExitCode -ne 2 ){
    Write-Output "Recompiling Shaders..."
    Get-ChildItem $DEST\"shaders" -Exclude *.exe,*.bat,*.spv | Foreach-Object {
        $newPath=$_.FullName+".spv"
        & $glslangValidator -V $_.FullName -o $newPath
        if($RELEASE) { Remove-Item $_ }
    }
    if($RELEASE) { Remove-Item $DEST\shaders\glslangValidator.exe }

    # Update Manifest!
    $includePath=$SRC+"..\include"
    if(!(Test-Path -Path $includePath)) {
        New-Item -Path $includePath -ItemType Directory | out-null
    }

    $headerFile=$SRC+"..\include\ManifestHeader.hpp"
    if(!(Test-Path -Path $headerFile)) {
        Write-Output "Path: " + $headerFile
        New-Item $headerFile -ItemType file
    }
    "#pragma once" | Out-File -FilePath $headerFile -Encoding utf8

    $manifestXML=$DEST+"Manifest.xml"
    if(!(Test-Path -Path $manifestXML)) {
        New-Item $manifestXML | out-null
    }

    $xmlsettings = New-Object System.Xml.XmlWriterSettings
    $xmlsettings.Indent = $true
    $xmlsettings.IndentChars = "    "

    $XmlWriter = [System.XML.XmlWriter]::Create($manifestXML, $xmlsettings)
    $xmlWriter.WriteStartDocument()

    $xmlWriter.WriteStartElement("manifest")
    $currentDirectory=""
    $type=""
    Get-ChildItem $SRC -Recurse -Exclude *.mtl,*.exe | Foreach-Object {
        if(! ($_ -is [System.IO.DirectoryInfo]) -and !($_.BaseName -eq "Manifest")) 
        {
            if($currentDirectory -ne $_.Directory.BaseName)
            {
                if($currentDirectory -ne "")
                {
                    $xmlWriter.WriteEndElement()
                }

                $currentDirectory=$_.Directory.BaseName
                $type=$currentDirectory.SubString(0, $currentDirectory.Length -1)
                $xmlWriter.WriteStartElement($currentDirectory)
            }

            $xmlWriter.WriteStartElement($type)
            $xmlWriter.WriteAttributeString("name", $_.BaseName)
            if($_.Extension -eq ".obj")
            {
                $xmlWriter.WriteAttributeString("path", $currentDirectory+"/"+$_.BaseName)
            }
            else 
            {
                $xmlWriter.WriteAttributeString("path", $currentDirectory+"/"+$_.name)
            }

            $random= [uint64](Get-Random -Minimum 0 -Maximum 18446744073709551615)

            $xmlWriter.WriteAttributeString("uuid", $random)
            $xmlWriter.WriteEndElement()

            "#define "+$type.ToUpper()+"_"+$_.BaseName.ToUpper()+"_"+$_.Extension.Substring(1).ToUpper()+" "+$random+"ull" | Out-File -FilePath $headerFile -Append -Encoding utf8
        }
    }

    if($currentDirectory -ne "")
    {
        $xmlWriter.WriteEndElement()
    }

    $xmlWriter.WriteEndDocument()
    $xmlWriter.Flush()
    $xmlWriter.Close()
}
