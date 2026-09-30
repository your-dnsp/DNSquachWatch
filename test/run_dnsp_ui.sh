#!/bin/sh
# All test settings live in a fresh temporary directory, never a saved user profile.
set -eu
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
fixture_dir=$(mktemp -d "${TMPDIR:-/tmp}/dnsp-ui.XXXXXX")
trap 'rm -rf "$fixture_dir"' EXIT HUP INT TERM
make -C "$project_dir/sim" -j4
SQUACHSIM_NVS="$fixture_dir" DNSP_UI_TEST=1 "$project_dir/sim/squachsim-live"
mkdir "$fixture_dir/portrait"
SQUACHSIM_NVS="$fixture_dir/portrait" SQUACHSIM_ROTATE=1 DNSP_UI_TEST=1 DNSP_UI_PORTRAIT=1 "$project_dir/sim/squachsim-live"

mkdir "$fixture_dir/reboot"
SQUACHSIM_NVS="$fixture_dir/reboot" DNSP_UI_TEST=1 DNSP_TEST_REBOOT=1 "$project_dir/sim/squachsim-live"

# Compare full-frame and half-frame painting at fixed state in both orientations.
mkdir "$fixture_dir/banded"
SQUACHSIM_NVS="$fixture_dir/banded" DNSP_BAND_TEST=1 "$project_dir/sim/squachsim-live"
