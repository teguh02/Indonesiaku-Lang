# ===============================
# Dockerfile - Indonesiaku Image
# ===============================
FROM alpine:3.20 AS build

# Build tools (build-base = gcc + make + musl-dev).
RUN apk add --no-cache build-base

# Copy sources needed to build.
WORKDIR /opt/indonesiaku
COPY src ./src
COPY Makefile .

# Build the interpreter.
RUN make clean && make && strip indk

# ---- Runtime image (small) ----
FROM alpine:3.20

LABEL maintainer="Teguh Rijanandi <teguhrijanandi02@gmail.com>"
LABEL description="Docker image for Bahasa Pemrograman Indonesiaku (Python-like language built in C)"
LABEL version="1.7.0"

# musl provides the C runtime + sockets; no extra packages needed to run.
WORKDIR /opt/indonesiaku

# Interpreter binary.
COPY --from=build /opt/indonesiaku/indk /usr/local/bin/indk

# Standard library at the conventional PREFIX/share location: the
# interpreter at /usr/local/bin finds /usr/local/share/indonesiaku/pustaka by
# itself, so `impor pola` works from ANY working directory (docker run -w
# /app ...). /opt/indonesiaku/pustaka stays as a symlink for `impor
# "pustaka/..."` from this WORKDIR and for the playground's PUSTAKA_DIR.
COPY pustaka /usr/local/share/indonesiaku/pustaka
RUN ln -s /usr/local/share/indonesiaku/pustaka ./pustaka
COPY examples ./examples

# Default command.
ENTRYPOINT ["indk"]
CMD ["--help"]

# ===============================
# End of Dockerfile
# ===============================
