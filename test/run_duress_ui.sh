#!/bin/sh
# Isolated simulator checks; never touch a real card or flash device.
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
fixture_dir=$(mktemp -d "${TMPDIR:-/tmp}/dnsp-duress.XXXXXX")
trap 'rm -rf "$fixture_dir"' EXIT HUP INT TERM
make -C "$project_dir/sim" -j4 live
mkdir "$fixture_dir/land" "$fixture_dir/port" "$fixture_dir/boot"
SQUACHSIM_NVS="$fixture_dir/land" DNSP_DURESS_TEST=1 "$project_dir/sim/squachsim-live"
SQUACHSIM_NVS="$fixture_dir/port" SQUACHSIM_ROTATE=1 DNSP_UI_PORTRAIT=1 DNSP_DURESS_TEST=1 "$project_dir/sim/squachsim-live"
SQUACHSIM_NVS="$fixture_dir/boot" DNSP_DURESS_BOOT=1 DNSP_DURESS_TEST=1 "$project_dir/sim/squachsim-live"
# No normal settings/services may create files on the isolated early boot.
test -z "$(ls -A "$fixture_dir/boot")"
