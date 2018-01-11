var player;

$(function() {

player = videojs("myVideo", {
    controls: true,
    width: 1080,
    height: 810,
    fluid: true,
    controlBar: {
        volumePanel: false
    },
    plugins: {
        record: {
        //   image: true,
        //   debug: true
            video: {
                // video constraints: set resolution of camera
                mandatory: {
                    minWidth: 1080,
                    minHeight: 810,
                },
            },
            frameWidth: 1280,
            frameHeight: 720,
            audio: false,
            video: true,
            maxLength: 60,
            debug: true,
            videoMimeType: "video/webm; codecs=H264"//"video/mp4"//"video/webm;codecs=h264"
        }
    }
}, function(){
    // print version information at startup
    videojs.log('Using video.js', videojs.VERSION,
        'with videojs-record', videojs.getPluginVersion('record'),
        'and recordrtc', RecordRTC.version);
});

// error handling
player.on('deviceError', function() {
    console.warn('device error:', player.deviceErrorCode);
});

player.on('error', function(error) {
    console.log('error:', error);
});

// monitor stream data during recording
player.on('timestamp', function() {
    // timestamps
    console.log('current timestamp: ', player.currentTimestamp);
    console.log('all timestamps: ', player.allTimestamps);

    // stream data
    console.log('array of blobs: ', player.recordedData);
    // or construct a single blob:
    // var blob = new Blob(blobs, {
    //     type: 'video/webm'
    // });
});

// user clicked the record button and started recording
player.on('startRecord', function() {
    console.log('started recording!');
});

// user completed recording and stream is available
player.on('finishRecord', function() {
    // the blob object contains the recorded data that
    // can be downloaded by the user, stored on server etc.
    console.log('finished recording: ', player.recordedData);
    player.record().saveAs({'video': 'my-video-file-name.webm'});
    // player.record().saveAs({'image':'myImage.jpg'})
    // zip.file("image1.jpg",player.recordedData,{binary: true})
});

});