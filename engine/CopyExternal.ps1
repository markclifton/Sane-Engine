param (
    [Parameter(Mandatory=$true)][string]$SRC,
    [Parameter(Mandatory=$true)][string]$DEST
)

$DEST=$DEST+"/include/"

if(!(Test-Path -Path $DEST)) {
    New-Item -Path $DEST -ItemType Directory | out-null
}

Write-Output "Updating External Headers..."
Get-ChildItem $SRC -Directory | Foreach-Object {
    robocopy $_.FullName $DEST /mt /z *.h | out-null
    robocopy $_.FullName $DEST /mt /z *.hpp | out-null
    robocopy $_.FullName $DEST /mt /z *.inl | out-null

    $NestedIncludeFolder=$_.FullName+"/include/"
    if((Test-Path -Path $NestedIncludeFolder)) {
        robocopy $NestedIncludeFolder $DEST /mt /z /s *.h | out-null
        robocopy $NestedIncludeFolder $DEST /mt /z /s *.hpp | out-null
        robocopy $NestedIncludeFolder $DEST /mt /z /s *.inl | out-null
    }

    $NestedIncludeFolder=$_.FullName+"/single_include/"
    if((Test-Path -Path $NestedIncludeFolder)) {
        robocopy $NestedIncludeFolder $DEST /mt /z /s *.h | out-null
        robocopy $NestedIncludeFolder $DEST /mt /z /s *.hpp | out-null
        robocopy $NestedIncludeFolder $DEST /mt /z /s *.inl | out-null
    }

    $NestedIncludeFolder=$_.FullName+"/glm/"
    if((Test-Path -Path $NestedIncludeFolder)) {
        robocopy $NestedIncludeFolder $DEST"/glm/" /mt /z /s *.h | out-null
        robocopy $NestedIncludeFolder $DEST"/glm/" /mt /z /s *.hpp | out-null
        robocopy $NestedIncludeFolder $DEST"/glm/" /mt /z /s *.inl | out-null
    }

    $NestedIncludeFolder=$_.FullName+"/backends/"
    if((Test-Path -Path $NestedIncludeFolder)) {
        robocopy $NestedIncludeFolder $DEST"/backends/" /mt /z /s *.h | out-null
        robocopy $NestedIncludeFolder $DEST"/backends/" /mt /z /s *.hpp | out-null
        robocopy $NestedIncludeFolder $DEST"/backends/" /mt /z /s *.inl | out-null
    }
}
