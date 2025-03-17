ARG PLATFORM=${BUILDPLATFORM:-linux/amd64}
ARG IMAGE=debian:bookworm
ARG ENABLE_VALGRIND=1

FROM --platform=${PLATFORM} ${IMAGE}

# Install development dependencies
RUN if test -f "/etc/debian_version"; then \
      apt-get update \
 &&   apt-get install -y "git" "ssh" "xz-utils" "gcc" "g++" "clang-16" "clang++-16" \
                         "cmake" "make" "lcov" "gdb" "libfec-dev" \
 &&   update-alternatives --install "/usr/bin/clang" "clang" "$(which clang-16)" 100 \
 &&   update-alternatives --install "/usr/bin/clang++" "clang++" "$(which clang++-16)" 100; \
    elif test -f "/etc/alpine-release"; then \
      apk add --no-cache "git" "openssh" "xz" "gcc" "g++" "clang" "cmake" \
                         "make" "lcov" "gdb"; \
    else \
      echo "Unsupported distribution" && exit 1; \
    fi

ARG ENABLE_VALGRIND

# Build and install valgrind
# - Debian 12 (Bookworm) has valgrind 3.19.0, which is too old for AArch64 support
COPY "third_party/valgrind" "/valgrind"
RUN if test "${ENABLE_VALGRIND}" != "0"; then \
      if test -f "/etc/debian_version"; then \
        apt-get update \
 &&     apt-get install -y "build-essential" "automake" "autoconf" "git" "libc6-dbg"; \
      elif test -f "/etc/alpine-release"; then \
        apk add --no-cache "build-base" "automake" "autoconf" "git" "linux-headers" "perl" "bash"; \
      else \
        echo "Unsupported distribution" && exit 1; \
      fi \
 &&   cd "/valgrind" \
 &&     ./autogen.sh \
 &&     ./configure \
 &&     make -j"$(nproc)" \
 &&     make install \
 &&   cd -; \
    fi

# Copy the project files
COPY . "/libpoporon"

WORKDIR "/libpoporon"

ENV CTEST_OUTPUT_ON_FAILURE=1

CMD [ "/libpoporon/container_ci.sh" ]
