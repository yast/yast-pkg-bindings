#! /bin/sh

# exit on error immediately
set -e -x

# install the built package
# Note: use zypper if the dependencies are really required:
# zypper --non-interactive in --no-recommends /usr/src/packages/RPMS/*/*.rpm
rpm -iv --force --nodeps /usr/src/packages/RPMS/*/*.rpm

# clear the log if present, it will be checked later
rm -f /var/log/YaST2/y2log

# disable the OSS repository, it contains over 60k packages and that slows down
# some Pkg tests a lot
zypper mr -d repo-oss
