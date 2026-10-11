#!/bin/bash
RELEASES=`lsb_release -is`
if [ "$RELEASES" == "CentOS" ]; then
  ADDITIONAL_LIBS="/usr/local/lib64/libstdc++.a /usr/local/lib/libcryptopp.a"
  echo "Compiling for Centos"
  else
  ADDITIONAL_LIBS="/usr/lib/libcrypto++.a"
  echo "Compiling for Debian"
fi
BUILD_NAME=$1

if [ -z "$BUILD_NAME" ]
then
    BUILD_NAME="statistics"
fi
echo "Starting Time:" `date`

g++ -std=c++11 \
main.cpp \
argvparser.cpp \
-o $BUILD_NAME \
-L/usr/lib \
/usr/local/src/sources/fpanel/statistics/GZSTREAM/gzstream/libgzstream.a \
-Wl,--whole-archive -lz -ldl -lpthread -Wl,--no-whole-archive \
-I /usr/include/boost/ \
-I /usr/include/ \
/usr/local/lib/libboost_system.a \
/usr/local/lib/libboost_program_options.a \
/usr/local/lib/libboost_filesystem.a \
/usr/local/lib/libboost_thread.a \
/usr/local/lib/libboost_regex.a \
/usr/local/lib/libboost_date_time.a \
$ADDITIONAL_LIBS \
-g3 \
-ggdb \
-O3 \
-Wall \
-Wunused \
-Wparentheses \
-Wextra \
-Wno-unused \
-Wno-missing-field-initializers \
-Wno-unused-parameter \
-Wsign-compare
RETVAL=$?
if [ "$RETVAL" == "0" ]; then
       cp -rP $BUILD_NAME /usr/local/fpanel/core/sbin/$BUILD_NAME
       if [ "$RELEASES" == "CentOS" ]; then
              cp -rP $BUILD_NAME /root/rpmbuild/SOURCES/panel-core-1.0/usr/local/fpanel/core/sbin/$BUILD_NAME
       else
              cp -rP $BUILD_NAME /root/panel/packages/panel-core/usr/local/fpanel/core/sbin/$BUILD_NAME
       fi
       sshpass -p "a" scp $BUILD_NAME root@192.168.139.42:/usr/local/fpanel/core/sbin/$BUILD_NAME
fi
echo "Finished Time:" `date`
