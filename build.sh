#!/bin/bash


# Variables
DIR=`readlink -f .`;
PARENT_DIR=`readlink -f ${DIR}/..`;

DEFCONFIG_NAME=wirus_defconfig
CHIPSET_NAME=kona
VARIANT=r8q
ARCH=arm64
VERSION=Nethunter_${VARIANT}_v0.0


BUILD_CROSS_COMPILE=$PARENT_DIR/toolchains/aarch64-linux-android-4.9/bin/aarch64-linux-android-
KERNEL_LLVM_BIN=$PARENT_DIR/toolchains/llvm-arm-toolchain-ship_8.0.6/bin/clang
CLANG_TRIPLE=aarch64-linux-gnu-
KERNEL_MAKE_ENV="DTC_EXT=$(pwd)/tools/dtc CONFIG_BUILD_ARM64_DT_OVERLAY=y LOCALVERSION=-${VERSION}"

DTS_DIR=$PARENT_DIR/out/arch/$ARCH/boot/dts

#Compile kernel:
[ ! -d "$PARENT_DIR/out" ] && mkdir $PARENT_DIR/out
  make -j$(nproc) -C $(pwd) O=$PARENT_DIR/out $KERNEL_MAKE_ENV ARCH=arm64 CROSS_COMPILE=$BUILD_CROSS_COMPILE REAL_CC=$KERNEL_LLVM_BIN CLANG_TRIPLE=$CLANG_TRIPLE CFP_CC=$KERNEL_LLVM_BIN $DEFCONFIG_NAME
  make -j$(nproc) -C $(pwd) O=$PARENT_DIR/out $KERNEL_MAKE_ENV ARCH=arm64 CROSS_COMPILE=$BUILD_CROSS_COMPILE REAL_CC=$KERNEL_LLVM_BIN CLANG_TRIPLE=$CLANG_TRIPLE CFP_CC=$KERNEL_LLVM_BIN

#Generate boot.img:
 [ -e $PARENT_DIR/out/arch/arm64/boot/Image.gz ] && cp $PARENT_DIR/out/arch/arm64/boot/Image.gz $PARENT_DIR/out/Image.gz
  if [ -e $PARENT_DIR/out/arch/arm64/boot/Image.gz-dtb ]; then
    cp $PARENT_DIR/out/arch/arm64/boot/Image.gz-dtb $PARENT_DIR/out/Image.gz-dtb

    DTBO_FILES=$(find ${DTS_DIR}/samsung/ -name ${CHIPSET_NAME}-sec-${VARIANT}-*-r*.dtbo)
    cat ${DTS_DIR}/vendor/qcom/*.dtb > $PARENT_DIR/out/dtb.img
    $(pwd)/tools/mkdtimg create $PARENT_DIR/out/dtbo.img --page_size=4096 ${DTBO_FILES}
fi

#Create flashable zip:
  if [ ! -d $PARENT_DIR/AnyKernel3 ]; then
    echo "clone AnyKernel3 - Flashable Zip Template"
    git clone https://github.com/osm0sis/AnyKernel3 $PARENT_DIR/AnyKernel3
  fi

  [ -e $PARENT_DIR/${VERSION}.zip ] && rm $PARENT_DIR/${VERSION}.zip
  if [ -e $PARENT_DIR/out/arch/arm64/boot/Image.gz-dtb ]; then
    cp $PARENT_DIR/out/arch/arm64/boot/Image.gz-dtb $PARENT_DIR/AnyKernel3/zImage
  elif [ -e $PARENT_DIR/out/arch/arm64/boot/Image.gz ]; then
    cp $PARENT_DIR/out/arch/arm64/boot/Image.gz $PARENT_DIR/AnyKernel3/zImage
  else
    echo "Error"
  fi
  cd $PARENT_DIR/AnyKernel3
  git reset --hard
  sed -i "s/ExampleKernel by osm0sis/${VARIANT} kernel by Svirusx/g" anykernel.sh
  sed -i "s/=maguro/=${VARIANT}/g" anykernel.sh
  sed -i "s/=toroplus/=/g" anykernel.sh
  sed -i "s/=toro/=/g" anykernel.sh
  sed -i "s/=tuna/=/g" anykernel.sh
  sed -i "s/omap\/omap_hsmmc\.0\/by-name\/boot/soc\/1d84000\.ufshc\/by-name\/boot/g" anykernel.sh
  sed -i "s/backup_file/#backup_file/g" anykernel.sh
  sed -i "s/replace_string/#replace_string/g" anykernel.sh
  sed -i "s/insert_line/#insert_line/g" anykernel.sh
  sed -i "s/append_file/#append_file/g" anykernel.sh
  sed -i "s/patch_fstab/#patch_fstab/g" anykernel.sh

  mkdir -p $PARENT_DIR/build/$VARIANT/modules
  zip -r9 $PARENT_DIR/build/$VARIANT/${VERSION}.zip * -x .git README.md *placeholder
  cd $DIR

find $PARENT_DIR/out/ -name '*.ko'  -not -path "$PARENT_DIR/build/*" -exec cp --parents -f '{}' $PARENT_DIR/build/$VARIANT/modules  \;
mv -f $PARENT_DIR/build/$VARIANT/modules/home/svirusx/out/* $PARENT_DIR/build/$VARIANT/modules




