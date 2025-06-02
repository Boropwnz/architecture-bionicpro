FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    qt6-base-dev \
    qt6-declarative-dev \
    qt6-tools-dev-tools \
    qt6-httpserver-dev \
    qt6-websockets-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN mkdir build && cd build && \
    cmake \
    --debug-output \
    --trace-expand \
    -DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
    .. > cmake_log.txt 2>&1 && \
    cat cmake_log.txt && \
    cmake --build . --verbose > build_log.txt 2>&1 && \
    cat build_log.txt

FROM ubuntu:24.04

RUN apt-get update && apt-get install -y \
    libqt6core6 \
    libqt6network6 \
    libqt6httpserver6 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/api .

EXPOSE 8000
CMD ["./api"]