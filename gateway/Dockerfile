FROM debian:bookworm-slim

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        g++ \
        make \
        cmake \
        libpistache-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN cmake -S gateway -B build/gateway && cmake --build build/gateway

EXPOSE 8080 8081

CMD ["/app/build/gateway/gateway"]
