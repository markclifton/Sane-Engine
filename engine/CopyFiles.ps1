param (
    [Parameter(Mandatory=$true)][string]$SRC,
    [Parameter(Mandatory=$true)][string]$DEST
)

$DEST=$DEST+"/include/"

if(!(Test-Path -Path $DEST)) {
    New-Item -Path $DEST -ItemType Directory | out-null
}

$lastRecompileTimeLog=$SRC+"/lastRecompileTime.log"

if(Test-Path -Path $lastRecompileTimeLog) {
    $lastRecompileTime=(ls $lastRecompileTimeLog).LastWriteTime
} else {
    New-Item -Path $lastRecompileTimeLog -ItemType File | out-null
    $lastRecompileTime=Get-Date -Date "01/01/1970"
}

Get-Date | Out-File -FilePath $lastRecompileTimeLog -Encoding utf8

$shouldRecompile=$false
$headerFile=$SRC+"/include/resources/EngineShaders.hpp"
if(!(Test-Path -Path $headerFile)) {
    New-Item -Path $headerFile -ItemType File -Force | out-null
    $shouldRecompile=$true
}

$glslangValidator=$SRC+"/glslangValidator.exe"

Get-ChildItem $SRC/resources/shaders -Recurse -Exclude *.spv | Where{$_.LastWriteTime -gt $lastRecompileTime} | Foreach-Object {
    $shouldRecompile=$true
}

if($shouldRecompile -eq $true) {
    Write-Output "Recompiling shaders..."

    "#pragma once" | Out-File -FilePath $headerFile -Encoding utf8
    Get-ChildItem $SRC/resources/shaders -Recurse -Exclude *.spv | Foreach-Object {
        $newPath=$_.FullName+".spv"
        & $glslangValidator -V $_.FullName -o $newPath | out-null

        $baseName=$_.BaseName.ToUpper()
        $type=$_.Extension.Substring(1).ToUpper()
        $contents=Get-Content -Path $newPath -Encoding Byte -Raw

        "static const char* SHADER_"+$type.ToUpper()+"_"+$baseName+'= R"###('+$contents+')###";'| Out-File -FilePath $headerFile -Append -Encoding utf8

        Remove-Item $newPath
    }
}

$dataBlob=$SRC+"/include/resources/EngineData.blob"
if((Test-Path -Path $dataBlob)) {
    Remove-Item $dataBlob
    New-Item -Path $dataBlob -ItemType File -Force | out-null
}

$DataPacker=$SRC+"/DataPacker.exe"
Get-ChildItem $SRC/resources/textures -Recurse | Foreach-Object {
    & $DataPacker -o $dataBlob -t texture $_.FullName | out-null
}

Write-Output "Updating Engine Headers..."
robocopy $SRC"/include" $DEST /mt /z /s | out-null
