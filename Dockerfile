FROM yastdevel/cpp:sle15-sp1
RUN zypper --gpg-auto-import-keys --non-interactive in --no-recommends \
  libzypp-devel yast2-ruby-bindings iproute2
COPY . /usr/src/app

