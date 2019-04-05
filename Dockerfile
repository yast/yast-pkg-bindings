FROM registry.opensuse.org/yast/head/containers/yast-cpp:latest
RUN zypper --gpg-auto-import-keys --non-interactive in --no-recommends \
  libzypp-devel yast2-ruby-bindings iproute2
COPY . /usr/src/app

