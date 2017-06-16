FROM yastdevel/cpp:sle12-sp3
RUN zypper --gpg-auto-import-keys --non-interactive in --no-recommends \
  libzypp-devel
COPY . /usr/src/app

