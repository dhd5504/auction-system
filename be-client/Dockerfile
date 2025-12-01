FROM debian:bookworm-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        g++ \
        make \
        cmake \
        libsqlite3-dev \
        libpistache-dev \
        libssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN mkdir -p build && cd build && cmake .. && make

EXPOSE 8080

CMD ["/app/build/be-client"]
