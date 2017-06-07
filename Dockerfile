FROM yastdevel/cpp:sle12-sp2
RUN zypper --gpg-auto-import-keys --non-interactive in --no-recommends \
  libzypp-devel
COPY . /usr/src/app
