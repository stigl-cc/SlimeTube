FROM alpine:latest AS build
WORKDIR /code/

RUN echo 'https://dl-cdn.alpinelinux.org/alpine/edge/testing' >> /etc/apk/repositories && \
    echo 'https://dl-cdn.alpinelinux.org/alpine/edge/community' >> /etc/apk/repositories && \
    echo 'https://dl-cdn.alpinelinux.org/alpine/edge/main' >> /etc/apk/repositories && \
    apk add --no-cache drogon cmake g++ c-ares-dev sqlite-dev jsoncpp-dev brotli-dev openssl-dev zstd-dev util-linux-dev ffmpeg-dev
COPY . /code
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --config Release -j12 && \
    mkdir -pv /code/install && \
    cmake --install build --prefix /code/install --verbose

FROM alpine:latest AS publish
WORKDIR /

RUN
    echo 'https://dl-cdn.alpinelinux.org/alpine/edge/testing' >> /etc/apk/repositories && \
    echo 'https://dl-cdn.alpinelinux.org/alpine/edge/community' >> /etc/apk/repositories && \
    echo 'https://dl-cdn.alpinelinux.org/alpine/edge/main' >> /etc/apk/repositories && \
    apk add --no-cache c-ares sqlite jsoncpp openssl zstd libuuid ffmpeg
COPY --from=build /code/install /

EXPOSE 8080
EXPOSE 8443

CMD ["/bin/slimytube"]