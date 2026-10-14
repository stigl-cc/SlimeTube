const upload_progress = document.getElementById("upload-progress");
const video_upload = document.getElementById("video-upload");
const chunk_size = 1024 * 1024;

async function uploadVideo(id) {
    const video_file = video_upload.files[0],
          video_size = video_file.size;

    console.log(`Sending file with ${video_size}B with ${chunk_size}B chunks.`);
    var attempts = 0;
    chunkLoop: for(let offset = 0; offset < video_size;) {
        const video_chunk = video_file.slice(offset, offset + chunk_size);
        console.log(`Chunk size: ${video_chunk.size} at offset ${offset}`);
        console.log(typeof(offset));

        try {
            const result = await fetch(`/rsc/video/${id}`, {
                method: "PUT",
                body: video_chunk,
                headers: {
                    "X-Offset" : offset,
                    "X-Last" : (offset >= video_size - chunk_size) ? "true" : "false",
                }
            });

            // Handle errors
            switch(result.status) {
            case 202:
                if(result.headers.has("X-Position")) {
                    offset = Number(result.headers.get("X-Position"));
                } else {
                    offset += chunk_size;
                }
                console.log(`offset after upload: ${offset}`);
                break;
            case 400: // id empty or X-Offset is NaN
                break chunkLoop;
            case 401:
                break chunkLoop;
            case 403: // user doesn't own video
                break chunkLoop;
            case 404:
                break chunkLoop;
            case 409: // X-Offset too small
            case 424: // X-Offset too large
                if(result.headers.has("X-Position")) {
                    offset = Number(result.headers.get("X-Position"));
                    continue chunkLoop;
                }
            case 500:
                if(attempts++ < 3) { // re-try then exit
                    continue chunkLoop;
                }
                break chunkLoop;
            }
        } catch(error) {
            console.error("Error message: " + error.message);
        }

        upload_progress.setAttribute("value", (offset + 1) / video_size * 100);
    }
}
