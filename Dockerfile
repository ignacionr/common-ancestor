FROM ubuntu
ENV DEBIAN_FRONTEND="noninteractive" TZ="America/Argentina/Buenos_Aires"
RUN apt-get update -y && apt-get install curl clang-11 libsqlite3-dev git cmake -y
RUN ln -s /usr/bin/clang++-11 /usr/bin/clang++
WORKDIR /app
COPY . .
RUN make test
EXPOSE 8080
ENTRYPOINT [ "build/common-ancestor" ]
