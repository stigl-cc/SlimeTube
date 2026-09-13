const speed_select = document.getElementById("speed-selector");
const quality_select = document.getElementById("quality-selector");

const searchParams = new URLSearchParams(window.location.search);

if(searchParams.has("quality")) {
    const quality = searchParams.get("quality");
    for(let i =0; i < quality_select.length; ++i)
        if(quality_select.options[i].value == quality)
            quality_select.value = quality;
}

if(searchParams.has("speed")) {
    const speed = searchParams.get("speed");
    for(let i =0; i < speed_select.length; ++i)
        if(speed_select.options[i].value == speed)
            speed_select.value = speed;
}

function change_playback_speed() {
    const playback_speed = speed_select.value;
    console.log(`Changing playback speed to ${playback_speed}.`);
    document.getElementById('video-element').playbackRate = playback_speed;

    if ('URLSearchParams' in window) {
        const url = new URL(window.location)
        url.searchParams.set("speed", playback_speed)
        history.pushState(null, '', url);
    }
}

function change_playback_quality() {
    const playback_quality = quality_select.value;
    console.log(`Changing playback quality to ${playback_quality}.`);

    if ('URLSearchParams' in window) {
        const searchParams = new URLSearchParams(window.location.search);
        searchParams.set("quality", playback_quality);
        window.location.search = searchParams.toString(); // force refresh
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

        if(event.code === "KeyF") {
            if (!document.fullscreenElement) {
                video_element.requestFullscreen();
            } else {
                document.exitFullscreen();
            }
            event.preventDefault();
        }
    }
}

addEventListener("keydown", on_keypress);
