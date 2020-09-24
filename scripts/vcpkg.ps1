. "$PSScriptRoot\helpers.ps1"

& "$vcpkgCmd" --vcpkg-root "$vcpkgRoot" --triplet $vcpkgTriplet $args
