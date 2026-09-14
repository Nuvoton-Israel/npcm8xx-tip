#!/bin/bash
# Build TIP FW (L0 + L1) from a repo-tool workspace root.
#
# Intended to be checked out at the top of the workspace (see the
# <copyfile> entry in the npcm8xx-tip-fw-manifest default.xml), next to the
# cerberus/ directory produced by `repo sync`. Output binaries are copied
# to this same directory for convenience.

set -e

top="$(cd "$(dirname "$(realpath "$BASH_SOURCE")")" && pwd)"
tip_dir="$top/cerberus/projects/nuvoton/tip"

"$tip_dir/build_tip_image.sh"

cp "$tip_dir"/build_L0/arbel_tip_fw_L0.bin "$tip_dir"/build_L0/arbel_tip_fw_L0.elf "$tip_dir"/build_L0/arbel_tip_fw_L0.map "$top/"
cp "$tip_dir"/build_L1/arbel_tip_fw_L1.bin "$tip_dir"/build_L1/arbel_tip_fw_L1.elf "$tip_dir"/build_L1/arbel_tip_fw_L1.map "$top/"

echo "==============="
echo "build done"
uptime
echo "==============="
