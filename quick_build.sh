#!/bin/bash
# Build TIP FW L0+L1 from a repo-tool workspace root.

set -e

top="$(cd "$(dirname "$(realpath "$BASH_SOURCE")")" && pwd)"
tip_dir="$top/cerberus/projects/nuvoton/tip"

"$tip_dir/build_tip_image.sh" "$top/bld_boot" "$top/bld_cerberus"

echo "==============="
echo "build done"
uptime
echo "==============="

