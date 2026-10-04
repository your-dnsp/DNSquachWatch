# SquachWatch-CYD — host tests.
#
# The firmware's decoders are pure arithmetic over byte buffers with no
# radio behind them, which means they can be tested on a desktop against
# hand-built packets. That is worth doing precisely because the
# alternative is standing in a field with a drone: an endianness slip in
# a coordinate field produces a plausible-looking wrong answer, not a
# crash, and nothing on the device would ever tell you.
#
#   make          # build and run everything
#   make clean
#
# Include path mirrors sim/Makefile: the sim directory comes first so
# <Arduino.h> and friends resolve to its shims rather than to an ESP32
# core that is not installed on a build machine.

SRC  := ../src
INC  := ../include
SIM  := ../sim
OUT  := out

CXX      ?= g++
CXXFLAGS := -std=c++17 -O1 -g -Wall -Wextra -Wno-unused-parameter \
            -I$(SIM) -I$(INC)

installation_guide_test_FLAGS := -DINSTALLATION_GUIDE_TEST -Istorage_shim
installation_guide_test_SRCS := $(SRC)/installation_guide.cpp

research_test_SRCS := $(SRC)/location_label.cpp  $(SRC)/research_report.cpp $(SRC)/research.cpp $(SRC)/deauth_tracker.cpp $(SRC)/signatures.cpp $(SRC)/field_tools.cpp $(SRC)/remote_id.cpp

field_test_SRCS := $(SRC)/field_tools.cpp $(SRC)/remote_id.cpp

breakout_test_SRCS := $(SRC)/breakout.cpp

care_test_SRCS := $(SRC)/care.cpp $(SRC)/field_tools.cpp $(SRC)/remote_id.cpp

drone_watch_test_SRCS := $(SRC)/location_label.cpp $(SRC)/drone_watch.cpp $(SRC)/remote_id.cpp

duress_test_SRCS := $(SRC)/ui_scratch.cpp  $(SRC)/duress_core.cpp $(SRC)/pixel_tide.cpp

alert_snooze_test_SRCS := $(SRC)/alert_snooze.cpp

location_label_test_SRCS := $(SRC)/location_label.cpp

research_submission_test_FLAGS := -DRESEARCH_SUBMISSION_TEST -Istorage_shim
research_submission_test_SRCS := $(SRC)/research_submission.cpp $(SRC)/user_labels.cpp $(SRC)/user_labels_storage.cpp

card_content_test_FLAGS := -DCARD_CONTENT_TEST -Istorage_shim
card_content_test_SRCS := $(SRC)/card_content.cpp

ui_scratch_test_SRCS := $(SRC)/ui_scratch.cpp $(SRC)/pixel_tide.cpp

backup_maintenance_test_FLAGS := -DBACKUP_MAINTENANCE_TEST -Istorage_shim
backup_maintenance_test_SRCS := $(SRC)/backup_maintenance.cpp

TESTS := backup_maintenance_test ui_scratch_test card_content_test sd_retry_test research_submission_test location_label_test  installation_guide_test alert_snooze_test duress_test drone_watch_test care_test verified_copy_test breakout_test glyph_test field_test research_test deauth_tracker_test dnsp_detection_test dnsp_csv_test dnsp_settings_test remote_id_test ibeacon_test signatures_test emitter_roundtrip_test ignore_list_test hacker_test pwnagotchi_test shim_fidelity_test squachmesh_test qwerty_test meshmsg_test device_info_test security_test emote_script_test blackbox_test bingo_test hunt_test bgretire_test autoquiet_test framepush_test touchfit_test logindex_test dex_test regulars_test gnss_test user_labels_test sketchy_rule_test

dnsp_detection_test_SRCS := $(SIM)/detection_sim.cpp $(SRC)/remote_id.cpp privacy_stub.cpp $(SRC)/detection_info.cpp $(SRC)/device_info.cpp $(SRC)/signatures.cpp
deauth_tracker_test_SRCS := $(SRC)/deauth_tracker.cpp
dnsp_settings_test_SRCS := $(SRC)/settings.cpp
gnss_test_SRCS := $(SRC)/gnss.cpp
user_labels_test_SRCS := $(SRC)/user_labels.cpp
sketchy_rule_test_FLAGS := -DSKETCHY_RULE_TEST -Istorage_shim
sketchy_rule_test_SRCS := $(SRC)/location_label.cpp  $(SRC)/sketchy_rule.cpp $(SIM)/detection_sim.cpp $(SRC)/remote_id.cpp

# Only the units under test get linked in. Pulling the whole firmware
# would drag in TFT_eSPI, NimBLE and the UI, and a test that needs a
# display driver to check a coordinate is not a test anybody runs.
remote_id_test_SRCS          := $(SRC)/remote_id.cpp
ibeacon_test_SRCS            := $(SRC)/signatures.cpp
signatures_test_SRCS         := $(SRC)/signatures.cpp
emitter_roundtrip_test_SRCS  := $(SRC)/remote_id.cpp
ignore_list_test_SRCS        := $(SRC)/ignore_list.cpp
hacker_test_SRCS             := $(SRC)/signatures.cpp
pwnagotchi_test_SRCS         := $(SRC)/signatures.cpp
squachmesh_test_SRCS         := $(SRC)/squachmesh.cpp
# The codec is #if SQUACH_MESH so it stays out of every shipping board.
# The test is the one place that wants it compiled regardless of build env.
squachmesh_test_FLAGS        := -DSQUACH_MESH=1
qwerty_test_SRCS             := $(SRC)/qwerty.cpp
qwerty_test_FLAGS            := -DSQUACH_MESH=1
# The protocol without the cipher -- see the top of meshmsg_test.cpp.
meshmsg_test_SRCS            := $(SRC)/meshmsg.cpp $(SRC)/meshwords.cpp
meshmsg_test_FLAGS           := -DSQUACH_MESH=1
# The emote scripts' tables and the setup byte's arithmetic.
emote_script_test_SRCS       := $(SRC)/emote_script.cpp
emote_script_test_FLAGS      := -DSQUACH_MESH=1
# The pages against the signature tables and the emulator's profiles.
device_info_test_SRCS        := $(SRC)/device_info.cpp $(SRC)/signatures.cpp
# The lock's arithmetic, through the sim's Preferences shim.
security_test_SRCS           := $(SRC)/location_label.cpp  $(SRC)/security.cpp $(SRC)/ignore_list.cpp $(SRC)/field_tools.cpp $(SRC)/remote_id.cpp
# The rings, on the emulator's RAM flash. The two clock calls it makes are
# stubbed in the test.
blackbox_test_SRCS           := $(SRC)/location_label.cpp  $(SRC)/blackbox.cpp
blackbox_test_FLAGS          := -DBLACKBOX_TEST
# One LOG row per device, over the same RAM flash.
logindex_test_SRCS           := $(SRC)/location_label.cpp  $(SRC)/blackbox.cpp $(SRC)/log_index.cpp
# The DEX's cards and the record, through the Preferences shim; the clock is stubbed.
dex_test_SRCS                := $(SRC)/dex.cpp $(SIM)/detection_sim.cpp $(SRC)/remote_id.cpp
# The neighbours table; the clock is stubbed.
regulars_test_SRCS           := $(SRC)/regulars.cpp
# The card: winnable, and it keeps what it earned. The clock is stubbed in
# the test; the engine comes along for its lifetime counts.
bingo_test_SRCS              := $(SRC)/bingo.cpp $(SIM)/detection_sim.cpp $(SRC)/remote_id.cpp
bingo_test_FLAGS             := -DSQUACH_MESH=1
# An unfinished egg hunt, through the settings store. Theme and Clock are
# stubbed in the test; settings.cpp only calls a handful of each.
autoquiet_test_SRCS       := $(SIM)/detection_sim.cpp $(SRC)/remote_id.cpp
bgretire_test_SRCS := $(SRC)/settings.cpp
hunt_test_SRCS               := $(SRC)/settings.cpp
# Header-only, both: the colour arithmetic the overlapped frame push
# builds its table from is inline in frame_push.h, and the shim under
# test IS sim/TFT_eSPI.h.
framepush_test_SRCS          :=
shim_fidelity_test_SRCS      :=
# The touch mapping is plain arithmetic in touch_fit.h.
touchfit_test_SRCS           :=

readable_storage_test_SRCS := $(SRC)/readable_logs.cpp $(SRC)/blackbox.cpp $(SRC)/location_label.cpp $(SRC)/care.cpp $(SRC)/field_tools.cpp $(SRC)/remote_id.cpp
readable_storage_test_FLAGS := -DREADABLE_LOGS_TEST -DBLACKBOX_TEST -Istorage_shim

$(OUT)/sd_recovery_test: CXXFLAGS := -Isd_shim $(CXXFLAGS)
sd_recovery_test_SRCS := $(SRC)/sd_log.cpp
sd_recovery_test_FLAGS := -DSD_LOG_TEST -DCYD=1 -Isd_shim

TESTS += sd_recovery_test readable_storage_test backup_progress_test ble_advert_fields_test ambient_light_test crash_reports_test spam_test
crash_reports_test_SRCS := $(SRC)/crash_reports.cpp

.PHONY: all clean
all: $(addprefix run-,$(TESTS))

$(OUT):
	@mkdir -p $(OUT)

define TEST_rule
$(OUT)/$(1): $(1).cpp $(SRC)/simulation.cpp $$($(1)_SRCS) $$(wildcard $$(INC)/*.h) $$(wildcard $$(INC)/*.inc) $$(wildcard $$(SIM)/*.h) Makefile | $(OUT)
	@$$(CXX) $$(CXXFLAGS) $$($(1)_FLAGS) -o $$@ $$< $$($(1)_SRCS) $(SRC)/simulation.cpp

.PHONY: run-$(1)
run-$(1): $(OUT)/$(1)
	@./$(OUT)/$(1)
endef

$(foreach t,$(TESTS),$(eval $(call TEST_rule,$(t))))

clean:
	@rm -rf $(OUT)

# ---- the real cipher, opt-in ------------------------------------------------
# meshcrypto.cpp against the host's own mbedtls. NOT in TESTS above and not run
# by plain `make`, because it needs libmbedtls-dev and CI does not have it. A
# test that quietly skipped itself when the header was missing would report
# green for a check that never ran, which is worse than not having it.
#
#   sudo apt-get install libmbedtls-dev
#   make -C test crypto
#
# It is meaningful only for the mbedtls 3 path -- which is exactly the one the
# ESP32-C5 environment introduced and which no other test compiles. See the
# header comment in meshcrypto_test.cpp.
CRYPTO_SRCS := meshcrypto_test.cpp $(SRC)/meshcrypto.cpp $(SRC)/meshmsg.cpp $(SRC)/meshwords.cpp
CRYPTO_OBJS := $(addprefix $(OUT)/crypto/,$(notdir $(CRYPTO_SRCS:.cpp=.o)))

crypto: $(OUT)/meshcrypto_test
	@$(OUT)/meshcrypto_test

# Each source gets its OWN object and its own .d, which is the only way the
# header dependencies are correct. Compiling all four in one g++ call writes a
# single .d that every translation unit overwrites in turn, so it ends up
# listing the last file's headers and nothing else -- meshmsg_vectors.h and
# mbedtls_compat.h were both invisible that way, and both mutation tests of
# this suite passed against a stale binary until it was split up.
#
# The TEST_rule above has the same gap (it lists only .cpp prerequisites), so
# editing a header and re-running `make` re-runs the previous build there too.
# Not changed here: that would alter how every other test builds.
$(OUT)/crypto:
	@mkdir -p $(OUT)/crypto

$(OUT)/crypto/%.o: %.cpp | $(OUT)/crypto
	$(CXX) $(CXXFLAGS) -MMD -MP -DSQUACH_MESH=1 -c -o $@ $<

$(OUT)/crypto/%.o: $(SRC)/%.cpp | $(OUT)/crypto
	$(CXX) $(CXXFLAGS) -MMD -MP -DSQUACH_MESH=1 -c -o $@ $<

$(OUT)/meshcrypto_test: $(CRYPTO_OBJS)
	$(CXX) -o $@ $^ -lmbedcrypto

-include $(CRYPTO_OBJS:.o=.d)

.PHONY: crypto
