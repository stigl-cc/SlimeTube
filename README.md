## SlimeTube
<sub>(or SlimyTube? SlimeyTube? I don't even know anymore)</sub>

Is a flexible self-hosted C++ video platform for user videos (ok, let's face it, its just a youtube clone).

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

### Configuration
For example values, see [data/config.json](./data/config.json)  
The settings inside "listeners", "db_clients", "app", "ssl" and "plugins" are drogon-specific - see [drogon docs](https://drogonframework.github.io/drogon-docs/#/ENG/ENG-11-Configuration-File) for more details for those.  
The setting "custom_config" contains all settings for this app specifically.  

- `title` - Title of the app (shows up in the website title and server response header)
- `videos_dir` - directory for uploaded (and encoded) videos
- `thumbnails_dir` - directory for video thumbnails
- `blobs_dir` - directory for all videos that have not yet been encoded
- `salt_length` - length of user password salts(recommended atleast  16)
- `ffmpeg_log` - directory where the log for ffmpeg output should go
- `ffmpeg_log_level` - how much ffmpeg should log
- `limits` - contains various limits on what the users are allowed to do on the platform
- `limits` - `max_blob_size` - how large of a file can a user submit to be encoded
- `limits` - `total_user_size` - how much space is a user allowed to use
- `limits` - `max_get_video_range_size` - maximum amount of video data the server will allow user to download with a http range request
- `limits` - `max_unlisted_bitrate` - maximum bitrate an uploaded video will be encoded to
- `limits` - `max_aspect_ratio` - maximum width/height multiplier for video to be encoded
- `limits` - `max_input_video_width` - maximum width for a video to be encoded
- `limits` - `max_input_video_height` - maximum height for a video to be encoded
- `limits` - `max_encode_video_width` - maximum width for a video to be encoded to
- `limits` - `max_encode_video_height` - maximum width for a video to be encoded to
- `resolutions` - list of resolutions natively accepted by the encoder in format: `[[ratio_width, ratio_height], width, height, video_bitrate, [also]]` also: heights of other resolutions with the same ratio (defined below this one) that will also be encoded with this one - example: `[[16, 9], 1920, 1080, 3150000, [720]]` - where 1280x720 must be defined below this line
- `allow_unlisted_ratios` - whether to allow encoding of ratios not in the `resolutions` list
- `encoder_frame_rate` - what frame rate to encode all videos into
- `audio_bitrate` - what bitrate to use for videos
- `audio_sample_rate` - what sample rate to use for videos
