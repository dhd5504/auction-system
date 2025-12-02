FROM debian:bookworm-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        g++ \
        make \
        cmake \
        libsqlite3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN cmake -S tcp-server -B build/tcp-server && cmake --build build/tcp-server

EXPOSE 9000

CMD ["/app/build/tcp-server/tcp_server"]
