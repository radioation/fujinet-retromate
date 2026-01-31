PRODUCT = retromat
# PLATFORMS = coco apple2 atari c64 adam msdos msxrom
PLATFORMS = apple2 atari c64

# You can run 'make <platform>' to build for a specific platform,
# or 'make <platform>/<target>' for a platform-specific target.
# Example shortcuts:
#   make coco        → build for coco
#   make apple2/disk → build the 'disk' target for apple2

# SRC_DIRS may use the literal %PLATFORM% token.
# It expands to the chosen PLATFORM plus any of its combos.
SRC_DIRS = src src/%PLATFORM%

# FUJINET_LIB can be
# - a version number such as 4.7.6
# - a directory which contains the libs for each platform
# - a zip file with an archived fujinet-lib
# - a URL to a git repo
# - empty which will use whatever is the latest
# - undefined, no fujinet-lib will be used
FUJINET_LIB = 4.8.3

# Define extra dirs ("combos") that expand with a platform.
# Format: platform+=combo1,combo2
PLATFORM_COMBOS = \
  c64+=commodore \
  atarixe+=atari \
  atarixl+=atari \
  msxrom+=msx \
  msxdos+=msx

include makefiles/toplevel-rules.mk

# If you need to add extra platform-specific steps, do it below:
#   coco/r2r:: coco/custom-step1
#   coco/r2r:: coco/custom-step2
# or
#   apple2/disk: apple2/custom-step1 apple2/custom-step2

LDFLAGS_EXTRA_ATARI = -C src/atari/atari.cfg --mapfile ./fnrm_atari.map -Ln ./fnrm_atari.lbl --debug-info  -Wl -D__SYSTEM_CHECK__=1
LDFLAGS_EXTRA_APPLE2 = -C apple2-hgr.cfg  --mapfile ./fnrm_apple.map -Ln ./fnrm_apple.lbl  -Wl -D,__HIMEM__=0xBF00
LDFLAGS_EXTRA_C64 =  --mapfile ./fnrm_c64.map -Ln ./fnrm_c64.lbl  -Wl -D,__HIMEM__=0xC000
