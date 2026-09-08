function change_playback_speed() {
    const playback_speed = document.getElementById("speed-selector").value;
    console.log(`Changing playback speed to ${playback_speed}.`);
    document.getElementById('video-element').playbackRate = playback_speed;

    if ('URLSearchParams' in window) {
        const url = new URL(window.location)
        url.searchParams.set("speed", playback_speed)
        history.pushState(null, '', url);
    }
}

function change_playback_quality() {
    const playback_quality = document.getElementById("quality-selector").value;
    console.log(`Changing playback quality to ${playback_quality}.`);

    if ('URLSearchParams' in window) {
        var searchParams = new URLSearchParams(window.location.search);
        searchParams.set("quality", playback_quality);
        window.location.search = searchParams.toString();
    }
}

function on_keypress(event) {
    if (event.altKey || event.ctrlKey || event.shiftKey || event.metaKey)
        return;

    const video_element = document.getElementById('video-element');
    if(video_element) {
        if(event.code === "Space") {
            if(video_element.paused) {
                video_element.play();
            } else {
                video_element.pause();
            }
            event.preventDefault();
        }

        if(event.code === "ArrowLeft") {
            video_element.currentTime -= 1.00;
            event.preventDefault();
        } else if(event.code === "ArrowRight") {
            video_element.currentTime += 1.00;
            event.preventDefault();
        }
    }
}

addEventListener("keydown", on_keypress);
