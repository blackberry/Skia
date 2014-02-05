function exportVar {
  NAME=$1
  VALUE=$2
  export $NAME="$VALUE"
}

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

exportVar CC "${QNX_HOST}/usr/bin/qcc -Vgcc_ntoarmv7le_cpp-ne"
exportVar CFLAGS "-I${QNX_TARGET}/usr/include"
exportVar CXX "${CC}"
exportVar CXXFLAGS "-I${QNX_TARGET}/usr/include"
exportVar LINK "${CC}"
exportVar AR "${QNX_HOST}/usr/bin/ntoarm-ar"
# Helper function to configure the GYP defines to the appropriate values
# based on the target device.
setup_device() {
  DEFINES="OS=qnx"
  DEFINES="${DEFINES} host_os=$(uname -s | sed -e 's/Linux/linux/;s/Darwin/mac/')"
  DEFINES="${DEFINES} skia_os=qnx skia_angle=0 skia_distancefield_fonts=1"
#  DEFINES="${DEFINES} android_base=${SCRIPT_DIR}/.."
#  DEFINES="${DEFINES} android_toolchain=${TOOLCHAIN_TYPE}"

  # Setup the build variation depending on the target device
  TARGET_DEVICE="$1"

  if [ -z "$TARGET_DEVICE" ]; then
    echo "INFO: no target device type was specified so using the default 'arm_v7'"
    TARGET_DEVICE="arm_v7"
  fi

  case $TARGET_DEVICE in
    nexus_s)
        DEFINES="${DEFINES} skia_arch_type=arm arm_neon=1 arm_version=7 arm_thumb=0"
        DEFINES="${DEFINES} skia_resource_cache_mb_limit=24"
        ;;
    nexus_4 | nexus_7 | nexus_10)
        DEFINES="${DEFINES} skia_arch_type=arm arm_neon=1 arm_version=7 arm_thumb=0"
        ;;
    xoom)
        DEFINES="${DEFINES} skia_arch_type=arm arm_neon=0 arm_version=7 arm_thumb=0"
        ;;
    galaxy_nexus)
        DEFINES="${DEFINES} skia_arch_type=arm arm_neon=1 arm_version=7 arm_thumb=0"
        DEFINES="${DEFINES} skia_resource_cache_mb_limit=32"
        ;;
    razr_i)
        DEFINES="${DEFINES} skia_arch_type=x86 skia_arch_width=32"
        DEFINES="${DEFINES} skia_resource_cache_mb_limit=32"
        ;;
    arm_v7)
        DEFINES="${DEFINES} skia_arch_type=arm arm_neon=1 arm_version=7 arm_thumb=0"
        ;;
    arm_v7_thumb)
        DEFINES="${DEFINES} skia_arch_type=arm arm_neon_optional=1 arm_version=7 arm_thumb=1"
        ;;
    arm)
        DEFINES="${DEFINES} skia_arch_type=arm arm_neon=0 arm_version=0 arm_thumb=0"
        ;;
    arm_thumb)
        DEFINES="${DEFINES} skia_arch_type=arm arm_neon=0 arm_version=0 arm_thumb=1"
        ;;
    x86)
        DEFINES="${DEFINES} skia_arch_type=x86 skia_arch_width=32"
        DEFINES="${DEFINES} skia_resource_cache_mb_limit=32"
        ;;
    *)
        echo -n "ERROR: unknown device specified ($TARGET_DEVICE), valid values: "
        echo "nexus_[s,4,7,10] xoom galaxy_nexus arm arm_thumb arm_v7 arm_v7_thumb x86"
        return 1;
        ;;
  esac

  echo "The build is targeting the device: $TARGET_DEVICE"

  exportVar GYP_DEFINES "$DEFINES"
  exportVar SKIA_OUT "out/qnx"
}

# Run the setup device command initially as a convenience for the user
setup_device
#echo "** The device has been setup for you by default. If you would like to **"
#echo "** use a different device then run the setup_device function with the **"
#echo "** appropriate input.                                                 **"

# Use the "android" flavor of the Makefile generator for both Linux and OS X.
exportVar GYP_GENERATORS "ninja-blackberry"

# Helper function so that when we run "make" to build for clank it exports
# the toolchain variables to make.
#make_android() {
#  CC="$CROSS_CC" CXX="$CROSS_CXX" LINK="$CROSS_LINK" \
#  AR="$CROSS_AR" RANLIB="$CROSS_RANLIB" \
#    command make $*
#}
