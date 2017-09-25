FROM yastdevel/cpp
RUN zypper --gpg-auto-import-keys --non-interactive in --no-recommends \
  libzypp-devel yast2-ruby-bindings
COPY . /usr/src/app

