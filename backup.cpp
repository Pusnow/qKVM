
// Create a widget to serve as the main window
QWidget window;
window.setWindowTitle("Qt6 Webcam View");
QVBoxLayout layout(&window);

// QLabel to display video frames
QLabel videoLabel;
videoLabel.setMinimumSize(640, 480);
layout.addWidget(&videoLabel);

// Create a QCamera object for the default camera
QCamera camera;

// Create a media capture session and set the camera
QMediaCaptureSession captureSession;
captureSession.setCamera(&camera);

// Create a QVideoSink to receive video frames
QVideoSink videoSink;
captureSession.setVideoSink(&videoSink);

// Connect to the videoFrameChanged signal to update the video label
QObject::connect(&videoSink, &QVideoSink::videoFrameChanged,
                 [&](const QVideoFrame &frame) {
                   if (frame.isValid()) {
                     // Convert the video frame to a QImage. Note: this
                     // conversion may require adjustments depending on your
                     // platform and frame format.
                     QImage image = frame.toImage();
                     if (!image.isNull()) {
                       videoLabel.setPixmap(QPixmap::fromImage(image));
                     }
                   }
                 });

// Start the camera
camera.start();

window.show();
return app.exec();