// var zip = new JSZip();

$(function() {

//   zip.file("Hello.txt", "Hello world\n");

//   var video = document.getElementById('myVideo');
//   var canvas = document.getElementById('cnvs');
//   var ctx = canvas.getContext('2d');
//   var localMediaStream = null;

//   var errorCallback = function(e) {
//     console.log('Reeeejected!', e);
//   };

//   function snapshot() {
//     if (localMediaStream) {
//       ctx.drawImage(video, 0, 0);
//       // "image/webp" works in Chrome.
//       // Other browsers will fall back to image/png.
//       document.getElementById('snap').src = canvas.toDataURL('image/webp');
//       console.log(canvas.toDataURL('image/webp'))
//       zip.file("image1.png",canvas.toDataURL('image/webp'),{binary: true});
//     }
//   }

//   video.addEventListener('click', snapshot, false);

//   // Not showing vendor prefixes or code that works cross-browser.
//   navigator.getUserMedia({video: true}, function(stream) {
//     video.src = window.URL.createObjectURL(stream);
//     localMediaStream = stream;
//   }, errorCallback);


// zip.file("Hello.txt", "Hello world\n");

// jQuery("#blob").on("click", function () {
//     zip.generateAsync({type:"blob"}).then(function (blob) { // 1) generate the zip file
//         saveAs(blob, "hello.zip");                          // 2) trigger the download
//     }, function (err) {
//         jQuery("#blob").text(err);
//     });
// });

// });



  // var videoElement = document.getElementById('myVideo');
  // var videoSelect = document.getElementById("videoSource");

  // navigator.mediaDevices.enumerateDevices()
  //   .then(gotDevices).then(getStream).catch(handleError);

  // videoSelect.onchange = getStream;

  // function gotDevices(deviceInfos) {
  //   for (var i = 0; i !== deviceInfos.length; ++i) {
  //     var deviceInfo = deviceInfos[i];
  //     var option = document.createElement('option');
  //     option.value = deviceInfo.deviceId;
  //     if (deviceInfo.kind === 'videoinput') {
  //       option.text = deviceInfo.label || 'camera ' +
  //         (videoSelect.length + 1);
  //       videoSelect.appendChild(option);
  //     } else {
  //       // console.log('Found one other kind of source/device: ', deviceInfo);
  //     }
  //   }
  // }

  // function getStream() {
  //   if (window.stream) {
  //     window.stream.getTracks().forEach(function(track) {
  //       track.stop();
  //     });
  //   }

  //   var constraints = {
  //     video: {
  //       deviceId: {exact: videoSelect.value}
  //     }
  //   };

  //   navigator.mediaDevices.getUserMedia(constraints).
  //     then(gotStream).catch(handleError);
  // }

  // function gotStream(stream) {
  //   window.stream = stream; // make stream available to console
  //   videoElement.srcObject = stream;
  // }

  // function handleError(error) {
  //   console.log('Error: ', error);
  // }

 

  var player = videojs("myVideo", {
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
      player.record().saveAs({'video': 'my-video-file-name.mp4'});
      // player.record().saveAs({'image':'myImage.jpg'})
      // zip.file("image1.jpg",player.recordedData,{binary: true})
  });

  // jQuery("#data_uri").on("click", function () {
  //     zip.generateAsync({type:"base64"}).then(function (base64) {
  //         window.location = "data:application/zip;base64," + base64;
  //     }, function (err) {
  //         jQuery("#data_uri").text(err);
  //     });
  // });

});