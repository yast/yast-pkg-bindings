#! /bin/bash

# helper script for publishing generated documentation to surge.sh
# Manual uploading require credentials, so only travis can do this.
# ENV SURGE_LOGIN and SURGE_TOKEN si set on travis only.
# For more info see https://surge.sh/ and especially integration of travis
# and surge https://surge.sh/help/integrating-with-travis-ci

if [ "$TRAVIS_EVENT_TYPE" = "push" -a "$TRAVIS_BRANCH" = "master" ]; then
  docker run -it -e TRAVIS=1 -e TRAVIS_JOB_ID="$TRAVIS_JOB_ID" \
    -e SURGE_LOGIN="$SURGE_LOGIN" -e SURGE_TOKEN="$SURGE_TOKEN" \
    yast-ycp-ui-bindings-image \
    /bin/bash -c \
    "zypper  in -y npm doxygen; npm install --global surge; rake doc;
    surge --project ./autodocs/html --domain yast-pkg-bindings.surge.sh"
else
  echo "not published"
fi
