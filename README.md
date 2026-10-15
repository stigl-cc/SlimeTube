## SlimeTube
<sub>(or SlimyTube? I don't know anymore)</sub>

Is a lexible self-hosted C++ video platform for user videos (ok, let's face it, its just a youtube clone).

### How to run

#### docker/podman
1) clone the repository **with --recursive**
2) `docker build --rm -t slimetube:latest .`
3) Place your certificates inside data/certs/ (server.crt and server.key) or disable https by removing it in data/config.json
4) Review `data/config.json` and make sure it matches your settings
6) To run: `docker run -it -d -p 8443:8443 -p 8080:8080 -v ./data:/data --name slimetube slimetube:latest`
   if you're using SELinux, you might need to add :z/:Z after the :/data (z - all containers can access it, Z - only this container can)

#### normally
1) clone the repository **with --recursive**
2) cd into the repository
3) install the following dependencies: `cmake make g++ c-ares-dev sqlite-dev jsoncpp-dev brotli-dev openssl-dev zlib-dev zstd-dev libuuid-dev ffmpeg-dev drogon` (drogon for drogon_ctl - can be installed using some package manager, you can also build from [source](https://drogon.org/))
4) `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
5) `cmake --build build --config Release -j<number-of-threads>` (replace the <number-of-threads> with the number of threads your CPU contains - to find out on linux `nproc --all`)
6) To run: `./build/slimytube`
