FROM alpine:latest

RUN apk add --no-cache bash git
RUN apk add --no-cache autoconf automake make binutils cmake libtool
RUN apk add --no-cache gcc g++
RUN apk add --no-cache libunwind linux-headers

WORKDIR /
COPY . .

RUN ls -altr && cmake -DAL_WITH_TESTS=OFF -DAL_WITH_MAIN=ON -B docker-build && \
    cmake --build docker-build  && \
    cmake --build docker-build --target main


CMD ["/docker-build/main/alcache"]
